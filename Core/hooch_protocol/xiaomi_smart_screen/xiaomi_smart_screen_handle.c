


#include "xiaomi_smart_screen_circular_bufferc.h"
#include "xiaomi_smart_screen_handle.h"
#include "hooch_protocol.h"
#include <string.h>
static void xiaomi_smart_screen_handle_custom_config(Frame_t *frame);
static void xiaomi_smart_screen_handle_custom_config_3_free_command(Frame_t *frame);
static void xiaomi_smart_screen_setting_frame_reset(HOOCH_PROTOCOL_SettingFrame_t *setting_frame)
{
    if (setting_frame == NULL)
    {
        return;
    }

    setting_frame->page = HOOCH_PROTOCOL_SETTING_PAGE_INVALID;
    setting_frame->sequence = 0U;
    setting_frame->valid = 0U;
    setting_frame->screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_INVALID;
    setting_frame->param1 = 0xFFU;
    setting_frame->param2 = 0xFFU;
    setting_frame->param3 = 0xFFU;
    setting_frame->param4 = 0xFFU;
    setting_frame->item = HOOCH_PROTOCOL_SETTING_ITEM_INVALID;
    setting_frame->value = 0U;
}

void xiaomi_smart_screen_handle_frame(Frame_t *frame)
{
    XIAOMI_SMART_SCREEN_LOG_INFO("[HANDLE] CMD=0x%02X, VER=%d, LEN=%d\r\n",
                                 frame->command, frame->version, frame->data_len);
    
    /* 在这里添加你的帧处理逻辑 */
    switch (frame->command)
    {
        case XIAOMI_SMART_SCREEN_NET_STATUS_CONTROL:
            XIAOMI_SMART_SCREEN_LOG_INFO("[HANDLE] Command 0x%02X received: XIAOMI_SMART_SCREEN_NET_STATUS_CONTROL\r\n", frame->command);
            xiaomi_smart_screen_reply_no_param_frame(frame->command);
            break;
        case XIAOMI_SMART_SCREEN_RESET_CONTROL:
            XIAOMI_SMART_SCREEN_LOG_INFO("[HANDLE] Command 0x%02X received: XIAOMI_SMART_SCREEN_RESET_CONTROL\r\n", frame->command);
            break;
        case XIAOMI_SMART_SCREEN_SET_PRODUCTION_CONTROL:
            XIAOMI_SMART_SCREEN_LOG_INFO("[HANDLE] Command 0x%02X received: XIAOMI_SMART_SCREEN_SET_PRODUCTION_CONTROL\r\n", frame->command);
            break;
        case XIAOMI_SMART_SCREEN_PRODUCTION_STATUS_CONTROL:
            XIAOMI_SMART_SCREEN_LOG_INFO("[HANDLE] Command 0x%02X received: XIAOMI_SMART_SCREEN_PRODUCTION_STATUS_CONTROL\r\n", frame->command);
            break;
        case XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL:
            xiaomi_smart_screen_handle_custom_config(frame);
            break;
        default:
            XIAOMI_SMART_SCREEN_LOG_WARN("[HANDLE] Unknown command: 0x%02X\r\n", frame->command);
            break;
    }
}


/*回复模组消息无参数*/
void xiaomi_smart_screen_reply_no_param_frame(unsigned char command)
{
    Frame_t frame;
    
    /* 构建回复帧 */
    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();
    frame.command = command;
    frame.data_len = 0;  /* 无数据 */
    
    /* 发送帧（检验和会自动计算） */
    if (xiaoni_smart_screen_uart_sendframe(&frame) == 0) {
        XIAOMI_SMART_SCREEN_LOG_DEBUG("[REPLY] Frame sent: CMD=0x%02X\r\n", command);
    } else {
        XIAOMI_SMART_SCREEN_LOG_ERROR("[REPLY] Frame send failed\r\n");
    }
}

/*将收到的消息完整回复模组消息*/
void xiaomi_smart_screen_report_frame(Frame_t *frame)
{
    Frame_t reply_frame;
    /* 构建回复帧 */
    reply_frame.header = FRAME_HEADER;
    reply_frame.version = xiaoni_smart_screen_get_version();
    reply_frame.command = frame->command;
    reply_frame.data_len = frame->data_len;
    memcpy(reply_frame.data, frame->data, frame->data_len);
    
    /* 发送帧（检验和会自动计算） */
    if (xiaoni_smart_screen_uart_sendframe(&reply_frame) == 0) {
        XIAOMI_SMART_SCREEN_LOG_DEBUG("[REPLY] Frame sent: CMD=0x%02X, VER=%d, LEN=%d\r\n",
                                      reply_frame.command, reply_frame.version, reply_frame.data_len);
    } else {
        XIAOMI_SMART_SCREEN_LOG_ERROR("[REPLY] Frame send failed\r\n");
    }
}
/*将收到的消息完整回复模组消息*/
void xiaomi_smart_screen_report_frame_v2(Frame_t *frame)
{
    Frame_t reply_frame;
    /* 构建回复帧 */
    reply_frame.header = FRAME_HEADER;
    reply_frame.version = xiaoni_smart_screen_get_version();
    reply_frame.command = frame->command;
    reply_frame.data_len = 3U;
    reply_frame.data[0] = frame->data[0];
    reply_frame.data[1] = frame->data[1];
    reply_frame.data[2] = frame->data[2];
    
    /* 发送帧（检验和会自动计算） */
    if (xiaoni_smart_screen_uart_sendframe(&reply_frame) == 0) {
        XIAOMI_SMART_SCREEN_LOG_DEBUG("[REPLY] Frame sent: CMD=0x%02X, VER=%d, LEN=%d\r\n",
                                      reply_frame.command, reply_frame.version, reply_frame.data_len);
    } else {
        XIAOMI_SMART_SCREEN_LOG_ERROR("[REPLY] Frame send failed\r\n");
    }
}

