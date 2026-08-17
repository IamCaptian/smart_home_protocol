#include "hooch_dimmer_light.h"

static HOOCH_PROTOCOL_DimmerLightFrame_t s_hooch_protocol_dimmer_light_frame;

static HOOCH_PROTOCOL_DimmerLightReportCallback1_t s_hooch_protocol_dimmer_light_report_callback1;
static HOOCH_PROTOCOL_DimmerLightReportCallback2_t s_hooch_protocol_dimmer_light_report_callback2;
static HOOCH_PROTOCOL_DimmerLightReportCallback3_t s_hooch_protocol_dimmer_light_report_callback3;

static void HOOCH_PROTOCOL_DimmerLight_NotifyReportCallback1(void);
static void HOOCH_PROTOCOL_DimmerLight_NotifyReportCallback2(void);
static void HOOCH_PROTOCOL_DimmerLight_NotifyReportCallback3(void);

static uint8_t HOOCH_PROTOCOL_DimmerLight_IsValidControlItem(
    HOOCH_PROTOCOL_DimmerLightControlItem_t control_item)
{
    return (uint8_t)((control_item >= HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_ALL) &&
                     (control_item <= HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_SWITCH));
}

static uint8_t HOOCH_PROTOCOL_DimmerLight_IsValidKey(HOOCH_PROTOCOL_DimmerLightKey_t key)
{
    return (uint8_t)((key >= HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_1) &&
                     (key <= HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_8));
}

static uint8_t HOOCH_PROTOCOL_DimmerLight_IsValidSwitchState(
    HOOCH_PROTOCOL_DimmerLightSwitchState_t switch_state)
{
    return (uint8_t)((switch_state == HOOCH_PROTOCOL_DIMMER_LIGHT_SWITCH_OFF) ||
                     (switch_state == HOOCH_PROTOCOL_DIMMER_LIGHT_SWITCH_ON));
}

static HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SetSwitchState(
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    HOOCH_PROTOCOL_DimmerLightSwitchState_t switch_state)
{
    if ((HOOCH_PROTOCOL_DimmerLight_IsValidKey(key) == 0U) ||
        (HOOCH_PROTOCOL_DimmerLight_IsValidSwitchState(switch_state) == 0U))
    {
        return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_dimmer_light_frame.key = key;
    s_hooch_protocol_dimmer_light_frame.switch_state = (uint8_t)switch_state;
    s_hooch_protocol_dimmer_light_frame.control_item =
        HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_SWITCH;
    s_hooch_protocol_dimmer_light_frame.sequence++;
    s_hooch_protocol_dimmer_light_frame.valid = 1U;

    HOOCH_PROTOCOL_DimmerLight_NotifyReportCallback1();

    return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_OK;
}

static void HOOCH_PROTOCOL_DimmerLight_NotifyReportCallback1(void)
{
    if (s_hooch_protocol_dimmer_light_report_callback1 != 0)
    {
        s_hooch_protocol_dimmer_light_report_callback1(&s_hooch_protocol_dimmer_light_frame);
    }
}

static void HOOCH_PROTOCOL_DimmerLight_NotifyReportCallback2(void)
{
    if (s_hooch_protocol_dimmer_light_report_callback2 != 0)
    {
        s_hooch_protocol_dimmer_light_report_callback2(&s_hooch_protocol_dimmer_light_frame);
    }
}

static void HOOCH_PROTOCOL_DimmerLight_NotifyReportCallback3(void)
{
    if (s_hooch_protocol_dimmer_light_report_callback3 != 0)
    {
        s_hooch_protocol_dimmer_light_report_callback3(&s_hooch_protocol_dimmer_light_frame);
    }
}

void HOOCH_DimmerLight_Init(void)
{
    s_hooch_protocol_dimmer_light_report_callback1 = 0;
    s_hooch_protocol_dimmer_light_report_callback2 = 0;
    s_hooch_protocol_dimmer_light_report_callback3 = 0;
    HOOCH_PROTOCOL_DimmerLight_Clear();
}

void HOOCH_PROTOCOL_DimmerLight_Clear(void)
{
    s_hooch_protocol_dimmer_light_frame.key = HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_INVALID;
    s_hooch_protocol_dimmer_light_frame.page = 0U;
    s_hooch_protocol_dimmer_light_frame.address = 0U;
    s_hooch_protocol_dimmer_light_frame.brightness = 0U;
    s_hooch_protocol_dimmer_light_frame.color_temperature = 0U;
    s_hooch_protocol_dimmer_light_frame.switch_state = 0U;
    s_hooch_protocol_dimmer_light_frame.control_item =
        HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_INVALID;
    s_hooch_protocol_dimmer_light_frame.sequence = 0U;
    s_hooch_protocol_dimmer_light_frame.valid = 0U;
}

HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SetValue(uint8_t value)
{
    (void)value;
    return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_INVALID_PARAM;
}

HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SetFrame(
    const HOOCH_PROTOCOL_DimmerLightFrame_t *frame)
{
    if (frame == 0)
    {
        return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_INVALID_PARAM;
    }

    if (HOOCH_PROTOCOL_DimmerLight_IsValidControlItem(frame->control_item) == 0U)
    {
        return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_INVALID_PARAM;
    }

    switch (frame->control_item)
    {
        case HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_ALL:
            s_hooch_protocol_dimmer_light_frame.brightness = frame->brightness;
            s_hooch_protocol_dimmer_light_frame.color_temperature = frame->color_temperature;
            s_hooch_protocol_dimmer_light_frame.switch_state = frame->switch_state;
            break;

        case HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_BRIGHTNESS:
            s_hooch_protocol_dimmer_light_frame.brightness = frame->brightness;
            break;

        case HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_COLOR_TEMP:
            s_hooch_protocol_dimmer_light_frame.color_temperature = frame->color_temperature;
            break;

        case HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_SWITCH:
            s_hooch_protocol_dimmer_light_frame.switch_state = frame->switch_state;
            break;

        default:
            return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_dimmer_light_frame.key = frame->key;
    s_hooch_protocol_dimmer_light_frame.control_item = frame->control_item;
    s_hooch_protocol_dimmer_light_frame.sequence = frame->sequence;
    s_hooch_protocol_dimmer_light_frame.valid = 1U;

    HOOCH_PROTOCOL_DimmerLight_NotifyReportCallback1();

    return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_OK;
}

const HOOCH_PROTOCOL_DimmerLightFrame_t *HOOCH_PROTOCOL_DimmerLight_GetFrame(void)
{
    return &s_hooch_protocol_dimmer_light_frame;
}

void HOOCH_PROTOCOL_DimmerLight_RegisterReportCallback(
    HOOCH_PROTOCOL_DimmerLightReportCallback1_t callback)
{
    s_hooch_protocol_dimmer_light_report_callback1 = callback;
}

void HOOCH_PROTOCOL_DimmerLight_UnregisterReportCallback(void)
{
    s_hooch_protocol_dimmer_light_report_callback1 = 0;
}

void HOOCH_PROTOCOL_DimmerLight_RegisterReportCallback2(
    HOOCH_PROTOCOL_DimmerLightReportCallback2_t callback)
{
    s_hooch_protocol_dimmer_light_report_callback2 = callback;
}

void HOOCH_PROTOCOL_DimmerLight_UnregisterReportCallback2(void)
{
    s_hooch_protocol_dimmer_light_report_callback2 = 0;
}

void HOOCH_PROTOCOL_DimmerLight_RegisterReportCallback3(
    HOOCH_PROTOCOL_DimmerLightReportCallback3_t callback)
{
    s_hooch_protocol_dimmer_light_report_callback3 = callback;
}

void HOOCH_PROTOCOL_DimmerLight_UnregisterReportCallback3(void)
{
    s_hooch_protocol_dimmer_light_report_callback3 = 0;
}

HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_Send(uint8_t value)
{
    (void)value;
    return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_INVALID_PARAM;
}

HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendSwitch(
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    HOOCH_PROTOCOL_DimmerLightSwitchState_t state)
{
    return HOOCH_PROTOCOL_DimmerLight_SetSwitchState(key, state);
}

HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendBrightness(
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    uint8_t brightness)
{
    if (HOOCH_PROTOCOL_DimmerLight_IsValidKey(key) == 0U)
    {
        return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_dimmer_light_frame.key = key;
    s_hooch_protocol_dimmer_light_frame.brightness = brightness;
    s_hooch_protocol_dimmer_light_frame.control_item =
        HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_BRIGHTNESS;
    s_hooch_protocol_dimmer_light_frame.sequence++;
    s_hooch_protocol_dimmer_light_frame.valid = 1U;

    HOOCH_PROTOCOL_DimmerLight_NotifyReportCallback1();

    return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_OK;
}

HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendColorTemperature(
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    uint8_t color_temperature)
{
    if (HOOCH_PROTOCOL_DimmerLight_IsValidKey(key) == 0U)
    {
        return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_dimmer_light_frame.key = key;
    s_hooch_protocol_dimmer_light_frame.color_temperature = color_temperature;
    s_hooch_protocol_dimmer_light_frame.control_item =
        HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_COLOR_TEMP;
    s_hooch_protocol_dimmer_light_frame.sequence++;
    s_hooch_protocol_dimmer_light_frame.valid = 1U;

    HOOCH_PROTOCOL_DimmerLight_NotifyReportCallback1();

    return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_OK;
}

/* ======================== 类型2 上报：页面+通道+数值 ======================== */

HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendSwitchPage(
    uint8_t page,
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    HOOCH_PROTOCOL_DimmerLightSwitchState_t state)
{
    if ((HOOCH_PROTOCOL_DimmerLight_IsValidKey(key) == 0U) ||
        (HOOCH_PROTOCOL_DimmerLight_IsValidSwitchState(state) == 0U))
    {
        return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_dimmer_light_frame.page = page;
    s_hooch_protocol_dimmer_light_frame.key = key;
    s_hooch_protocol_dimmer_light_frame.switch_state = (uint8_t)state;
    s_hooch_protocol_dimmer_light_frame.control_item =
        HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_SWITCH;
    s_hooch_protocol_dimmer_light_frame.sequence++;
    s_hooch_protocol_dimmer_light_frame.valid = 1U;

    HOOCH_PROTOCOL_DimmerLight_NotifyReportCallback2();

    return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_OK;
}

HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendBrightnessPage(
    uint8_t page,
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    uint8_t brightness)
{
    if (HOOCH_PROTOCOL_DimmerLight_IsValidKey(key) == 0U)
    {
        return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_dimmer_light_frame.page = page;
    s_hooch_protocol_dimmer_light_frame.key = key;
    s_hooch_protocol_dimmer_light_frame.brightness = brightness;
    s_hooch_protocol_dimmer_light_frame.control_item =
        HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_BRIGHTNESS;
    s_hooch_protocol_dimmer_light_frame.sequence++;
    s_hooch_protocol_dimmer_light_frame.valid = 1U;

    HOOCH_PROTOCOL_DimmerLight_NotifyReportCallback2();

    return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_OK;
}

HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendColorTemperaturePage(
    uint8_t page,
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    uint8_t color_temperature)
{
    if (HOOCH_PROTOCOL_DimmerLight_IsValidKey(key) == 0U)
    {
        return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_dimmer_light_frame.page = page;
    s_hooch_protocol_dimmer_light_frame.key = key;
    s_hooch_protocol_dimmer_light_frame.color_temperature = color_temperature;
    s_hooch_protocol_dimmer_light_frame.control_item =
        HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_COLOR_TEMP;
    s_hooch_protocol_dimmer_light_frame.sequence++;
    s_hooch_protocol_dimmer_light_frame.valid = 1U;

    HOOCH_PROTOCOL_DimmerLight_NotifyReportCallback2();

    return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_OK;
}

/* ======================== 类型3 上报：地址+数值 ======================== */

HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendSwitchAddr(
    uint16_t address,
    HOOCH_PROTOCOL_DimmerLightSwitchState_t state)
{
    if (HOOCH_PROTOCOL_DimmerLight_IsValidSwitchState(state) == 0U)
    {
        return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_dimmer_light_frame.address = address;
    s_hooch_protocol_dimmer_light_frame.switch_state = (uint8_t)state;
    s_hooch_protocol_dimmer_light_frame.control_item =
        HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_SWITCH;
    s_hooch_protocol_dimmer_light_frame.sequence++;
    s_hooch_protocol_dimmer_light_frame.valid = 1U;

    HOOCH_PROTOCOL_DimmerLight_NotifyReportCallback3();

    return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_OK;
}

HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendBrightnessAddr(
    uint16_t address,
    uint8_t brightness)
{
    s_hooch_protocol_dimmer_light_frame.address = address;
    s_hooch_protocol_dimmer_light_frame.brightness = brightness;
    s_hooch_protocol_dimmer_light_frame.control_item =
        HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_BRIGHTNESS;
    s_hooch_protocol_dimmer_light_frame.sequence++;
    s_hooch_protocol_dimmer_light_frame.valid = 1U;

    HOOCH_PROTOCOL_DimmerLight_NotifyReportCallback3();

    return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_OK;
}

HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendColorTemperatureAddr(
    uint16_t address,
    uint8_t color_temperature)
{
    s_hooch_protocol_dimmer_light_frame.address = address;
    s_hooch_protocol_dimmer_light_frame.color_temperature = color_temperature;
    s_hooch_protocol_dimmer_light_frame.control_item =
        HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_COLOR_TEMP;
    s_hooch_protocol_dimmer_light_frame.sequence++;
    s_hooch_protocol_dimmer_light_frame.valid = 1U;

    HOOCH_PROTOCOL_DimmerLight_NotifyReportCallback3();

    return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_OK;
}

