#include "hooch_fresh_air.h"

/* 保存当前一次新风上报数据。 */
static HOOCH_PROTOCOL_FreshAirFrame_t s_hooch_protocol_fresh_air_report_frame;

/* 保存外部注册的新风上报回调（通道版）。 */
static HOOCH_PROTOCOL_FreshAirCallback_t s_hooch_protocol_fresh_air_report_callback;

/* 保存外部注册的新风上报回调（地址版）。 */
static HOOCH_PROTOCOL_FreshAirCallback_t s_hooch_protocol_fresh_air_report_callback2;

/* 有上报回调注册时，把最新数据通知出去。 */
static void HOOCH_PROTOCOL_FreshAir_NotifyReportCallback(void)
{
    if (s_hooch_protocol_fresh_air_report_callback != 0)
    {
        s_hooch_protocol_fresh_air_report_callback(
            &s_hooch_protocol_fresh_air_report_frame);
    }
}

/* 有地址版上报回调注册时，把最新数据通知出去。 */
static void HOOCH_PROTOCOL_FreshAir_NotifyReportCallback2(void)
{
    if (s_hooch_protocol_fresh_air_report_callback2 != 0)
    {
        s_hooch_protocol_fresh_air_report_callback2(
            &s_hooch_protocol_fresh_air_report_frame);
    }
}

/* 统一清空一帧新风数据。 */
static void HOOCH_PROTOCOL_FreshAir_ClearFrame(
    HOOCH_PROTOCOL_FreshAirFrame_t *frame)
{
    if (frame == 0)
    {
        return;
    }

    frame->channel = 0U;
    frame->address = 0U;
    frame->power = HOOCH_PROTOCOL_FRESH_AIR_POWER_OFF;
    frame->mode = HOOCH_PROTOCOL_FRESH_AIR_MODE_INVALID;
    frame->fan_speed = HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID;
    frame->control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_INVALID;
    frame->sequence = 0U;
    frame->valid = 0U;
}

uint8_t HOOCH_PROTOCOL_FreshAir_IsValidPower(HOOCH_PROTOCOL_FreshAirPower_t power)
{
    return (uint8_t)((power == HOOCH_PROTOCOL_FRESH_AIR_POWER_OFF) ||
                     (power == HOOCH_PROTOCOL_FRESH_AIR_POWER_ON));
}

uint8_t HOOCH_PROTOCOL_FreshAir_IsValidMode(HOOCH_PROTOCOL_FreshAirMode_t mode)
{
    return (uint8_t)((mode >= HOOCH_PROTOCOL_FRESH_AIR_MODE_AUTO) &&
                     (mode <= HOOCH_PROTOCOL_FRESH_AIR_MODE_BOOST));
}

uint8_t HOOCH_PROTOCOL_FreshAir_IsValidFanSpeed(HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed)
{
    return (uint8_t)((fan_speed >= HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_LOW) &&
                     (fan_speed <= HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_AUTO));
}

uint8_t HOOCH_PROTOCOL_FreshAir_IsValidControlItem(
    HOOCH_PROTOCOL_FreshAirControlItem_t control_item)
{
    return (uint8_t)((control_item >= HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL) &&
                     (control_item < HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_MAX));
}

/* 统一写入新风上报状态。 */
static HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SetReportState(
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

    s_hooch_protocol_fresh_air_report_frame.power = power;
    s_hooch_protocol_fresh_air_report_frame.mode = mode;
    s_hooch_protocol_fresh_air_report_frame.fan_speed = fan_speed;
    s_hooch_protocol_fresh_air_report_frame.channel = 0U;
    s_hooch_protocol_fresh_air_report_frame.control_item =
        HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL;
    s_hooch_protocol_fresh_air_report_frame.sequence++;
    s_hooch_protocol_fresh_air_report_frame.valid = 1U;

    HOOCH_PROTOCOL_FreshAir_NotifyReportCallback();

    return HOOCH_PROTOCOL_FRESH_AIR_RESULT_OK;
}

