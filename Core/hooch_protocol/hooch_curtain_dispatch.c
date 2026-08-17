#include "hooch_curtain.h"

/* 保存当前一次窗帘下发数据。 */
static HOOCH_PROTOCOL_CurtainFrame_t s_hooch_protocol_curtain_dispatch_frame;

/* 保存外部注册的窗帘下发回调。 */
static HOOCH_PROTOCOL_CurtainCallback_t s_hooch_protocol_curtain_dispatch_callback;

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
                     (control_item < HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_MAX));
}



/* 有下发回调注册时，把最新下发数据通知出去。 */
static void HOOCH_PROTOCOL_Curtain_NotifyDispatchCallback(void)
{
    if (s_hooch_protocol_curtain_dispatch_callback != 0)
    {
        s_hooch_protocol_curtain_dispatch_callback(&s_hooch_protocol_curtain_dispatch_frame);
    }
}

/* 统一写入单个窗帘字段，并维护更新序号。 */
static HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SetDispatchItem(
    HOOCH_PROTOCOL_CurtainKey_t key,
    HOOCH_PROTOCOL_CurtainItem_t *item,
    HOOCH_PROTOCOL_CurtainControlItem_t control_item,
    uint8_t value)
{
    if ((item == 0) || (HOOCH_PROTOCOL_Curtain_IsValidKey(key) == 0U))
    {
        return HOOCH_PROTOCOL_CURTAIN_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_curtain_dispatch_frame.key = key;
    item->value = value;
    item->sequence++;
    item->valid = 1U;
    s_hooch_protocol_curtain_dispatch_frame.control_item = control_item;
    s_hooch_protocol_curtain_dispatch_frame.valid = 1U;

    HOOCH_PROTOCOL_Curtain_NotifyDispatchCallback();

    return HOOCH_PROTOCOL_CURTAIN_RESULT_OK;
}

/* 统一写入完整窗帘下发帧，并按控制项只更新指定字段。 */
static HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SetDispatchFrameInternal(
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
            s_hooch_protocol_curtain_dispatch_frame.switch_status.value = frame->switch_status.value;
            s_hooch_protocol_curtain_dispatch_frame.switch_status.sequence++;
            s_hooch_protocol_curtain_dispatch_frame.switch_status.valid = 1U;

            s_hooch_protocol_curtain_dispatch_frame.stop.value = frame->stop.value;
            s_hooch_protocol_curtain_dispatch_frame.stop.sequence++;
            s_hooch_protocol_curtain_dispatch_frame.stop.valid = 1U;

            s_hooch_protocol_curtain_dispatch_frame.percent.value = frame->percent.value;
            s_hooch_protocol_curtain_dispatch_frame.percent.sequence++;
            s_hooch_protocol_curtain_dispatch_frame.percent.valid = 1U;

            s_hooch_protocol_curtain_dispatch_frame.angle.value = frame->angle.value;
            s_hooch_protocol_curtain_dispatch_frame.angle.sequence++;
            s_hooch_protocol_curtain_dispatch_frame.angle.valid = 1U;
            break;

        case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_SWITCH:
            s_hooch_protocol_curtain_dispatch_frame.switch_status.value = frame->switch_status.value;
            s_hooch_protocol_curtain_dispatch_frame.switch_status.sequence++;
            s_hooch_protocol_curtain_dispatch_frame.switch_status.valid = 1U;
            break;

        case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_STOP:
            s_hooch_protocol_curtain_dispatch_frame.stop.value = frame->stop.value;
            s_hooch_protocol_curtain_dispatch_frame.stop.sequence++;
            s_hooch_protocol_curtain_dispatch_frame.stop.valid = 1U;
            break;

        case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_PERCENT:
            s_hooch_protocol_curtain_dispatch_frame.percent.value = frame->percent.value;
            s_hooch_protocol_curtain_dispatch_frame.percent.sequence++;
            s_hooch_protocol_curtain_dispatch_frame.percent.valid = 1U;
            break;

        case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ANGLE:
            s_hooch_protocol_curtain_dispatch_frame.angle.value = frame->angle.value;
            s_hooch_protocol_curtain_dispatch_frame.angle.sequence++;
            s_hooch_protocol_curtain_dispatch_frame.angle.valid = 1U;
            break;

        case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_DEVICE_DESCRIPTOR:
            (void)memcpy(s_hooch_protocol_curtain_dispatch_frame.device_desc,
                         frame->device_desc,
                         sizeof(s_hooch_protocol_curtain_dispatch_frame.device_desc));
            break;

        case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_DEFAULT_ICON:
            s_hooch_protocol_curtain_dispatch_frame.value = frame->value;
            break;

        case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_SELECTED_ICON:
            s_hooch_protocol_curtain_dispatch_frame.value = frame->value;
            break;

        case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_CURTAIN_TYPE:
            s_hooch_protocol_curtain_dispatch_frame.value = frame->value;
            break;

        default:
            return HOOCH_PROTOCOL_CURTAIN_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_curtain_dispatch_frame.key = frame->key;
    s_hooch_protocol_curtain_dispatch_frame.page = frame->page;
    s_hooch_protocol_curtain_dispatch_frame.address = frame->address;
    s_hooch_protocol_curtain_dispatch_frame.control_item = frame->control_item;
    s_hooch_protocol_curtain_dispatch_frame.sequence++;
    s_hooch_protocol_curtain_dispatch_frame.valid = 1U;

    HOOCH_PROTOCOL_Curtain_NotifyDispatchCallback();

    return HOOCH_PROTOCOL_CURTAIN_RESULT_OK;
}

void HOOCH_PROTOCOL_Curtain_RegisterCallback(
    HOOCH_PROTOCOL_CurtainCallback_t callback)
{
    s_hooch_protocol_curtain_dispatch_callback = callback;
}

void HOOCH_PROTOCOL_Curtain_UnregisterCallback(void)
{
    s_hooch_protocol_curtain_dispatch_callback = 0;
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_DispatchFrame(
    const HOOCH_PROTOCOL_CurtainFrame_t *frame)
{
    return HOOCH_PROTOCOL_Curtain_SetDispatchFrameInternal(frame);
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_DispatchSwitch(
    HOOCH_PROTOCOL_CurtainKey_t key,
    HOOCH_PROTOCOL_CurtainSwitchState_t state)
{
    return HOOCH_PROTOCOL_Curtain_SetDispatchItem(
        key,
        &s_hooch_protocol_curtain_dispatch_frame.switch_status,
        HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_SWITCH,
        (uint8_t)state);
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_DispatchStop(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value)
{
    return HOOCH_PROTOCOL_Curtain_SetDispatchItem(
        key,
        &s_hooch_protocol_curtain_dispatch_frame.stop,
        HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_STOP,
        value);
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_DispatchPercent(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value)
{
    return HOOCH_PROTOCOL_Curtain_SetDispatchItem(
        key,
        &s_hooch_protocol_curtain_dispatch_frame.percent,
        HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_PERCENT,
        value);
}

HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_DispatchAngle(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value)
{
    return HOOCH_PROTOCOL_Curtain_SetDispatchItem(
        key,
        &s_hooch_protocol_curtain_dispatch_frame.angle,
        HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ANGLE,
        value);
}
