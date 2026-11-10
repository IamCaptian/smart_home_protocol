/*
 * Xiaomi 86 Box — 地暖设备处理
 */
#include "xiaomi_86box_device_floor.h"
#include "hooch_protocol.h"
#include <string.h>

uint8_t xiaomi_86box_device_floor_handle(const xiaomi_86box_device_header_t *header,
                                         uint8_t subcmd,
                                         const uint8_t *params,
                                         uint16_t params_len)
{
    HOOCH_PROTOCOL_FloorHeatingFrame_t frame;
    uint8_t value;

    if (params_len < 1U)
    {
        return 0U;
    }
    value = params[params_len - 1U];

    switch (subcmd)
    {
    case XIAOMI_86BOX_FLOOR_HEATING_POWER:
        memset(&frame, 0, sizeof(frame));
        frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
        frame.channel = header->device_id;
        frame.power = (value != 0U) ? HOOCH_PROTOCOL_FLOOR_HEATING_POWER_ON
                                    : HOOCH_PROTOCOL_FLOOR_HEATING_POWER_OFF;
        frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_POWER;
        (void)HOOCH_PROTOCOL_FloorHeating_DispatchFrame(&frame);
        xiaomi_86box_cache_set(header->device_type, header->device_id,
                               XIAOMI_86BOX_CACHE_POWER, value);
        break;

    case XIAOMI_86BOX_FLOOR_HEATING_TEMP:
    {
        uint16_t temperature = xiaomi_86box_floor_temperature_from_raw(value);

        memset(&frame, 0, sizeof(frame));
        frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
        frame.channel = header->device_id;
        frame.target_temperature = temperature;
        frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TARGET_TEMPERATURE;
        (void)HOOCH_PROTOCOL_FloorHeating_DispatchFrame(&frame);
        xiaomi_86box_cache_set(header->device_type, header->device_id,
                               XIAOMI_86BOX_CACHE_TEMPERATURE, temperature);
        break;
    }

    case XIAOMI_86BOX_FLOOR_HEATING_INTERLOCK:
        xiaomi_86box_dispatch_interlock(value);
        break;

    case XIAOMI_86BOX_FLOOR_HEATING_INTERLOCK_STATUS:
        XIAOMI_86BOX_LOG_INFO("[86BOX] Floor heating interlock status: %u\r\n", (unsigned int)value);
        break;

    default:
        return 0U;
    }

    return 1U;
}

uint8_t xiaomi_86box_device_floor_report_all(const xiaomi_86box_device_header_t *header,
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
    params[1] = (uint8_t)state->temperature;
    *params_len = 2U;

    return 1U;
}
