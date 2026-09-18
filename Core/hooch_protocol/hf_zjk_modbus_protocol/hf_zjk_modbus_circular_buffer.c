#include "hf_zjk_modbus_circular_buffer.h"
#include "hooch_protocol_common.h"

#if HOOCH_PROTOCOL_ENABLE_HF_CENTRAL_CONTROL_485

extern UART_HandleTypeDef huart2;

/* 主循环连续空转多少次未收到新字节即认为半包超时复位解析器
 * (9600bps 下一个字节约 1ms，主循环节拍远大于此) */
#define HF_ZJK_MODBUS_FRAME_PARSER_TIMEOUT_TICKS 5U

/* 全局变量 */
static hf_zjk_modbus_ring_buffer_t s_hf_zjk_modbus_rx_ring_buffer; /* 接收环形缓冲区 */
static hf_zjk_modbus_parser_t     s_hf_zjk_modbus_frame_parser;    /* 帧解析器 */
static uint8_t                    s_hf_zjk_modbus_parser_timeout_ticks; /* 半包超时计数 */

/* ---------------- 环形缓冲区 ---------------- */

/**
 * @brief 环形缓冲区初始化
 */
void hf_zjk_modbus_ring_buffer_init(hf_zjk_modbus_ring_buffer_t *rb)
{
    rb->head = 0U;
    rb->tail = 0U;
    rb->count = 0U;
    (void)memset(rb->buffer, 0, HF_ZJK_MODBUS_RING_BUFFER_SIZE);
}

/**
 * @brief 向环形缓冲区写入数据
 * @return 0-成功 1-缓冲区已满
 */
uint8_t hf_zjk_modbus_ring_buffer_push(hf_zjk_modbus_ring_buffer_t *rb, uint8_t data)
{
    if (rb->count >= (uint16_t)HF_ZJK_MODBUS_RING_BUFFER_SIZE)
    {
        return 1U;
    }

    rb->buffer[rb->head] = data;
    rb->head = (uint16_t)((rb->head + 1U) % HF_ZJK_MODBUS_RING_BUFFER_SIZE);
    rb->count++;

    return 0U;
}

/**
 * @brief 从环形缓冲区读取数据
 * @return 0-成功 1-缓冲区为空
 */
uint8_t hf_zjk_modbus_ring_buffer_pop(hf_zjk_modbus_ring_buffer_t *rb, uint8_t *data)
{
    if (rb->count == 0U)
    {
        return 1U;
    }

    *data = rb->buffer[rb->tail];
    HF_ZJK_MODBUS_LOG_RX_BYTE(*data);
    rb->tail = (uint16_t)((rb->tail + 1U) % HF_ZJK_MODBUS_RING_BUFFER_SIZE);
    rb->count--;

    return 0U;
}

/**
 * @brief 获取环形缓冲区中的数据数量
 */
uint16_t hf_zjk_modbus_ring_buffer_get_count(hf_zjk_modbus_ring_buffer_t *rb)
{
    return rb->count;
}

/**
 * @brief 判断环形缓冲区是否为空
 */
uint8_t hf_zjk_modbus_ring_buffer_is_empty(hf_zjk_modbus_ring_buffer_t *rb)
{
    return (rb->count == 0U) ? 1U : 0U;
}

/**
 * @brief 判断环形缓冲区是否已满
 */
uint8_t hf_zjk_modbus_ring_buffer_is_full(hf_zjk_modbus_ring_buffer_t *rb)
{
    return (rb->count >= (uint16_t)HF_ZJK_MODBUS_RING_BUFFER_SIZE) ? 1U : 0U;
}

/* ---------------- 累加和校验 ---------------- */

/**
 * @brief 逐字节累加校验和（溢出只取低 8 位）
 */
uint8_t hf_zjk_modbus_checksum_update(uint8_t sum, uint8_t byte)
{
    return (uint8_t)(sum + byte);
}

/**
 * @brief 一次性计算一段数据的校验和
 */
uint8_t hf_zjk_modbus_checksum(const uint8_t *data, uint16_t len)
{
    uint8_t  sum = 0U;
    uint16_t i;

    for (i = 0U; i < len; i++)
    {
        sum = hf_zjk_modbus_checksum_update(sum, data[i]);
    }

    return sum;
}

