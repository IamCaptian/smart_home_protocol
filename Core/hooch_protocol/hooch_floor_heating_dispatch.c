#include "hooch_floor_heating.h"

/* 保存当前一次地暖下发数据。 */
static HOOCH_PROTOCOL_FloorHeatingFrame_t s_hooch_protocol_floor_heating_dispatch_frame;

/* 保存外部注册的地暖下发回调。 */
static HOOCH_PROTOCOL_FloorHeatingCallback_t s_hooch_protocol_floor_heating_dispatch_callback;

/* 校验地暖温度是否在当前模块支持的范围内。 */
static uint8_t HOOCH_PROTOCOL_FloorHeating_IsValidTemperature(uint8_t temperature)
{
    return (uint8_t)(temperature <= 60U);
}

/* 有下发回调注册时，把最新数据通知出去。 */
static void HOOCH_PROTOCOL_FloorHeating_NotifyDispatchCallback(void)
{
    if (s_hooch_protocol_floor_heating_dispatch_callback != 0)
    {
        s_hooch_protocol_floor_heating_dispatch_callback(
            &s_hooch_protocol_floor_heating_dispatch_frame);
    }
}

/* 统一写入地暖下发状态。 */
static HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SetDispatchState(
    HOOCH_PROTOCOL_FloorHeatingPower_t power,
    HOOCH_PROTOCOL_FloorHeatingMode_t mode,
    uint8_t target_temperature,
    uint8_t current_temperature)
{
    if ((HOOCH_PROTOCOL_FloorHeating_IsValidPower(power) == 0U) ||
        (HOOCH_PROTOCOL_FloorHeating_IsValidMode(mode) == 0U) ||
        (HOOCH_PROTOCOL_FloorHeating_IsValidTemperature(target_temperature) == 0U) ||
        (HOOCH_PROTOCOL_FloorHeating_IsValidTemperature(current_temperature) == 0U))
    {
        return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_floor_heating_dispatch_frame.power = power;
    s_hooch_protocol_floor_heating_dispatch_frame.mode = mode;
    s_hooch_protocol_floor_heating_dispatch_frame.target_temperature = target_temperature;
    s_hooch_protocol_floor_heating_dispatch_frame.current_temperature = current_temperature;
    s_hooch_protocol_floor_heating_dispatch_frame.channel = 0U;
    s_hooch_protocol_floor_heating_dispatch_frame.control_item =
        HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL;
    s_hooch_protocol_floor_heating_dispatch_frame.sequence++;
    s_hooch_protocol_floor_heating_dispatch_frame.valid = 1U;

    HOOCH_PROTOCOL_FloorHeating_NotifyDispatchCallback();

    return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_OK;
}

/* 统一按 control_item 写入地暖下发帧。 */
static HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SetDispatchFrameInternal(
    const HOOCH_PROTOCOL_FloorHeatingFrame_t *frame)
{
    if (frame == 0)
    {
        return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_INVALID_PARAM;
    }

    if (HOOCH_PROTOCOL_FloorHeating_IsValidControlItem(frame->control_item) == 0U)
    {
        return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_INVALID_PARAM;
    }

    switch (frame->control_item)
    {
        case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL:
            if ((HOOCH_PROTOCOL_FloorHeating_IsValidPower(frame->power) == 0U) ||
                (HOOCH_PROTOCOL_FloorHeating_IsValidMode(frame->mode) == 0U) ||
                (HOOCH_PROTOCOL_FloorHeating_IsValidTemperature(frame->target_temperature) == 0U) ||
                (HOOCH_PROTOCOL_FloorHeating_IsValidTemperature(frame->current_temperature) == 0U))
            {
                return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_floor_heating_dispatch_frame.power = frame->power;
            s_hooch_protocol_floor_heating_dispatch_frame.mode = frame->mode;
            s_hooch_protocol_floor_heating_dispatch_frame.target_temperature = frame->target_temperature;
            s_hooch_protocol_floor_heating_dispatch_frame.current_temperature = frame->current_temperature;
            break;

        case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_POWER:
            if (HOOCH_PROTOCOL_FloorHeating_IsValidPower(frame->power) == 0U)
            {
                return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_floor_heating_dispatch_frame.power = frame->power;
            break;

        case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_MODE:
            if (HOOCH_PROTOCOL_FloorHeating_IsValidMode(frame->mode) == 0U)
            {
                return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_floor_heating_dispatch_frame.mode = frame->mode;
            break;

        case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TARGET_TEMPERATURE:
            if (HOOCH_PROTOCOL_FloorHeating_IsValidTemperature(frame->target_temperature) == 0U)
            {
                return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_floor_heating_dispatch_frame.target_temperature = frame->target_temperature;
            break;

        case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_CURRENT_TEMPERATURE:
            if (HOOCH_PROTOCOL_FloorHeating_IsValidTemperature(frame->current_temperature) == 0U)
            {
                return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_floor_heating_dispatch_frame.current_temperature = frame->current_temperature;
            break;

        case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_DEVICE_DESC:
            (void)memcpy(s_hooch_protocol_floor_heating_dispatch_frame.device_desc,
                         frame->device_desc,
                         sizeof(s_hooch_protocol_floor_heating_dispatch_frame.device_desc));
            break;

        case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_DEFAULT_ICON:
            s_hooch_protocol_floor_heating_dispatch_frame.value = frame->value;
            break;

        case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_SELECTED_ICON:
            s_hooch_protocol_floor_heating_dispatch_frame.value = frame->value;
            break;

        case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TEMP_STEP:
            s_hooch_protocol_floor_heating_dispatch_frame.value = frame->value;
            break;

        case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TEMP_MIN:
            s_hooch_protocol_floor_heating_dispatch_frame.value = frame->value;
            break;

        case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TEMP_MAX:
            s_hooch_protocol_floor_heating_dispatch_frame.value = frame->value;
            break;

        default:
            return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_floor_heating_dispatch_frame.channel = frame->channel;
    s_hooch_protocol_floor_heating_dispatch_frame.control_item = frame->control_item;
    s_hooch_protocol_floor_heating_dispatch_frame.sequence = frame->sequence;
    s_hooch_protocol_floor_heating_dispatch_frame.valid = 1U;

    HOOCH_PROTOCOL_FloorHeating_NotifyDispatchCallback();

    return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_OK;
}

void HOOCH_PROTOCOL_FloorHeating_RegisterCallback(
    HOOCH_PROTOCOL_FloorHeatingCallback_t callback)
{
    s_hooch_protocol_floor_heating_dispatch_callback = callback;
}

void HOOCH_PROTOCOL_FloorHeating_UnregisterCallback(void)
{
    s_hooch_protocol_floor_heating_dispatch_callback = 0;
}

HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_DispatchFrame(
    const HOOCH_PROTOCOL_FloorHeatingFrame_t *frame)
{
    return HOOCH_PROTOCOL_FloorHeating_SetDispatchFrameInternal(frame);
}

/* 这个接口是下发全量控制  通道0就是默认 */
HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_Dispatch(
    HOOCH_PROTOCOL_FloorHeatingPower_t power,
    HOOCH_PROTOCOL_FloorHeatingMode_t mode,
    uint8_t target_temperature,
    uint8_t current_temperature)
{
    return HOOCH_PROTOCOL_FloorHeating_SetDispatchState(power, mode, target_temperature, current_temperature);
}
