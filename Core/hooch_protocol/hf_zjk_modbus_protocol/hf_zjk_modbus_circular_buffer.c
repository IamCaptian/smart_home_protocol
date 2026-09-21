#include "hf_zjk_modbus_circular_buffer.h"
#include "hf_zjk_modbus_handle.h"
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
static uint8_t                    s_hf_zjk_modbus_enabled = (uint8_t)HF_ZJK_MODBUS_ENABLE_DEFAULT; /* 模块使能: 1=开启 0=关闭 */

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

/* ---------------- 空调上报（HOOCH -> 集控网关） ---------------- */

/* 下行控制帧 PDU 长度：网关地址 + 功能码 + 控制值(1 字节) + 设备数量 + 设备地址(2 字节) */
#define HF_ZJK_MODBUS_CTRL_PDU_LEN 6U

/* 控制帧的设备数量：单项控制恒为单台 */
#define HF_ZJK_MODBUS_CTRL_DEVICE_COUNT 1U

/* 查询帧 PDU 长度（状态查询 / 性能查询）：网关地址 + 功能码 + 控制值(1 字节) + 设备数量 + 设备地址(2 字节) */
#define HF_ZJK_MODBUS_QUERY_PDU_LEN 6U

/* 状态查询的设备数量：查询单台内机固定为 1 台 */
#define HF_ZJK_MODBUS_QUERY_SINGLE_DEVICE_COUNT 1U

/* ---------------- 发送队列（控制与周期查询共用半双工 485 总线） ---------------- */

/* 主循环节拍约 20ms（main.c 中 HAL_Delay(20)），用于把协议时间换算成节拍数 */
#define HF_ZJK_MODBUS_LOOP_TICK_MS 20U

/* 任意两帧之间的最小间隔：仿线控器，连续发帧间隔 >100ms */
#define HF_ZJK_MODBUS_TX_MIN_GAP_TICKS (100U / HF_ZJK_MODBUS_LOOP_TICK_MS)

/* 内机状态查询周期：每秒查询一次 */
#define HF_ZJK_MODBUS_QUERY_INTERVAL_TICKS (1000U / HF_ZJK_MODBUS_LOOP_TICK_MS)

/* 上电后首次查询延时：5 秒后开始获取外机地址 */
#define HF_ZJK_MODBUS_FIRST_QUERY_DELAY_TICKS (5000U / HF_ZJK_MODBUS_LOOP_TICK_MS)

/* 查询最大推迟时间：控制帧会不断重置查询倒计时，用这个上限保证查询最迟每 3 秒仍会发出一次 */
#define HF_ZJK_MODBUS_QUERY_MAX_DELAY_TICKS (3000U / HF_ZJK_MODBUS_LOOP_TICK_MS)

/* 待发控制帧队列深度：平台侧一次全量控制会连续下发多项（开关/模式/风速/温度） */
#define HF_ZJK_MODBUS_TX_QUEUE_DEPTH 4U

/* 控制帧待发队列：上报回调只入队，由主循环节拍统一出队，避免与查询抢总线 */
static uint8_t s_hf_zjk_modbus_tx_queue[HF_ZJK_MODBUS_TX_QUEUE_DEPTH][HF_ZJK_MODBUS_CTRL_PDU_LEN];
static uint8_t s_hf_zjk_modbus_tx_head  = 0U;
static uint8_t s_hf_zjk_modbus_tx_tail  = 0U;
static uint8_t s_hf_zjk_modbus_tx_count = 0U;

/**
 * @brief 控制帧入队（队列满时丢弃最旧一帧，保证最新控制不丢）
 */
