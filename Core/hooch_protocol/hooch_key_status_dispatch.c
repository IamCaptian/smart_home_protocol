#include "hooch_key_status.h"

/* 保存当前一次按键状态下发的数据。 */
static HOOCH_PROTOCOL_KeyStatusFrame_t s_hooch_protocol_key_status_dispatch_frame;

/* 保存外部注册的按键状态下发通知回调。 */
static HOOCH_PROTOCOL_KeyStatusCallback_t s_hooch_protocol_key_status_dispatch_callback;

static uint8_t HOOCH_PROTOCOL_KeyStatus_IsValidKey(HOOCH_PROTOCOL_KeyStatusKey_t key)
{
    return (uint8_t)((key >= HOOCH_PROTOCOL_KEY_STATUS_KEY_1) &&
                     (key <= HOOCH_PROTOCOL_KEY_STATUS_KEY_16));
}

static uint8_t HOOCH_PROTOCOL_KeyStatus_IsValidState(HOOCH_PROTOCOL_KeyStatusState_t state)
{
    return (uint8_t)((state == HOOCH_PROTOCOL_KEY_STATUS_STATE_OFF) ||
                     (state == HOOCH_PROTOCOL_KEY_STATUS_STATE_ON));
}

/* 有回调注册时，把最新下发数据通知出去。 */
static void HOOCH_PROTOCOL_KeyStatus_NotifyDispatchCallback(void)
{
    if (s_hooch_protocol_key_status_dispatch_callback != 0)
    {
        s_hooch_protocol_key_status_dispatch_callback(&s_hooch_protocol_key_status_dispatch_frame);
    }
}

/* 注册按键状态下发回调函数（允许覆盖已有回调）。 */
void HOOCH_PROTOCOL_KeyStatus_RegisterCallback(
    HOOCH_PROTOCOL_KeyStatusCallback_t callback)
{
    s_hooch_protocol_key_status_dispatch_callback = callback;
}

/* 注销按键状态下发回调函数。 */
void HOOCH_PROTOCOL_KeyStatus_UnregisterCallback(void)
{
    s_hooch_protocol_key_status_dispatch_callback = 0;
}

/* 下发按键开关状态并触发回调。 */
HOOCH_PROTOCOL_KeyStatusResult_t HOOCH_PROTOCOL_KeyStatus_DispatchState(
    HOOCH_PROTOCOL_KeyStatusKey_t key,
    HOOCH_PROTOCOL_KeyStatusState_t state)
{
    if ((HOOCH_PROTOCOL_KeyStatus_IsValidKey(key) == 0U) ||
        (HOOCH_PROTOCOL_KeyStatus_IsValidState(state) == 0U))
    {
        return HOOCH_PROTOCOL_KEY_STATUS_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_key_status_dispatch_frame.key = key;
    s_hooch_protocol_key_status_dispatch_frame.state = state;
    s_hooch_protocol_key_status_dispatch_frame.control_item =
        HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_STATE;
    s_hooch_protocol_key_status_dispatch_frame.sequence++;
    s_hooch_protocol_key_status_dispatch_frame.valid = 1U;

    HOOCH_PROTOCOL_KeyStatus_NotifyDispatchCallback();

    return HOOCH_PROTOCOL_KEY_STATUS_RESULT_OK;
}

/* 写入完整的按键状态下发数据帧并触发回调。 */
HOOCH_PROTOCOL_KeyStatusResult_t HOOCH_PROTOCOL_KeyStatus_DispatchFrame(
    const HOOCH_PROTOCOL_KeyStatusFrame_t *frame)
{
    if ((frame == 0) ||
        (HOOCH_PROTOCOL_KeyStatus_IsValidKey(frame->key) == 0U))
    {
        return HOOCH_PROTOCOL_KEY_STATUS_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_key_status_dispatch_frame.key = frame->key;
    s_hooch_protocol_key_status_dispatch_frame.state = frame->state;
    s_hooch_protocol_key_status_dispatch_frame.control_item = frame->control_item;
    s_hooch_protocol_key_status_dispatch_frame.source = frame->source;
    s_hooch_protocol_key_status_dispatch_frame.value = frame->value;
    if (frame->control_item == HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_DEVICE_DESCRIPTOR)
    {
        /* 设备描述：整段拷贝(24 byte, UTF-8) */
        (void)memcpy(s_hooch_protocol_key_status_dispatch_frame.device_desc,
                     frame->device_desc,
                     sizeof(s_hooch_protocol_key_status_dispatch_frame.device_desc));
    }
    s_hooch_protocol_key_status_dispatch_frame.sequence++;
    s_hooch_protocol_key_status_dispatch_frame.valid = 1U;

    HOOCH_PROTOCOL_KeyStatus_NotifyDispatchCallback();

    return HOOCH_PROTOCOL_KEY_STATUS_RESULT_OK;
}
