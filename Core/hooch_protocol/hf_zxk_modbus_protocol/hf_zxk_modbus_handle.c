#include "hf_zxk_modbus_handle.h"
#include "hooch_protocol_common.h"

#if HOOCH_PROTOCOL_ENABLE_HF_WIRED_CONTROLLER_485

/*
 * 0x03 回复帧里只有"字节数 + 寄存器数据"，不带寄存器起始地址，
 * 因此只能按回复的寄存器数量区分查询类型：
 *   3 个寄存器  -> 性能查询(HF_ZXK_MODBUS_REG_PERFORMANCE, 0x07D0)
 *   6/8/9 个寄存器 -> 单台内机状态(HF_ZXK_MODBUS_REG_UNIT_STATUS, 0x0000)
 * 若以后出现"查询 3 个内机寄存器"的用法，需要额外记录最近一次查询的起始地址。
 */
#define HF_ZXK_MODBUS_PERFORMANCE_REG_COUNT 3U

/**
 * @brief 取出 0x03 回复中第 index 个寄存器（大端），越界返回 0
 */
static uint16_t hf_zxk_modbus_get_register(const hf_zxk_modbus_frame_t *frame, uint16_t index)
{
    uint16_t offset = (uint16_t)(index * 2U);

    if ((uint16_t)(offset + 1U) >= (uint16_t)frame->byte_count)
    {
        return 0U;
    }

    return (uint16_t)(((uint16_t)frame->data[offset] << 8) | (uint16_t)frame->data[offset + 1U]);
}

/* 协议模式 -> HOOCH 空调模式 */
static HOOCH_PROTOCOL_AirConditionerMode_t hf_zxk_modbus_map_mode(hf_zxk_modbus_mode_t mode)
{
    switch (mode)
    {
    case HF_ZXK_MODBUS_MODE_COOL:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_COOL;
    case HF_ZXK_MODBUS_MODE_FAN:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_FAN;
    case HF_ZXK_MODBUS_MODE_DRY:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_DRY;
    case HF_ZXK_MODBUS_MODE_HEAT:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_HEAT;
    default:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_INVALID;
    }
}

/* 协议风速 -> HOOCH 空调风速 */
static HOOCH_PROTOCOL_AirConditionerFanSpeed_t hf_zxk_modbus_map_fan_speed(
    hf_zxk_modbus_fan_speed_t fan_speed)
{
    switch (fan_speed)
    {
    case HF_ZXK_MODBUS_FAN_SPEED_AUTO:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_AUTO;
    case HF_ZXK_MODBUS_FAN_SPEED_LOW:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_LOW;
    case HF_ZXK_MODBUS_FAN_SPEED_MEDIUM:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_MEDIUM;
    case HF_ZXK_MODBUS_FAN_SPEED_HIGH:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_HIGH;
    default:
        return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_INVALID;
    }
}

/**
 * @brief 把 0x03 内机状态回复解析为语义化结构
 */
static void hf_zxk_modbus_parse_unit_status(const hf_zxk_modbus_frame_t *frame,
                                            hf_zxk_modbus_unit_status_t *status)
{
    uint8_t reg_count = (uint8_t)(frame->byte_count / 2U);

    status->address = frame->address;
    status->reg_count = reg_count;
    status->power = (hf_zxk_modbus_power_t)hf_zxk_modbus_get_register(
        frame, (uint16_t)HF_ZXK_MODBUS_UNIT_REG_POWER);
    status->mode = (hf_zxk_modbus_mode_t)hf_zxk_modbus_get_register(
        frame, (uint16_t)HF_ZXK_MODBUS_UNIT_REG_MODE);
    status->set_temperature = hf_zxk_modbus_get_register(
        frame, (uint16_t)HF_ZXK_MODBUS_UNIT_REG_SET_TEMP);
    status->fan_speed = (hf_zxk_modbus_fan_speed_t)hf_zxk_modbus_get_register(
        frame, (uint16_t)HF_ZXK_MODBUS_UNIT_REG_FAN_SPEED);
    status->wind_direction = (hf_zxk_modbus_wind_direction_t)hf_zxk_modbus_get_register(
        frame, (uint16_t)HF_ZXK_MODBUS_UNIT_REG_WIND);
    status->fault_code = hf_zxk_modbus_get_register(
        frame, (uint16_t)HF_ZXK_MODBUS_UNIT_REG_FAULT);
    status->room_temperature = 0U;
    status->permission = 0U;
    status->mode_allowed = 0U;

    if (reg_count > (uint8_t)HF_ZXK_MODBUS_UNIT_REG_ROOM_TEMP)
    {
        status->room_temperature = hf_zxk_modbus_get_register(
            frame, (uint16_t)HF_ZXK_MODBUS_UNIT_REG_ROOM_TEMP);
    }

    if (reg_count > (uint8_t)HF_ZXK_MODBUS_UNIT_REG_PERMISSION)
    {
        status->permission = hf_zxk_modbus_get_register(
            frame, (uint16_t)HF_ZXK_MODBUS_UNIT_REG_PERMISSION);
    }

    if (reg_count > (uint8_t)HF_ZXK_MODBUS_UNIT_REG_MODE_ALLOWED)
    {
        status->mode_allowed = hf_zxk_modbus_get_register(
            frame, (uint16_t)HF_ZXK_MODBUS_UNIT_REG_MODE_ALLOWED);
    }
}