static void hf_zjk_modbus_tx_queue_push(const uint8_t *pdu)
{
    if (s_hf_zjk_modbus_tx_count >= HF_ZJK_MODBUS_TX_QUEUE_DEPTH)
    {
        s_hf_zjk_modbus_tx_tail =
            (uint8_t)((s_hf_zjk_modbus_tx_tail + 1U) % HF_ZJK_MODBUS_TX_QUEUE_DEPTH);
        s_hf_zjk_modbus_tx_count--;
        HF_ZJK_MODBUS_LOG_WARN("[ZJK] tx queue full, drop oldest\r\n");
    }

    (void)memcpy(s_hf_zjk_modbus_tx_queue[s_hf_zjk_modbus_tx_head], pdu, HF_ZJK_MODBUS_CTRL_PDU_LEN);
    s_hf_zjk_modbus_tx_head =
        (uint8_t)((s_hf_zjk_modbus_tx_head + 1U) % HF_ZJK_MODBUS_TX_QUEUE_DEPTH);
    s_hf_zjk_modbus_tx_count++;
}

/* 当前内机地址（云端下发，供与外机地址拼成设备地址查询）；0xFFFF 表示未设置 */
static uint16_t s_hf_zjk_modbus_current_ac_addr = 0xFFFF;

/**
 * @brief 读取当前内机地址
 */
uint16_t hf_zjk_modbus_get_current_ac_addr(void)
{
    return s_hf_zjk_modbus_current_ac_addr;
}

/**
 * @brief 直接设置当前内机地址（供不需走空调上报回调的调用方使用）
 * @param address 内机地址；0xFFFF 表示未设置
 * @note  周期查询单台内机状态时，与外机地址拼成 (外机<<8 | 内机) 作为设备地址
 */
void hf_zjk_modbus_set_current_ac_addr(uint16_t address)
{
    s_hf_zjk_modbus_current_ac_addr = address;
    HF_ZJK_MODBUS_LOG_INFO("[ZJK] inner address set: %u\r\n",
                           (unsigned int)(address & 0xFFU));
}

/* HOOCH 空调模式 -> 协议模式值（与 0x33 控制值一致，地暖等品牌专属模式无对应项） */
static uint8_t hf_zjk_modbus_map_ac_mode(HOOCH_PROTOCOL_AirConditionerMode_t mode)
{
    switch (mode)
    {
    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_AUTO:
        return (uint8_t)HF_ZJK_MODBUS_MODE_AUTO;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_COOL:
        return (uint8_t)HF_ZJK_MODBUS_MODE_COOL;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_DRY:
        return (uint8_t)HF_ZJK_MODBUS_MODE_DRY;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_FAN:
        return (uint8_t)HF_ZJK_MODBUS_MODE_FAN;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_HEAT:
        return (uint8_t)HF_ZJK_MODBUS_MODE_HEAT;

    default:
        return (uint8_t)HF_ZJK_MODBUS_MODE_UNKNOWN;
    }
}

/* HOOCH 空调风速 -> 协议风速值（与 0x34 控制值一致，只发自动/低/中/高四档） */
static uint8_t hf_zjk_modbus_map_ac_fan_speed(
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed)
{
    switch (fan_speed)
    {
    case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_AUTO:
        return (uint8_t)HF_ZJK_MODBUS_FAN_AUTO;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_LOW:
        return (uint8_t)HF_ZJK_MODBUS_FAN_LOW;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_MEDIUM:
        return (uint8_t)HF_ZJK_MODBUS_FAN_MEDIUM;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_HIGH:
        return (uint8_t)HF_ZJK_MODBUS_FAN_HIGH;

    default:
        return (uint8_t)HF_ZJK_MODBUS_FAN_UNKNOWN;
    }
}

/**
 * @brief 组一帧单项控制指令并入队（校验和与发送由调度统一完成）
 * @param function 功能码：0x31 开关 / 0x32 设定温度 / 0x33 模式 / 0x34 风速
 * @param control  控制值
 * @param address  设备地址 = (室外机地址 << 8) | 室内机地址
 * @note  只入队不立即发帧，由 hf_zjk_modbus_tx_schedule() 按 100ms 帧间隔串行发出，
 *        避免多条控制指令挤在同一时刻抢占总线
 */
