#include "hooch_setting_report.h"

static HOOCH_PROTOCOL_SettingReportEvent_t s_hooch_protocol_setting_report_event;
static const void *s_hooch_protocol_setting_report_data;
static uint16_t s_hooch_protocol_setting_report_data_len;
static HOOCH_PROTOCOL_SettingReportCallback_t s_hooch_protocol_setting_report_callback;

static uint8_t HOOCH_PROTOCOL_SettingReport_IsValidEvent(
    HOOCH_PROTOCOL_SettingReportEvent_t event)
{
    return (uint8_t)((event > HOOCH_PROTOCOL_SETTING_REPORT_EVENT_INVALID) &&
                     (event <= HOOCH_PROTOCOL_SETTING_REPORT_EVENT_COUNT));
}

static void HOOCH_PROTOCOL_SettingReport_NotifyCallback(void)
{
    if (s_hooch_protocol_setting_report_callback != 0)
    {
        s_hooch_protocol_setting_report_callback(
            s_hooch_protocol_setting_report_event,
            s_hooch_protocol_setting_report_data,
            s_hooch_protocol_setting_report_data_len);
    }
}

void HOOCH_SettingReport_Init(void)
{
    s_hooch_protocol_setting_report_callback = 0;
    HOOCH_PROTOCOL_SettingReport_Clear();
}

void HOOCH_PROTOCOL_SettingReport_Clear(void)
{
    s_hooch_protocol_setting_report_event = HOOCH_PROTOCOL_SETTING_REPORT_EVENT_INVALID;
    s_hooch_protocol_setting_report_data = 0;
    s_hooch_protocol_setting_report_data_len = 0U;
}

HOOCH_PROTOCOL_SettingResult_t HOOCH_PROTOCOL_SettingReport_SetEvent(
    HOOCH_PROTOCOL_SettingReportEvent_t event,
    const void *data,
    uint16_t data_len)
{
    if (HOOCH_PROTOCOL_SettingReport_IsValidEvent(event) == 0U)
    {
        return HOOCH_PROTOCOL_SETTING_RESULT_INVALID_PARAM;
    }

    if ((data == 0) && (data_len != 0U))
    {
        return HOOCH_PROTOCOL_SETTING_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_setting_report_event = event;
    s_hooch_protocol_setting_report_data = data;
    s_hooch_protocol_setting_report_data_len = data_len;
    HOOCH_PROTOCOL_SettingReport_NotifyCallback();
    return HOOCH_PROTOCOL_SETTING_RESULT_OK;
}

HOOCH_PROTOCOL_SettingReportEvent_t HOOCH_PROTOCOL_SettingReport_GetEvent(void)
{
    return s_hooch_protocol_setting_report_event;
}

const void *HOOCH_PROTOCOL_SettingReport_GetData(void)
{
    return s_hooch_protocol_setting_report_data;
}

uint16_t HOOCH_PROTOCOL_SettingReport_GetDataLen(void)
{
    return s_hooch_protocol_setting_report_data_len;
}

void HOOCH_PROTOCOL_SettingReport_RegisterCallback(
    HOOCH_PROTOCOL_SettingReportCallback_t callback)
{
    s_hooch_protocol_setting_report_callback = callback;
}

void HOOCH_PROTOCOL_SettingReport_UnregisterCallback(void)
{
    s_hooch_protocol_setting_report_callback = 0;
}

HOOCH_PROTOCOL_SettingResult_t HOOCH_PROTOCOL_SettingReport_Send(
    HOOCH_PROTOCOL_SettingReportEvent_t event,
    const void *data,
    uint16_t data_len)
{
    return HOOCH_PROTOCOL_SettingReport_SetEvent(event, data, data_len);
}
