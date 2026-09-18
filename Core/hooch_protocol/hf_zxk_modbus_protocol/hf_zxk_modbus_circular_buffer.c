#include "hf_zxk_modbus_circular_buffer.h"
#include "hooch_protocol_common.h"

#if HOOCH_PROTOCOL_ENABLE_HF_WIRED_CONTROLLER_485

extern UART_HandleTypeDef huart2;

/* 主循环连续空转多少次未收到新字节即认为半包超时复位解析器
 * (MODBUS-RTU 帧边界为 3.5 个字符静默，9600bps 约 4ms，主循环节拍远大于此) */
#define HF_ZXK_MODBUS_FRAME_PARSER_TIMEOUT_TICKS 5U

/* 全局变量 */
static hf_zxk_modbus_ring_buffer_t s_hf_zxk_modbus_rx_ring_buffer; /* 接收环形缓冲区 */
static hf_zxk_modbus_parser_t     s_hf_zxk_modbus_frame_parser;    /* 帧解析器 */
static uint8_t                    s_hf_zxk_modbus_parser_timeout_ticks; /* 半包超时计数 */

/* ---------------- 环形缓冲区 ---------------- */

/**
 * @brief 环形缓冲区初始化
 */
void hf_zxk_modbus_ring_buffer_init(hf_zxk_modbus_ring_buffer_t *rb)
{
    rb->head = 0U;
    rb->tail = 0U;
    rb->count = 0U;
    (void)memset(rb->buffer, 0, HF_ZXK_MODBUS_RING_BUFFER_SIZE);
}

/**
 * @brief 向环形缓冲区写入数据
 * @return 0-成功 1-缓冲区已满
 */
uint8_t hf_zxk_modbus_ring_buffer_push(hf_zxk_modbus_ring_buffer_t *rb, uint8_t data)
{
    if (rb->count >= (uint16_t)HF_ZXK_MODBUS_RING_BUFFER_SIZE)
    {
        return 1U;
    }

    rb->buffer[rb->head] = data;
    rb->head = (uint16_t)((rb->head + 1U) % HF_ZXK_MODBUS_RING_BUFFER_SIZE);
    rb->count++;

    return 0U;
}

/**
 * @brief 从环形缓冲区读取数据
 * @return 0-成功 1-缓冲区为空
 */
uint8_t hf_zxk_modbus_ring_buffer_pop(hf_zxk_modbus_ring_buffer_t *rb, uint8_t *data)
{
    if (rb->count == 0U)
    {
        return 1U;
    }

    *data = rb->buffer[rb->tail];
    HF_ZXK_MODBUS_LOG_RX_BYTE(*data);
    rb->tail = (uint16_t)((rb->tail + 1U) % HF_ZXK_MODBUS_RING_BUFFER_SIZE);
    rb->count--;

    return 0U;
}

/**
 * @brief 获取环形缓冲区中的数据数量
 */
uint16_t hf_zxk_modbus_ring_buffer_get_count(hf_zxk_modbus_ring_buffer_t *rb)
{
    return rb->count;
}

/**
 * @brief 判断环形缓冲区是否为空
 */
uint8_t hf_zxk_modbus_ring_buffer_is_empty(hf_zxk_modbus_ring_buffer_t *rb)
{
    return (rb->count == 0U) ? 1U : 0U;
}

/**
 * @brief 判断环形缓冲区是否已满
 */
uint8_t hf_zxk_modbus_ring_buffer_is_full(hf_zxk_modbus_ring_buffer_t *rb)
{
    return (rb->count >= (uint16_t)HF_ZXK_MODBUS_RING_BUFFER_SIZE) ? 1U : 0U;
}

/* ---------------- CRC-16/MODBUS ---------------- */

/**
 * @brief 逐字节累积计算 CRC（反射多项式 0xA001，初值 0xFFFF）
 * @note  采用逐位移位实现，不占用 512 字节查表空间；9600bps 下每帧仅十几字节，
 *        运算量可忽略。
 */
