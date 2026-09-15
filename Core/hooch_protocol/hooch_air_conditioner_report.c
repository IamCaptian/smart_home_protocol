#include "hooch_air_conditioner.h"

/* 保存当前一次空调上报数据。 */
static HOOCH_PROTOCOL_AirConditionerFrame_t s_hooch_protocol_air_conditioner_report_frame;

/* 保存外部注册的空调上报回调（通道版）。 */
static HOOCH_PROTOCOL_AirConditionerCallback_t s_hooch_protocol_air_conditioner_report_callback;

/* 保存外部注册的空调上报回调（地址版）。 */
static HOOCH_PROTOCOL_AirConditionerCallback_t s_hooch_protocol_air_conditioner_report_callback2;

/* 校验空调温度是否在当前模块支持的范围内。 */
static uint8_t HOOCH_PROTOCOL_AirConditioner_IsValidTemperature(uint8_t temperature)
{
    /* 温度限制暂时屏蔽，直接放行 */
    (void)temperature;
    return 1U;
}

/* 有上报回调注册时，把最新数据通知出去。 */
static void HOOCH_PROTOCOL_AirConditioner_NotifyReportCallback(void)
{
    if (s_hooch_protocol_air_conditioner_report_callback != 0)
    {
        s_hooch_protocol_air_conditioner_report_callback(
            &s_hooch_protocol_air_conditioner_report_frame);
    }
}

/* 有地址版上报回调注册时，把最新数据通知出去。 */
static void HOOCH_PROTOCOL_AirConditioner_NotifyReportCallback2(void)
{
    if (s_hooch_protocol_air_conditioner_report_callback2 != 0)
    {
        s_hooch_protocol_air_conditioner_report_callback2(
            &s_hooch_protocol_air_conditioner_report_frame);
    }
}

/* 统一清空一帧空调数据。 */
static void HOOCH_PROTOCOL_AirConditioner_ClearFrame(
    HOOCH_PROTOCOL_AirConditionerFrame_t *frame)
{
    if (frame == 0)
    {
        return;
    }

    frame->channel = 0U;
    frame->address = 0U;
    frame->power = HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_OFF;
    frame->mode = HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_INVALID;
    frame->fan_speed = HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_INVALID;
    frame->temperature = 16U;
    frame->control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_INVALID;
    frame->sequence = 0U;
    frame->valid = 0U;
}

uint8_t HOOCH_PROTOCOL_AirConditioner_IsValidPower(HOOCH_PROTOCOL_AirConditionerPower_t power)
{
    return (uint8_t)((power == HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_OFF) ||
                     (power == HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_ON));
}

uint8_t HOOCH_PROTOCOL_AirConditioner_IsValidMode(HOOCH_PROTOCOL_AirConditionerMode_t mode)
{
    return (uint8_t)((mode >= HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_COOL) &&
                     (mode <= HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_AUTO));
}

uint8_t HOOCH_PROTOCOL_AirConditioner_IsValidFanSpeed(
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed)
{
    return (uint8_t)((fan_speed >= HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_LOW) &&
                     (fan_speed <= HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_AUTO));
}

uint8_t HOOCH_PROTOCOL_AirConditioner_IsValidControlItem(
    HOOCH_PROTOCOL_AirConditionerControlItem_t control_item)
{
    return (uint8_t)((control_item >= HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL) &&
                     (control_item < HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MAX));
}

