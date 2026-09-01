#include "xiaomi_smart_screen_circular_bufferc.h"
#include "xiaomi_smart_screen_handle.h"
#include "hooch_protocol.h"
#include "hooch_protocol_common.h"
#include <stdio.h>

extern UART_HandleTypeDef huart2;

/* 记录模组版本 */
static unsigned char module_version;
/* 全局变量 */
static RingBuffer_t rx_ring_buffer; /* 接收环形缓冲区 */
static FrameParser_t frame_parser;  /* 帧解析器 */
static uint8_t frame_parser_timeout_ticks; /* 半包超时计数 */

#define FRAME_PARSER_TIMEOUT_TICKS 5U /* 主循环连续空转5次后复位解析器 */
#define XIAOMI_SMART_SCREEN_NET_LEAVE_COMMAND 0x04U

static volatile uint8_t s_xiaomi_switch_control_pending = 0U;
static uint8_t s_xiaomi_switch_control_bits = 0U;
/*开关状态上报*/
static uint8_t xiaoni_smart_screen_send_x88_switch_control_report(uint8_t switch_bits)
{
    Frame_t frame;

    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();
    frame.command = XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL;
    frame.data_len = 4U;

    frame.data[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
    frame.data[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
    frame.data[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_SWITCH_CONTROL;
    frame.data[3] = switch_bits;

    return xiaoni_smart_screen_uart_sendframe(&frame);
}

/* 一次性更新全部4路开关状态（仅更新内部状态，不上报模组），用于handle层SWITCH_CONTROL场景 */
void xiaoni_smart_screen_switch_control_update(uint8_t switch_bits)
{
    s_xiaomi_switch_control_bits = switch_bits;
}

/*开关页调光开关状态上报*/
static uint8_t xiaoni_smart_screen_send_x88_switch_dimmer_report_report(HOOCH_PROTOCOL_DimmerLightKey_t key, uint8_t state)
{
    Frame_t frame;

    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();
    frame.command = XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL;
    frame.data_len = 5U;

    frame.data[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
    frame.data[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
    frame.data[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DIMMING_COLOR_CURTAIN_SWITCH_STATUS;
    frame.data[3] = key; /*页面 255*/
    frame.data[4] = state;
    return xiaoni_smart_screen_uart_sendframe(&frame);
}
/*灯光页面调光开关状态上报*/
static uint8_t xiaoni_smart_screen_send_x88_light_dimmer_report_report(uint8_t page,
     HOOCH_PROTOCOL_DimmerLightKey_t key, 
     uint8_t state)
{
    Frame_t frame;

    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();
    frame.command = XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL;
    frame.data_len = 7U;

    frame.data[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
    frame.data[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
    frame.data[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_BRIGHTNESS_COLOR_TEMP_TRAVEL_STATUS_EXT;
    frame.data[3] = page; /*页面 255*/
    frame.data[4] = key; /*页面 255*/
    frame.data[5] = 0;  /*灯光开关*/
    frame.data[6] = state;
    return xiaoni_smart_screen_uart_sendframe(&frame);
}


/*开关页调光亮度上报*/
static uint8_t xiaoni_smart_screen_send_x88_switch_dimmer_brightness_report(HOOCH_PROTOCOL_DimmerLightKey_t key, uint8_t brightness)
{
    Frame_t frame;

    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();
    frame.command = XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL;
    frame.data_len = 6U;

    frame.data[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
    frame.data[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
    frame.data[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_BRIGHTNESS_COLOR_TEMP_TRAVEL_STATUS;
    frame.data[3] = key;
    frame.data[4] = 0x01;/*亮度*/
    frame.data[5] = brightness;
    return xiaoni_smart_screen_uart_sendframe(&frame);
}
/*灯光页面调光亮度上报*/
static uint8_t xiaoni_smart_screen_send_x88_light_dimmer_brightness_report(uint8_t page,
     HOOCH_PROTOCOL_DimmerLightKey_t key, 
     uint8_t brightness)
{
    Frame_t frame;

    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();
    frame.command = XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL;
    frame.data_len = 7U;

    frame.data[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
    frame.data[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
    frame.data[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_BRIGHTNESS_COLOR_TEMP_TRAVEL_STATUS_EXT;
    frame.data[3] = page; /*页面 255*/
    frame.data[4] = key; /*页面 255*/
    frame.data[5] = 1;  /*灯光亮度*/
    frame.data[6] = brightness;
    return xiaoni_smart_screen_uart_sendframe(&frame);
}
/*开关页调光色温上报*/
static uint8_t xiaoni_smart_screen_send_x88_switch_dimmer_temperature_report(HOOCH_PROTOCOL_DimmerLightKey_t key, uint8_t temperature)
{
    Frame_t frame;

    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();
    frame.command = XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL;
    frame.data_len = 6U;

    frame.data[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
    frame.data[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
    frame.data[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_BRIGHTNESS_COLOR_TEMP_TRAVEL_STATUS;
    frame.data[3] = key;
    frame.data[4] = 0x02;/*色温*/
    frame.data[5] = temperature;
    return xiaoni_smart_screen_uart_sendframe(&frame);
}
/*灯光页面调光色温上报*/
static uint8_t xiaoni_smart_screen_send_x88_light_dimmer_temperature_report(uint8_t page,
     HOOCH_PROTOCOL_DimmerLightKey_t key, 
     uint8_t temperature)
{
    Frame_t frame;

    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();
    frame.command = XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL;
    frame.data_len = 7U;

    frame.data[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
    frame.data[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
    frame.data[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_BRIGHTNESS_COLOR_TEMP_TRAVEL_STATUS_EXT;
    frame.data[3] = page; /*页面 255*/
    frame.data[4] = key; /*页面 255*/
    frame.data[5] = 2;  /*灯光色温*/
    frame.data[6] = temperature;
    return xiaoni_smart_screen_uart_sendframe(&frame);
}
/*开关页窗帘开关停上报*/
static uint8_t xiaoni_smart_screen_send_x88_switch_curtain_switch_report(HOOCH_PROTOCOL_KeyStatusKey_t key, uint8_t state)
{
    Frame_t frame;

    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();
    frame.command = XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL;
    frame.data_len = 5U;

    frame.data[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
    frame.data[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
    frame.data[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DIMMING_COLOR_CURTAIN_SWITCH_STATUS;
    frame.data[3] = key;
    frame.data[4] = state;
    return xiaoni_smart_screen_uart_sendframe(&frame);
}
/*窗帘页页窗帘开关停上报*/
static uint8_t xiaoni_smart_screen_send_x88_curtain_switch_report(uint8_t page,
     HOOCH_PROTOCOL_KeyStatusKey_t key, 
     uint8_t state)
{
    Frame_t frame;

    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();
    frame.command = XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL;
    frame.data_len = 7U;

    frame.data[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
    frame.data[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
    frame.data[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_BRIGHTNESS_COLOR_TEMP_TRAVEL_STATUS_EXT;
    frame.data[3] = page; /*页面 255*/
    frame.data[4] = key; /*页面 255*/
    frame.data[5] = (state == 1U) ? 9U : ((state == 0U) ? 10U : 11U);  /*窗帘开关停: 关->10, 开->9, 停->11*/
    frame.data[6] = 0U;
    return xiaoni_smart_screen_uart_sendframe(&frame);
}
/*窗帘行程*/
static uint8_t xiaoni_smart_screen_send_x88_curtain_travel_report(HOOCH_PROTOCOL_KeyStatusKey_t key, uint8_t travel)
{
    Frame_t frame;
    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();
    frame.command = XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL;
    frame.data_len = 6U;

    frame.data[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
    frame.data[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
    frame.data[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_BRIGHTNESS_COLOR_TEMP_TRAVEL_STATUS;
    frame.data[3] = key;
    frame.data[4] = 0x03;
    frame.data[5] = travel;
    return xiaoni_smart_screen_uart_sendframe(&frame);
}
/*窗帘页窗帘行程上报*/
static uint8_t xiaoni_smart_screen_send_x88_curtain_page_travel_report(uint8_t page,
     HOOCH_PROTOCOL_KeyStatusKey_t key, 
     uint8_t travel)
{
    Frame_t frame;

    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();
    frame.command = XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL;
    frame.data_len = 7U;

    frame.data[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
    frame.data[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
    frame.data[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_BRIGHTNESS_COLOR_TEMP_TRAVEL_STATUS_EXT;
    frame.data[3] = page; /*页面 255*/
    frame.data[4] = key; /*页面 255*/
    frame.data[5] = 3;  /*窗帘行程*/
    frame.data[6] = travel;
    return xiaoni_smart_screen_uart_sendframe(&frame);
}
/*按键点击事件*/
static uint8_t xiaoni_smart_screen_send_x88_key_click_report(HOOCH_PROTOCOL_KeyClickReportKey_t key, HOOCH_PROTOCOL_KeyClickReportEvent_t click_type)
{
    Frame_t frame;
    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();
    frame.command = XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL;
    frame.data_len = 5U;

    frame.data[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
    frame.data[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
    frame.data[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_EVENT_STATUS;
    frame.data[3] = key;
    switch (click_type)
    {
    case HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_SINGLE_CLICK:
        frame.data[4] = 0x00;//单击
        break;
    case HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_DOUBLE_CLICK:
        frame.data[4] = 0x01;//双击
        break;
    case HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_LONG_CLICK:
    case HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_LONG_CLICK_1P5S:
        frame.data[4] = 0x02;//长按
        break;
    default:
        return 1U;
    }
    return xiaoni_smart_screen_uart_sendframe(&frame);
}

/* 场景按键事件上报（小米专用：页面+通道+类型）
   Byte[3]=页面, Byte[4]=通道, Byte[5]=类型(0=单击,1=双击,2=长按) */
static uint8_t xiaoni_smart_screen_send_x88_scene_report(
    uint8_t page,
    HOOCH_PROTOCOL_SceneReportKey_t key,
    HOOCH_PROTOCOL_SceneReportType_t type)
{
    Frame_t frame;
    if(page != 0x01)
    {
        return 1U;
    }
    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();
    frame.command = XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL;
    frame.data_len = 5U;

    frame.data[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
    frame.data[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
    frame.data[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_EVENT_STATUS;
    frame.data[3] = 4 + key;
    switch (type)
    {
    case HOOCH_PROTOCOL_SCENE_REPORT_TYPE_SINGLE_CLICK:
        frame.data[4] = 0x00U; /* 单击 */
        break;
    case HOOCH_PROTOCOL_SCENE_REPORT_TYPE_DOUBLE_CLICK:
        frame.data[4] = 0x01U; /* 双击 */
        break;
    case HOOCH_PROTOCOL_SCENE_REPORT_TYPE_LONG_CLICK:
        frame.data[4] = 0x02U; /* 长按 */
        break;
    default:
        return 1U;
    }
    return xiaoni_smart_screen_uart_sendframe(&frame);
}

static uint8_t xiaoni_smart_screen_send_setting_report(
    uint8_t command,
    const uint8_t *payload,
    uint16_t payload_len)
{
    Frame_t frame;

    if ((payload == NULL) && (payload_len != 0U))
    {
        return 1U;
    }

    if (payload_len > (uint16_t)MAX_FRAME_DATA_LEN)
    {
        return 1U;
    }

    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();
    frame.command = command;
    frame.data_len = payload_len;

    if (payload_len != 0U)
    {
        memcpy(frame.data, payload, payload_len);
    }

    return xiaoni_smart_screen_uart_sendframe(&frame);
}

static void xiaoni_smart_screen_key_click_report_callback(
    const HOOCH_PROTOCOL_KeyClickReportFrame_t *frame)
{
    uint8_t send_result;

    if (frame == NULL)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] KeyClickReport: frame is NULL\r\n");
        return;
    }

    XIAOMI_SMART_SCREEN_LOG_INFO("[HOOCH] KeyClickReport key=%u, event=%u, sequence=%u, valid=%u\r\n",
                                 (unsigned int)frame->key,
                                 (unsigned int)frame->event,
                                 (unsigned int)frame->sequence,
                                 (unsigned int)frame->valid);

    if (frame->event == HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_NONE)
    {
        return;
    }

    send_result = xiaoni_smart_screen_send_x88_key_click_report(frame->key, frame->event);
    if (send_result != 0U)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] KeyClickReport report send failed, key=%u, event=%u\r\n",
                                     (unsigned int)frame->key,
                                     (unsigned int)frame->event);
    }
}

static void xiaoni_smart_screen_scene_report_callback(
    const HOOCH_PROTOCOL_SceneReportFrame_t *frame)
{
    uint8_t send_result;

    if (frame == NULL)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] SceneReport: frame is NULL\r\n");
        return;
    }

    XIAOMI_SMART_SCREEN_LOG_INFO("[HOOCH] SceneReport page=%u, key=%u, type=%u, sequence=%u, valid=%u\r\n",
                                 (unsigned int)frame->page,
                                 (unsigned int)frame->key,
                                 (unsigned int)frame->type,
                                 (unsigned int)frame->sequence,
                                 (unsigned int)frame->valid);

    send_result = xiaoni_smart_screen_send_x88_scene_report(
        frame->page, frame->key, frame->type);
    if (send_result != 0U)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] SceneReport send failed, page=%u, key=%u, type=%u\r\n",
                                     (unsigned int)frame->page,
                                     (unsigned int)frame->key,
                                     (unsigned int)frame->type);
    }
}

static void xiaoni_smart_screen_key_status_report_callback(
    const HOOCH_PROTOCOL_KeyStatusFrame_t *frame)
{
    uint8_t send_result;

    if (frame == NULL)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] KeyStatusReport: frame is NULL\r\n");
        return;
    }

    XIAOMI_SMART_SCREEN_LOG_INFO("[HOOCH] KeyStatusReport key=%u, state=%u, sequence=%u, valid=%u\r\n",
                                 (unsigned int)frame->key,
                                 (unsigned int)frame->state,
                                 (unsigned int)frame->sequence,
                                 (unsigned int)frame->valid);

    if ((frame->key >= HOOCH_PROTOCOL_KEY_STATUS_KEY_1) &&
        (frame->key <= HOOCH_PROTOCOL_KEY_STATUS_KEY_8))
    {
        uint8_t mask = (uint8_t)(1U << ((uint8_t)frame->key - 1U));

        if (frame->state == HOOCH_PROTOCOL_KEY_STATUS_STATE_ON)
        {
            s_xiaomi_switch_control_bits |= mask;
        }
        else
        {
            s_xiaomi_switch_control_bits &= (uint8_t)~mask;
        }

        send_result = xiaoni_smart_screen_send_x88_switch_control_report(s_xiaomi_switch_control_bits);
        if (send_result != 0U)
        {
            s_xiaomi_switch_control_pending = 1U;
        }
        else
        {
            s_xiaomi_switch_control_pending = 0U;
        }
    }
}



static void xiaoni_smart_screen_dimmer_light_callback2(
    const HOOCH_PROTOCOL_DimmerLightFrame_t *frame)
{
    uint8_t send_result;

    if (frame == NULL)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] DimmerLight: frame is NULL\r\n");
        return;
    }

    XIAOMI_SMART_SCREEN_LOG_INFO("[HOOCH] DimmerLight key=%u, brightness=%u, color_temperature=%u, switch_state=%u, control_item=%u, sequence=%u, valid=%u\r\n",
                                 (unsigned int)frame->key,
                                 (unsigned int)frame->brightness,
                                 (unsigned int)frame->color_temperature,
                                 (unsigned int)frame->switch_state,
                                 (unsigned int)frame->control_item,
                                 (unsigned int)frame->sequence,
                                 (unsigned int)frame->valid);
    send_result = 0U;
    if ((unsigned int)frame->control_item == HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_SWITCH)
    {
        send_result = xiaoni_smart_screen_send_x88_light_dimmer_report_report(
            HOOCH_PROTOCOL_CODE_MATCH_PAGE_LIGHT,
            frame->key,
            frame->switch_state);
    }
    else if ((unsigned int)frame->control_item == HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_BRIGHTNESS)
    {
        send_result = xiaoni_smart_screen_send_x88_light_dimmer_brightness_report(
            HOOCH_PROTOCOL_CODE_MATCH_PAGE_LIGHT,
            frame->key,
            frame->brightness);
    }
    else if ((unsigned int)frame->control_item == HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_COLOR_TEMP)
    {
        send_result = xiaoni_smart_screen_send_x88_light_dimmer_temperature_report(
            HOOCH_PROTOCOL_CODE_MATCH_PAGE_LIGHT,
            frame->key,
            frame->color_temperature);
    }

    if (send_result != 0U)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] DimmerLight report send failed, key=%u, control_item=%u\r\n",
                                     (unsigned int)frame->key,
                                     (unsigned int)frame->control_item);
    }
}

static void xiaoni_smart_screen_dimmer_light_callback(
    const HOOCH_PROTOCOL_DimmerLightFrame_t *frame)
{
    uint8_t send_result = 0U;



    if ((unsigned int)frame->control_item == HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_SWITCH)
    {
        send_result = xiaoni_smart_screen_send_x88_switch_dimmer_report_report(
            frame->key,
            frame->switch_state);
    }
    else if ((unsigned int)frame->control_item == HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_BRIGHTNESS)
    {
        send_result = xiaoni_smart_screen_send_x88_switch_dimmer_brightness_report(
            frame->key,
            frame->brightness);
    }
    else if ((unsigned int)frame->control_item == HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_COLOR_TEMP)
    {
        send_result = xiaoni_smart_screen_send_x88_switch_dimmer_temperature_report(
            frame->key,
            frame->color_temperature);
    }
    XIAOMI_SMART_SCREEN_LOG_WARN("DimmerLight report send failedsend_result=%d\r\n", send_result);

}

static void xiaoni_smart_screen_curtain_report_callback2(
    const HOOCH_PROTOCOL_CurtainFrame_t *frame)
{
    uint8_t send_result;

    if (frame == NULL)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] CurtainReport: frame is NULL\r\n");
        return;
    }
    
    XIAOMI_SMART_SCREEN_LOG_INFO("[HOOCH] CurtainReport key=%u, switch=%u, stop=%u, percent=%u, angle=%u, control_item=%u, valid=%u\r\n",
                                 (unsigned int)frame->key,
                                 (unsigned int)frame->switch_status.value,
                                 (unsigned int)frame->stop.value,
                                 (unsigned int)frame->percent.value,
                                 (unsigned int)frame->angle.value,
                                 (unsigned int)frame->control_item,
                                 (unsigned int)frame->valid);

    send_result = 0U;

    switch (frame->control_item)
    {
    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_SWITCH:
        send_result = xiaoni_smart_screen_send_x88_curtain_switch_report(
            HOOCH_PROTOCOL_CODE_MATCH_PAGE_CURTAIN,
            (HOOCH_PROTOCOL_KeyStatusKey_t)frame->key,
            frame->switch_status.value);
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_STOP:
        send_result = xiaoni_smart_screen_send_x88_curtain_switch_report(
            HOOCH_PROTOCOL_CODE_MATCH_PAGE_CURTAIN,
            (HOOCH_PROTOCOL_KeyStatusKey_t)frame->key,
            frame->stop.value);
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_PERCENT:
        send_result = xiaoni_smart_screen_send_x88_curtain_page_travel_report(
            HOOCH_PROTOCOL_CODE_MATCH_PAGE_CURTAIN,
            (HOOCH_PROTOCOL_KeyStatusKey_t)frame->key,
            frame->percent.value);
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ANGLE:
        send_result = xiaoni_smart_screen_send_x88_curtain_page_travel_report(
            HOOCH_PROTOCOL_CODE_MATCH_PAGE_CURTAIN,
            (HOOCH_PROTOCOL_KeyStatusKey_t)frame->key,
            frame->angle.value);
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ALL:
        send_result = xiaoni_smart_screen_send_x88_curtain_switch_report(
            HOOCH_PROTOCOL_CODE_MATCH_PAGE_CURTAIN,
            (HOOCH_PROTOCOL_KeyStatusKey_t)frame->key,
            frame->switch_status.value);
        if (send_result == 0U)
        {
            send_result = xiaoni_smart_screen_send_x88_curtain_page_travel_report(
                HOOCH_PROTOCOL_CODE_MATCH_PAGE_CURTAIN,
                (HOOCH_PROTOCOL_KeyStatusKey_t)frame->key,
                frame->percent.value);
        }
        break;

    default:
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] CurtainReport unsupported control_item=%u\r\n",
                                     (unsigned int)frame->control_item);
        return;
    }

    if (send_result != 0U)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] CurtainReport send failed, key=%u, control_item=%u\r\n",
                                     (unsigned int)frame->key,
                                     (unsigned int)frame->control_item);
    }
}

static void xiaoni_smart_screen_curtain_report_callback(
    const HOOCH_PROTOCOL_CurtainFrame_t *frame)
{
    uint8_t send_result;

    if (frame == NULL)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] CurtainReport2: frame is NULL\r\n");
        return;
    }

    XIAOMI_SMART_SCREEN_LOG_INFO("[HOOCH] CurtainReport2 key=%u, switch=%u, stop=%u, percent=%u, angle=%u, control_item=%u, valid=%u\r\n",
                                 (unsigned int)frame->key,
                                 (unsigned int)frame->switch_status.value,
                                 (unsigned int)frame->stop.value,
                                 (unsigned int)frame->percent.value,
                                 (unsigned int)frame->angle.value,
                                 (unsigned int)frame->control_item,
                                 (unsigned int)frame->valid);

    send_result = 0U;

    switch (frame->control_item)
    {
    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_SWITCH:
        send_result = xiaoni_smart_screen_send_x88_switch_curtain_switch_report(
            (HOOCH_PROTOCOL_KeyStatusKey_t)frame->key,
            frame->switch_status.value);
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_STOP:
        send_result = xiaoni_smart_screen_send_x88_switch_curtain_switch_report(
            (HOOCH_PROTOCOL_KeyStatusKey_t)frame->key,
            frame->stop.value);
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_PERCENT:
        send_result = xiaoni_smart_screen_send_x88_curtain_travel_report(
            (HOOCH_PROTOCOL_KeyStatusKey_t)frame->key,
            frame->percent.value);
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ANGLE:
        send_result = xiaoni_smart_screen_send_x88_curtain_travel_report(
            (HOOCH_PROTOCOL_KeyStatusKey_t)frame->key,
            frame->angle.value);
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ALL:
        send_result = xiaoni_smart_screen_send_x88_switch_curtain_switch_report(
            (HOOCH_PROTOCOL_KeyStatusKey_t)frame->key,
            frame->switch_status.value);
        if (send_result == 0U)
        {
            send_result = xiaoni_smart_screen_send_x88_curtain_travel_report(
                (HOOCH_PROTOCOL_KeyStatusKey_t)frame->key,
                frame->percent.value);
        }
        break;

    default:
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] CurtainReport2 unsupported control_item=%u\r\n",
                                     (unsigned int)frame->control_item);
        return;
    }

    if (send_result != 0U)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] CurtainReport2 send failed, key=%u, control_item=%u\r\n",
                                     (unsigned int)frame->key,
                                     (unsigned int)frame->control_item);
    }
}

/* 将 HOOCH 协议风速枚举映射回小米协议值。
   HOOCH枚举：LOW=1, MEDIUM=2, HIGH=3, AUTO=4
   小米协议：1=自动, 2=低风, 3=中风, 4=高风 */
static uint8_t xiaomi_smart_screen_map_fan_speed_reverse(HOOCH_PROTOCOL_AirConditionerFanSpeed_t fs)
{
    switch (fs)
    {
        case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_LOW:    return 2U;
        case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_MEDIUM: return 3U;
        case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_HIGH:   return 4U;
        case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_AUTO:   return 1U;
        default: return 0U;
    }
}

static void xiaoni_smart_screen_air_conditioner_report_callback(
    const HOOCH_PROTOCOL_AirConditionerFrame_t *frame)
{
    uint8_t send_result;
    uint8_t payload[5];

    if (frame == NULL)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] AirConditioner: frame is NULL\r\n");
        return;
    }

    XIAOMI_SMART_SCREEN_LOG_INFO("[HOOCH] AirConditioner channel=%u, power=%u, mode=%u, fan_speed=%u, temperature=%u, control_item=%u, sequence=%u, valid=%u\r\n",
                                 (unsigned int)frame->channel,
                                 (unsigned int)frame->power,
                                 (unsigned int)frame->mode,
                                 (unsigned int)frame->fan_speed,
                                 (unsigned int)frame->temperature,
                                 (unsigned int)frame->control_item,
                                 (unsigned int)frame->sequence,
                                 (unsigned int)frame->valid);

    /* 帧头：REPORT + DEFAULT + AC_STATUS，后2字节为数据 */
    payload[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
    payload[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
    payload[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_AC_STATUS;

    /* 按小米协议构建数据：Byte[3]=控制类型, Byte[4]=参数值 */
    switch (frame->control_item)
    {
    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL:
    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_KNX:
    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_XIAOMI:
        /* 全量上报暂不处理 */
        return;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_POWER:
        /* Byte[3]=0: 空调开关, Byte[4]: 0=关, 1=开 */
        payload[3] = 0U;
        payload[4] = (uint8_t)frame->power;
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMPERATURE:
        /* Byte[3]=1: 温度, Byte[4]: 16~32度 */
        payload[3] = 1U;
        payload[4] = frame->temperature;
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MODE:
        /* Byte[3]=2: 模式, Byte[4]: 1=制冷/2=送风/3=除湿/4=制热/5=自动 */
        payload[3] = 2U;
        payload[4] = (uint8_t)frame->mode;
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_FAN_SPEED:
        /* Byte[3]=3: 风速, Byte[4]: 1=自动/2=低风/3=中风/4=高风 */
        payload[3] = 3U;
        payload[4] = xiaomi_smart_screen_map_fan_speed_reverse(frame->fan_speed);
        break;

    default:
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] AirConditioner unsupported control_item=%u\r\n",
                                     (unsigned int)frame->control_item);
        return;
    }

    send_result = xiaoni_smart_screen_send_setting_report(
        (uint8_t)XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL,
        payload,
        sizeof(payload));

    if (send_result != 0U)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] AirConditioner report send failed, channel=%u, control_item=%u\r\n",
                                     (unsigned int)frame->channel,
                                     (unsigned int)frame->control_item);
    }
}

static void xiaoni_smart_screen_setting_report_callback(
    HOOCH_PROTOCOL_SettingReportEvent_t event,
    const void *data,
    uint16_t data_len)
{
    uint8_t send_result;

    send_result = 0U;

    XIAOMI_SMART_SCREEN_LOG_INFO("[HOOCH] SettingReport event=%u, has_data=%u, data_len=%u\r\n",
                                 (unsigned int)event,
                                 (unsigned int)(data != 0),
                                 (unsigned int)data_len);
    (void)data_len;

    switch (event)
    {
    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_NETWORK_JOIN:
    {
        uint8_t payload[1] = { 0x00U };
        send_result = xiaoni_smart_screen_send_setting_report(
            (uint8_t)XIAOMI_SMART_SCREEN_NET_STATUS_CONTROL,
            payload,
            1U);
        break;
    }

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_NETWORK_LEAVE:
        send_result = xiaoni_smart_screen_send_setting_report(
            XIAOMI_SMART_SCREEN_NET_LEAVE_COMMAND,
            NULL,
            0U);
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_RESET:
    {
        uint8_t payload[1] = { 0x01U };
        send_result = xiaoni_smart_screen_send_setting_report(
            (uint8_t)XIAOMI_SMART_SCREEN_RESET_CONTROL,
            payload,
            1U);
        break;
    }

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_PRODUCTION_TEST:
        send_result = xiaoni_smart_screen_send_setting_report(
            (uint8_t)XIAOMI_SMART_SCREEN_SET_PRODUCTION_CONTROL,
            NULL,
            0U);
        break;
    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_HUMAN_PRESENCE:
    {
        uint8_t payload[4];
        payload[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
        payload[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
        payload[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_SENSOR_STATUS_REPORT;
        payload[3] = (data != 0) ? ((const uint8_t *)data)[0] : 0U;
        send_result = xiaoni_smart_screen_send_setting_report(
            (uint8_t)XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL,
            payload,
            sizeof(payload));
    }
        break;
        case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_GET_WEATHER:
        {
        uint8_t payload[5];
        payload[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
        payload[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
        payload[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_WEATHER_CODE;
        payload[3] = 0U;
        payload[4] = 0U;
        send_result = xiaoni_smart_screen_send_setting_report(
            (uint8_t)XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL,
            payload,
            sizeof(payload));
        }
        break;
    default:
        send_result = 1U;
        break;
    }

    if (send_result != 0U)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] SettingReport send failed, event=%u\r\n",
                                     (unsigned int)event);
    }
}


static void xiaoni_smart_screen_code_match_report_callback(
    const HOOCH_PROTOCOL_CodeMatchReportFrame_t *frame)
{




     Frame_t tx_frame;
     uint8_t send_result;

    /* 拼帧并通过 UART 透传给智能屏（3字节原始 payload 直通） */
    tx_frame.header  = FRAME_HEADER;
    tx_frame.version = xiaoni_smart_screen_get_version();
    tx_frame.command = XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL;
    tx_frame.data_len = 4U;

    tx_frame.data[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
    tx_frame.data[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
    tx_frame.data[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_ENTER_PAIRING_CLEAR_INTERLOCK;


    switch (frame->page) {
    case 0x00U: /* 开关 */
        tx_frame.data[3] = frame->channel;
        break;
    case 0x01U: /* 情景 */
        break;
    case 0x02U: /* 灯光 */
        tx_frame.data[3] = 5;
        break;
    case 0x03U: /* 窗帘 */
        if (frame->channel == 1)
        {
            tx_frame.data[3] = 6;
        }
        else
        {
            tx_frame.data[3] = 7;
        }
        break;
    case 0x04U: /* 空调 */
        tx_frame.data[3] = 8;
        break;
    default:
        break;
    }

    send_result = xiaoni_smart_screen_uart_sendframe(&tx_frame);
    if (send_result != 0U)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] CodeMatchReport send failed\r\n");
    }

}




/*
 * 开关页面对码专用回调: 收到已解码的 key/mode，直接组帧发送给智能屏。
 */
static void xiaoni_smart_screen_code_match_switch_callback(
    uint8_t page,
    uint8_t key,
    uint8_t mode,
    HOOCH_PROTOCOL_CodeMatchReportAction_t action)
{



     Frame_t tx_frame;
     uint8_t send_result;

    /* 拼帧并通过 UART 透传给智能屏（3字节原始 payload 直通） */
    tx_frame.header  = FRAME_HEADER;
    tx_frame.version = xiaoni_smart_screen_get_version();
    tx_frame.command = XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL;
    tx_frame.data_len = 4U;

    tx_frame.data[0] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_REPORT;
    tx_frame.data[1] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT;
    tx_frame.data[2] = (uint8_t)XIAOMI_SMART_SCREEN_SUBCMD_ENTER_PAIRING_CLEAR_INTERLOCK;


    switch (page) {
    case 0x00U: /* 开关 */
        tx_frame.data[3] = key;
        break;
    case 0x01U: /* 情景 */
        break;
    case 0x02U: /* 灯光 */
        break;
    case 0x03U: /* 窗帘 */
        break;
    case 0x04U: /* 空调 */
        break;
    default:
        break;
    }

    send_result = xiaoni_smart_screen_uart_sendframe(&tx_frame);
    if (send_result != 0U)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("[HOOCH] CodeMatchReport send failed\r\n");
    }

}




/**
 * @brief 环形缓冲区初始化
 * @param rb 环形缓冲区指针
 * @param rb 环形缓冲区指针
 */
void RingBuffer_Init(RingBuffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    memset(rb->buffer, 0, RING_BUFFER_SIZE);
}

/**
 * @brief 向环形缓冲区写入数据
 * @param rb 环形缓冲区指针
 * @param data 要写入的数据
 * @return 0表示成功，1表示缓冲区已满
 */
uint8_t RingBuffer_Push(RingBuffer_t *rb, uint8_t data)
{
    if (rb->count >= RING_BUFFER_SIZE)
    {
        return 1; /* 缓冲区已满 */
    }

    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % RING_BUFFER_SIZE;
    rb->count++;

    return 0;
}

/**
 * @brief 从环形缓冲区读取数据
 * @param rb 环形缓冲区指针
 * @param data 读取数据的存储指针
 * @return 0表示成功，1表示缓冲区为空
 */
uint8_t RingBuffer_Pop(RingBuffer_t *rb, uint8_t *data)
{

    if (rb->count == 0)
    {
        // LOG_INFO("==== RingBuffer_Pop FAIL ====\r\n");
        return 1; /* 缓冲区为空 */
    }
    XIAOMI_SMART_SCREEN_LOG_RX_BYTE(rb->buffer[rb->tail]);
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % RING_BUFFER_SIZE;
    rb->count--;

    return 0;
}

/**
 * @brief 获取环形缓冲区中的数据数量
 * @param rb 环形缓冲区指针
 * @return 数据数量
 */
uint16_t RingBuffer_GetCount(RingBuffer_t *rb)
{
    return rb->count;
}

/**
 * @brief 判断环形缓冲区是否为空
 * @param rb 环形缓冲区指针
 * @return 0表示非空，1表示为空
 */
uint8_t RingBuffer_IsEmpty(RingBuffer_t *rb)
{
    return (rb->count == 0) ? 1 : 0;
}

/**
 * @brief 判断环形缓冲区是否已满
 * @param rb 环形缓冲区指针
 * @return 0表示未满，1表示已满
 */
uint8_t RingBuffer_IsFull(RingBuffer_t *rb)
{
    return (rb->count >= RING_BUFFER_SIZE) ? 1 : 0;
}

/**
 * @brief 帧解析器初始化
 * @param parser 帧解析器指针
 */
void FrameParser_Init(FrameParser_t *parser)
{
    parser->state = FRAME_STATE_IDLE;
    parser->data_index = 0;
    parser->calc_checksum = 0;
    parser->frame_ready = 0;
    memset(&parser->frame, 0, sizeof(Frame_t));
}

/**
 * @brief 计算帧检验和
 * @param frame 帧指针
 * @return 计算得到的检验和
 */
uint8_t FrameParser_CalcChecksum(Frame_t *frame)
{
    uint32_t sum = 0;

    /* 帧头高字节 */
    sum += (frame->header >> 8) & 0xFF;
    /* 帧头低字节 */
    sum += frame->header & 0xFF;
    /* 版本 */
    sum += frame->version;
    /* 命令 */
    sum += frame->command;
    /* 数据长度高字节 */
    sum += (frame->data_len >> 8) & 0xFF;
    /* 数据长度低字节 */
    sum += frame->data_len & 0xFF;
    /* 数据 */
    for (uint16_t i = 0; i < frame->data_len; i++)
    {
        sum += frame->data[i];
    }

    return (uint8_t)(sum % 256);
}

/**
 * @brief 验证帧检验和
 * @param frame 帧指针
 * @return 0表示检验和正确，1表示检验和错误
 */
uint8_t FrameParser_VerifyChecksum(Frame_t *frame)
{
    uint8_t calc_checksum = FrameParser_CalcChecksum(frame);
    return (calc_checksum == frame->checksum) ? 0 : 1;
}

/**
 * @brief 处理接收到的字节数据（状态机解析）
 * @param parser 帧解析器指针
 * @param byte 接收到的字节
 * @param out_frame 解析完成的帧输出指针
 */
void FrameParser_ProcessByte(FrameParser_t *parser, uint8_t byte, Frame_t *out_frame)
{
    parser->frame_ready = 0;

    switch (parser->state)
    {
    case FRAME_STATE_IDLE:
        /* 等待帧头高字节 0x55 */
        if (byte == 0x55)
        {
            parser->calc_checksum = byte;
            parser->state = FRAME_STATE_HEADER_H;
            //  LOG_INFO("FrameParser_ProcessByte: FRAME_STATE_HEADER_H\r\n");
        }
        break;

    case FRAME_STATE_HEADER_H:
        /* 等待帧头低字节 0xAA */
        if (byte == 0xAA)
        {
            parser->calc_checksum += byte;
            parser->frame.header = 0x55AA;
            parser->state = FRAME_STATE_VERSION;
            //    LOG_INFO("FrameParser_ProcessByte: FRAME_STATE_VERSION\r\n");
        }
        else
        {
            /* 不是完整的帧头，重新开始 */
            parser->state = FRAME_STATE_IDLE;
            /* 检查是否是新帧头的开始 */
            if (byte == 0x55)
            {
                parser->calc_checksum = byte;
                parser->state = FRAME_STATE_HEADER_H;
            }
        }
        break;

    case FRAME_STATE_VERSION:
        parser->calc_checksum += byte;
        parser->frame.version = byte;
        //  LOG_INFO("FrameParser_ProcessByte: FRAME_STATE_CMD\r\n");
        parser->state = FRAME_STATE_CMD;
        break;

    case FRAME_STATE_CMD:
        parser->calc_checksum += byte;
        //  LOG_INFO("FrameParser_ProcessByte: FRAME_STATE_DATA_LEN_H\r\n");
        parser->frame.command = byte;
        parser->state = FRAME_STATE_DATA_LEN_H;
        break;

    case FRAME_STATE_DATA_LEN_H:
        parser->calc_checksum += byte;
        parser->frame.data_len = byte << 8; /* 高字节 */
        parser->state = FRAME_STATE_DATA_LEN_L;
        break;

    case FRAME_STATE_DATA_LEN_L:
        //   LOG_INFO("FrameParser_ProcessByte: FRAME_STATE_DATA_LEN_L\r\n");
        parser->calc_checksum += byte;
        parser->frame.data_len |= byte; /* 低字节 */

        /* 检查数据长度是否合法 */
        if (parser->frame.data_len > MAX_FRAME_DATA_LEN)
        {
            /* 数据长度超出范围，重新开始 */
            parser->state = FRAME_STATE_IDLE;
        }
        else if (parser->frame.data_len == 0)
        {
            /* 没有数据，直接跳到检验和 */
            parser->state = FRAME_STATE_CHECKSUM;
        }
        else
        {
            parser->data_index = 0;
            parser->state = FRAME_STATE_DATA;
        }
        break;

    case FRAME_STATE_DATA:
        parser->calc_checksum += byte;
        parser->frame.data[parser->data_index] = byte;
        parser->data_index++;

        if (parser->data_index >= parser->frame.data_len)
        {
            parser->state = FRAME_STATE_CHECKSUM;
        }
        break;

    case FRAME_STATE_CHECKSUM:
        parser->frame.checksum = byte;

        /* 验证检验和 */
        if (FrameParser_VerifyChecksum(&parser->frame) == 0)
        {
            /* 检验和正确，帧解析完成 */
            parser->frame_ready = 1;
            if (out_frame != NULL)
            {
                memcpy(out_frame, &parser->frame, sizeof(Frame_t));
            }
            //      LOG_INFO("Frame parsed: CMD=0x%02X, LEN=%d\r\n",
            //               parser->frame.command, parser->frame.data_len);
        }
        else
        {
            XIAOMI_SMART_SCREEN_LOG_WARN("Checksum error!\r\n");
        }

        /* 重新开始解析 */
        parser->state = FRAME_STATE_IDLE;
        break;

    default:
        parser->state = FRAME_STATE_IDLE;
        break;
    }
}







/**
 * @brief 小米 UART 初始化
 */
void xiaoni_smart_screen_uart_init(void)
{
    /* 初始化环形缓冲区 */
    RingBuffer_Init(&rx_ring_buffer);

    /* 初始化帧解析器 */
    FrameParser_Init(&frame_parser);
    frame_parser_timeout_ticks = 0;
   HOOCH_PROTOCOL_KeyStatus_RegisterReportCallback(xiaoni_smart_screen_key_status_report_callback);
   HOOCH_PROTOCOL_KeyClickReport_RegisterCallback(xiaoni_smart_screen_key_click_report_callback);
   HOOCH_PROTOCOL_SceneReport_RegisterReportCallback4(xiaoni_smart_screen_scene_report_callback);
   HOOCH_PROTOCOL_DimmerLight_RegisterReportCallback(xiaoni_smart_screen_dimmer_light_callback);
   HOOCH_PROTOCOL_DimmerLight_RegisterReportCallback2(xiaoni_smart_screen_dimmer_light_callback2);

   HOOCH_PROTOCOL_Curtain_RegisterReportCallback(xiaoni_smart_screen_curtain_report_callback);
   HOOCH_PROTOCOL_Curtain_RegisterReportCallback2(xiaoni_smart_screen_curtain_report_callback2);

   HOOCH_PROTOCOL_AirConditioner_RegisterReportCallback(xiaoni_smart_screen_air_conditioner_report_callback);

   HOOCH_PROTOCOL_SettingReport_RegisterCallback(xiaoni_smart_screen_setting_report_callback);
   HOOCH_PROTOCOL_CodeMatchReport_RegisterCallback(xiaoni_smart_screen_code_match_report_callback);
   HOOCH_PROTOCOL_CodeMatchReport_RegisterSwitchCallback(xiaoni_smart_screen_code_match_switch_callback);





    XIAOMI_SMART_SCREEN_LOG_INFO("XiaoMi Quan UART initialized\r\n");
}

/**
 * @brief 处理环形缓冲区中的数据（在主循环中调用）
 */
void xiaoni_smart_screen_uart_process(void)
{
    uint8_t byte;
    uint8_t received_data = 0;
    Frame_t parsed_frame;

    while (RingBuffer_Pop(&rx_ring_buffer, &byte) == 0)
    {
        received_data = 1;
        frame_parser_timeout_ticks = 0;
        FrameParser_ProcessByte(&frame_parser, byte, &parsed_frame);

        if (frame_parser.frame_ready)
        {
        extern void hooch_cpu1_set_detected_protocol(uint8_t protocol);
            hooch_cpu1_set_detected_protocol(2);
            /* 帧解析完成，可以在这里处理解析后的帧 */
            XIAOMI_SMART_SCREEN_LOG_DEBUG("Received frame - Version: %d, Command: 0x%02X, DataLen: %d\r\n",
                                          parsed_frame.version, parsed_frame.command, parsed_frame.data_len);

                      /*记录版本*/
            module_version = parsed_frame.version;
            xiaomi_smart_screen_handle_frame(&parsed_frame);

        }
    }

    if (received_data == 0)
    {
        if (frame_parser.state != FRAME_STATE_IDLE)
        {
            if (frame_parser_timeout_ticks < FRAME_PARSER_TIMEOUT_TICKS)
            {
                frame_parser_timeout_ticks++;
            }

            if (frame_parser_timeout_ticks >= FRAME_PARSER_TIMEOUT_TICKS)
            {
                XIAOMI_SMART_SCREEN_LOG_WARN("Frame parser timeout, reset parser state\r\n");
                FrameParser_Init(&frame_parser);
                frame_parser_timeout_ticks = 0;
            }
        }
        else
        {
            frame_parser_timeout_ticks = 0;
        }
    }
    else if (frame_parser.state == FRAME_STATE_IDLE)
    {
        frame_parser_timeout_ticks = 0;
    }

    if (s_xiaomi_switch_control_pending != 0U)
    {
        uint8_t switch_bits = s_xiaomi_switch_control_bits;
        s_xiaomi_switch_control_pending = 0U;

        if (xiaoni_smart_screen_send_x88_switch_control_report(switch_bits) != 0U)
        {
            s_xiaomi_switch_control_pending = 1U;
        }
    }
}

/**
 * @brief 发送帧数据
 * @param frame 要发送的帧
 * @return 0表示成功，1表示失败
 */
uint8_t xiaoni_smart_screen_uart_sendframe(Frame_t *frame)
{
    uint8_t tx_buffer[MAX_FRAME_LEN];
    uint16_t tx_index = 0;

    /* 检查数据长度 */
    if (frame->data_len > MAX_FRAME_DATA_LEN)
    {
        XIAOMI_SMART_SCREEN_LOG_ERROR("Frame data length too large: %d\r\n", frame->data_len);
        return 1;
    }

    /* 构建帧数据 */
    /* 帧头 */
    tx_buffer[tx_index++] = (frame->header >> 8) & 0xFF;
    tx_buffer[tx_index++] = frame->header & 0xFF;

    /* 版本 */
    tx_buffer[tx_index++] = frame->version;

    /* 命令 */
    tx_buffer[tx_index++] = frame->command;

    /* 数据长度（高字节在前） */
    tx_buffer[tx_index++] = (frame->data_len >> 8) & 0xFF;
    tx_buffer[tx_index++] = frame->data_len & 0xFF;

    /* 数据 */
    for (uint16_t i = 0; i < frame->data_len; i++)
    {
        tx_buffer[tx_index++] = frame->data[i];
    }

    /* 计算并添加检验和 */
    frame->checksum = FrameParser_CalcChecksum(frame);
    tx_buffer[tx_index++] = frame->checksum;

    /* 发送数据 */
    XIAOMI_SMART_SCREEN_TX_SEND(tx_buffer, tx_index);

    /* 打印发送的原始数据（十六进制） */
    XIAOMI_SMART_SCREEN_LOG_TX_HEX_BEGIN(tx_index);
    for (uint16_t i = 0; i < tx_index; i++)
    {
        XIAOMI_SMART_SCREEN_LOG_TX_HEX_BYTE(tx_buffer[i]);
    }
    XIAOMI_SMART_SCREEN_LOG_TX_HEX_END();

    XIAOMI_SMART_SCREEN_LOG_DEBUG("Frame sent: CMD=0x%02X, LEN=%d, Checksum=0x%02X\r\n",
                                  frame->command, frame->data_len, frame->checksum);

    return 0;
}
/**
 * @brief 将接收到的数据写入环形缓冲区（在中断中调用）
 * @param data 接收到的数据
 */
void xiaoni_smart_screen_uart_rxcallback(uint8_t data)
{
    if (RingBuffer_Push(&rx_ring_buffer, data) != 0)
    {
        XIAOMI_SMART_SCREEN_LOG_WARN("Ring buffer overflow!\r\n");
    }
}
/*获取模组版本*/
unsigned char xiaoni_smart_screen_get_version(void)
{
    return module_version;
}