static void hf_zjk_modbus_queue_control(uint8_t function, uint8_t control, uint16_t address)
{
    uint8_t pdu[HF_ZJK_MODBUS_CTRL_PDU_LEN];

    pdu[0] = (uint8_t)HF_ZJK_MODBUS_DEFAULT_GATEWAY_ADDR;
    pdu[1] = function;
    pdu[2] = control;
    pdu[3] = (uint8_t)HF_ZJK_MODBUS_CTRL_DEVICE_COUNT;
    pdu[4] = (uint8_t)(address >> HF_ZJK_MODBUS_DEV_ADDR_OUTER_SHIFT);
    pdu[5] = (uint8_t)(address & 0xFFU);

    hf_zjk_modbus_tx_queue_push(pdu);
}

/**
 * @brief 组一帧"查询单台内机状态"指令并发出（校验和由发送接口统一追加）
 * @param address 设备地址 = (室外机地址 << 8) | 室内机地址
 * @note  功能码 0x50 + 控制值 0x01(查询单台) + 设备数量 1，网关回复由 hf_zjk_modbus_handle_frame 解析
 */
static void hf_zjk_modbus_send_query_status(uint16_t address)
{
    uint8_t pdu[HF_ZJK_MODBUS_QUERY_PDU_LEN];

    pdu[0] = (uint8_t)HF_ZJK_MODBUS_DEFAULT_GATEWAY_ADDR;
    pdu[1] = (uint8_t)HF_ZJK_MODBUS_FUNC_AC_QUERY_STATUS;
    pdu[2] = (uint8_t)HF_ZJK_MODBUS_QUERY_SINGLE;
    pdu[3] = (uint8_t)HF_ZJK_MODBUS_QUERY_SINGLE_DEVICE_COUNT;
    pdu[4] = (uint8_t)(address >> HF_ZJK_MODBUS_DEV_ADDR_OUTER_SHIFT);
    pdu[5] = (uint8_t)(address & 0xFFU);

    if (hf_zjk_modbus_uart_send_pdu(pdu, (uint16_t)HF_ZJK_MODBUS_QUERY_PDU_LEN) != 0U)
    {
        HF_ZJK_MODBUS_LOG_WARN("[ZJK] send status query failed: outer=%u inner=%u\r\n",
                               (unsigned int)(address >> HF_ZJK_MODBUS_DEV_ADDR_OUTER_SHIFT),
                               (unsigned int)(address & 0xFFU));
    }
}

/**
 * @brief 组一帧"查询全部内机状态"指令并发出（校验和由发送接口统一追加）
 * @note  功能码 0x50 + 控制值 0xFF(全部)，设备数量与设备地址域均填 0xFF，
 *        对应协议举例 01 50 FF FF FF FF 4D；网关回传每台内机的
 *        外机地址/内机地址与完整状态，可用于发现外机地址
 */
static void hf_zjk_modbus_send_query_all_status(void)
{
    uint8_t pdu[HF_ZJK_MODBUS_QUERY_PDU_LEN];

    pdu[0] = (uint8_t)HF_ZJK_MODBUS_DEFAULT_GATEWAY_ADDR;
    pdu[1] = (uint8_t)HF_ZJK_MODBUS_FUNC_AC_QUERY_STATUS;
    pdu[2] = (uint8_t)HF_ZJK_MODBUS_QUERY_ALL;
    pdu[3] = (uint8_t)HF_ZJK_MODBUS_ADDR_GROUP_ALL;
    pdu[4] = (uint8_t)HF_ZJK_MODBUS_ADDR_GROUP_ALL;
    pdu[5] = (uint8_t)HF_ZJK_MODBUS_ADDR_GROUP_ALL;

    if (hf_zjk_modbus_uart_send_pdu(pdu, (uint16_t)HF_ZJK_MODBUS_QUERY_PDU_LEN) != 0U)
    {
        HF_ZJK_MODBUS_LOG_WARN("[ZJK] send all status query failed\r\n");
    }
}

