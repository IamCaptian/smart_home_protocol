#include "knx_protocol_uart.h"
#include "knx_handle.h"
#include "hooch_protocol.h"
#include "hooch_protocol_common.h"
#include <string.h>

extern UART_HandleTypeDef huart2;

#define KNX_FRAME_PARSER_TIMEOUT_TICKS 5U
#define KNX_ACK_FRAME_LEN              5U
#define KNX_MAX_TX_FRAME_LEN           (2U + 1U + 1U + KNX_MAX_FUN_COUNT + 1U + KNX_MAX_DATA_LEN + 1U)
typedef struct
{
    uint8_t buffer[KNX_RING_BUFFER_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} KNX_RingBuffer_t;

typedef enum
{
    KNX_FRAME_STATE_IDLE = 0,
    KNX_FRAME_STATE_HEADER_H,
    KNX_FRAME_STATE_COMMAND,
    KNX_FRAME_STATE_A1_FUN_COUNT,
    KNX_FRAME_STATE_A1_FUN,
    KNX_FRAME_STATE_A1_DATA_LEN,
    KNX_FRAME_STATE_A1_DATA,
    KNX_FRAME_STATE_A23_FUN_COUNT,
    KNX_FRAME_STATE_A23_FUN,
    KNX_FRAME_STATE_C12_ADDRESS_H,
    KNX_FRAME_STATE_C12_ADDRESS_L,
    KNX_FRAME_STATE_C12_DATA_LEN_H,
    KNX_FRAME_STATE_C12_DATA_LEN_L,
    KNX_FRAME_STATE_C1_DATA,
    KNX_FRAME_STATE_RESP_TYPE,
    KNX_FRAME_STATE_CHECKSUM,
} KNX_FrameState_t;

typedef struct
{
    KNX_FrameState_t state;
    KNX_Frame_t frame;
    uint16_t field_index;
    uint16_t data_index;
    uint8_t calc_checksum;
    uint8_t frame_ready;
} KNX_FrameParser_t;

static KNX_RingBuffer_t s_knx_rx_ring_buffer;
static KNX_FrameParser_t s_knx_frame_parser;
static uint8_t s_knx_parser_timeout_ticks;
static uint8_t s_knx_callback_suppress;

static void knx_setting_report_callback(
    HOOCH_PROTOCOL_SettingReportEvent_t event,
    const void *data,
    uint16_t data_len)
{
    uint8_t send_result;
    uint8_t request_value;
    uint8_t request_data[2];

    if (s_knx_callback_suppress != 0U)
    {
        return;
    }

    send_result = 0U;
    request_value = 1U;
    request_data[0] = 0U;
    request_data[1] = 0U;
    if ((data != 0) && (data_len >= 2U))
    {
        request_data[0] = ((const uint8_t *)data)[0];
        request_data[1] = ((const uint8_t *)data)[1];
    }

#if KNX_LOG_ENABLE
    KNX_LOG_INFO("[HOOCH] SettingReport event=%u, has_data=%u, data_len=%u\r\n",
                 (unsigned int)event,
                 (unsigned int)(data != 0),
                 (unsigned int)data_len);
#endif

    switch (event)
    {
    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_KNX_UPDATE_CONFIG:
        if ((data != 0) && (data_len >= 1U))
        {
            request_value = ((const uint8_t *)data)[0];
        }

        send_result = knx_send_basic_function(KNX_BASIC_FUNCTION_UPDATE_CONFIG,
                                              &request_value,
                                              1U);
        break;
    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_VERSION_INFO:        /* [7]  版本信息 */
        send_result = knx_send_basic_function(KNX_BASIC_FUNCTION_VERSION,
                                              request_data,
                                              2U);
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_HUMAN_PRESENCE:      /* [9]  人体存在 */
        send_result = knx_send_basic_function(KNX_BASIC_FUNCTION_HUMAN_PRESENCE_SENSOR,
                                              &request_value,
                                              1U);
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_UPDATE_CONFIG:       /* [12] 更新配置 */
        send_result = knx_send_basic_function(KNX_BASIC_FUNCTION_UPDATE_CONFIG,
                                              &request_value,
                                              1U);
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_PANEL_UPDATE_STATUS: /* [13] 面板更新状态 */
        send_result = knx_send_basic_function(KNX_BASIC_FUNCTION_PANEL_UPDATE_STATUS,
                                              &request_value,
                                              1U);
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_DEVICE_TYPE:         /* [14] 设备类型 */
        send_result = knx_send_basic_function(KNX_BASIC_FUNCTION_DEVICE_TYPE,
                                              request_data,
                                              2U);
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_PROGRAMMING_MODE:    /* [15] 编程模式 */
        send_result = knx_send_basic_function(KNX_BASIC_FUNCTION_PROGRAM_MODE,
                                              &request_value,
                                              1U);
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_TEMPERATURE_SENSOR:  /* [16] 温度传感器 */
        send_result = knx_send_basic_function(KNX_BASIC_FUNCTION_TEMPERATURE_SENSOR,
                                              request_data,
                                              2U);
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_HUMIDITY_SENSOR:     /* [17] 湿度传感器 */
        send_result = knx_send_basic_function(KNX_BASIC_FUNCTION_HUMIDITY_SENSOR,
                                              request_data,
                                              2U);

        break;


    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_KNX_HEARTBEAT:
        send_result = knx_uart_send_ack(KNX_RESPONSE_TYPE_HEARTBEAT);
        break;


    default:
        return;
    }

    if (send_result != 0U)
    {
        KNX_LOG_INFO("[HOOCH] SettingReport send failed, event=%u\r\n",
                     (unsigned int)event);
    }
}
static void knx_key_status_report_callback(
    const HOOCH_PROTOCOL_KeyStatusFrame_t *frame)
{
 KNX_LOG_INFO("=====key_status_report_callback=====key=%d, state=%d\r\n", frame->key, frame->state);
    uint8_t value;

    if ((frame == NULL) ||
        (s_knx_callback_suppress != 0U) ||
        (frame->key < HOOCH_PROTOCOL_KEY_STATUS_KEY_1) ||
        (frame->key > HOOCH_PROTOCOL_KEY_STATUS_KEY_8))
    {
        return;
    }

    value = (frame->state == HOOCH_PROTOCOL_KEY_STATUS_STATE_ON) ? 1U : 0U;
    (void)knx_send_device_function((uint8_t)(frame->key - HOOCH_PROTOCOL_KEY_STATUS_KEY_1),
                                   KNX_DEVICE_TYPE_KEY,
                                   KNX_DEVICE_CATEGORY_CONTROL,
                                   (uint8_t)KNX_KEY_ITEM_SWITCH,
                                   &value,
                                   1U);
}

static void knx_key_click_report_callback(
    const HOOCH_PROTOCOL_KeyClickReportFrame_t *frame)
{
    (void)frame;
    /* The current KNX protocol set in this project has no dedicated key-click event frame. */
}

static void knx_dimmer_light_callback(
    const HOOCH_PROTOCOL_DimmerLightFrame_t *frame)
{
    if ((frame == NULL) ||
        (s_knx_callback_suppress != 0U) ||
        (frame->key < HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_1) ||
        (frame->key > HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_8))
    {
        return;
    }
 KNX_LOG_INFO("=====dimmer_light_callback=====\r\n");
    switch (frame->control_item)
    {
    case HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_SWITCH:
        (void)knx_send_dimming_control_u8((uint8_t)(frame->key - HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_1),
                                          KNX_DIMMING_ITEM_SWITCH,
                                          frame->switch_state);
        break;

    case HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_BRIGHTNESS:
        (void)knx_send_dimming_control_u8((uint8_t)(frame->key - HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_1),
                                          KNX_DIMMING_ITEM_BRIGHTNESS,
                                          (uint8_t)((frame->brightness * 255U) / 100U));
        break;

    case HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_COLOR_TEMP:
        (void)knx_send_dimming_control_u16((uint8_t)(frame->key - HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_1),
                                          KNX_DIMMING_ITEM_COLOR_TEMPERATURE,
                                          (uint16_t)(((uint32_t)frame->color_temperature * 65535U) / 100U));
        break;

    default:
        break;
    }
}

static void knx_curtain_report_callback(
    const HOOCH_PROTOCOL_CurtainFrame_t *frame)
{
 KNX_LOG_INFO("=====curtain_report_callback=====\r\n");
    if ((frame == NULL) ||
        (s_knx_callback_suppress != 0U) ||
        (frame->key < HOOCH_PROTOCOL_CURTAIN_KEY_1) ||
        (frame->key > HOOCH_PROTOCOL_CURTAIN_KEY_8))
    {
        return;
    }
/*打印参数*/

    switch (frame->control_item)
    {
    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_SWITCH:
        (void)knx_send_curtain_control_u8((uint8_t)(frame->key - HOOCH_PROTOCOL_CURTAIN_KEY_1),
                                          KNX_CURTAIN_ITEM_POSITION_OPEN_CLOSE,
                                          frame->switch_status.value);
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_STOP:
        (void)knx_send_curtain_control_u8((uint8_t)(frame->key - HOOCH_PROTOCOL_CURTAIN_KEY_1),
                                          KNX_CURTAIN_ITEM_POSITION_STOP,
                                          frame->stop.value);
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_PERCENT:
        (void)knx_send_curtain_control_u8((uint8_t)(frame->key - HOOCH_PROTOCOL_CURTAIN_KEY_1),
                                          KNX_CURTAIN_ITEM_POSITION_PERCENT,
                                          (uint8_t)((frame->percent.value * 255U) / 100U));
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ANGLE:
        (void)knx_send_curtain_control_u8((uint8_t)(frame->key - HOOCH_PROTOCOL_CURTAIN_KEY_1),
                                          KNX_CURTAIN_ITEM_ANGLE_PERCENT,
                                          (uint8_t)((frame->angle.value * 255U) / 100U));
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ALL:
        // (void)knx_send_curtain_control_u8((uint8_t)(frame->key - HOOCH_PROTOCOL_CURTAIN_KEY_1),
        //                                   KNX_CURTAIN_ITEM_POSITION_OPEN_CLOSE,
        //                                   frame->switch_status.value);
        // (void)knx_send_curtain_control_u8((uint8_t)(frame->key - HOOCH_PROTOCOL_CURTAIN_KEY_1),
        //                                   KNX_CURTAIN_ITEM_POSITION_PERCENT,
        //                                   frame->percent.value);
        break;

    default:
        break;
    }
}

static void knx_scene_report_callback(
    const HOOCH_PROTOCOL_SceneReportFrame_t *frame)
{
    uint8_t value;

    if ((frame == NULL) ||
        (s_knx_callback_suppress != 0U) ||
        (frame->key < HOOCH_PROTOCOL_SCENE_REPORT_KEY_1) ||
        (frame->key > HOOCH_PROTOCOL_SCENE_REPORT_KEY_8))
    {
        return;
    }

    /* status: 1=触发, 0=非触发 */
    value = frame->valid;

    (void)knx_send_device_function((uint8_t)(frame->key - HOOCH_PROTOCOL_SCENE_REPORT_KEY_1),
                                   KNX_DEVICE_TYPE_SCENE,
                                   KNX_DEVICE_CATEGORY_CONTROL,
                                   (uint8_t)KNX_SCENE_ITEM_TRIGGER,
                                   &value,
                                   1U);
}

/* HOOCH 模式枚举映射到 KNX 模式值（knx_air_mode_to_hooch 的逆映射） */
static uint8_t hooch_air_mode_to_knx(HOOCH_PROTOCOL_AirConditionerMode_t mode)
{
    switch (mode)
    {
    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_COOL: return 1U;
    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_FAN:  return 4U;
    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_DRY:  return 3U;
    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_HEAT: return 2U;
    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_AUTO: return 5U;
    default:                                         return 0U;
    }
}

static void knx_air_conditioner_report_callback(
    const HOOCH_PROTOCOL_AirConditionerFrame_t *frame)
{
    uint8_t send_result;
    uint8_t channel;

    if ((frame == NULL) || (s_knx_callback_suppress != 0U))
    {
        return;
    }

    /* 通用协议通道从 1 开始，KNX 设备号从 0 开始；0 不参与减 1 */
    if (frame->channel > 0U)
    {
        channel = (uint8_t)(frame->channel - 1U);
    }
    else
    {
        channel = frame->channel;
    }

#if KNX_LOG_ENABLE
    KNX_LOG_INFO("[HOOCH] AirConditionerReport channel=%u, power=%u, mode=%u, fan_speed=%u, temperature=%u, control_item=%u\r\n",
                 (unsigned int)frame->channel,
                 (unsigned int)frame->power,
                 (unsigned int)frame->mode,
                 (unsigned int)frame->fan_speed,
                 (unsigned int)frame->temperature,
                 (unsigned int)frame->control_item);
#endif

    send_result = 0U;

    switch (frame->control_item)
    {
    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL:
        send_result |= knx_send_air_conditioner_control_u8(
            channel,
            KNX_AIR_ITEM_SWITCH,
            (uint8_t)frame->power);
        send_result |= knx_send_air_conditioner_control_u8(
            channel,
            KNX_AIR_ITEM_MODE,
            hooch_air_mode_to_knx(frame->mode));
        send_result |= knx_send_air_conditioner_control_u8(
            channel,
            KNX_AIR_ITEM_FAN_SPEED,
            (uint8_t)frame->fan_speed);
        send_result |= knx_send_air_conditioner_control_u16(
            channel,
            KNX_AIR_ITEM_SET_TEMPERATURE,
            (uint16_t)frame->temperature * 10U);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_POWER:
        send_result = knx_send_air_conditioner_control_u8(
            channel,
            KNX_AIR_ITEM_SWITCH,
            (uint8_t)frame->power);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MODE:
        send_result = knx_send_air_conditioner_control_u8(
            channel,
            KNX_AIR_ITEM_MODE,
            hooch_air_mode_to_knx(frame->mode));
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_FAN_SPEED:
        send_result = knx_send_air_conditioner_control_u8(
            channel,
            KNX_AIR_ITEM_FAN_SPEED,
            (uint8_t)frame->fan_speed);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMPERATURE:
        send_result = knx_send_air_conditioner_control_u16(
            channel,
            KNX_AIR_ITEM_SET_TEMPERATURE,
            (uint16_t)frame->temperature * 10U);
        break;

    default:
        return;
    }

    if (send_result != 0U)
    {
        KNX_LOG_INFO("[HOOCH] AirConditionerReport send failed, control_item=%u\r\n",
                     (unsigned int)frame->control_item);
    }
}

/* HOOCH 地暖模式映射到 KNX 手动/自动值（knx_floor_mode_to_hooch 的逆映射）。 */
static uint8_t hooch_floor_mode_to_knx(HOOCH_PROTOCOL_FloorHeatingMode_t mode)
{
    switch (mode)
    {
    case HOOCH_PROTOCOL_FLOOR_HEATING_MODE_MANUAL: return 0U;
    case HOOCH_PROTOCOL_FLOOR_HEATING_MODE_AUTO:   return 1U;
    default:                                       return 0xFFU;
    }
}

/* HOOCH 新风模式映射到 KNX FAN_SPEED_MODE 值（knx_fresh_air_mode_to_hooch 的逆映射）。 */
static uint8_t hooch_fresh_air_mode_to_knx(HOOCH_PROTOCOL_FreshAirMode_t mode)
{
    switch (mode)
    {
    case HOOCH_PROTOCOL_FRESH_AIR_MODE_AUTO: return 4U;
    default:                                 return 0xFFU;
    }
}

/* HOOCH 新风风速映射到 KNX FAN_SPEED_MODE 值（knx_fresh_air_speed_to_hooch 的逆映射）。 */
static uint8_t hooch_fresh_air_speed_to_knx(HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed)
{
    switch (fan_speed)
    {
    case HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_LOW:    return 1U;
    case HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_MEDIUM: return 2U;
    case HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_HIGH:   return 3U;
    case HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_AUTO:   return 4U;
    default:                                        return 0xFFU;
    }
}

static void knx_floor_heating_report_callback(
    const HOOCH_PROTOCOL_FloorHeatingFrame_t *frame)
{
    uint8_t send_result;
    uint8_t channel;
    uint8_t mode;

    if ((frame == NULL) || (s_knx_callback_suppress != 0U))
    {
        return;
    }

    /* 通用协议通道从 1 开始，KNX 设备号从 0 开始；0 不参与减 1 */
    if (frame->channel > 0U)
    {
        channel = (uint8_t)(frame->channel - 1U);
    }
    else
    {
        channel = frame->channel;
    }

#if KNX_LOG_ENABLE
    KNX_LOG_INFO("[HOOCH] FloorHeatingReport channel=%u, power=%u, mode=%u, target_temp=%u, control_item=%u\r\n",
                 (unsigned int)frame->channel,
                 (unsigned int)frame->power,
                 (unsigned int)frame->mode,
                 (unsigned int)frame->target_temperature,
                 (unsigned int)frame->control_item);
#endif

    send_result = 0U;

    switch (frame->control_item)
    {
    case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL:
        send_result |= knx_send_floor_heating_control_u8(
            channel,
            KNX_FLOOR_ITEM_SWITCH,
            (uint8_t)frame->power);

        mode = hooch_floor_mode_to_knx(frame->mode);
        if (mode != 0xFFU)
        {
            send_result |= knx_send_floor_heating_control_u8(
                channel,
                KNX_FLOOR_ITEM_MANUAL_AUTO,
                mode);
        }

        send_result |= knx_send_floor_heating_control_u16(
            channel,
            KNX_FLOOR_ITEM_SET_TEMPERATURE,
            (uint16_t)frame->target_temperature * 10U);
        break;

    case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_POWER:
        send_result = knx_send_floor_heating_control_u8(
            channel,
            KNX_FLOOR_ITEM_SWITCH,
            (uint8_t)frame->power);
        break;

    case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_MODE:
        mode = hooch_floor_mode_to_knx(frame->mode);
        if (mode == 0xFFU)
        {
            return;
        }
        send_result = knx_send_floor_heating_control_u8(
            channel,
            KNX_FLOOR_ITEM_MANUAL_AUTO,
            mode);
        break;

    case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TARGET_TEMPERATURE:
        send_result = knx_send_floor_heating_control_u16(
            channel,
            KNX_FLOOR_ITEM_SET_TEMPERATURE,
            (uint16_t)frame->target_temperature * 10U);
        break;

    case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_CURRENT_TEMPERATURE:
        send_result = knx_send_floor_heating_control_u16(
            channel,
            KNX_FLOOR_ITEM_ACTUAL_TEMPERATURE,
            (uint16_t)frame->current_temperature * 10U);
        break;

    default:
        return;
    }

    if (send_result != 0U)
    {
        KNX_LOG_INFO("[HOOCH] FloorHeatingReport send failed, control_item=%u\r\n",
                     (unsigned int)frame->control_item);
    }
}

static void knx_fresh_air_report_callback(
    const HOOCH_PROTOCOL_FreshAirFrame_t *frame)
{
    uint8_t send_result;
    uint8_t channel;
    uint8_t speed_mode;

    if ((frame == NULL) || (s_knx_callback_suppress != 0U))
    {
        return;
    }

    /* 通用协议通道从 1 开始，KNX 设备号从 0 开始；0 不参与减 1 */
    if (frame->channel > 0U)
    {
        channel = (uint8_t)(frame->channel - 1U);
    }
    else
    {
        channel = frame->channel;
    }

#if KNX_LOG_ENABLE
    KNX_LOG_INFO("[HOOCH] FreshAirReport channel=%u, power=%u, mode=%u, fan_speed=%u, control_item=%u\r\n",
                 (unsigned int)frame->channel,
                 (unsigned int)frame->power,
                 (unsigned int)frame->mode,
                 (unsigned int)frame->fan_speed,
                 (unsigned int)frame->control_item);
#endif

    send_result = 0U;

    switch (frame->control_item)
    {
    case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL:
        send_result |= knx_send_fresh_air_control_u8(
            channel,
            KNX_FRESH_AIR_ITEM_SWITCH,
            (uint8_t)frame->power);

        speed_mode = 0xFFU;
        if (frame->power == HOOCH_PROTOCOL_FRESH_AIR_POWER_OFF)
        {
            speed_mode = 0U;
        }
        else if (frame->mode == HOOCH_PROTOCOL_FRESH_AIR_MODE_AUTO)
        {
            speed_mode = 4U;
        }
        else
        {
            speed_mode = hooch_fresh_air_speed_to_knx(frame->fan_speed);
        }

        if (speed_mode != 0xFFU)
        {
            send_result |= knx_send_fresh_air_control_u8(
                channel,
                KNX_FRESH_AIR_ITEM_FAN_SPEED_MODE,
                speed_mode);
        }
        break;

    case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_POWER:
        send_result = knx_send_fresh_air_control_u8(
            channel,
            KNX_FRESH_AIR_ITEM_SWITCH,
            (uint8_t)frame->power);
        break;

    case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_MODE:
        speed_mode = hooch_fresh_air_mode_to_knx(frame->mode);
        if (speed_mode == 0xFFU)
        {
            return;
        }
        send_result = knx_send_fresh_air_control_u8(
            channel,
            KNX_FRESH_AIR_ITEM_FAN_SPEED_MODE,
            speed_mode);
        break;

    case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED:
        speed_mode = hooch_fresh_air_speed_to_knx(frame->fan_speed);
        if (speed_mode == 0xFFU)
        {
            return;
        }
        send_result = knx_send_fresh_air_control_u8(
            channel,
            KNX_FRESH_AIR_ITEM_FAN_SPEED_MODE,
            speed_mode);
        break;

    default:
        return;
    }

    if (send_result != 0U)
    {
        KNX_LOG_INFO("[HOOCH] FreshAirReport send failed, control_item=%u\r\n",
                     (unsigned int)frame->control_item);
    }
}

static void knx_ring_buffer_init(KNX_RingBuffer_t *rb)
{
    rb->head = 0U;
    rb->tail = 0U;
    rb->count = 0U;
    memset(rb->buffer, 0, sizeof(rb->buffer));
}

static uint8_t knx_ring_buffer_push(KNX_RingBuffer_t *rb, uint8_t data)
{
    if (rb->count >= KNX_RING_BUFFER_SIZE)
    {
        return 1U;
    }

    rb->buffer[rb->head] = data;
    rb->head = (uint16_t)((rb->head + 1U) % KNX_RING_BUFFER_SIZE);
    rb->count++;

    return 0U;
}

static uint8_t knx_ring_buffer_pop(KNX_RingBuffer_t *rb, uint8_t *data)
{
    if ((rb == NULL) || (data == NULL) || (rb->count == 0U))
    {
        return 1U;
    }

    *data = rb->buffer[rb->tail];
    rb->tail = (uint16_t)((rb->tail + 1U) % KNX_RING_BUFFER_SIZE);
    rb->count--;

    return 0U;
}

static void knx_frame_parser_init(KNX_FrameParser_t *parser)
{
    if (parser == NULL)
    {
        return;
    }

    parser->state = KNX_FRAME_STATE_IDLE;
    parser->field_index = 0U;
    parser->data_index = 0U;
    parser->calc_checksum = 0U;
    parser->frame_ready = 0U;
    memset(&parser->frame, 0, sizeof(parser->frame));
}

static void knx_frame_parser_restart_from_header(KNX_FrameParser_t *parser)
{
    knx_frame_parser_init(parser);
    parser->calc_checksum = KNX_FRAME_HEADER_H;
    parser->state = KNX_FRAME_STATE_HEADER_H;
}

static uint8_t knx_frame_should_ack(const KNX_Frame_t *frame)
{
    if (frame == NULL)
    {
        return 0U;
    }

    switch (frame->command)
    {
    case KNX_CMD_WRITE_DATA:
    case KNX_CMD_QUERY_DATA:
    case KNX_CMD_REQUEST_CONFIG:
    case KNX_CMD_WRITE_CONFIG:
    case KNX_CMD_READ_CONFIG:
        return 1U;

    default:
        return 0U;
    }
}

static uint8_t knx_frame_calc_checksum(const KNX_Frame_t *frame)
{
    uint32_t sum;
    uint16_t index;

    if (frame == NULL)
    {
        return 0U;
    }

    sum = 0U;
    sum += (uint8_t)((frame->header >> 8) & 0xFFU);
    sum += (uint8_t)(frame->header & 0xFFU);
    sum += frame->command;

    switch (frame->command)
    {
    case KNX_CMD_WRITE_DATA:
        sum += frame->fun_count;
        for (index = 0U; index < frame->fun_count; index++)
        {
            sum += frame->fun[index];
        }
        sum += (uint8_t)(frame->data_len & 0xFFU);
        for (index = 0U; index < frame->data_len; index++)
        {
            sum += frame->data[index];
        }
        break;

    case KNX_CMD_QUERY_DATA:
    case KNX_CMD_REQUEST_CONFIG:
        sum += frame->fun_count;
        for (index = 0U; index < frame->fun_count; index++)
        {
            sum += frame->fun[index];
        }
        break;

    case KNX_CMD_WRITE_CONFIG:
        sum += (uint8_t)((frame->address >> 8) & 0xFFU);
        sum += (uint8_t)(frame->address & 0xFFU);
        sum += (uint8_t)((frame->data_len >> 8) & 0xFFU);
        sum += (uint8_t)(frame->data_len & 0xFFU);
        for (index = 0U; index < frame->data_len; index++)
        {
            sum += frame->data[index];
        }
        break;

    case KNX_CMD_READ_CONFIG:
        sum += (uint8_t)((frame->address >> 8) & 0xFFU);
        sum += (uint8_t)(frame->address & 0xFFU);
        sum += (uint8_t)((frame->data_len >> 8) & 0xFFU);
        sum += (uint8_t)(frame->data_len & 0xFFU);
        break;

    case KNX_CMD_RESPONSE:
        sum += frame->resp_type;
        break;

    default:
        break;
    }

    return (uint8_t)(sum & 0xFFU);
}

static void knx_frame_parser_process_byte(KNX_FrameParser_t *parser, uint8_t byte, KNX_Frame_t *out_frame)
{
    parser->frame_ready = 0U;

    switch (parser->state)
    {
    case KNX_FRAME_STATE_IDLE:
        if (byte == KNX_FRAME_HEADER_H)
        {
            parser->calc_checksum = byte;
            parser->state = KNX_FRAME_STATE_HEADER_H;
        }
        break;

    case KNX_FRAME_STATE_HEADER_H:
        if (byte == KNX_FRAME_HEADER_L)
        {
            parser->calc_checksum = (uint8_t)(parser->calc_checksum + byte);
            parser->frame.header = KNX_FRAME_HEADER;
            parser->state = KNX_FRAME_STATE_COMMAND;
        }
        else if (byte == KNX_FRAME_HEADER_H)
        {
            parser->calc_checksum = byte;
            parser->state = KNX_FRAME_STATE_HEADER_H;
        }
        else
        {
            knx_frame_parser_init(parser);
        }
        break;

    case KNX_FRAME_STATE_COMMAND:
        parser->frame.command = byte;
        parser->calc_checksum = (uint8_t)(parser->calc_checksum + byte);
        parser->field_index = 0U;
        parser->data_index = 0U;
        parser->frame.fun_count = 0U;
        parser->frame.address = 0U;
        parser->frame.data_len = 0U;
        parser->frame.resp_type = 0U;
        memset(parser->frame.fun, 0, sizeof(parser->frame.fun));
        memset(parser->frame.data, 0, sizeof(parser->frame.data));

        switch (byte)
        {
        case KNX_CMD_WRITE_DATA:
            parser->state = KNX_FRAME_STATE_A1_FUN_COUNT;
            break;

        case KNX_CMD_QUERY_DATA:
        case KNX_CMD_REQUEST_CONFIG:
            parser->state = KNX_FRAME_STATE_A23_FUN_COUNT;
            break;

        case KNX_CMD_WRITE_CONFIG:
        case KNX_CMD_READ_CONFIG:
            parser->state = KNX_FRAME_STATE_C12_ADDRESS_H;
            break;

        case KNX_CMD_RESPONSE:
            parser->state = KNX_FRAME_STATE_RESP_TYPE;
            break;

        default:
            KNX_LOG_WARN("UNSUP_CMD %02X\r\n", (unsigned int)byte);
            if (byte == KNX_FRAME_HEADER_H)
            {
                knx_frame_parser_restart_from_header(parser);
            }
            else
            {
                knx_frame_parser_init(parser);
            }
            break;
        }
        break;

    case KNX_FRAME_STATE_A1_FUN_COUNT:
        parser->frame.fun_count = byte;
        parser->calc_checksum = (uint8_t)(parser->calc_checksum + byte);

        if (parser->frame.fun_count > KNX_MAX_FUN_COUNT)
        {
            KNX_LOG_WARN("BAD_FUN_CNT %u\r\n", (unsigned int)parser->frame.fun_count);
            knx_frame_parser_init(parser);
        }
        else if (parser->frame.fun_count == 0U)
        {
            parser->state = KNX_FRAME_STATE_A1_DATA_LEN;
        }
        else
        {
            parser->field_index = 0U;
            parser->state = KNX_FRAME_STATE_A1_FUN;
        }
        break;

    case KNX_FRAME_STATE_A1_FUN:
        parser->frame.fun[parser->field_index] = byte;
        parser->calc_checksum = (uint8_t)(parser->calc_checksum + byte);
        parser->field_index++;
        if (parser->field_index >= parser->frame.fun_count)
        {
            parser->state = KNX_FRAME_STATE_A1_DATA_LEN;
        }
        break;

    case KNX_FRAME_STATE_A1_DATA_LEN:
        parser->frame.data_len = byte;
        parser->calc_checksum = (uint8_t)(parser->calc_checksum + byte);
        if (parser->frame.data_len > KNX_MAX_DATA_LEN)
        {
            KNX_LOG_WARN("BAD_A1_LEN %u\r\n", (unsigned int)parser->frame.data_len);
            knx_frame_parser_init(parser);
        }
        else if (parser->frame.data_len == 0U)
        {
            parser->state = KNX_FRAME_STATE_CHECKSUM;
        }
        else
        {
            parser->data_index = 0U;
            parser->state = KNX_FRAME_STATE_A1_DATA;
        }
        break;

    case KNX_FRAME_STATE_A1_DATA:
        parser->frame.data[parser->data_index] = byte;
        parser->calc_checksum = (uint8_t)(parser->calc_checksum + byte);
        parser->data_index++;
        if (parser->data_index >= parser->frame.data_len)
        {
            parser->state = KNX_FRAME_STATE_CHECKSUM;
        }
        break;

    case KNX_FRAME_STATE_A23_FUN_COUNT:
        parser->frame.fun_count = byte;
        parser->calc_checksum = (uint8_t)(parser->calc_checksum + byte);

        if (parser->frame.fun_count > KNX_MAX_FUN_COUNT)
        {
            KNX_LOG_WARN("BAD_FUN_CNT %u\r\n", (unsigned int)parser->frame.fun_count);
            knx_frame_parser_init(parser);
        }
        else if (parser->frame.fun_count == 0U)
        {
            parser->state = KNX_FRAME_STATE_CHECKSUM;
        }
        else
        {
            parser->field_index = 0U;
            parser->state = KNX_FRAME_STATE_A23_FUN;
        }
        break;

    case KNX_FRAME_STATE_A23_FUN:
        parser->frame.fun[parser->field_index] = byte;
        parser->calc_checksum = (uint8_t)(parser->calc_checksum + byte);
        parser->field_index++;
        if (parser->field_index >= parser->frame.fun_count)
        {
            parser->state = KNX_FRAME_STATE_CHECKSUM;
        }
        break;

    case KNX_FRAME_STATE_C12_ADDRESS_H:
        parser->frame.address = (uint16_t)((uint16_t)byte << 8);
        parser->calc_checksum = (uint8_t)(parser->calc_checksum + byte);
        parser->state = KNX_FRAME_STATE_C12_ADDRESS_L;
        break;

    case KNX_FRAME_STATE_C12_ADDRESS_L:
        parser->frame.address |= byte;
        parser->calc_checksum = (uint8_t)(parser->calc_checksum + byte);
        parser->state = KNX_FRAME_STATE_C12_DATA_LEN_H;
        break;

    case KNX_FRAME_STATE_C12_DATA_LEN_H:
        parser->frame.data_len = (uint16_t)((uint16_t)byte << 8);
        parser->calc_checksum = (uint8_t)(parser->calc_checksum + byte);
        parser->state = KNX_FRAME_STATE_C12_DATA_LEN_L;
        break;

    case KNX_FRAME_STATE_C12_DATA_LEN_L:
        parser->frame.data_len |= byte;
        parser->calc_checksum = (uint8_t)(parser->calc_checksum + byte);
        if (parser->frame.data_len > KNX_MAX_DATA_LEN)
        {
            KNX_LOG_WARN("BAD_C_LEN %u\r\n", (unsigned int)parser->frame.data_len);
            knx_frame_parser_init(parser);
        }
        else if (parser->frame.command == KNX_CMD_WRITE_CONFIG)
        {
            if (parser->frame.data_len == 0U)
            {
                parser->state = KNX_FRAME_STATE_CHECKSUM;
            }
            else
            {
                parser->data_index = 0U;
                parser->state = KNX_FRAME_STATE_C1_DATA;
            }
        }
        else
        {
            parser->state = KNX_FRAME_STATE_CHECKSUM;
        }
        break;

    case KNX_FRAME_STATE_C1_DATA:
        parser->frame.data[parser->data_index] = byte;
        parser->calc_checksum = (uint8_t)(parser->calc_checksum + byte);
        parser->data_index++;
        if (parser->data_index >= parser->frame.data_len)
        {
            parser->state = KNX_FRAME_STATE_CHECKSUM;
        }
        break;

    case KNX_FRAME_STATE_RESP_TYPE:
        parser->frame.resp_type = byte;
        parser->calc_checksum = (uint8_t)(parser->calc_checksum + byte);
        parser->state = KNX_FRAME_STATE_CHECKSUM;
        break;

    case KNX_FRAME_STATE_CHECKSUM:
        parser->frame.checksum = byte;
        if (parser->calc_checksum == parser->frame.checksum)
        {
            parser->frame_ready = 1U;
            if (out_frame != NULL)
            {
                memcpy(out_frame, &parser->frame, sizeof(KNX_Frame_t));
            }
        }
        else
        {
            KNX_LOG_WARN("CHK_ERR c=%02X r=%02X cmd=%02X\r\n",
                         (unsigned int)parser->calc_checksum,
                         (unsigned int)parser->frame.checksum,
                         (unsigned int)parser->frame.command);
        }
        parser->state = KNX_FRAME_STATE_IDLE;
        parser->field_index = 0U;
        parser->data_index = 0U;
        parser->calc_checksum = 0U;
        memset(&parser->frame, 0, sizeof(parser->frame));
        break;

    default:
        knx_frame_parser_init(parser);
        break;
    }
}

void knx_uart_init(void)
{
 KNX_LOG_INFO("=====knx_uart_init\r\n");
    knx_ring_buffer_init(&s_knx_rx_ring_buffer);
    knx_frame_parser_init(&s_knx_frame_parser);
    s_knx_parser_timeout_ticks = 0U;
    s_knx_callback_suppress = 0U;
     HOOCH_PROTOCOL_KeyStatus_RegisterReportCallback(knx_key_status_report_callback);
     HOOCH_PROTOCOL_KeyClickReport_RegisterCallback(knx_key_click_report_callback);
     HOOCH_PROTOCOL_DimmerLight_RegisterReportCallback(knx_dimmer_light_callback);
     HOOCH_PROTOCOL_Curtain_RegisterReportCallback(knx_curtain_report_callback);
     HOOCH_PROTOCOL_SettingReport_RegisterCallback(knx_setting_report_callback);
     HOOCH_PROTOCOL_AirConditioner_RegisterReportCallback(knx_air_conditioner_report_callback);
     HOOCH_PROTOCOL_FloorHeating_RegisterReportCallback(knx_floor_heating_report_callback);
     HOOCH_PROTOCOL_FreshAir_RegisterReportCallback(knx_fresh_air_report_callback);



     
     HOOCH_PROTOCOL_SceneReport_RegisterReportCallback(knx_scene_report_callback);
     
    KNX_LOG_INFO("UART_INIT\r\n");
}

void knx_uart_process(void)
{
    uint8_t byte;
    uint8_t received_data;
    KNX_Frame_t parsed_frame;
    uint8_t rx_frame_bytes[KNX_MAX_TX_FRAME_LEN];
    uint16_t rx_frame_index;
    uint16_t log_index;
    uint8_t was_parsing;

    received_data = 0U;
    rx_frame_index = 0U;
    memset(&parsed_frame, 0, sizeof(parsed_frame));
           
    while (knx_ring_buffer_pop(&s_knx_rx_ring_buffer, &byte) == 0U)
    {
         
        received_data = 1U;
        s_knx_parser_timeout_ticks = 0U;
        was_parsing = (s_knx_frame_parser.state != KNX_FRAME_STATE_IDLE) ? 1U : 0U;
        knx_frame_parser_process_byte(&s_knx_frame_parser, byte, &parsed_frame);

        /* accumulate raw bytes for full-frame RX logging */
        if ((was_parsing != 0U) ||
            (s_knx_frame_parser.state != KNX_FRAME_STATE_IDLE) ||
            (s_knx_frame_parser.frame_ready != 0U))
        {
            if ((was_parsing == 0U) && (s_knx_frame_parser.state != KNX_FRAME_STATE_IDLE))
            {
                rx_frame_index = 0U; /* start of new frame */
            }
            if (rx_frame_index < sizeof(rx_frame_bytes))
            {
                rx_frame_bytes[rx_frame_index++] = byte;
            }
        }

        if (s_knx_frame_parser.frame_ready != 0U)
        {
            /* 整帧打印接收到的原始字节 */
            KNX_LOG_RX_FRAME_BEGIN(rx_frame_index);
            for (log_index = 0U; log_index < rx_frame_index; log_index++)
            {
                KNX_LOG_RX_FRAME_BYTE(rx_frame_bytes[log_index]);
            }
            KNX_LOG_RX_FRAME_END();
            rx_frame_index = 0U;

        extern void hooch_cpu1_set_detected_protocol(uint8_t protocol);
            hooch_cpu1_set_detected_protocol(3);
            KNX_LOG_DEBUG("RX cmd=%02X fn=%u dl=%u\r\n",
                          (unsigned int)parsed_frame.command,
                          (unsigned int)parsed_frame.fun_count,
                          (unsigned int)parsed_frame.data_len);

            s_knx_callback_suppress = 1U;
            knx_handle_frame(&parsed_frame);
            s_knx_callback_suppress = 0U;

            if (knx_frame_should_ack(&parsed_frame) != 0U)
            {
                (void)knx_uart_send_ack(KNX_RESPONSE_TYPE_ACK);
            }
        }
    }

    if (received_data == 0U)
    {
        if (s_knx_frame_parser.state != KNX_FRAME_STATE_IDLE)
        {
            if (s_knx_parser_timeout_ticks < KNX_FRAME_PARSER_TIMEOUT_TICKS)
            {
                s_knx_parser_timeout_ticks++;
            }

            if (s_knx_parser_timeout_ticks >= KNX_FRAME_PARSER_TIMEOUT_TICKS)
            {
                KNX_LOG_WARN("PARSER_TO\r\n");
                knx_frame_parser_init(&s_knx_frame_parser);
                s_knx_parser_timeout_ticks = 0U;
            }
        }
        else
        {
            s_knx_parser_timeout_ticks = 0U;
        }
    }
    else if (s_knx_frame_parser.state == KNX_FRAME_STATE_IDLE)
    {
        s_knx_parser_timeout_ticks = 0U;
    }
}

void knx_uart_rxcallback(uint8_t data)
{
    
    if (knx_ring_buffer_push(&s_knx_rx_ring_buffer, data) != 0U)
    {
        KNX_LOG_WARN("RB_OVF\r\n");
    }
}

uint8_t knx_uart_send_ack(uint8_t resp_type)
{
    KNX_Frame_t ack_frame;
    uint8_t tx_buffer[KNX_ACK_FRAME_LEN];
    uint8_t checksum;
    uint8_t tx_index;
    uint8_t log_index;

    memset(&ack_frame, 0, sizeof(ack_frame));
    ack_frame.header = KNX_FRAME_HEADER;
    ack_frame.command = KNX_CMD_RESPONSE;
    ack_frame.resp_type = resp_type;
    checksum = knx_frame_calc_checksum(&ack_frame);
    ack_frame.checksum = checksum;

    tx_index = 0U;
    tx_buffer[tx_index++] = KNX_FRAME_HEADER_H;
    tx_buffer[tx_index++] = KNX_FRAME_HEADER_L;
    tx_buffer[tx_index++] = ack_frame.command;
    tx_buffer[tx_index++] = ack_frame.resp_type;
    tx_buffer[tx_index++] = ack_frame.checksum;

    KNX_TX_SEND(tx_buffer, tx_index);

    KNX_LOG_TX_HEX_BEGIN(tx_index);
    for (log_index = 0U; log_index < tx_index; log_index++)
    {
        KNX_LOG_TX_HEX_BYTE(tx_buffer[log_index]);
    }
    KNX_LOG_TX_HEX_END();

    return 0U;
}

/* 把 16bit 数值按协议要求写成大端字节序。 */
static void knx_write_be_u16(uint8_t *buffer, uint16_t value)
{
    if (buffer == NULL)
    {
        return;
    }

    buffer[1] = (uint8_t)(value & 0xFFU);
    buffer[0] = (uint8_t)((value >> 8) & 0xFFU);
}

/* 把 KNX_Frame_t 序列化为可直接发送的原始字节流。 */
static uint8_t knx_uart_serialize_frame(const KNX_Frame_t *frame,
                                        uint8_t *tx_buffer,
                                        uint16_t *tx_len)
{
    uint16_t tx_index;
    uint16_t index;

    if ((frame == NULL) || (tx_buffer == NULL) || (tx_len == NULL))
    {
        return 1U;
    }

    tx_index = 0U;
    tx_buffer[tx_index++] = KNX_FRAME_HEADER_H;
    tx_buffer[tx_index++] = KNX_FRAME_HEADER_L;
    tx_buffer[tx_index++] = frame->command;

    switch (frame->command)
    {
    case KNX_CMD_WRITE_DATA:
        tx_buffer[tx_index++] = frame->fun_count;
        for (index = 0U; index < frame->fun_count; index++)
        {
            tx_buffer[tx_index++] = frame->fun[index];
        }

        tx_buffer[tx_index++] = (uint8_t)(frame->data_len & 0xFFU);
        for (index = 0U; index < frame->data_len; index++)
        {
            tx_buffer[tx_index++] = frame->data[index];
        }
        break;

    case KNX_CMD_QUERY_DATA:
    case KNX_CMD_REQUEST_CONFIG:
        tx_buffer[tx_index++] = frame->fun_count;
        for (index = 0U; index < frame->fun_count; index++)
        {
            tx_buffer[tx_index++] = frame->fun[index];
        }
        break;

    case KNX_CMD_WRITE_CONFIG:
        tx_buffer[tx_index++] = (uint8_t)((frame->address >> 8) & 0xFFU);
        tx_buffer[tx_index++] = (uint8_t)(frame->address & 0xFFU);
        tx_buffer[tx_index++] = (uint8_t)((frame->data_len >> 8) & 0xFFU);
        tx_buffer[tx_index++] = (uint8_t)(frame->data_len & 0xFFU);
        for (index = 0U; index < frame->data_len; index++)
        {
            tx_buffer[tx_index++] = frame->data[index];
        }
        break;

    case KNX_CMD_READ_CONFIG:
        tx_buffer[tx_index++] = (uint8_t)((frame->address >> 8) & 0xFFU);
        tx_buffer[tx_index++] = (uint8_t)(frame->address & 0xFFU);
        tx_buffer[tx_index++] = (uint8_t)((frame->data_len >> 8) & 0xFFU);
        tx_buffer[tx_index++] = (uint8_t)(frame->data_len & 0xFFU);
        break;

    case KNX_CMD_RESPONSE:
        tx_buffer[tx_index++] = frame->resp_type;
        break;

    default:
        return 1U;
    }

    tx_buffer[tx_index++] = frame->checksum;
    *tx_len = tx_index;

    return 0U;
}

/* 统一发送已经填充好的 KNX 帧。 */
static uint8_t knx_uart_send_frame(const KNX_Frame_t *frame)
{
    uint8_t tx_buffer[KNX_MAX_TX_FRAME_LEN];
    uint16_t tx_len;
    uint16_t index;

    if (knx_uart_serialize_frame(frame, tx_buffer, &tx_len) != 0U)
    {
        KNX_LOG_ERROR("SER_FAIL\r\n");
        return 1U;
    }

    KNX_TX_SEND(tx_buffer, tx_len);

    KNX_LOG_TX_HEX_BEGIN(tx_len);
    for (index = 0U; index < tx_len; index++)
    {
        KNX_LOG_TX_HEX_BYTE(tx_buffer[index]);
    }
    KNX_LOG_TX_HEX_END();

    KNX_LOG_DEBUG("TX cmd=%02X len=%u\r\n",
                  (unsigned int)frame->command,
                  (unsigned int)tx_len);

    return 0U;
}

/* 统一构建并发送 A1/A2/A3/C1/C2 帧。 */
static uint8_t knx_uart_send_business_frame(uint8_t command,
                                            uint8_t fun_count,
                                            const uint8_t *fun,
                                            uint16_t address,
                                            uint16_t data_len,
                                            const uint8_t *data)
{
    KNX_Frame_t frame;

    memset(&frame, 0, sizeof(frame));
    frame.header = KNX_FRAME_HEADER;
    frame.command = command;

    switch (command)
    {
    case KNX_CMD_WRITE_DATA:
        if ((fun == NULL) || (fun_count == 0U) || (fun_count > KNX_MAX_FUN_COUNT) ||
            (data_len > 0xFFU) || ((data_len > 0U) && (data == NULL)))
        {
            return 1U;
        }

        frame.fun_count = fun_count;
        (void)memcpy(frame.fun, fun, fun_count);
        frame.data_len = data_len;
        if (data_len > 0U)
        {
            (void)memcpy(frame.data, data, data_len);
        }
        break;

    case KNX_CMD_QUERY_DATA:
    case KNX_CMD_REQUEST_CONFIG:
        if ((fun == NULL) || (fun_count == 0U) || (fun_count > KNX_MAX_FUN_COUNT))
        {
            return 1U;
        }

        frame.fun_count = fun_count;
        (void)memcpy(frame.fun, fun, fun_count);
        break;

    case KNX_CMD_WRITE_CONFIG:
        if ((data_len > KNX_MAX_DATA_LEN) || ((data_len > 0U) && (data == NULL)))
        {
            return 1U;
        }

        frame.address = address;
        frame.data_len = data_len;
        if (data_len > 0U)
        {
            (void)memcpy(frame.data, data, data_len);
        }
        break;

    case KNX_CMD_READ_CONFIG:
        frame.address = address;
        frame.data_len = data_len;
        break;

    default:
        return 1U;
    }

    frame.checksum = knx_frame_calc_checksum(&frame);
    return knx_uart_send_frame(&frame);
}

/* 选择设备查询时使用的命令字。控制类走 A2，配置类走 A3。 */
static uint8_t knx_get_device_query_command(KNX_DeviceCategory_t category)
{
    return (uint8_t)((category == KNX_DEVICE_CATEGORY_CONFIG)
        ? KNX_CMD_REQUEST_CONFIG
        : KNX_CMD_QUERY_DATA);
}

uint8_t knx_uart_send_write_frame(uint8_t fun_count,
                                  const uint8_t *fun,
                                  uint16_t data_len,
                                  const uint8_t *data)
{
    return knx_uart_send_business_frame(KNX_CMD_WRITE_DATA,
                                        fun_count,
                                        fun,
                                        0U,
                                        data_len,
                                        data);
}

uint8_t knx_uart_send_query_frame(uint8_t fun_count,
                                  const uint8_t *fun)
{
    return knx_uart_send_business_frame(KNX_CMD_QUERY_DATA,
                                        fun_count,
                                        fun,
                                        0U,
                                        0U,
                                        NULL);
}

uint8_t knx_uart_send_request_config_frame(uint8_t fun_count,
                                           const uint8_t *fun)
{
    return knx_uart_send_business_frame(KNX_CMD_REQUEST_CONFIG,
                                        fun_count,
                                        fun,
                                        0U,
                                        0U,
                                        NULL);
}

uint8_t knx_uart_send_write_config(uint16_t address,
                                   uint16_t data_len,
                                   const uint8_t *data)
{
    return knx_uart_send_business_frame(KNX_CMD_WRITE_CONFIG,
                                        0U,
                                        NULL,
                                        address,
                                        data_len,
                                        data);
}

uint8_t knx_uart_send_read_config(uint16_t address,
                                  uint16_t data_len)
{
    return knx_uart_send_business_frame(KNX_CMD_READ_CONFIG,
                                        0U,
                                        NULL,
                                        address,
                                        data_len,
                                        NULL);
}

uint8_t knx_send_basic_function(KNX_BasicFunction_t item,
                                const uint8_t *data,
                                uint16_t data_len)
{
    uint8_t fun[2];

    fun[0] = (uint8_t)KNX_FUN1_BASIC_FUNCTION;
    fun[1] = (uint8_t)item;

    return knx_uart_send_write_frame(2U, fun, data_len, data);
}

uint8_t knx_query_basic_function(KNX_BasicFunction_t item)
{
    uint8_t fun[2];

    fun[0] = (uint8_t)KNX_FUN1_BASIC_FUNCTION;
    fun[1] = (uint8_t)item;

    return knx_uart_send_query_frame(2U, fun);
}

uint8_t knx_send_basic_setting(KNX_BasicSetting_t item,
                               uint8_t ext_fun3,
                               const uint8_t *data,
                               uint16_t data_len)
{
    uint8_t fun[3];
    uint8_t fun_count;

    fun[0] = (uint8_t)KNX_FUN1_BASIC_SETTING;
    fun[1] = (uint8_t)item;
    fun_count = 2U;

    if (ext_fun3 != 0U)
    {
        fun[2] = ext_fun3;
        fun_count = 3U;
    }

    return knx_uart_send_write_frame(fun_count, fun, data_len, data);
}

uint8_t knx_query_basic_setting(KNX_BasicSetting_t item,
                                uint8_t ext_fun3)
{
    uint8_t fun[3];
    uint8_t fun_count;

    fun[0] = (uint8_t)KNX_FUN1_BASIC_SETTING;
    fun[1] = (uint8_t)item;
    fun_count = 2U;

    if (ext_fun3 != 0U)
    {
        fun[2] = ext_fun3;
        fun_count = 3U;
    }

    return knx_uart_send_request_config_frame(fun_count, fun);
}

uint8_t knx_send_device_function(uint8_t device_no,
                                 KNX_DeviceType_t device_type,
                                 KNX_DeviceCategory_t category,
                                 uint8_t item,
                                 const uint8_t *data,
                                 uint16_t data_len)
{
    uint8_t fun[5];

    fun[0] = (uint8_t)KNX_FUN1_DEVICE;
    fun[1] = device_no;
    fun[2] = (uint8_t)device_type;
    fun[3] = (uint8_t)category;
    fun[4] = item;

    return knx_uart_send_write_frame(5U, fun, data_len, data);
}

uint8_t knx_query_device_function(uint8_t device_no,
                                  KNX_DeviceType_t device_type,
                                  KNX_DeviceCategory_t category,
                                  uint8_t item)
{
    uint8_t fun[5];
    uint8_t command;

    fun[0] = (uint8_t)KNX_FUN1_DEVICE;
    fun[1] = device_no;
    fun[2] = (uint8_t)device_type;
    fun[3] = (uint8_t)category;
    fun[4] = item;
    command = knx_get_device_query_command(category);

    if (command == KNX_CMD_REQUEST_CONFIG)
    {
        return knx_uart_send_request_config_frame(5U, fun);
    }

    return knx_uart_send_query_frame(5U, fun);
}

uint8_t knx_send_air_conditioner_control_u8(uint8_t device_no,
                                            KNX_AirItem_t item,
                                            uint8_t value)
{
    return knx_send_device_function(device_no,
                                    KNX_DEVICE_TYPE_AIR_CONDITIONER,
                                    KNX_DEVICE_CATEGORY_CONTROL,
                                    (uint8_t)item,
                                    &value,
                                    1U);
}

uint8_t knx_send_air_conditioner_control_u16(uint8_t device_no,
                                             KNX_AirItem_t item,
                                             uint16_t value)
{
    uint8_t data[2];

    knx_write_be_u16(data, value);
    return knx_send_device_function(device_no,
                                    KNX_DEVICE_TYPE_AIR_CONDITIONER,
                                    KNX_DEVICE_CATEGORY_CONTROL,
                                    (uint8_t)item,
                                    data,
                                    2U);
}

uint8_t knx_send_air_conditioner_config_u8(uint8_t device_no,
                                           KNX_AirConfigItem_t item,
                                           uint8_t value)
{
    return knx_send_device_function(device_no,
                                    KNX_DEVICE_TYPE_AIR_CONDITIONER,
                                    KNX_DEVICE_CATEGORY_CONFIG,
                                    (uint8_t)item,
                                    &value,
                                    1U);
}

uint8_t knx_send_air_conditioner_config_u16(uint8_t device_no,
                                            KNX_AirConfigItem_t item,
                                            uint16_t value)
{
    uint8_t data[2];

    knx_write_be_u16(data, value);
    return knx_send_device_function(device_no,
                                    KNX_DEVICE_TYPE_AIR_CONDITIONER,
                                    KNX_DEVICE_CATEGORY_CONFIG,
                                    (uint8_t)item,
                                    data,
                                    2U);
}

uint8_t knx_send_floor_heating_control_u8(uint8_t device_no,
                                          KNX_FloorHeatingItem_t item,
                                          uint8_t value)
{
    return knx_send_device_function(device_no,
                                    KNX_DEVICE_TYPE_FLOOR_HEATING,
                                    KNX_DEVICE_CATEGORY_CONTROL,
                                    (uint8_t)item,
                                    &value,
                                    1U);
}

uint8_t knx_send_floor_heating_control_u16(uint8_t device_no,
                                           KNX_FloorHeatingItem_t item,
                                           uint16_t value)
{
    uint8_t data[2];

    knx_write_be_u16(data, value);
    return knx_send_device_function(device_no,
                                    KNX_DEVICE_TYPE_FLOOR_HEATING,
                                    KNX_DEVICE_CATEGORY_CONTROL,
                                    (uint8_t)item,
                                    data,
                                    2U);
}

uint8_t knx_send_fresh_air_control_u8(uint8_t device_no,
                                      KNX_FreshAirItem_t item,
                                      uint8_t value)
{
    return knx_send_device_function(device_no,
                                    KNX_DEVICE_TYPE_FRESH_AIR,
                                    KNX_DEVICE_CATEGORY_CONTROL,
                                    (uint8_t)item,
                                    &value,
                                    1U);
}

uint8_t knx_send_dimming_control_u8(uint8_t device_no,
                                    KNX_DimmingItem_t item,
                                    uint8_t value)
{
    return knx_send_device_function(device_no,
                                    KNX_DEVICE_TYPE_DIMMING,
                                    KNX_DEVICE_CATEGORY_CONTROL,
                                    (uint8_t)item,
                                    &value,
                                    1U);
}

uint8_t knx_send_dimming_control_u16(uint8_t device_no,
                                     KNX_DimmingItem_t item,
                                     uint16_t value)
{
    uint8_t data[2];

    knx_write_be_u16(data, value);
    return knx_send_device_function(device_no,
                                    KNX_DEVICE_TYPE_DIMMING,
                                    KNX_DEVICE_CATEGORY_CONTROL,
                                    (uint8_t)item,
                                    data,
                                    2U);
}

uint8_t knx_send_curtain_control_u8(uint8_t device_no,
                                    KNX_CurtainItem_t item,
                                    uint8_t value)
{
    return knx_send_device_function(device_no,
                                    KNX_DEVICE_TYPE_CURTAIN,
                                    KNX_DEVICE_CATEGORY_CONTROL,
                                    (uint8_t)item,
                                    &value,
                                    1U);
}
