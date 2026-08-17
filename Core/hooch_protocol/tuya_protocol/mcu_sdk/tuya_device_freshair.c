/*
 * Tuya DP Dispatch — Fresh Air (DP 116, 123, 150, 135)
 */
#include "tuya_device_freshair.h"
#include "protocol.h"
#include "hooch_fresh_air.h"
#include <stddef.h>

/* Tuya 风速(off=0,low=1,mid=2,high=3) → Hooch (INVALID=0,LOW=1,MEDIUM=2,HIGH=3) */
static HOOCH_PROTOCOL_FreshAirFanSpeed_t map_supply_speed(unsigned char raw)
{
    switch (raw) {
        case 0: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID;
        case 1: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_LOW;
        case 2: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_MEDIUM;
        case 3: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_HIGH;
        default: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID;
    }
}

/* Tuya 风速(low=0,mid=1,high=2) → Hooch (LOW=1,MEDIUM=2,HIGH=3, 无off) */
static HOOCH_PROTOCOL_FreshAirFanSpeed_t map_speed(unsigned char raw)
{
    switch (raw) {
        case 0: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_LOW;
        case 1: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_MEDIUM;
        case 2: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_HIGH;
        default: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID;
    }
}

static void fill_and_set(const HOOCH_PROTOCOL_FreshAirFrame_t *cur,
                         HOOCH_PROTOCOL_FreshAirPower_t   power,
                         HOOCH_PROTOCOL_FreshAirMode_t    mode,
                         HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed)
{
    HOOCH_PROTOCOL_FreshAirFrame_t frame;
    frame.power     = power;
    frame.mode      = mode;
    frame.fan_speed = fan_speed;
    frame.sequence  = 0U;
    frame.valid     = 0U;

    if (cur != NULL) {
        if (power    == HOOCH_PROTOCOL_FRESH_AIR_POWER_INVALID) frame.power     = cur->power;
        if (mode     == HOOCH_PROTOCOL_FRESH_AIR_MODE_INVALID)  frame.mode      = cur->mode;
        if (fan_speed == HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID) frame.fan_speed = cur->fan_speed;
    }
    (void)HOOCH_PROTOCOL_FreshAir_SetFrame(&frame);
}

/* ---- DP 116: 循环模式（enum） ---- */
static unsigned char handle_loop_mode(unsigned short dp_len, unsigned char *dp_data)
{
    unsigned char raw = mcu_get_dp_download_enum(dp_data, dp_len);
    HOOCH_PROTOCOL_FreshAirMode_t mode;
    /* Tuya: auto=0, indoor_loop=1, outdoor_loop=2 → Hooch: AUTO=*, MANUAL=* */
    switch (raw) {
        case 0: mode = HOOCH_PROTOCOL_FRESH_AIR_MODE_AUTO;   break;
        case 1: /* fall through */
        case 2: mode = HOOCH_PROTOCOL_FRESH_AIR_MODE_MANUAL; break;
        default: return 0U;
    }
    const HOOCH_PROTOCOL_FreshAirFrame_t *cur = HOOCH_PROTOCOL_FreshAir_GetFrame();
    fill_and_set(cur, HOOCH_PROTOCOL_FRESH_AIR_POWER_INVALID, mode,
                 HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID);
    return 1U;
}

/* ---- DP 123: 送风风速（enum） ---- */
static unsigned char handle_supply_fan_speed(unsigned short dp_len, unsigned char *dp_data)
{
    unsigned char raw = mcu_get_dp_download_enum(dp_data, dp_len);
    HOOCH_PROTOCOL_FreshAirFanSpeed_t speed = map_supply_speed(raw);
    if (speed == HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID && raw != 0U) return 0U;
    const HOOCH_PROTOCOL_FreshAirFrame_t *cur = HOOCH_PROTOCOL_FreshAir_GetFrame();
    fill_and_set(cur, HOOCH_PROTOCOL_FRESH_AIR_POWER_INVALID,
                 HOOCH_PROTOCOL_FRESH_AIR_MODE_INVALID, speed);
    return 1U;
}

/* ---- DP 150: 新风风速（enum，无off） ---- */
static unsigned char handle_fresh_air_speed(unsigned short dp_len, unsigned char *dp_data)
{
    unsigned char raw = mcu_get_dp_download_enum(dp_data, dp_len);
    HOOCH_PROTOCOL_FreshAirFanSpeed_t speed = map_speed(raw);
    if (speed == HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID) return 0U;
    const HOOCH_PROTOCOL_FreshAirFrame_t *cur = HOOCH_PROTOCOL_FreshAir_GetFrame();
    fill_and_set(cur, HOOCH_PROTOCOL_FRESH_AIR_POWER_INVALID,
                 HOOCH_PROTOCOL_FRESH_AIR_MODE_INVALID, speed);
    return 1U;
}

/* ---- DP 135: 新风组合信息 (raw: 3byte) ---- */
static unsigned char handle_fan_info(unsigned short dp_len, unsigned char *dp_data)
{
    if (dp_len < 3U) return 0U;

    HOOCH_PROTOCOL_FreshAirFrame_t frame;
    frame.power = dp_data[0] ? HOOCH_PROTOCOL_FRESH_AIR_POWER_ON
                             : HOOCH_PROTOCOL_FRESH_AIR_POWER_OFF;
    /* dp_data[1]: auto=0,indoor_loop=1,outdoor_loop=2 → Hooch */
    switch (dp_data[1]) {
        case 0: frame.mode = HOOCH_PROTOCOL_FRESH_AIR_MODE_AUTO;   break;
        case 1: /* fall through */
        case 2: frame.mode = HOOCH_PROTOCOL_FRESH_AIR_MODE_MANUAL; break;
        default: frame.mode = HOOCH_PROTOCOL_FRESH_AIR_MODE_INVALID; break;
    }
    /* dp_data[2]: off=0,low=1,mid=2,high=3 */
    frame.fan_speed = map_supply_speed(dp_data[2]);
    frame.sequence = 0U;
    frame.valid    = 0U;
    (void)HOOCH_PROTOCOL_FreshAir_SetFrame(&frame);
    return 1U;
}

unsigned char tuya_dp_dispatch_freshair(unsigned char dp_id, unsigned char dp_type,
                                        unsigned short dp_len, unsigned char *dp_data)
{
    (void)dp_type;

    switch (dp_id) {
    case DPID_LOOP_MODE:        return handle_loop_mode(dp_len, dp_data);
    case DPID_SUPPLY_FAN_SPEED: return handle_supply_fan_speed(dp_len, dp_data);
    case DPID_FRESH_AIR_SPEED:  return handle_fresh_air_speed(dp_len, dp_data);
    case DPID_FAN_INFO:         return handle_fan_info(dp_len, dp_data);
    default: break;
    }
    return 0U;
}
