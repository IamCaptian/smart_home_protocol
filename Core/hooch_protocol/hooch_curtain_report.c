#include "hooch_curtain.h"

/* 保存当前一次窗帘上报数据。 */
static HOOCH_PROTOCOL_CurtainFrame_t s_hooch_protocol_curtain_report_frame;

/* 保存外部注册的窗帘上报回调（类型1：仅通道+数值）。 */
static HOOCH_PROTOCOL_CurtainReportCallback1_t s_hooch_protocol_curtain_report_callback1;

/* 保存外部注册的窗帘上报回调（类型2：页面+通道+数值）。 */
static HOOCH_PROTOCOL_CurtainReportCallback2_t s_hooch_protocol_curtain_report_callback2;

/* 保存外部注册的窗帘上报回调（类型3：地址+数值）。 */
static HOOCH_PROTOCOL_CurtainReportCallback3_t s_hooch_protocol_curtain_report_callback3;

/* 校验窗帘 key 是否在当前支持范围内。 */
static uint8_t HOOCH_PROTOCOL_Curtain_IsValidKey(
    HOOCH_PROTOCOL_CurtainKey_t key)
{
    return (uint8_t)((key >= HOOCH_PROTOCOL_CURTAIN_KEY_1) &&
                     (key <= HOOCH_PROTOCOL_CURTAIN_KEY_8));
}

/* 校验控制项是否为当前模块支持的单项更新类型。 */
static uint8_t HOOCH_PROTOCOL_Curtain_IsValidControlItem(
    HOOCH_PROTOCOL_CurtainControlItem_t control_item)
{
    return (uint8_t)((control_item >= HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ALL) &&
                     (control_item <= HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ANGLE));
}

/* 统一清空单个窗帘字段，避免重复初始化代码。 */
static void HOOCH_PROTOCOL_Curtain_ResetItem(
    HOOCH_PROTOCOL_CurtainItem_t *item)
{
    if (item == 0)
    {
        return;
    }

    item->value = 0U;
    item->sequence = 0U;
    item->valid = 0U;
}

/* 统一清空一帧窗帘数据。 */
static void HOOCH_PROTOCOL_Curtain_ClearFrame(
    HOOCH_PROTOCOL_CurtainFrame_t *frame)
{
    if (frame == 0)
    {
        return;
    }

    frame->key = HOOCH_PROTOCOL_CURTAIN_KEY_INVALID;
    frame->page = 0U;
    frame->address = 0U;
    HOOCH_PROTOCOL_Curtain_ResetItem(&frame->switch_status);
    HOOCH_PROTOCOL_Curtain_ResetItem(&frame->stop);
    HOOCH_PROTOCOL_Curtain_ResetItem(&frame->percent);
    HOOCH_PROTOCOL_Curtain_ResetItem(&frame->angle);
    frame->control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_INVALID;
    frame->sequence = 0U;
    frame->valid = 0U;
}

/* 有上报回调注册时，把最新上报数据通知出去（类型1：仅通道+数值）。 */
static void HOOCH_PROTOCOL_Curtain_NotifyReportCallback1(void)
{
    if (s_hooch_protocol_curtain_report_callback1 != 0)
    {
        s_hooch_protocol_curtain_report_callback1(&s_hooch_protocol_curtain_report_frame);
    }
}

/* 有上报回调注册时，把最新上报数据通知出去（类型2：页面+通道+数值）。 */
static void HOOCH_PROTOCOL_Curtain_NotifyReportCallback2(void)
{
    if (s_hooch_protocol_curtain_report_callback2 != 0)
    {
        s_hooch_protocol_curtain_report_callback2(&s_hooch_protocol_curtain_report_frame);
    }
}

/* 有上报回调注册时，把最新上报数据通知出去（类型3：地址+数值）。 */
static void HOOCH_PROTOCOL_Curtain_NotifyReportCallback3(void)
{
    if (s_hooch_protocol_curtain_report_callback3 != 0)
    {
        s_hooch_protocol_curtain_report_callback3(&s_hooch_protocol_curtain_report_frame);
    }
}

/* 统一写入单个窗帘字段，并维护更新序号。 */
static HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SetItem(
    HOOCH_PROTOCOL_CurtainFrame_t *frame,
    HOOCH_PROTOCOL_CurtainItem_t *item,
    HOOCH_PROTOCOL_CurtainKey_t key,
    HOOCH_PROTOCOL_CurtainControlItem_t control_item,
    uint8_t value)
{
    if ((frame == 0) || (item == 0) ||
        (HOOCH_PROTOCOL_Curtain_IsValidKey(key) == 0U))
    {
        return HOOCH_PROTOCOL_CURTAIN_RESULT_INVALID_PARAM;
    }

    frame->key = key;
    item->value = value;
    item->sequence++;
    item->valid = 1U;
    frame->control_item = control_item;
    frame->valid = 1U;

    HOOCH_PROTOCOL_Curtain_NotifyReportCallback1();

    return HOOCH_PROTOCOL_CURTAIN_RESULT_OK;
}