uint16_t hf_zxk_modbus_crc_update(uint16_t crc, uint8_t byte)
{
    uint8_t bit;

    crc ^= (uint16_t)byte;
    for (bit = 0U; bit < 8U; bit++)
    {
        if ((crc & 0x0001U) != 0U)
        {
            crc = (uint16_t)((crc >> 1U) ^ HF_ZXK_MODBUS_CRC_POLY);
        }
        else
        {
            crc = (uint16_t)(crc >> 1U);
        }
    }

    return crc;
}

/**
 * @brief 一次性计算一段数据的 CRC
 */
uint16_t hf_zxk_modbus_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = HF_ZXK_MODBUS_CRC_INIT;
    uint16_t i;

    for (i = 0U; i < len; i++)
    {
        crc = hf_zxk_modbus_crc_update(crc, data[i]);
    }

    return crc;
}

/* ---------------- 接收状态机 ---------------- */

/**
 * @brief 开始解析新的一帧（以从机地址作为帧起始）
 */
static void hf_zxk_modbus_parser_start_frame(hf_zxk_modbus_parser_t *parser, uint8_t address)
{
    hf_zxk_modbus_frame_t *frame = &parser->frame;

    frame->address        = address;
    frame->function       = 0U;
    frame->is_exception   = 0U;
    frame->exception_code = 0U;
    frame->reg_addr       = 0U;
    frame->reg_value      = 0U;
    frame->reg_quantity   = 0U;
    frame->byte_count     = 0U;
    frame->crc            = 0U;
    /* data[] 不必清零：解析结果只按 byte_count 读取 */

    parser->data_index = 0U;
    parser->calc_crc   = hf_zxk_modbus_crc_update(HF_ZXK_MODBUS_CRC_INIT, address);
}

/**
 * @brief 帧解析器初始化
 */
void hf_zxk_modbus_parser_init(hf_zxk_modbus_parser_t *parser)
{
    parser->state       = HF_ZXK_MODBUS_STATE_IDLE;
    parser->data_index  = 0U;
    parser->calc_crc    = HF_ZXK_MODBUS_CRC_INIT;
    parser->frame_ready = 0U;
    (void)memset(&parser->frame, 0, sizeof(hf_zxk_modbus_frame_t));
}

/**
 * @brief 处理接收到的单个字节（接收状态机）
 * @param parser    帧解析器指针
 * @param byte      接收到的字节
 * @param out_frame 解析完成的帧输出指针（可为 NULL）
 */