/* 统一写入空调上报状态。 */
static HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SetReportState(
    HOOCH_PROTOCOL_AirConditionerPower_t power,
    HOOCH_PROTOCOL_AirConditionerMode_t mode,
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed,
    uint8_t temperature)
{
    if ((HOOCH_PROTOCOL_AirConditioner_IsValidPower(power) == 0U) ||
        (HOOCH_PROTOCOL_AirConditioner_IsValidMode(mode) == 0U) ||
        (HOOCH_PROTOCOL_AirConditioner_IsValidFanSpeed(fan_speed) == 0U) ||
        (HOOCH_PROTOCOL_AirConditioner_IsValidTemperature(temperature) == 0U))
    {
        return HOOCH_PROTOCOL_AIR_CONDITIONER_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_air_conditioner_report_frame.power = power;
    s_hooch_protocol_air_conditioner_report_frame.mode = mode;
    s_hooch_protocol_air_conditioner_report_frame.fan_speed = fan_speed;
    s_hooch_protocol_air_conditioner_report_frame.temperature = temperature;
    s_hooch_protocol_air_conditioner_report_frame.channel = 0U;
    s_hooch_protocol_air_conditioner_report_frame.control_item =
        HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL;
    s_hooch_protocol_air_conditioner_report_frame.sequence++;
    s_hooch_protocol_air_conditioner_report_frame.valid = 1U;

    HOOCH_PROTOCOL_AirConditioner_NotifyReportCallback();

    return HOOCH_PROTOCOL_AIR_CONDITIONER_RESULT_OK;
}

/* 统一按 control_item 写入空调上报帧。 */
static HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SetReportFrame(
    const HOOCH_PROTOCOL_AirConditionerFrame_t *frame)
{
    if (frame == 0)
    {
        return HOOCH_PROTOCOL_AIR_CONDITIONER_RESULT_INVALID_PARAM;
    }

    if (HOOCH_PROTOCOL_AirConditioner_IsValidControlItem(frame->control_item) == 0U)
    {
        return HOOCH_PROTOCOL_AIR_CONDITIONER_RESULT_INVALID_PARAM;
    }

    switch (frame->control_item)
    {
        case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL:
        case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_KNX:
        case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_XIAOMI:
            if ((HOOCH_PROTOCOL_AirConditioner_IsValidPower(frame->power) == 0U) ||
                (HOOCH_PROTOCOL_AirConditioner_IsValidMode(frame->mode) == 0U) ||
                (HOOCH_PROTOCOL_AirConditioner_IsValidFanSpeed(frame->fan_speed) == 0U) ||
                (HOOCH_PROTOCOL_AirConditioner_IsValidTemperature(frame->temperature) == 0U))
            {
                return HOOCH_PROTOCOL_AIR_CONDITIONER_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_air_conditioner_report_frame.power = frame->power;
            s_hooch_protocol_air_conditioner_report_frame.mode = frame->mode;
            s_hooch_protocol_air_conditioner_report_frame.fan_speed = frame->fan_speed;
            s_hooch_protocol_air_conditioner_report_frame.temperature = frame->temperature;
            break;

        case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_POWER:
            if (HOOCH_PROTOCOL_AirConditioner_IsValidPower(frame->power) == 0U)
            {
                return HOOCH_PROTOCOL_AIR_CONDITIONER_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_air_conditioner_report_frame.power = frame->power;
            break;

        case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MODE:
            if (HOOCH_PROTOCOL_AirConditioner_IsValidMode(frame->mode) == 0U)
            {
                return HOOCH_PROTOCOL_AIR_CONDITIONER_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_air_conditioner_report_frame.mode = frame->mode;
            break;

        case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_FAN_SPEED:
            if (HOOCH_PROTOCOL_AirConditioner_IsValidFanSpeed(frame->fan_speed) == 0U)
            {
                return HOOCH_PROTOCOL_AIR_CONDITIONER_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_air_conditioner_report_frame.fan_speed = frame->fan_speed;
            break;

        case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMPERATURE:
            if (HOOCH_PROTOCOL_AirConditioner_IsValidTemperature(frame->temperature) == 0U)
            {
                return HOOCH_PROTOCOL_AIR_CONDITIONER_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_air_conditioner_report_frame.temperature = frame->temperature;
            break;

        default:
            return HOOCH_PROTOCOL_AIR_CONDITIONER_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_air_conditioner_report_frame.channel = frame->channel;
    s_hooch_protocol_air_conditioner_report_frame.address = frame->address;
    s_hooch_protocol_air_conditioner_report_frame.control_item = frame->control_item;
    s_hooch_protocol_air_conditioner_report_frame.sequence = frame->sequence;
    s_hooch_protocol_air_conditioner_report_frame.valid = 1U;

    /* 通知通道版回调 */
    HOOCH_PROTOCOL_AirConditioner_NotifyReportCallback();

    /* 地址非零时通知地址版回调 */
    if (frame->address != 0U)
    {
        HOOCH_PROTOCOL_AirConditioner_NotifyReportCallback2();
    }

    return HOOCH_PROTOCOL_AIR_CONDITIONER_RESULT_OK;
}

void HOOCH_AirConditioner_Init(void)
{
    HOOCH_PROTOCOL_AirConditioner_UnregisterCallback();
    HOOCH_PROTOCOL_AirConditioner_UnregisterReportCallback();
    HOOCH_PROTOCOL_AirConditioner_UnregisterReportCallback2();
    HOOCH_PROTOCOL_AirConditioner_Clear();
}

void HOOCH_PROTOCOL_AirConditioner_Clear(void)
{
    HOOCH_PROTOCOL_AirConditioner_ClearFrame(&s_hooch_protocol_air_conditioner_report_frame);
}

HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_Set(
    HOOCH_PROTOCOL_AirConditionerPower_t power,
    HOOCH_PROTOCOL_AirConditionerMode_t mode,
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed,
    uint8_t temperature)
{
    return HOOCH_PROTOCOL_AirConditioner_SetReportState(power, mode, fan_speed, temperature);
}

HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SetFrame(
    const HOOCH_PROTOCOL_AirConditionerFrame_t *frame)
{
    return HOOCH_PROTOCOL_AirConditioner_SetReportFrame(frame);
}

const HOOCH_PROTOCOL_AirConditionerFrame_t *HOOCH_PROTOCOL_AirConditioner_GetFrame(void)
{
    return &s_hooch_protocol_air_conditioner_report_frame;
}

void HOOCH_PROTOCOL_AirConditioner_RegisterReportCallback(
    HOOCH_PROTOCOL_AirConditionerCallback_t callback)
{
    s_hooch_protocol_air_conditioner_report_callback = callback;
}

void HOOCH_PROTOCOL_AirConditioner_UnregisterReportCallback(void)
{
    s_hooch_protocol_air_conditioner_report_callback = 0;
}

void HOOCH_PROTOCOL_AirConditioner_RegisterReportCallback2(
    HOOCH_PROTOCOL_AirConditionerCallback_t callback)
{
    s_hooch_protocol_air_conditioner_report_callback2 = callback;
}

void HOOCH_PROTOCOL_AirConditioner_UnregisterReportCallback2(void)
{
    s_hooch_protocol_air_conditioner_report_callback2 = 0;
}

HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_Send(
    uint8_t channel,
    HOOCH_PROTOCOL_AirConditionerPower_t power,
    HOOCH_PROTOCOL_AirConditionerMode_t mode,
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed,
    uint8_t temperature)
{
    HOOCH_PROTOCOL_AirConditionerFrame_t frame;

    HOOCH_PROTOCOL_AirConditioner_ClearFrame(&frame);
    frame.channel = channel;
    frame.power = power;
    frame.mode = mode;
    frame.fan_speed = fan_speed;
    frame.temperature = temperature;
    frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL;
    frame.sequence = (uint8_t)(s_hooch_protocol_air_conditioner_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_AirConditioner_SetFrame(&frame);
}

HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendPower(
    uint8_t channel,
    HOOCH_PROTOCOL_AirConditionerPower_t power)
{
    HOOCH_PROTOCOL_AirConditionerFrame_t frame;

    HOOCH_PROTOCOL_AirConditioner_ClearFrame(&frame);
    frame.channel = channel;
    frame.power = power;
    frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_POWER;
    frame.sequence = (uint8_t)(s_hooch_protocol_air_conditioner_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_AirConditioner_SetFrame(&frame);
}

HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendMode(
    uint8_t channel,
    HOOCH_PROTOCOL_AirConditionerMode_t mode)
{
    HOOCH_PROTOCOL_AirConditionerFrame_t frame;

    HOOCH_PROTOCOL_AirConditioner_ClearFrame(&frame);
    frame.channel = channel;
    frame.mode = mode;
    frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MODE;
    frame.sequence = (uint8_t)(s_hooch_protocol_air_conditioner_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_AirConditioner_SetFrame(&frame);
}

HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendFanSpeed(
    uint8_t channel,
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed)
{
    HOOCH_PROTOCOL_AirConditionerFrame_t frame;

    HOOCH_PROTOCOL_AirConditioner_ClearFrame(&frame);
    frame.channel = channel;
    frame.fan_speed = fan_speed;
    frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_FAN_SPEED;
    frame.sequence = (uint8_t)(s_hooch_protocol_air_conditioner_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_AirConditioner_SetFrame(&frame);
}

HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendTemperature(
    uint8_t channel,
    uint8_t temperature)
{
    HOOCH_PROTOCOL_AirConditionerFrame_t frame;

    HOOCH_PROTOCOL_AirConditioner_ClearFrame(&frame);
    frame.channel = channel;
    frame.temperature = temperature;
    frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMPERATURE;
    frame.sequence = (uint8_t)(s_hooch_protocol_air_conditioner_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_AirConditioner_SetFrame(&frame);
}

HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendPowerAddr(
    uint16_t address,
    HOOCH_PROTOCOL_AirConditionerPower_t power)
{
    HOOCH_PROTOCOL_AirConditionerFrame_t frame;

    HOOCH_PROTOCOL_AirConditioner_ClearFrame(&frame);
    frame.address = address;
    frame.power = power;
    frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_POWER;
    frame.sequence = (uint8_t)(s_hooch_protocol_air_conditioner_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_AirConditioner_SetFrame(&frame);
}

HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendModeAddr(
    uint16_t address,
    HOOCH_PROTOCOL_AirConditionerMode_t mode)
{
    HOOCH_PROTOCOL_AirConditionerFrame_t frame;

    HOOCH_PROTOCOL_AirConditioner_ClearFrame(&frame);
    frame.address = address;
    frame.mode = mode;
    frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MODE;
    frame.sequence = (uint8_t)(s_hooch_protocol_air_conditioner_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_AirConditioner_SetFrame(&frame);
}

HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendFanSpeedAddr(
    uint16_t address,
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed)
{
    HOOCH_PROTOCOL_AirConditionerFrame_t frame;

    HOOCH_PROTOCOL_AirConditioner_ClearFrame(&frame);
    frame.address = address;
    frame.fan_speed = fan_speed;
    frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_FAN_SPEED;
    frame.sequence = (uint8_t)(s_hooch_protocol_air_conditioner_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_AirConditioner_SetFrame(&frame);
}

HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendTemperatureAddr(
    uint16_t address,
    uint8_t temperature)
{
    HOOCH_PROTOCOL_AirConditionerFrame_t frame;

    HOOCH_PROTOCOL_AirConditioner_ClearFrame(&frame);
    frame.address = address;
    frame.temperature = temperature;
    frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMPERATURE;
    frame.sequence = (uint8_t)(s_hooch_protocol_air_conditioner_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_AirConditioner_SetFrame(&frame);
}

HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendFrame(
    uint8_t channel,
    HOOCH_PROTOCOL_AirConditionerPower_t power,
    HOOCH_PROTOCOL_AirConditionerMode_t mode,
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed,
    uint8_t temperature)
{
    HOOCH_PROTOCOL_AirConditionerFrame_t frame;

    HOOCH_PROTOCOL_AirConditioner_ClearFrame(&frame);
    frame.channel = channel;
    frame.power = power;
    frame.mode = mode;
    frame.fan_speed = fan_speed;
    frame.temperature = temperature;
    frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_XIAOMI;
    frame.sequence = (uint8_t)(s_hooch_protocol_air_conditioner_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_AirConditioner_SetFrame(&frame);
}

HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendFrameAddr(
    uint16_t address,
    HOOCH_PROTOCOL_AirConditionerPower_t power,
    HOOCH_PROTOCOL_AirConditionerMode_t mode,
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed,
    uint8_t temperature)
{
    HOOCH_PROTOCOL_AirConditionerFrame_t frame;

    HOOCH_PROTOCOL_AirConditioner_ClearFrame(&frame);
    frame.address = address;
    frame.power = power;
    frame.mode = mode;
    frame.fan_speed = fan_speed;
    frame.temperature = temperature;
    frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_XIAOMI;
    frame.sequence = (uint8_t)(s_hooch_protocol_air_conditioner_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_AirConditioner_SetFrame(&frame);
}
