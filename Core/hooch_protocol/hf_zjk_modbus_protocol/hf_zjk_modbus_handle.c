#include "hf_zjk_modbus_handle.h"
#include "hooch_protocol_common.h"

#if HOOCH_PROTOCOL_ENABLE_HF_CENTRAL_CONTROL_485

/*
 * 状态查询回复的数据域长度与设备数量一一对应（详见 hf_zjk_modbus_circular_buffer.h 的帧格式说明），
 * 因此这里按"整帧长度 / 单台长度"反推实际台数，避免依赖网关是否如实填写设备数量域。
 */

/* 协议模式 -> HOOCH 空调模式：
 * 地暖/强热同属制热，一并映射为制热；清爽/自动除湿/贴心睡眠等品牌专属模式无对应项。 */
static HOOCH_PROTOCOL_AirConditionerMode_t hf_zjk_modbus_map_mode(hf_zjk_modbus_mode_t mode)
{
    switch (mode)
    {
    case HF_ZJK_MODBUS_MODE_AUTO:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_AUTO;
    case HF_ZJK_MODBUS_MODE_COOL:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_COOL;
    case HF_ZJK_MODBUS_MODE_DRY:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_DRY;
    case HF_ZJK_MODBUS_MODE_FAN:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_FAN;
    case HF_ZJK_MODBUS_MODE_HEAT:
    case HF_ZJK_MODBUS_MODE_FLOOR_HEAT:
    case HF_ZJK_MODBUS_MODE_HEAT_BOOST:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_HEAT;
    default:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_INVALID;
    }
}

/* 协议风速 -> HOOCH 空调风速：
 * HOOCH 只有低/中/高三档，协议的中高速、中低速、极低速、超高速按最接近的档位归并。 */
static HOOCH_PROTOCOL_AirConditionerFanSpeed_t hf_zjk_modbus_map_fan_speed(
    hf_zjk_modbus_fan_speed_t fan_speed)
{
    switch (fan_speed)
    {
    case HF_ZJK_MODBUS_FAN_AUTO:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_AUTO;
    case HF_ZJK_MODBUS_FAN_HIGH:
    case HF_ZJK_MODBUS_FAN_ULTRA_HIGH:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_HIGH;
    case HF_ZJK_MODBUS_FAN_MEDIUM:
    case HF_ZJK_MODBUS_FAN_MEDIUM_HIGH:
    case HF_ZJK_MODBUS_FAN_MEDIUM_LOW:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_MEDIUM;
    case HF_ZJK_MODBUS_FAN_LOW:
    case HF_ZJK_MODBUS_FAN_ULTRA_LOW:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_LOW;
    default:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_INVALID;
    }
}

/**
 * @brief 解析一台空调的状态数据块
 * @param frame 0x50 / 0x53 状态回复帧
 * @param base  该设备数据块在 frame->data 中的起始偏移
 */
