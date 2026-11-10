#include "xiaomi_smart_screen_86box_buffer.h"
#include "xiaomi_smart_screen_86box_handle.h"
#include "hooch_protocol.h"

extern UART_HandleTypeDef huart2;

/* 模组版本 */
static unsigned char module_version;

/* 接收环形缓冲区与帧解析器 */
static XIAOMI_86BOX_RingBuffer_t rx_ring_buffer;
static XIAOMI_86BOX_FrameParser_t frame_parser;
static uint8_t frame_parser_timeout_ticks;

#define FRAME_PARSER_TIMEOUT_TICKS 5U   /* 主循环连续空转 5 次后复位解析器 */

/* ============================ 序号 ID 映射 ============================ */

/*
 * 序号ID(遥控分组)由全面屏的排列顺序决定, 这里按协议附表给出的固定顺序映射。
 * 屏幕排列发生变化时, 只需调整本函数。
 */
static uint8_t xiaomi_86box_seq_id(uint8_t device_type, uint8_t device_id)
{
    if ((device_type == XIAOMI_86BOX_DEV_WY_LIGHT) ||
        (device_type == XIAOMI_86BOX_DEV_RGB_LIGHT) ||
        (device_type == XIAOMI_86BOX_DEV_SINGLE_LIGHT) ||
        (device_type == XIAOMI_86BOX_DEV_WY_RGB_LIGHT))
    {
        return (uint8_t)(device_id + 2U);       /* 灯1~4 => 3~6 */
    }

    if (device_type == XIAOMI_86BOX_DEV_CURTAIN)
    {
        return (uint8_t)(device_id + 6U);       /* 窗帘1~3 => 7~9 */
    }

    if (device_type == XIAOMI_86BOX_DEV_AIR_CONDITIONER)
    {
        return 0x0BU;                            /* 空调 => 0x0B */
    }

    if ((device_type == XIAOMI_86BOX_DEV_RELAY_SWITCH) ||
        (device_type == XIAOMI_86BOX_DEV_86_SWITCH))
    {
        return device_id;                        /* 按键1~4 => 1~4 */
    }

    return 0U;
}

/* ============================ 上报发送函数 ============================ */

/* 空调/新风/地暖参数: VRF标识 | 网关地址 | 地址高 | 地址低 | 控制值 */
static uint8_t xiaomi_86box_send_hvac_value(uint8_t device_type,
                                            uint8_t device_id,
                                            uint8_t subcmd,
                                            uint8_t value)
{
    uint8_t params[5];

    params[0] = 1U;             /* VRF 与无线标示: 1-无线 */
    params[1] = 0U;             /* 网关地址 */
    params[2] = 0U;             /* 设备地址码高位 */
    params[3] = device_id;      /* 设备地址码低位 */
    params[4] = value;

    return xiaomi_86box_send_report(device_type, device_id,
                                    xiaomi_86box_seq_id(device_type, device_id),
                                    subcmd, params, sizeof(params));
}

/* 主命令级上报(0x81/0x82/0x85/0x86), 数据区直接为命令参数, 不带设备头 */
static uint8_t xiaomi_86box_send_command(uint8_t command,
                                         const uint8_t *params,
                                         uint16_t params_len)
{
    XIAOMI_86BOX_Frame_t frame;

    if (params_len > XIAOMI_86BOX_MAX_FRAME_DATA_LEN)
    {
        return 1U;
    }

    frame.header = XIAOMI_86BOX_FRAME_HEADER;
    frame.version = xiaomi_86box_get_version();
    frame.command = command;
    frame.data_len = params_len;

    if ((params != NULL) && (params_len > 0U))
    {
        memcpy(frame.data, params, params_len);
    }

    return xiaomi_86box_uart_send_frame(&frame);
}

/* 主动下发 0x83 重置模块: 数据 1=重置模块, 用于清配网/CLI 测试 */
uint8_t xiaomi_86box_send_reset_module(void)
{
    uint8_t params[1];

    params[0] = 1U;
    return xiaomi_86box_send_command((uint8_t)XIAOMI_86BOX_CMD_RESET_CONTROL,
                                     params, sizeof(params));
}

static HOOCH_PROTOCOL_KeyClickReportEvent_t xiaomi_86box_click_event_from_scene(
    HOOCH_PROTOCOL_SceneReportType_t type)
{
    switch (type)
    {
    case HOOCH_PROTOCOL_SCENE_REPORT_TYPE_SINGLE_CLICK:
        return HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_SINGLE_CLICK;
    case HOOCH_PROTOCOL_SCENE_REPORT_TYPE_DOUBLE_CLICK:
        return HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_DOUBLE_CLICK;
    case HOOCH_PROTOCOL_SCENE_REPORT_TYPE_LONG_CLICK:
        return HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_LONG_CLICK;
    default:
        return HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_NONE;
    }
}

