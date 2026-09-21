#include "hf_zxk_modbus_circular_buffer.h"
#include "hf_zxk_modbus_handle.h"
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
static uint8_t                    s_hf_zxk_modbus_enabled = (uint8_t)HF_ZXK_MODBUS_ENABLE_DEFAULT; /* 模块使能: 1=开启 0=关闭 */

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

/* ---------------- 发送调度（控制与查询共用半双工 485 总线） ---------------- */

/* 主循环节拍约 20ms（main.c 中 HAL_Delay(20)），用于把协议推荐的时间换算成节拍数 */
#define HF_ZXK_MODBUS_LOOP_TICK_MS 20U

/* 任意两帧之间的最小间隔：协议要求连续 06 帧间隔 >100ms */
#define HF_ZXK_MODBUS_TX_MIN_GAP_TICKS (100U / HF_ZXK_MODBUS_LOOP_TICK_MS)

/* 查询最大推迟时间：控制帧会不断重置查询倒计时，用这个上限保证查询
 * 最迟每 3 秒仍会发出一次（协议推荐查询间隔 1~3 秒） */
#define HF_ZXK_MODBUS_QUERY_MAX_DELAY_TICKS (3000U / HF_ZXK_MODBUS_LOOP_TICK_MS)

/* 待发控制帧队列深度：平台侧一次全量控制会连续下发多项（开关/模式/风速/温度） */
#define HF_ZXK_MODBUS_TX_QUEUE_DEPTH 4U

/* 0x03 查询帧与 0x06 控制帧数据域等长（2 字节寄存器 + 2 字节值/数量） */
#define HF_ZXK_MODBUS_PDU_LEN 6U

/* 控制帧待发队列：回调只入队，由主循环节拍统一出队，避免与查询抢总线 */
static uint8_t s_hf_zxk_modbus_tx_queue[HF_ZXK_MODBUS_TX_QUEUE_DEPTH][HF_ZXK_MODBUS_PDU_LEN];
static uint8_t s_hf_zxk_modbus_tx_head  = 0U;
static uint8_t s_hf_zxk_modbus_tx_tail  = 0U;
static uint8_t s_hf_zxk_modbus_tx_count = 0U;

/* 距上次发帧的节拍数（初值取满，上电即可发帧） */
static uint16_t s_hf_zxk_modbus_since_tx_ticks = 0xFFFFU;

/* 距上次发出查询帧的节拍数（初值取满，上电即发性能查询） */
static uint16_t s_hf_zxk_modbus_since_query_ticks = 0xFFFFU;

/**
 * @brief 控制帧入队（队列满时丢弃最旧一帧，保证最新控制不丢）
 */
static void hf_zxk_modbus_tx_queue_push(const uint8_t *pdu)
{
    if (s_hf_zxk_modbus_tx_count >= HF_ZXK_MODBUS_TX_QUEUE_DEPTH)
    {
        s_hf_zxk_modbus_tx_tail =
            (uint8_t)((s_hf_zxk_modbus_tx_tail + 1U) % HF_ZXK_MODBUS_TX_QUEUE_DEPTH);
        s_hf_zxk_modbus_tx_count--;
        HF_ZXK_MODBUS_LOG_WARN("[MODBUS] tx queue full, drop oldest\r\n");
    }

    (void)memcpy(s_hf_zxk_modbus_tx_queue[s_hf_zxk_modbus_tx_head], pdu, HF_ZXK_MODBUS_PDU_LEN);
    s_hf_zxk_modbus_tx_head =
        (uint8_t)((s_hf_zxk_modbus_tx_head + 1U) % HF_ZXK_MODBUS_TX_QUEUE_DEPTH);
    s_hf_zxk_modbus_tx_count++;
}

/* ---------------- 空调上报（HOOCH -> 线控网关） ---------------- */

/* HOOCH 空调模式 -> 线控模式位（协议未定义自动模式，AUTO 视为非法） */
static uint8_t hf_zxk_modbus_map_ac_mode(HOOCH_PROTOCOL_AirConditionerMode_t mode)
{
    switch (mode)
    {
    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_HEAT:
        return (uint8_t)HF_ZXK_MODBUS_MODE_HEAT;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_COOL:
        return (uint8_t)HF_ZXK_MODBUS_MODE_COOL;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_FAN:
        return (uint8_t)HF_ZXK_MODBUS_MODE_FAN;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_DRY:
        return (uint8_t)HF_ZXK_MODBUS_MODE_DRY;

    default:
        return (uint8_t)HF_ZXK_MODBUS_MODE_INVALID;
    }
}