/* ---------------- 接收状态机 ---------------- */

/* 是否为空调状态查询回复的功能码（0x50 / 0x53） */
static uint8_t hf_zjk_modbus_is_status_function(uint8_t function)
{
    return ((function == (uint8_t)HF_ZJK_MODBUS_FUNC_AC_QUERY_STATUS) ||
            (function == (uint8_t)HF_ZJK_MODBUS_FUNC_AC_QUERY_STATUS_DEC)) ? 1U : 0U;
}

/* 是否为空调性能查询回复的功能码（0x90） */
static uint8_t hf_zjk_modbus_is_performance_function(uint8_t function)
{
    return (function == (uint8_t)HF_ZJK_MODBUS_FUNC_AC_QUERY_PERFORMANCE) ? 1U : 0U;
}

/* 是否为空调控制指令（下行指令的回显帧）的功能码 */
uint8_t hf_zjk_modbus_is_control_function(uint8_t function)
{
    switch (function)
    {
    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_POWER:
    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_TEMP:
    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_MODE:
    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_FAN_SPEED:
    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_WIND:
    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_TEMP_FRAC:
    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_LOCK_POWER:
    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_LOCK_TEMP_MIN:
    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_LOCK_TEMP_MAX:
    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_LOCK_MODE:
    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_ALL:
    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_ALL_FRAC:
        return 1U;
    default:
        return 0U;
    }
}

/* 控制值占用字节数：0x36 为"整数 + 小数"2 字节，0x60/0x61 为"开关+温度+模式+风速"4/5 字节 */
static uint8_t hf_zjk_modbus_control_value_len(uint8_t function)
{
    switch (function)
    {
    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_TEMP_FRAC:
        return 2U;
    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_ALL:
        return 4U;
    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_ALL_FRAC:
        return 5U;
    default:
        return 1U;
    }
}

/* 状态查询回复中单个设备数据块的长度 */
static uint16_t hf_zjk_modbus_status_block_len(uint8_t function, uint8_t control)
{
    if (control == (uint8_t)HF_ZJK_MODBUS_QUERY_ONLINE)
    {
        return HF_ZJK_MODBUS_ONLINE_BLOCK_LEN;
    }

    return (function == (uint8_t)HF_ZJK_MODBUS_FUNC_AC_QUERY_STATUS_DEC)
        ? HF_ZJK_MODBUS_DEV_BLOCK_LEN_DEC
        : HF_ZJK_MODBUS_DEV_BLOCK_LEN;
}

/**
 * @brief 开始解析新的一帧（以网关地址作为帧起始）
 */
static void hf_zjk_modbus_parser_start_frame(hf_zjk_modbus_parser_t *parser, uint8_t gateway_addr)
{
    hf_zjk_modbus_frame_t *frame = &parser->frame;

    frame->gateway_addr = gateway_addr;
    frame->function     = 0U;
    frame->control      = 0U;
    frame->device_count = 0U;
    frame->overflow     = 0U;
    frame->data_len     = 0U;
    frame->checksum     = 0U;
    /* data[] 不必清零：解析结果只按 data_len 读取 */

    parser->data_index        = 0U;
    parser->payload_remaining = 0U;
    parser->control_remaining = 0U;
    parser->control_index     = 0U;
    parser->calc_sum          = gateway_addr;
}

/**
 * @brief 帧解析器初始化
 */
void hf_zjk_modbus_parser_init(hf_zjk_modbus_parser_t *parser)
{
    parser->state       = HF_ZJK_MODBUS_STATE_IDLE;
    parser->frame_ready = 0U;
    (void)memset(&parser->frame, 0, sizeof(hf_zjk_modbus_frame_t));
    hf_zjk_modbus_parser_start_frame(parser, 0U);
    parser->state = HF_ZJK_MODBUS_STATE_IDLE;
}

/**
 * @brief 处理接收到的单个字节（接收状态机）
 * @param parser    帧解析器指针
 * @param byte      接收到的字节
 * @param out_frame 解析完成的帧输出指针（可为 NULL）
 */