/**
 * @brief 组一帧"查询空调性能"指令并发出（校验和由发送接口统一追加）
 * @note  功能码 0x90 + 控制值 0xFF(全部)，设备数量与设备地址域均填 0xFF，
 *        对应协议举例 01 90 FF FF FF FF 8D；网关回复由 hf_zjk_modbus_handle_frame 解析
 */
static void hf_zjk_modbus_send_query_performance(void)
{
    uint8_t pdu[HF_ZJK_MODBUS_QUERY_PDU_LEN];

    pdu[0] = (uint8_t)HF_ZJK_MODBUS_DEFAULT_GATEWAY_ADDR;
    pdu[1] = (uint8_t)HF_ZJK_MODBUS_FUNC_AC_QUERY_PERFORMANCE;
    pdu[2] = (uint8_t)HF_ZJK_MODBUS_QUERY_ALL;
    pdu[3] = (uint8_t)HF_ZJK_MODBUS_ADDR_GROUP_ALL;
    pdu[4] = (uint8_t)HF_ZJK_MODBUS_ADDR_GROUP_ALL;
    pdu[5] = (uint8_t)HF_ZJK_MODBUS_ADDR_GROUP_ALL;

    if (hf_zjk_modbus_uart_send_pdu(pdu, (uint16_t)HF_ZJK_MODBUS_QUERY_PDU_LEN) != 0U)
    {
        HF_ZJK_MODBUS_LOG_WARN("[ZJK] send performance query failed\r\n");
    }
}

/**
 * @brief 空调上报回调：把 HOOCH 空调帧转成集控网关的控制/查询指令
 * @note  承接 Send*AddrHfCentralControl485（控制）与 QueryStatusAddrHfCentralControl485（查询）；
 *        frame->address 即设备地址（室外机地址 << 8 | 室内机地址）
 */
static void hf_zjk_modbus_air_conditioner_report_callback(
    const HOOCH_PROTOCOL_AirConditionerFrame_t *frame)
{
    uint8_t value;

    if (frame == NULL)
    {
        return;
    }

    switch (frame->control_item)
    {
    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_POWER:
        /* HOOCH 与协议的开/关取值一致(0=关 1=开) */
        hf_zjk_modbus_queue_control((uint8_t)HF_ZJK_MODBUS_FUNC_AC_POWER,
                                    (uint8_t)frame->power,
                                    frame->address);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MODE:
        value = hf_zjk_modbus_map_ac_mode(frame->mode);
        if (value == (uint8_t)HF_ZJK_MODBUS_MODE_UNKNOWN)
        {
            HF_ZJK_MODBUS_LOG_WARN("[ZJK] unsupported mode: %u\r\n",
                                   (unsigned int)frame->mode);
            break;
        }

        hf_zjk_modbus_queue_control((uint8_t)HF_ZJK_MODBUS_FUNC_AC_MODE, value, frame->address);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_FAN_SPEED:
        value = hf_zjk_modbus_map_ac_fan_speed(frame->fan_speed);
        if (value == (uint8_t)HF_ZJK_MODBUS_FAN_UNKNOWN)
        {
            HF_ZJK_MODBUS_LOG_WARN("[ZJK] unsupported fan speed: %u\r\n",
                                   (unsigned int)frame->fan_speed);
            break;
        }

        hf_zjk_modbus_queue_control((uint8_t)HF_ZJK_MODBUS_FUNC_AC_FAN_SPEED,
                                    value,
                                    frame->address);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMPERATURE:
        hf_zjk_modbus_queue_control((uint8_t)HF_ZJK_MODBUS_FUNC_AC_TEMP,
                                    (uint8_t)frame->temperature,
                                    frame->address);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_CENTRAL_CONTROL_ADDRESS:
        /* 设置当前设备内机地址：只记录，供后续查询/控制使用 */
        hf_zjk_modbus_set_current_ac_addr(frame->address);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_QUERY_STATUS:
        /* 查询单台内机状态：按帧内地址发起 0x50 查询 */
        hf_zjk_modbus_send_query_status(frame->address);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_QUERY_ALL_STATUS:
        /* 查询全部内机状态：全局查询，回包含各内机的外机/内机地址 */
        hf_zjk_modbus_send_query_all_status();
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_QUERY_PERFORMANCE:
        /* 查询空调性能：全局查询，无需地址 */
        hf_zjk_modbus_send_query_performance();
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_HF_CENTRAL_CONTROL_485_ENABLE:
        /* 本模块使能开关：value 0=关闭 1=打开，不产生 485 收发 */
        hf_zjk_modbus_set_enable((frame->value != 0U) ? 1U : 0U);
        break;

    default:
        HF_ZJK_MODBUS_LOG_WARN("[ZJK] unsupported control item: %u\r\n",
                               (unsigned int)frame->control_item);
        break;
    }
}

