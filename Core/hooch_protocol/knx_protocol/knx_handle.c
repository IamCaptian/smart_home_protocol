/*
 * KNX Panel Communication Protocol (V2.4) - Frame Dispatch Layer
 *
 * This module is responsible for:
 * 1) Decoding the parsed KNX_Frame_t structure into human-readable meaning.
 * 2) Printing protocol semantics using unified "[KNX]" English logs.
 *
 * 中文说明：
 * - 本文件只做"协议含义解析 + 打印"，不在这里实现具体业务控制。
 * - 解析器(拼帧/校验)在 knx_circular_buffer.c，这里只接收解析完成的 KNX_Frame_t。
 * - fun[] 字段是协议的"路径"，不同 FUN1 下 fun[1]..fun[x] 的含义不同，详见下文注释。
 *
 * Parser architecture (see knx_circular_buffer.c):
 * - USART2 IRQ pushes bytes into a ring buffer.
 * - Main loop consumes bytes and runs a state machine to assemble frames.
 * - After a frame passes checksum verification, knx_handle_frame() is called.
 *
 * Frame types:
 * - A1: WRITE_DATA   -> fun_count + fun[] + data_len(1B) + data + checksum
 * - A2/A3: QUERY/REQUEST -> fun_count + fun[] + checksum
 * - C1/C2: CONFIG    -> address(2B) + data_len(2B) + data(optional) + checksum
 * - C3: RESPONSE -> resp_type(1B) + checksum
 *
 * Function path conventions used in this implementation:
 * - fun[0] (FUN1): category (BASIC_FUNCTION / DEVICE / BASIC_SETTING)
 *
 * BASIC_FUNCTION (FUN1=1):
 * - fun[1] (FUN2): item
 *
 * DEVICE (FUN1=2):
 * - fun[1] (FUN2): device_no
 * - fun[2] (FUN3): device_type
 * - fun[3] (FUN4): device_category (CONTROL=0 / CONFIG=1)
 * - fun[4] (FUN5): item (meaning depends on device_type + category)
 *
 * BASIC_SETTING (FUN1=3):
 * - fun[1] (FUN2): item
 * - fun[2] (FUN3): optional extension field (depends on item)
 *
 * Device-level summary functions are split by category:
 * - knx_device_air.c     : 空调控制+配置
 * - knx_device_key.c     : 开关控制+配置
 * - knx_device_dimming.c : 灯光控制+配置
 * - knx_device_curtain.c : 窗帘控制+配置
 * - knx_basic_function.c : 基本功能处理
 * - knx_basic_setting.c  : 基本设置处理
 */

#include "knx_internal.h"

#if KNX_LOG_ENABLE
/* ===== Name helpers ====================================================== */
static const char *knx_get_command_name(uint8_t command);
static const char *knx_get_fun1_name(uint8_t fun1);
static const char *knx_get_basic_function_name(uint8_t item);
static const char *knx_get_basic_setting_name(uint8_t item);
static const char *knx_get_device_type_name(uint8_t device_type);
static const char *knx_get_device_category_name(uint8_t category);
static const char *knx_get_device_item_name(uint8_t device_type, uint8_t category, uint8_t item);
static const char *knx_get_basic_setting_function_type_name(uint8_t function_type);

/* ===== Logging helpers =================================================== */
static void knx_log_fun_path(const KNX_Frame_t *frame);
static void knx_log_data_hex(const uint8_t *data, uint16_t len);
static void knx_log_basic_function(const KNX_Frame_t *frame);
static void knx_log_device_function(const KNX_Frame_t *frame);
static void knx_log_basic_setting(const KNX_Frame_t *frame);
static void knx_log_config_frame(const KNX_Frame_t *frame);
static void knx_log_response_frame(const KNX_Frame_t *frame);
static void knx_log_single_byte_value(const char *label, const KNX_Frame_t *frame);
static void knx_log_u16_value(const char *label, const KNX_Frame_t *frame);
static void knx_log_s16_tenths_value(const char *label, const KNX_Frame_t *frame);
static void knx_log_utf8_string(const char *label, const KNX_Frame_t *frame);
#endif

#if KNX_LOG_ENABLE
static KNX_SummaryCallback_t s_knx_summary_callback = NULL;

void knx_register_summary_callback(KNX_SummaryCallback_t callback)
{
    s_knx_summary_callback = callback;
}
#else
void knx_register_summary_callback(KNX_SummaryCallback_t callback)
{
    (void)callback;
}
#endif

void knx_setting_frame_reset(HOOCH_PROTOCOL_SettingFrame_t *setting_frame)
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
    (void)memset(setting_frame->data, 0, sizeof(setting_frame->data));
}

/*
 * Entry point for protocol dispatch.
 *
 * Input:
 * - frame: A parsed and checksum-verified KNX frame.
 *
 * Behavior:
 * - Prints frame summary.
 * - Routes frame by command type (A1/A2/A3/C1/C2/C3).
 * - For A1/A2/A3, decodes fun path and prints semantic meaning.
 */
void knx_handle_frame(const KNX_Frame_t *frame)
{
    if (frame == NULL)
    {
        KNX_LOG_WARN("[KNX] frame is NULL\r\n");
        return;
    }

#if KNX_LOG_ENABLE
    KNX_LOG_INFO("[KNX] CMD=%s(0x%02X), FUN_COUNT=%u, DATA_LEN=%u, RESP=0x%02X\r\n",
             knx_get_command_name(frame->command),
             frame->command,
             (unsigned int)frame->fun_count,
             (unsigned int)frame->data_len,
             frame->resp_type);
#endif

    /* 根据 command 分发不同类型的帧处理（A1/A2/A3/C1/C2/C3）。 */
    switch (frame->command)
    {
    case KNX_CMD_WRITE_DATA:
    case KNX_CMD_QUERY_DATA:
    case KNX_CMD_REQUEST_CONFIG:
#if KNX_LOG_ENABLE
        knx_log_fun_path(frame);
        if (frame->fun_count == 0U)
        {
            KNX_LOG_WARN("[KNX] Empty function path\r\n");
            break;
        }

        /* 根据 fun1（功能大类）进一步分发。 */
        switch (frame->fun[0])
        {
        case KNX_FUN1_BASIC_FUNCTION:
            knx_log_basic_function(frame);
            break;

        case KNX_FUN1_DEVICE:
            knx_log_device_function(frame);
            break;

        case KNX_FUN1_BASIC_SETTING:
            knx_log_basic_setting(frame);
            break;

        default:
            KNX_LOG_WARN("[KNX] Unknown FUN1=0x%02X\r\n", frame->fun[0]);
            if (frame->data_len > 0U)
            {
                knx_log_data_hex(frame->data, frame->data_len);
            }
            break;
        }
#else
        if (frame->fun_count == 0U)
        {
            break;
        }

        switch (frame->fun[0])
        {
        case KNX_FUN1_BASIC_FUNCTION:
            knx_summary_basic_function(frame);
            break;

        case KNX_FUN1_DEVICE:
            if (frame->fun_count < 5U)
            {
                break;
            }

            switch (frame->fun[2])
            {
            case KNX_DEVICE_TYPE_AIR_CONDITIONER:
                if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL)
                {
                    knx_summary_air_control(frame);
                }
                else if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG)
                {
                    knx_summary_air_config(frame);
                }
                break;

            case KNX_DEVICE_TYPE_KEY:
                if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL)
                {
                    knx_summary_key_control(frame);
                }
                else if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG)
                {
                    knx_summary_key_config(frame);
                }
                break;

            case KNX_DEVICE_TYPE_DIMMING:
                if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL)
                {
                    knx_summary_dimming_control(frame);
                }
                else if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG)
                {
                    knx_summary_dimming_config(frame);
                }
                break;

            case KNX_DEVICE_TYPE_CURTAIN:
                if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL)
                {
                    knx_summary_curtain_control(frame);
                }
                else if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG)
                {
                    knx_summary_curtain_config(frame);
                }
                break;

            case KNX_DEVICE_TYPE_FLOOR_HEATING:
                if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL)
                {
                    knx_summary_floor_heating_control(frame);
                }
                else if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG)
                {
                    knx_summary_floor_heating_config(frame);
                }
                break;

            case KNX_DEVICE_TYPE_FRESH_AIR:
                if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL)
                {
                    knx_summary_fresh_air_control(frame);
                }
                else if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG)
                {
                    knx_summary_fresh_air_config(frame);
                }
                break;

            case KNX_DEVICE_TYPE_SCENE:
                if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL)
                {
                    knx_summary_scene_control(frame);
                }
                else if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG)
                {
                    knx_summary_scene_config(frame);
                }
                break;

            default:
                break;
            }
            break;

        case KNX_FUN1_BASIC_SETTING:
            knx_summary_basic_setting(frame);
            break;

        default:
            break;
        }
