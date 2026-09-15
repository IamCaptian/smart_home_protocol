/*
 * Tuya DP Dispatch — Key Mode & Key Name (DP 139, 145)
 */
#include "tuya_device_key.h"
#include "protocol.h"
#include "hooch_key_mode.h"
#include "hooch_key_name.h"
#include "hooch_setting.h"
#include <string.h>

/* ---- DP 139: 按键模式 (raw: 2byte) ---- */
static unsigned char handle_key_mode(unsigned short dp_len, unsigned char *dp_data)
{
    /* dp_data[0]: 按键通道 (1-based: 1~8)，dp_data[1]: 按键模式 (0-based) */
    if (dp_len < 2U || dp_data[0] < 1U || dp_data[0] > 8U) return 0U;
    HOOCH_PROTOCOL_KeyModeKey_t key = (HOOCH_PROTOCOL_KeyModeKey_t)(dp_data[0]);

    /* Tuya 按键模式 → Hooch KeyModeType
     *  0~3: switch_1~4          → NORMAL_SWITCH + RELAY_CONFIG
     *  4~7: unreal_switch_1~4   → WIRELESS_SWITCH
     *  8~11:jog_1~4             → MOMENTARY_SWITCH
     *  12:  light               → DIMMER_SWITCH
     *  13:  curtain             → CURTAIN_SWITCH
     *  14:  scene               → SCENARIO_SWITCH */
    HOOCH_PROTOCOL_KeyModeType_t type;
    if (dp_data[1] <= 3U) {
        type = HOOCH_PROTOCOL_KEY_MODE_TYPE_NORMAL_SWITCH;
    } else if (dp_data[1] <= 7U) {
        type = HOOCH_PROTOCOL_KEY_MODE_TYPE_VIRTUAL_SWITCH;
    } else if (dp_data[1] <= 11U) {
        type = HOOCH_PROTOCOL_KEY_MODE_TYPE_MOMENTARY_SWITCH;
    } else if (dp_data[1] == 12U) {
        type = HOOCH_PROTOCOL_KEY_MODE_TYPE_DIMMER_SWITCH;
    } else if (dp_data[1] == 13U) {
        type = HOOCH_PROTOCOL_KEY_MODE_TYPE_CURTAIN_SWITCH;
    } else if (dp_data[1] == 14U) {
        type = HOOCH_PROTOCOL_KEY_MODE_TYPE_SCENARIO_SWITCH;
    }else {
        return 0U;
    }
    (void)HOOCH_PROTOCOL_KeyMode_Send(key, HOOCH_PROTOCOL_KEY_MODE_POWER_ON_MEMORY, type);

    /* 如果是 switch_1~4 (NORMAL_SWITCH)，同步进一次 RELAY_CONFIG  */
    if (dp_data[1] <= 3U) {
        HOOCH_PROTOCOL_SettingFrame_t sf;
        sf.item                 = HOOCH_PROTOCOL_SETTING_ITEM_KEY_RELAY_MAPPING;
        sf.value               = key;
        sf.screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_INVALID;
        sf.page                = HOOCH_PROTOCOL_SETTING_PAGE_INVALID;
        sf.param1              = dp_data[1];
        sf.param2              = 0U;
        sf.param3              = 0U;
        sf.param4              = 0U;
        sf.sequence            = 0U;
        sf.valid               = 0U;
        (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);
    }

    /* 如果是 jog_1~4 (MOMENTARY_SWITCH)，也进一次 RELAY_CONFIG  */
    if (dp_data[1] >= 8U && dp_data[1] <= 11U) {
        HOOCH_PROTOCOL_SettingFrame_t sf;
        sf.item                 = HOOCH_PROTOCOL_SETTING_ITEM_KEY_RELAY_MAPPING;
        sf.value               = key;
        sf.screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_INVALID;
        sf.page                = HOOCH_PROTOCOL_SETTING_PAGE_INVALID;
        sf.param1              = dp_data[1] - 8U;
        sf.param2              = 0U;
        sf.param3              = 0U;
        sf.param4              = 0U;
        sf.sequence            = 0U;
        sf.valid               = 0U;
        (void)HOOCH_PROTOCOL_Setting_SetFrame(&sf);
    }
    return 1U;
}

/* ---- DP 145: 按键名称 (raw) ---- */
static unsigned char handle_key_name(unsigned short dp_len, unsigned char *dp_data)
{
    if (dp_len < 3U) return 0U;

    HOOCH_PROTOCOL_KeyNameFrame_t frame;
    frame.key = (HOOCH_PROTOCOL_KeyNameKey_t)(dp_data[0]);
    frame.icon_index = (HOOCH_PROTOCOL_KeyNameIconIndex_t)dp_data[1];
    frame.page     = HOOCH_PROTOCOL_KEY_NAME_PAGE_INVALID;
    frame.delivery = (frame.key == HOOCH_PROTOCOL_KEY_NAME_KEY_INVALID)
                     ? HOOCH_PROTOCOL_KEY_NAME_DELIVERY_OFFSCREEN_TEXT
                     : HOOCH_PROTOCOL_KEY_NAME_DELIVERY_KEY;

    unsigned char name_len = dp_data[2];
    if (name_len > (unsigned char)(dp_len - 3U)) name_len = (unsigned char)(dp_len - 3U);
    uint16_t copy_len = name_len;
    if (copy_len > (HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH - 1U))
        copy_len = (HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH - 1U);
    if (copy_len > 0U) {
        (void)memcpy(frame.name, &dp_data[3], copy_len);
    }
    frame.name[copy_len] = '\0';
    frame.sequence = 0U;
    frame.valid    = 0U;
    HOOCH_PROTOCOL_KeyName_SetFrame(&frame);
    return 1U;
}

unsigned char tuya_dp_dispatch_key(unsigned char dp_id, unsigned char dp_type,
                                   unsigned short dp_len, unsigned char *dp_data)
{
    (void)dp_type;

    switch (dp_id) {
    case DPID_KEY_MODE: return handle_key_mode(dp_len, dp_data);
    case DPID_KEY_NAME: return handle_key_name(dp_len, dp_data);
    default: break;
    }
    return 0U;
}