static void hf_zjk_modbus_parse_ac_status(const hf_zjk_modbus_frame_t *frame,
                                          uint16_t base,
                                          hf_zjk_modbus_ac_status_t *status)
{
    const uint8_t *data = &frame->data[base];

    status->outer_address = data[HF_ZJK_MODBUS_DEV_OFFSET_OUTER];
    status->inner_address = data[HF_ZJK_MODBUS_DEV_OFFSET_INNER];
    status->power         = (hf_zjk_modbus_power_t)data[HF_ZJK_MODBUS_DEV_OFFSET_POWER];

    if (frame->function == (uint8_t)HF_ZJK_MODBUS_FUNC_AC_QUERY_STATUS_DEC)
    {
        /* 0x53：设定温度与房间温度各占 2 字节(整数 + 小数)，其后字段整体后移 2 */
        status->set_temperature       = data[HF_ZJK_MODBUS_DEV_DEC_OFFSET_SET_TEMP_INT];
        status->set_temperature_frac  = data[HF_ZJK_MODBUS_DEV_DEC_OFFSET_SET_TEMP_FRAC];
        status->mode                  = (hf_zjk_modbus_mode_t)data[HF_ZJK_MODBUS_DEV_DEC_OFFSET_MODE];
        status->fan_speed             = (hf_zjk_modbus_fan_speed_t)data[HF_ZJK_MODBUS_DEV_DEC_OFFSET_FAN_SPEED];
        status->room_temperature      = data[HF_ZJK_MODBUS_DEV_DEC_OFFSET_ROOM_TEMP_INT];
        status->room_temperature_frac = data[HF_ZJK_MODBUS_DEV_DEC_OFFSET_ROOM_TEMP_FRAC];
        status->fault_code            = data[HF_ZJK_MODBUS_DEV_DEC_OFFSET_FAULT];
        status->wind_direction        = data[HF_ZJK_MODBUS_DEV_DEC_OFFSET_WIND];
        status->other_info            = data[HF_ZJK_MODBUS_DEV_DEC_OFFSET_OTHER];
    }
    else
    {
        status->set_temperature       = data[HF_ZJK_MODBUS_DEV_OFFSET_SET_TEMP];
        status->set_temperature_frac  = HF_ZJK_MODBUS_TEMP_FRAC_NONE;
        status->mode                  = (hf_zjk_modbus_mode_t)data[HF_ZJK_MODBUS_DEV_OFFSET_MODE];
        status->fan_speed             = (hf_zjk_modbus_fan_speed_t)data[HF_ZJK_MODBUS_DEV_OFFSET_FAN_SPEED];
        status->room_temperature      = data[HF_ZJK_MODBUS_DEV_OFFSET_ROOM_TEMP];
        status->room_temperature_frac = HF_ZJK_MODBUS_TEMP_FRAC_NONE;
        status->fault_code            = data[HF_ZJK_MODBUS_DEV_OFFSET_FAULT];
        status->wind_direction        = data[HF_ZJK_MODBUS_DEV_OFFSET_WIND];
        status->other_info            = data[HF_ZJK_MODBUS_DEV_OFFSET_OTHER];
    }
}

/**
 * @brief 把一台空调的状态同步到 HOOCH 空调平台层
 * @note  HOOCH 的 ALL 控制项要求开关/模式/风速同时合法，任一项映射失败会整帧丢弃，
 *        因此这里按单项分别下发，保证已识别的参数都能同步。
 *        风向与故障码在 HOOCH 空调帧里没有对应字段，只记录日志。
 */
static void hf_zjk_modbus_dispatch_ac_status(const hf_zjk_modbus_ac_status_t *status)
{
    HOOCH_PROTOCOL_AirConditionerFrame_t ac_frame;
    HOOCH_PROTOCOL_AirConditionerMode_t mode;
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed;

    HOOCH_PROTOCOL_AirConditioner_ClearFrame(&ac_frame);
    ac_frame.source  = HOOCH_PROTOCOL_SOURCE_HF_CENTRAL_CONTROL_485;
    ac_frame.channel = 1U;  /* 集控网关下的内机：通道固定 1，用 16 位地址区分设备 */
    ac_frame.address = (uint16_t)(((uint16_t)status->outer_address <<
                                   HF_ZJK_MODBUS_DEV_ADDR_OUTER_SHIFT) |
                                  (uint16_t)status->inner_address);
    ac_frame.valid   = 1U;

    ac_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_POWER;
    ac_frame.power = (status->power == HF_ZJK_MODBUS_POWER_ON)
        ? HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_ON
        : HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_OFF;
    (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&ac_frame);

    mode = hf_zjk_modbus_map_mode(status->mode);
    if (mode != HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_INVALID)
    {
        ac_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MODE;
        ac_frame.mode = mode;
        (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&ac_frame);
    }
    else
    {
        HF_ZJK_MODBUS_LOG_WARN("[ZJK] unknown mode: 0x%02X\r\n", (unsigned int)status->mode);
    }

    fan_speed = hf_zjk_modbus_map_fan_speed(status->fan_speed);
    if (fan_speed != HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_INVALID)
    {
        ac_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_FAN_SPEED;
        ac_frame.fan_speed = fan_speed;
        (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&ac_frame);
    }
    else
    {
        HF_ZJK_MODBUS_LOG_WARN("[ZJK] unknown fan speed: 0x%02X\r\n",
                               (unsigned int)status->fan_speed);
    }

    ac_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMPERATURE;
    ac_frame.temperature = status->set_temperature;
    (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&ac_frame);

    ac_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_CURRENT_TEMPERATURE;
    ac_frame.current_temperature = status->room_temperature;
    (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&ac_frame);
}

/**
 * @brief 处理"设备状态"回复（控制值 0x01 / 0x0F / 0xFF）
 */
