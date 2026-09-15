/*
 * Tuya DP Dispatch — Air Conditioner (DP 102, 105, 106, 152, 134)
 */
#include "tuya_device_air.h"
#include "protocol.h"
#include "hooch_air_conditioner.h"
#include "hooch_setting.h"

/* Tuya 风速枚举 (low=0,mid=1,high=2,auto=3) → Hooch (LOW=1,MEDIUM=2,HIGH=3,AUTO=4) */
static HOOCH_PROTOCOL_AirConditionerFanSpeed_t map_fan_speed(unsigned char raw)
{
    switch (raw) {
        case 0: return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_LOW;
        case 1: return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_MEDIUM;
        case 2: return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_HIGH;
        case 3: return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_AUTO;
        default: return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_INVALID;
    }
}

/* Tuya 模式枚举 (cold=0,hot=1,dry=2,fan=3,auto=4) → Hooch (COOL=1,HEAT=4,DRY=3,FAN=2,AUTO=5) */
static HOOCH_PROTOCOL_AirConditionerMode_t map_mode(unsigned char raw)
{
    switch (raw) {
        case 0: return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_COOL;
        case 1: return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_HEAT;
        case 2: return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_DRY;
        case 3: return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_FAN;
        case 4: return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_AUTO;
        default: return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_INVALID;
    }
}

/* 填充帧公共字段并分发 */
static void dispatch_air_frame(HOOCH_PROTOCOL_AirConditionerControlItem_t item,
                               HOOCH_PROTOCOL_AirConditionerPower_t     power,
                               HOOCH_PROTOCOL_AirConditionerMode_t      mode,
                               HOOCH_PROTOCOL_AirConditionerFanSpeed_t  fan_speed,
                               unsigned char                            temperature)
{
    HOOCH_PROTOCOL_AirConditionerFrame_t frame;
    frame.channel      = 0U;
    frame.power        = power;
    frame.mode         = mode;
    frame.fan_speed    = fan_speed;
    frame.temperature  = temperature;
    frame.control_item = item;
    frame.sequence     = 0U;
    frame.valid        = 0U;
    HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&frame);
}

/* ---- DP 102: 风速（枚举） ---- */
static unsigned char handle_fan_speed_enum(unsigned short dp_len, unsigned char *dp_data)
{
    unsigned char raw = mcu_get_dp_download_enum(dp_data, dp_len);
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t speed = map_fan_speed(raw);
    if (speed == HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_INVALID) return 0U;
    dispatch_air_frame(HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_FAN_SPEED,
                       HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_ON,
                       HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_INVALID, speed, 0U);
    return 1U;
}

/* ---- DP 105: 模式（枚举） ---- */
static unsigned char handle_mode(unsigned short dp_len, unsigned char *dp_data)
{
    unsigned char raw = mcu_get_dp_download_enum(dp_data, dp_len);
    HOOCH_PROTOCOL_AirConditionerMode_t mode = map_mode(raw);
    if (mode == HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_INVALID) return 0U;
    dispatch_air_frame(HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MODE,
                       HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_ON, mode,
                       HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_INVALID, 0U);
    return 1U;
}

/* ---- DP 106: 温度设定（value） ---- */
static unsigned char handle_temp_set(unsigned short dp_len, unsigned char *dp_data)
{
    unsigned char temp = (unsigned char)mcu_get_dp_download_value(dp_data, dp_len);
    dispatch_air_frame(HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMPERATURE,
                       HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_ON,
                       HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_INVALID,
                       HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_INVALID, temp);
    return 1U;
}

/* ---- DP 152: 空调开关（bool） ---- */
static unsigned char handle_switch(unsigned short dp_len, unsigned char *dp_data)
{
    unsigned char power = mcu_get_dp_download_bool(dp_data, dp_len);
    dispatch_air_frame(HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_POWER,
                       power ? HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_ON
                             : HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_OFF,
                       HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_INVALID,
                       HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_INVALID, 0U);
    return 1U;
}

/* ---- DP 134: 空调组合信息 (raw: 5byte) ---- */
static unsigned char handle_ac_info(unsigned short dp_len, unsigned char *dp_data)
{
    if (dp_len < 5U) return 0U;

    HOOCH_PROTOCOL_AirConditionerFrame_t frame;
    frame.channel      = 0U;
    frame.power        = dp_data[0] ? HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_ON
                                    : HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_OFF;
    /* dp_data[1]: cold=0,hot=1,dry=2,fan=3,auto=4 → Hooch */
    frame.mode = map_mode(dp_data[1]);
    /* dp_data[2]: low=0,middle=1,high=2,auto=3 → Hooch */
    frame.fan_speed = map_fan_speed(dp_data[2]);
    /* dp_data[3]: 风向，暂忽略 */
    frame.temperature  = dp_data[4];
    frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL;
    frame.sequence     = 0U;
    frame.valid        = 0U;
    HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&frame);
    return 1U;
}

/* ---- DP 133: 空调温度上下限 (raw: byte0=0上限/1下限, byte1=数值5~35℃) ---- */
static unsigned char handle_ac_temp_limit(unsigned short dp_len, unsigned char *dp_data)
{
    if (dp_len < 2U) return 0U;

    const unsigned char is_max = (dp_data[0] == 0U);

    /* 同步到屏幕设置缓存（与 KNX 空调限位配置一致） */
    HOOCH_PROTOCOL_SettingFrame_t setting_frame;
    setting_frame.item = is_max ? HOOCH_PROTOCOL_SETTING_ITEM_AIR_CONDITIONER_TEMPERATURE_MAX
                                : HOOCH_PROTOCOL_SETTING_ITEM_AIR_CONDITIONER_TEMPERATURE_MIN;
    setting_frame.value = dp_data[1];
    setting_frame.screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_NEVER;
    setting_frame.page = HOOCH_PROTOCOL_SETTING_PAGE_SWITCH;
    setting_frame.param1 = 0U;
    setting_frame.param2 = 0U;
    setting_frame.param3 = 0U;
    setting_frame.param4 = 0U;
    setting_frame.sequence = 0U;
    setting_frame.valid = 0U;
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);

    /* 空调下发帧：控制项 TEMP_MIN/TEMP_MAX，数值放在 value 字段 */
    HOOCH_PROTOCOL_AirConditionerFrame_t frame;
    frame.channel           = 0U;
    frame.power             = HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_OFF;
    frame.mode              = HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_INVALID;
    frame.fan_speed         = HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_INVALID;
    frame.temperature       = 0U;
    frame.current_temperature = 0U;
    frame.control_item      = is_max ? HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMP_MAX
                                     : HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMP_MIN;
    frame.value             = dp_data[1];
    frame.sequence          = 0U;
    frame.valid             = 0U;
    (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&frame);
    return 1U;
}

unsigned char tuya_dp_dispatch_air(unsigned char dp_id, unsigned char dp_type,
                                   unsigned short dp_len, unsigned char *dp_data)
{
    (void)dp_type;

    switch (dp_id) {
    case DPID_FAN_SPEED_ENUM: return handle_fan_speed_enum(dp_len, dp_data);
    case DPID_MODE:           return handle_mode(dp_len, dp_data);
    case DPID_TEMP_SET:       return handle_temp_set(dp_len, dp_data);
    case DPID_AIR_SWITCH:      return handle_switch(dp_len, dp_data);
    case DPID_AC_INFO:        return handle_ac_info(dp_len, dp_data);
    case DPID_AC_LIMIT:       return handle_ac_temp_limit(dp_len, dp_data);
    default: break;
    }
    return 0U;
}