static uint8_t xiaomi_86box_click_to_wire(HOOCH_PROTOCOL_KeyClickReportEvent_t event)
{
    switch (event)
    {
    case HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_SINGLE_CLICK: return 0U;
    case HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_DOUBLE_CLICK: return 1U;
    case HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_LONG_CLICK:
    case HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_LONG_CLICK_1P5S: return 2U;
    default: return 0xFFU;
    }
}

static uint8_t xiaomi_86box_fan_speed_to_wire(HOOCH_PROTOCOL_AirConditionerFanSpeed_t speed)
{
    switch (speed)
    {
    case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_AUTO:   return 1U;
    case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_LOW:    return 2U;
    case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_MEDIUM: return 3U;
    case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_HIGH:   return 4U;
    default: return 0U;
    }
}

static uint8_t xiaomi_86box_fresh_air_fan_speed_to_wire(
    HOOCH_PROTOCOL_FreshAirFanSpeed_t speed)
{
    switch (speed)
    {
    case HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_AUTO:   return 1U;
    case HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_LOW:    return 2U;
    case HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_MEDIUM: return 3U;
    case HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_HIGH:   return 4U;
    default: return 0U;
    }
}

/* ============================ 上报回调 ============================ */

static void xiaomi_86box_key_status_report_callback(
    const HOOCH_PROTOCOL_KeyStatusFrame_t *frame)
{
    uint8_t params[1];
    uint8_t key;

    if (frame == NULL)
    {
        return;
    }

    key = (uint8_t)frame->key;
    if ((key < 1U) || (key > 8U))
    {
        return;
    }

    params[0] = (frame->state == HOOCH_PROTOCOL_KEY_STATUS_STATE_ON) ? 1U : 0U;
    xiaomi_86box_cache_set(XIAOMI_86BOX_DEV_RELAY_SWITCH, key,
                           XIAOMI_86BOX_CACHE_SWITCH, params[0]);

    (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_RELAY_SWITCH, key, key,
                                   XIAOMI_86BOX_SWITCH_SWITCH, params, sizeof(params));
}

static void xiaomi_86box_key_click_report_callback(
    const HOOCH_PROTOCOL_KeyClickReportFrame_t *frame)
{
    uint8_t params[1];
    uint8_t device_type;
    uint8_t device_id;

    if (frame == NULL)
    {
        return;
    }

    params[0] = xiaomi_86box_click_to_wire(frame->event);
    if (params[0] == 0xFFU)
    {
        return;
    }

    if ((frame->key >= HOOCH_PROTOCOL_KEY_CLICK_REPORT_KEY_1) &&
        (frame->key <= HOOCH_PROTOCOL_KEY_CLICK_REPORT_KEY_4))
    {
        /* 按键 1~4: 智能开关(继电器) */
        device_type = XIAOMI_86BOX_DEV_RELAY_SWITCH;
        device_id = (uint8_t)frame->key;
    }
    else
    {
        /* 其余按键: 情景开关(1~12) */
        device_type = XIAOMI_86BOX_DEV_SCENE_SWITCH;
        device_id = (uint8_t)((uint8_t)frame->key - 4U);
    }

    (void)xiaomi_86box_send_report(device_type, device_id,
                                   xiaomi_86box_seq_id(device_type, device_id),
                                   XIAOMI_86BOX_SWITCH_EVENT, params, sizeof(params));
}

static void xiaomi_86box_scene_report_callback(
    const HOOCH_PROTOCOL_SceneReportFrame_t *frame)
{
    uint8_t params[1];
    HOOCH_PROTOCOL_KeyClickReportEvent_t event;

    if (frame == NULL)
    {
        return;
    }

    event = xiaomi_86box_click_event_from_scene(frame->type);
    if (event == HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_NONE)
    {
        return;
    }

    params[0] = xiaomi_86box_click_to_wire(event);
    (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_SCENE_SWITCH,
                                   (uint8_t)frame->key, 0U,
                                   XIAOMI_86BOX_SCENE_SWITCH_EVENT,
                                   params, sizeof(params));
}