/* HOOCH 空调风速 -> 线控风速 */
static uint8_t hf_zxk_modbus_map_ac_fan_speed(
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed)
{
    switch (fan_speed)
    {
    case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_AUTO:
        return (uint8_t)HF_ZXK_MODBUS_FAN_SPEED_AUTO;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_LOW:
        return (uint8_t)HF_ZXK_MODBUS_FAN_SPEED_LOW;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_MEDIUM:
        return (uint8_t)HF_ZXK_MODBUS_FAN_SPEED_MEDIUM;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_HIGH:
        return (uint8_t)HF_ZXK_MODBUS_FAN_SPEED_HIGH;

    default:
        return (uint8_t)HF_ZXK_MODBUS_FAN_SPEED_INVALID;
    }
}

/**
 * @brief 组一帧 0x06 写单寄存器控制帧并入队（由发送调度统一发出）
 */
static void hf_zxk_modbus_queue_write_register(uint8_t slave_addr,
                                               uint16_t reg_addr,
                                               uint16_t reg_value)
{
    uint8_t pdu[HF_ZXK_MODBUS_PDU_LEN];

    pdu[0] = slave_addr;
    pdu[1] = (uint8_t)HF_ZXK_MODBUS_FUNC_WRITE_SINGLE;
    pdu[2] = (uint8_t)(reg_addr >> 8);
    pdu[3] = (uint8_t)(reg_addr & 0xFFU);
    pdu[4] = (uint8_t)(reg_value >> 8);
    pdu[5] = (uint8_t)(reg_value & 0xFFU);

    hf_zxk_modbus_tx_queue_push(pdu);
}

/**
 * @brief 空调上报回调：把 HOOCH 空调帧转成线控网关的 0x06 控制帧
 * @note  只承接 Send*AddrHfWired485 系列接口发起的单项上报，
 *        frame->address 即 485 从机地址（485_1 拨码 0~15 / 485_2 固定 1）
 */
static void hf_zxk_modbus_air_conditioner_report_callback(
    const HOOCH_PROTOCOL_AirConditionerFrame_t *frame)
{
    uint8_t slave_addr;
    uint8_t mode;
    uint8_t fan_speed;

    if (frame == NULL)
    {
        return;
    }

    slave_addr = (uint8_t)frame->address;

    switch (frame->control_item)
    {
    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_POWER:
        hf_zxk_modbus_queue_write_register(slave_addr,
                                           (uint16_t)HF_ZXK_MODBUS_REG_CONTROL_POWER,
                                           (uint16_t)frame->power);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MODE:
        mode = hf_zxk_modbus_map_ac_mode(frame->mode);
        if (mode == (uint8_t)HF_ZXK_MODBUS_MODE_INVALID)
        {
            HF_ZXK_MODBUS_LOG_WARN("[MODBUS] unsupported mode: %u\r\n",
                                   (unsigned int)frame->mode);
            break;
        }

        hf_zxk_modbus_queue_write_register(slave_addr,
                                           (uint16_t)HF_ZXK_MODBUS_REG_CONTROL_MODE,
                                           (uint16_t)mode);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_FAN_SPEED:
        fan_speed = hf_zxk_modbus_map_ac_fan_speed(frame->fan_speed);
        if (fan_speed == (uint8_t)HF_ZXK_MODBUS_FAN_SPEED_INVALID)
        {
            HF_ZXK_MODBUS_LOG_WARN("[MODBUS] unsupported fan speed: %u\r\n",
                                   (unsigned int)frame->fan_speed);
            break;
        }

        hf_zxk_modbus_queue_write_register(slave_addr,
                                           (uint16_t)HF_ZXK_MODBUS_REG_CONTROL_FAN_SPEED,
                                           (uint16_t)fan_speed);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMPERATURE:
        hf_zxk_modbus_queue_write_register(slave_addr,
                                           (uint16_t)HF_ZXK_MODBUS_REG_CONTROL_TEMP,
                                           frame->temperature);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_HF_WIRED_CONTROLLER_485_ENABLE:
        /* 本模块使能开关：value 0=关闭 1=打开，不产生 485 收发 */
        hf_zxk_modbus_set_enable((frame->value != 0U) ? 1U : 0U);
        break;

    default:
        HF_ZXK_MODBUS_LOG_WARN("[MODBUS] unsupported control item: %u\r\n",
                               (unsigned int)frame->control_item);
        break;
    }
}

