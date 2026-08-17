#include "hooch_key_status_dispatch.h"

/* 保存当前一次按键状态下发的数据。 */
static HOOCH_PROTOCOL_KeyStatusDispatchFrame_t s_hooch_protocol_key_status_dispatch_frame;

/* 保存外部注册的按键状态下发通知回调。 */
static HOOCH_PROTOCOL_KeyStatusDispatchCallback_t s_hooch_protocol_key_status_dispatch_callback;

/* 校验状态是否为按下或松开。 */
static uint8_t HOOCH_PROTOCOL_KeyStatusDispatch_IsValidState(
    HOOCH_PROTOCOL_KeyStatusDispatchState_t state)
{
    return (uint8_t)((state == HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_STATE_RELEASED) ||
                     (state == HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_STATE_PRESSED));
}

/* 有回调注册时，把最新下发数据通知出去。 */
static void HOOCH_PROTOCOL_KeyStatusDispatch_NotifyCallback(void)
{
    if (s_hooch_protocol_key_status_dispatch_callback != 0)
    {
        s_hooch_protocol_key_status_dispatch_callback(&s_hooch_protocol_key_status_dispatch_frame);
    }
}

void HOOCH_KeyStatusDispatch_Init(void)
{
    /* 初始化时先清掉回调，避免使用野指针。 */
    s_hooch_protocol_key_status_dispatch_callback = 0;
    HOOCH_PROTOCOL_KeyStatusDispatch_Clear();
}

void HOOCH_PROTOCOL_KeyStatusDispatch_Clear(void)
{
    s_hooch_protocol_key_status_dispatch_frame.key = HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_INVALID;
    s_hooch_protocol_key_status_dispatch_frame.state = HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_STATE_RELEASED;
    s_hooch_protocol_key_status_dispatch_frame.sequence = 0U;
    s_hooch_protocol_key_status_dispatch_frame.valid = 0U;
}

uint8_t HOOCH_PROTOCOL_KeyStatusDispatch_IsValidKey(HOOCH_PROTOCOL_KeyStatusDispatchKey_t key)
{
    return (uint8_t)((key >= HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_1) &&
                     (key <= HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_8));
}

HOOCH_PROTOCOL_KeyStatusDispatchResult_t HOOCH_PROTOCOL_KeyStatusDispatch_Set(
    HOOCH_PROTOCOL_KeyStatusDispatchKey_t key,
    HOOCH_PROTOCOL_KeyStatusDispatchState_t state)
{
    if ((HOOCH_PROTOCOL_KeyStatusDispatch_IsValidKey(key) == 0U) ||
        (HOOCH_PROTOCOL_KeyStatusDispatch_IsValidState(state) == 0U))
    {
        /* 只要按键或状态任一非法，就直接返回。 */
        return HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_key_status_dispatch_frame.key = key;
    s_hooch_protocol_key_status_dispatch_frame.state = state;
    s_hooch_protocol_key_status_dispatch_frame.sequence++;
    s_hooch_protocol_key_status_dispatch_frame.valid = 1U;

    /* 当前帧更新成功后，通知外部业务层。 */
    HOOCH_PROTOCOL_KeyStatusDispatch_NotifyCallback();

    return HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_RESULT_OK;
}
HOOCH_PROTOCOL_KeyStatusDispatchResult_t HOOCH_PROTOCOL_KeyStatusDispatch_SetFrame(
    const HOOCH_PROTOCOL_KeyStatusDispatchFrame_t *frame)
{
    if (frame == 0)
    {
        /* 空指针不允许写入。 */
        return HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_RESULT_INVALID_PARAM;
    }

    if ((HOOCH_PROTOCOL_KeyStatusDispatch_IsValidKey(frame->key) == 0U) ||
        (HOOCH_PROTOCOL_KeyStatusDispatch_IsValidState(frame->state) == 0U))
    {
        /* 完整帧中的关键字段必须全部合法。 */
        return HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_key_status_dispatch_frame = *frame;
    s_hooch_protocol_key_status_dispatch_frame.valid = 1U;

    /* 直接写帧成功后，也同步回调一次。 */
    HOOCH_PROTOCOL_KeyStatusDispatch_NotifyCallback();

    return HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_RESULT_OK;
}

const HOOCH_PROTOCOL_KeyStatusDispatchFrame_t *HOOCH_PROTOCOL_KeyStatusDispatch_GetFrame(void)
{
    return &s_hooch_protocol_key_status_dispatch_frame;
}

void HOOCH_PROTOCOL_KeyStatusDispatch_RegisterCallback(
    HOOCH_PROTOCOL_KeyStatusDispatchCallback_t callback)
{
    /* 允许外部替换为新的回调函数。 */
    s_hooch_protocol_key_status_dispatch_callback = callback;
}

void HOOCH_PROTOCOL_KeyStatusDispatch_UnregisterCallback(void)
{
    /* 注销后不再向外通知。 */
    s_hooch_protocol_key_status_dispatch_callback = 0;
}

HOOCH_PROTOCOL_KeyStatusDispatchResult_t HOOCH_PROTOCOL_KeyStatusDispatch_Send(
    HOOCH_PROTOCOL_KeyStatusDispatchKey_t key,
    HOOCH_PROTOCOL_KeyStatusDispatchState_t state)
{
    /* 这里先保留为统一下发入口，后续可接真实协议发送流程。 */
    return HOOCH_PROTOCOL_KeyStatusDispatch_Set(key, state);
}
