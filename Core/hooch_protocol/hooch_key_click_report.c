#include "hooch_key_click_report.h"

static HOOCH_PROTOCOL_KeyClickReportFrame_t s_hooch_protocol_key_click_report_frame;
static HOOCH_PROTOCOL_KeyClickReportCallback_t s_hooch_protocol_key_click_report_callback;

static uint8_t HOOCH_PROTOCOL_KeyClickReport_IsValidEvent(
    HOOCH_PROTOCOL_KeyClickReportEvent_t event)
{
    return (uint8_t)((event >= HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_SINGLE_CLICK) &&
                     (event <= HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_LONG_CLICK_1P5S));
}

static uint8_t HOOCH_PROTOCOL_KeyClickReport_IsValidState(
    HOOCH_PROTOCOL_KeyClickReportState_t state)
{
    return (uint8_t)((state == HOOCH_PROTOCOL_KEY_CLICK_REPORT_STATE_OFF) ||
                     (state == HOOCH_PROTOCOL_KEY_CLICK_REPORT_STATE_ON));
}

static void HOOCH_PROTOCOL_KeyClickReport_NotifyCallback(void)
{
    if (s_hooch_protocol_key_click_report_callback != 0)
    {
        s_hooch_protocol_key_click_report_callback(&s_hooch_protocol_key_click_report_frame);
    }
}

void HOOCH_KeyClickReport_Init(void)
{
    s_hooch_protocol_key_click_report_callback = 0;
    HOOCH_PROTOCOL_KeyClickReport_Clear();
}

void HOOCH_PROTOCOL_KeyClickReport_Clear(void)
{
    s_hooch_protocol_key_click_report_frame.key = HOOCH_PROTOCOL_KEY_CLICK_REPORT_KEY_INVALID;
    s_hooch_protocol_key_click_report_frame.event = HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_NONE;
    s_hooch_protocol_key_click_report_frame.state = HOOCH_PROTOCOL_KEY_CLICK_REPORT_STATE_OFF;
    s_hooch_protocol_key_click_report_frame.sequence = 0U;
    s_hooch_protocol_key_click_report_frame.valid = 0U;
}

uint8_t HOOCH_PROTOCOL_KeyClickReport_IsValidKey(HOOCH_PROTOCOL_KeyClickReportKey_t key)
{
    return (uint8_t)((key >= HOOCH_PROTOCOL_KEY_CLICK_REPORT_KEY_1) &&
                     (key <= HOOCH_PROTOCOL_KEY_CLICK_REPORT_KEY_8));
}

HOOCH_PROTOCOL_KeyClickReportResult_t HOOCH_PROTOCOL_KeyClickReport_Set(
    HOOCH_PROTOCOL_KeyClickReportKey_t key,
    HOOCH_PROTOCOL_KeyClickReportEvent_t event,
    HOOCH_PROTOCOL_KeyClickReportState_t state)
{
    if ((HOOCH_PROTOCOL_KeyClickReport_IsValidKey(key) == 0U) ||
        (HOOCH_PROTOCOL_KeyClickReport_IsValidEvent(event) == 0U) ||
        (HOOCH_PROTOCOL_KeyClickReport_IsValidState(state) == 0U))
    {
        return HOOCH_PROTOCOL_KEY_CLICK_REPORT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_key_click_report_frame.key = key;
    s_hooch_protocol_key_click_report_frame.event = event;
    s_hooch_protocol_key_click_report_frame.state = state;
    s_hooch_protocol_key_click_report_frame.sequence++;
    s_hooch_protocol_key_click_report_frame.valid = 1U;
    HOOCH_PROTOCOL_KeyClickReport_NotifyCallback();
    return HOOCH_PROTOCOL_KEY_CLICK_REPORT_RESULT_OK;
}

HOOCH_PROTOCOL_KeyClickReportResult_t HOOCH_PROTOCOL_KeyClickReport_SetFrame(
    const HOOCH_PROTOCOL_KeyClickReportFrame_t *frame)
{
    if ((frame == 0) ||
        (HOOCH_PROTOCOL_KeyClickReport_IsValidKey(frame->key) == 0U) ||
        (HOOCH_PROTOCOL_KeyClickReport_IsValidEvent(frame->event) == 0U) ||
        (HOOCH_PROTOCOL_KeyClickReport_IsValidState(frame->state) == 0U))
    {
        return HOOCH_PROTOCOL_KEY_CLICK_REPORT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_key_click_report_frame = *frame;
    s_hooch_protocol_key_click_report_frame.valid = 1U;
    HOOCH_PROTOCOL_KeyClickReport_NotifyCallback();
    return HOOCH_PROTOCOL_KEY_CLICK_REPORT_RESULT_OK;
}

const HOOCH_PROTOCOL_KeyClickReportFrame_t *HOOCH_PROTOCOL_KeyClickReport_GetFrame(void)
{
    return &s_hooch_protocol_key_click_report_frame;
}

void HOOCH_PROTOCOL_KeyClickReport_RegisterCallback(
    HOOCH_PROTOCOL_KeyClickReportCallback_t callback)
{
    s_hooch_protocol_key_click_report_callback = callback;
}

void HOOCH_PROTOCOL_KeyClickReport_UnregisterCallback(void)
{
    s_hooch_protocol_key_click_report_callback = 0;
}

HOOCH_PROTOCOL_KeyClickReportResult_t HOOCH_PROTOCOL_KeyClickReport_Send(
    HOOCH_PROTOCOL_KeyClickReportKey_t key,
    HOOCH_PROTOCOL_KeyClickReportEvent_t event,
    HOOCH_PROTOCOL_KeyClickReportState_t state)
{
    return HOOCH_PROTOCOL_KeyClickReport_Set(key, event, state);
}