static void xiaomi_86box_dimmer_light_report_callback(
    const HOOCH_PROTOCOL_DimmerLightFrame_t *frame)
{
    uint8_t params[2];
    uint8_t key;

    if (frame == NULL)
    {
        return;
    }

    key = (uint8_t)frame->key;
    if ((key < 1U) || (key > 4U))
    {
        return;
    }

    switch (frame->control_item)
    {
    case HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_SWITCH:
        params[0] = (frame->switch_state != 0U) ? 1U : 0U;
        xiaomi_86box_cache_set(XIAOMI_86BOX_DEV_WY_RGB_LIGHT, key,
                               XIAOMI_86BOX_CACHE_SWITCH, params[0]);
        (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_WY_RGB_LIGHT, key,
                                       xiaomi_86box_seq_id(XIAOMI_86BOX_DEV_WY_RGB_LIGHT, key),
                                       XIAOMI_86BOX_LIGHT_SWITCH, params, 1U);
        break;

    case HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_BRIGHTNESS:
    case HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_BRIGHTNESS_RAW:
        params[0] = (uint8_t)frame->brightness;
        xiaomi_86box_cache_set(XIAOMI_86BOX_DEV_WY_RGB_LIGHT, key,
                               XIAOMI_86BOX_CACHE_BRIGHTNESS, params[0]);
        (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_WY_RGB_LIGHT, key,
                                       xiaomi_86box_seq_id(XIAOMI_86BOX_DEV_WY_RGB_LIGHT, key),
                                       XIAOMI_86BOX_LIGHT_BRIGHTNESS, params, 1U);
        break;

    case HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_COLOR_TEMP:
    case HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_COLOR_TEMP_RAW:
        params[0] = (uint8_t)(frame->color_temperature >> 8);
        params[1] = (uint8_t)(frame->color_temperature);
        xiaomi_86box_cache_set(XIAOMI_86BOX_DEV_WY_RGB_LIGHT, key,
                               XIAOMI_86BOX_CACHE_COLOR_TEMP, frame->color_temperature);
        (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_WY_RGB_LIGHT, key,
                                       xiaomi_86box_seq_id(XIAOMI_86BOX_DEV_WY_RGB_LIGHT, key),
                                       XIAOMI_86BOX_LIGHT_COLOR_TEMP, params, 2U);
        break;

    case HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_ALL:
        params[0] = (frame->switch_state != 0U) ? 1U : 0U;
        (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_WY_RGB_LIGHT, key,
                                       xiaomi_86box_seq_id(XIAOMI_86BOX_DEV_WY_RGB_LIGHT, key),
                                       XIAOMI_86BOX_LIGHT_SWITCH, params, 1U);
        params[0] = (uint8_t)frame->brightness;
        (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_WY_RGB_LIGHT, key,
                                       xiaomi_86box_seq_id(XIAOMI_86BOX_DEV_WY_RGB_LIGHT, key),
                                       XIAOMI_86BOX_LIGHT_BRIGHTNESS, params, 1U);
        params[0] = (uint8_t)(frame->color_temperature >> 8);
        params[1] = (uint8_t)(frame->color_temperature);
        (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_WY_RGB_LIGHT, key,
                                       xiaomi_86box_seq_id(XIAOMI_86BOX_DEV_WY_RGB_LIGHT, key),
                                       XIAOMI_86BOX_LIGHT_COLOR_TEMP, params, 2U);
        break;

    default:
        break;
    }
}

static void xiaomi_86box_curtain_report_callback(
    const HOOCH_PROTOCOL_CurtainFrame_t *frame)
{
    uint8_t params[1];
    uint8_t key;

    if (frame == NULL)
    {
        return;
    }

    key = (uint8_t)frame->key;
    if ((key < 1U) || (key > 3U))
    {
        return;
    }

    switch (frame->control_item)
    {
    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_SWITCH:
        /* 0-打开 1-暂停 2-关闭 */
        params[0] = (frame->switch_status.value != 0U) ? 0U : 2U;
        xiaomi_86box_cache_set(XIAOMI_86BOX_DEV_CURTAIN, key,
                               XIAOMI_86BOX_CACHE_SWITCH,
                               (uint8_t)((params[0] == 0U) ? 1U : 0U));
        (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_CURTAIN, key,
                                       xiaomi_86box_seq_id(XIAOMI_86BOX_DEV_CURTAIN, key),
                                       XIAOMI_86BOX_CURTAIN_SWITCH, params, 1U);
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_STOP:
        params[0] = 1U;
        (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_CURTAIN, key,
                                       xiaomi_86box_seq_id(XIAOMI_86BOX_DEV_CURTAIN, key),
                                       XIAOMI_86BOX_CURTAIN_SWITCH, params, 1U);
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_PERCENT:
    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_PERCENT_RAW:
        params[0] = (uint8_t)frame->percent.value;
        xiaomi_86box_cache_set(XIAOMI_86BOX_DEV_CURTAIN, key,
                               XIAOMI_86BOX_CACHE_PERCENT, params[0]);
        xiaomi_86box_cache_set(XIAOMI_86BOX_DEV_CURTAIN, key,
                               XIAOMI_86BOX_CACHE_CURRENT_TRAVEL, params[0]);
        (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_CURTAIN, key,
                                       xiaomi_86box_seq_id(XIAOMI_86BOX_DEV_CURTAIN, key),
                                       XIAOMI_86BOX_CURTAIN_TRAVEL, params, 1U);
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ALL:
        params[0] = (frame->switch_status.value != 0U) ? 0U : 2U;
        (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_CURTAIN, key,
                                       xiaomi_86box_seq_id(XIAOMI_86BOX_DEV_CURTAIN, key),
                                       XIAOMI_86BOX_CURTAIN_SWITCH, params, 1U);
        params[0] = (uint8_t)frame->percent.value;
        (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_CURTAIN, key,
                                       xiaomi_86box_seq_id(XIAOMI_86BOX_DEV_CURTAIN, key),
                                       XIAOMI_86BOX_CURTAIN_TRAVEL, params, 1U);
        break;

    default:
        break;
    }
}