#endif
        break;

    case KNX_CMD_WRITE_CONFIG:
    case KNX_CMD_READ_CONFIG:
#if KNX_LOG_ENABLE
        knx_log_config_frame(frame);
#endif
        break;

    case KNX_CMD_RESPONSE:
#if KNX_LOG_ENABLE
        knx_log_response_frame(frame);
#endif
        break;

    default:
#if KNX_LOG_ENABLE
        KNX_LOG_WARN("[KNX] Unsupported frame command 0x%02X\r\n", frame->command);
#endif
        break;
    }
}

#if KNX_LOG_ENABLE
static const char *knx_get_command_name(uint8_t command)
{
    /* 将命令字转换为可读字符串，便于日志输出。 */
    switch (command)
    {
    case KNX_CMD_WRITE_DATA:
        return "WRITE_DATA";
    case KNX_CMD_QUERY_DATA:
        return "QUERY_DATA";
    case KNX_CMD_REQUEST_CONFIG:
        return "REQUEST_CONFIG";
    case KNX_CMD_WRITE_CONFIG:
        return "WRITE_CONFIG";
    case KNX_CMD_READ_CONFIG:
        return "READ_CONFIG";
    case KNX_CMD_RESPONSE:
        return "RESPONSE";
    default:
        return "UNKNOWN_COMMAND";
    }
}

static const char *knx_get_response_type_name(uint8_t resp_type)
{
    switch (resp_type)
    {
    case KNX_RESPONSE_TYPE_ACK:
        return "ACK";
    case KNX_RESPONSE_TYPE_HEARTBEAT:
        return "HEARTBEAT";
    case KNX_RESPONSE_TYPE_BUSY:
        return "BUSY";
    case KNX_RESPONSE_TYPE_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN_RESPONSE";
    }
}

static const char *knx_get_fun1_name(uint8_t fun1)
{
    /* 将 fun1（功能大类）转换为可读字符串。 */
    switch (fun1)
    {
    case KNX_FUN1_BASIC_FUNCTION:
        return "BASIC_FUNCTION";
    case KNX_FUN1_DEVICE:
        return "DEVICE";
    case KNX_FUN1_BASIC_SETTING:
        return "BASIC_SETTING";
    default:
        return "UNKNOWN_FUN1";
    }
}

static const char *knx_get_basic_function_name(uint8_t item)
{
    /* BASIC_FUNCTION 下，fun2=item，将 item 转换为可读字符串。 */
    switch (item)
    {
    case KNX_BASIC_FUNCTION_DATE:               return "DATE";
    case KNX_BASIC_FUNCTION_TIME:               return "TIME";
    case KNX_BASIC_FUNCTION_VERSION:            return "VERSION_INFO";
    case KNX_BASIC_FUNCTION_UPDATE_CONFIG:      return "UPDATE_CONFIG";
    case KNX_BASIC_FUNCTION_CONFIG_SYNC_STATUS: return "CONFIG_SYNC_STATUS";
    case KNX_BASIC_FUNCTION_PANEL_UPDATE_STATUS:return "PANEL_UPDATE_STATUS";
    case KNX_BASIC_FUNCTION_DEVICE_TYPE:        return "DEVICE_TYPE";
    case KNX_BASIC_FUNCTION_UNIQUE_SERIAL:      return "UNIQUE_SERIAL";
    case KNX_BASIC_FUNCTION_SUPPORT_PARAMETER:  return "SUPPORT_PARAMETER";
    case KNX_BASIC_FUNCTION_PROGRAM_MODE:       return "PROGRAM_MODE";
    case KNX_BASIC_FUNCTION_DEVICE_REBOOT:      return "DEVICE_REBOOT";
    case KNX_BASIC_FUNCTION_RESTORE_FACTORY:    return "RESTORE_FACTORY";
    case KNX_BASIC_FUNCTION_DEVICE_LOCK:        return "DEVICE_LOCK";
    case KNX_BASIC_FUNCTION_WORLD_TIME:         return "WORLD_TIME";
    case KNX_BASIC_FUNCTION_TEMPERATURE_SENSOR: return "TEMPERATURE_SENSOR";
    case KNX_BASIC_FUNCTION_HUMIDITY_SENSOR:    return "HUMIDITY_SENSOR";
    case KNX_BASIC_FUNCTION_HUMAN_PRESENCE_SENSOR: return "HUMAN_PRESENCE_SENSOR";
    case KNX_BASIC_FUNCTION_BACKLIGHT_SCREEN_CONTROL: return "BACKLIGHT_SCREEN_CONTROL";
    default:
        return "UNKNOWN_BASIC_FUNCTION";
    }
}

static const char *knx_get_basic_setting_name(uint8_t item)
{
    /* BASIC_SETTING 下，fun2=item，将 item 转换为可读字符串。 */
    switch (item)
    {
    case 1U:
        return "SCREENSAVER_TYPE";
    case 2U:
        return "THEME";
    case 3U:
        return "BACKGROUND";
    case 4U:
        return "DEVICE_LOCK_CONTROL";
    case 5U:
        return "KEY_CHILD_LOCK";
    case 6U:
        return "SCREEN_BACKLIGHT_BRIGHTNESS";
    case 7U:
        return "AUTO_BRIGHTNESS_ENABLE";
    case 8U:
        return "KEY_BACKLIGHT_BRIGHTNESS_NIGHT";
    case 9U:
        return "KEY_BRIGHTNESS_NIGHT";
    case 10U:
        return "KEY_BACKLIGHT_MODE";
    case 11U:
        return "KEY_WAKEUP_BACKLIGHT_ENABLE";
    case 12U:
        return "KEY_STATUS_LINK_BACKLIGHT";
    case 13U:
        return "KEY_LONG_PRESS_TIME";
    case 14U:
        return "KEY_MULTI_EFFECTIVE";
    case 15U:
        return "KEY_BACKLIGHT_TIME";
    case 16U:
        return "PRESENCE_SENSOR_ENABLE";
    case 17U:
        return "PRESENCE_SENSOR_SENSITIVITY";
    case 18U:
        return "KEY_BACKLIGHT_BRIGHTNESS_DAY";
    case 19U:
        return "KEY_BRIGHTNESS_DAY";
    case 20U:
        return "SCREEN_STANDBY_BRIGHTNESS";
    case 22U:
        return "SCREEN_SCREENSAVER_ENABLE";
    case 23U:
        return "SCREEN_CHILD_LOCK";
    case 24U:
        return "SCREEN_OFF_TIME";
    case 25U:
        return "RETURN_HOME_TIME";
    case 26U:
        return "ENTER_SCREENSAVER_TIME";
    case 27U:
        return "SCREEN_PASSWORD";
    case 28U:
        return "TEMPERATURE_UNIT";
    case 29U:
        return "PAGE_BUTTON_FUNCTION";
    case KNX_BASIC_SETTING_LANGUAGE_SELECT:
        return "LANGUAGE_SELECT";
    case KNX_BASIC_SETTING_SOUND_ENABLE:
        return "SOUND_ENABLE";
    case KNX_BASIC_SETTING_VIBRATION_ENABLE:
        return "VIBRATION_ENABLE";
    case KNX_BASIC_SETTING_PAGE_MANAGEMENT:
        return "PAGE_MANAGEMENT";
    default:
        return "UNKNOWN_BASIC_SETTING";
    }
}