/* ---------------- 周期查询调度（仿线控器） ---------------- */

/* 查询阶段：0-查询全部以获取外机地址，1-周期查询单台内机状态 */
static uint8_t s_hf_zjk_modbus_query_phase = 0U;

/* "查询全部"回复里得到的外机地址（制冷系统地址），用于与内机地址拼成设备地址 */
static uint8_t s_hf_zjk_modbus_outer_address = 0U;

/* 距上次发帧的节拍数（初值取满，上电即可发帧） */
static uint16_t s_hf_zjk_modbus_since_tx_ticks = 0xFFFFU;

/* 距上次发出查询帧的节拍数（0xFFFF 表示上电后尚未发过查询） */
static uint16_t s_hf_zjk_modbus_since_query_ticks = 0xFFFFU;

/* 距下次查询的倒计时节拍数 */
static uint16_t s_hf_zjk_modbus_query_countdown_ticks = 0U;

/**
 * @brief 业务层解析到"查询全部"回复(已拿到外机地址)后调用，切到内机状态轮询阶段
 * @param outer_address 回复里解析出的外机地址
 */
void hf_zjk_modbus_query_notify_outer_address_received(uint8_t outer_address)
{
    s_hf_zjk_modbus_outer_address = outer_address;
    s_hf_zjk_modbus_query_phase = 1U;
    s_hf_zjk_modbus_query_countdown_ticks = 0U; /* 立即发一次内机状态查询 */
}

/**
 * @brief 发送调度：控制帧优先出队，空闲时按 1 秒周期查询
 * @note  控制帧与查询帧共用半双工 485 总线，统一在这里串行发出：
 *        - 任意两帧之间至少间隔 HF_ZJK_MODBUS_TX_MIN_GAP_TICKS（100ms）
 *        - 有控制帧排队时查询让位
 *        - 上电后先等待 HF_ZJK_MODBUS_FIRST_QUERY_DELAY_TICKS(5 秒)，
 *          再周期查询全部以获取外机地址；拿到外机地址后切到单台内机状态轮询
 *        - 单台查询的设备地址 = (外机地址 << 8) | 内机地址（内机地址由平台下发）；
 *          内机地址为 0xFFFF(未设置)时跳过查询
 *        - 控制帧连续下发导致倒计时被反复重置时，查询最迟推迟
 *          HF_ZJK_MODBUS_QUERY_MAX_DELAY_TICKS（3 秒）后强制发出
 */
