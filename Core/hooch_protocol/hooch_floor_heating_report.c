#include "hooch_floor_heating.h"

/* 保存当前一次地暖上报数据。 */
static HOOCH_PROTOCOL_FloorHeatingFrame_t s_hooch_protocol_floor_heating_report_frame;

/* 保存外部注册的地暖上报回调（通道版）。 */
static HOOCH_PROTOCOL_FloorHeatingCallback_t s_hooch_protocol_floor_heating_report_callback;

/* 保存外部注册的地暖上报回调（地址版）。 */
static HOOCH_PROTOCOL_FloorHeatingCallback_t s_hooch_protocol_floor_heating_report_callback2;

/* 校验地暖温度是否在当前模块支持的范围内。 */
static uint8_t HOOCH_PROTOCOL_FloorHeating_IsValidTemperature(uint8_t temperature)
{
    /* 温度限制暂时屏蔽，直接放行 */
    (void)temperature;
    return 1U;
}

/* 有上报回调注册时，把最新数据通知出去。 */
static void HOOCH_PROTOCOL_FloorHeating_NotifyReportCallback(void)
{
    if (s_hooch_protocol_floor_heating_report_callback != 0)
    {
        s_hooch_protocol_floor_heating_report_callback(
            &s_hooch_protocol_floor_heating_report_frame);
    }
}

/* 有地址版上报回调注册时，把最新数据通知出去。 */
static void HOOCH_PROTOCOL_FloorHeating_NotifyReportCallback2(void)
{
    if (s_hooch_protocol_floor_heating_report_callback2 != 0)
    {
        s_hooch_protocol_floor_heating_report_callback2(
            &s_hooch_protocol_floor_heating_report_frame);
    }
}

/* 统一清空一帧地暖数据。 */
static void HOOCH_PROTOCOL_FloorHeating_ClearFrame(
    HOOCH_PROTOCOL_FloorHeatingFrame_t *frame)
{
    if (frame == 0)
    {
        return;
    }

    frame->channel = 0U;
    frame->address = 0U;
    frame->power = HOOCH_PROTOCOL_FLOOR_HEATING_POWER_OFF;
    frame->mode = HOOCH_PROTOCOL_FLOOR_HEATING_MODE_INVALID;
    frame->target_temperature = 0U;
    frame->current_temperature = 0U;
    frame->control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_INVALID;
    frame->sequence = 0U;
    frame->valid = 0U;
}

uint8_t HOOCH_PROTOCOL_FloorHeating_IsValidPower(HOOCH_PROTOCOL_FloorHeatingPower_t power)
{
    return (uint8_t)((power == HOOCH_PROTOCOL_FLOOR_HEATING_POWER_OFF) ||
                     (power == HOOCH_PROTOCOL_FLOOR_HEATING_POWER_ON));
}

uint8_t HOOCH_PROTOCOL_FloorHeating_IsValidMode(HOOCH_PROTOCOL_FloorHeatingMode_t mode)
{
    return (uint8_t)((mode >= HOOCH_PROTOCOL_FLOOR_HEATING_MODE_MANUAL) &&
                     (mode <= HOOCH_PROTOCOL_FLOOR_HEATING_MODE_ECO));
}

uint8_t HOOCH_PROTOCOL_FloorHeating_IsValidControlItem(
    HOOCH_PROTOCOL_FloorHeatingControlItem_t control_item)
{
    return (uint8_t)((control_item >= HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL) &&
                     (control_item < HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_MAX));
}

/* 统一写入地暖上报状态。 */
static HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SetReportState(
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

    s_hooch_protocol_floor_heating_report_frame.power = power;
    s_hooch_protocol_floor_heating_report_frame.mode = mode;
    s_hooch_protocol_floor_heating_report_frame.target_temperature = target_temperature;
    s_hooch_protocol_floor_heating_report_frame.current_temperature = current_temperature;
    s_hooch_protocol_floor_heating_report_frame.channel = 0U;
    s_hooch_protocol_floor_heating_report_frame.control_item =
        HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL;
    s_hooch_protocol_floor_heating_report_frame.sequence++;
    s_hooch_protocol_floor_heating_report_frame.valid = 1U;

    HOOCH_PROTOCOL_FloorHeating_NotifyReportCallback();

    return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_OK;
}