/* 上次已投递给平台的语义化内机状态（变化检测用） */
typedef struct
{
    uint8_t  address;              /* 485 从机地址，地址变化时整帧重投 */
    uint8_t  power;                /* 已映射的开关值 */
    uint8_t  mode;                 /* 已映射的模式值 */
    uint8_t  fan_speed;            /* 已映射的风速值 */
    uint16_t set_temperature;      /* 设定温度 */
    uint16_t room_temperature;     /* 室温 */
    uint8_t  has_room_temperature; /* 上次回复是否包含室温 */
} hf_zxk_modbus_unit_state_cache_t;

static hf_zxk_modbus_unit_state_cache_t s_hf_zxk_modbus_unit_state_cache;
static uint8_t s_hf_zxk_modbus_unit_state_cache_valid = 0U;

/**
 * @brief 把内机状态同步到 HOOCH 空调平台层（仅投递发生变化的控制项）
 * @note  0x03 回复是内机全量状态且每秒轮询一次，这里用缓存做变化检测：
 *        只有参数真正变化（或首帧 / 从机地址变化）才触发平台侧回调，避免重复投递；
 *        单项映射失败会跳过该项，不影响其它参数。
 */
static void hf_zxk_modbus_dispatch_unit_status(const hf_zxk_modbus_unit_status_t *status)
{
    HOOCH_PROTOCOL_AirConditionerFrame_t ac_frame;
    HOOCH_PROTOCOL_AirConditionerPower_t power;
    HOOCH_PROTOCOL_AirConditionerMode_t mode;
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed;
    uint8_t address_changed;
    uint8_t has_room_temperature;
    uint8_t power_changed;
    uint8_t mode_changed;
    uint8_t fan_speed_changed;
    uint8_t temperature_changed;
    uint8_t room_temperature_changed;

    power = (status->power == HF_ZXK_MODBUS_POWER_ON)
        ? HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_ON
        : HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_OFF;
    mode = hf_zxk_modbus_map_mode(status->mode);
    fan_speed = hf_zxk_modbus_map_fan_speed(status->fan_speed);
    has_room_temperature =
        (status->reg_count > (uint8_t)HF_ZXK_MODBUS_UNIT_REG_ROOM_TEMP) ? 1U : 0U;

    /* 首帧或从机地址变化：整帧重投，保证平台侧与网关一致 */
    address_changed =
        ((s_hf_zxk_modbus_unit_state_cache_valid == 0U) ||
         (s_hf_zxk_modbus_unit_state_cache.address != status->address)) ? 1U : 0U;

    power_changed =
        ((address_changed != 0U) ||
         (s_hf_zxk_modbus_unit_state_cache.power != (uint8_t)power)) ? 1U : 0U;
    mode_changed =
        ((mode != HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_INVALID) &&
         ((address_changed != 0U) ||
          (s_hf_zxk_modbus_unit_state_cache.mode != (uint8_t)mode))) ? 1U : 0U;
    fan_speed_changed =
        ((fan_speed != HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_INVALID) &&
         ((address_changed != 0U) ||
          (s_hf_zxk_modbus_unit_state_cache.fan_speed != (uint8_t)fan_speed))) ? 1U : 0U;
    temperature_changed =
        ((address_changed != 0U) ||
         (s_hf_zxk_modbus_unit_state_cache.set_temperature != status->set_temperature)) ? 1U : 0U;
    room_temperature_changed =
        ((has_room_temperature != 0U) &&
         ((address_changed != 0U) ||
          (s_hf_zxk_modbus_unit_state_cache.has_room_temperature == 0U) ||
          (s_hf_zxk_modbus_unit_state_cache.room_temperature != status->room_temperature))) ? 1U : 0U;

    HOOCH_PROTOCOL_AirConditioner_ClearFrame(&ac_frame);
    ac_frame.source  = HOOCH_PROTOCOL_SOURCE_HF_WIRED_CONTROLLER_485;
    ac_frame.channel = 1U;  /* 单台内机：通道暂固定为 1，多内机时按 485 地址映射 */
    ac_frame.address = status->address;
    ac_frame.valid   = 1U;

    if (power_changed != 0U)
    {
        ac_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_POWER;
        ac_frame.power = power;
        (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&ac_frame);
    }

    if (mode_changed != 0U)
    {
        ac_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MODE;
        ac_frame.mode = mode;
        (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&ac_frame);
    }
    else if (mode == HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_INVALID)
    {
        HF_ZXK_MODBUS_LOG_WARN("[MODBUS] unknown mode: 0x%04X\r\n", (unsigned int)status->mode);
    }

    if (fan_speed_changed != 0U)
    {
        ac_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_FAN_SPEED;
        ac_frame.fan_speed = fan_speed;
        (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&ac_frame);
    }
    else if (fan_speed == HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_INVALID)
    {
        HF_ZXK_MODBUS_LOG_WARN("[MODBUS] unknown fan speed: 0x%04X\r\n",
                               (unsigned int)status->fan_speed);
    }

    if (temperature_changed != 0U)
    {
        ac_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMPERATURE;
        ac_frame.temperature = status->set_temperature;
        (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&ac_frame);
    }

    if (room_temperature_changed != 0U)
    {
        ac_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_CURRENT_TEMPERATURE;
        ac_frame.current_temperature = status->room_temperature;
        (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&ac_frame);
    }

    /* 缓存本次网关侧状态，供下次变化检测 */
    s_hf_zxk_modbus_unit_state_cache.address = status->address;
    s_hf_zxk_modbus_unit_state_cache.power = (uint8_t)power;

    if (mode != HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_INVALID)
    {
        s_hf_zxk_modbus_unit_state_cache.mode = (uint8_t)mode;
    }

    if (fan_speed != HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_INVALID)
    {
        s_hf_zxk_modbus_unit_state_cache.fan_speed = (uint8_t)fan_speed;
    }

    s_hf_zxk_modbus_unit_state_cache.set_temperature = status->set_temperature;

    if (has_room_temperature != 0U)
    {
        s_hf_zxk_modbus_unit_state_cache.room_temperature = status->room_temperature;
    }

    s_hf_zxk_modbus_unit_state_cache.has_room_temperature = has_room_temperature;
    s_hf_zxk_modbus_unit_state_cache_valid = 1U;
}

