#include "hooch_unsupported_attribute.h"

#include <stdarg.h>

/* 当前不支持属性列表，上电默认全部支持（空列表），业务层可用 SendAttributes 覆盖。 */
static HOOCH_PROTOCOL_UnsupportedAttribute_t
    s_hooch_protocol_unsupported_attribute_list[HOOCH_PROTOCOL_UNSUPPORTED_REPORT_LIST_MAX] = {
        HOOCH_PROTOCOL_UNSUPPORTED_ATTRIBUTE_NONE,
    };
static uint8_t s_hooch_protocol_unsupported_attribute_count = 0U;

/* 保存外部注册的不支持属性上报回调（只保留一个，后注册覆盖先注册）。 */
static HOOCH_PROTOCOL_UnsupportedAttributeReportCallback_t
    s_hooch_protocol_unsupported_attribute_report_callback;

/* 有回调注册时，把当前属性列表通知出去。 */
static void HOOCH_PROTOCOL_UnsupportedAttribute_NotifyReportCallback(void)
{
    if (s_hooch_protocol_unsupported_attribute_report_callback != 0)
    {
        s_hooch_protocol_unsupported_attribute_report_callback(
            s_hooch_protocol_unsupported_attribute_list,
            s_hooch_protocol_unsupported_attribute_count);
    }
}

HOOCH_PROTOCOL_SettingResult_t HOOCH_PROTOCOL_UnsupportedAttribute_SendAttributes(
    HOOCH_PROTOCOL_UnsupportedAttribute_t first,
    ...)
{
    va_list args;
    uint8_t count = 0U;
    uint8_t attribute = (uint8_t)first;

    /* 以 NONE(0) 作为列表结束标志，逐个取参存入列表。 */
    va_start(args, first);
    while (attribute != (uint8_t)HOOCH_PROTOCOL_UNSUPPORTED_ATTRIBUTE_NONE)
    {
        if (count < (uint8_t)HOOCH_PROTOCOL_UNSUPPORTED_REPORT_LIST_MAX)
        {
            s_hooch_protocol_unsupported_attribute_list[count] =
                (HOOCH_PROTOCOL_UnsupportedAttribute_t)attribute;
            count++;
        }
        /* enum 形参在可变参数中会提升为 int，必须按 int 读取后转回 uint8_t。 */
        attribute = (uint8_t)va_arg(args, int);
    }
    va_end(args);

    s_hooch_protocol_unsupported_attribute_count = count;

    HOOCH_PROTOCOL_UnsupportedAttribute_NotifyReportCallback();

    return HOOCH_PROTOCOL_SETTING_RESULT_OK;
}

void HOOCH_PROTOCOL_UnsupportedAttribute_RegisterReportCallback(
    HOOCH_PROTOCOL_UnsupportedAttributeReportCallback_t callback)
{
    /* 只保留一个回调，后注册覆盖先注册。 */
    s_hooch_protocol_unsupported_attribute_report_callback = callback;
}

void HOOCH_PROTOCOL_UnsupportedAttribute_UnregisterReportCallback(void)
{
    s_hooch_protocol_unsupported_attribute_report_callback = 0;
}

HOOCH_PROTOCOL_SettingResult_t HOOCH_PROTOCOL_UnsupportedAttribute_Report(void)
{
    HOOCH_PROTOCOL_UnsupportedAttribute_NotifyReportCallback();

    return HOOCH_PROTOCOL_SETTING_RESULT_OK;
}