void hf_zjk_modbus_parser_process_byte(hf_zjk_modbus_parser_t *parser,
                                       uint8_t byte,
                                       hf_zjk_modbus_frame_t *out_frame)
{
    hf_zjk_modbus_frame_t *frame;

    if (parser == NULL)
    {
        return;
    }

    parser->frame_ready = 0U;
    frame = &parser->frame;

    switch (parser->state)
    {
    case HF_ZJK_MODBUS_STATE_IDLE:
        /* 自定义协议没有帧头，任意字节都按网关地址处理；
           功能码非法或校验错误时会回到本状态重新同步 */
        hf_zjk_modbus_parser_start_frame(parser, byte);
        parser->state = HF_ZJK_MODBUS_STATE_FUNC;
        break;

    case HF_ZJK_MODBUS_STATE_FUNC:
        parser->calc_sum = hf_zjk_modbus_checksum_update(parser->calc_sum, byte);
        frame->function = byte;

        if (hf_zjk_modbus_is_status_function(byte) != 0U)
        {
            parser->control_remaining = 1U;
            parser->control_index     = 0U;
            parser->state             = HF_ZJK_MODBUS_STATE_CONTROL;
        }
        else if (hf_zjk_modbus_is_performance_function(byte) != 0U)
        {
            parser->control_remaining = 1U;
            parser->control_index     = 0U;
            parser->state             = HF_ZJK_MODBUS_STATE_CONTROL;
        }
        else if (hf_zjk_modbus_is_control_function(byte) != 0U)
        {
            parser->control_remaining = hf_zjk_modbus_control_value_len(byte);
            parser->control_index     = 0U;
            parser->state             = HF_ZJK_MODBUS_STATE_CONTROL;
        }
        else
        {
            /* 功能码不认识：当前字节也可能是一帧的起始地址，直接重新开始 */
            HF_ZJK_MODBUS_LOG_WARN("[ZJK] unsupported function: 0x%02X\r\n", (unsigned int)byte);
            hf_zjk_modbus_parser_start_frame(parser, byte);
            parser->state = HF_ZJK_MODBUS_STATE_FUNC;
        }
        break;

    case HF_ZJK_MODBUS_STATE_CONTROL:
        parser->calc_sum = hf_zjk_modbus_checksum_update(parser->calc_sum, byte);
        if (parser->control_index == 0U)
        {
            frame->control = byte;
        }
        parser->control_index++;
        parser->control_remaining--;

        if (parser->control_remaining > 0U)
        {
            break;
        }

        if (hf_zjk_modbus_is_performance_function(frame->function) != 0U)
        {
            /* 性能查询回复：控制值(0xFF)之后直接是固定 7 字节数据域，没有设备数量 */
            parser->payload_remaining = HF_ZJK_MODBUS_PERFORMANCE_DATA_LEN;
            parser->state             = HF_ZJK_MODBUS_STATE_PAYLOAD;
        }
        else if (hf_zjk_modbus_is_status_function(frame->function) != 0U)
        {
            if (frame->control == (uint8_t)HF_ZJK_MODBUS_QUERY_FAULT)
            {
                /* 故障码回复不含设备数量，长度只能靠校验和终结 */
                parser->state = HF_ZJK_MODBUS_STATE_PAYLOAD_TO_SUM;
            }
            else
            {
                parser->state = HF_ZJK_MODBUS_STATE_COUNT;
            }
        }
        else
        {
            /* 控制指令回显：设备数量之后是固定 2 字节的设备地址域 */
            parser->state = HF_ZJK_MODBUS_STATE_COUNT;
        }
        break;

    case HF_ZJK_MODBUS_STATE_COUNT:
        parser->calc_sum = hf_zjk_modbus_checksum_update(parser->calc_sum, byte);
        frame->device_count = byte;

        if (hf_zjk_modbus_is_status_function(frame->function) != 0U)
        {
            parser->payload_remaining = (uint16_t)((uint16_t)byte *
                hf_zjk_modbus_status_block_len(frame->function, frame->control));
        }
        else
        {
            parser->payload_remaining = HF_ZJK_MODBUS_CTRL_ACK_ADDR_LEN;
        }

        if (parser->payload_remaining == 0U)
        {
            /* 设备数量为 0(网关尚未准备好)：数据域为空，下一字节就是校验和 */
            parser->state = HF_ZJK_MODBUS_STATE_CHECKSUM;
        }
        else
        {
            parser->state = HF_ZJK_MODBUS_STATE_PAYLOAD;
        }
        break;

    case HF_ZJK_MODBUS_STATE_PAYLOAD:
        parser->calc_sum = hf_zjk_modbus_checksum_update(parser->calc_sum, byte);
        if (parser->data_index < (uint16_t)HF_ZJK_MODBUS_MAX_DATA_LEN)
        {
            frame->data[parser->data_index] = byte;
        }
        else
        {
            frame->overflow = 1U;
        }
        parser->data_index++;
        parser->payload_remaining--;

        if (parser->payload_remaining == 0U)
        {
            frame->data_len = parser->data_index;
            parser->state   = HF_ZJK_MODBUS_STATE_CHECKSUM;
        }
        break;

    case HF_ZJK_MODBUS_STATE_PAYLOAD_TO_SUM:
        /* 0x04 故障码回复：校验和 = 之前所有字节的累加和，
           当某字节正好等于当前累加和时，即认为它是校验字节（帧结束）。
           数据域前段用最小长度保护，避免误判。 */
        if ((parser->data_index >= HF_ZJK_MODBUS_SUM_TERMINATED_MIN_LEN) &&
            (byte == parser->calc_sum))
        {
            frame->data_len   = parser->data_index;
            frame->checksum   = byte;
            parser->frame_ready = 1U;
            if (out_frame != NULL)
            {
                *out_frame = *frame;
            }
            parser->state = HF_ZJK_MODBUS_STATE_IDLE;
        }
        else
        {
            parser->calc_sum = hf_zjk_modbus_checksum_update(parser->calc_sum, byte);
            if (parser->data_index < (uint16_t)HF_ZJK_MODBUS_MAX_DATA_LEN)
            {
                frame->data[parser->data_index] = byte;
            }
            else
            {
                frame->overflow = 1U;
            }
            parser->data_index++;
        }
        break;

    case HF_ZJK_MODBUS_STATE_CHECKSUM:
        /* 校验和自身不参与累加 */
        frame->checksum = byte;

        if (byte == parser->calc_sum)
        {
            parser->frame_ready = 1U;
            if (out_frame != NULL)
            {
                *out_frame = *frame;
            }
        }
        else
        {
            HF_ZJK_MODBUS_LOG_WARN("[ZJK] checksum error: calc=0x%02X recv=0x%02X\r\n",
                                   (unsigned int)parser->calc_sum,
                                   (unsigned int)byte);
        }

        /* 无论校验是否通过都重新开始解析 */
        parser->state = HF_ZJK_MODBUS_STATE_IDLE;
        break;

    default:
        parser->state = HF_ZJK_MODBUS_STATE_IDLE;
        break;
    }
}