static void xiaomi_86box_air_conditioner_report_callback(
    const HOOCH_PROTOCOL_AirConditionerFrame_t *frame)
{
    uint8_t channel;

    if (frame == NULL)
    {
        return;
    }

    channel = (frame->channel != 0U) ? frame->channel : 1U;

    switch (frame->control_item)
    {
    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_POWER:
        xiaomi_86box_cache_set(XIAOMI_86BOX_DEV_AIR_CONDITIONER, channel,
                               XIAOMI_86BOX_CACHE_POWER, (uint16_t)frame->power);
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_AIR_CONDITIONER, channel,
                                           XIAOMI_86BOX_AC_POWER, (uint8_t)frame->power);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MODE:
        xiaomi_86box_cache_set(XIAOMI_86BOX_DEV_AIR_CONDITIONER, channel,
                               XIAOMI_86BOX_CACHE_MODE, (uint16_t)frame->mode);
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_AIR_CONDITIONER, channel,
                                           XIAOMI_86BOX_AC_MODE, (uint8_t)frame->mode);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_FAN_SPEED:
        xiaomi_86box_cache_set(XIAOMI_86BOX_DEV_AIR_CONDITIONER, channel,
                               XIAOMI_86BOX_CACHE_FAN_SPEED,
                               xiaomi_86box_fan_speed_to_wire(frame->fan_speed));
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_AIR_CONDITIONER, channel,
                                           XIAOMI_86BOX_AC_FAN_SPEED,
                                           xiaomi_86box_fan_speed_to_wire(frame->fan_speed));
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMPERATURE:
        xiaomi_86box_cache_set(XIAOMI_86BOX_DEV_AIR_CONDITIONER, channel,
                               XIAOMI_86BOX_CACHE_TEMPERATURE, frame->temperature);
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_AIR_CONDITIONER, channel,
                                           XIAOMI_86BOX_AC_TEMPERATURE,
                                           (uint8_t)frame->temperature);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL:
    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_XIAOMI:
    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_KNX:
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_AIR_CONDITIONER, channel,
                                           XIAOMI_86BOX_AC_POWER, (uint8_t)frame->power);
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_AIR_CONDITIONER, channel,
                                           XIAOMI_86BOX_AC_MODE, (uint8_t)frame->mode);
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_AIR_CONDITIONER, channel,
                                           XIAOMI_86BOX_AC_FAN_SPEED,
                                           xiaomi_86box_fan_speed_to_wire(frame->fan_speed));
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_AIR_CONDITIONER, channel,
                                           XIAOMI_86BOX_AC_TEMPERATURE,
                                           (uint8_t)frame->temperature);
        break;

    default:
        break;
    }
}

static void xiaomi_86box_fresh_air_report_callback(
    const HOOCH_PROTOCOL_FreshAirFrame_t *frame)
{
    uint8_t channel;
    uint8_t mode_wire;

    if (frame == NULL)
    {
        return;
    }

    channel = (frame->channel != 0U) ? frame->channel : 1U;
    mode_wire = (frame->mode == HOOCH_PROTOCOL_FRESH_AIR_MODE_BOOST) ? 1U : 0U;

    switch (frame->control_item)
    {
    case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_POWER:
        xiaomi_86box_cache_set(XIAOMI_86BOX_DEV_FRESH_AIR, channel,
                               XIAOMI_86BOX_CACHE_POWER, (uint16_t)frame->power);
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_FRESH_AIR, channel,
                                           XIAOMI_86BOX_FRESH_AIR_POWER, (uint8_t)frame->power);
        break;

    case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED:
        xiaomi_86box_cache_set(XIAOMI_86BOX_DEV_FRESH_AIR, channel,
                               XIAOMI_86BOX_CACHE_FAN_SPEED,
                               xiaomi_86box_fresh_air_fan_speed_to_wire(frame->fan_speed));
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_FRESH_AIR, channel,
                                           XIAOMI_86BOX_FRESH_AIR_FAN_SPEED,
                                           xiaomi_86box_fresh_air_fan_speed_to_wire(frame->fan_speed));
        break;

    case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_MODE:
        xiaomi_86box_cache_set(XIAOMI_86BOX_DEV_FRESH_AIR, channel,
                               XIAOMI_86BOX_CACHE_MODE, mode_wire);
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_FRESH_AIR, channel,
                                           XIAOMI_86BOX_FRESH_AIR_MODE, mode_wire);
        break;

    case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL:
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_FRESH_AIR, channel,
                                           XIAOMI_86BOX_FRESH_AIR_POWER, (uint8_t)frame->power);
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_FRESH_AIR, channel,
                                           XIAOMI_86BOX_FRESH_AIR_FAN_SPEED,
                                           xiaomi_86box_fresh_air_fan_speed_to_wire(frame->fan_speed));
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_FRESH_AIR, channel,
                                           XIAOMI_86BOX_FRESH_AIR_MODE, mode_wire);
        break;

    default:
        break;
    }
}