void xiaomi_smart_screen_reply_frame(Frame_t *frame)
{
    xiaomi_smart_screen_report_frame_v2(frame);
}

















/*处理模组设备自定义配置*/
static void xiaomi_smart_screen_handle_custom_config(Frame_t *frame)
{
    uint8_t subcommand;

    if (frame == NULL)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[CUSTOM] frame is NULL\r\n");
        return;
    }

    if (frame->data_len < 1)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[CUSTOM] invalid data len: %d\r\n", frame->data_len);
        return;
    }

    subcommand = frame->data[0];

    switch (subcommand)
    {
        case XIAOMI_SMART_SCREEN_SUBCMD_READ:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] READ request received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_SET:
                XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] SET request received\r\n");
            xiaomi_smart_screen_handle_custom_config_2(frame);

            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_REPORT:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] REPORT received\r\n");
            break;

        default:
            XIAOMI_SMART_SCREEN_LOG_WARN("[CUSTOM] Unknown subcommand: 0x%02X\r\n", subcommand);
            break;
    }
}

/*处理模组设备自定义配置参数2*/
static void xiaomi_smart_screen_handle_custom_config_2(Frame_t *frame)
{
    uint8_t subcommand;
    subcommand = frame->data[1];
        switch (subcommand)
    {
        case XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT:
                XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] DEFAULT request received\r\n");
            xiaomi_smart_screen_handle_custom_config_3(frame);

            break;

        default:
            XIAOMI_SMART_SCREEN_LOG_WARN("[CUSTOM] Unknown subcommand: 0x%02X\r\n", subcommand);
            break;
    }
}