static void hf_zjk_modbus_handle_device_status(const hf_zjk_modbus_frame_t *frame)
{
    uint16_t block_len;
    uint16_t block_count;
    uint16_t index;
    uint16_t base;
    hf_zjk_modbus_ac_status_t status;

    block_len = (frame->function == (uint8_t)HF_ZJK_MODBUS_FUNC_AC_QUERY_STATUS_DEC)
        ? HF_ZJK_MODBUS_DEV_BLOCK_LEN_DEC
        : HF_ZJK_MODBUS_DEV_BLOCK_LEN;
    block_count = (uint16_t)(frame->data_len / block_len);

    if (block_count == 0U)
    {
        /* 设备数量为 0：网关尚未搜索到室内机 */
        HF_ZJK_MODBUS_LOG_WARN("[ZJK] no indoor unit reported yet\r\n");
        return;
    }

    for (index = 0U; index < block_count; index++)
    {
        base = (uint16_t)(index * block_len);
        hf_zjk_modbus_parse_ac_status(frame, base, &status);

        HF_ZJK_MODBUS_LOG_INFO("[ZJK] ac: dev=%u-%u power=%u mode=0x%02X set=%u.%u room=%u.%u fan=0x%02X wind=0x%02X fault=%u host=%u\r\n",
                               (unsigned int)status.outer_address,
                               (unsigned int)status.inner_address,
                               (unsigned int)status.power,
                               (unsigned int)status.mode,
                               (unsigned int)status.set_temperature,
                               (unsigned int)((status.set_temperature_frac ==
                                               (uint8_t)HF_ZJK_MODBUS_TEMP_FRAC_HALF) ? 5U : 0U),
                               (unsigned int)status.room_temperature,
                               (unsigned int)((status.room_temperature_frac ==
                                               (uint8_t)HF_ZJK_MODBUS_TEMP_FRAC_HALF) ? 5U : 0U),
                               (unsigned int)status.fan_speed,
                               (unsigned int)status.wind_direction,
                               (unsigned int)status.fault_code,
                               (unsigned int)((status.other_info & HF_ZJK_MODBUS_OTHER_HOST_BIT) != 0U));

        if (status.fault_code != 0U)
        {
            HF_ZJK_MODBUS_LOG_WARN("[ZJK] ac fault: dev=%u-%u code=%u\r\n",
                                   (unsigned int)status.outer_address,
                                   (unsigned int)status.inner_address,
                                   (unsigned int)status.fault_code);
        }

        hf_zjk_modbus_dispatch_ac_status(&status);
    }
}

/**
 * @brief 处理"在线状态"回复（控制值 0x02），每台设备 3 字节：外机地址 / 内机地址 / 在线标志
 */
static void hf_zjk_modbus_handle_online_status(const hf_zjk_modbus_frame_t *frame)
{
    uint16_t block_count = (uint16_t)(frame->data_len / HF_ZJK_MODBUS_ONLINE_BLOCK_LEN);
    uint16_t index;
    uint16_t base;

    for (index = 0U; index < block_count; index++)
    {
        base = (uint16_t)(index * HF_ZJK_MODBUS_ONLINE_BLOCK_LEN);
        HF_ZJK_MODBUS_LOG_INFO("[ZJK] online: dev=%u-%u %s\r\n",
                               (unsigned int)frame->data[base],
                               (unsigned int)frame->data[base + 1U],
                               (frame->data[base + 2U] != 0U) ? "online" : "offline");
    }
}

/**
 * @brief 处理"原厂故障码"回复（控制值 0x04），每台设备 10 字节：
 *        外机地址 / 内机地址 / 有效长度 / 7 字节 ASCII
 */
static void hf_zjk_modbus_handle_fault_code(const hf_zjk_modbus_frame_t *frame)
{
    uint16_t block_count = (uint16_t)(frame->data_len / HF_ZJK_MODBUS_DEV_BLOCK_LEN);
    uint16_t index;
    uint16_t base;
    uint8_t  code_len;
    uint8_t  i;

    for (index = 0U; index < block_count; index++)
    {
        base = (uint16_t)(index * HF_ZJK_MODBUS_DEV_BLOCK_LEN);
        code_len = frame->data[base + 2U];
        if (code_len > 7U)
        {
            code_len = 7U;
        }

        HF_ZJK_MODBUS_LOG_INFO("[ZJK] fault: dev=%u-%u len=%u code=",
                               (unsigned int)frame->data[base],
                               (unsigned int)frame->data[base + 1U],
                               (unsigned int)code_len);
        for (i = 0U; i < code_len; i++)
        {
            HF_ZJK_MODBUS_LOG_INFO("%c", (char)frame->data[base + 3U + i]);
        }
        HF_ZJK_MODBUS_LOG_INFO("\r\n");
    }
}