static const char *knx_get_device_type_name(uint8_t device_type)
{
    /* DEVICE 下，fun3=device_type，将设备类型转换为可读字符串。 */
    switch (device_type)
    {
    case KNX_DEVICE_TYPE_AIR_CONDITIONER:
        return "AIR_CONDITIONER";
    case KNX_DEVICE_TYPE_FLOOR_HEATING:
        return "FLOOR_HEATING";
    case KNX_DEVICE_TYPE_FRESH_AIR:
        return "FRESH_AIR";
    case KNX_DEVICE_TYPE_HVAC:
        return "HVAC";
    case KNX_DEVICE_TYPE_KEY:
        return "KEY";
    case KNX_DEVICE_TYPE_DIMMING:
        return "DIMMING";
    case KNX_DEVICE_TYPE_CURTAIN:
        return "CURTAIN";
    case KNX_DEVICE_TYPE_SCENE:
        return "SCENE";
    case KNX_DEVICE_TYPE_AV:
        return "AV";
    case KNX_DEVICE_TYPE_VALUE_DISPLAY:
        return "VALUE_DISPLAY";
    case KNX_DEVICE_TYPE_TOUCH_KEY:
        return "TOUCH_KEY";
    case KNX_DEVICE_TYPE_PUBLIC_DISPLAY:
        return "PUBLIC_DISPLAY";
    default:
        return "UNKNOWN_DEVICE_TYPE";
    }
}

static const char *knx_get_device_category_name(uint8_t category)
{
    /* DEVICE 下，fun4=category，区分 CONTROL(控制参数) / CONFIG(配置参数)。 */
    switch (category)
    {
    case KNX_DEVICE_CATEGORY_CONTROL:
        return "CONTROL";
    case KNX_DEVICE_CATEGORY_CONFIG:
        return "CONFIG";
    default:
        return "UNKNOWN_CATEGORY";
    }
}