void hf_zxk_modbus_parser_process_byte(hf_zxk_modbus_parser_t *parser,
                                       uint8_t byte,
                                       hf_zxk_modbus_frame_t *out_frame)
{
    hf_zxk_modbus_frame_t *frame;

    if (parser == NULL)
    {
        return;
    }

    parser->frame_ready = 0U;
    frame = &parser->frame;

    switch (parser->state)
    {
    case HF_ZXK_MODBUS_STATE_IDLE:
        /* MODBUS-RTU 没有帧头，任意字节都按从机地址处理；
           功能码非法或 CRC 错误时会回到本状态重新同步 */
        hf_zxk_modbus_parser_start_frame(parser, byte);
        parser->state = HF_ZXK_MODBUS_STATE_FUNC;
        break;

    case HF_ZXK_MODBUS_STATE_FUNC:
        parser->calc_crc = hf_zxk_modbus_crc_update(parser->calc_crc, byte);
        frame->function = byte;

        switch (byte)
        {
        case (uint8_t)HF_ZXK_MODBUS_FUNC_READ_HOLDING:
            parser->state = HF_ZXK_MODBUS_STATE_BYTE_COUNT;
            break;

        case (uint8_t)HF_ZXK_MODBUS_FUNC_WRITE_SINGLE:
        case (uint8_t)HF_ZXK_MODBUS_FUNC_WRITE_MULTIPLE:
            parser->state = HF_ZXK_MODBUS_STATE_REG_H;
            break;

        case (uint8_t)HF_ZXK_MODBUS_FUNC_READ_HOLDING | HF_ZXK_MODBUS_FUNC_EXCEPTION_MASK:
        case (uint8_t)HF_ZXK_MODBUS_FUNC_WRITE_SINGLE | HF_ZXK_MODBUS_FUNC_EXCEPTION_MASK:
        case (uint8_t)HF_ZXK_MODBUS_FUNC_WRITE_MULTIPLE | HF_ZXK_MODBUS_FUNC_EXCEPTION_MASK:
            frame->is_exception = 1U;
            frame->function = (uint8_t)(byte & (uint8_t)(~HF_ZXK_MODBUS_FUNC_EXCEPTION_MASK));
            parser->state = HF_ZXK_MODBUS_STATE_EXCEPTION;
            break;

        default:
            /* 功能码不认识：当前字节也可能是一帧的起始地址，直接重新开始 */
            hf_zxk_modbus_parser_start_frame(parser, byte);
            parser->state = HF_ZXK_MODBUS_STATE_FUNC;
            break;
        }
        break;

    case HF_ZXK_MODBUS_STATE_BYTE_COUNT:
        parser->calc_crc = hf_zxk_modbus_crc_update(parser->calc_crc, byte);
        frame->byte_count = byte;

        /* 0x03 回复的数据域必须是偶数个字节(寄存器为 16 位)且不超过缓冲区 */
        if ((byte == 0U) || ((byte % 2U) != 0U) || (byte > HF_ZXK_MODBUS_MAX_DATA_LEN))
        {
            parser->state = HF_ZXK_MODBUS_STATE_IDLE;
        }
        else
        {
            parser->data_index = 0U;
            parser->state = HF_ZXK_MODBUS_STATE_DATA;
        }
        break;

    case HF_ZXK_MODBUS_STATE_DATA:
        parser->calc_crc = hf_zxk_modbus_crc_update(parser->calc_crc, byte);
        if (parser->data_index < (uint16_t)HF_ZXK_MODBUS_MAX_DATA_LEN)
        {
            frame->data[parser->data_index] = byte;
        }
        parser->data_index++;

        if (parser->data_index >= (uint16_t)frame->byte_count)
        {
            parser->state = HF_ZXK_MODBUS_STATE_CRC_L;
        }
        break;

    case HF_ZXK_MODBUS_STATE_REG_H:
        parser->calc_crc = hf_zxk_modbus_crc_update(parser->calc_crc, byte);
        frame->reg_addr = (uint16_t)((uint16_t)byte << 8);
        parser->state = HF_ZXK_MODBUS_STATE_REG_L;
        break;

    case HF_ZXK_MODBUS_STATE_REG_L:
        parser->calc_crc = hf_zxk_modbus_crc_update(parser->calc_crc, byte);
        frame->reg_addr |= (uint16_t)byte;
        parser->state = HF_ZXK_MODBUS_STATE_VALUE_H;
        break;

    case HF_ZXK_MODBUS_STATE_VALUE_H:
        parser->calc_crc = hf_zxk_modbus_crc_update(parser->calc_crc, byte);
        if (frame->function == (uint8_t)HF_ZXK_MODBUS_FUNC_WRITE_SINGLE)
        {
            frame->reg_value = (uint16_t)((uint16_t)byte << 8);
        }
        else
        {
            frame->reg_quantity = (uint16_t)((uint16_t)byte << 8);
        }
        parser->state = HF_ZXK_MODBUS_STATE_VALUE_L;
        break;

    case HF_ZXK_MODBUS_STATE_VALUE_L:
        parser->calc_crc = hf_zxk_modbus_crc_update(parser->calc_crc, byte);
        if (frame->function == (uint8_t)HF_ZXK_MODBUS_FUNC_WRITE_SINGLE)
        {
            frame->reg_value |= (uint16_t)byte;
        }
        else
        {
            frame->reg_quantity |= (uint16_t)byte;
        }
        /* 0x06/0x10 的回显帧到此结束，后面直接是 CRC */
        parser->state = HF_ZXK_MODBUS_STATE_CRC_L;
        break;

    case HF_ZXK_MODBUS_STATE_EXCEPTION:
        parser->calc_crc = hf_zxk_modbus_crc_update(parser->calc_crc, byte);
        frame->exception_code = byte;
        parser->state = HF_ZXK_MODBUS_STATE_CRC_L;
        break;

    case HF_ZXK_MODBUS_STATE_CRC_L:
        /* CRC 自身不参与计算；MODBUS 低字节先发 */
        frame->crc = (uint16_t)byte;
        parser->state = HF_ZXK_MODBUS_STATE_CRC_H;
        break;

    case HF_ZXK_MODBUS_STATE_CRC_H:
        frame->crc |= (uint16_t)((uint16_t)byte << 8);

        if (parser->calc_crc == frame->crc)
        {
            parser->frame_ready = 1U;
            if (out_frame != NULL)
            {
                *out_frame = *frame;
            }
        }
        else
        {
            HF_ZXK_MODBUS_LOG_WARN("[MODBUS] CRC error: calc=0x%04X recv=0x%04X\r\n",
                                   (unsigned int)parser->calc_crc,
                                   (unsigned int)frame->crc);
        }

        /* 无论校验是否通过都重新开始解析 */
        parser->state = HF_ZXK_MODBUS_STATE_IDLE;
        break;

    default:
        parser->state = HF_ZXK_MODBUS_STATE_IDLE;
        break;
    }
}

