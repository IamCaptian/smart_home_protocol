/*
 * KNX Basic Function Summary
 *
 * Panel Communication Protocol V2.4, fun1=1(BASIC_FUNCTION)
 */
#include "knx_internal.h"

/* 下发"立即进入/退出息屏"动作，面板收到即刻执行 */
static void knx_dispatch_screen_sleep_action(HOOCH_PROTOCOL_SettingScreenSleepAction_t action)
{
    HOOCH_PROTOCOL_SettingFrame_t setting_frame;

    knx_setting_frame_reset(&setting_frame);
    setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_SLEEP_ACTION;
    setting_frame.value = (uint32_t)action;
    setting_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
}

/*基本功能处理*/
void knx_summary_basic_function(const KNX_Frame_t *frame)
{
    const char *item_desc;

    if ((frame == NULL) || (frame->fun_count < 2U))
    {
        return;
    }

    item_desc = KNX_DESC("Unknown Basic Function");

    /* 基本功能：item(fun2) -> 功能描述。参照 面板通信协议V2.4 Sheet S2 */
    switch (frame->fun[1])
    {
    case KNX_BASIC_FUNCTION_DATE:
        item_desc = KNX_DESC("Date (K->P)"); /* 日期：年(2byte BE) + 月(1) + 日(1) */
        if (frame->data_len >= 4U)
        {
            KNX_DateData_t *date_out;
            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            knx_setting_frame_reset(&setting_frame);
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_DATE;
            date_out = (KNX_DateData_t *)setting_frame.data;
            date_out->year  = knx_read_be_u16(frame->data);
            date_out->month = frame->data[2];
            date_out->day   = frame->data[3];
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;
            (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
        }
        break;

    case KNX_BASIC_FUNCTION_TIME:
        item_desc = KNX_DESC("Time (K->P)"); /* 时间：时(1) + 分(1) + 秒(1) + 星期(1:1-7) */
        if (frame->data_len >= 4U)
        {
            KNX_TimeData_t *time_out;
            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            knx_setting_frame_reset(&setting_frame);
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_TIME;
            time_out = (KNX_TimeData_t *)setting_frame.data;
            time_out->hour    = frame->data[0];
            time_out->minute  = frame->data[1];
            time_out->second  = frame->data[2];
            time_out->weekday = frame->data[3];
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;
            (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
        }
        break;

    case KNX_BASIC_FUNCTION_VERSION:
        item_desc = KNX_DESC("Version Info (K<->P)"); /* 版本：xx(H).xx(L) */
        if (frame->data_len >= 2U)
        {
            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            knx_setting_frame_reset(&setting_frame);
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_VERSION;
            KNX_VersionData_t *version_out = (KNX_VersionData_t *)setting_frame.data;
            version_out->major = frame->data[0];
            version_out->minor = frame->data[1];
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;
            (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
        }
        break;

    case KNX_BASIC_FUNCTION_UPDATE_CONFIG:
        item_desc = KNX_DESC("Update Config (K<->P)"); /* 更新配置：1=请求更新, 0=结束更新 */
        break;

    case KNX_BASIC_FUNCTION_CONFIG_SYNC_STATUS:
        item_desc = KNX_DESC("Config Sync Status (K->P)"); /* 配置状态：1=开始发送, 0=结束发送 */
        if (frame->data_len >= 1U)
        {
            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            knx_setting_frame_reset(&setting_frame);
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_CONFIG_STATUS;
            setting_frame.value = frame->data[0];
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;
            (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
        }
        break;

    case KNX_BASIC_FUNCTION_PANEL_UPDATE_STATUS:
        item_desc = KNX_DESC("Panel Update Status (K<->P)"); /* 面板升级状态：0=未更新, 1=更新中, 2=已更新 */
        if (frame->data_len >= 1U)
        {
            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            knx_setting_frame_reset(&setting_frame);
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_PANEL_UPDATE_STATUS;
            setting_frame.value = frame->data[0];
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;
            (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
        }
        break;

    case KNX_BASIC_FUNCTION_DEVICE_TYPE:
        item_desc = KNX_DESC("Device Type (K<->P)"); /* 设备类型：2byte 类型码 */
        // if (frame->data_len >= 2U)
        // {
        //     HOOCH_PROTOCOL_SettingFrame_t setting_frame;
        //     knx_setting_frame_reset(&setting_frame);
        //     setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_MODULE_TYPE;
        //     setting_frame.value = knx_read_be_u16(frame->data); 
        //     (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
        // } 
        break;

    case KNX_BASIC_FUNCTION_UNIQUE_SERIAL:
        item_desc = KNX_DESC("Unique Serial (K->P)"); /* 模组唯一序列号 */
        break;

    case KNX_BASIC_FUNCTION_SUPPORT_PARAMETER:
        item_desc = KNX_DESC("Support Parameter Table (P->K)"); /* 支持参数表：1=支持 */
        break;

    case KNX_BASIC_FUNCTION_PROGRAM_MODE:
        item_desc = KNX_DESC("Program Mode (K<->P)"); /* 编程模式：0=非编程, 1=编程 */
        if (frame->data_len >= 1U)
        {
            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            knx_setting_frame_reset(&setting_frame);
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_PROGRAMMING_MODE;
            setting_frame.value = frame->data[0];
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;
            (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
        }
        break;

    case KNX_BASIC_FUNCTION_DEVICE_REBOOT:
        item_desc = KNX_DESC("Device Reboot (P->K)"); /* 设备重启：1=重启 */
        {
        HOOCH_PROTOCOL_SettingFrame_t setting_frame;
        knx_setting_frame_reset(&setting_frame);
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_DEVICE_REBOOT;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;
            (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
        }
        break;
        }

    case KNX_BASIC_FUNCTION_RESTORE_FACTORY:
        item_desc = KNX_DESC("Factory Reset (P->K)"); /* 恢复出厂设置：1=恢复 */
        {
        HOOCH_PROTOCOL_SettingFrame_t setting_frame;
        knx_setting_frame_reset(&setting_frame);
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_FACTORY_RESET;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;
            (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
        }
        break;
        }

    case KNX_BASIC_FUNCTION_DEVICE_LOCK:
        item_desc = KNX_DESC("Device Lock (K->P)"); /* 设备锁/童锁：0=解锁, 1=上锁 */
        {
        HOOCH_PROTOCOL_SettingFrame_t setting_frame;
        knx_setting_frame_reset(&setting_frame);
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_DEVICE_LOCK;
        if (frame->data_len >= 1U)
        {
            setting_frame.value = frame->data[0];
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;
            (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
        }
        break;
        }

    case KNX_BASIC_FUNCTION_WORLD_TIME:
        item_desc = KNX_DESC("World Time (K<->P)"); /* 世界时间：8 byte */
        {
        /* 取前4字节 UTC 时间戳，走 TIME_CALIBRATION 设置项 */
        if (frame->data_len >= 4U)
        {
            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            knx_setting_frame_reset(&setting_frame);
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_TIME_CALIBRATION;
            setting_frame.value = knx_read_be_u32(frame->data);
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;
            (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
        }
        break;
        }

    case KNX_BASIC_FUNCTION_TEMPERATURE_SENSOR:
        item_desc = KNX_DESC("Temperature Sensor (P->K)"); /* 温度传感器 */
        break;

    case KNX_BASIC_FUNCTION_HUMIDITY_SENSOR:
        item_desc = KNX_DESC("Humidity Sensor (P->K)"); /* 湿度传感器 */
        break;

    case KNX_BASIC_FUNCTION_HUMAN_PRESENCE_SENSOR:
        item_desc = KNX_DESC("Human Presence Sensor (P->K)"); /* 人存传感器 */
        break;

    case KNX_BASIC_FUNCTION_BACKLIGHT_SCREEN_CONTROL:
        item_desc = KNX_DESC("Backlight/Screen Control (K->P)"); /* 背光/屏幕控制：0=关背光/息屏, 1=亮背光/亮屏 */
        if (frame->data_len >= 1U)
        {
            /* 协议 18 的 0/1 与"进入/退出"相反：0=息屏(进入), 1=亮屏(退出) */
            knx_dispatch_screen_sleep_action((frame->data[0] == 0U)
                                                 ? HOOCH_PROTOCOL_SETTING_SCREEN_SLEEP_ACTION_ENTER
                                                 : HOOCH_PROTOCOL_SETTING_SCREEN_SLEEP_ACTION_EXIT);
        }
        break;

    case KNX_BASIC_FUNCTION_SCREENSAVER_CONTROL:
        item_desc = KNX_DESC("Screensaver Control (K->P)"); /* 屏保控制：0=退出屏保, 1=进入屏保 */
        if (frame->data_len >= 1U)
        {
            /* 协议 19 的 0/1 与"退出/进入"一致：直通 */
            knx_dispatch_screen_sleep_action((frame->data[0] != 0U)
                                                 ? HOOCH_PROTOCOL_SETTING_SCREEN_SLEEP_ACTION_ENTER
                                                 : HOOCH_PROTOCOL_SETTING_SCREEN_SLEEP_ACTION_EXIT);
        }
        break;

    case KNX_BASIC_FUNCTION_DATABASE_UNIQUE_CODE:
        item_desc = KNX_DESC("Database Config Unique Code (K->P)"); /* 数据库配置唯一码：0-65535, 用于判断配置有无变化 */
        if (frame->data_len >= 2U)
        {
            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            knx_setting_frame_reset(&setting_frame);
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_DATABASE_UNIQUE_CODE;
            setting_frame.value = knx_read_be_u16(frame->data);
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;
            (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
        }
        break;

    default:
        break;
    }

    KNX_SUMMARY_EMIT_ITEM(item_desc, frame);
}
