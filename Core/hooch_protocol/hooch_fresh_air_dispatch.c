#include "hooch_fresh_air.h"

/* 保存当前一次新风下发数据。 */
static HOOCH_PROTOCOL_FreshAirFrame_t s_hooch_protocol_fresh_air_dispatch_frame;

/* 保存外部注册的新风下发回调。 */
static HOOCH_PROTOCOL_FreshAirCallback_t s_hooch_protocol_fresh_air_dispatch_callback;

/* 有下发回调注册时，把最新数据通知出去。 */
static void HOOCH_PROTOCOL_FreshAir_NotifyDispatchCallback(void)
{
    if (s_hooch_protocol_fresh_air_dispatch_callback != 0)
    {
        s_hooch_protocol_fresh_air_dispatch_callback(
            &s_hooch_protocol_fresh_air_dispatch_frame);
    }
}

/* 统一写入新风下发状态。 */
static HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SetDispatchState(
    HOOCH_PROTOCOL_FreshAirPower_t power,
    HOOCH_PROTOCOL_FreshAirMode_t mode,
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed)
{
    if ((HOOCH_PROTOCOL_FreshAir_IsValidPower(power) == 0U) ||
        (HOOCH_PROTOCOL_FreshAir_IsValidMode(mode) == 0U) ||
        (HOOCH_PROTOCOL_FreshAir_IsValidFanSpeed(fan_speed) == 0U))
    {
        return HOOCH_PROTOCOL_FRESH_AIR_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_fresh_air_dispatch_frame.power = power;
    s_hooch_protocol_fresh_air_dispatch_frame.mode = mode;
    s_hooch_protocol_fresh_air_dispatch_frame.fan_speed = fan_speed;
    s_hooch_protocol_fresh_air_dispatch_frame.channel = 0U;
    s_hooch_protocol_fresh_air_dispatch_frame.control_item =
        HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL;
    s_hooch_protocol_fresh_air_dispatch_frame.sequence++;
    s_hooch_protocol_fresh_air_dispatch_frame.valid = 1U;

    HOOCH_PROTOCOL_FreshAir_NotifyDispatchCallback();

    return HOOCH_PROTOCOL_FRESH_AIR_RESULT_OK;
}

/* 统一按 control_item 写入新风下发帧。 */
static HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SetDispatchFrameInternal(
    const HOOCH_PROTOCOL_FreshAirFrame_t *frame)
{
    if (frame == 0)
    {
        return HOOCH_PROTOCOL_FRESH_AIR_RESULT_INVALID_PARAM;
    }

    if (HOOCH_PROTOCOL_FreshAir_IsValidControlItem(frame->control_item) == 0U)
    {
        return HOOCH_PROTOCOL_FRESH_AIR_RESULT_INVALID_PARAM;
    }

    switch (frame->control_item)
    {
        case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL:
            if ((HOOCH_PROTOCOL_FreshAir_IsValidPower(frame->power) == 0U) ||
                (HOOCH_PROTOCOL_FreshAir_IsValidMode(frame->mode) == 0U) ||
                (HOOCH_PROTOCOL_FreshAir_IsValidFanSpeed(frame->fan_speed) == 0U))
            {
                return HOOCH_PROTOCOL_FRESH_AIR_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_fresh_air_dispatch_frame.power = frame->power;
            s_hooch_protocol_fresh_air_dispatch_frame.mode = frame->mode;
            s_hooch_protocol_fresh_air_dispatch_frame.fan_speed = frame->fan_speed;
            break;

        case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_POWER:
            if (HOOCH_PROTOCOL_FreshAir_IsValidPower(frame->power) == 0U)
            {
                return HOOCH_PROTOCOL_FRESH_AIR_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_fresh_air_dispatch_frame.power = frame->power;
            break;

        case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_MODE:
            if (HOOCH_PROTOCOL_FreshAir_IsValidMode(frame->mode) == 0U)
            {
                return HOOCH_PROTOCOL_FRESH_AIR_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_fresh_air_dispatch_frame.mode = frame->mode;
            break;

        case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED:
            if (HOOCH_PROTOCOL_FreshAir_IsValidFanSpeed(frame->fan_speed) == 0U)
            {
                return HOOCH_PROTOCOL_FRESH_AIR_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_fresh_air_dispatch_frame.fan_speed = frame->fan_speed;
            break;

        case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_CURRENT_TEMPERATURE:
            s_hooch_protocol_fresh_air_dispatch_frame.current_temperature = frame->current_temperature;
            break;

        case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_DEVICE_DESC:
            (void)memcpy(s_hooch_protocol_fresh_air_dispatch_frame.device_desc,
                         frame->device_desc,
                         sizeof(s_hooch_protocol_fresh_air_dispatch_frame.device_desc));
            break;

        case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_DEFAULT_ICON:
            s_hooch_protocol_fresh_air_dispatch_frame.value = frame->value;
            break;

        case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_SELECTED_ICON:
            s_hooch_protocol_fresh_air_dispatch_frame.value = frame->value;
            break;

        default:
            return HOOCH_PROTOCOL_FRESH_AIR_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_fresh_air_dispatch_frame.channel = frame->channel;
    s_hooch_protocol_fresh_air_dispatch_frame.control_item = frame->control_item;
    s_hooch_protocol_fresh_air_dispatch_frame.sequence = frame->sequence;
    s_hooch_protocol_fresh_air_dispatch_frame.valid = 1U;

    HOOCH_PROTOCOL_FreshAir_NotifyDispatchCallback();

    return HOOCH_PROTOCOL_FRESH_AIR_RESULT_OK;
}

void HOOCH_PROTOCOL_FreshAir_RegisterCallback(
    HOOCH_PROTOCOL_FreshAirCallback_t callback)
{
    s_hooch_protocol_fresh_air_dispatch_callback = callback;
}

void HOOCH_PROTOCOL_FreshAir_UnregisterCallback(void)
{
    s_hooch_protocol_fresh_air_dispatch_callback = 0;
}

HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_DispatchFrame(
    const HOOCH_PROTOCOL_FreshAirFrame_t *frame)
{
    return HOOCH_PROTOCOL_FreshAir_SetDispatchFrameInternal(frame);
}

/* 这个接口是下发全量控制  通道0就是默认 */
HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_Dispatch(
    HOOCH_PROTOCOL_FreshAirPower_t power,
    HOOCH_PROTOCOL_FreshAirMode_t mode,
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed)
{
    return HOOCH_PROTOCOL_FreshAir_SetDispatchState(power, mode, fan_speed);
}