static void xiaomi_86box_floor_heating_report_callback(
    const HOOCH_PROTOCOL_FloorHeatingFrame_t *frame)
{
    uint8_t channel;

    if (frame == NULL)
    {
        return;
    }

    channel = (frame->channel != 0U) ? frame->channel : 1U;

    switch (frame->control_item)
    {
    case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_POWER:
        xiaomi_86box_cache_set(XIAOMI_86BOX_DEV_FLOOR_HEATING, channel,
                               XIAOMI_86BOX_CACHE_POWER, (uint16_t)frame->power);
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_FLOOR_HEATING, channel,
                                           XIAOMI_86BOX_FLOOR_HEATING_POWER, (uint8_t)frame->power);
        break;

    case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TARGET_TEMPERATURE:
        xiaomi_86box_cache_set(XIAOMI_86BOX_DEV_FLOOR_HEATING, channel,
                               XIAOMI_86BOX_CACHE_TEMPERATURE, frame->target_temperature);
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_FLOOR_HEATING, channel,
                                           XIAOMI_86BOX_FLOOR_HEATING_TEMP,
                                           (uint8_t)frame->target_temperature);
        break;

    case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL:
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_FLOOR_HEATING, channel,
                                           XIAOMI_86BOX_FLOOR_HEATING_POWER, (uint8_t)frame->power);
        (void)xiaomi_86box_send_hvac_value(XIAOMI_86BOX_DEV_FLOOR_HEATING, channel,
                                           XIAOMI_86BOX_FLOOR_HEATING_TEMP,
                                           (uint8_t)frame->target_temperature);
        break;

    default:
        break;
    }
}

static void xiaomi_86box_setting_report_callback(
    HOOCH_PROTOCOL_SettingReportEvent_t event,
    const void *data,
    uint16_t data_len)
{
    uint8_t params[4];

    switch (event)
    {
    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_NETWORK_JOIN:
        params[0] = 1U;     /* 配对成功 */
        (void)xiaomi_86box_send_command(XIAOMI_86BOX_CMD_PAIRING_STATUS, params, 1U);
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_NETWORK_LEAVE:
        params[0] = 0U;     /* 未配对 */
        (void)xiaomi_86box_send_command(XIAOMI_86BOX_CMD_PAIRING_STATUS, params, 1U);
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_RESET:
        params[0] = 1U;     /* 重置成功 */
        XIAOMI_86BOX_LOG_PRINTF("REPORT RESET -> 0x86\r\n");
        (void)xiaomi_86box_send_command(XIAOMI_86BOX_CMD_RESET_STATUS, params, 1U);
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_PRODUCTION_TEST:
        params[0] = 1U;     /* 产测成功 */
        (void)xiaomi_86box_send_command(XIAOMI_86BOX_CMD_PRODUCTION_STATUS, params, 1U);
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_HUMAN_PRESENCE:
        /* 0-无感应 1-触发无人 2-触发有人 */
        params[0] = ((data != NULL) && (((const uint8_t *)data)[0] != 0U)) ? 2U : 1U;
        (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_86PIR, 1U, 0U,
                                       XIAOMI_86BOX_RADAR_STATUS_REPORT, params, 1U);
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_GET_WEATHER:
    {
        uint32_t weather = xiaomi_86box_get_weather_code();
        params[0] = (uint8_t)(weather >> 24);
        params[1] = (uint8_t)(weather >> 16);
        params[2] = (uint8_t)(weather >> 8);
        params[3] = (uint8_t)(weather);
        (void)xiaomi_86box_send_report(XIAOMI_86BOX_DEV_BLE_BASIC, 1U, 0U,
                                       XIAOMI_86BOX_BLE_WEATHER_REPORT, params, 4U);
        break;
    }

    default:
        break;
    }
}