static const char *knx_get_device_item_name(uint8_t device_type, uint8_t category, uint8_t item)
{
    /*
     * DEVICE item name lookup.
     *
     * category == CONTROL:
     * - item means "control/status item" (e.g. SWITCH, MODE, BRIGHTNESS, ...)
     *
     * category == CONFIG:
     * - item means "configuration item" (e.g. ENABLE_BITMAP, DESCRIPTION, ...)
     */
    /* 先按 category 区分：控制项名称 / 配置项名称。 */
    if (category == KNX_DEVICE_CATEGORY_CONTROL)
    {
        /* CONTROL：根据 device_type 选择对应的控制项枚举集合。 */
        switch (device_type)
        {
        case KNX_DEVICE_TYPE_AIR_CONDITIONER:
            /* 空调控制项：item -> 名称。 */
            switch (item)
            {
            case KNX_AIR_ITEM_SWITCH: return "SWITCH";
            case KNX_AIR_ITEM_SWITCH_STATUS: return "SWITCH_STATUS";
            case KNX_AIR_ITEM_MODE: return "MODE";
            case KNX_AIR_ITEM_MODE_STATUS: return "MODE_STATUS";
            case KNX_AIR_ITEM_FAN_SPEED: return "FAN_SPEED";
            case KNX_AIR_ITEM_FAN_SPEED_STATUS: return "FAN_SPEED_STATUS";
            case KNX_AIR_ITEM_SET_TEMPERATURE: return "SET_TEMPERATURE";
            case KNX_AIR_ITEM_SET_TEMPERATURE_STATUS: return "SET_TEMPERATURE_STATUS";
            case KNX_AIR_ITEM_ACTUAL_TEMPERATURE: return "ACTUAL_TEMPERATURE";
            default: return "UNKNOWN_AIR_ITEM";
            }

        case KNX_DEVICE_TYPE_FLOOR_HEATING:
            /* 地暖控制项：item -> 名称。 */
            switch (item)
            {
            case KNX_FLOOR_ITEM_SWITCH: return "SWITCH";
            case KNX_FLOOR_ITEM_SWITCH_STATUS: return "SWITCH_STATUS";
            case KNX_FLOOR_ITEM_SET_TEMPERATURE: return "SET_TEMPERATURE";
            case KNX_FLOOR_ITEM_SET_TEMPERATURE_STATUS: return "SET_TEMPERATURE_STATUS";
            case KNX_FLOOR_ITEM_ACTUAL_TEMPERATURE: return "ACTUAL_TEMPERATURE";
            case KNX_FLOOR_ITEM_MANUAL_AUTO: return "MANUAL_AUTO";
            case KNX_FLOOR_ITEM_MANUAL_AUTO_STATUS: return "MANUAL_AUTO_STATUS";
            case KNX_FLOOR_ITEM_RELAY_SWITCH: return "RELAY_SWITCH";
            case KNX_FLOOR_ITEM_RELAY_SWITCH_STATUS: return "RELAY_SWITCH_STATUS";
            default: return "UNKNOWN_FLOOR_ITEM";
            }

        case KNX_DEVICE_TYPE_FRESH_AIR:
            /* 新风控制项：item -> 名称。 */
            switch (item)
            {
            case KNX_FRESH_AIR_ITEM_SWITCH: return "SWITCH";
            case KNX_FRESH_AIR_ITEM_SWITCH_STATUS: return "SWITCH_STATUS";
            case KNX_FRESH_AIR_ITEM_FAN_SPEED_MODE: return "FAN_SPEED_MODE";
            case KNX_FRESH_AIR_ITEM_FAN_SPEED_MODE_STATUS: return "FAN_SPEED_MODE_STATUS";
            case KNX_FRESH_AIR_ITEM_ACTUAL_FAN_SPEED: return "ACTUAL_FAN_SPEED";
            case KNX_FRESH_AIR_ITEM_ACTUAL_TEMPERATURE: return "ACTUAL_TEMPERATURE";
            default: return "UNKNOWN_FRESH_AIR_ITEM";
            }

        case KNX_DEVICE_TYPE_KEY:
            /* 按键控制项：item -> 名称。 */
            switch (item)
            {
            case KNX_KEY_ITEM_SWITCH: return "SWITCH";
            case KNX_KEY_ITEM_SWITCH_STATUS: return "SWITCH_STATUS";
            default: return "UNKNOWN_KEY_ITEM";
            }

        case KNX_DEVICE_TYPE_DIMMING:
            /* 调光控制项：item -> 名称。 */
            switch (item)
            {
            case KNX_DIMMING_ITEM_SWITCH: return "SWITCH";
            case KNX_DIMMING_ITEM_SWITCH_STATUS: return "SWITCH_STATUS";
            case KNX_DIMMING_ITEM_BRIGHTNESS: return "BRIGHTNESS";
            case KNX_DIMMING_ITEM_BRIGHTNESS_STATUS: return "BRIGHTNESS_STATUS";
            case KNX_DIMMING_ITEM_COLOR_TEMPERATURE: return "COLOR_TEMPERATURE";
            case KNX_DIMMING_ITEM_COLOR_TEMPERATURE_STATUS: return "COLOR_TEMPERATURE_STATUS";
            case KNX_DIMMING_ITEM_RGBW: return "RGBW";
            case KNX_DIMMING_ITEM_RGBW_STATUS: return "RGBW_STATUS";
            case KNX_DIMMING_ITEM_COLOR_TEMP_PERCENT: return "COLOR_TEMP_PERCENT";
            case KNX_DIMMING_ITEM_COLOR_TEMP_PERCENT_STATUS: return "COLOR_TEMP_PERCENT_STATUS";
            default: return "UNKNOWN_DIMMING_ITEM";
            }

        case KNX_DEVICE_TYPE_CURTAIN:
            /* 窗帘控制项：item -> 名称。 */
            switch (item)
            {
            case KNX_CURTAIN_ITEM_POSITION_OPEN_CLOSE: return "POSITION_OPEN_CLOSE";
            case KNX_CURTAIN_ITEM_POSITION_STOP: return "POSITION_STOP";
            case KNX_CURTAIN_ITEM_POSITION_PERCENT: return "POSITION_PERCENT";
            case KNX_CURTAIN_ITEM_POSITION_PERCENT_STATUS: return "POSITION_PERCENT_STATUS";
            case KNX_CURTAIN_ITEM_ANGLE_OPEN_CLOSE: return "ANGLE_OPEN_CLOSE";
            case KNX_CURTAIN_ITEM_ANGLE_STOP: return "ANGLE_STOP";
            case KNX_CURTAIN_ITEM_ANGLE_PERCENT: return "ANGLE_PERCENT";
            case KNX_CURTAIN_ITEM_ANGLE_PERCENT_STATUS: return "ANGLE_PERCENT_STATUS";
            default: return "UNKNOWN_CURTAIN_ITEM";
            }

        case KNX_DEVICE_TYPE_SCENE:
            /* 场景控制项：item -> 名称。 */
            switch (item)
            {
            case KNX_SCENE_ITEM_TRIGGER: return "TRIGGER";
            case KNX_SCENE_ITEM_STATUS: return "STATUS";
            case KNX_SCENE_ITEM_LEARN: return "LEARN";
            default: return "UNKNOWN_SCENE_ITEM";
            }

        case KNX_DEVICE_TYPE_AV:
            /* 影音控制项：item -> 名称。 */
            switch (item)
            {
            case KNX_AV_ITEM_SWITCH: return "SWITCH";
            case KNX_AV_ITEM_SWITCH_STATUS: return "SWITCH_STATUS";
            case KNX_AV_ITEM_UP_DOWN: return "UP_DOWN";
            case KNX_AV_ITEM_LEFT_RIGHT: return "LEFT_RIGHT";
            case KNX_AV_ITEM_VOLUME_UP_DOWN: return "VOLUME_UP_DOWN";
            case KNX_AV_ITEM_VOLUME_SET: return "VOLUME_SET";
            case KNX_AV_ITEM_VOLUME_STATUS: return "VOLUME_STATUS";
            case KNX_AV_ITEM_CONFIRM: return "CONFIRM";
            case KNX_AV_ITEM_BACK: return "BACK";
            case KNX_AV_ITEM_MUTE: return "MUTE";
            case KNX_AV_ITEM_MUTE_STATUS: return "MUTE_STATUS";
            case KNX_AV_ITEM_PLAY_PAUSE: return "PLAY_PAUSE";
            case KNX_AV_ITEM_PLAY_MODE_CONTROL: return "PLAY_MODE_CONTROL";
            case KNX_AV_ITEM_PLAY_MODE_STATUS: return "PLAY_MODE_STATUS";
            case KNX_AV_ITEM_PREV_NEXT: return "PREV_NEXT";
            default: return "UNKNOWN_AV_ITEM";
            }

        case KNX_DEVICE_TYPE_VALUE_DISPLAY:
            /* 数值显示控制项：item -> 名称。 */
            switch (item)
            {
            case KNX_VALUE_DISPLAY_ITEM_VALUE: return "VALUE";
            default: return "UNKNOWN_VALUE_DISPLAY_ITEM";
            }

        case KNX_DEVICE_TYPE_TOUCH_KEY:
            /* 轻触按键控制项：item -> 名称。 */
            switch (item)
            {
            case KNX_TOUCH_KEY_ITEM_KEY_STATUS: return "KEY_STATUS";
            case KNX_TOUCH_KEY_ITEM_LED_STATUS: return "LED_STATUS";
            case KNX_TOUCH_KEY_ITEM_KEY_LOCK: return "KEY_LOCK";
            default: return "UNKNOWN_TOUCH_KEY_ITEM";
            }

        case KNX_DEVICE_TYPE_PUBLIC_DISPLAY:
            /* 公共显示控制项：item -> 名称。 */
            switch (item)
            {
            case KNX_PUBLIC_DISPLAY_ITEM_TEMPERATURE: return "TEMPERATURE";
            case KNX_PUBLIC_DISPLAY_ITEM_HUMIDITY: return "HUMIDITY";
            case KNX_PUBLIC_DISPLAY_ITEM_ILLUMINANCE: return "ILLUMINANCE";
            case KNX_PUBLIC_DISPLAY_ITEM_CO: return "CO";
            case KNX_PUBLIC_DISPLAY_ITEM_PM2_5: return "PM2_5";
            case KNX_PUBLIC_DISPLAY_ITEM_PM10: return "PM10";
            case KNX_PUBLIC_DISPLAY_ITEM_HCHO: return "HCHO";
            case KNX_PUBLIC_DISPLAY_ITEM_TVOC: return "TVOC";
            case KNX_PUBLIC_DISPLAY_ITEM_CO2: return "CO2";
            case KNX_PUBLIC_DISPLAY_ITEM_AQI: return "AQI";
            case KNX_PUBLIC_DISPLAY_ITEM_TEMPERATURE_CALIBRATION: return "TEMPERATURE_CALIBRATION";
            case KNX_PUBLIC_DISPLAY_ITEM_HUMIDITY_CALIBRATION: return "HUMIDITY_CALIBRATION";
            case KNX_PUBLIC_DISPLAY_ITEM_ILLUMINANCE_CALIBRATION: return "ILLUMINANCE_CALIBRATION";
            default: return "UNKNOWN_PUBLIC_DISPLAY_ITEM";
            }

        default:
            return "UNKNOWN_DEVICE_ITEM";
        }
    }

    /* CONFIG：根据 device_type 选择对应的配置项枚举集合。 */
    switch (device_type)
    {
    case KNX_DEVICE_TYPE_AIR_CONDITIONER:
        /* 空调配置项：item -> 名称。 */
        switch (item)
        {
        case KNX_AIR_CONFIG_ENABLE_BITMAP: return "ENABLE_BITMAP";
        case KNX_AIR_CONFIG_DESCRIPTION: return "DESCRIPTION";
        case KNX_AIR_CONFIG_DEFAULT_ICON: return "DEFAULT_ICON";
        case KNX_AIR_CONFIG_SELECTED_ICON: return "SELECTED_ICON";
        case KNX_AIR_CONFIG_TEMP_STEP: return "TEMP_STEP";
        case KNX_AIR_CONFIG_TEMP_MIN: return "TEMP_MIN";
        case KNX_AIR_CONFIG_TEMP_MAX: return "TEMP_MAX";
        default: return "UNKNOWN_AIR_CONFIG";
        }

    case KNX_DEVICE_TYPE_FLOOR_HEATING:
        /* 地暖配置项：item -> 名称。 */
        switch (item)
        {
        case KNX_FLOOR_CONFIG_ENABLE_BITMAP: return "ENABLE_BITMAP";
        case KNX_FLOOR_CONFIG_DESCRIPTION: return "DESCRIPTION";
        case KNX_FLOOR_CONFIG_DEFAULT_ICON: return "DEFAULT_ICON";
        case KNX_FLOOR_CONFIG_SELECTED_ICON: return "SELECTED_ICON";
        case KNX_FLOOR_CONFIG_TEMP_STEP: return "TEMP_STEP";
        case KNX_FLOOR_CONFIG_TEMP_MIN: return "TEMP_MIN";
        case KNX_FLOOR_CONFIG_TEMP_MAX: return "TEMP_MAX";
        default: return "UNKNOWN_FLOOR_CONFIG";
        }

    case KNX_DEVICE_TYPE_FRESH_AIR:
        /* 新风配置项：item -> 名称。 */
        switch (item)
        {
        case KNX_FRESH_AIR_CONFIG_ENABLE_BITMAP: return "ENABLE_BITMAP";
        case KNX_FRESH_AIR_CONFIG_DESCRIPTION: return "DESCRIPTION";
        case KNX_FRESH_AIR_CONFIG_DEFAULT_ICON: return "DEFAULT_ICON";
        case KNX_FRESH_AIR_CONFIG_SELECTED_ICON: return "SELECTED_ICON";
        default: return "UNKNOWN_FRESH_AIR_CONFIG";
        }

    case KNX_DEVICE_TYPE_KEY:
        /* 按键配置项：item -> 名称。 */
        switch (item)
        {
        case KNX_KEY_CONFIG_ENABLE_BITMAP: return "ENABLE_BITMAP";
        case KNX_KEY_CONFIG_DESCRIPTION: return "DESCRIPTION";
        case KNX_KEY_CONFIG_DEFAULT_ICON: return "DEFAULT_ICON";
        case KNX_KEY_CONFIG_SELECTED_ICON: return "SELECTED_ICON";
        default: return "UNKNOWN_KEY_CONFIG";
        }

    case KNX_DEVICE_TYPE_DIMMING:
        /* 调光配置项：item -> 名称。 */
        switch (item)
        {
        case KNX_DIMMING_CONFIG_ENABLE_BITMAP: return "ENABLE_BITMAP";
        case KNX_DIMMING_CONFIG_DESCRIPTION: return "DESCRIPTION";
        case KNX_DIMMING_CONFIG_DEFAULT_ICON: return "DEFAULT_ICON";
        case KNX_DIMMING_CONFIG_SELECTED_ICON: return "SELECTED_ICON";
        case KNX_DIMMING_CONFIG_COLOR_TEMP_STEP: return "COLOR_TEMP_STEP";
        case KNX_DIMMING_CONFIG_COLOR_TEMP_MIN: return "COLOR_TEMP_MIN";
        case KNX_DIMMING_CONFIG_COLOR_TEMP_MAX: return "COLOR_TEMP_MAX";
        default: return "UNKNOWN_DIMMING_CONFIG";
        }

    case KNX_DEVICE_TYPE_CURTAIN:
        /* 窗帘配置项：item -> 名称。 */
        switch (item)
        {
        case KNX_CURTAIN_CONFIG_ENABLE_BITMAP: return "ENABLE_BITMAP";
        case KNX_CURTAIN_CONFIG_DESCRIPTION: return "DESCRIPTION";
        case KNX_CURTAIN_CONFIG_DEFAULT_ICON: return "DEFAULT_ICON";
        case KNX_CURTAIN_CONFIG_SELECTED_ICON: return "SELECTED_ICON";
        case KNX_CURTAIN_CONFIG_TYPE: return "TYPE";
        default: return "UNKNOWN_CURTAIN_CONFIG";
        }

    case KNX_DEVICE_TYPE_SCENE:
        /* 场景配置项：item -> 名称。 */
        switch (item)
        {
        case KNX_SCENE_CONFIG_ENABLE_BITMAP: return "ENABLE_BITMAP";
        case KNX_SCENE_CONFIG_DESCRIPTION: return "DESCRIPTION";
        case KNX_SCENE_CONFIG_DEFAULT_ICON: return "DEFAULT_ICON";
        case KNX_SCENE_CONFIG_SELECTED_ICON: return "SELECTED_ICON";
        default: return "UNKNOWN_SCENE_CONFIG";
        }

    case KNX_DEVICE_TYPE_AV:
        /* 影音配置项：item -> 名称。 */
        switch (item)
        {
        case KNX_AV_CONFIG_ENABLE_BITMAP: return "ENABLE_BITMAP";
        case KNX_AV_CONFIG_DESCRIPTION: return "DESCRIPTION";
        case KNX_AV_CONFIG_DEFAULT_ICON: return "DEFAULT_ICON";
        case KNX_AV_CONFIG_SELECTED_ICON: return "SELECTED_ICON";
        default: return "UNKNOWN_AV_CONFIG";
        }

    case KNX_DEVICE_TYPE_VALUE_DISPLAY:
        /* 数值显示配置项：item -> 名称。 */
        switch (item)
        {
        case KNX_VALUE_DISPLAY_CONFIG_ENABLE_BITMAP: return "ENABLE_BITMAP";
        case KNX_VALUE_DISPLAY_CONFIG_DESCRIPTION: return "DESCRIPTION";
        case KNX_VALUE_DISPLAY_CONFIG_DEFAULT_ICON: return "DEFAULT_ICON";
        case KNX_VALUE_DISPLAY_CONFIG_SELECTED_ICON: return "SELECTED_ICON";
        case KNX_VALUE_DISPLAY_CONFIG_UNIT: return "UNIT";
        case KNX_VALUE_DISPLAY_CONFIG_ARRAY_SIZE: return "ARRAY_SIZE";
        case KNX_VALUE_DISPLAY_CONFIG_DATA_TYPE: return "DATA_TYPE";
        case KNX_VALUE_DISPLAY_CONFIG_DECIMAL_COUNT: return "DECIMAL_COUNT";
        default: return "UNKNOWN_VALUE_DISPLAY_CONFIG";
        }

    default:
        return "UNKNOWN_DEVICE_CONFIG";
    }
}