/* ---------------- 查询（MCU -> 网关 0x03 读保持寄存器） ---------------- */

/* 查询间隔：协议推荐 1~3 秒，取 1 秒 */
#define HF_ZXK_MODBUS_QUERY_INTERVAL_TICKS (1000U / HF_ZXK_MODBUS_LOOP_TICK_MS)

/* 性能查询固定查 3 个寄存器 */
#define HF_ZXK_MODBUS_QUERY_PERFORMANCE_REG_COUNT 3U

/* 内机状态查询取文档中最全的 9 个寄存器（含室温/权限/允许模式） */
#define HF_ZXK_MODBUS_QUERY_STATUS_REG_COUNT 9U

/* 周期查询目标地址：默认 1（协议文档示例地址，485_2 固定为 1）。
 * 不能取 0：MODBUS 地址 0 是广播地址，从机不会回复，查询会永远等不到应答。
 * 现场 485_1 带拨码或多内机时，由上层调用 hf_zxk_modbus_set_query_addr() 指定。 */
static uint8_t s_hf_zxk_modbus_query_addr = HF_ZXK_MODBUS_ADDR_PANEL_2;

/* 查询阶段：0-查性能（网关搜索空调阶段），1-查内机状态 */
static uint8_t s_hf_zxk_modbus_query_phase = 0U;

/* 距下次查询的倒计时节拍数：发任意帧（查询或控制）后都重置为查询间隔 */
static uint16_t s_hf_zxk_modbus_query_countdown_ticks = 0U;

/**
 * @brief 发送 0x03 读保持寄存器查询
 * @return 0-成功 1-参数非法或发送失败
 */
static uint8_t hf_zxk_modbus_send_read_holding(uint8_t slave_addr,
                                               uint16_t reg_addr,
                                               uint16_t reg_count)
{
    uint8_t pdu[HF_ZXK_MODBUS_PDU_LEN];

    if ((reg_count == 0U) || (reg_count > (uint16_t)HF_ZXK_MODBUS_MAX_REG_COUNT))
    {
        HF_ZXK_MODBUS_LOG_WARN("[MODBUS] invalid reg count: %u\r\n", (unsigned int)reg_count);
        return 1U;
    }

    pdu[0] = slave_addr;
    pdu[1] = (uint8_t)HF_ZXK_MODBUS_FUNC_READ_HOLDING;
    pdu[2] = (uint8_t)(reg_addr >> 8);
    pdu[3] = (uint8_t)(reg_addr & 0xFFU);
    pdu[4] = (uint8_t)(reg_count >> 8);
    pdu[5] = (uint8_t)(reg_count & 0xFFU);

    return hf_zxk_modbus_uart_send_pdu(pdu, HF_ZXK_MODBUS_PDU_LEN);
}

/**
 * @brief 查询性能（协议推荐上电后先查，网关搜索空调完毕后回复）
 */
uint8_t hf_zxk_modbus_query_performance(uint8_t slave_addr)
{
    return hf_zxk_modbus_send_read_holding(slave_addr,
                                           (uint16_t)HF_ZXK_MODBUS_REG_PERFORMANCE,
                                           (uint16_t)HF_ZXK_MODBUS_QUERY_PERFORMANCE_REG_COUNT);
}

/**
 * @brief 查询单台内机状态
 * @param reg_count 寄存器数量，文档取值为 6 / 8 / 9
 */
uint8_t hf_zxk_modbus_query_unit_status(uint8_t slave_addr, uint16_t reg_count)
{
    return hf_zxk_modbus_send_read_holding(slave_addr,
                                           (uint16_t)HF_ZXK_MODBUS_REG_UNIT_STATUS,
                                           reg_count);
}

/**
 * @brief 设置周期查询的目标地址
 */
void hf_zxk_modbus_set_query_addr(uint8_t slave_addr)
{
    s_hf_zxk_modbus_query_addr = slave_addr;
}

/**
 * @brief 业务层解析到性能回复后调用，把查询调度切到"内机状态轮询"阶段
 */
void hf_zxk_modbus_query_notify_performance_received(void)
{
    s_hf_zxk_modbus_query_phase = 1U;
    s_hf_zxk_modbus_query_countdown_ticks = 0U; /* 立即发一次内机状态查询 */
}