static void xiaomi_86box_key_name_report_callback(
    const HOOCH_PROTOCOL_KeyNameFrame_t *frame)
{
    uint8_t params[1U + HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH];
    uint8_t device_type;
    uint8_t device_id;
    uint16_t name_len;

    if (frame == NULL)
    {
        return;
    }

    switch (frame->page)
    {
    case HOOCH_PROTOCOL_KEY_NAME_PAGE_SWITCH: device_type = XIAOMI_86BOX_DEV_RELAY_SWITCH; break;
    case HOOCH_PROTOCOL_KEY_NAME_PAGE_SCENE:  device_type = XIAOMI_86BOX_DEV_SCENE_SWITCH; break;
    case HOOCH_PROTOCOL_KEY_NAME_PAGE_LIGHT:  device_type = XIAOMI_86BOX_DEV_WY_RGB_LIGHT; break;
    case HOOCH_PROTOCOL_KEY_NAME_PAGE_CURTAIN:device_type = XIAOMI_86BOX_DEV_CURTAIN;      break;
    default: return;
    }

    device_id = (uint8_t)frame->key;
    params[0] = (uint8_t)frame->icon_index;

    name_len = (uint16_t)strlen(frame->name);
    if (name_len > (HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH - 1U))
    {
        name_len = (uint16_t)(HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH - 1U);
    }

    if (name_len > 0U)
    {
        memcpy(&params[1], frame->name, name_len);
    }

    (void)xiaomi_86box_send_report(device_type, device_id,
                                   xiaomi_86box_seq_id(device_type, device_id),
                                   XIAOMI_86BOX_SUBCMD_REPORT_NAME,
                                   params, (uint16_t)(1U + name_len));
}

/* ============================ 环形缓冲区 ============================ */

static void ring_buffer_init(XIAOMI_86BOX_RingBuffer_t *rb)
{
    rb->head = 0U;
    rb->tail = 0U;
    rb->count = 0U;
    memset(rb->buffer, 0, sizeof(rb->buffer));
}

static uint8_t ring_buffer_push(XIAOMI_86BOX_RingBuffer_t *rb, uint8_t data)
{
    if (rb->count >= XIAOMI_86BOX_RING_BUFFER_SIZE)
    {
        return 1U;
    }

    rb->buffer[rb->head] = data;
    rb->head = (uint16_t)((rb->head + 1U) % XIAOMI_86BOX_RING_BUFFER_SIZE);
    rb->count++;
    return 0U;
}

static uint8_t ring_buffer_pop(XIAOMI_86BOX_RingBuffer_t *rb, uint8_t *data)
{
    if (rb->count == 0U)
    {
        return 1U;
    }

    *data = rb->buffer[rb->tail];
    rb->tail = (uint16_t)((rb->tail + 1U) % XIAOMI_86BOX_RING_BUFFER_SIZE);
    rb->count--;
    return 0U;
}

/* ============================ 帧解析器 ============================ */

static void frame_parser_init(XIAOMI_86BOX_FrameParser_t *parser)
{
    parser->state = XIAOMI_86BOX_FRAME_STATE_IDLE;
    parser->data_index = 0U;
    parser->calc_checksum = 0U;
    parser->frame_ready = 0U;
    memset(&parser->frame, 0, sizeof(parser->frame));
}

static uint8_t frame_parser_calc_checksum(const XIAOMI_86BOX_Frame_t *frame)
{
    uint32_t sum = 0U;
    uint16_t i;

    sum += (frame->header >> 8) & 0xFFU;
    sum += frame->header & 0xFFU;
    sum += frame->version;
    sum += frame->command;
    sum += (frame->data_len >> 8) & 0xFFU;
    sum += frame->data_len & 0xFFU;

    for (i = 0U; i < frame->data_len; i++)
    {
        sum += frame->data[i];
    }

    return (uint8_t)(sum % 256U);
}

