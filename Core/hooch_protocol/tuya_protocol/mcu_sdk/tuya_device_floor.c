/*
 * Tuya DP Dispatch — Floor Heating (DP 130, 131, 136)
 */
#include "tuya_device_floor.h"
#include "protocol.h"
#include "hooch_floor_heating.h"
#include <stddef.h>

/* 填充帧通用字段并分发 */
static void fill_and_set(const HOOCH_PROTOCOL_FloorHeatingFrame_t *cur,
                         HOOCH_PROTOCOL_FloorHeatingPower_t   power,
                         unsigned char                        target_temp)
{
    HOOCH_PROTOCOL_FloorHeatingFrame_t frame;
    frame.power              = power;
    frame.mode               = (cur != NULL) ? cur->mode : HOOCH_PROTOCOL_FLOOR_HEATING_MODE_INVALID;
    frame.target_temperature  = target_temp;
    frame.current_temperature = (cur != NULL) ? cur->current_temperature : 0U;
    frame.sequence           = 0U;
    frame.valid              = 0U;
    (void)HOOCH_PROTOCOL_FloorHeating_SetFrame(&frame);
}

/* ---- DP 130: 地暖开关（bool） ---- */
static unsigned char handle_floor_sw(unsigned short dp_len, unsigned char *dp_data)
{
    const HOOCH_PROTOCOL_FloorHeatingFrame_t *cur = HOOCH_PROTOCOL_FloorHeating_GetFrame();
    HOOCH_PROTOCOL_FloorHeatingPower_t power = mcu_get_dp_download_bool(dp_data, dp_len)
        ? HOOCH_PROTOCOL_FLOOR_HEATING_POWER_ON
        : HOOCH_PROTOCOL_FLOOR_HEATING_POWER_OFF;
    unsigned char target_temp = (cur != NULL) ? cur->target_temperature : 0U;
    fill_and_set(cur, power, target_temp);
    return 1U;
}

/* ---- DP 131: 地暖温度（value） ---- */
static unsigned char handle_floor_temp(unsigned short dp_len, unsigned char *dp_data)
{
    const HOOCH_PROTOCOL_FloorHeatingFrame_t *cur = HOOCH_PROTOCOL_FloorHeating_GetFrame();
    HOOCH_PROTOCOL_FloorHeatingPower_t power = (cur != NULL) ? cur->power : HOOCH_PROTOCOL_FLOOR_HEATING_POWER_ON;
    unsigned char target_temp = (unsigned char)mcu_get_dp_download_value(dp_data, dp_len);
    fill_and_set(cur, power, target_temp);
    return 1U;
}

/* ---- DP 136: 地暖组合信息 (raw: 2byte) ---- */
static unsigned char handle_floor_info(unsigned short dp_len, unsigned char *dp_data)
{
    if (dp_len < 2U) return 0U;

    HOOCH_PROTOCOL_FloorHeatingFrame_t frame;
    frame.power             = dp_data[0] ? HOOCH_PROTOCOL_FLOOR_HEATING_POWER_ON
                                         : HOOCH_PROTOCOL_FLOOR_HEATING_POWER_OFF;
    frame.mode              = HOOCH_PROTOCOL_FLOOR_HEATING_MODE_INVALID;
    frame.target_temperature = dp_data[1];
    frame.current_temperature = 0U;
    frame.sequence          = 0U;
    frame.valid             = 0U;
    (void)HOOCH_PROTOCOL_FloorHeating_SetFrame(&frame);
    return 1U;
}

unsigned char tuya_dp_dispatch_floor(unsigned char dp_id, unsigned char dp_type,
                                     unsigned short dp_len, unsigned char *dp_data)
{
    (void)dp_type;

    switch (dp_id) {
    case DPID_FLOOR_SW:   return handle_floor_sw(dp_len, dp_data);
    case DPID_FLOOR_TEMP: return handle_floor_temp(dp_len, dp_data);
    case DPID_FLOOR_INFO: return handle_floor_info(dp_len, dp_data);
    default: break;
    }
    return 0U;
}