/* ---------------- UART 收发 ---------------- */

/**
 * @brief 线控 MODBUS 模块初始化
 */
void hf_zxk_modbus_uart_init(void)
{
    hf_zxk_modbus_ring_buffer_init(&s_hf_zxk_modbus_rx_ring_buffer);
    hf_zxk_modbus_parser_init(&s_hf_zxk_modbus_frame_parser);
    s_hf_zxk_modbus_parser_timeout_ticks = 0U;

    HF_ZXK_MODBUS_LOG_INFO("[MODBUS] UART initialized\r\n");
}

/**
 * @brief 将接收到的字节写入环形缓冲区（在 USART2 中断中调用）
 */
void hf_zxk_modbus_uart_rxcallback(uint8_t data)
{
    if (hf_zxk_modbus_ring_buffer_push(&s_hf_zxk_modbus_rx_ring_buffer, data) != 0U)
    {
        HF_ZXK_MODBUS_LOG_WARN("[MODBUS] Ring buffer overflow!\r\n");
    }
}

/**
 * @brief 处理环形缓冲区中的数据（在主循环中调用）
 */
void hf_zxk_modbus_uart_process(void)
{
    uint8_t byte;
    uint8_t received_data = 0U;
    hf_zxk_modbus_frame_t parsed_frame;

    while (hf_zxk_modbus_ring_buffer_pop(&s_hf_zxk_modbus_rx_ring_buffer, &byte) == 0U)
    {
        /* 调试用：打印解析前的原始字节 */
        HF_ZXK_MODBUS_LOG_INFO("[MODBUS] RX byte: 0x%02X\r\n", (unsigned int)byte);

        received_data = 1U;
        s_hf_zxk_modbus_parser_timeout_ticks = 0U;
        hf_zxk_modbus_parser_process_byte(&s_hf_zxk_modbus_frame_parser, byte, &parsed_frame);
        if (s_hf_zxk_modbus_frame_parser.frame_ready != 0U)
        {
            hf_zxk_modbus_handle_frame(&parsed_frame);
        }
    }

    if (received_data == 0U)
    {
        if (s_hf_zxk_modbus_frame_parser.state != HF_ZXK_MODBUS_STATE_IDLE)
        {
            if (s_hf_zxk_modbus_parser_timeout_ticks < HF_ZXK_MODBUS_FRAME_PARSER_TIMEOUT_TICKS)
            {
                s_hf_zxk_modbus_parser_timeout_ticks++;
            }

            if (s_hf_zxk_modbus_parser_timeout_ticks >= HF_ZXK_MODBUS_FRAME_PARSER_TIMEOUT_TICKS)
            {
                HF_ZXK_MODBUS_LOG_WARN("[MODBUS] Frame parser timeout, reset\r\n");
                hf_zxk_modbus_parser_init(&s_hf_zxk_modbus_frame_parser);
                s_hf_zxk_modbus_parser_timeout_ticks = 0U;
            }
        }
        else
        {
            s_hf_zxk_modbus_parser_timeout_ticks = 0U;
        }
    }
    else if (s_hf_zxk_modbus_frame_parser.state == HF_ZXK_MODBUS_STATE_IDLE)
    {
        s_hf_zxk_modbus_parser_timeout_ticks = 0U;
    }
}