static void frame_parser_process_byte(XIAOMI_86BOX_FrameParser_t *parser,
                                      uint8_t byte,
                                      XIAOMI_86BOX_Frame_t *out_frame)
{
    parser->frame_ready = 0U;

    switch (parser->state)
    {
    case XIAOMI_86BOX_FRAME_STATE_IDLE:
        if (byte == 0x55U)
        {
            parser->calc_checksum = byte;
            parser->state = XIAOMI_86BOX_FRAME_STATE_HEADER_H;
        }
        break;

    case XIAOMI_86BOX_FRAME_STATE_HEADER_H:
        if (byte == 0xAAU)
        {
            parser->calc_checksum += byte;
            parser->frame.header = XIAOMI_86BOX_FRAME_HEADER;
            parser->state = XIAOMI_86BOX_FRAME_STATE_VERSION;
        }
        else if (byte == 0x55U)
        {
            parser->calc_checksum = byte;
        }
        else
        {
            parser->state = XIAOMI_86BOX_FRAME_STATE_IDLE;
        }
        break;

    case XIAOMI_86BOX_FRAME_STATE_VERSION:
        parser->calc_checksum += byte;
        parser->frame.version = byte;
        parser->state = XIAOMI_86BOX_FRAME_STATE_CMD;
        break;

    case XIAOMI_86BOX_FRAME_STATE_CMD:
        parser->calc_checksum += byte;
        parser->frame.command = byte;
        parser->state = XIAOMI_86BOX_FRAME_STATE_DATA_LEN_H;
        break;

    case XIAOMI_86BOX_FRAME_STATE_DATA_LEN_H:
        parser->calc_checksum += byte;
        parser->frame.data_len = (uint16_t)((uint16_t)byte << 8);
        parser->state = XIAOMI_86BOX_FRAME_STATE_DATA_LEN_L;
        break;

    case XIAOMI_86BOX_FRAME_STATE_DATA_LEN_L:
        parser->calc_checksum += byte;
        parser->frame.data_len |= byte;

        if (parser->frame.data_len > XIAOMI_86BOX_MAX_FRAME_DATA_LEN)
        {
            parser->state = XIAOMI_86BOX_FRAME_STATE_IDLE;
        }
        else if (parser->frame.data_len == 0U)
        {
            parser->state = XIAOMI_86BOX_FRAME_STATE_CHECKSUM;
        }
        else
        {
            parser->data_index = 0U;
            parser->state = XIAOMI_86BOX_FRAME_STATE_DATA;
        }
        break;

    case XIAOMI_86BOX_FRAME_STATE_DATA:
        parser->calc_checksum += byte;
        parser->frame.data[parser->data_index] = byte;
        parser->data_index++;

        if (parser->data_index >= parser->frame.data_len)
        {
            parser->state = XIAOMI_86BOX_FRAME_STATE_CHECKSUM;
        }
        break;

    case XIAOMI_86BOX_FRAME_STATE_CHECKSUM:
        parser->frame.checksum = byte;

        if (parser->calc_checksum == parser->frame.checksum)
        {
            parser->frame_ready = 1U;
            if (out_frame != NULL)
            {
                memcpy(out_frame, &parser->frame, sizeof(XIAOMI_86BOX_Frame_t));
            }
        }
        else
        {
            XIAOMI_86BOX_LOG_WARN("[86BOX] checksum error\r\n");
        }

        parser->state = XIAOMI_86BOX_FRAME_STATE_IDLE;
        break;

    default:
        parser->state = XIAOMI_86BOX_FRAME_STATE_IDLE;
        break;
    }
}

/* ============================ 对外接口 ============================ */

#if XIAOMI_86BOX_LOG_RX_FRAME_ENABLE
/* 打印一整帧接收数据(十六进制) */
static void xiaomi_86box_log_rx_frame(const XIAOMI_86BOX_Frame_t *frame)
{
    uint16_t i;

    if (frame == NULL)
    {
        return;
    }

    XIAOMI_86BOX_LOG_RX_FRAME_BEGIN(XIAOMI_86BOX_FRAME_MIN_LEN + frame->data_len);
    XIAOMI_86BOX_LOG_RX_FRAME_BYTE((frame->header >> 8) & 0xFFU);
    XIAOMI_86BOX_LOG_RX_FRAME_BYTE(frame->header & 0xFFU);
    XIAOMI_86BOX_LOG_RX_FRAME_BYTE(frame->version);
    XIAOMI_86BOX_LOG_RX_FRAME_BYTE(frame->command);
    XIAOMI_86BOX_LOG_RX_FRAME_BYTE((frame->data_len >> 8) & 0xFFU);
    XIAOMI_86BOX_LOG_RX_FRAME_BYTE(frame->data_len & 0xFFU);

    for (i = 0U; i < frame->data_len; i++)
    {
        XIAOMI_86BOX_LOG_RX_FRAME_BYTE(frame->data[i]);
    }

    XIAOMI_86BOX_LOG_RX_FRAME_BYTE(frame->checksum);
    XIAOMI_86BOX_LOG_RX_FRAME_END();
}
#endif

void xiaomi_86box_uart_init(void)
{
    ring_buffer_init(&rx_ring_buffer);
    frame_parser_init(&frame_parser);
    frame_parser_timeout_ticks = 0U;

    HOOCH_PROTOCOL_KeyStatus_RegisterReportCallback(xiaomi_86box_key_status_report_callback);
    HOOCH_PROTOCOL_KeyClickReport_RegisterCallback(xiaomi_86box_key_click_report_callback);
    HOOCH_PROTOCOL_SceneReport_RegisterReportCallback4(xiaomi_86box_scene_report_callback);

    HOOCH_PROTOCOL_DimmerLight_RegisterReportCallback(xiaomi_86box_dimmer_light_report_callback);
    HOOCH_PROTOCOL_Curtain_RegisterReportCallback(xiaomi_86box_curtain_report_callback);
    HOOCH_PROTOCOL_AirConditioner_RegisterReportCallback(xiaomi_86box_air_conditioner_report_callback);
    HOOCH_PROTOCOL_FreshAir_RegisterReportCallback(xiaomi_86box_fresh_air_report_callback);
    HOOCH_PROTOCOL_FloorHeating_RegisterReportCallback(xiaomi_86box_floor_heating_report_callback);

    HOOCH_PROTOCOL_KeyName_RegisterCallback(xiaomi_86box_key_name_report_callback);
    HOOCH_PROTOCOL_SettingReport_RegisterCallback(xiaomi_86box_setting_report_callback);

    XIAOMI_86BOX_LOG_INFO("[86BOX] UART protocol initialized\r\n");
}