/**
 * @brief 处理 0x03 读保持寄存器回复
 */
static void hf_zxk_modbus_handle_read_response(const hf_zxk_modbus_frame_t *frame)
{
    uint8_t reg_count = (uint8_t)(frame->byte_count / 2U);

    if (reg_count == HF_ZXK_MODBUS_PERFORMANCE_REG_COUNT)
    {
        /* 性能查询回复：文档只说明第 3 个寄存器为风向(0=无风向)，前两个未定义 */
        HF_ZXK_MODBUS_LOG_INFO("[MODBUS] performance: addr=%u param0=%u param1=%u wind=%u\r\n",
                               (unsigned int)frame->address,
                               (unsigned int)hf_zxk_modbus_get_register(
                                   frame, (uint16_t)HF_ZXK_MODBUS_PERFORMANCE_REG_UNKNOWN_0),
                               (unsigned int)hf_zxk_modbus_get_register(
                                   frame, (uint16_t)HF_ZXK_MODBUS_PERFORMANCE_REG_UNKNOWN_1),
                               (unsigned int)hf_zxk_modbus_get_register(
                                   frame, (uint16_t)HF_ZXK_MODBUS_PERFORMANCE_REG_WIND));

        /* 网关已完成搜索：周期查询可以从"查性能"切到"查内机状态" */
        hf_zxk_modbus_query_notify_performance_received();
        return;
    }

    if (reg_count <= (uint8_t)HF_ZXK_MODBUS_UNIT_REG_FAULT)
    {
        /* 文档中内机状态至少包含到故障码(6 个寄存器)，更短的不是状态回复 */
        HF_ZXK_MODBUS_LOG_WARN("[MODBUS] unsupported register count: %u\r\n",
                               (unsigned int)reg_count);
        return;
    }

    {
        hf_zxk_modbus_unit_status_t status;

        hf_zxk_modbus_parse_unit_status(frame, &status);

        HF_ZXK_MODBUS_LOG_INFO("[MODBUS] unit: addr=%u power=%u mode=0x%02X set_temp=%u fan=%u wind=%u fault=%u\r\n",
                               (unsigned int)status.address,
                               (unsigned int)status.power,
                               (unsigned int)status.mode,
                               (unsigned int)status.set_temperature,
                               (unsigned int)status.fan_speed,
                               (unsigned int)status.wind_direction,
                               (unsigned int)status.fault_code);

        if (status.reg_count > (uint8_t)HF_ZXK_MODBUS_UNIT_REG_ROOM_TEMP)
        {
            HF_ZXK_MODBUS_LOG_INFO("[MODBUS] unit: room_temp=%u permission=0x%04X switch_right=%u\r\n",
                                   (unsigned int)status.room_temperature,
                                   (unsigned int)status.permission,
                                   (unsigned int)((status.permission & HF_ZXK_MODBUS_PERMISSION_SWITCH_BIT) != 0U));
        }

        if (status.reg_count > (uint8_t)HF_ZXK_MODBUS_UNIT_REG_MODE_ALLOWED)
        {
            HF_ZXK_MODBUS_LOG_INFO("[MODBUS] unit: mode_allowed=0x%04X\r\n",
                                   (unsigned int)status.mode_allowed);
        }

        hf_zxk_modbus_dispatch_unit_status(&status);
    }
}