/* 统一按 control_item 写入地暖上报帧。 */
static HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SetReportFrame(
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
        case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL_KNX:
        case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL_XIAOMI:
            if ((HOOCH_PROTOCOL_FloorHeating_IsValidPower(frame->power) == 0U) ||
                (HOOCH_PROTOCOL_FloorHeating_IsValidMode(frame->mode) == 0U) ||
                (HOOCH_PROTOCOL_FloorHeating_IsValidTemperature(frame->target_temperature) == 0U) ||
                (HOOCH_PROTOCOL_FloorHeating_IsValidTemperature(frame->current_temperature) == 0U))
            {
                return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_floor_heating_report_frame.power = frame->power;
            s_hooch_protocol_floor_heating_report_frame.mode = frame->mode;
            s_hooch_protocol_floor_heating_report_frame.target_temperature = frame->target_temperature;
            s_hooch_protocol_floor_heating_report_frame.current_temperature = frame->current_temperature;
            break;

        case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_POWER:
            if (HOOCH_PROTOCOL_FloorHeating_IsValidPower(frame->power) == 0U)
            {
                return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_floor_heating_report_frame.power = frame->power;
            break;

        case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_MODE:
            if (HOOCH_PROTOCOL_FloorHeating_IsValidMode(frame->mode) == 0U)
            {
                return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_floor_heating_report_frame.mode = frame->mode;
            break;

        case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TARGET_TEMPERATURE:
            if (HOOCH_PROTOCOL_FloorHeating_IsValidTemperature(frame->target_temperature) == 0U)
            {
                return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_floor_heating_report_frame.target_temperature = frame->target_temperature;
            break;

        case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_CURRENT_TEMPERATURE:
            if (HOOCH_PROTOCOL_FloorHeating_IsValidTemperature(frame->current_temperature) == 0U)
            {
                return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_floor_heating_report_frame.current_temperature = frame->current_temperature;
            break;

        default:
            return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_floor_heating_report_frame.channel = frame->channel;
    s_hooch_protocol_floor_heating_report_frame.address = frame->address;
    s_hooch_protocol_floor_heating_report_frame.control_item = frame->control_item;
    s_hooch_protocol_floor_heating_report_frame.sequence = frame->sequence;
    s_hooch_protocol_floor_heating_report_frame.valid = 1U;

    /* 通知通道版回调 */
    HOOCH_PROTOCOL_FloorHeating_NotifyReportCallback();

    /* 地址非零时通知地址版回调 */
    if (frame->address != 0U)
    {
        HOOCH_PROTOCOL_FloorHeating_NotifyReportCallback2();
    }

    return HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_OK;
}

void HOOCH_FloorHeating_Init(void)
{
    HOOCH_PROTOCOL_FloorHeating_UnregisterCallback();
    HOOCH_PROTOCOL_FloorHeating_UnregisterReportCallback();
    HOOCH_PROTOCOL_FloorHeating_UnregisterReportCallback2();
    HOOCH_PROTOCOL_FloorHeating_Clear();
}

void HOOCH_PROTOCOL_FloorHeating_Clear(void)
{
    HOOCH_PROTOCOL_FloorHeating_ClearFrame(&s_hooch_protocol_floor_heating_report_frame);
}

HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_Set(
    HOOCH_PROTOCOL_FloorHeatingPower_t power,
    HOOCH_PROTOCOL_FloorHeatingMode_t mode,
    uint8_t target_temperature,
    uint8_t current_temperature)
{
    return HOOCH_PROTOCOL_FloorHeating_SetReportState(power, mode, target_temperature, current_temperature);
}

HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SetFrame(
    const HOOCH_PROTOCOL_FloorHeatingFrame_t *frame)
{
    return HOOCH_PROTOCOL_FloorHeating_SetReportFrame(frame);
}

const HOOCH_PROTOCOL_FloorHeatingFrame_t *HOOCH_PROTOCOL_FloorHeating_GetFrame(void)
{
    return &s_hooch_protocol_floor_heating_report_frame;
}

void HOOCH_PROTOCOL_FloorHeating_RegisterReportCallback(
    HOOCH_PROTOCOL_FloorHeatingCallback_t callback)
{
    s_hooch_protocol_floor_heating_report_callback = callback;
}

void HOOCH_PROTOCOL_FloorHeating_UnregisterReportCallback(void)
{
    s_hooch_protocol_floor_heating_report_callback = 0;
}

void HOOCH_PROTOCOL_FloorHeating_RegisterReportCallback2(
    HOOCH_PROTOCOL_FloorHeatingCallback_t callback)
{
    s_hooch_protocol_floor_heating_report_callback2 = callback;
}

void HOOCH_PROTOCOL_FloorHeating_UnregisterReportCallback2(void)
{
    s_hooch_protocol_floor_heating_report_callback2 = 0;
}

HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_Send(
    uint8_t channel,
    HOOCH_PROTOCOL_FloorHeatingPower_t power,
    HOOCH_PROTOCOL_FloorHeatingMode_t mode,
    uint8_t target_temperature,
    uint8_t current_temperature)
{
    HOOCH_PROTOCOL_FloorHeatingFrame_t frame;

    HOOCH_PROTOCOL_FloorHeating_ClearFrame(&frame);
    frame.channel = channel;
    frame.power = power;
    frame.mode = mode;
    frame.target_temperature = target_temperature;
    frame.current_temperature = current_temperature;
    frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL;
    frame.sequence = (uint8_t)(s_hooch_protocol_floor_heating_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FloorHeating_SetFrame(&frame);
}

HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendPower(
    uint8_t channel,
    HOOCH_PROTOCOL_FloorHeatingPower_t power)
{
    HOOCH_PROTOCOL_FloorHeatingFrame_t frame;

    HOOCH_PROTOCOL_FloorHeating_ClearFrame(&frame);
    frame.channel = channel;
    frame.power = power;
    frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_POWER;
    frame.sequence = (uint8_t)(s_hooch_protocol_floor_heating_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FloorHeating_SetFrame(&frame);
}

HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendMode(
    uint8_t channel,
    HOOCH_PROTOCOL_FloorHeatingMode_t mode)
{
    HOOCH_PROTOCOL_FloorHeatingFrame_t frame;

    HOOCH_PROTOCOL_FloorHeating_ClearFrame(&frame);
    frame.channel = channel;
    frame.mode = mode;
    frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_MODE;
    frame.sequence = (uint8_t)(s_hooch_protocol_floor_heating_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FloorHeating_SetFrame(&frame);
}

HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendTargetTemperature(
    uint8_t channel,
    uint8_t target_temperature)
{
    HOOCH_PROTOCOL_FloorHeatingFrame_t frame;

    HOOCH_PROTOCOL_FloorHeating_ClearFrame(&frame);
    frame.channel = channel;
    frame.target_temperature = target_temperature;
    frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TARGET_TEMPERATURE;
    frame.sequence = (uint8_t)(s_hooch_protocol_floor_heating_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FloorHeating_SetFrame(&frame);
}

HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendCurrentTemperature(
    uint8_t channel,
    uint8_t current_temperature)
{
    HOOCH_PROTOCOL_FloorHeatingFrame_t frame;

    HOOCH_PROTOCOL_FloorHeating_ClearFrame(&frame);
    frame.channel = channel;
    frame.current_temperature = current_temperature;
    frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_CURRENT_TEMPERATURE;
    frame.sequence = (uint8_t)(s_hooch_protocol_floor_heating_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FloorHeating_SetFrame(&frame);
}

HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendPowerAddr(
    uint16_t address,
    HOOCH_PROTOCOL_FloorHeatingPower_t power)
{
    HOOCH_PROTOCOL_FloorHeatingFrame_t frame;

    HOOCH_PROTOCOL_FloorHeating_ClearFrame(&frame);
    frame.address = address;
    frame.power = power;
    frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_POWER;
    frame.sequence = (uint8_t)(s_hooch_protocol_floor_heating_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FloorHeating_SetFrame(&frame);
}

HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendModeAddr(
    uint16_t address,
    HOOCH_PROTOCOL_FloorHeatingMode_t mode)
{
    HOOCH_PROTOCOL_FloorHeatingFrame_t frame;

    HOOCH_PROTOCOL_FloorHeating_ClearFrame(&frame);
    frame.address = address;
    frame.mode = mode;
    frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_MODE;
    frame.sequence = (uint8_t)(s_hooch_protocol_floor_heating_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FloorHeating_SetFrame(&frame);
}

HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendTargetTemperatureAddr(
    uint16_t address,
    uint8_t target_temperature)
{
    HOOCH_PROTOCOL_FloorHeatingFrame_t frame;

    HOOCH_PROTOCOL_FloorHeating_ClearFrame(&frame);
    frame.address = address;
    frame.target_temperature = target_temperature;
    frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TARGET_TEMPERATURE;
    frame.sequence = (uint8_t)(s_hooch_protocol_floor_heating_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FloorHeating_SetFrame(&frame);
}

HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendCurrentTemperatureAddr(
    uint16_t address,
    uint8_t current_temperature)
{
    HOOCH_PROTOCOL_FloorHeatingFrame_t frame;

    HOOCH_PROTOCOL_FloorHeating_ClearFrame(&frame);
    frame.address = address;
    frame.current_temperature = current_temperature;
    frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_CURRENT_TEMPERATURE;
    frame.sequence = (uint8_t)(s_hooch_protocol_floor_heating_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FloorHeating_SetFrame(&frame);
}

HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendFrame(
    uint8_t channel,
    HOOCH_PROTOCOL_FloorHeatingPower_t power,
    HOOCH_PROTOCOL_FloorHeatingMode_t mode,
    uint8_t target_temperature,
    uint8_t current_temperature)
{
    HOOCH_PROTOCOL_FloorHeatingFrame_t frame;

    HOOCH_PROTOCOL_FloorHeating_ClearFrame(&frame);
    frame.channel = channel;
    frame.power = power;
    frame.mode = mode;
    frame.target_temperature = target_temperature;
    frame.current_temperature = current_temperature;
    frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL_XIAOMI;
    frame.sequence = (uint8_t)(s_hooch_protocol_floor_heating_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FloorHeating_SetFrame(&frame);
}

HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendFrameAddr(
    uint16_t address,
    HOOCH_PROTOCOL_FloorHeatingPower_t power,
    HOOCH_PROTOCOL_FloorHeatingMode_t mode,
    uint8_t target_temperature,
    uint8_t current_temperature)
{
    HOOCH_PROTOCOL_FloorHeatingFrame_t frame;

    HOOCH_PROTOCOL_FloorHeating_ClearFrame(&frame);
    frame.address = address;
    frame.power = power;
    frame.mode = mode;
    frame.target_temperature = target_temperature;
    frame.current_temperature = current_temperature;
    frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL_XIAOMI;
    frame.sequence = (uint8_t)(s_hooch_protocol_floor_heating_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FloorHeating_SetFrame(&frame);
}
