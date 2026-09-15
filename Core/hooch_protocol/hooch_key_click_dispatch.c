#include "hooch_key_click_dispatch.h"

/* 保存当前一次按键点击下发的数据。 */
static HOOCH_PROTOCOL_KeyClickDispatchFrame_t s_hooch_protocol_key_click_dispatch_frame;

/* 保存外部注册的按键点击下发通知回调。 */
static HOOCH_PROTOCOL_KeyClickDispatchCallback_t s_hooch_protocol_key_click_dispatch_callback;

/* 校验事件类型是否在当前模块支持的范围内。 */
static uint8_t HOOCH_PROTOCOL_KeyClickDispatch_IsValidEvent(
    HOOCH_PROTOCOL_KeyClickDispatchEvent_t event)
{
    return (uint8_t)((event >= HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_EVENT_SINGLE_CLICK) &&
                     (event <= HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_EVENT_LONG_CLICK_1P5S));
}

/* 校验状态是否为开或关。 */
static uint8_t HOOCH_PROTOCOL_KeyClickDispatch_IsValidState(
    HOOCH_PROTOCOL_KeyClickDispatchState_t state)
{
    return (uint8_t)((state == HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_STATE_OFF) ||
                     (state == HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_STATE_ON));
}

/* 有回调注册时，把最新下发数据通知出去。 */
static void HOOCH_PROTOCOL_KeyClickDispatch_NotifyCallback(void)
{
    if (s_hooch_protocol_key_click_dispatch_callback != 0)
    {
        s_hooch_protocol_key_click_dispatch_callback(&s_hooch_protocol_key_click_dispatch_frame);
    }
}

void HOOCH_KeyClickDispatch_Init(void)
{
    /* 初始化时先清掉回调，避免使用野指针。 */
    s_hooch_protocol_key_click_dispatch_callback = 0;
    HOOCH_PROTOCOL_KeyClickDispatch_Clear();
}

void HOOCH_PROTOCOL_KeyClickDispatch_Clear(void)
{
    s_hooch_protocol_key_click_dispatch_frame.key = HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_INVALID;
    s_hooch_protocol_key_click_dispatch_frame.event = HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_EVENT_NONE;
    s_hooch_protocol_key_click_dispatch_frame.state = HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_STATE_OFF;
    s_hooch_protocol_key_click_dispatch_frame.sequence = 0U;
    s_hooch_protocol_key_click_dispatch_frame.valid = 0U;
}

uint8_t HOOCH_PROTOCOL_KeyClickDispatch_IsValidKey(HOOCH_PROTOCOL_KeyClickDispatchKey_t key)
{
    return (uint8_t)((key >= HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_1) &&
                     (key <= HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_16));
}

HOOCH_PROTOCOL_KeyClickDispatchResult_t HOOCH_PROTOCOL_KeyClickDispatch_Set(
    HOOCH_PROTOCOL_KeyClickDispatchKey_t key,
    HOOCH_PROTOCOL_KeyClickDispatchEvent_t event,
    HOOCH_PROTOCOL_KeyClickDispatchState_t state)
{
    if ((HOOCH_PROTOCOL_KeyClickDispatch_IsValidKey(key) == 0U) ||
        (HOOCH_PROTOCOL_KeyClickDispatch_IsValidEvent(event) == 0U) ||
        (HOOCH_PROTOCOL_KeyClickDispatch_IsValidState(state) == 0U))
    {
        /* 只要按键、事件、状态任一非法，就直接返回。 */
        return HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_key_click_dispatch_frame.key = key;
    s_hooch_protocol_key_click_dispatch_frame.event = event;
    s_hooch_protocol_key_click_dispatch_frame.state = state;
    s_hooch_protocol_key_click_dispatch_frame.sequence++;
    s_hooch_protocol_key_click_dispatch_frame.valid = 1U;

    /* 当前帧更新成功后，通知外部业务层。 */
    HOOCH_PROTOCOL_KeyClickDispatch_NotifyCallback();

    return HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_RESULT_OK;
}

HOOCH_PROTOCOL_KeyClickDispatchResult_t HOOCH_PROTOCOL_KeyClickDispatch_SetFrame(
    const HOOCH_PROTOCOL_KeyClickDispatchFrame_t *frame)
{
    if (frame == 0)
    {
        /* 空指针不允许写入。 */
        return HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_RESULT_INVALID_PARAM;
    }

    if ((HOOCH_PROTOCOL_KeyClickDispatch_IsValidKey(frame->key) == 0U) ||
        (HOOCH_PROTOCOL_KeyClickDispatch_IsValidEvent(frame->event) == 0U) ||
        (HOOCH_PROTOCOL_KeyClickDispatch_IsValidState(frame->state) == 0U))
    {
        /* 完整帧中的关键字段必须全部合法。 */
        return HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_key_click_dispatch_frame = *frame;
    s_hooch_protocol_key_click_dispatch_frame.valid = 1U;

    /* 直接写帧成功后，也同步回调一次。 */
    HOOCH_PROTOCOL_KeyClickDispatch_NotifyCallback();

    return HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_RESULT_OK;
}

const HOOCH_PROTOCOL_KeyClickDispatchFrame_t *HOOCH_PROTOCOL_KeyClickDispatch_GetFrame(void)
{
    return &s_hooch_protocol_key_click_dispatch_frame;
}

void HOOCH_PROTOCOL_KeyClickDispatch_RegisterCallback(
    HOOCH_PROTOCOL_KeyClickDispatchCallback_t callback)
{
    /* 允许外部替换为新的回调函数。 */
    s_hooch_protocol_key_click_dispatch_callback = callback;
}

void HOOCH_PROTOCOL_KeyClickDispatch_UnregisterCallback(void)
{
    /* 注销后不再向外通知。 */
    s_hooch_protocol_key_click_dispatch_callback = 0;
}

HOOCH_PROTOCOL_KeyClickDispatchResult_t HOOCH_PROTOCOL_KeyClickDispatch_Send(
    HOOCH_PROTOCOL_KeyClickDispatchKey_t key,
    HOOCH_PROTOCOL_KeyClickDispatchEvent_t event,
    HOOCH_PROTOCOL_KeyClickDispatchState_t state)
{
    /* 这里先保留为统一下发入口，后续可接真实协议发送流程。 */
    return HOOCH_PROTOCOL_KeyClickDispatch_Set(key, event, state);
}
