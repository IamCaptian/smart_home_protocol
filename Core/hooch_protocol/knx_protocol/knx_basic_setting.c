/*
 * KNX Basic Setting Summary
 *
 * Panel Communication Protocol V2.4, fun1=3(BASIC_SETTING)
 */
#include "knx_internal.h"

/*基本配置处理*/
void knx_summary_basic_setting(const KNX_Frame_t *frame)
{
    const char *item_desc;
    uint8_t need_set;
    HOOCH_PROTOCOL_SettingFrame_t setting_frame;
    if ((frame == NULL) || (frame->fun_count < 2U))
    {
        return;
    }

    item_desc = KNX_DESC("Unknown Basic Setting");
    need_set = 0U;
    knx_setting_frame_reset(&setting_frame);

    /* 基本配置：item(fun2) -> 功能描述 */
    switch (frame->fun[1])
    {
    case KNX_BASIC_SETTING_SCREENSAVER_TYPE:
        item_desc = KNX_DESC("Screensaver Type (K->P)"); /* 屏保类型 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_SCREENSAVER_TYPE;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_THEME:
        item_desc = KNX_DESC("Theme (K->P)"); /* 主题 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_THEME;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_BACKGROUND:
        item_desc = KNX_DESC("Background (K->P)"); /* 背景 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_BACKGROUND;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_DEVICE_LOCK_CONTROL:
        item_desc = KNX_DESC("Device Lock Enable/Disable (K->P)"); /* 设备锁启用/禁用 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_DEVICE_LOCK;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_KEY_CHILD_LOCK:
        item_desc = KNX_DESC("Key Child Lock (K->P)"); /* 按键童锁 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_KEY_CHILD_LOCK;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_SCREEN_BACKLIGHT_BRIGHTNESS:
        item_desc = KNX_DESC("Screen Backlight Brightness (K->P)"); /* 屏幕背光亮度 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_BRIGHTNESS;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = (uint8_t)(((uint32_t)frame->data[0] * 100U) / 255U);
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_AUTO_BRIGHTNESS_ENABLE:
        item_desc = KNX_DESC("Auto Brightness Enable (K->P)"); /* 自动亮度使能 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_AUTO_BRIGHTNESS_ENABLE;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_KEY_BACKLIGHT_BRIGHTNESS_NIGHT:
        item_desc = KNX_DESC("Key Backlight Brightness (Night) (K->P)"); /* 按键背光亮度(黑夜) */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_KEY_BACKLIGHT_BRIGHTNESS_NIGHT;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = (uint8_t)(((uint32_t)frame->data[0] * 100U) / 255U);
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_KEY_BRIGHTNESS_NIGHT:
        item_desc = KNX_DESC("Key Brightness (Night) (K->P)"); /* 按键亮度(黑夜) */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_KEY_BRIGHTNESS_NIGHT;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = (uint8_t)(((uint32_t)frame->data[0] * 100U) / 255U);
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_KEY_BACKLIGHT_MODE:
        item_desc = KNX_DESC("Key Backlight Mode (K->P)"); /* 按键背光模式: 0=常亮, 1=延时 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_KEY_BACKLIGHT_MODE;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_KEY_WAKEUP_BACKLIGHT_ENABLE:
        item_desc = KNX_DESC("Key Wake Backlight Enable (K->P)"); /* 按键唤醒背光使能 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_KEY_WAKEUP_BACKLIGHT_ENABLE;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_KEY_STATUS_LINK_BACKLIGHT:
        item_desc = KNX_DESC("Key State Linked Backlight (K->P)"); /* 按键状态关联背光 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_KEY_STATE_LINKED_BACKLIGHT;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_KEY_LONG_PRESS_TIME:
        item_desc = KNX_DESC("Key Long Press Time (K->P)");     /* 按键长按时间 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_KEY_LONG_PRESS_TIME;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_KEY_MULTI_EFFECTIVE:
        item_desc = KNX_DESC("Keys Apply Simultaneously (K->P)"); /* 按键是否同时生效 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_KEYS_APPLY_SIMULTANEOUSLY;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_KEY_BACKLIGHT_TIME:
        item_desc = KNX_DESC("Key Backlight Time (K->P)"); /* 按键背光时间 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_KEY_BACKLIGHT_TIME;
        if (frame->data_len >= 2U)
        {
            setting_frame.value = ((uint32_t)frame->data[0] << 8U) | frame->data[1];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_PRESENCE_SENSOR_ENABLE:
        item_desc = KNX_DESC("Sensor Enable (K->P)"); /* 传感器使能 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_SWITCH;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_PRESENCE_SENSOR_SENSITIVITY:
        item_desc = KNX_DESC("Presence Sensor Sensitivity (K->P)"); /* 人存传感器灵敏度 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_SENSITIVITY;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_KEY_BACKLIGHT_BRIGHTNESS_DAY:
        item_desc = KNX_DESC("Key Backlight Brightness Day (K->P)"); /* 按键背光亮度(白天) */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_KEY_BACKLIGHT_BRIGHTNESS_DAY;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = (uint8_t)(((uint32_t)frame->data[0] * 100U) / 255U);
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_KEY_BRIGHTNESS_DAY:
            item_desc = KNX_DESC("Key Backlight Brightness Day (K->P)"); /* 按键背光亮度(白天) */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_KEY_BRIGHTNESS_DAY;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = (uint8_t)(((uint32_t)frame->data[0] * 100U) / 255U);
        }
        need_set = 1U;
        item_desc = KNX_DESC("Key Brightness Day (K->P)"); /* 按键亮度(白天) */
        break;
    case KNX_BASIC_SETTING_SCREEN_STANDBY_BRIGHTNESS:
        item_desc = KNX_DESC("Standby Brightness (K->P)"); /* 待机亮度 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_STANDBY_BRIGHTNESS;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = (uint8_t)(((uint32_t)frame->data[0] * 100U) / 255U);
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_SCREEN_SCREENSAVER_ENABLE:
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_OFF_TIME;
        if (frame->data_len >= 1U)
        {
            if(frame->data[0] == 1)
            {
                setting_frame.value = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_BACKLIGHT_OFF;
            }else
            {
                setting_frame.value = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_AUTO;
            }
        }
        need_set = 1U;
        item_desc = KNX_DESC("Screensaver Enable (K->P)"); /* 屏保启用 */
        break;
    case KNX_BASIC_SETTING_SCREEN_CHILD_LOCK:
        item_desc = KNX_DESC("Screen Child Lock (K->P)"); /* 屏体童锁 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_CHILD_LOCK;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_SCREEN_OFF_TIME:
        item_desc = KNX_DESC("Screen Off Time (K->P)"); /* 息屏时间 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_OFF_TIME;
        if (frame->data_len >= 2U)
        {
            setting_frame.value = frame->data[0] << 8U;
            setting_frame.value |= frame->data[1];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_RETURN_HOME_TIME:
        item_desc = KNX_DESC("Back To Home Time (K->P)"); /* 返回主页时间 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_BACK_TO_HOME_TIME;
        if (frame->data_len >= 2U)
        {
            setting_frame.value = ((uint32_t)frame->data[0] << 8U) | frame->data[1];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_ENTER_SCREENSAVER_TIME:
        item_desc = KNX_DESC("Enter Screensaver Time (K->P)"); /* 进入屏保时间 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_ENTER_SCREENSAVER_TIME;
        if (frame->data_len >= 2U)
        {
            setting_frame.value = ((uint32_t)frame->data[0] << 8U) | frame->data[1];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_SCREEN_PASSWORD:
        item_desc = KNX_DESC("Screen Password (K->P)"); /* 屏密码 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_PASSWORD;
        setting_frame.param1 = (frame->fun_count >= 3U) ? frame->fun[2] : 0U;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_TEMPERATURE_UNIT:
        item_desc = KNX_DESC("Temperature Unit (K->P)"); /* 温度显示单位 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_TEMPERATURE_UNIT;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_PAGE_BUTTON_FUNCTION:
        item_desc = KNX_DESC("Page Key Loaded Function (K<->P)"); /* 页面按键装载的功能 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_PAGE_KEY_LOADED_FUNCTION;
        setting_frame.channel = (frame->fun_count >= 3U) ? frame->fun[2] : 0U;
        if (frame->data_len >= 2U)
        {
            setting_frame.value = ((uint32_t)frame->data[0] << 8U) | frame->data[1];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_LANGUAGE_SELECT:
        item_desc = KNX_DESC("Language (K->P)"); /* 语言选择 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_LANGUAGE;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_SOUND_ENABLE:
        item_desc = KNX_DESC("Sound Enable (K->P)"); /* 声音使能 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_SOUND_ENABLE;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_VIBRATION_ENABLE:
        item_desc = KNX_DESC("Vibration Enable (K->P)"); /* 震动使能 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_VIBRATION_SWITCH;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    case KNX_BASIC_SETTING_PAGE_MANAGEMENT:
        item_desc = KNX_DESC("Page Management (K->P)"); /* 页面管理 */
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_AVAILABLE_PAGE;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
        }
        need_set = 1U;
        break;
    default:
        break;
    }

    if (need_set != 0U)
    {
        if (frame->data_len >= 1U)
        {
            (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
        }
    }

    KNX_SUMMARY_EMIT_ITEM(item_desc, frame);
}
