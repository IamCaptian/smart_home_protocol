/*
 * Tuya DP Dispatch — Fresh Air
 *   116 新风模式 / 125 新风开关 / 150 新风风速
 *   （另有 123 送风风速、135 组合信息，保留处理，实际产品未用到不会触发）
 *
 * 下发语义：App/网关下发的 DP 指令 → Hooch FreshAir 下发回调(Dispatch)。
 * 注意：不要走 SetFrame/上报路径，否则 main.c 里 RegisterCallback 注册的
 *       新风下发回调永远收不到数据（上报与下发是两套独立的缓存与回调）。
 */
#include "tuya_device_freshair.h"
#include "protocol.h"
#include "hooch_fresh_air.h"
#include <stddef.h>

/* Tuya 风速(off=0,low=1,mid=2,high=3) → Hooch (INVALID=0,LOW=1,MEDIUM=2,HIGH=3) */
static HOOCH_PROTOCOL_FreshAirFanSpeed_t map_supply_speed(unsigned char raw)
{
    switch (raw) {
        case 0: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID;   /* off 由 power 表达 */
        case 1: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_LOW;
        case 2: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_MEDIUM;
        case 3: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_HIGH;
        default: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID;
    }
}

/* Tuya 风速(low=0,mid=1,high=2) → Hooch (LOW=1,MEDIUM=2,HIGH=3, 无 off) */
static HOOCH_PROTOCOL_FreshAirFanSpeed_t map_speed(unsigned char raw)
{
    switch (raw) {
        case 0: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_LOW;
        case 1: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_MEDIUM;
        case 2: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_HIGH;
        default: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID;
    }
}

/* Tuya 模式(auto=0,indoor_loop=1,outdoor_loop=2) → Hooch (AUTO=1, MANUAL=2) */
static HOOCH_PROTOCOL_FreshAirMode_t map_mode(unsigned char raw)
{
    switch (raw) {
        case 0: return HOOCH_PROTOCOL_FRESH_AIR_MODE_AUTO;
        case 1: /* fall through */
        case 2: return HOOCH_PROTOCOL_FRESH_AIR_MODE_MANUAL;
        default: return HOOCH_PROTOCOL_FRESH_AIR_MODE_INVALID;
    }
}

/* 按单个控制项装配下发帧并分发；
 * dispatch 层内部会保留上一帧其他字段，所以单项 DP 只需填本项字段即可。 */
static void dispatch_fresh_air_frame(HOOCH_PROTOCOL_FreshAirControlItem_t item,
                                     HOOCH_PROTOCOL_FreshAirPower_t       power,
                                     HOOCH_PROTOCOL_FreshAirMode_t        mode,
                                     HOOCH_PROTOCOL_FreshAirFanSpeed_t    fan_speed)
{
    HOOCH_PROTOCOL_FreshAirFrame_t frame;

    frame.channel            = 1U;    /* 涂鸦单新风：通用协议通道从 1 开始，固定通道 1 */
    frame.power              = power;
    frame.mode               = mode;
    frame.fan_speed          = fan_speed;
    frame.current_temperature = 0U;
    frame.control_item       = item;
    frame.sequence           = 0U;
    frame.valid              = 0U;
    frame.source             = HOOCH_PROTOCOL_SOURCE_TUYA;
    (void)HOOCH_PROTOCOL_FreshAir_DispatchFrame(&frame);
}

/* ---- DP 116: 循环模式（enum） ---- */
static unsigned char handle_loop_mode(unsigned short dp_len, unsigned char *dp_data)
{
    unsigned char raw = mcu_get_dp_download_enum(dp_data, dp_len);
    HOOCH_PROTOCOL_FreshAirMode_t mode = map_mode(raw);
    if (mode == HOOCH_PROTOCOL_FRESH_AIR_MODE_INVALID) return 0U;
    dispatch_fresh_air_frame(HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_MODE,
                             HOOCH_PROTOCOL_FRESH_AIR_POWER_OFF, mode,
                             HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID);
    return 1U;
}

/* ---- DP 125: 新风开关（bool） ---- */
static unsigned char handle_fresh_air_switch(unsigned short dp_len, unsigned char *dp_data)
{
    HOOCH_PROTOCOL_FreshAirPower_t power = mcu_get_dp_download_bool(dp_data, dp_len)
        ? HOOCH_PROTOCOL_FRESH_AIR_POWER_ON
        : HOOCH_PROTOCOL_FRESH_AIR_POWER_OFF;
    dispatch_fresh_air_frame(HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_POWER,
                             power, HOOCH_PROTOCOL_FRESH_AIR_MODE_INVALID,
                             HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID);
    return 1U;
}