static const char *knx_get_basic_setting_function_type_name(uint8_t function_type)
{
    /* BASIC_SETTING: item=4 时，fun3=function_type（功能类型）。 */
    switch (function_type)
    {
    case KNX_SETTING_FUNCTION_TYPE_AIR_CONDITIONER:
        return "AIR_CONDITIONER";
    case KNX_SETTING_FUNCTION_TYPE_FLOOR_HEATING:
        return "FLOOR_HEATING";
    case KNX_SETTING_FUNCTION_TYPE_FRESH_AIR:
        return "FRESH_AIR";
    case KNX_SETTING_FUNCTION_TYPE_DIMMING:
        return "DIMMING";
    case KNX_SETTING_FUNCTION_TYPE_CURTAIN:
        return "CURTAIN";
    default:
        return "UNKNOWN_FUNCTION_TYPE";
    }
}

#endif /* KNX_LOG_ENABLE (name helpers) */

/* ===== Endian helpers (always available) ================================= */

uint16_t knx_read_be_u16(const uint8_t *data)
{
    /* Convert 2-byte little-endian to uint16_t. */
    return (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
}

int16_t knx_read_be_s16(const uint8_t *data)
{
    /* Convert 2-byte big-endian to int16_t. */
    return (int16_t)knx_read_be_u16(data);
}

uint32_t knx_read_be_u32(const uint8_t *data)
{
    /* Convert 4-byte big-endian to uint32_t. */
    return ((uint32_t)data[0] << 24)
         | ((uint32_t)data[1] << 16)
         | ((uint32_t)data[2] << 8)
         | (uint32_t)data[3];
}

float knx_read_be_float(const uint8_t *data)
{
    /* Convert 4-byte big-endian IEEE754 to float. */
    union
    {
        uint32_t u32;
        float f32;
    } converter;

    converter.u32 = knx_read_be_u32(data);
    return converter.f32;
}

#if KNX_LOG_ENABLE /* logging helpers */

static void knx_log_fun_path(const KNX_Frame_t *frame)
{
    uint8_t index;

    KNX_LOG_INFO("[KNX] FUN1=%s(0x%02X)\r\n",
             knx_get_fun1_name(frame->fun[0]),
             frame->fun[0]);

    KNX_LOG_INFO("[KNX] FUN_PATH:");
    for (index = 0U; index < frame->fun_count; index++)
    {
        KNX_LOG_INFO(" %02X", frame->fun[index]);
    }
    KNX_LOG_INFO("\r\n");
}

static void knx_log_data_hex(const uint8_t *data, uint16_t len)
{
    uint16_t index;

    if ((data == NULL) || (len == 0U))
    {
        return;
    }

    KNX_LOG_INFO("[KNX] DATA:");
    for (index = 0U; index < len; index++)
    {
        KNX_LOG_INFO(" %02X", data[index]);
    }
    KNX_LOG_INFO("\r\n");
}

static void knx_log_basic_function(const KNX_Frame_t *frame)
{
    /*
     * BASIC_FUNCTION:
     * - fun[0] = FUN1 = 1
     * - fun[1] = FUN2 = item
     * - data layout depends on item
     */
    if (frame->fun_count < 2U)
    {
        KNX_LOG_WARN("[KNX] BASIC_FUNCTION requires FUN2\r\n");
        return;
    }

    KNX_LOG_INFO("[KNX] BASIC_FUNCTION item=%s(0x%02X)\r\n",
             knx_get_basic_function_name(frame->fun[1]),
             frame->fun[1]);

    knx_summary_basic_function(frame);

    /* 根据 BASIC_FUNCTION 的 item（fun2）选择数据解析方式。 */
    switch (frame->fun[1])
    {
    case KNX_BASIC_FUNCTION_DATE:
        if (frame->data_len >= 4U)
        {
            KNX_LOG_INFO("[KNX] DATE year=%u month=%u day=%u\r\n",
                     (unsigned int)knx_read_be_u16(frame->data),
                     (unsigned int)frame->data[2],
                     (unsigned int)frame->data[3]);
        }
        break;

    case KNX_BASIC_FUNCTION_TIME:
        if (frame->data_len >= 4U)
        {
            KNX_LOG_INFO("[KNX] TIME hour=%u minute=%u second=%u weekday=%u\r\n",
                     (unsigned int)frame->data[0],
                     (unsigned int)frame->data[1],
                     (unsigned int)frame->data[2],
                     (unsigned int)frame->data[3]);
        }
        break;

    case KNX_BASIC_FUNCTION_VERSION:
        if (frame->data_len >= 2U)
        {
            KNX_LOG_INFO("[KNX] VERSION %u.%u\r\n",
                     (unsigned int)frame->data[0],
                     (unsigned int)frame->data[1]);
        }
        break;

    case KNX_BASIC_FUNCTION_UPDATE_CONFIG:
    case KNX_BASIC_FUNCTION_CONFIG_SYNC_STATUS:
    case KNX_BASIC_FUNCTION_PANEL_UPDATE_STATUS:
    case KNX_BASIC_FUNCTION_SUPPORT_PARAMETER:
    case KNX_BASIC_FUNCTION_PROGRAM_MODE:
    case KNX_BASIC_FUNCTION_DEVICE_REBOOT:
    case KNX_BASIC_FUNCTION_RESTORE_FACTORY:
    case KNX_BASIC_FUNCTION_DEVICE_LOCK:
        knx_log_single_byte_value("VALUE", frame);
        break;

    case KNX_BASIC_FUNCTION_DEVICE_TYPE:
        if (frame->data_len >= 2U)
        {
            KNX_LOG_INFO("[KNX] DEVICE_TYPE raw=0x%04X\r\n",
                     (unsigned int)knx_read_be_u16(frame->data));
        }
        break;

    case KNX_BASIC_FUNCTION_WORLD_TIME:
        KNX_LOG_INFO("[KNX] WORLD_TIME len=%u\r\n",
                 (unsigned int)frame->data_len);
        break;

    default:
        break;
    }

    if (frame->data_len > 0U)
    {
        knx_log_data_hex(frame->data, frame->data_len);
    }
}

static void knx_log_device_function(const KNX_Frame_t *frame)
{
    /*
     * DEVICE:
     * - fun[0] = FUN1 (2)
     * - fun[1] = device_no
     * - fun[2] = device_type
     * - fun[3] = category (CONTROL=0 / CONFIG=1)
     * - fun[4] = item
     *
     * NOTE:
     * - Some device types define extra fun fields (fun[5]..fun[x]) as extensions.
     * - Data payload interpretation depends on device_type + category + item.
     */
    if (frame->fun_count < 5U)
    {
        KNX_LOG_WARN("[KNX] DEVICE frame requires FUN1..FUN5\r\n");
        if (frame->data_len > 0U)
        {
            knx_log_data_hex(frame->data, frame->data_len);
        }
        return;
    }

    KNX_LOG_INFO("[KNX] DEVICE no=%u type=%s(0x%02X) category=%s(0x%02X) item=%s(0x%02X)\r\n",
             (unsigned int)frame->fun[1],
             knx_get_device_type_name(frame->fun[2]),
             frame->fun[2],
             knx_get_device_category_name(frame->fun[3]),
             frame->fun[3],
             knx_get_device_item_name(frame->fun[2], frame->fun[3], frame->fun[4]),
             frame->fun[4]);

    if (frame->fun_count > 5U)
    {
        uint8_t extra_index;
        for (extra_index = 5U; extra_index < frame->fun_count; extra_index++)
        {
            KNX_LOG_INFO("[KNX] DEVICE extra_fun[%u]=0x%02X\r\n",
                     (unsigned int)extra_index,
                     frame->fun[extra_index]);
        }
    }

    /* 根据 device_type（fun3）选择数据解析/打印方式。 */
    switch (frame->fun[2])
    {
    case KNX_DEVICE_TYPE_AIR_CONDITIONER:
        if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL)
        {
            knx_summary_air_control(frame);
        }
        else if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG)
        {
            knx_summary_air_config(frame);
        }

        if ((frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL) &&
            ((frame->fun[4] == KNX_AIR_ITEM_SET_TEMPERATURE) ||
             (frame->fun[4] == KNX_AIR_ITEM_SET_TEMPERATURE_STATUS) ||
             (frame->fun[4] == KNX_AIR_ITEM_ACTUAL_TEMPERATURE)))
        {
            if (frame->fun[4] == KNX_AIR_ITEM_ACTUAL_TEMPERATURE)
            {
                knx_log_s16_tenths_value("TEMPERATURE", frame);
            }
            else
            {
                knx_log_u16_value("TEMPERATURE", frame);
            }
        }
        else if ((frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG) &&
                 (frame->fun[4] == KNX_AIR_CONFIG_DESCRIPTION))
        {
            knx_log_utf8_string("DESCRIPTION", frame);
        }
        else if ((frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG) &&
                 ((frame->fun[4] == KNX_AIR_CONFIG_TEMP_MIN) ||
                  (frame->fun[4] == KNX_AIR_CONFIG_TEMP_MAX)))
        {
            knx_log_u16_value("TEMPERATURE_LIMIT", frame);
        }
        else
        {
            knx_log_single_byte_value("VALUE", frame);
        }
        break;

    case KNX_DEVICE_TYPE_FLOOR_HEATING:
        if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL)
        {
            knx_summary_floor_heating_control(frame);
        }
        else if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG)
        {
            knx_summary_floor_heating_config(frame);
        }

        if ((frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL) &&
            ((frame->fun[4] == KNX_FLOOR_ITEM_SET_TEMPERATURE) ||
             (frame->fun[4] == KNX_FLOOR_ITEM_SET_TEMPERATURE_STATUS)))
        {
            knx_log_u16_value("TEMPERATURE", frame);
        }
        else if ((frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL) &&
                 (frame->fun[4] == KNX_FLOOR_ITEM_ACTUAL_TEMPERATURE))
        {
            knx_log_s16_tenths_value("TEMPERATURE", frame);
        }
        else if ((frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG) &&
                 (frame->fun[4] == KNX_FLOOR_CONFIG_DESCRIPTION))
        {
            knx_log_utf8_string("DESCRIPTION", frame);
        }
        else if ((frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG) &&
                 ((frame->fun[4] == KNX_FLOOR_CONFIG_TEMP_MIN) ||
                  (frame->fun[4] == KNX_FLOOR_CONFIG_TEMP_MAX)))
        {
            knx_log_u16_value("TEMPERATURE_LIMIT", frame);
        }
        else
        {
            knx_log_single_byte_value("VALUE", frame);
        }
        break;

    case KNX_DEVICE_TYPE_FRESH_AIR:
        if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL)
        {
            knx_summary_fresh_air_control(frame);
        }
        else if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG)
        {
            knx_summary_fresh_air_config(frame);
        }

        if ((frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG) &&
            (frame->fun[4] == KNX_FRESH_AIR_CONFIG_DESCRIPTION))
        {
            knx_log_utf8_string("DESCRIPTION", frame);
        }
        else
        {
            knx_log_single_byte_value("VALUE", frame);
        }
        break;

    case KNX_DEVICE_TYPE_KEY:
    case KNX_DEVICE_TYPE_AV:
        if (frame->fun[2] == KNX_DEVICE_TYPE_KEY)
        {
            if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL)
            {
                knx_summary_key_control(frame);
            }
            else if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG)
            {
                knx_summary_key_config(frame);
            }
        }

        if ((frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG) &&
            ((frame->fun[4] == KNX_KEY_CONFIG_DESCRIPTION) ||
             (frame->fun[4] == KNX_AV_CONFIG_DESCRIPTION)))
        {
            knx_log_utf8_string("DESCRIPTION", frame);
        }
        else
        {
            knx_log_single_byte_value("VALUE", frame);
        }
        break;

    case KNX_DEVICE_TYPE_SCENE:
        if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL)
        {
            knx_summary_scene_control(frame);
        }
        else if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG)
        {
            knx_summary_scene_config(frame);
        }

        if ((frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG) &&
            (frame->fun[4] == KNX_SCENE_CONFIG_DESCRIPTION))
        {
            knx_log_utf8_string("DESCRIPTION", frame);
        }
        else
        {
            knx_log_single_byte_value("VALUE", frame);
        }
        break;

    case KNX_DEVICE_TYPE_DIMMING:
        if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL)
        {
            knx_summary_dimming_control(frame);
        }
        else if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG)
        {
            knx_summary_dimming_config(frame);
        }

        if ((frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL) &&
            ((frame->fun[4] == KNX_DIMMING_ITEM_COLOR_TEMPERATURE) ||
             (frame->fun[4] == KNX_DIMMING_ITEM_COLOR_TEMPERATURE_STATUS) ||
             (frame->fun[4] == KNX_DIMMING_ITEM_COLOR_TEMP_PERCENT) ||
             (frame->fun[4] == KNX_DIMMING_ITEM_COLOR_TEMP_PERCENT_STATUS)))
        {
            knx_log_u16_value("COLOR_TEMPERATURE", frame);
        }
        else if ((frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG) &&
                 (frame->fun[4] == KNX_DIMMING_CONFIG_DESCRIPTION))
        {
            knx_log_utf8_string("DESCRIPTION", frame);
        }
        else if ((frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL) &&
                 ((frame->fun[4] == KNX_DIMMING_ITEM_RGBW) ||
                  (frame->fun[4] == KNX_DIMMING_ITEM_RGBW_STATUS)))
        {
            if (frame->data_len >= 6U)
            {
                KNX_LOG_INFO("[KNX] RGBW r=%u g=%u b=%u w=%u bri=%u mask=0x%02X\r\n",
                         (unsigned int)frame->data[0],
                         (unsigned int)frame->data[1],
                         (unsigned int)frame->data[2],
                         (unsigned int)frame->data[3],
                         (unsigned int)frame->data[4],
                         (unsigned int)frame->data[5]);
            }
        }
        else
        {
            knx_log_single_byte_value("VALUE", frame);
        }
        break;

    case KNX_DEVICE_TYPE_CURTAIN:
        if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL)
        {
            knx_summary_curtain_control(frame);
        }
        else if (frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG)
        {
            knx_summary_curtain_config(frame);
        }

        if ((frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG) &&
            (frame->fun[4] == KNX_CURTAIN_CONFIG_DESCRIPTION))
        {
            knx_log_utf8_string("DESCRIPTION", frame);
        }
        else
        {
            knx_log_single_byte_value("VALUE", frame);
        }
        break;

    case KNX_DEVICE_TYPE_VALUE_DISPLAY:
        if ((frame->fun[3] == KNX_DEVICE_CATEGORY_CONFIG) &&
            (frame->fun[4] == KNX_VALUE_DISPLAY_CONFIG_DESCRIPTION))
        {
            knx_log_utf8_string("DESCRIPTION", frame);
        }
        else if ((frame->fun[3] == KNX_DEVICE_CATEGORY_CONTROL) &&
                 (frame->fun[4] == KNX_VALUE_DISPLAY_ITEM_VALUE) &&
                 (frame->data_len >= 4U))
        {
            KNX_LOG_INFO("[KNX] VALUE_DISPLAY raw_u32=%lu raw_float=%f\r\n",
                     (unsigned long)knx_read_be_u32(frame->data),
                     (double)knx_read_be_float(frame->data));
        }
        else
        {
            knx_log_single_byte_value("VALUE", frame);
        }
        break;

    case KNX_DEVICE_TYPE_TOUCH_KEY:
        if (frame->data_len >= 2U)
        {
            KNX_LOG_INFO("[KNX] TOUCH_KEY bitmap=0x%04X\r\n",
                     (unsigned int)knx_read_be_u16(frame->data));
        }
        break;

    case KNX_DEVICE_TYPE_PUBLIC_DISPLAY:
        if ((frame->fun[4] == KNX_PUBLIC_DISPLAY_ITEM_TEMPERATURE) ||
            (frame->fun[4] == KNX_PUBLIC_DISPLAY_ITEM_TEMPERATURE_CALIBRATION))
        {
            knx_log_s16_tenths_value("TEMPERATURE", frame);
        }
        else if ((frame->fun[4] == KNX_PUBLIC_DISPLAY_ITEM_HUMIDITY) ||
                 (frame->fun[4] == KNX_PUBLIC_DISPLAY_ITEM_HUMIDITY_CALIBRATION))
        {
            knx_log_u16_value("HUMIDITY", frame);
        }
        else if ((frame->fun[4] == KNX_PUBLIC_DISPLAY_ITEM_AQI) ||
                 (frame->fun[4] == KNX_PUBLIC_DISPLAY_ITEM_ILLUMINANCE_CALIBRATION))
        {
            knx_log_u16_value("VALUE", frame);
        }
        else if (frame->data_len >= 4U)
        {
            KNX_LOG_INFO("[KNX] PUBLIC_DISPLAY raw_u32=%lu raw_float=%f\r\n",
                     (unsigned long)knx_read_be_u32(frame->data),
                     (double)knx_read_be_float(frame->data));
        }
        break;

    default:
        break;
    }

    if (frame->data_len > 0U)
    {
        knx_log_data_hex(frame->data, frame->data_len);
    }
}

