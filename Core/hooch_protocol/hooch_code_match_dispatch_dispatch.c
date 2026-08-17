#include "hooch_code_match_dispatch_dispatch.h"

/* 保存当前一次对码下发数据。 */
static HOOCH_PROTOCOL_CodeMatchReportFrame_t s_hooch_protocol_code_match_dispatch_frame;

/* 保存外部注册的对码下发回调。 */
static HOOCH_PROTOCOL_CodeMatchDispatchCallback_t s_hooch_protocol_code_match_dispatch_callback;

/* 有下发回调注册时，把最新数据通知出去。 */
static void HOOCH_PROTOCOL_CodeMatchDispatch_NotifyCallback(void)
{
    if (s_hooch_protocol_code_match_dispatch_callback != 0)
    {
        s_hooch_protocol_code_match_dispatch_callback(&s_hooch_protocol_code_match_dispatch_frame);
    }
}

void HOOCH_CodeMatchDispatch_Init(void)
{
    s_hooch_protocol_code_match_dispatch_callback = 0;
    HOOCH_PROTOCOL_CodeMatchDispatch_Clear();
}

void HOOCH_PROTOCOL_CodeMatchDispatch_Clear(void)
{
    s_hooch_protocol_code_match_dispatch_frame.page = 0U;
    s_hooch_protocol_code_match_dispatch_frame.channel = 0U;
    s_hooch_protocol_code_match_dispatch_frame.action = HOOCH_PROTOCOL_CODE_MATCH_REPORT_ACTION_CLEAR;
    s_hooch_protocol_code_match_dispatch_frame.sequence = 0U;
    s_hooch_protocol_code_match_dispatch_frame.valid = 0U;
}

HOOCH_PROTOCOL_CodeMatchReportResult_t HOOCH_PROTOCOL_CodeMatchDispatch_Set(
    uint8_t page,
    uint8_t channel,
    HOOCH_PROTOCOL_CodeMatchReportAction_t action)
{
    s_hooch_protocol_code_match_dispatch_frame.page = page;
    s_hooch_protocol_code_match_dispatch_frame.channel = channel;
    s_hooch_protocol_code_match_dispatch_frame.action = action;
    s_hooch_protocol_code_match_dispatch_frame.sequence++;
    s_hooch_protocol_code_match_dispatch_frame.valid = 1U;
    HOOCH_PROTOCOL_CodeMatchDispatch_NotifyCallback();
    return HOOCH_PROTOCOL_CODE_MATCH_REPORT_RESULT_OK;
}

HOOCH_PROTOCOL_CodeMatchReportResult_t HOOCH_PROTOCOL_CodeMatchDispatch_SetFrame(
    const HOOCH_PROTOCOL_CodeMatchReportFrame_t *frame)
{
    if (frame == 0)
    {
        return HOOCH_PROTOCOL_CODE_MATCH_REPORT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_code_match_dispatch_frame = *frame;
    s_hooch_protocol_code_match_dispatch_frame.valid = 1U;
    HOOCH_PROTOCOL_CodeMatchDispatch_NotifyCallback();
    return HOOCH_PROTOCOL_CODE_MATCH_REPORT_RESULT_OK;
}

const HOOCH_PROTOCOL_CodeMatchReportFrame_t *HOOCH_PROTOCOL_CodeMatchDispatch_GetFrame(void)
{
    return &s_hooch_protocol_code_match_dispatch_frame;
}

void HOOCH_PROTOCOL_CodeMatchDispatch_RegisterCallback(
    HOOCH_PROTOCOL_CodeMatchDispatchCallback_t callback)
{
    s_hooch_protocol_code_match_dispatch_callback = callback;
}

void HOOCH_PROTOCOL_CodeMatchDispatch_UnregisterCallback(void)
{
    s_hooch_protocol_code_match_dispatch_callback = 0;
}

HOOCH_PROTOCOL_CodeMatchReportResult_t HOOCH_PROTOCOL_CodeMatchDispatch_Dispatch(
    uint8_t page,
    uint8_t channel,
    HOOCH_PROTOCOL_CodeMatchReportAction_t action)
{
    return HOOCH_PROTOCOL_CodeMatchDispatch_Set(page, channel, action);
}

HOOCH_PROTOCOL_CodeMatchReportResult_t HOOCH_PROTOCOL_CodeMatchDispatch_DispatchFrame(
    const HOOCH_PROTOCOL_CodeMatchReportFrame_t *frame)
{
    return HOOCH_PROTOCOL_CodeMatchDispatch_SetFrame(frame);
}
