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
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);
    return 1U;
}

/* ---- DP 146: 童锁（bool） ---- */
static unsigned char handle_child_lock(unsigned short dp_len, unsigned char *dp_data)
{
    unsigned char locked = mcu_get_dp_download_bool(dp_data, dp_len);
    HOOCH_PROTOCOL_SettingFrame_t sf;
    init_setting_frame(&sf, HOOCH_PROTOCOL_SETTING_ITEM_KEY_CHILD_LOCK, locked);
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
        HOOCH_PROTOCOL_SETTING_PAGE_INVALID,  /* fresh_air */
        HOOCH_PROTOCOL_SETTING_PAGE_INVALID,  /* floor_heating */
    };
    HOOCH_PROTOCOL_SettingFrame_t sf;
    init_setting_frame(&sf, HOOCH_PROTOCOL_SETTING_ITEM_CURRENT_DISPLAY_PAGE, val);
    sf.page = (val < 5U) ? page_map[val] : HOOCH_PROTOCOL_SETTING_PAGE_INVALID;
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);
    return 1U;
}

/* ---- DP 149: 总开总关（bool） ---- */
static unsigned char handle_master_sw(unsigned short dp_len, unsigned char *dp_data)
{
    unsigned char sw = mcu_get_dp_download_bool(dp_data, dp_len);
    HOOCH_PROTOCOL_SettingFrame_t sf;
    init_setting_frame(&sf, HOOCH_PROTOCOL_SETTING_ITEM_MASTER_SW, sw);
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
        (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);
    }
    return 1U;
}

/* ---- DP 143: 高级设置 (raw: 8byte) ----
 * 布局: [0]指示灯高亮 0~100
 *       [1-5]页面排序(5byte "12345")
 *       [6]温度传感器矫正(int8 -10~+10)
 *       [7]湿度传感器矫正(int8 -20%~+20%)
 */
static unsigned char handle_adv_set(unsigned short dp_len, unsigned char *dp_data)
{
    HOOCH_PROTOCOL_SettingFrame_t sf;

    if (dp_len < 8U) return 1U;

    /* [0] 指示灯高亮 0~100 */
    init_setting_frame(&sf, HOOCH_PROTOCOL_SETTING_ITEM_BACKLIGHT_BRIGHTNESS_HIGHLIGHT,
                       dp_data[0]);
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);

    /* [1-5] 页面排序 (5byte) */
    init_setting_frame(&sf, HOOCH_PROTOCOL_SETTING_ITEM_PAGE_ORDER, 0U);
    memset(sf.data, 0, sizeof(sf.data));
    memcpy(sf.data, &dp_data[1], 5U);
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);

    /* [6] 温度传感器矫正: int8 符号扩展后存入 value */
    init_setting_frame(&sf, HOOCH_PROTOCOL_SETTING_ITEM_TEMPERATURE_CALIBRATION, 0U);
    sf.value = (uint32_t)(int32_t)(int8_t)dp_data[6];
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);

    /* [7] 湿度传感器矫正: int8 符号扩展后存入 value */
    init_setting_frame(&sf, HOOCH_PROTOCOL_SETTING_ITEM_HUMIDITY_CALIBRATION, 0U);
    sf.value = (uint32_t)(int32_t)(int8_t)dp_data[7];
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);

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
