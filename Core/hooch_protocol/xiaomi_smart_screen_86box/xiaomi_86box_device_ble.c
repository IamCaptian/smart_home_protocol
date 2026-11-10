/*
 * Xiaomi 86 Box — BLE 基本资料/屏显参数处理
 */
#include "xiaomi_86box_device_ble.h"

/* 屏显参数设置: 参数1=功能类型, 参数2~5=值(大端) */
static void xiaomi_86box_ble_handle_screen_param_set(const uint8_t *params)
{
    uint8_t func_type = params[0];
    uint32_t value = xiaomi_86box_read_be32(&params[1]);

    switch (func_type)
    {
    case 0U:    /* 屏保选择 */
        xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_SCREENSAVER_TYPE,
                                      value, HOOCH_PROTOCOL_SETTING_PAGE_INVALID);
        break;
    case 1U:    /* 熄屏效果 */
        xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_OFF_EFFECT,
                                      value, HOOCH_PROTOCOL_SETTING_PAGE_INVALID);
        break;
    case 2U:    /* 主题页 */
        xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_THEME,
                                      value, HOOCH_PROTOCOL_SETTING_PAGE_INVALID);
        break;
    case 3U:    /* 熄屏时长(秒), 0xFFFFFFFF 表示永不熄屏 */
        xiaomi_86box_dispatch_screen_off_time(value);
        break;
    default:
        break;
    }
}

/* 屏显隐藏参数设置: 参数1=设备类型, 参数3 bit0=显示/隐藏 */
static void xiaomi_86box_ble_handle_page_hide_set(const uint8_t *params)
{
    HOOCH_PROTOCOL_SettingPage_t page = xiaomi_86box_page_from_device_type(params[0]);
    uint8_t visible = (uint8_t)(params[2] & 0x01U);

    if (page != HOOCH_PROTOCOL_SETTING_PAGE_INVALID)
    {
        xiaomi_86box_dispatch_setting(
            (visible != 0U) ? HOOCH_PROTOCOL_SETTING_ITEM_ADD_PAGE
                            : HOOCH_PROTOCOL_SETTING_ITEM_DELETE_PAGE,
            visible, page);
    }
}

/* 三合一当前工作界面类型: 1-空调 2-新风 3-地暖 */
static void xiaomi_86box_ble_handle_page_type_set(uint8_t raw_page)
{
    HOOCH_PROTOCOL_SettingPage_t page;

    switch (raw_page)
    {
    case 1U: page = HOOCH_PROTOCOL_SETTING_PAGE_AIR_CONDITIONER; break;
    case 2U: page = HOOCH_PROTOCOL_SETTING_PAGE_FRESH_AIR;       break;
    case 3U: page = HOOCH_PROTOCOL_SETTING_PAGE_FLOOR_HEATING;   break;
    default: page = HOOCH_PROTOCOL_SETTING_PAGE_INVALID;         break;
    }

    if (page != HOOCH_PROTOCOL_SETTING_PAGE_INVALID)
    {
        xiaomi_86box_dispatch_setting(
            HOOCH_PROTOCOL_SETTING_ITEM_CURRENT_DISPLAY_PAGE, raw_page, page);
    }
}

