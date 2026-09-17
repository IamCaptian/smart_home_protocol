/*
 * Tuya DP Dispatch — Floor Heating (DP 130, 131, 132, 136)
 *
 * 下发语义：App/网关下发的 DP 指令 → Hooch FloorHeating 下发回调(Dispatch)。
 * 注意：不要走 SetFrame/上报路径，否则 main.c 里 RegisterCallback 注册的
 *       地暖下发回调永远收不到数据（上报与下发是两套独立的缓存与回调）。
 */
#include "tuya_device_floor.h"
#include "protocol.h"
#include "hooch_floor_heating.h"
#include "hooch_setting.h"
#include <stddef.h>

/* 按单个控制项装配下发帧并分发；
 * dispatch 层内部会保留上一帧其他字段，所以单项 DP 只需填本项字段即可。 */
static void dispatch_floor_frame(HOOCH_PROTOCOL_FloorHeatingControlItem_t item,
                                 HOOCH_PROTOCOL_FloorHeatingPower_t      power,
                                 unsigned char                           target_temp)
{
    HOOCH_PROTOCOL_FloorHeatingFrame_t frame;

    frame.channel             = 1U;    /* 涂鸦单地暖：通用协议通道从 1 开始，固定通道 1 */
    frame.power               = power;
    frame.mode                = HOOCH_PROTOCOL_FLOOR_HEATING_MODE_INVALID;
    frame.target_temperature  = target_temp;
    frame.current_temperature = 0U;
    frame.control_item        = item;
    frame.sequence            = 0U;
    frame.valid               = 0U;
    frame.source              = HOOCH_PROTOCOL_SOURCE_TUYA;
    (void)HOOCH_PROTOCOL_FloorHeating_DispatchFrame(&frame);
}

/* ---- DP 130: 地暖开关（bool） ---- */
static unsigned char handle_floor_sw(unsigned short dp_len, unsigned char *dp_data)
{
    HOOCH_PROTOCOL_FloorHeatingPower_t power = mcu_get_dp_download_bool(dp_data, dp_len)
        ? HOOCH_PROTOCOL_FLOOR_HEATING_POWER_ON
        : HOOCH_PROTOCOL_FLOOR_HEATING_POWER_OFF;
    dispatch_floor_frame(HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_POWER, power, 0U);
    return 1U;
}

/* ---- DP 131: 地暖温度（value） ---- */
static unsigned char handle_floor_temp(unsigned short dp_len, unsigned char *dp_data)
{
    unsigned char target_temp = (unsigned char)mcu_get_dp_download_value(dp_data, dp_len);
    dispatch_floor_frame(HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TARGET_TEMPERATURE,
                         HOOCH_PROTOCOL_FLOOR_HEATING_POWER_OFF, target_temp);
    return 1U;
}

/* ---- DP 136: 地暖组合信息 (raw: 2byte) ---- */
static unsigned char handle_floor_info(unsigned short dp_len, unsigned char *dp_data)
{
    if (dp_len < 2U) return 0U;

    /* byte0=开关, byte1=目标温度。地暖帧没有“开关+温度”合并控制项，
     * 拆成两项先后下发（dispatch 缓存会合并，最后一次回调携带完整状态）。 */
    dispatch_floor_frame(HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_POWER,
                         dp_data[0] ? HOOCH_PROTOCOL_FLOOR_HEATING_POWER_ON
                                    : HOOCH_PROTOCOL_FLOOR_HEATING_POWER_OFF,
                         0U);
    dispatch_floor_frame(HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TARGET_TEMPERATURE,
                         HOOCH_PROTOCOL_FLOOR_HEATING_POWER_OFF, dp_data[1]);
    return 1U;
}

/* ---- DP 132: 地暖温度上下限 (raw: byte0=0上限/1下限, byte1=数值5~35℃) ---- */
static unsigned char handle_heat_limit(unsigned short dp_len, unsigned char *dp_data)
{
    if (dp_len < 2U) return 0U;

    const unsigned char is_max = (dp_data[0] == 0U);

    /* 同步到屏幕设置缓存（与 KNX 地暖限位配置一致） */
    HOOCH_PROTOCOL_SettingFrame_t setting_frame;
    setting_frame.item = is_max ? HOOCH_PROTOCOL_SETTING_ITEM_FLOOR_HEATING_TEMPERATURE_MAX
                                : HOOCH_PROTOCOL_SETTING_ITEM_FLOOR_HEATING_TEMPERATURE_MIN;
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

    /* 地暖下发帧：控制项 TEMP_MIN/TEMP_MAX，数值放在 value 字段 */
    HOOCH_PROTOCOL_FloorHeatingFrame_t frame;
    frame.channel              = 1U;    /* 涂鸦单地暖：固定通道 1 */
    frame.power                = HOOCH_PROTOCOL_FLOOR_HEATING_POWER_OFF;
    frame.mode                 = HOOCH_PROTOCOL_FLOOR_HEATING_MODE_INVALID;
    frame.target_temperature   = 0U;
    frame.current_temperature  = 0U;
    frame.control_item         = is_max ? HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TEMP_MAX
                                        : HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TEMP_MIN;
    frame.value                = dp_data[1];
    frame.sequence             = 0U;
    frame.valid                = 0U;
    frame.source               = HOOCH_PROTOCOL_SOURCE_TUYA;
    (void)HOOCH_PROTOCOL_FloorHeating_DispatchFrame(&frame);
    return 1U;
}

unsigned char tuya_dp_dispatch_floor(unsigned char dp_id, unsigned char dp_type,
                                     unsigned short dp_len, unsigned char *dp_data)
{
    (void)dp_type;

    switch (dp_id) {
    case DPID_FLOOR_SW:   return handle_floor_sw(dp_len, dp_data);
    case DPID_FLOOR_TEMP: return handle_floor_temp(dp_len, dp_data);
    case DPID_HEAT_LIMIT: return handle_heat_limit(dp_len, dp_data);
    case DPID_FLOOR_INFO: return handle_floor_info(dp_len, dp_data);
    default: break;
    }
    return 0U;
}