static void knx_log_basic_setting(const KNX_Frame_t *frame)
{
    /*
     * BASIC_SETTING:
     * - fun[0] = FUN1 (3)
     * - fun[1] = FUN2 = setting item
     * - fun[2] = optional extension (e.g. function_type, sensor_no, page_button_index, ...)
     * - data payload interpretation depends on item
     */
    if (frame->fun_count < 2U)
    {
        KNX_LOG_WARN("[KNX] BASIC_SETTING requires FUN2\r\n");
        return;
    }

    KNX_LOG_INFO("[KNX] BASIC_SETTING item=%s(0x%02X)\r\n",
             knx_get_basic_setting_name(frame->fun[1]),
             frame->fun[1]);

    knx_summary_basic_setting(frame);

    /* 根据 BASIC_SETTING 的 item（fun2）选择数据解析/打印方式。 */
    switch (frame->fun[1])
    {
    case KNX_BASIC_SETTING_DEVICE_LOCK_CONTROL:
        if (frame->fun_count >= 3U)
        {
            KNX_LOG_INFO("[KNX] BASIC_SETTING function_type=%s(0x%02X)\r\n",
                     knx_get_basic_setting_function_type_name(frame->fun[2]),
                     frame->fun[2]);
        }
        if (frame->data_len >= 1U)
        {
            KNX_LOG_INFO("[KNX] BASIC_SETTING device_id=%u\r\n",
                     (unsigned int)frame->data[0]);
        }
        break;

    case KNX_BASIC_SETTING_PRESENCE_SENSOR_ENABLE:
        if (frame->fun_count >= 3U)
        {
            KNX_LOG_INFO("[KNX] BASIC_SETTING sensor_no=%u\r\n",
                     (unsigned int)frame->fun[2]);
        }
        knx_log_single_byte_value("VALUE", frame);
        break;

    case 17U:
        knx_log_single_byte_value("VALUE", frame);
        break;

    case KNX_BASIC_SETTING_SCREEN_PASSWORD:
        if (frame->fun_count >= 3U)
        {
            KNX_LOG_INFO("[KNX] BASIC_SETTING password_length=%u\r\n",
                     (unsigned int)frame->fun[2]);
        }
        if (frame->data_len > 0U)
        {
            knx_log_utf8_string("PASSWORD", frame);
        }
        break;

    case KNX_BASIC_SETTING_PAGE_BUTTON_FUNCTION:
        if (frame->fun_count >= 3U)
        {
            KNX_LOG_INFO("[KNX] BASIC_SETTING page_button_index=%u\r\n",
                     (unsigned int)frame->fun[2]);
        }
        if (frame->data_len >= 4U)
        {
            uint32_t raw = knx_read_be_u32(frame->data);
            KNX_LOG_INFO("[KNX] BASIC_SETTING mapped_device_type=%lu mapped_device_no=%lu raw=0x%08lX\r\n",
                     (unsigned long)(raw & 0xFFFFUL),
                     (unsigned long)((raw >> 16) & 0xFFFFUL),
                     (unsigned long)raw);
        }
        break;

    case KNX_BASIC_SETTING_KEY_BACKLIGHT_TIME:
    case KNX_BASIC_SETTING_SCREEN_OFF_TIME:
    case KNX_BASIC_SETTING_RETURN_HOME_TIME:
    case KNX_BASIC_SETTING_ENTER_SCREENSAVER_TIME:
        knx_log_u16_value("TIME", frame);
        break;

    default:
        knx_log_single_byte_value("VALUE", frame);
        break;
    }

    if (frame->data_len > 0U)
    {
        knx_log_data_hex(frame->data, frame->data_len);
    }
}