static void hf_zjk_modbus_tx_schedule(void)
{
    uint16_t inner_address;
    uint16_t address;

    /* 时间节拍推进（与总线是否空闲无关，保证查询周期计时准确） */
    if (s_hf_zjk_modbus_since_tx_ticks < 0xFFFFU)
    {
        s_hf_zjk_modbus_since_tx_ticks++;
    }

    if (s_hf_zjk_modbus_since_query_ticks < 0xFFFFU)
    {
        s_hf_zjk_modbus_since_query_ticks++;
    }

    if (s_hf_zjk_modbus_query_countdown_ticks > 0U)
    {
        s_hf_zjk_modbus_query_countdown_ticks--;
    }

    /* 帧间隔保护：总线上刚有帧发出 */
    if (s_hf_zjk_modbus_since_tx_ticks < HF_ZJK_MODBUS_TX_MIN_GAP_TICKS)
    {
        return;
    }

    /* 控制帧优先出队，且让查询重新计时 */
    if (s_hf_zjk_modbus_tx_count != 0U)
    {
        if (hf_zjk_modbus_uart_send_pdu(s_hf_zjk_modbus_tx_queue[s_hf_zjk_modbus_tx_tail],
                                        HF_ZJK_MODBUS_CTRL_PDU_LEN) != 0U)
        {
            HF_ZJK_MODBUS_LOG_WARN("[ZJK] send control frame failed\r\n");
        }

        s_hf_zjk_modbus_tx_tail =
            (uint8_t)((s_hf_zjk_modbus_tx_tail + 1U) % HF_ZJK_MODBUS_TX_QUEUE_DEPTH);
        s_hf_zjk_modbus_tx_count--;
        s_hf_zjk_modbus_since_tx_ticks = 0U;

        /* 首次查询(取外机地址)发出前不重置倒计时，保证上电 5 秒的起始延时不被控制帧打断 */
        if (s_hf_zjk_modbus_since_query_ticks != 0xFFFFU)
        {
            s_hf_zjk_modbus_query_countdown_ticks = HF_ZJK_MODBUS_QUERY_INTERVAL_TICKS;
        }
        return;
    }

    if (s_hf_zjk_modbus_query_countdown_ticks > 0U)
    {
        /* 上电后尚未发过查询(0xFFFF)：只按倒计时(首次 5 秒)触发，不受最大推迟约束 */
        if (s_hf_zjk_modbus_since_query_ticks == 0xFFFFU)
        {
            return;
        }

        /* 控制帧反复重置倒计时时，最迟推迟 MAX_DELAY 仍强制发一次查询 */
        if (s_hf_zjk_modbus_since_query_ticks < HF_ZJK_MODBUS_QUERY_MAX_DELAY_TICKS)
        {
            return;
        }
    }

    if (s_hf_zjk_modbus_query_phase == 0U)
    {
        /* 阶段 0：查询全部，用于获取外机地址 */
        hf_zjk_modbus_send_query_all_status();
    }
    else
    {
        /* 阶段 1：外机地址已获取，用 (外机<<8 | 内机) 组成设备地址查询单台内机状态；
           内机地址未下发(0xFFFF)时跳过 */
        inner_address = s_hf_zjk_modbus_current_ac_addr;
        if (inner_address != 0xFFFFU)
        {
            address = (uint16_t)(((uint16_t)s_hf_zjk_modbus_outer_address <<
                                  HF_ZJK_MODBUS_DEV_ADDR_OUTER_SHIFT) |
                                 (inner_address & 0xFFU));
            hf_zjk_modbus_send_query_status(address);
        }
        else
        {
            HF_ZJK_MODBUS_LOG_WARN("[ZJK] inner address not set, skip status query\r\n");
        }
    }

    s_hf_zjk_modbus_since_tx_ticks = 0U;
    s_hf_zjk_modbus_since_query_ticks = 0U;
    s_hf_zjk_modbus_query_countdown_ticks = HF_ZJK_MODBUS_QUERY_INTERVAL_TICKS;
}

/* ---------------- UART 收发 ---------------- */

/**
 * @brief 复位收发残留状态：清空接收队列/解析器/待发控制帧，发帧计时恢复成上电状态
 */
static void hf_zjk_modbus_reset_transport_state(void)
{
    hf_zjk_modbus_ring_buffer_init(&s_hf_zjk_modbus_rx_ring_buffer);
    hf_zjk_modbus_parser_init(&s_hf_zjk_modbus_frame_parser);
    s_hf_zjk_modbus_parser_timeout_ticks = 0U;

    s_hf_zjk_modbus_tx_head  = 0U;
    s_hf_zjk_modbus_tx_tail  = 0U;
    s_hf_zjk_modbus_tx_count = 0U;

    /* 重新按上电流程计时：先等待首次查询延时，再查询全部以获取外机地址 */
    s_hf_zjk_modbus_query_phase           = 0U;
    s_hf_zjk_modbus_query_countdown_ticks = HF_ZJK_MODBUS_FIRST_QUERY_DELAY_TICKS;
    s_hf_zjk_modbus_since_tx_ticks        = 0xFFFFU;
    s_hf_zjk_modbus_since_query_ticks     = 0xFFFFU;
}

