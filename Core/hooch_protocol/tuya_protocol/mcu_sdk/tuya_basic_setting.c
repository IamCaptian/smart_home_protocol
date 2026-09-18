/*
 * Tuya DP Dispatch — Basic Settings (DP 140, 141, 142, 143, 144, 146, 148, 149)
 *
 * 注：DP 132/133(地暖/空调温度上下限) 已由 tuya_device_floor.c / tuya_device_air.c
 *     接管（同时分发到设备帧与设置项），此处不再重复处理。
 */
#include "tuya_basic_setting.h"
#include "protocol.h"
#include "hooch_setting.h"
#include "hooch_key_mode.h"
#include "hooch_air_conditioner.h"
#include "hooch_fresh_air.h"
#include "hooch_floor_heating.h"

/* 填充 SettingFrame 公共字段 */
static void init_setting_frame(HOOCH_PROTOCOL_SettingFrame_t *sf,
                               HOOCH_PROTOCOL_SettingItem_t item,
                               unsigned char value)
{
    sf->item                 = item;
    sf->value               = value;
    sf->screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_NEVER;
    sf->page                = HOOCH_PROTOCOL_SETTING_PAGE_SWITCH;
    sf->param1              = 0U;
    sf->param2              = 0U;
    sf->param3              = 0U;
    sf->param4              = 0U;
    sf->sequence            = 0U;
    sf->valid               = 0U;
}

/* ---- DP 144: 主题（enum） ---- */
static unsigned char handle_theme(unsigned short dp_len, unsigned char *dp_data)
{
    unsigned char theme_val = mcu_get_dp_download_enum(dp_data, dp_len);
    HOOCH_PROTOCOL_SettingFrame_t sf;
    init_setting_frame(&sf, HOOCH_PROTOCOL_SETTING_ITEM_THEME, theme_val);
    sf.source = HOOCH_PROTOCOL_SOURCE_TUYA;
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);
    return 1U;
}

/* ---- DP 146: 童锁（bool） ---- */
static unsigned char handle_child_lock(unsigned short dp_len, unsigned char *dp_data)
{
    unsigned char locked = mcu_get_dp_download_bool(dp_data, dp_len);
    HOOCH_PROTOCOL_SettingFrame_t sf;
    init_setting_frame(&sf, HOOCH_PROTOCOL_SETTING_ITEM_KEY_CHILD_LOCK, locked);
    sf.source = HOOCH_PROTOCOL_SOURCE_TUYA;
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);
    return 1U;
}



/* ---- DP 141: 点动时间 (raw: 3byte) ---- */
static unsigned char handle_jog_time(unsigned short dp_len, unsigned char *dp_data)
{
    if (dp_len < 3U) return 1U;
    HOOCH_PROTOCOL_SettingFrame_t sf;
    init_setting_frame(&sf, HOOCH_PROTOCOL_SETTING_ITEM_KEY_JOG_TIME, dp_data[0] + 1U);
    sf.param1 = dp_data[1];
    sf.param2 = dp_data[2];
    sf.source = HOOCH_PROTOCOL_SOURCE_TUYA;
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);
    return 1U;
}

/* ---- DP 140: 页面管理 (raw: 5byte) ---- */
static unsigned char handle_page_mgr(unsigned short dp_len, unsigned char *dp_data)
{
    if (dp_len < 5U) return 1U;

    static const HOOCH_PROTOCOL_SettingPage_t page_map[5] = {
        HOOCH_PROTOCOL_SETTING_PAGE_PAGE1,
        HOOCH_PROTOCOL_SETTING_PAGE_PAGE2,
        HOOCH_PROTOCOL_SETTING_PAGE_AIR_CONDITIONER,
        HOOCH_PROTOCOL_SETTING_PAGE_FRESH_AIR, 
        HOOCH_PROTOCOL_SETTING_PAGE_FLOOR_HEATING, 
    };

    unsigned char i;
    for (i = 0U; i < 5U; i++) {
        HOOCH_PROTOCOL_SettingFrame_t sf;
        init_setting_frame(&sf,
            (dp_data[i] == 0U)
                ? HOOCH_PROTOCOL_SETTING_ITEM_DELETE_PAGE
                : HOOCH_PROTOCOL_SETTING_ITEM_ADD_PAGE,
            dp_data[i]);
        sf.page   = page_map[i];
        sf.param1 = i;
        sf.source = HOOCH_PROTOCOL_SOURCE_TUYA;
        (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);
    }
    return 1U;
}

