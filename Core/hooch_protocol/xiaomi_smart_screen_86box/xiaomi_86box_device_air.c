/*
 * Xiaomi 86 Box — 空调设备处理
 */
#include "xiaomi_86box_device_air.h"
#include "hooch_protocol.h"
#include <string.h>

/* 0x41 上报所有状态: 开关 | 模式 | 风速 | 温度 */
static uint8_t xiaomi_86box_air_handle_report_all(
    const xiaomi_86box_device_header_t *header,
    const uint8_t *params,
    uint16_t params_len)
{
    HOOCH_PROTOCOL_AirConditionerFrame_t frame;
    uint16_t temperature;

    if (params_len < 4U)
    {
        return 1U;
    }

    temperature = xiaomi_86box_ac_temperature_from_raw(params[3]);

    HOOCH_PROTOCOL_AirConditioner_ClearFrame(&frame);
    frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
    frame.channel = header->device_id;
    frame.power = (params[0] != 0U) ? HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_ON
                                    : HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_OFF;
    frame.mode = (HOOCH_PROTOCOL_AirConditionerMode_t)params[1];
    frame.fan_speed = xiaomi_86box_fan_speed_from_raw(params[2]);
    frame.temperature = temperature;
    frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL;
    (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&frame);

    xiaomi_86box_cache_set(header->device_type, header->device_id,
                           XIAOMI_86BOX_CACHE_POWER, params[0]);
    xiaomi_86box_cache_set(header->device_type, header->device_id,
                           XIAOMI_86BOX_CACHE_MODE, params[1]);
    xiaomi_86box_cache_set(header->device_type, header->device_id,
                           XIAOMI_86BOX_CACHE_FAN_SPEED, params[2]);
    xiaomi_86box_cache_set(header->device_type, header->device_id,
                           XIAOMI_86BOX_CACHE_TEMPERATURE, temperature);

    return 1U;
}

uint8_t xiaomi_86box_device_air_handle(const xiaomi_86box_device_header_t *header,
                                       uint8_t subcmd,
                                       const uint8_t *params,
                                       uint16_t params_len)
{
    HOOCH_PROTOCOL_AirConditionerFrame_t frame;
    uint8_t value;

    if (subcmd == XIAOMI_86BOX_SUBCMD_REPORT_ALL)
    {
        return xiaomi_86box_air_handle_report_all(header, params, params_len);
    }

    if ((subcmd != XIAOMI_86BOX_AC_POWER) &&
        (subcmd != XIAOMI_86BOX_AC_MODE) &&
        (subcmd != XIAOMI_86BOX_AC_FAN_SPEED) &&
        (subcmd != XIAOMI_86BOX_AC_TEMPERATURE) &&
        (subcmd != XIAOMI_86BOX_AC_PAIRING) &&
        (subcmd != XIAOMI_86BOX_AC_INTERLOCK))
    {
        return 0U;
    }

    /* 空调参数: VRF标识 | 网关地址 | 地址高 | 地址低 | 控制值 */
    if (params_len < 1U)
    {
        return 0U;
    }
    value = params[params_len - 1U];

    switch (subcmd)
    {
    case XIAOMI_86BOX_AC_POWER:
        HOOCH_PROTOCOL_AirConditioner_ClearFrame(&frame);
        frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
        frame.channel = header->device_id;
        frame.power = (value != 0U) ? HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_ON
                                    : HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_OFF;
        frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_POWER;
        (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&frame);
        xiaomi_86box_cache_set(header->device_type, header->device_id,
                               XIAOMI_86BOX_CACHE_POWER, value);
        break;

    case XIAOMI_86BOX_AC_MODE:
        HOOCH_PROTOCOL_AirConditioner_ClearFrame(&frame);
        frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
        frame.channel = header->device_id;
        frame.mode = (HOOCH_PROTOCOL_AirConditionerMode_t)value;
        frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MODE;
        (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&frame);
        xiaomi_86box_cache_set(header->device_type, header->device_id,
                               XIAOMI_86BOX_CACHE_MODE, value);
        break;

    case XIAOMI_86BOX_AC_FAN_SPEED:
        HOOCH_PROTOCOL_AirConditioner_ClearFrame(&frame);
        frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
        frame.channel = header->device_id;
        frame.fan_speed = xiaomi_86box_fan_speed_from_raw(value);
        frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_FAN_SPEED;
        (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&frame);
        xiaomi_86box_cache_set(header->device_type, header->device_id,
                               XIAOMI_86BOX_CACHE_FAN_SPEED, value);
        break;

    case XIAOMI_86BOX_AC_TEMPERATURE:
    {
        uint16_t temperature = xiaomi_86box_ac_temperature_from_raw(value);
        HOOCH_PROTOCOL_AirConditioner_ClearFrame(&frame);
        frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
        frame.channel = header->device_id;
        frame.temperature = temperature;
        frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMPERATURE;
        (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&frame);
        xiaomi_86box_cache_set(header->device_type, header->device_id,
                               XIAOMI_86BOX_CACHE_TEMPERATURE, temperature);
        break;
    }

    case XIAOMI_86BOX_AC_PAIRING:
        xiaomi_86box_dispatch_pairing(HOOCH_PROTOCOL_CODE_MATCH_PAGE_AC,
                                      header->device_id, value);
        break;

    case XIAOMI_86BOX_AC_INTERLOCK:
        xiaomi_86box_dispatch_interlock(value);
        break;

    default:
        break;
    }

    return 1U;
}

uint8_t xiaomi_86box_device_air_report_all(const xiaomi_86box_device_header_t *header,
                                           uint8_t *params,
                                           uint16_t *params_len)
{
    xiaomi_86box_state_t *state = xiaomi_86box_state_get(header->device_type,
                                                         header->device_id);
    if (state == NULL)
    {
        return 0U;
    }

    params[0] = state->power;
    params[1] = state->mode;
    params[2] = state->fan_speed;
    params[3] = (uint8_t)state->temperature;
    *params_len = 4U;

    return 1U;
}
