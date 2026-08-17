#include "hooch_code_match_dispatch_report.h"

/* 保存当前一次对码上报数据。 */
static HOOCH_PROTOCOL_CodeMatchReportFrame_t s_hooch_protocol_code_match_report_frame;

/* 保存外部注册的对码上报回调。 */
static HOOCH_PROTOCOL_CodeMatchReportCallback_t s_hooch_protocol_code_match_report_callback;

/* 保存开关页面专用的对码上报回调。 */
static HOOCH_PROTOCOL_CodeMatchReportSwitchCallback_t s_hooch_protocol_code_match_report_switch_callback;

/* 有上报回调注册时，把最新数据通知出去。 */
static void HOOCH_PROTOCOL_CodeMatchReport_NotifyCallback(void)
{
    if (s_hooch_protocol_code_match_report_callback != 0)
    {
        s_hooch_protocol_code_match_report_callback(&s_hooch_protocol_code_match_report_frame);
    }
}

void HOOCH_CodeMatchReport_Init(void)
{
    s_hooch_protocol_code_match_report_callback = 0;
    s_hooch_protocol_code_match_report_switch_callback = 0;
    HOOCH_PROTOCOL_CodeMatchReport_Clear();
}

void HOOCH_PROTOCOL_CodeMatchReport_Clear(void)
{
    s_hooch_protocol_code_match_report_frame.page = 0U;
    s_hooch_protocol_code_match_report_frame.channel = 0U;
    s_hooch_protocol_code_match_report_frame.action = HOOCH_PROTOCOL_CODE_MATCH_REPORT_ACTION_CLEAR;
    s_hooch_protocol_code_match_report_frame.sequence = 0U;
    s_hooch_protocol_code_match_report_frame.valid = 0U;
}

HOOCH_PROTOCOL_CodeMatchReportResult_t HOOCH_PROTOCOL_CodeMatchReport_Set(
    uint8_t page,
    uint8_t channel,
    HOOCH_PROTOCOL_CodeMatchReportAction_t action)
{
    s_hooch_protocol_code_match_report_frame.page = page;
    s_hooch_protocol_code_match_report_frame.channel = channel;
    s_hooch_protocol_code_match_report_frame.action = action;
    s_hooch_protocol_code_match_report_frame.sequence++;
    s_hooch_protocol_code_match_report_frame.valid = 1U;
    HOOCH_PROTOCOL_CodeMatchReport_NotifyCallback();
    return HOOCH_PROTOCOL_CODE_MATCH_REPORT_RESULT_OK;
}

HOOCH_PROTOCOL_CodeMatchReportResult_t HOOCH_PROTOCOL_CodeMatchReport_SetFrame(
    const HOOCH_PROTOCOL_CodeMatchReportFrame_t *frame)
{
    if (frame == 0)
    {
        return HOOCH_PROTOCOL_CODE_MATCH_REPORT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_code_match_report_frame = *frame;
    s_hooch_protocol_code_match_report_frame.valid = 1U;
    HOOCH_PROTOCOL_CodeMatchReport_NotifyCallback();
    return HOOCH_PROTOCOL_CODE_MATCH_REPORT_RESULT_OK;
}

const HOOCH_PROTOCOL_CodeMatchReportFrame_t *HOOCH_PROTOCOL_CodeMatchReport_GetFrame(void)
{
    return &s_hooch_protocol_code_match_report_frame;
}

void HOOCH_PROTOCOL_CodeMatchReport_RegisterCallback(
    HOOCH_PROTOCOL_CodeMatchReportCallback_t callback)
{
    s_hooch_protocol_code_match_report_callback = callback;
}

void HOOCH_PROTOCOL_CodeMatchReport_UnregisterCallback(void)
{
    s_hooch_protocol_code_match_report_callback = 0;
}

HOOCH_PROTOCOL_CodeMatchReportResult_t HOOCH_PROTOCOL_CodeMatchReport_Send(
    uint8_t page,
    uint8_t channel,
    HOOCH_PROTOCOL_CodeMatchReportAction_t action)
{
    return HOOCH_PROTOCOL_CodeMatchReport_Set(page, channel, action);
}

/*
 * ======================== 开关页面对码上报 ========================
 */

void HOOCH_PROTOCOL_CodeMatchReport_RegisterSwitchCallback(
    HOOCH_PROTOCOL_CodeMatchReportSwitchCallback_t callback)
{
    s_hooch_protocol_code_match_report_switch_callback = callback;
}

void HOOCH_PROTOCOL_CodeMatchReport_UnregisterSwitchCallback(void)
{
    s_hooch_protocol_code_match_report_switch_callback = 0;
}

HOOCH_PROTOCOL_CodeMatchReportResult_t HOOCH_PROTOCOL_CodeMatchReport_SendSwitch(
    uint8_t page,
    uint8_t key,
    uint8_t mode,
    HOOCH_PROTOCOL_CodeMatchReportAction_t action)
{

    if ((key < 1U) || (key > 4U) || (mode > 1U))
    {
        return HOOCH_PROTOCOL_CODE_MATCH_REPORT_RESULT_INVALID_PARAM;
    }

    /* 通知开关页面专用回调 */
    if (s_hooch_protocol_code_match_report_switch_callback != 0)
    {
        s_hooch_protocol_code_match_report_switch_callback(page, key, mode, action);
    }

    return HOOCH_PROTOCOL_CODE_MATCH_REPORT_RESULT_OK;
}