/* ---- DP 148: 页面同步（enum） ---- */
static unsigned char handle_page_sync(unsigned short dp_len, unsigned char *dp_data)
{
    unsigned char val = mcu_get_dp_download_enum(dp_data, dp_len);
    static const HOOCH_PROTOCOL_SettingPage_t page_map[] = {
        HOOCH_PROTOCOL_SETTING_PAGE_PAGE1,
        HOOCH_PROTOCOL_SETTING_PAGE_PAGE2,
        HOOCH_PROTOCOL_SETTING_PAGE_AIR_CONDITIONER,
        HOOCH_PROTOCOL_SETTING_PAGE_FRESH_AIR,  /* fresh_air */
        HOOCH_PROTOCOL_SETTING_PAGE_FLOOR_HEATING,  /* floor_heating */
    };
    HOOCH_PROTOCOL_SettingFrame_t sf;
    init_setting_frame(&sf, HOOCH_PROTOCOL_SETTING_ITEM_CURRENT_DISPLAY_PAGE, val);
    sf.page = (val < 5U) ? page_map[val] : HOOCH_PROTOCOL_SETTING_PAGE_INVALID;
    sf.source = HOOCH_PROTOCOL_SOURCE_TUYA;
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);
    return 1U;
}

/* ---- DP 149: 总开总关（bool） ---- */
static unsigned char handle_master_sw(unsigned short dp_len, unsigned char *dp_data)
{
    unsigned char sw = mcu_get_dp_download_bool(dp_data, dp_len);
    HOOCH_PROTOCOL_SettingFrame_t sf;
    init_setting_frame(&sf, HOOCH_PROTOCOL_SETTING_ITEM_MASTER_SW, sw);
    sf.source = HOOCH_PROTOCOL_SOURCE_TUYA;
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);
    return 1U;
}

/* ---- DP 142: 基础设置 (raw: 13byte) ----
 * 布局: [0]上电状态 [1]振动开关 [2]人感上报开关 [3]人感开关 [4]人感倒计时
 *       [5]人感灵敏度 [6-8]指示灯颜色RGB888 [9]指示灯亮度 [10]背光亮度
 *       [11]关闭指示灯 [12]屏幕待机样式
 */
static unsigned char handle_base_set(unsigned short dp_len, unsigned char *dp_data)
{
    if (dp_len < 13U) return 1U;

    static const struct {
        HOOCH_PROTOCOL_SettingItem_t item;
        unsigned char offset;
    } dispatch_table[] = {
        { HOOCH_PROTOCOL_SETTING_ITEM_KEY_POWER_ON_STATUS,      0U  },
        { HOOCH_PROTOCOL_SETTING_ITEM_VIBRATION_SWITCH,         1U  },
        { HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_REPORT,            2U  },
        { HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_SWITCH,            3U  },
        { HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_COUNTDOWN,         4U  },
        { HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_SENSITIVITY,       5U  },
        { HOOCH_PROTOCOL_SETTING_ITEM_INDICATOR_LIGHT_COLOR,    6U  }, /* RGB888: dp[6]/dp[7]/dp[8] */
        { HOOCH_PROTOCOL_SETTING_ITEM_BACKLIGHT_BRIGHTNESS,     9U  }, /* 指示灯亮度 */
        { HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_BRIGHTNESS,        10U }, /* 背光亮度 */
        { HOOCH_PROTOCOL_SETTING_ITEM_INDICATOR_LIGHT_SWITCH,   11U }, /* 关闭指示灯 */
        { HOOCH_PROTOCOL_SETTING_ITEM_SCREENSAVER_TYPE,         12U }, /* 屏幕待机样式 */
    };
    unsigned char i;
    for (i = 0U; i < sizeof(dispatch_table) / sizeof(dispatch_table[0]); i++) {
        HOOCH_PROTOCOL_SettingFrame_t sf;
        init_setting_frame(&sf, dispatch_table[i].item, 0U);

        if (dispatch_table[i].offset == 6U) {
            /* RGB888: R=dp[6], G=dp[7], B=dp[8] */
            sf.value = ((uint32_t)dp_data[6] << 16)
                     | ((uint32_t)dp_data[7] << 8)
                     |  (uint32_t)dp_data[8];
        } else {
            sf.value = dp_data[dispatch_table[i].offset];
        }
        sf.source = HOOCH_PROTOCOL_SOURCE_TUYA;
        (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);
    }
    return 1U;
}

