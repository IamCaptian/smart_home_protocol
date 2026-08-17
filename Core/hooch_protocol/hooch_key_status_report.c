#include "hooch_key_status_report.h"

/* 保存当前一次按键状态上报帧。 */
static HOOCH_PROTOCOL_KeyStatusReportFrame_t s_hooch_protocol_key_status_report_frame;

/* 保存外部注册的按键状态上报回调。 */
static HOOCH_PROTOCOL_KeyStatusReportCallback_t s_hooch_protocol_key_status_report_callback;

/* 校验按键状态字段是否合法。 */
static uint8_t HOOCH_PROTOCOL_KeyStatusReport_IsValidState(
    HOOCH_PROTOCOL_KeyStatusReportState_t state)
{
    return (uint8_t)((state == HOOCH_PROTOCOL_KEY_STATUS_REPORT_STATE_OFF) ||
                     (state == HOOCH_PROTOCOL_KEY_STATUS_REPORT_STATE_ON));
}

/* 有回调注册时，把最新帧通知出去。 */
static void HOOCH_PROTOCOL_KeyStatusReport_NotifyCallback(void)
{
    if (s_hooch_protocol_key_status_report_callback != 0)
    {
        s_hooch_protocol_key_status_report_callback(&s_hooch_protocol_key_status_report_frame);
    }
}

/* 初始化按键状态上报模块：清除回调并重置当前帧。 */
void HOOCH_KeyStatusReport_Init(void)
{
    s_hooch_protocol_key_status_report_callback = 0;
    HOOCH_PROTOCOL_KeyStatusReport_Clear();
}

/* 清空当前保存的按键状态上报帧（恢复默认值）。 */
void HOOCH_PROTOCOL_KeyStatusReport_Clear(void)
{
    s_hooch_protocol_key_status_report_frame.key = HOOCH_PROTOCOL_KEY_STATUS_REPORT_KEY_INVALID;
    s_hooch_protocol_key_status_report_frame.state = HOOCH_PROTOCOL_KEY_STATUS_REPORT_STATE_OFF;
    s_hooch_protocol_key_status_report_frame.sequence = 0U;
    s_hooch_protocol_key_status_report_frame.valid = 0U;
}

/* 校验按键编号是否合法（目前支持 1~8）。 */
uint8_t HOOCH_PROTOCOL_KeyStatusReport_IsValidKey(HOOCH_PROTOCOL_KeyStatusReportKey_t key)
{
    return (uint8_t)((key >= HOOCH_PROTOCOL_KEY_STATUS_REPORT_KEY_1) &&
                     (key <= HOOCH_PROTOCOL_KEY_STATUS_REPORT_KEY_8));
}

/* 设置一条按键状态并触发回调：对 key/state 做合法性校验，成功后更新序号与有效位。 */
HOOCH_PROTOCOL_KeyStatusReportResult_t HOOCH_PROTOCOL_KeyStatusReport_Set(
    HOOCH_PROTOCOL_KeyStatusReportKey_t key,
    HOOCH_PROTOCOL_KeyStatusReportState_t state)
{
    if ((HOOCH_PROTOCOL_KeyStatusReport_IsValidKey(key) == 0U) ||
        (HOOCH_PROTOCOL_KeyStatusReport_IsValidState(state) == 0U))
    {
        return HOOCH_PROTOCOL_KEY_STATUS_REPORT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_key_status_report_frame.key = key;
    s_hooch_protocol_key_status_report_frame.state = state;
    s_hooch_protocol_key_status_report_frame.sequence++;
    s_hooch_protocol_key_status_report_frame.valid = 1U;
    HOOCH_PROTOCOL_KeyStatusReport_NotifyCallback();
    return HOOCH_PROTOCOL_KEY_STATUS_REPORT_RESULT_OK;
}

/* 直接写入完整帧并触发回调：对 frame/key/state 做合法性校验，成功后置 valid=1。 */
HOOCH_PROTOCOL_KeyStatusReportResult_t HOOCH_PROTOCOL_KeyStatusReport_SetFrame(
    const HOOCH_PROTOCOL_KeyStatusReportFrame_t *frame)
{
    if ((frame == 0) ||
        (HOOCH_PROTOCOL_KeyStatusReport_IsValidKey(frame->key) == 0U) ||
        (HOOCH_PROTOCOL_KeyStatusReport_IsValidState(frame->state) == 0U))
    {
        return HOOCH_PROTOCOL_KEY_STATUS_REPORT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_key_status_report_frame = *frame;
    s_hooch_protocol_key_status_report_frame.valid = 1U;
    HOOCH_PROTOCOL_KeyStatusReport_NotifyCallback();
    return HOOCH_PROTOCOL_KEY_STATUS_REPORT_RESULT_OK;
}

/* 获取当前保存的按键状态上报帧（只读）。 */
const HOOCH_PROTOCOL_KeyStatusReportFrame_t *HOOCH_PROTOCOL_KeyStatusReport_GetFrame(void)
{
    return &s_hooch_protocol_key_status_report_frame;
}

/* 注册按键状态上报回调函数（允许覆盖已有回调）。 */
void HOOCH_PROTOCOL_KeyStatusReport_RegisterCallback(
    HOOCH_PROTOCOL_KeyStatusReportCallback_t callback)
{
    s_hooch_protocol_key_status_report_callback = callback;
}

/* 注销按键状态上报回调函数。 */
void HOOCH_PROTOCOL_KeyStatusReport_UnregisterCallback(void)
{
    s_hooch_protocol_key_status_report_callback = 0;
}

/* 统一的发送入口：当前版本等价于 Set(key, state)。 */
HOOCH_PROTOCOL_KeyStatusReportResult_t HOOCH_PROTOCOL_KeyStatusReport_Send(
    HOOCH_PROTOCOL_KeyStatusReportKey_t key,
    HOOCH_PROTOCOL_KeyStatusReportState_t state)
{
    return HOOCH_PROTOCOL_KeyStatusReport_Set(key, state);
}