/**
 * @brief 开关本模块
 * @param enable 1=开启 0=关闭
 * @note  关闭时接收队列不再接收任何字节（已缓存的也一并清空），帧解析与所有发送
 *        （周期查询 / 控制帧 / 手动查询）同时停止；重新开启后按上电流程重新计时。
 */
void hf_zjk_modbus_set_enable(uint8_t enable)
{
    uint8_t new_state = (enable != 0U) ? 1U : 0U;

    if (new_state == s_hf_zjk_modbus_enabled)
    {
        return;
    }

    s_hf_zjk_modbus_enabled = new_state;
    hf_zjk_modbus_reset_transport_state();

    HF_ZJK_MODBUS_LOG_INFO("[ZJK] module %s\r\n",
                           (new_state != 0U) ? "enabled" : "disabled");
}

/**
 * @brief 读取当前使能状态
 * @return 1-开启 0-关闭
 */
uint8_t hf_zjk_modbus_get_enable(void)
{
    return s_hf_zjk_modbus_enabled;
}

/**
 * @brief 集控 自定义 485 模块初始化
 */
void hf_zjk_modbus_uart_init(void)
{
    s_hf_zjk_modbus_enabled = (uint8_t)HF_ZJK_MODBUS_ENABLE_DEFAULT;
    hf_zjk_modbus_reset_transport_state();

    /* 注册空调上报回调：Send*AddrHfCentralControl485 发起的控制经此转成控制帧下发给集控网关 */
    HOOCH_PROTOCOL_AirConditioner_RegisterReportCallbackHfCentralControl485(
        hf_zjk_modbus_air_conditioner_report_callback);

    HF_ZJK_MODBUS_LOG_INFO("[ZJK] UART initialized\r\n");
}

/**
 * @brief 将接收到的字节写入环形缓冲区（在 USART2 中断中调用）
 */
void hf_zjk_modbus_uart_rxcallback(uint8_t data)
{
    /* 模块关闭：接收队列不接收任何消息 */
    if (s_hf_zjk_modbus_enabled == 0U)
    {
        return;
    }

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

    /* 模块关闭：不解析也不发帧 */
    if (s_hf_zjk_modbus_enabled == 0U)
    {
        return;
    }

    while (hf_zjk_modbus_ring_buffer_pop(&s_hf_zjk_modbus_rx_ring_buffer, &byte) == 0U)
    {
         //HF_ZJK_MODBUS_LOG_INFO("[MODBUS] RX byte: 0x%02X\r\n", (unsigned int)byte);
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

    /* 发送调度：控制帧优先，空闲时 5 秒后查外机地址、之后每秒查内机状态 */
    hf_zjk_modbus_tx_schedule();
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

    /* 模块关闭：不收也不发 */
    if (s_hf_zjk_modbus_enabled == 0U)
    {
        HF_ZJK_MODBUS_LOG_WARN("[ZJK] module disabled, drop tx\r\n");
        return 1U;
    }

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

void hf_zjk_modbus_set_enable(uint8_t enable)
{
    (void)enable;
}

uint8_t hf_zjk_modbus_get_enable(void)
{
    return 0U;
}

void hf_zjk_modbus_query_notify_outer_address_received(uint8_t outer_address)
{
    (void)outer_address;
}

uint16_t hf_zjk_modbus_get_current_ac_addr(void)
{
    return 0xFFFFU;
}

void hf_zjk_modbus_set_current_ac_addr(uint16_t address)
{
    (void)address;
}

#endif /* HOOCH_PROTOCOL_ENABLE_HF_CENTRAL_CONTROL_485 */