/*处理模组设备自定义配置参数3*/
static void xiaomi_smart_screen_handle_custom_config_3(Frame_t *frame)
{
    uint8_t subcommand;

    if (frame == NULL)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[CUSTOM] frame is NULL\r\n");
        return;
    }

    if (frame->data_len < 3)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[CUSTOM] invalid data len for param3: %d\r\n", frame->data_len);
        return;
    }

    subcommand = frame->data[2];
    switch (subcommand)
    {
        case XIAOMI_SMART_SCREEN_SUBCMD_SWITCH_CONTROL:
        {
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] SWITCH_CONTROL received, bits=0x%02X\r\n",
                                         (unsigned int)frame->data[3]);
            /* 一次性全量更新4路开关内部状态（模组下发，仅消费不回报） */
            xiaoni_smart_screen_switch_control_update(frame->data[3]);

            /* 逐键分发，通知其他协议层（BLE/KNX等）开关状态变化 */
            HOOCH_PROTOCOL_KeyStatusFrame_t key_status_frame;
            key_status_frame.sequence = 0U;
            key_status_frame.valid = 0U;
            key_status_frame.control_item = HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_STATE;
            key_status_frame.value = 0U;
            key_status_frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
            key_status_frame.key = HOOCH_PROTOCOL_KEY_STATUS_KEY_1;
            key_status_frame.state = ((frame->data[3] & 0x01U) != 0U)
                ? HOOCH_PROTOCOL_KEY_STATUS_STATE_ON
                : HOOCH_PROTOCOL_KEY_STATUS_STATE_OFF;
            HOOCH_PROTOCOL_KeyStatus_DispatchFrame(&key_status_frame);
            key_status_frame.key = HOOCH_PROTOCOL_KEY_STATUS_KEY_2;
            key_status_frame.state = ((frame->data[3] & 0x02U) != 0U)
                ? HOOCH_PROTOCOL_KEY_STATUS_STATE_ON
                : HOOCH_PROTOCOL_KEY_STATUS_STATE_OFF;
            HOOCH_PROTOCOL_KeyStatus_DispatchFrame(&key_status_frame);
            key_status_frame.key = HOOCH_PROTOCOL_KEY_STATUS_KEY_3;
            key_status_frame.state = ((frame->data[3] & 0x04U) != 0U)
                ? HOOCH_PROTOCOL_KEY_STATUS_STATE_ON
                : HOOCH_PROTOCOL_KEY_STATUS_STATE_OFF;
            HOOCH_PROTOCOL_KeyStatus_DispatchFrame(&key_status_frame);
            key_status_frame.key = HOOCH_PROTOCOL_KEY_STATUS_KEY_4;
            key_status_frame.state = ((frame->data[3] & 0x08U) != 0U)
                ? HOOCH_PROTOCOL_KEY_STATUS_STATE_ON
                : HOOCH_PROTOCOL_KEY_STATUS_STATE_OFF;
            HOOCH_PROTOCOL_KeyStatus_DispatchFrame(&key_status_frame);

            break;
        }

        case XIAOMI_SMART_SCREEN_SUBCMD_EVENT_STATUS:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] EVENT_STATUS received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_RELAY_KEY_FUNCTION_CONFIG:
        {
            uint8_t channel;
            uint8_t power_on_status;
            uint8_t key_type;

            if (frame->data_len < 6U)
            {
                XIAOMI_SMART_SCREEN_LOG_WARN("[CUSTOM] RELAY_KEY_FUNCTION_CONFIG invalid data len: %u\r\n",
                                             (unsigned int)frame->data_len);
                break;
            }

            channel = frame->data[3];
            power_on_status = frame->data[4];
            key_type = frame->data[5];

            if (channel == 0U)
            {
                uint8_t key;
                for (key = 1U; key <= HOOCH_PROTOCOL_KEY_MODE_KEY_COUNT; key++)
                {
                    (void)HOOCH_PROTOCOL_KeyMode_Send(
                        (HOOCH_PROTOCOL_KeyModeKey_t)key,
                        (HOOCH_PROTOCOL_KeyModePowerOnStatus_t)power_on_status,
                        (HOOCH_PROTOCOL_KeyModeType_t)key_type);
                }
            }
            else
            {
                (void)HOOCH_PROTOCOL_KeyMode_Send(
                    (HOOCH_PROTOCOL_KeyModeKey_t)channel,
                    (HOOCH_PROTOCOL_KeyModePowerOnStatus_t)power_on_status,
                    (HOOCH_PROTOCOL_KeyModeType_t)key_type);
            }

            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] RELAY_KEY_FUNCTION_CONFIG received\r\n");
            break;
        }

        case XIAOMI_SMART_SCREEN_SUBCMD_KEY_NAME_INFO:
        {
            HOOCH_PROTOCOL_KeyNameFrame_t key_name_frame;
            uint16_t name_len;
            uint16_t copy_len;
            /*当前页面按键号*/
            key_name_frame.key = (HOOCH_PROTOCOL_KeyNameKey_t)frame->data[3];
            /*当前页面图标索引*/
            key_name_frame.icon_index = (HOOCH_PROTOCOL_KeyNameIconIndex_t)frame->data[4];
            /*当前所在页面*/
            key_name_frame.page = HOOCH_PROTOCOL_KEY_NAME_PAGE_SWITCH;
            /*当前文字可见范围*/
            key_name_frame.delivery = (key_name_frame.key == HOOCH_PROTOCOL_KEY_NAME_KEY_INVALID)
                ? HOOCH_PROTOCOL_KEY_NAME_DELIVERY_OFFSCREEN_TEXT
                : HOOCH_PROTOCOL_KEY_NAME_DELIVERY_KEY;
            
            if (frame->data_len < 5U) {
                XIAOMI_SMART_SCREEN_LOG_WARN("[CUSTOM] KEY_NAME_INFO invalid data len: %u\r\n", (unsigned int)frame->data_len);
                break;
            }

            name_len = (uint16_t)(frame->data_len - 5U);
            copy_len = name_len;
            if (copy_len > (HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH - 1U)) {
                copy_len = (uint16_t)(HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH - 1U);
            }

            if (copy_len > 0U) {
                memcpy(key_name_frame.name, &frame->data[5], copy_len);
            }
            key_name_frame.name[copy_len] = '\0';

            if (strcmp(key_name_frame.name, "undefined") == 0) {
                break;
            }

            key_name_frame.sequence = 0U;
            key_name_frame.valid = 0U;
            HOOCH_PROTOCOL_KeyName_SetFrame(&key_name_frame);
            break;
        }
        case XIAOMI_SMART_SCREEN_SUBCMD_ENTER_PAIRING_CLEAR_INTERLOCK:
        {
            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            xiaomi_smart_screen_setting_frame_reset(&setting_frame);
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
            setting_frame.param1 = (frame->data_len >= 4U) ? frame->data[3] : 0xFFU;
            setting_frame.param2 = (frame->data_len >= 5U) ? frame->data[4] : 0xFFU;
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_ENTER_PAIRING_CLEAR_INTERLOCK;
            setting_frame.value = (uint32_t)setting_frame.param1;
            HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] ENTER_PAIRING_CLEAR_INTERLOCK received\r\n");
            break;
        }

        case XIAOMI_SMART_SCREEN_SUBCMD_PAIRING_CLEAR_INTERLOCK_STATUS:
        {
            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            xiaomi_smart_screen_setting_frame_reset(&setting_frame);
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
            setting_frame.param1 = (HOOCH_PROTOCOL_SettingPairingClearInterlockOp_t)(frame->data_len >= 4U ? frame->data[4] : 0xFFU);
            setting_frame.param2 = (frame->data_len >= 5U) ? frame->data[5] : 0xFFU; 
            /*对码、清码类型 0：失败
                           1：成功
                 互控类型：  0：解控状态
                            1：互控状态
            */
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_PAIRING_CLEAR_INTERLOCK_STATUS;
            setting_frame.value = frame->data[3];
            HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] PAIRING_CLEAR_INTERLOCK_STATUS received\r\n");
            break;
        }

        case XIAOMI_SMART_SCREEN_SUBCMD_MOMENTARY_FUNCTION:
        {
            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            xiaomi_smart_screen_setting_frame_reset(&setting_frame);
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_KEY_JOG_TIME;
            memcpy(&setting_frame.data[0], &frame->data[3], 5);/*一共五个字节*/
            /*这里解析使用xiaomi_screen_jog_ctrl_t 结构体*/
            HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] MOMENTARY_FUNCTION received\r\n");
        }
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_INTERLOCK_FUNCTION:
        {
            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            xiaomi_smart_screen_setting_frame_reset(&setting_frame);
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_INTERLOCK_FUNCTION;
            setting_frame.value = frame->data[3];
            HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] INTERLOCK_FUNCTION received\r\n");
        }
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_FUNCTION_CONFIG:
        {
            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            xiaomi_smart_screen_setting_frame_reset(&setting_frame);
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_BACKLIGHT_SWITCH;
            setting_frame.value = frame->data[3];
            HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_KEY_STATUS_LIGHT;
            setting_frame.value = frame->data[4];
            HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_LOCK_PARAMETER;
            setting_frame.value = frame->data[5];
            HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_TYPE_IDENTIFICATION;
            setting_frame.value = frame->data[6];
            HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_SWITCH;
            setting_frame.value = frame->data[7];
            HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] FUNCTION_CONFIG received\r\n");
            break;
        }
        case XIAOMI_SMART_SCREEN_SUBCMD_SENSOR_STATUS_REPORT:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] SENSOR_STATUS_REPORT received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_BACKLIGHT_BRIGHTNESS_CONTROL:
        {
            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            xiaomi_smart_screen_setting_frame_reset(&setting_frame);
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_BACKLIGHT_BRIGHTNESS;
            setting_frame.value = frame->data[3];
            HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] BACKLIGHT_BRIGHTNESS_CONTROL received\r\n");
            break;
        }
        case XIAOMI_SMART_SCREEN_SUBCMD_SWITCH_CONTROL_EXT:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] SWITCH_CONTROL_EXT received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_CLEAR_ALL_REMOTE_CONTROL_FUNCTIONS:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] CLEAR_ALL_REMOTE_CONTROL_FUNCTIONS received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_DIMMING_COLOR_CURTAIN_SWITCH_STATUS:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] DIMMING_COLOR_CURTAIN_SWITCH_STATUS received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_BRIGHTNESS_COLOR_TEMP_TRAVEL_STATUS:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] BRIGHTNESS_COLOR_TEMP_TRAVEL_STATUS received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_UTC_TIME_CONTROL:
{

            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            xiaomi_smart_screen_setting_frame_reset(&setting_frame);
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_TIME_CALIBRATION;
            setting_frame.value = ((uint32_t)frame->data[3] << 24)
                                | ((uint32_t)frame->data[4] << 16)
                                | ((uint32_t)frame->data[5] << 8)
                                | (uint32_t)frame->data[6];
            HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] UTC_TIME_CONTROL received\r\n");
            break;
}
        case XIAOMI_SMART_SCREEN_SUBCMD_TIME_ZONE_CONFIG:

            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] TIME_ZONE_CONFIG received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_WEATHER_CODE:
        {
                HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            xiaomi_smart_screen_setting_frame_reset(&setting_frame);
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_WEATHER;
                  setting_frame.value = ((uint32_t)frame->data[3] << 8)
                                | ((uint32_t)frame->data[4]);
            HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] WEATHER_CODE received\r\n");
        }
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_SCREEN_BRIGHTNESS_CONTROL:
{
            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            xiaomi_smart_screen_setting_frame_reset(&setting_frame);
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_BRIGHTNESS;
            setting_frame.value = frame->data[3];
            HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] SCREEN_BRIGHTNESS_CONTROL received\r\n");
            break;
}
        case XIAOMI_SMART_SCREEN_SUBCMD_SCREEN_OFF_MODE_CONTROL:
        {
            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            xiaomi_smart_screen_setting_frame_reset(&setting_frame);
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_OFF_EFFECT;
            setting_frame.value = frame->data[3];
            HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_OFF_TIME;
            if(frame->data[4] == 0U) /*永不熄灭*/
            {
                setting_frame.screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_NEVER;
            }
            else if(frame->data[4] == 1U) /*自动熄灭*/
            {
                setting_frame.screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_AUTO;
            }
            else
            {
                setting_frame.screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_VALUE;
            }
            setting_frame.param1 = frame->data[4];
            setting_frame.param2 = 0xFFU;
            setting_frame.param3 = 0xFFU;
            setting_frame.param4 = 0xFFU;
            setting_frame.value = frame->data[4];
            HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);



            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] SCREEN_OFF_MODE_CONTROL received\r\n");
            break;
        }

        case XIAOMI_SMART_SCREEN_SUBCMD_RELAY_CONFIG:
        {
            if (frame->data_len < 7U)
            {
                XIAOMI_SMART_SCREEN_LOG_WARN("[CUSTOM] RELAY_CONFIG invalid data len: %u\r\n", (unsigned int)frame->data_len);
                break;
            }

            /* 拆为 4 次回调: value=按键通道(1-based), param1=对应继电器 */
            unsigned char i;
            for (i = 0U; i < 4U; i++)
            {
                HOOCH_PROTOCOL_SettingFrame_t setting_frame;
                xiaomi_smart_screen_setting_frame_reset(&setting_frame);
                setting_frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
                setting_frame.item   = HOOCH_PROTOCOL_SETTING_ITEM_KEY_RELAY_MAPPING;
                setting_frame.value  = i + 1U;                /* 按键通道: 1~4 */
                setting_frame.param1 = frame->data[3U + i];   /* 对应继电器 */
                HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
            }

            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] RELAY_CONFIG received\r\n");
            break;
        }

        case XIAOMI_SMART_SCREEN_SUBCMD_MCU_VERSION_REPORT:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] MCU_VERSION_REPORT received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_DIMMER_DOUBLE_TAP_SWITCH_SETTING:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] DIMMER_DOUBLE_TAP_SWITCH_SETTING received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_SCREEN_SAVER:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] SCREEN_SAVER received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_AVAILABLE_PAGE:
        {
            /*
            Byte[0]: 默认页面 (0-7)
            0: 默认不锁定页面（记忆）
            1: 默认开关页面
            2: 默认情景页面
            3: 灯光
            4: 窗帘
            5: 空调
            6: 新风
            7: 地暖
            */
            static const HOOCH_PROTOCOL_SettingPage_t page_map[8] = {
                HOOCH_PROTOCOL_SETTING_PAGE_MEMORY,          /* 0: 记忆 */
                HOOCH_PROTOCOL_SETTING_PAGE_SWITCH,          /* 1: 开关 */
                HOOCH_PROTOCOL_SETTING_PAGE_SCENE,           /* 2: 情景 */
                HOOCH_PROTOCOL_SETTING_PAGE_LIGHT,           /* 3: 灯光 */
                HOOCH_PROTOCOL_SETTING_PAGE_CURTAIN,         /* 4: 窗帘 */
                HOOCH_PROTOCOL_SETTING_PAGE_AIR_CONDITIONER, /* 5: 空调 */
                HOOCH_PROTOCOL_SETTING_PAGE_FRESH_AIR,       /* 6: 新风 */
                HOOCH_PROTOCOL_SETTING_PAGE_FLOOR_HEATING,   /* 7: 地暖 */
            };
            uint8_t page_val;
            HOOCH_PROTOCOL_SettingFrame_t setting_frame;
            page_val = frame->data[3];
            xiaomi_smart_screen_setting_frame_reset(&setting_frame);
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;

            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_DEFAULT_MAIN_PAGE;
            setting_frame.page = (page_val < 8U) ? page_map[page_val] : HOOCH_PROTOCOL_SETTING_PAGE_INVALID;

            HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);

            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] AVAILABLE_PAGE received\r\n");
            break;
        }







        case XIAOMI_SMART_SCREEN_SUBCMD_TEXT_MESSAGE_DISPLAY_SWITCH:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] TEXT_MESSAGE_DISPLAY_SWITCH received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_SPEED_MODE_SWITCH:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] SPEED_MODE_SWITCH received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_SCENE_NAME_INFO:
        {
            HOOCH_PROTOCOL_KeyNameFrame_t key_name_frame;
            uint16_t name_len;
            uint16_t copy_len;
            /*当前页面按键号*/
            key_name_frame.key = (HOOCH_PROTOCOL_KeyNameKey_t)frame->data[3];
            /*当前页面图标索引*/
            key_name_frame.icon_index = (HOOCH_PROTOCOL_KeyNameIconIndex_t)frame->data[4];
            /*当前所在页面*/
            key_name_frame.page = HOOCH_PROTOCOL_KEY_NAME_PAGE_SCENE;
            /*当前文字可见范围*/
            key_name_frame.delivery = (key_name_frame.key == HOOCH_PROTOCOL_KEY_NAME_KEY_INVALID)
                ? HOOCH_PROTOCOL_KEY_NAME_DELIVERY_OFFSCREEN_TEXT
                : HOOCH_PROTOCOL_KEY_NAME_DELIVERY_KEY;
            
            if (frame->data_len < 5U) {
                XIAOMI_SMART_SCREEN_LOG_WARN("[CUSTOM] KEY_NAME_INFO invalid data len: %u\r\n", (unsigned int)frame->data_len);
                break;
            }

            name_len = (uint16_t)(frame->data_len - 5U);
            copy_len = name_len;
            if (copy_len > (HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH - 1U)) {
                copy_len = (uint16_t)(HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH - 1U);
            }

            if (copy_len > 0U) {
                memcpy(key_name_frame.name, &frame->data[5], copy_len);
            }
            key_name_frame.name[copy_len] = '\0';

            if (strcmp(key_name_frame.name, "undefined") == 0) {
                break;
            }

            key_name_frame.sequence = 0U;
            key_name_frame.valid = 0U;
            HOOCH_PROTOCOL_KeyName_SetFrame(&key_name_frame);
            break;
        }

        case XIAOMI_SMART_SCREEN_SUBCMD_LIGHT_NAME_INFO:
        {
            HOOCH_PROTOCOL_KeyNameFrame_t key_name_frame;
            uint16_t name_len;
            uint16_t copy_len;
            key_name_frame.key = (HOOCH_PROTOCOL_KeyNameKey_t)frame->data[3];
            key_name_frame.icon_index = (HOOCH_PROTOCOL_KeyNameIconIndex_t)frame->data[4];
            key_name_frame.page = HOOCH_PROTOCOL_KEY_NAME_PAGE_LIGHT;
            key_name_frame.delivery = (key_name_frame.key == HOOCH_PROTOCOL_KEY_NAME_KEY_INVALID)
                ? HOOCH_PROTOCOL_KEY_NAME_DELIVERY_OFFSCREEN_TEXT
                : HOOCH_PROTOCOL_KEY_NAME_DELIVERY_KEY;

            if (frame->data_len < 5U) {
                XIAOMI_SMART_SCREEN_LOG_WARN("[CUSTOM] KEY_NAME_INFO invalid data len: %u\r\n", (unsigned int)frame->data_len);
                break;
            }

            name_len = (uint16_t)(frame->data_len - 5U);
            copy_len = name_len;
            if (copy_len > (HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH - 1U)) {
                copy_len = (uint16_t)(HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH - 1U);
            }

            if (copy_len > 0U) {
                memcpy(key_name_frame.name, &frame->data[5], copy_len);
            }
            key_name_frame.name[copy_len] = '\0';

            if (strcmp(key_name_frame.name, "undefined") == 0) {
                break;
            }

            key_name_frame.sequence = 0U;
            key_name_frame.valid = 0U;
            HOOCH_PROTOCOL_KeyName_SetFrame(&key_name_frame);
            break;
        }

        case XIAOMI_SMART_SCREEN_SUBCMD_CURTAIN_NAME_INFO:
        {
            HOOCH_PROTOCOL_KeyNameFrame_t key_name_frame;
            uint16_t name_len;
            uint16_t copy_len;
            key_name_frame.key = (HOOCH_PROTOCOL_KeyNameKey_t)frame->data[3];
            key_name_frame.icon_index = (HOOCH_PROTOCOL_KeyNameIconIndex_t)frame->data[4];
            key_name_frame.page = HOOCH_PROTOCOL_KEY_NAME_PAGE_CURTAIN;
            key_name_frame.delivery = (key_name_frame.key == HOOCH_PROTOCOL_KEY_NAME_KEY_INVALID)
                ? HOOCH_PROTOCOL_KEY_NAME_DELIVERY_OFFSCREEN_TEXT
                : HOOCH_PROTOCOL_KEY_NAME_DELIVERY_KEY;

            if (frame->data_len < 5U) {
                XIAOMI_SMART_SCREEN_LOG_WARN("[CUSTOM] KEY_NAME_INFO invalid data len: %u\r\n", (unsigned int)frame->data_len);
                break;
            }

            name_len = (uint16_t)(frame->data_len - 5U);
            copy_len = name_len;
            if (copy_len > (HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH - 1U)) {
                copy_len = (uint16_t)(HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH - 1U);
            }

            if (copy_len > 0U) {
                memcpy(key_name_frame.name, &frame->data[5], copy_len);
            }
            key_name_frame.name[copy_len] = '\0';

            if (strcmp(key_name_frame.name, "undefined") == 0) {
                break;
            }

            key_name_frame.sequence = 0U;
            key_name_frame.valid = 0U;
            HOOCH_PROTOCOL_KeyName_SetFrame(&key_name_frame);
            break;
        }

        case XIAOMI_SMART_SCREEN_SUBCMD_AC_NAME_INFO:
        {
            HOOCH_PROTOCOL_AirConditionerFrame_t air_conditioner_frame;
            uint16_t name_len;
            uint16_t copy_len;

            if (frame->data_len < 5U)
            {
                XIAOMI_SMART_SCREEN_LOG_WARN("[CUSTOM] AC_NAME_INFO invalid data len: %u\r\n", (unsigned int)frame->data_len);
                break;
            }

            HOOCH_PROTOCOL_AirConditioner_ClearFrame(&air_conditioner_frame);
            air_conditioner_frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
            air_conditioner_frame.channel = frame->data[3];
            air_conditioner_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_DEVICE_DESC;

            name_len = (uint16_t)(frame->data_len - 5U);
            copy_len = name_len;
            if (copy_len > (sizeof(air_conditioner_frame.device_desc) - 1U))
            {
                copy_len = (uint16_t)(sizeof(air_conditioner_frame.device_desc) - 1U);
            }
 
            if (copy_len > 0U)
            {
                memcpy(air_conditioner_frame.device_desc, &frame->data[5], copy_len);
            }
            air_conditioner_frame.device_desc[copy_len] = '\0';

            (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&air_conditioner_frame);

            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] AC_NAME_INFO received\r\n");
            break;
        }

        case XIAOMI_SMART_SCREEN_SUBCMD_FRESH_AIR_NAME_INFO:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] FRESH_AIR_NAME_INFO received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_FLOOR_HEATING_NAME_INFO:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] FLOOR_HEATING_NAME_INFO received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_AC_STATUS:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] AC_STATUS received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_FRESH_AIR_STATUS:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] FRESH_AIR_STATUS received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_FLOOR_HEATING_STATUS:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] FLOOR_HEATING_STATUS received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_CURRENT_PAGE_READ_SETTING:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] CURRENT_PAGE_READ_SETTING received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_PAGE_DISPLAY_VISIBILITY:
        {
   
            uint16_t page_bitmap;
            uint8_t page_index;
            HOOCH_PROTOCOL_SettingPage_t page_map[8] =
            {
                HOOCH_PROTOCOL_SETTING_PAGE_INVALID, /* bit0: 无效页面 */
                HOOCH_PROTOCOL_SETTING_PAGE_SWITCH,          /* bit1: 开关页面 */
                HOOCH_PROTOCOL_SETTING_PAGE_SCENE,           /* bit2: 场景页面 */
                HOOCH_PROTOCOL_SETTING_PAGE_LIGHT,           /* bit3: 灯光页面 */
                HOOCH_PROTOCOL_SETTING_PAGE_CURTAIN,         /* bit4: 窗帘页面 */
                HOOCH_PROTOCOL_SETTING_PAGE_AIR_CONDITIONER, /* bit5: 空调页面 */
                HOOCH_PROTOCOL_SETTING_PAGE_INVALID, /* bit6: 无效页面 */
                HOOCH_PROTOCOL_SETTING_PAGE_INVALID, /* bit7: 无效页面 */
            };

            if (frame->data_len < 5U) {
                XIAOMI_SMART_SCREEN_LOG_WARN("[CUSTOM] PAGE_DISPLAY_VISIBILITY invalid data len: %u\r\n", (unsigned int)frame->data_len);
                break;
            }

            page_bitmap = ((uint16_t)frame->data[3] << 8) | frame->data[4];

            for (page_index = 0U; page_index < 8U; page_index++)
            {
                HOOCH_PROTOCOL_SettingFrame_t setting_frame;
                uint16_t page_mask = (uint16_t)1U << page_index;
                xiaomi_smart_screen_setting_frame_reset(&setting_frame);
                setting_frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;

                setting_frame.item = ((page_bitmap & page_mask) != 0U)
                    ? HOOCH_PROTOCOL_SETTING_ITEM_ADD_PAGE
                    : HOOCH_PROTOCOL_SETTING_ITEM_DELETE_PAGE;
                setting_frame.value = (uint16_t)((page_bitmap & page_mask) != 0U);
                setting_frame.page = page_map[page_index];

                HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
            }
        }
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] PAGE_DISPLAY_VISIBILITY received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_KEY_EVENT_STATUS:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] KEY_EVENT_STATUS received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_MULTI_FUNCTION_STATUS_CONFIG:
        {
             HOOCH_PROTOCOL_SettingFrame_t setting_frame;
        if(frame->data[3] == 0xFF)  /*任何页面*/
        {
        if(frame->data[4] == 0x01)   /*通道*/
        {
           if(frame->data[5] == 0x06)  /*人体传感器灵敏度*/
           {
            xiaomi_smart_screen_setting_frame_reset(&setting_frame);
            setting_frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_SENSITIVITY;
            setting_frame.value = frame->data[6];
            HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
           }
        }
        }
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] MULTI_FUNCTION_STATUS_CONFIG received\r\n");
    }
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_TEXT_MESSAGE_DELIVERY:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] TEXT_MESSAGE_DELIVERY received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_BRIGHTNESS_COLOR_TEMP_TRAVEL_STATUS_EXT:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] BRIGHTNESS_COLOR_TEMP_TRAVEL_STATUS_EXT received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_PAIRING_CLEAR_STATUS:
        {
            /*
             * 对码/清码协议帧 (从 data[3] 开始的 3 字节 payload):
             *   data[3] = page   页面 (0x00=开关, 0x02=灯光, 0x03=窗帘, 0x04=空调)
             *   data[4] = item   页面对应项 (高4bit=按键索引, 低4bit=模式/子项)
             *   data[5] = action 操作 (0=清码, 1=对码)
             */
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] PAIRING_CLEAR_STATUS received\r\n");

            if (frame->data_len >= 6U)
            {
                uint8_t page = frame->data[3];
                uint8_t item = frame->data[4];
                HOOCH_PROTOCOL_CodeMatchReportAction_t action =
                    (frame->data[5] == 0U) ? HOOCH_PROTOCOL_CODE_MATCH_REPORT_ACTION_CLEAR
                                           : HOOCH_PROTOCOL_CODE_MATCH_REPORT_ACTION_MATCH;

                XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] Pairing: page=0x%02X, item=0x%02X, action=%s\r\n",
                    (unsigned int)page, (unsigned int)item,
                    (action == HOOCH_PROTOCOL_CODE_MATCH_REPORT_ACTION_MATCH) ? "MATCH" : "CLEAR");

                HOOCH_PROTOCOL_CodeMatchDispatch_Dispatch(page, item, action);
            }
            else
            {
                XIAOMI_SMART_SCREEN_LOG_WARN("[CUSTOM] PAIRING_CLEAR_STATUS invalid data len: %u\r\n",
                    (unsigned int)frame->data_len);
            }
            break;
        }

        case XIAOMI_SMART_SCREEN_SUBCMD_INDICATOR_COLOR_CONFIG:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] INDICATOR_COLOR_CONFIG received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_BACKLIGHT_COLOR_CONFIG:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] BACKLIGHT_COLOR_CONFIG received\r\n");
            break;

        case XIAOMI_SMART_SCREEN_SUBCMD_RADAR_SENSITIVITY_CONFIG:
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] RADAR_SENSITIVITY_CONFIG received\r\n");
            break;
            
        case XIAOMI_SMART_SCREEN_SUBCMD_FREE_DEFINE:
        xiaomi_smart_screen_handle_custom_config_3_free_command(frame);
            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] FREE_DEFINE received\r\n");
            break;
        default:
            XIAOMI_SMART_SCREEN_LOG_WARN("[CUSTOM] Unknown subcommand: 0x%02X\r\n", subcommand);
            break;
    }
                xiaomi_smart_screen_reply_frame(frame);
}