/* ---------------- UART 收发 ---------------- */

/**
 * @brief 集控 自定义 485 模块初始化
 */
void hf_zjk_modbus_uart_init(void)
{
    hf_zjk_modbus_ring_buffer_init(&s_hf_zjk_modbus_rx_ring_buffer);
    hf_zjk_modbus_parser_init(&s_hf_zjk_modbus_frame_parser);
    s_hf_zjk_modbus_parser_timeout_ticks = 0U;

    HF_ZJK_MODBUS_LOG_INFO("[ZJK] UART initialized\r\n");
}

/**
 * @brief 将接收到的字节写入环形缓冲区（在 USART2 中断中调用）
 */
void hf_zjk_modbus_uart_rxcallback(uint8_t data)
{
    if (hf_zjk_modbus_ring_buffer_push(&s_hf_zjk_modbus_rx_ring_buffer, data) != 0U)
    {
        HF_ZJK_MODBUS_LOG_WARN("[ZJK] Ring buffer overflow!\r\n");
    }
}

/**
 * @brief 处理环形缓冲区中的数据（在主循环中调用）
 */
void hf_zjk_modbus_uart_process(void)
{
    uint8_t byte;
    uint8_t received_data = 0U;
    hf_zjk_modbus_frame_t parsed_frame;

    while (hf_zjk_modbus_ring_buffer_pop(&s_hf_zjk_modbus_rx_ring_buffer, &byte) == 0U)
    {
        received_data = 1U;
        s_hf_zjk_modbus_parser_timeout_ticks = 0U;
        hf_zjk_modbus_parser_process_byte(&s_hf_zjk_modbus_frame_parser, byte, &parsed_frame);
        if (s_hf_zjk_modbus_frame_parser.frame_ready != 0U)
        {
            hf_zjk_modbus_handle_frame(&parsed_frame);
        }
    }

    if (received_data == 0U)
    {
        if (s_hf_zjk_modbus_frame_parser.state != HF_ZJK_MODBUS_STATE_IDLE)
        {
            if (s_hf_zjk_modbus_parser_timeout_ticks < HF_ZJK_MODBUS_FRAME_PARSER_TIMEOUT_TICKS)
            {
                s_hf_zjk_modbus_parser_timeout_ticks++;
            }

            if (s_hf_zjk_modbus_parser_timeout_ticks >= HF_ZJK_MODBUS_FRAME_PARSER_TIMEOUT_TICKS)
            {
                HF_ZJK_MODBUS_LOG_WARN("[ZJK] Frame parser timeout, reset\r\n");
                hf_zjk_modbus_parser_init(&s_hf_zjk_modbus_frame_parser);
                s_hf_zjk_modbus_parser_timeout_ticks = 0U;
            }
        }
        else
        {
            s_hf_zjk_modbus_parser_timeout_ticks = 0U;
        }
    }
    else if (s_hf_zjk_modbus_frame_parser.state == HF_ZJK_MODBUS_STATE_IDLE)
    {
        s_hf_zjk_modbus_parser_timeout_ticks = 0U;
    }
}