/**
 * @brief 发送调度：控制帧优先出队，查到查询周期则发查询
 * @note  控制帧与查询帧共用半双工 485 总线，统一在这里串行发出：
 *        - 任意两帧之间至少间隔 HF_ZXK_MODBUS_TX_MIN_GAP_TICKS（协议要求 06 帧间隔 >100ms）
 *        - 有控制帧排队时查询让位
 *        - 查询按 HF_ZXK_MODBUS_QUERY_INTERVAL_TICKS 的倒计时触发，
 *          每发一帧（查询或控制）倒计时都重置，因此控制后必然是完整的一个查询周期
 *          （≥300ms）之后才发 03 查结果，符合协议推荐时序
 *        - 控制帧连续下发导致倒计时被反复重置时，查询最迟推迟
 *          HF_ZXK_MODBUS_QUERY_MAX_DELAY_TICKS（3 秒）后强制发出
 */
static void hf_zxk_modbus_tx_schedule(void)
{
    /* 时间节拍推进（与总线是否空闲无关，保证查询周期计时准确） */
    if (s_hf_zxk_modbus_since_tx_ticks < 0xFFFFU)
    {
        s_hf_zxk_modbus_since_tx_ticks++;
    }

    if (s_hf_zxk_modbus_since_query_ticks < 0xFFFFU)
    {
        s_hf_zxk_modbus_since_query_ticks++;
    }

    if (s_hf_zxk_modbus_query_countdown_ticks > 0U)
    {
        s_hf_zxk_modbus_query_countdown_ticks--;
    }

    /* 帧间隔保护：总线上刚有帧发出 */
    if (s_hf_zxk_modbus_since_tx_ticks < HF_ZXK_MODBUS_TX_MIN_GAP_TICKS)
    {
        return;
    }

    /* 控制帧优先出队，且让查询重新计时 */
    if (s_hf_zxk_modbus_tx_count != 0U)
    {
        if (hf_zxk_modbus_uart_send_pdu(s_hf_zxk_modbus_tx_queue[s_hf_zxk_modbus_tx_tail],
                                        HF_ZXK_MODBUS_PDU_LEN) != 0U)
        {
            HF_ZXK_MODBUS_LOG_WARN("[MODBUS] send control frame failed\r\n");
        }

        s_hf_zxk_modbus_tx_tail =
            (uint8_t)((s_hf_zxk_modbus_tx_tail + 1U) % HF_ZXK_MODBUS_TX_QUEUE_DEPTH);
        s_hf_zxk_modbus_tx_count--;
        s_hf_zxk_modbus_since_tx_ticks = 0U;
        s_hf_zxk_modbus_query_countdown_ticks = HF_ZXK_MODBUS_QUERY_INTERVAL_TICKS;
        return;
    }

    /* 正常按倒计时触发；若控制帧反复重置倒计时，最迟推迟到
       HF_ZXK_MODBUS_QUERY_MAX_DELAY_TICKS 时强制发一次查询 */
    if ((s_hf_zxk_modbus_query_countdown_ticks > 0U) &&
        (s_hf_zxk_modbus_since_query_ticks < HF_ZXK_MODBUS_QUERY_MAX_DELAY_TICKS))
    {
        return;
    }

    if (s_hf_zxk_modbus_query_phase == 0U)
    {
        /* 网关搜索空调阶段：反复查性能 */
        if (hf_zxk_modbus_query_performance(s_hf_zxk_modbus_query_addr) != 0U)
        {
            HF_ZXK_MODBUS_LOG_WARN("[MODBUS] query performance failed\r\n");
        }
    }
    else
    {
        if (hf_zxk_modbus_query_unit_status(s_hf_zxk_modbus_query_addr,
                                            (uint16_t)HF_ZXK_MODBUS_QUERY_STATUS_REG_COUNT) != 0U)
        {
            HF_ZXK_MODBUS_LOG_WARN("[MODBUS] query unit status failed\r\n");
        }
    }

    s_hf_zxk_modbus_since_tx_ticks = 0U;
    s_hf_zxk_modbus_since_query_ticks = 0U;
    s_hf_zxk_modbus_query_countdown_ticks = HF_ZXK_MODBUS_QUERY_INTERVAL_TICKS;
}

/* ---------------- UART 收发 ---------------- */

/**
 * @brief 复位收发残留状态：清空接收队列/解析器/待发控制帧，发帧计时恢复成上电状态
 */