static void knx_log_config_frame(const KNX_Frame_t *frame)
{
    /* C1/C2 configuration frame (address + data_len + data). */
    KNX_LOG_INFO("[KNX] CONFIG address=0x%04X data_len=%u\r\n",
             (unsigned int)frame->address,
             (unsigned int)frame->data_len);

    if (frame->data_len > 0U)
    {
        knx_log_data_hex(frame->data, frame->data_len);
    }
}

static void knx_log_response_frame(const KNX_Frame_t *frame)
{
    /* C3 response frame, resp_type indicates ACK/HEARTBEAT/BUSY/ERROR. */
    KNX_LOG_INFO("[KNX] RESPONSE type=%s(0x%02X)\r\n",
             knx_get_response_type_name(frame->resp_type),
             frame->resp_type);
}

static void knx_log_single_byte_value(const char *label, const KNX_Frame_t *frame)
{
    /* Log 1-byte payload as unsigned value and hex. */
    if (frame->data_len >= 1U)
    {
        KNX_LOG_INFO("[KNX] %s=%u (0x%02X)\r\n",
                 label,
                 (unsigned int)frame->data[0],
                 (unsigned int)frame->data[0]);
    }
}

static void knx_log_u16_value(const char *label, const KNX_Frame_t *frame)
{
    /* Log 2-byte big-endian payload as uint16. */
    if (frame->data_len >= 2U)
    {
        KNX_LOG_INFO("[KNX] %s=%u raw=0x%04X\r\n",
                 label,
                 (unsigned int)knx_read_be_u16(frame->data),
                 (unsigned int)knx_read_be_u16(frame->data));
    }
}