/**
 * @brief 发送一段集控 PDU（自动追加 1 字节累加和校验）
 * @param pdu     网关地址 + 功能码 + 控制值 + 设备数量 + 设备地址
 * @param pdu_len PDU 长度
 * @return 0-成功 1-参数非法
 * @note  供后续按协议组装状态查询帧 / 控制帧时调用
 */
uint8_t hf_zjk_modbus_uart_send_pdu(const uint8_t *pdu, uint16_t pdu_len)
{
    uint8_t  tx_buffer[HF_ZJK_MODBUS_MAX_TX_PDU_LEN + HF_ZJK_MODBUS_CHECKSUM_LEN];
    uint16_t i;

    if ((pdu == NULL) || (pdu_len == 0U) ||
        (pdu_len > (uint16_t)HF_ZJK_MODBUS_MAX_TX_PDU_LEN))
    {
        HF_ZJK_MODBUS_LOG_WARN("[ZJK] invalid pdu length: %u\r\n", (unsigned int)pdu_len);
        return 1U;
    }

    (void)memcpy(tx_buffer, pdu, pdu_len);
    tx_buffer[pdu_len] = hf_zjk_modbus_checksum(pdu, pdu_len);

    HF_ZJK_MODBUS_TX_SEND(tx_buffer, (uint16_t)(pdu_len + HF_ZJK_MODBUS_CHECKSUM_LEN));

    HF_ZJK_MODBUS_LOG_TX_HEX_BEGIN(pdu_len + HF_ZJK_MODBUS_CHECKSUM_LEN);
    for (i = 0U; i < (uint16_t)(pdu_len + HF_ZJK_MODBUS_CHECKSUM_LEN); i++)
    {
        HF_ZJK_MODBUS_LOG_TX_HEX_BYTE(tx_buffer[i]);
    }
    HF_ZJK_MODBUS_LOG_TX_HEX_END();

    return 0U;
}

#else /* HOOCH_PROTOCOL_ENABLE_HF_CENTRAL_CONTROL_485 */

/* 集控器协议关闭：只保留空实现，调用方无需屏蔽代码，也不占用 Flash / RAM */

void hf_zjk_modbus_uart_init(void)
{
}

void hf_zjk_modbus_uart_process(void)
{
}

uint8_t hf_zjk_modbus_uart_send_pdu(const uint8_t *pdu, uint16_t pdu_len)
{
    (void)pdu;
    (void)pdu_len;
    return 1U;
}

void hf_zjk_modbus_uart_rxcallback(uint8_t data)
{
    (void)data;
}

#endif /* HOOCH_PROTOCOL_ENABLE_HF_CENTRAL_CONTROL_485 */