/**
 * @brief 发送一段 MODBUS PDU（自动追加 CRC 低字节、CRC 高字节）
 * @param pdu     从机地址 + 功能码 + 数据域
 * @param pdu_len PDU 长度
 * @return 0-成功 1-参数非法
 * @note  供后续按协议组装 0x03 查询帧 / 0x06、0x10 控制帧时调用
 */
uint8_t hf_zxk_modbus_uart_send_pdu(const uint8_t *pdu, uint16_t pdu_len)
{
    uint8_t tx_buffer[HF_ZXK_MODBUS_MAX_FRAME_LEN];
    uint16_t crc;
    uint16_t i;

    if ((pdu == NULL) || (pdu_len == 0U) ||
        (pdu_len > (uint16_t)(HF_ZXK_MODBUS_MAX_FRAME_LEN - HF_ZXK_MODBUS_CRC_LEN)))
    {
        HF_ZXK_MODBUS_LOG_WARN("[MODBUS] invalid pdu length: %u\r\n", (unsigned int)pdu_len);
        return 1U;
    }

    (void)memcpy(tx_buffer, pdu, pdu_len);
    crc = hf_zxk_modbus_crc16(pdu, pdu_len);
    tx_buffer[pdu_len] = (uint8_t)(crc & 0xFFU);
    tx_buffer[pdu_len + 1U] = (uint8_t)((crc >> 8) & 0xFFU);

    HF_ZXK_MODBUS_TX_SEND(tx_buffer, (uint16_t)(pdu_len + HF_ZXK_MODBUS_CRC_LEN));

    HF_ZXK_MODBUS_LOG_TX_HEX_BEGIN(pdu_len + HF_ZXK_MODBUS_CRC_LEN);
    for (i = 0U; i < (uint16_t)(pdu_len + HF_ZXK_MODBUS_CRC_LEN); i++)
    {
        HF_ZXK_MODBUS_LOG_TX_HEX_BYTE(tx_buffer[i]);
    }
    HF_ZXK_MODBUS_LOG_TX_HEX_END();

    return 0U;
}

#else /* HOOCH_PROTOCOL_ENABLE_HF_WIRED_CONTROLLER_485 */

/* 线控器协议关闭：只保留空实现，调用方无需屏蔽代码，也不占用 Flash / RAM */

void hf_zxk_modbus_uart_init(void)
{
}

void hf_zxk_modbus_uart_process(void)
{
}

uint8_t hf_zxk_modbus_uart_send_pdu(const uint8_t *pdu, uint16_t pdu_len)
{
    (void)pdu;
    (void)pdu_len;
    return 1U;
}

void hf_zxk_modbus_uart_rxcallback(uint8_t data)
{
    (void)data;
}

#endif /* HOOCH_PROTOCOL_ENABLE_HF_WIRED_CONTROLLER_485 */
