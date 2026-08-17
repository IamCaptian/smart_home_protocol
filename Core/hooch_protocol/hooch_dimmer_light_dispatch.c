#include "hooch_dimmer_light.h"

static HOOCH_PROTOCOL_DimmerLightFrame_t s_hooch_protocol_dimmer_light_dispatch_frame;

static HOOCH_PROTOCOL_DimmerLightCallback_t s_hooch_protocol_dimmer_light_dispatch_callback;

static void HOOCH_PROTOCOL_DimmerLight_NotifyDispatchCallback(void);

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

static void HOOCH_PROTOCOL_DimmerLight_NotifyDispatchCallback(void)
{
    if (s_hooch_protocol_dimmer_light_dispatch_callback != 0)
    {
        s_hooch_protocol_dimmer_light_dispatch_callback(&s_hooch_protocol_dimmer_light_dispatch_frame);
    }
}

void HOOCH_PROTOCOL_DimmerLight_RegisterCallback(
    HOOCH_PROTOCOL_DimmerLightCallback_t callback)
{
    s_hooch_protocol_dimmer_light_dispatch_callback = callback;
}

void HOOCH_PROTOCOL_DimmerLight_UnregisterCallback(void)
{
    s_hooch_protocol_dimmer_light_dispatch_callback = 0;
}

HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_DispatchSwitch(
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    HOOCH_PROTOCOL_DimmerLightSwitchState_t state)
{
    if ((HOOCH_PROTOCOL_DimmerLight_IsValidKey(key) == 0U) ||
        (HOOCH_PROTOCOL_DimmerLight_IsValidSwitchState(state) == 0U))
    {
        return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_dimmer_light_dispatch_frame.key = key;
    s_hooch_protocol_dimmer_light_dispatch_frame.switch_state = (uint8_t)state;
    s_hooch_protocol_dimmer_light_dispatch_frame.control_item =
        HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_SWITCH;
    s_hooch_protocol_dimmer_light_dispatch_frame.sequence++;
    s_hooch_protocol_dimmer_light_dispatch_frame.valid = 1U;

    HOOCH_PROTOCOL_DimmerLight_NotifyDispatchCallback();

    return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_OK;
}

HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_DispatchBrightness(
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    uint8_t brightness)
{
    if (HOOCH_PROTOCOL_DimmerLight_IsValidKey(key) == 0U)
    {
        return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_dimmer_light_dispatch_frame.key = key;
    s_hooch_protocol_dimmer_light_dispatch_frame.brightness = brightness;
    s_hooch_protocol_dimmer_light_dispatch_frame.control_item =
        HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_BRIGHTNESS;
    s_hooch_protocol_dimmer_light_dispatch_frame.sequence++;
    s_hooch_protocol_dimmer_light_dispatch_frame.valid = 1U;

    HOOCH_PROTOCOL_DimmerLight_NotifyDispatchCallback();

    return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_OK;
}

HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_DispatchColorTemperature(
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    uint8_t color_temperature)
{
    if (HOOCH_PROTOCOL_DimmerLight_IsValidKey(key) == 0U)
    {
        return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_dimmer_light_dispatch_frame.key = key;
    s_hooch_protocol_dimmer_light_dispatch_frame.color_temperature = color_temperature;
    s_hooch_protocol_dimmer_light_dispatch_frame.control_item =
        HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_COLOR_TEMP;
    s_hooch_protocol_dimmer_light_dispatch_frame.sequence++;
    s_hooch_protocol_dimmer_light_dispatch_frame.valid = 1U;

    HOOCH_PROTOCOL_DimmerLight_NotifyDispatchCallback();

    return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_OK;
}

HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_DispatchFrame(
    const HOOCH_PROTOCOL_DimmerLightFrame_t *frame)
{
    if ((frame == 0) ||
        (HOOCH_PROTOCOL_DimmerLight_IsValidKey(frame->key) == 0U))
    {
        return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_dimmer_light_dispatch_frame.key = frame->key;
    s_hooch_protocol_dimmer_light_dispatch_frame.brightness = frame->brightness;
    s_hooch_protocol_dimmer_light_dispatch_frame.color_temperature = frame->color_temperature;
    s_hooch_protocol_dimmer_light_dispatch_frame.switch_state = frame->switch_state;
    s_hooch_protocol_dimmer_light_dispatch_frame.control_item = frame->control_item;
    s_hooch_protocol_dimmer_light_dispatch_frame.sequence++;
    s_hooch_protocol_dimmer_light_dispatch_frame.valid = 1U;

    HOOCH_PROTOCOL_DimmerLight_NotifyDispatchCallback();

    return HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_OK;
}