/* 将协议中的风速值映射为 HOOCH 协议枚举。
   协议：1=自动, 2=低速, 3=中速, 4=高速
   枚举：LOW=1, MEDIUM=2, HIGH=3, AUTO=4 */
static HOOCH_PROTOCOL_AirConditionerFanSpeed_t xiaomi_smart_screen_map_fan_speed(uint8_t raw)
{
    switch (raw)
    {
        case 1U: return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_AUTO;
        case 2U: return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_LOW;
        case 3U: return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_MEDIUM;
        case 4U: return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_HIGH;
        default: return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_INVALID;
    }
}

/*处理模组设备自定义配置参数3 自由定义*/
static void xiaomi_smart_screen_handle_custom_config_3_free_command(Frame_t *frame)
{
 uint8_t subcommand;  
subcommand = frame->data[3];
    switch (subcommand)
    {
        case XIAOMI_SMART_SCREEN_FREE_DEFINE_AC_PARAM:
        {
            /* 协议格式（data[3]为子命令0x03，payload从data[4]开始）：
               data[4]: 空调电源(0=关, 1=开)
               data[5]: 空调温度(16-32)
               data[6]: bit4-7=模式(1:制冷 2:送风 3:除湿 4:制热),
                        bit0-3=风速(1:自动 2:低速 3:中速 4:高速) */
            uint8_t data6 = frame->data[6];
            HOOCH_PROTOCOL_AirConditionerMode_t mode =
                (HOOCH_PROTOCOL_AirConditionerMode_t)((data6 >> 4U) & 0x0FU);
            HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed =
                xiaomi_smart_screen_map_fan_speed(data6 & 0x0FU);
            HOOCH_PROTOCOL_AirConditionerFrame_t air_conditioner_frame;

            HOOCH_PROTOCOL_AirConditioner_ClearFrame(&air_conditioner_frame);
            air_conditioner_frame.source       = HOOCH_PROTOCOL_SOURCE_XIAOMI;
            air_conditioner_frame.channel      = 1U;    /* 单空调默认通道 1 */
            air_conditioner_frame.power        = (HOOCH_PROTOCOL_AirConditionerPower_t)frame->data[4];   /* 电源 */
            air_conditioner_frame.mode         = mode;                                                   /* 模式 */
            air_conditioner_frame.fan_speed    = fan_speed;                                              /* 风速 */
            air_conditioner_frame.temperature  = frame->data[5];                                         /* 温度 */
            air_conditioner_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL;
            (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&air_conditioner_frame);

            XIAOMI_SMART_SCREEN_LOG_INFO("[CUSTOM] FREE_DEFINE_AC_PARAM received\r\n");
            break;
        }
       default:
            break;
    }


    }





uint8_t xiaoni_smart_screen_send_report_status(xiaomi_screen_subcmd_t dp)
{
    Frame_t frame;

    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();
    frame.command = XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL;
    frame.data_len = 3U;

    frame.data[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
    frame.data[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
    frame.data[2] = (uint8_t)dp;
    return xiaoni_smart_screen_uart_sendframe(&frame);
}
