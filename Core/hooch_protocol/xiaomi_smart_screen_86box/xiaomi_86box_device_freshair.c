/*
 * Xiaomi 86 Box — 新风设备处理
 */
#include "xiaomi_86box_device_freshair.h"
#include "hooch_protocol.h"
#include <string.h>

uint8_t xiaomi_86box_device_freshair_handle(const xiaomi_86box_device_header_t *header,
                                            uint8_t subcmd,
                                            const uint8_t *params,
                                            uint16_t params_len)
{
    HOOCH_PROTOCOL_FreshAirFrame_t frame;
    uint8_t value;

    if (params_len < 1U)
    {
        return 0U;
    }
    value = params[params_len - 1U];

    switch (subcmd)
    {
    case XIAOMI_86BOX_FRESH_AIR_POWER:
        memset(&frame, 0, sizeof(frame));
        frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
        frame.channel = header->device_id;
        frame.power = (value != 0U) ? HOOCH_PROTOCOL_FRESH_AIR_POWER_ON
                                    : HOOCH_PROTOCOL_FRESH_AIR_POWER_OFF;
        frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_POWER;
        (void)HOOCH_PROTOCOL_FreshAir_DispatchFrame(&frame);
        xiaomi_86box_cache_set(header->device_type, header->device_id,
                               XIAOMI_86BOX_CACHE_POWER, value);
        break;

    case XIAOMI_86BOX_FRESH_AIR_FAN_SPEED:
    {
        HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed;

        switch (value)
        {
        case 1U: fan_speed = HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_AUTO;   break;
        case 2U: fan_speed = HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_LOW;    break;
        case 3U: fan_speed = HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_MEDIUM; break;
        case 4U: fan_speed = HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_HIGH;   break;
        default: fan_speed = HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID;break;
        }

        memset(&frame, 0, sizeof(frame));
        frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
        frame.channel = header->device_id;
        frame.fan_speed = fan_speed;
        frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED;
        (void)HOOCH_PROTOCOL_FreshAir_DispatchFrame(&frame);
        xiaomi_86box_cache_set(header->device_type, header->device_id,
                               XIAOMI_86BOX_CACHE_FAN_SPEED, value);
        break;
    }

    case XIAOMI_86BOX_FRESH_AIR_MODE:
    {
        /* 0-送风, 1-排风; HOOCH 无对应语义, 映射为手动/强劲 */
        HOOCH_PROTOCOL_FreshAirMode_t mode = (value == 0U)
            ? HOOCH_PROTOCOL_FRESH_AIR_MODE_MANUAL
            : HOOCH_PROTOCOL_FRESH_AIR_MODE_BOOST;

        memset(&frame, 0, sizeof(frame));
        frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
        frame.channel = header->device_id;
        frame.mode = mode;
        frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_MODE;
        (void)HOOCH_PROTOCOL_FreshAir_DispatchFrame(&frame);
        xiaomi_86box_cache_set(header->device_type, header->device_id,
                               XIAOMI_86BOX_CACHE_MODE, value);
        break;
    }

    case XIAOMI_86BOX_FRESH_AIR_INTERLOCK:
        xiaomi_86box_dispatch_interlock(value);
        break;

    case XIAOMI_86BOX_FRESH_AIR_INTERLOCK_STATUS:
        XIAOMI_86BOX_LOG_INFO("[86BOX] Fresh air interlock status: %u\r\n", (unsigned int)value);
        break;

    default:
        return 0U;
    }

    return 1U;
}

uint8_t xiaomi_86box_device_freshair_report_all(const xiaomi_86box_device_header_t *header,
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
    params[1] = state->fan_speed;
    params[2] = state->mode;
    *params_len = 3U;

    return 1U;
}