/**
 * @brief 处理 0x50 / 0x53 状态查询回复，按控制值区分查询类型
 */
static void hf_zjk_modbus_handle_status_response(const hf_zjk_modbus_frame_t *frame)
{
    if (frame->overflow != 0U)
    {
        HF_ZJK_MODBUS_LOG_WARN("[ZJK] status frame too long, dropped: count=%u\r\n",
                               (unsigned int)frame->device_count);
        return;
    }

    switch (frame->control)
    {
    case (uint8_t)HF_ZJK_MODBUS_QUERY_SINGLE:
    case (uint8_t)HF_ZJK_MODBUS_QUERY_MULTI:
    case (uint8_t)HF_ZJK_MODBUS_QUERY_ALL:
        hf_zjk_modbus_handle_device_status(frame);
        break;

    case (uint8_t)HF_ZJK_MODBUS_QUERY_ONLINE:
        hf_zjk_modbus_handle_online_status(frame);
        break;

    case (uint8_t)HF_ZJK_MODBUS_QUERY_FAULT:
        hf_zjk_modbus_handle_fault_code(frame);
        break;

    default:
        HF_ZJK_MODBUS_LOG_WARN("[ZJK] unsupported query control: 0x%02X\r\n",
                               (unsigned int)frame->control);
        break;
    }
}

/**
 * @brief 处理 0x90 性能查询回复
 * @note  数据域 7 字节：品牌 / 模式位图(2 字节) / 风速位图 / 设定温度范围(最大,最小) / 特殊性能
 */
static void hf_zjk_modbus_handle_performance_response(const hf_zjk_modbus_frame_t *frame)
{
    const uint8_t *data = frame->data;

    if (frame->data_len < (uint16_t)HF_ZJK_MODBUS_PERFORMANCE_DATA_LEN)
    {
        HF_ZJK_MODBUS_LOG_WARN("[ZJK] performance frame too short: %u\r\n",
                               (unsigned int)frame->data_len);
        return;
    }

    HF_ZJK_MODBUS_LOG_INFO("[ZJK] performance: brand=0x%02X modes=0x%04X fan=0x%02X temp_min=%u temp_max=%u special=0x%02X\r\n",
                           (unsigned int)data[0],
                           (unsigned int)(((uint16_t)data[1] << 8) | (uint16_t)data[2]),
                           (unsigned int)data[3],
                           (unsigned int)data[5],
                           (unsigned int)data[4],
                           (unsigned int)data[6]);
}

/**
 * @brief 处理控制指令回显帧
 * @note  回显只说明网关接收了指令，空调是否真正执行需要靠随后的 0x50 查询确认
 */
static void hf_zjk_modbus_handle_control_echo(const hf_zjk_modbus_frame_t *frame)
{
    HF_ZJK_MODBUS_LOG_INFO("[ZJK] control ack: func=0x%02X control=0x%02X count=%u\r\n",
                           (unsigned int)frame->function,
                           (unsigned int)frame->control,
                           (unsigned int)frame->device_count);
}

/**
 * @brief 处理一帧解析完成的集控回复帧
 */
void hf_zjk_modbus_handle_frame(const hf_zjk_modbus_frame_t *frame)
{
    if (frame == NULL)
    {
        HF_ZJK_MODBUS_LOG_WARN("[ZJK] frame is NULL\r\n");
        return;
    }

    switch (frame->function)
    {
    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_QUERY_STATUS:
    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_QUERY_STATUS_DEC:
        hf_zjk_modbus_handle_status_response(frame);
        break;

    case (uint8_t)HF_ZJK_MODBUS_FUNC_AC_QUERY_PERFORMANCE:
        hf_zjk_modbus_handle_performance_response(frame);
        break;

    default:
        if (hf_zjk_modbus_is_control_function(frame->function) != 0U)
        {
            hf_zjk_modbus_handle_control_echo(frame);
        }
        else
        {
            HF_ZJK_MODBUS_LOG_WARN("[ZJK] unsupported function: 0x%02X\r\n",
                                   (unsigned int)frame->function);
        }
        break;
    }
}

#endif /* HOOCH_PROTOCOL_ENABLE_HF_CENTRAL_CONTROL_485 */