/* 统一写入完整窗帘上报帧，并按控制项只更新指定字段。 */
static HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SetReportFrame(
    const HOOCH_PROTOCOL_CurtainFrame_t *frame)
{
    if (frame == 0)
    {
        return HOOCH_PROTOCOL_CURTAIN_RESULT_INVALID_PARAM;
    }

    if (HOOCH_PROTOCOL_Curtain_IsValidControlItem(frame->control_item) == 0U)
    {
        return HOOCH_PROTOCOL_CURTAIN_RESULT_INVALID_PARAM;
    }

    if (HOOCH_PROTOCOL_Curtain_IsValidKey(frame->key) == 0U)
    {
        return HOOCH_PROTOCOL_CURTAIN_RESULT_INVALID_PARAM;
    }

    switch (frame->control_item)
    {
        case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ALL:
            s_hooch_protocol_curtain_report_frame.switch_status.value = frame->switch_status.value;
            s_hooch_protocol_curtain_report_frame.switch_status.sequence++;
            s_hooch_protocol_curtain_report_frame.switch_status.valid = 1U;

            s_hooch_protocol_curtain_report_frame.stop.value = frame->stop.value;
            s_hooch_protocol_curtain_report_frame.stop.sequence++;
            s_hooch_protocol_curtain_report_frame.stop.valid = 1U;

            s_hooch_protocol_curtain_report_frame.percent.value = frame->percent.value;
            s_hooch_protocol_curtain_report_frame.percent.sequence++;
            s_hooch_protocol_curtain_report_frame.percent.valid = 1U;

            s_hooch_protocol_curtain_report_frame.angle.value = frame->angle.value;
            s_hooch_protocol_curtain_report_frame.angle.sequence++;
            s_hooch_protocol_curtain_report_frame.angle.valid = 1U;
            break;

        case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_SWITCH:
            s_hooch_protocol_curtain_report_frame.switch_status.value = frame->switch_status.value;
            s_hooch_protocol_curtain_report_frame.switch_status.sequence++;
            s_hooch_protocol_curtain_report_frame.switch_status.valid = 1U;
            break;

        case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_STOP:
            s_hooch_protocol_curtain_report_frame.stop.value = frame->stop.value;
            s_hooch_protocol_curtain_report_frame.stop.sequence++;
            s_hooch_protocol_curtain_report_frame.stop.valid = 1U;
            break;

        case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_PERCENT:
            s_hooch_protocol_curtain_report_frame.percent.value = frame->percent.value;
            s_hooch_protocol_curtain_report_frame.percent.sequence++;
            s_hooch_protocol_curtain_report_frame.percent.valid = 1U;
            break;

        case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ANGLE:
            s_hooch_protocol_curtain_report_frame.angle.value = frame->angle.value;
            s_hooch_protocol_curtain_report_frame.angle.sequence++;
            s_hooch_protocol_curtain_report_frame.angle.valid = 1U;
            break;

        default:
            return HOOCH_PROTOCOL_CURTAIN_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_curtain_report_frame.key = frame->key;
    s_hooch_protocol_curtain_report_frame.control_item = frame->control_item;
    s_hooch_protocol_curtain_report_frame.valid = 1U;

    HOOCH_PROTOCOL_Curtain_NotifyReportCallback1();

    return HOOCH_PROTOCOL_CURTAIN_RESULT_OK;
}

void HOOCH_Curtain_Init(void)
{
    HOOCH_PROTOCOL_Curtain_UnregisterCallback();
    HOOCH_PROTOCOL_Curtain_UnregisterReportCallback();
    HOOCH_PROTOCOL_Curtain_Clear();
}

void HOOCH_PROTOCOL_Curtain_Clear(void)
{
    HOOCH_PROTOCOL_Curtain_ClearFrame(&s_hooch_protocol_curtain_report_frame);
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SetValue(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value)
{
    return HOOCH_PROTOCOL_Curtain_SetItem(
        &s_hooch_protocol_curtain_report_frame,
        &s_hooch_protocol_curtain_report_frame.switch_status,
        key,
        HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_SWITCH,
        value);
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SetStop(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value)
{
    return HOOCH_PROTOCOL_Curtain_SetItem(
        &s_hooch_protocol_curtain_report_frame,
        &s_hooch_protocol_curtain_report_frame.stop,
        key,
        HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_STOP,
        value);
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SetPercent(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value)
{
    return HOOCH_PROTOCOL_Curtain_SetItem(
        &s_hooch_protocol_curtain_report_frame,
        &s_hooch_protocol_curtain_report_frame.percent,
        key,
        HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_PERCENT,
        value);
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SetAngle(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value)
{
    return HOOCH_PROTOCOL_Curtain_SetItem(
        &s_hooch_protocol_curtain_report_frame,
        &s_hooch_protocol_curtain_report_frame.angle,
        key,
        HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ANGLE,
        value);
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SetFrame(
    const HOOCH_PROTOCOL_CurtainFrame_t *frame)
{
    return HOOCH_PROTOCOL_Curtain_SetReportFrame(frame);
}

const HOOCH_PROTOCOL_CurtainFrame_t *HOOCH_PROTOCOL_Curtain_GetFrame(void)
{
    return &s_hooch_protocol_curtain_report_frame;
}

void HOOCH_PROTOCOL_Curtain_RegisterReportCallback(
    HOOCH_PROTOCOL_CurtainReportCallback1_t callback)
{
    s_hooch_protocol_curtain_report_callback1 = callback;
}

void HOOCH_PROTOCOL_Curtain_UnregisterReportCallback(void)
{
    s_hooch_protocol_curtain_report_callback1 = 0;
}

void HOOCH_PROTOCOL_Curtain_RegisterReportCallback2(
    HOOCH_PROTOCOL_CurtainReportCallback2_t callback)
{
    s_hooch_protocol_curtain_report_callback2 = callback;
}

void HOOCH_PROTOCOL_Curtain_UnregisterReportCallback2(void)
{
    s_hooch_protocol_curtain_report_callback2 = 0;
}

void HOOCH_PROTOCOL_Curtain_RegisterReportCallback3(
    HOOCH_PROTOCOL_CurtainReportCallback3_t callback)
{
    s_hooch_protocol_curtain_report_callback3 = callback;
}

void HOOCH_PROTOCOL_Curtain_UnregisterReportCallback3(void)
{
    s_hooch_protocol_curtain_report_callback3 = 0;
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_Send(uint8_t value)
{
    (void)value;
    return HOOCH_PROTOCOL_CURTAIN_RESULT_INVALID_PARAM;
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendSwitch(
    HOOCH_PROTOCOL_CurtainKey_t key,
    HOOCH_PROTOCOL_CurtainSwitchState_t state)
{
    return HOOCH_PROTOCOL_Curtain_SetValue(key, (uint8_t)state);
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendStop(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value)
{
    return HOOCH_PROTOCOL_Curtain_SetStop(key, value);
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendPercent(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value)
{
    return HOOCH_PROTOCOL_Curtain_SetPercent(key, value);
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendAngle(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value)
{
    return HOOCH_PROTOCOL_Curtain_SetAngle(key, value);
}

/* ======================== 类型2 上报：页面+通道+数值 ======================== */

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendSwitchPage(
    uint8_t page,
    HOOCH_PROTOCOL_CurtainKey_t key,
    HOOCH_PROTOCOL_CurtainSwitchState_t state)
{
    if (HOOCH_PROTOCOL_Curtain_IsValidKey(key) == 0U)
    {
        return HOOCH_PROTOCOL_CURTAIN_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_curtain_report_frame.page = page;
    s_hooch_protocol_curtain_report_frame.key = key;
    s_hooch_protocol_curtain_report_frame.switch_status.value = (uint8_t)state;
    s_hooch_protocol_curtain_report_frame.switch_status.sequence++;
    s_hooch_protocol_curtain_report_frame.switch_status.valid = 1U;
    s_hooch_protocol_curtain_report_frame.control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_SWITCH;
    s_hooch_protocol_curtain_report_frame.sequence++;
    s_hooch_protocol_curtain_report_frame.valid = 1U;

    HOOCH_PROTOCOL_Curtain_NotifyReportCallback2();

    return HOOCH_PROTOCOL_CURTAIN_RESULT_OK;
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendStopPage(
    uint8_t page,
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value)
{
    if (HOOCH_PROTOCOL_Curtain_IsValidKey(key) == 0U)
    {
        return HOOCH_PROTOCOL_CURTAIN_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_curtain_report_frame.page = page;
    s_hooch_protocol_curtain_report_frame.key = key;
    s_hooch_protocol_curtain_report_frame.stop.value = value;
    s_hooch_protocol_curtain_report_frame.stop.sequence++;
    s_hooch_protocol_curtain_report_frame.stop.valid = 1U;
    s_hooch_protocol_curtain_report_frame.control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_STOP;
    s_hooch_protocol_curtain_report_frame.sequence++;
    s_hooch_protocol_curtain_report_frame.valid = 1U;

    HOOCH_PROTOCOL_Curtain_NotifyReportCallback2();

    return HOOCH_PROTOCOL_CURTAIN_RESULT_OK;
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendPercentPage(
    uint8_t page,
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value)
{
    if (HOOCH_PROTOCOL_Curtain_IsValidKey(key) == 0U)
    {
        return HOOCH_PROTOCOL_CURTAIN_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_curtain_report_frame.page = page;
    s_hooch_protocol_curtain_report_frame.key = key;
    s_hooch_protocol_curtain_report_frame.percent.value = value;
    s_hooch_protocol_curtain_report_frame.percent.sequence++;
    s_hooch_protocol_curtain_report_frame.percent.valid = 1U;
    s_hooch_protocol_curtain_report_frame.control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_PERCENT;
    s_hooch_protocol_curtain_report_frame.sequence++;
    s_hooch_protocol_curtain_report_frame.valid = 1U;

    HOOCH_PROTOCOL_Curtain_NotifyReportCallback2();

    return HOOCH_PROTOCOL_CURTAIN_RESULT_OK;
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendAnglePage(
    uint8_t page,
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value)
{
    if (HOOCH_PROTOCOL_Curtain_IsValidKey(key) == 0U)
    {
        return HOOCH_PROTOCOL_CURTAIN_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_curtain_report_frame.page = page;
    s_hooch_protocol_curtain_report_frame.key = key;
    s_hooch_protocol_curtain_report_frame.angle.value = value;
    s_hooch_protocol_curtain_report_frame.angle.sequence++;
    s_hooch_protocol_curtain_report_frame.angle.valid = 1U;
    s_hooch_protocol_curtain_report_frame.control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ANGLE;
    s_hooch_protocol_curtain_report_frame.sequence++;
    s_hooch_protocol_curtain_report_frame.valid = 1U;

    HOOCH_PROTOCOL_Curtain_NotifyReportCallback2();

    return HOOCH_PROTOCOL_CURTAIN_RESULT_OK;
}

/* ======================== 类型3 上报：地址+数值 ======================== */

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendSwitchAddr(
    uint16_t address,
    HOOCH_PROTOCOL_CurtainSwitchState_t state)
{
    s_hooch_protocol_curtain_report_frame.address = address;
    s_hooch_protocol_curtain_report_frame.switch_status.value = (uint8_t)state;
    s_hooch_protocol_curtain_report_frame.switch_status.sequence++;
    s_hooch_protocol_curtain_report_frame.switch_status.valid = 1U;
    s_hooch_protocol_curtain_report_frame.control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_SWITCH;
    s_hooch_protocol_curtain_report_frame.sequence++;
    s_hooch_protocol_curtain_report_frame.valid = 1U;

    HOOCH_PROTOCOL_Curtain_NotifyReportCallback3();

    return HOOCH_PROTOCOL_CURTAIN_RESULT_OK;
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendStopAddr(
    uint16_t address,
    uint8_t value)
{
    s_hooch_protocol_curtain_report_frame.address = address;
    HOOCH_PROTOCOL_Curtain_SetItem(&s_hooch_protocol_curtain_report_frame,
        &s_hooch_protocol_curtain_report_frame.stop,
        HOOCH_PROTOCOL_CURTAIN_KEY_1, HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_STOP, value);
    s_hooch_protocol_curtain_report_frame.sequence++;

    HOOCH_PROTOCOL_Curtain_NotifyReportCallback3();

    return HOOCH_PROTOCOL_CURTAIN_RESULT_OK;
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendPercentAddr(
    uint16_t address,
    uint8_t value)
{
    s_hooch_protocol_curtain_report_frame.address = address;
    HOOCH_PROTOCOL_Curtain_SetItem(&s_hooch_protocol_curtain_report_frame,
        &s_hooch_protocol_curtain_report_frame.percent,
        HOOCH_PROTOCOL_CURTAIN_KEY_1, HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_PERCENT, value);
    s_hooch_protocol_curtain_report_frame.sequence++;

    HOOCH_PROTOCOL_Curtain_NotifyReportCallback3();

    return HOOCH_PROTOCOL_CURTAIN_RESULT_OK;
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendAngleAddr(
    uint16_t address,
    uint8_t value)
{
    s_hooch_protocol_curtain_report_frame.address = address;
    HOOCH_PROTOCOL_Curtain_SetItem(&s_hooch_protocol_curtain_report_frame,
        &s_hooch_protocol_curtain_report_frame.angle,
        HOOCH_PROTOCOL_CURTAIN_KEY_1, HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ANGLE, value);
    s_hooch_protocol_curtain_report_frame.sequence++;

    HOOCH_PROTOCOL_Curtain_NotifyReportCallback3();

    return HOOCH_PROTOCOL_CURTAIN_RESULT_OK;
}