static void hf_zxk_modbus_reset_transport_state(void)
{
    hf_zxk_modbus_ring_buffer_init(&s_hf_zxk_modbus_rx_ring_buffer);
    hf_zxk_modbus_parser_init(&s_hf_zxk_modbus_frame_parser);
    s_hf_zxk_modbus_parser_timeout_ticks = 0U;

    s_hf_zxk_modbus_tx_head  = 0U;
    s_hf_zxk_modbus_tx_tail  = 0U;
    s_hf_zxk_modbus_tx_count = 0U;

    /* 重新按上电流程计时：上电即进入"查性能"阶段，发帧节拍置满 */
    s_hf_zxk_modbus_query_phase           = 0U;
    s_hf_zxk_modbus_query_countdown_ticks = 0U;
    s_hf_zxk_modbus_since_tx_ticks        = 0xFFFFU;
    s_hf_zxk_modbus_since_query_ticks     = 0xFFFFU;
}

/**
 * @brief 开关本模块
 * @param enable 1=开启 0=关闭
 * @note  关闭时接收队列不再接收任何字节（已缓存的也一并清空），帧解析与所有发送
 *        （周期查询 / 控制帧 / 手动查询）同时停止；重新开启后按上电流程重新计时。
 */
void hf_zxk_modbus_set_enable(uint8_t enable)
{
    uint8_t new_state = (enable != 0U) ? 1U : 0U;

    if (new_state == s_hf_zxk_modbus_enabled)
    {
        return;
    }

    s_hf_zxk_modbus_enabled = new_state;
    hf_zxk_modbus_reset_transport_state();

    HF_ZXK_MODBUS_LOG_INFO("[MODBUS] module %s\r\n",
                           (new_state != 0U) ? "enabled" : "disabled");
}

/**
 * @brief 读取当前使能状态
 * @return 1-开启 0-关闭
 */
uint8_t hf_zxk_modbus_get_enable(void)
{
    return s_hf_zxk_modbus_enabled;
}

/**
 * @brief 线控 MODBUS 模块初始化
 */
void hf_zxk_modbus_uart_init(void)
{
    s_hf_zxk_modbus_enabled = (uint8_t)HF_ZXK_MODBUS_ENABLE_DEFAULT;
    hf_zxk_modbus_reset_transport_state();

    /* 注册空调上报回调：Send*AddrHfWired485 发起的控制经此转成 0x06 帧下发给线控网关 */
    HOOCH_PROTOCOL_AirConditioner_RegisterReportCallbackHfWired485(
        hf_zxk_modbus_air_conditioner_report_callback);

    HF_ZXK_MODBUS_LOG_INFO("[MODBUS] UART initialized\r\n");
}

/**
 * @brief 将接收到的字节写入环形缓冲区（在 USART2 中断中调用）
 */
void hf_zxk_modbus_uart_rxcallback(uint8_t data)
{
    /* 模块关闭：接收队列不接收任何消息 */
    if (s_hf_zxk_modbus_enabled == 0U)
    {
        return;
    }

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

    /* 模块关闭：不解析也不发帧 */
    if (s_hf_zxk_modbus_enabled == 0U)
    {
        return;
    }

    while (hf_zxk_modbus_ring_buffer_pop(&s_hf_zxk_modbus_rx_ring_buffer, &byte) == 0U)
    {
        /* 调试用：打印解析前的原始字节 */
    //    HF_ZXK_MODBUS_LOG_INFO("[MODBUS] RX byte: 0x%02X\r\n", (unsigned int)byte);

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

    /* 发送调度：控制帧优先，空闲时按 1 秒周期查内机状态 */
    hf_zxk_modbus_tx_schedule();
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

    /* 模块关闭：不收也不发 */
    if (s_hf_zxk_modbus_enabled == 0U)
    {
        HF_ZXK_MODBUS_LOG_WARN("[MODBUS] module disabled, drop tx\r\n");
        return 1U;
    }

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

void hf_zxk_modbus_set_enable(uint8_t enable)
{
    (void)enable;
}

uint8_t hf_zxk_modbus_get_enable(void)
{
    return 0U;
}

uint8_t hf_zxk_modbus_query_performance(uint8_t slave_addr)
{
    (void)slave_addr;
    return 1U;
}

uint8_t hf_zxk_modbus_query_unit_status(uint8_t slave_addr, uint16_t reg_count)
{
    (void)slave_addr;
    (void)reg_count;
    return 1U;
}

void hf_zxk_modbus_set_query_addr(uint8_t slave_addr)
{
    (void)slave_addr;
}

void hf_zxk_modbus_query_notify_performance_received(void)
{
}

#endif /* HOOCH_PROTOCOL_ENABLE_HF_WIRED_CONTROLLER_485 */