static void knx_log_s16_tenths_value(const char *label, const KNX_Frame_t *frame)
{
    /* Log 2-byte big-endian payload as signed value and also scaled by 0.1. */
    if (frame->data_len >= 2U)
    {
        int16_t value = knx_read_be_s16(frame->data);
        KNX_LOG_INFO("[KNX] %s=%d (x0.1 => %.1f)\r\n",
                 label,
                 (int)value,
                 (double)value / 10.0);
    }
}

static void knx_log_utf8_string(const char *label, const KNX_Frame_t *frame)
{
    /*
     * Log payload bytes as a NUL-terminated string.
     * NOTE:
     * - The protocol documentation describes description strings as UTF-8.
     * - This helper assumes payload is printable and will stop only at data_len.
     */
    char text_buffer[KNX_MAX_DATA_LEN + 1U];
    uint16_t copy_len;

    if (frame->data_len == 0U)
    {
        return;
    }

    copy_len = frame->data_len;
    if (copy_len > KNX_MAX_DATA_LEN)
    {
        copy_len = KNX_MAX_DATA_LEN;
    }

    memcpy(text_buffer, frame->data, copy_len);
    text_buffer[copy_len] = '\0';
    KNX_LOG_INFO("[KNX] %s=%s\r\n", label, text_buffer);
}
#endif



#if KNX_LOG_ENABLE
void knx_summary_emit(const char *function_desc, const KNX_Frame_t *frame)
{
    if ((function_desc == NULL) || (frame == NULL))
    {
        return;
    }

    if (s_knx_summary_callback != NULL)
    {
        static char s_desc_buffer[128];
        (void)snprintf(s_desc_buffer, sizeof(s_desc_buffer), "%s", function_desc);
        s_knx_summary_callback(s_desc_buffer, frame->data, frame->data_len);
        return;
    }

    KNX_LOG_INFO("[KNX] SUMMARY: %s\r\n", function_desc);
    if (frame->data_len > 0U)
    {
        knx_log_data_hex(frame->data, frame->data_len);
    }
}
#endif