/* 统一按 control_item 写入新风上报帧。 */
static HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SetReportFrame(
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
        case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL_KNX:
        case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL_XIAOMI:
            if ((HOOCH_PROTOCOL_FreshAir_IsValidPower(frame->power) == 0U) ||
                (HOOCH_PROTOCOL_FreshAir_IsValidMode(frame->mode) == 0U) ||
                (HOOCH_PROTOCOL_FreshAir_IsValidFanSpeed(frame->fan_speed) == 0U))
            {
                return HOOCH_PROTOCOL_FRESH_AIR_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_fresh_air_report_frame.power = frame->power;
            s_hooch_protocol_fresh_air_report_frame.mode = frame->mode;
            s_hooch_protocol_fresh_air_report_frame.fan_speed = frame->fan_speed;
            break;

        case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_POWER:
            if (HOOCH_PROTOCOL_FreshAir_IsValidPower(frame->power) == 0U)
            {
                return HOOCH_PROTOCOL_FRESH_AIR_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_fresh_air_report_frame.power = frame->power;
            break;

        case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_MODE:
            if (HOOCH_PROTOCOL_FreshAir_IsValidMode(frame->mode) == 0U)
            {
                return HOOCH_PROTOCOL_FRESH_AIR_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_fresh_air_report_frame.mode = frame->mode;
            break;

        case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED:
            if (HOOCH_PROTOCOL_FreshAir_IsValidFanSpeed(frame->fan_speed) == 0U)
            {
                return HOOCH_PROTOCOL_FRESH_AIR_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_fresh_air_report_frame.fan_speed = frame->fan_speed;
            break;

        default:
            return HOOCH_PROTOCOL_FRESH_AIR_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_fresh_air_report_frame.channel = frame->channel;
    s_hooch_protocol_fresh_air_report_frame.address = frame->address;
    s_hooch_protocol_fresh_air_report_frame.control_item = frame->control_item;
    s_hooch_protocol_fresh_air_report_frame.sequence = frame->sequence;
    s_hooch_protocol_fresh_air_report_frame.valid = 1U;

    /* 通知通道版回调 */
    HOOCH_PROTOCOL_FreshAir_NotifyReportCallback();

    /* 地址非零时通知地址版回调 */
    if (frame->address != 0U)
    {
        HOOCH_PROTOCOL_FreshAir_NotifyReportCallback2();
    }

    return HOOCH_PROTOCOL_FRESH_AIR_RESULT_OK;
}

void HOOCH_FreshAir_Init(void)
{
    HOOCH_PROTOCOL_FreshAir_UnregisterCallback();
    HOOCH_PROTOCOL_FreshAir_UnregisterReportCallback();
    HOOCH_PROTOCOL_FreshAir_UnregisterReportCallback2();
    HOOCH_PROTOCOL_FreshAir_Clear();
}

void HOOCH_PROTOCOL_FreshAir_Clear(void)
{
    HOOCH_PROTOCOL_FreshAir_ClearFrame(&s_hooch_protocol_fresh_air_report_frame);
}

HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_Set(
    HOOCH_PROTOCOL_FreshAirPower_t power,
    HOOCH_PROTOCOL_FreshAirMode_t mode,
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed)
{
    return HOOCH_PROTOCOL_FreshAir_SetReportState(power, mode, fan_speed);
}

HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SetFrame(
    const HOOCH_PROTOCOL_FreshAirFrame_t *frame)
{
    return HOOCH_PROTOCOL_FreshAir_SetReportFrame(frame);
}

const HOOCH_PROTOCOL_FreshAirFrame_t *HOOCH_PROTOCOL_FreshAir_GetFrame(void)
{
    return &s_hooch_protocol_fresh_air_report_frame;
}

void HOOCH_PROTOCOL_FreshAir_RegisterReportCallback(
    HOOCH_PROTOCOL_FreshAirCallback_t callback)
{
    s_hooch_protocol_fresh_air_report_callback = callback;
}

void HOOCH_PROTOCOL_FreshAir_UnregisterReportCallback(void)
{
    s_hooch_protocol_fresh_air_report_callback = 0;
}

void HOOCH_PROTOCOL_FreshAir_RegisterReportCallback2(
    HOOCH_PROTOCOL_FreshAirCallback_t callback)
{
    s_hooch_protocol_fresh_air_report_callback2 = callback;
}

void HOOCH_PROTOCOL_FreshAir_UnregisterReportCallback2(void)
{
    s_hooch_protocol_fresh_air_report_callback2 = 0;
}

HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_Send(
    uint8_t channel,
    HOOCH_PROTOCOL_FreshAirPower_t power,
    HOOCH_PROTOCOL_FreshAirMode_t mode,
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed)
{
    HOOCH_PROTOCOL_FreshAirFrame_t frame;

    HOOCH_PROTOCOL_FreshAir_ClearFrame(&frame);
    frame.channel = channel;
    frame.power = power;
    frame.mode = mode;
    frame.fan_speed = fan_speed;
    frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL;
    frame.sequence = (uint8_t)(s_hooch_protocol_fresh_air_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FreshAir_SetFrame(&frame);
}

HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SendPower(
    uint8_t channel,
    HOOCH_PROTOCOL_FreshAirPower_t power)
{
    HOOCH_PROTOCOL_FreshAirFrame_t frame;

    HOOCH_PROTOCOL_FreshAir_ClearFrame(&frame);
    frame.channel = channel;
    frame.power = power;
    frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_POWER;
    frame.sequence = (uint8_t)(s_hooch_protocol_fresh_air_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FreshAir_SetFrame(&frame);
}

HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SendMode(
    uint8_t channel,
    HOOCH_PROTOCOL_FreshAirMode_t mode)
{
    HOOCH_PROTOCOL_FreshAirFrame_t frame;

    HOOCH_PROTOCOL_FreshAir_ClearFrame(&frame);
    frame.channel = channel;
    frame.mode = mode;
    frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_MODE;
    frame.sequence = (uint8_t)(s_hooch_protocol_fresh_air_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FreshAir_SetFrame(&frame);
}

HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SendFanSpeed(
    uint8_t channel,
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed)
{
    HOOCH_PROTOCOL_FreshAirFrame_t frame;

    HOOCH_PROTOCOL_FreshAir_ClearFrame(&frame);
    frame.channel = channel;
    frame.fan_speed = fan_speed;
    frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED;
    frame.sequence = (uint8_t)(s_hooch_protocol_fresh_air_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FreshAir_SetFrame(&frame);
}

HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SendPowerAddr(
    uint16_t address,
    HOOCH_PROTOCOL_FreshAirPower_t power)
{
    HOOCH_PROTOCOL_FreshAirFrame_t frame;

    HOOCH_PROTOCOL_FreshAir_ClearFrame(&frame);
    frame.address = address;
    frame.power = power;
    frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_POWER;
    frame.sequence = (uint8_t)(s_hooch_protocol_fresh_air_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FreshAir_SetFrame(&frame);
}

HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SendModeAddr(
    uint16_t address,
    HOOCH_PROTOCOL_FreshAirMode_t mode)
{
    HOOCH_PROTOCOL_FreshAirFrame_t frame;

    HOOCH_PROTOCOL_FreshAir_ClearFrame(&frame);
    frame.address = address;
    frame.mode = mode;
    frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_MODE;
    frame.sequence = (uint8_t)(s_hooch_protocol_fresh_air_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FreshAir_SetFrame(&frame);
}

HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SendFanSpeedAddr(
    uint16_t address,
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed)
{
    HOOCH_PROTOCOL_FreshAirFrame_t frame;

    HOOCH_PROTOCOL_FreshAir_ClearFrame(&frame);
    frame.address = address;
    frame.fan_speed = fan_speed;
    frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED;
    frame.sequence = (uint8_t)(s_hooch_protocol_fresh_air_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FreshAir_SetFrame(&frame);
}

HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SendFrame(
    uint8_t channel,
    HOOCH_PROTOCOL_FreshAirPower_t power,
    HOOCH_PROTOCOL_FreshAirMode_t mode,
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed)
{
    HOOCH_PROTOCOL_FreshAirFrame_t frame;

    HOOCH_PROTOCOL_FreshAir_ClearFrame(&frame);
    frame.channel = channel;
    frame.power = power;
    frame.mode = mode;
    frame.fan_speed = fan_speed;
    frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL_XIAOMI;
    frame.sequence = (uint8_t)(s_hooch_protocol_fresh_air_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FreshAir_SetFrame(&frame);
}

HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SendFrameAddr(
    uint16_t address,
    HOOCH_PROTOCOL_FreshAirPower_t power,
    HOOCH_PROTOCOL_FreshAirMode_t mode,
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed)
{
    HOOCH_PROTOCOL_FreshAirFrame_t frame;

    HOOCH_PROTOCOL_FreshAir_ClearFrame(&frame);
    frame.address = address;
    frame.power = power;
    frame.mode = mode;
    frame.fan_speed = fan_speed;
    frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL_XIAOMI;
    frame.sequence = (uint8_t)(s_hooch_protocol_fresh_air_report_frame.sequence + 1U);

    return HOOCH_PROTOCOL_FreshAir_SetFrame(&frame);
}
