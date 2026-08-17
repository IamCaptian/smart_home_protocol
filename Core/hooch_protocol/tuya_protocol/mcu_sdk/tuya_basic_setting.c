/*
 * Tuya DP Dispatch — Basic Settings (DP 132, 133, 140, 141, 142, 144, 146, 148, 149)
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

/* ---- DP 132/133: 空调/地暖温度上下限 (raw: 2byte) ---- */
static unsigned char handle_temp_limit(unsigned char dp_id,
                                       unsigned short dp_len, unsigned char *dp_data)
{
    if (dp_len < 2U) return 1U; /* dp_id matched but data invalid — still handled */
    HOOCH_PROTOCOL_SettingFrame_t sf;
    init_setting_frame(&sf,
        (dp_data[0] == 0U)
            ? HOOCH_PROTOCOL_SETTING_ITEM_AIR_CONDITIONER_TEMPERATURE_MAX
            : HOOCH_PROTOCOL_SETTING_ITEM_AIR_CONDITIONER_TEMPERATURE_MIN,
        dp_data[1]);
    (void)dp_id; /* both DPID_HEAT_LIMIT and DPID_AC_LIMIT map identically */
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);
    return 1U;
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

/* ---- DP 142: 基础设置 (raw: 11byte) ---- */
static unsigned char handle_base_set(unsigned short dp_len, unsigned char *dp_data)
{
    if (dp_len < 11U) return 1U;

    static const struct {
        HOOCH_PROTOCOL_SettingItem_t item;
        unsigned char offset;
    } dispatch_table[] = {
        { HOOCH_PROTOCOL_SETTING_ITEM_KEY_POWER_ON_STATUS, 0U  },
        { HOOCH_PROTOCOL_SETTING_ITEM_VIBRATION_SWITCH,    1U  },
        { HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_REPORT,       2U  },
        { HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_SWITCH,       3U  },
        { HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_COUNTDOWN,    4U  },
        { HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_SENSITIVITY,  5U  },
        { HOOCH_PROTOCOL_SETTING_ITEM_KEY_STATUS_LIGHT,    6U  }, /* RGB: param1=color */
        { HOOCH_PROTOCOL_SETTING_ITEM_KEY_STATUS_LIGHT,    9U  }, /* brightness: param1=0 */
        { HOOCH_PROTOCOL_SETTING_ITEM_BACKLIGHT_BRIGHTNESS, 10U },
    };
    unsigned char i;
    for (i = 0U; i < sizeof(dispatch_table) / sizeof(dispatch_table[0]); i++) {
        HOOCH_PROTOCOL_SettingFrame_t sf;
        init_setting_frame(&sf, dispatch_table[i].item, 0U);

        if (dispatch_table[i].offset == 6U) {
            /* RGB888: R=dp[6], G=dp[7], B=dp[8] */
            sf.value  = ((uint32_t)dp_data[6] << 16)
                      | ((uint32_t)dp_data[7] << 8)
                      |  (uint32_t)dp_data[8];
            sf.param1 = 0U; /* 颜色 */
        } else if (dispatch_table[i].offset == 9U) {
            sf.value  = dp_data[9];
            sf.param1 = 1U; /* 亮度 */
        } else {
            sf.value  = dp_data[dispatch_table[i].offset];
        }
        (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);
    }
    return 1U;
}

unsigned char tuya_dp_dispatch_setting(unsigned char dp_id, unsigned char dp_type,
                                       unsigned short dp_len, unsigned char *dp_data)
{
    (void)dp_type;

    switch (dp_id) {
    case DPID_HEAT_LIMIT: /* fall through */
    case DPID_AC_LIMIT:   return handle_temp_limit(dp_id, dp_len, dp_data);
    case DPID_THEME:      return handle_theme(dp_len, dp_data);
    case DPID_CHILD_LOCK: return handle_child_lock(dp_len, dp_data);
    case DPID_JOG_TIME:   return handle_jog_time(dp_len, dp_data);
    case DPID_PAGE_MGR:   return handle_page_mgr(dp_len, dp_data);
    case DPID_PAGE_SYNC:  return handle_page_sync(dp_len, dp_data);
    case DPID_MASTER_SW:  return handle_master_sw(dp_len, dp_data);
    case DPID_BASE_SET:   return handle_base_set(dp_len, dp_data);
    default: break;
    }
    return 0U;
}