void xiaomi_86box_uart_process(void)
{
    uint8_t byte;
    uint8_t received_data = 0U;
    XIAOMI_86BOX_Frame_t parsed_frame;

    while (ring_buffer_pop(&rx_ring_buffer, &byte) == 0U)
    {
        /* 每弹出一个字节打印一次原始值 */
        XIAOMI_86BOX_LOG_RX_BYTE(byte);

        received_data = 1U;
        frame_parser_timeout_ticks = 0U;
        frame_parser_process_byte(&frame_parser, byte, &parsed_frame);



        if (frame_parser.frame_ready != 0U)
        {
            extern void hooch_cpu1_set_detected_protocol(uint8_t protocol);
            hooch_cpu1_set_detected_protocol(2U);

            module_version = parsed_frame.version;

#if XIAOMI_86BOX_LOG_RX_FRAME_ENABLE
            xiaomi_86box_log_rx_frame(&parsed_frame);
#endif

            XIAOMI_86BOX_LOG_DEBUG("[86BOX] frame cmd=0x%02X len=%u\r\n",
                                   (unsigned int)parsed_frame.command,
                                   (unsigned int)parsed_frame.data_len);

            xiaomi_86box_handle_frame(&parsed_frame);
        }
    }

    if (received_data == 0U)
    {
        if (frame_parser.state != XIAOMI_86BOX_FRAME_STATE_IDLE)
        {
            if (frame_parser_timeout_ticks < FRAME_PARSER_TIMEOUT_TICKS)
            {
                frame_parser_timeout_ticks++;
            }

            if (frame_parser_timeout_ticks >= FRAME_PARSER_TIMEOUT_TICKS)
            {
                frame_parser_init(&frame_parser);
                frame_parser_timeout_ticks = 0U;
            }
        }
        else
        {
            frame_parser_timeout_ticks = 0U;
        }
    }
    else if (frame_parser.state == XIAOMI_86BOX_FRAME_STATE_IDLE)
    {
        frame_parser_timeout_ticks = 0U;
    }
}

uint8_t xiaomi_86box_uart_send_frame(XIAOMI_86BOX_Frame_t *frame)
{
    uint8_t tx_buffer[XIAOMI_86BOX_MAX_FRAME_LEN];
    uint16_t tx_index = 0U;
    uint16_t i;

    if (frame == NULL)
    {
        return 1U;
    }

    if (frame->data_len > XIAOMI_86BOX_MAX_FRAME_DATA_LEN)
    {
        XIAOMI_86BOX_LOG_ERROR("[86BOX] frame data too long: %u\r\n",
                               (unsigned int)frame->data_len);
        return 1U;
    }

    tx_buffer[tx_index++] = (frame->header >> 8) & 0xFFU;
    tx_buffer[tx_index++] = frame->header & 0xFFU;
    tx_buffer[tx_index++] = frame->version;
    tx_buffer[tx_index++] = frame->command;
    tx_buffer[tx_index++] = (frame->data_len >> 8) & 0xFFU;
    tx_buffer[tx_index++] = frame->data_len & 0xFFU;

    for (i = 0U; i < frame->data_len; i++)
    {
        tx_buffer[tx_index++] = frame->data[i];
    }

    frame->checksum = frame_parser_calc_checksum(frame);
    tx_buffer[tx_index++] = frame->checksum;

    XIAOMI_86BOX_LOG_TX_HEX_BEGIN(tx_index);
    for (i = 0U; i < tx_index; i++)
    {
        XIAOMI_86BOX_LOG_TX_HEX_BYTE(tx_buffer[i]);
    }
    XIAOMI_86BOX_LOG_TX_HEX_END();

    if (XIAOMI_86BOX_TX_SEND(tx_buffer, tx_index) != HAL_OK)
    {
        XIAOMI_86BOX_LOG_PRINTF("TX FAILED\r\n");
        return 1U;
    }

    return 0U;
}

void xiaomi_86box_uart_rx_callback(uint8_t data)
{
    if (ring_buffer_push(&rx_ring_buffer, data) != 0U)
    {
        XIAOMI_86BOX_LOG_WARN("[86BOX] ring buffer overflow\r\n");
    }
}

uint8_t xiaomi_86box_get_version(void)
{
    return module_version;
}