uint8_t xiaomi_86box_device_ble_handle(const xiaomi_86box_device_header_t *header,
                                       uint8_t subcmd,
                                       const uint8_t *params,
                                       uint16_t params_len)
{
    (void)header;

    switch (subcmd)
    {
    case XIAOMI_86BOX_BLE_CHILD_LOCK_SET:
        if (params_len >= 1U)
        {
            xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_KEY_CHILD_LOCK,
                                          params[0], HOOCH_PROTOCOL_SETTING_PAGE_INVALID);
        }
        break;

    case XIAOMI_86BOX_BLE_SCREEN_PARAM_SET:
        if (params_len >= 5U)
        {
            xiaomi_86box_ble_handle_screen_param_set(params);
        }
        break;

    case XIAOMI_86BOX_BLE_SCREEN_PARAM_GET:
    {
        uint8_t reply_params[5];
        uint8_t func_type = (params_len >= 1U) ? params[0] : 0U;

        reply_params[0] = func_type;
        reply_params[1] = 0U;
        reply_params[2] = 0U;
        reply_params[3] = 0U;
        reply_params[4] = 0U;
        (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_BLE_BASIC, 1U, 0U,
                                       XIAOMI_86BOX_BLE_SCREEN_PARAM_GET,
                                       reply_params, sizeof(reply_params));
        return 0U;
    }

    case XIAOMI_86BOX_BLE_WEATHER_GET:
    {
        uint8_t reply_params[4];
        uint32_t weather = xiaomi_86box_get_weather_code();

        reply_params[0] = (uint8_t)(weather >> 24);
        reply_params[1] = (uint8_t)(weather >> 16);
        reply_params[2] = (uint8_t)(weather >> 8);
        reply_params[3] = (uint8_t)(weather);
        (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_BLE_BASIC, 1U, 0U,
                                       XIAOMI_86BOX_BLE_WEATHER_GET,
                                       reply_params, sizeof(reply_params));
        return 0U;
    }

    case XIAOMI_86BOX_BLE_WEATHER_REPORT:
        if (params_len >= 4U)
        {
            uint32_t weather = xiaomi_86box_read_be32(params);

            xiaomi_86box_set_weather_code(weather);
            xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_WEATHER,
                                          weather, HOOCH_PROTOCOL_SETTING_PAGE_INVALID);
        }
        break;

    case XIAOMI_86BOX_BLE_BUZZER_SET:
        if (params_len >= 1U)
        {
            xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_SOUND_ENABLE,
                                          params[0], HOOCH_PROTOCOL_SETTING_PAGE_INVALID);
        }
        break;

    case XIAOMI_86BOX_BLE_PAGE_HIDE_SET:
        if (params_len >= 3U)
        {
            xiaomi_86box_ble_handle_page_hide_set(params);
        }
        break;

    case XIAOMI_86BOX_BLE_PAGE_HIDE_GET:
    {
        uint8_t reply_params[3];

        reply_params[0] = (params_len >= 1U) ? params[0] : 0U;
        reply_params[1] = 0x01U;
        reply_params[2] = 0x01U;
        (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_BLE_BASIC, 1U, 0U,
                                       XIAOMI_86BOX_BLE_PAGE_HIDE_GET,
                                       reply_params, sizeof(reply_params));
        return 0U;
    }

    case XIAOMI_86BOX_BLE_PAGE_TYPE_SET:
        if (params_len >= 1U)
        {
            xiaomi_86box_ble_handle_page_type_set(params[0]);
        }
        break;

    case XIAOMI_86BOX_BLE_BACKLIGHT_SWITCH:
        if (params_len >= 1U)
        {
            xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_BACKLIGHT_SWITCH,
                                          params[0], HOOCH_PROTOCOL_SETTING_PAGE_INVALID);
        }
        break;

    case XIAOMI_86BOX_BLE_BRIGHTNESS_SET:
        if (params_len >= 1U)
        {
            xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_BACKLIGHT_BRIGHTNESS,
                                          params[0], HOOCH_PROTOCOL_SETTING_PAGE_INVALID);
        }
        break;

    case XIAOMI_86BOX_BLE_SCREEN_BRIGHTNESS:
        if (params_len >= 1U)
        {
            xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_BRIGHTNESS,
                                          params[0], HOOCH_PROTOCOL_SETTING_PAGE_INVALID);
        }
        break;

    case XIAOMI_86BOX_BLE_SCREEN_OFF_TIME:
        if (params_len >= 2U)
        {
            uint32_t seconds = xiaomi_86box_read_be16(params);
            xiaomi_86box_dispatch_screen_off_time(seconds);
        }
        break;

    case XIAOMI_86BOX_BLE_SCREEN_OFF_EFFECT:
        if (params_len >= 1U)
        {
            xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_OFF_EFFECT,
                                          params[0], HOOCH_PROTOCOL_SETTING_PAGE_INVALID);
        }
        break;

    case XIAOMI_86BOX_BLE_TIME_REPORT:
    case XIAOMI_86BOX_BLE_TEMP_HUMIDITY:
    case XIAOMI_86BOX_BLE_PRODUCT_ID:
    case XIAOMI_86BOX_BLE_CODE_LIBRARY:
    case XIAOMI_86BOX_BLE_MODEL_PID_SET:
        /* 当前产品不消费这些数据, 仅记录 */
        XIAOMI_86BOX_LOG_INFO("[86BOX] BLE basic subcmd 0x%02X ignored\r\n",
                              (unsigned int)subcmd);
        break;

    case XIAOMI_86BOX_BLE_BACKLIGHT_SET:
        if (params_len >= 1U)
        {
            xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_BACKLIGHT_SWITCH,
                                          params[0], HOOCH_PROTOCOL_SETTING_PAGE_INVALID);
        }
        break;

    default:
        return 0U;
    }

    return 1U;
}