/* ---- DP 123: 送风风速（enum，0=off） ---- */
static unsigned char handle_supply_fan_speed(unsigned short dp_len, unsigned char *dp_data)
{
    unsigned char raw = mcu_get_dp_download_enum(dp_data, dp_len);
    if (raw == 0U) return 1U;   /* off 无对应 Hooch 风速档位，整机开关请走 DP135 */
    HOOCH_PROTOCOL_FreshAirFanSpeed_t speed = map_supply_speed(raw);
    if (speed == HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID) return 0U;
    dispatch_fresh_air_frame(HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED,
                             HOOCH_PROTOCOL_FRESH_AIR_POWER_OFF,
                             HOOCH_PROTOCOL_FRESH_AIR_MODE_INVALID, speed);
    return 1U;
}

/* ---- DP 150: 新风风速（enum，无 off） ---- */
static unsigned char handle_fresh_air_speed(unsigned short dp_len, unsigned char *dp_data)
{
    unsigned char raw = mcu_get_dp_download_enum(dp_data, dp_len);
    HOOCH_PROTOCOL_FreshAirFanSpeed_t speed = map_speed(raw);
    if (speed == HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID) return 0U;
    dispatch_fresh_air_frame(HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED,
                             HOOCH_PROTOCOL_FRESH_AIR_POWER_OFF,
                             HOOCH_PROTOCOL_FRESH_AIR_MODE_INVALID, speed);
    return 1U;
}

/* ---- DP 135: 新风组合信息 (raw: 3byte: 开关/模式/风速) ---- */
static unsigned char handle_fan_info(unsigned short dp_len, unsigned char *dp_data)
{
    if (dp_len < 3U) return 0U;

    HOOCH_PROTOCOL_FreshAirPower_t    power = dp_data[0] ? HOOCH_PROTOCOL_FRESH_AIR_POWER_ON
                                                        : HOOCH_PROTOCOL_FRESH_AIR_POWER_OFF;
    HOOCH_PROTOCOL_FreshAirMode_t     mode  = map_mode(dp_data[1]);
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan   = map_supply_speed(dp_data[2]);

    /* 全字段有效时一次 ALL 下发（单条回调）；否则逐项下发能表达的部分 */
    if ((mode != HOOCH_PROTOCOL_FRESH_AIR_MODE_INVALID) &&
        (fan  != HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID))
    {
        dispatch_fresh_air_frame(HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL,
                                 power, mode, fan);
    }
    else
    {
        dispatch_fresh_air_frame(HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_POWER,
                                 power, HOOCH_PROTOCOL_FRESH_AIR_MODE_INVALID,
                                 HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID);
        if (mode != HOOCH_PROTOCOL_FRESH_AIR_MODE_INVALID)
        {
            dispatch_fresh_air_frame(HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_MODE,
                                     HOOCH_PROTOCOL_FRESH_AIR_POWER_OFF, mode,
                                     HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID);
        }
        if (fan != HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID)
        {
            dispatch_fresh_air_frame(HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED,
                                     HOOCH_PROTOCOL_FRESH_AIR_POWER_OFF,
                                     HOOCH_PROTOCOL_FRESH_AIR_MODE_INVALID, fan);
        }
    }
    return 1U;
}

unsigned char tuya_dp_dispatch_freshair(unsigned char dp_id, unsigned char dp_type,
                                        unsigned short dp_len, unsigned char *dp_data)
{
    (void)dp_type;

    switch (dp_id) {
    case DPID_LOOP_MODE:        return handle_loop_mode(dp_len, dp_data);
    case DPID_FRESH_AIR_SWITCH: return handle_fresh_air_switch(dp_len, dp_data);
    case DPID_SUPPLY_FAN_SPEED: return handle_supply_fan_speed(dp_len, dp_data);
    case DPID_FRESH_AIR_SPEED:  return handle_fresh_air_speed(dp_len, dp_data);
    case DPID_FAN_INFO:         return handle_fan_info(dp_len, dp_data);
    default: break;
    }
    return 0U;
}