/**
 * @brief 处理 0x06 / 0x10 写寄存器回显
 * @note  回显只说明网关接收了指令，空调是否真正执行需要靠随后的 0x03 查询确认
 */
static void hf_zxk_modbus_handle_write_echo(const hf_zxk_modbus_frame_t *frame)
{
    if (frame->function == (uint8_t)HF_ZXK_MODBUS_FUNC_WRITE_SINGLE)
    {
        HF_ZXK_MODBUS_LOG_INFO("[MODBUS] 0x06 ack: addr=%u reg=0x%04X value=%u\r\n",
                               (unsigned int)frame->address,
                               (unsigned int)frame->reg_addr,
                               (unsigned int)frame->reg_value);
                           
    }
    else
    {
        HF_ZXK_MODBUS_LOG_INFO("[MODBUS] 0x10 ack: addr=%u reg=0x%04X quantity=%u\r\n",
                               (unsigned int)frame->address,
                               (unsigned int)frame->reg_addr,
                               (unsigned int)frame->reg_quantity);
    }
}

/**
 * @brief 处理一帧解析完成的 MODBUS 回复帧
 */
void hf_zxk_modbus_handle_frame(const hf_zxk_modbus_frame_t *frame)
{
    if (frame == NULL)
    {
        HF_ZXK_MODBUS_LOG_WARN("[MODBUS] frame is NULL\r\n");
        return;
    }

    if (frame->is_exception != 0U)
    {
        HF_ZXK_MODBUS_LOG_WARN("[MODBUS] exception: addr=%u func=0x%02X code=0x%02X\r\n",
                               (unsigned int)frame->address,
                               (unsigned int)frame->function,
                               (unsigned int)frame->exception_code);
        return;
    }

    switch (frame->function)
    {
    case (uint8_t)HF_ZXK_MODBUS_FUNC_READ_HOLDING:
        hf_zxk_modbus_handle_read_response(frame);
        break;

    case (uint8_t)HF_ZXK_MODBUS_FUNC_WRITE_SINGLE:
    case (uint8_t)HF_ZXK_MODBUS_FUNC_WRITE_MULTIPLE:
        hf_zxk_modbus_handle_write_echo(frame);
        break;

    default:
        HF_ZXK_MODBUS_LOG_WARN("[MODBUS] unsupported function: 0x%02X\r\n",
                               (unsigned int)frame->function);
        break;
    }
}

#endif /* HOOCH_PROTOCOL_ENABLE_HF_WIRED_CONTROLLER_485 */