/* ---- 涂鸦「绑定类型」→ Hooch 设备类型 ----
 * 涂鸦: 0-水机 1-WX线控 2-WX集控 3-WX485线控 4-WX485集控
 * Hooch: 1-水机 2-HF线控 3-HF线控485 4-HF集控485
 *       (AirConditioner/FreshAir/FloorHeating 三家 Type 枚举取值一致)
 * 注: 涂鸦 WX集控 与 WX485集控 在 Hooch 侧合并为 HF集控485; 未知取值按无效处理。
 */
static HOOCH_PROTOCOL_AirConditionerType_t map_bind_type(unsigned char raw)
{
    switch (raw) {
    case 0U: return HOOCH_PROTOCOL_AIR_CONDITIONER_TYPE_WATER_UNIT;
    case 1U: return HOOCH_PROTOCOL_AIR_CONDITIONER_TYPE_HF_WIRED_CONTROLLER;
    case 2U: /* fall through: WX集控 与 WX485集控 合并 */
    case 4U: return HOOCH_PROTOCOL_AIR_CONDITIONER_TYPE_HF_CENTRAL_CONTROL_485;
    case 3U: return HOOCH_PROTOCOL_AIR_CONDITIONER_TYPE_HF_WIRED_CONTROLLER_485;
    default: return HOOCH_PROTOCOL_AIR_CONDITIONER_TYPE_INVALID;
    }
}

/* 空调绑定类型与集控地址：拆成两项分别下发 */
static void dispatch_ac_bind(unsigned char raw_type, unsigned char address)
{
    HOOCH_PROTOCOL_AirConditionerFrame_t frame;

    memset(&frame, 0, sizeof(frame));
    frame.channel = 1U;    /* 涂鸦单空调：固定通道 1 */
    frame.source  = HOOCH_PROTOCOL_SOURCE_TUYA;

    frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_AC_TYPE;
    frame.value        = (uint16_t)map_bind_type(raw_type);
    if (frame.value != (uint16_t)HOOCH_PROTOCOL_AIR_CONDITIONER_TYPE_INVALID) {
        (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&frame);
    }

    frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_CENTRAL_CONTROL_ADDRESS;
    frame.value        = address;
    (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&frame);
}

/* 新风绑定类型与集控地址：拆成两项分别下发 */
static void dispatch_fresh_air_bind(unsigned char raw_type, unsigned char address)
{
    HOOCH_PROTOCOL_FreshAirFrame_t frame;

    memset(&frame, 0, sizeof(frame));
    frame.channel = 1U;    /* 涂鸦单新风：固定通道 1 */
    frame.source  = HOOCH_PROTOCOL_SOURCE_TUYA;

    frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_TYPE;
    frame.value        = (uint16_t)(HOOCH_PROTOCOL_FreshAirType_t)map_bind_type(raw_type);
    if (frame.value != (uint16_t)HOOCH_PROTOCOL_FRESH_AIR_TYPE_INVALID) {
        (void)HOOCH_PROTOCOL_FreshAir_DispatchFrame(&frame);
    }

    frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_CENTRAL_CONTROL_ADDRESS;
    frame.value        = address;
    (void)HOOCH_PROTOCOL_FreshAir_DispatchFrame(&frame);
}

/* 地暖绑定类型与集控地址：拆成两项分别下发 */
static void dispatch_floor_bind(unsigned char raw_type, unsigned char address)
{
    HOOCH_PROTOCOL_FloorHeatingFrame_t frame;

    memset(&frame, 0, sizeof(frame));
    frame.channel = 1U;    /* 涂鸦单地暖：固定通道 1 */
    frame.source  = HOOCH_PROTOCOL_SOURCE_TUYA;

    frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TYPE;
    frame.value        = (uint16_t)(HOOCH_PROTOCOL_FloorHeatingType_t)map_bind_type(raw_type);
    if (frame.value != (uint16_t)HOOCH_PROTOCOL_FLOOR_HEATING_TYPE_INVALID) {
        (void)HOOCH_PROTOCOL_FloorHeating_DispatchFrame(&frame);
    }

    frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_CENTRAL_CONTROL_ADDRESS;
    frame.value        = address;
    (void)HOOCH_PROTOCOL_FloorHeating_DispatchFrame(&frame);
}

