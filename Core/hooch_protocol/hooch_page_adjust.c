#include "hooch_page_adjust.h"

/* 保存当前一次页面调节数据。 */
static HOOCH_PROTOCOL_PageAdjustFrame_t s_hooch_protocol_page_adjust_frame;

/* 保存外部注册的页面调节通知回调。 */
static HOOCH_PROTOCOL_PageAdjustCallback_t s_hooch_protocol_page_adjust_callback;

/* 有回调注册时，把最新数据通知出去。 */
static void HOOCH_PROTOCOL_PageAdjust_NotifyCallback(void)
{
    if (s_hooch_protocol_page_adjust_callback != 0)
    {
        s_hooch_protocol_page_adjust_callback(&s_hooch_protocol_page_adjust_frame);
    }
}

void HOOCH_PageAdjust_Init(void)
{
    /* 初始化时先清掉回调，避免使用野指针。 */
    s_hooch_protocol_page_adjust_callback = 0;
    HOOCH_PROTOCOL_PageAdjust_Clear();
}

void HOOCH_PROTOCOL_PageAdjust_Clear(void)
{
    s_hooch_protocol_page_adjust_frame.value = 0U;
    s_hooch_protocol_page_adjust_frame.sequence = 0U;
    s_hooch_protocol_page_adjust_frame.valid = 0U;
}

HOOCH_PROTOCOL_PageAdjustResult_t HOOCH_PROTOCOL_PageAdjust_SetValue(uint8_t value)
{
    s_hooch_protocol_page_adjust_frame.value = value;
    s_hooch_protocol_page_adjust_frame.sequence++;
    s_hooch_protocol_page_adjust_frame.valid = 1U;

    /* 当前帧更新成功后，通知外部业务层。 */
    HOOCH_PROTOCOL_PageAdjust_NotifyCallback();

    return HOOCH_PROTOCOL_PAGE_ADJUST_RESULT_OK;
}

HOOCH_PROTOCOL_PageAdjustResult_t HOOCH_PROTOCOL_PageAdjust_SetFrame(
    const HOOCH_PROTOCOL_PageAdjustFrame_t *frame)
{
    if (frame == 0)
    {
        /* 空指针不允许写入。 */
        return HOOCH_PROTOCOL_PAGE_ADJUST_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_page_adjust_frame = *frame;
    s_hooch_protocol_page_adjust_frame.valid = 1U;

    /* 直接写帧成功后，也同步回调一次。 */
    HOOCH_PROTOCOL_PageAdjust_NotifyCallback();

    return HOOCH_PROTOCOL_PAGE_ADJUST_RESULT_OK;
}

const HOOCH_PROTOCOL_PageAdjustFrame_t *HOOCH_PROTOCOL_PageAdjust_GetFrame(void)
{
    return &s_hooch_protocol_page_adjust_frame;
}

void HOOCH_PROTOCOL_PageAdjust_RegisterCallback(
    HOOCH_PROTOCOL_PageAdjustCallback_t callback)
{
    /* 允许外部替换为新的回调函数。 */
    s_hooch_protocol_page_adjust_callback = callback;
}

void HOOCH_PROTOCOL_PageAdjust_UnregisterCallback(void)
{
    /* 注销后不再向外通知。 */
    s_hooch_protocol_page_adjust_callback = 0;
}

HOOCH_PROTOCOL_PageAdjustResult_t HOOCH_PROTOCOL_PageAdjust_Send(uint8_t value)
{
    /* 这里先保留为统一入口，后续可接真实协议流程。 */
    return HOOCH_PROTOCOL_PageAdjust_SetValue(value);
}