/* ---- DP 143: 高级设置 (raw: 8byte 旧版本 / 14byte 新版本) ----
 * 布局: [0]指示灯高亮 0~100
 *       [1-5]页面排序(5byte "12345")
 *       [6]温度传感器矫正(int8 -10~+10)
 *       [7]湿度传感器矫正(int8 -20%~+20%)
 *       [8]空调绑定类型  [9]空调地址
 *       [10]新风绑定类型 [11]新风地址
 *       [12]地暖绑定类型 [13]地暖地址
 * 注: [8]~[13] 为新增字段, 旧固件仅下发 8byte, 长度不足时跳过。
 */
static unsigned char handle_adv_set(unsigned short dp_len, unsigned char *dp_data)
{
    HOOCH_PROTOCOL_SettingFrame_t sf;

    if (dp_len < 8U) return 1U;

    /* [0] 指示灯高亮 0~100 */
    init_setting_frame(&sf, HOOCH_PROTOCOL_SETTING_ITEM_BACKLIGHT_BRIGHTNESS_HIGHLIGHT,
                       dp_data[0]);
    sf.source = HOOCH_PROTOCOL_SOURCE_TUYA;
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);

    /* [1-5] 页面排序 (5byte) */
    init_setting_frame(&sf, HOOCH_PROTOCOL_SETTING_ITEM_PAGE_ORDER, 0U);
    memset(sf.data, 0, sizeof(sf.data));
    memcpy(sf.data, &dp_data[1], 5U);
    sf.source = HOOCH_PROTOCOL_SOURCE_TUYA;
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);

    /* [6] 温度传感器矫正: int8 符号扩展后存入 value */
    init_setting_frame(&sf, HOOCH_PROTOCOL_SETTING_ITEM_TEMPERATURE_CALIBRATION, 0U);
    sf.value = (uint32_t)(int32_t)(int8_t)dp_data[6];
    sf.source = HOOCH_PROTOCOL_SOURCE_TUYA;
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);

    /* [7] 湿度传感器矫正: int8 符号扩展后存入 value */
    init_setting_frame(&sf, HOOCH_PROTOCOL_SETTING_ITEM_HUMIDITY_CALIBRATION, 0U);
    sf.value = (uint32_t)(int32_t)(int8_t)dp_data[7];
    sf.source = HOOCH_PROTOCOL_SOURCE_TUYA;
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);

    /* ---- 以下为新增字段，旧固件只下发 8byte，长度不足时跳过 ---- */
    if (dp_len < 14U) return 1U;

    /* [8][9] 空调绑定类型 / 空调地址 */
    dispatch_ac_bind(dp_data[8], dp_data[9]);

    /* [10][11] 新风绑定类型 / 新风地址 */
    dispatch_fresh_air_bind(dp_data[10], dp_data[11]);

    /* [12][13] 地暖绑定类型 / 地暖地址 */
    dispatch_floor_bind(dp_data[12], dp_data[13]);

    return 1U;
}





unsigned char tuya_dp_dispatch_setting(unsigned char dp_id, unsigned char dp_type,
                                       unsigned short dp_len, unsigned char *dp_data)
{
    (void)dp_type;

    switch (dp_id) {
    case DPID_THEME:      return handle_theme(dp_len, dp_data);
    case DPID_CHILD_LOCK: return handle_child_lock(dp_len, dp_data);
    case DPID_JOG_TIME:   return handle_jog_time(dp_len, dp_data);
    case DPID_PAGE_MGR:   return handle_page_mgr(dp_len, dp_data);
    case DPID_PAGE_SYNC:  return handle_page_sync(dp_len, dp_data);
    case DPID_MASTER_SW:  return handle_master_sw(dp_len, dp_data);
    case DPID_BASE_SET:   return handle_base_set(dp_len, dp_data);
    case DPID_ADV_SET:    return handle_adv_set(dp_len, dp_data);
    default: break;
    }
    return 0U;
}
