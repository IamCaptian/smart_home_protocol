#include "hooch_key_status.h"

/* 保存当前一次按键状态上报的数据。 */
static HOOCH_PROTOCOL_KeyStatusFrame_t s_hooch_protocol_key_status_report_frame;

/* 保存外部注册的按键状态上报回调。 */
static HOOCH_PROTOCOL_KeyStatusReportCallback_t s_hooch_protocol_key_status_report_callback;

static uint8_t HOOCH_PROTOCOL_KeyStatus_IsValidKey(HOOCH_PROTOCOL_KeyStatusKey_t key)
{
    return (uint8_t)((key >= HOOCH_PROTOCOL_KEY_STATUS_KEY_1) &&
                     (key <= HOOCH_PROTOCOL_KEY_STATUS_KEY_8));
}

static uint8_t HOOCH_PROTOCOL_KeyStatus_IsValidState(HOOCH_PROTOCOL_KeyStatusState_t state)
{
    return (uint8_t)((state == HOOCH_PROTOCOL_KEY_STATUS_STATE_OFF) ||
                     (state == HOOCH_PROTOCOL_KEY_STATUS_STATE_ON));
}

static uint8_t HOOCH_PROTOCOL_KeyStatus_IsValidControlItem(
    HOOCH_PROTOCOL_KeyStatusControlItem_t control_item)
{
    return (uint8_t)((control_item >= HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_ALL) &&
                     (control_item <= HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_STATE));
}

/* 有回调注册时，把最新上报数据通知出去。 */
static void HOOCH_PROTOCOL_KeyStatus_NotifyReportCallback(void)
{
    if (s_hooch_protocol_key_status_report_callback != 0)
    {
        s_hooch_protocol_key_status_report_callback(&s_hooch_protocol_key_status_report_frame);
    }
}

/* 初始化按键状态模块：清除回调并重置当前帧。 */
void HOOCH_KeyStatus_Init(void)
{
    s_hooch_protocol_key_status_report_callback = 0;
    HOOCH_PROTOCOL_KeyStatus_Clear();
}

/* 清空当前保存的按键状态数据（恢复默认值）。 */
void HOOCH_PROTOCOL_KeyStatus_Clear(void)
{
    s_hooch_protocol_key_status_report_frame.key = HOOCH_PROTOCOL_KEY_STATUS_KEY_INVALID;
    s_hooch_protocol_key_status_report_frame.page = 0U;
    s_hooch_protocol_key_status_report_frame.address = 0U;
    s_hooch_protocol_key_status_report_frame.state = HOOCH_PROTOCOL_KEY_STATUS_STATE_OFF;
    s_hooch_protocol_key_status_report_frame.control_item =
        HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_INVALID;
    s_hooch_protocol_key_status_report_frame.sequence = 0U;
    s_hooch_protocol_key_status_report_frame.valid = 0U;
}

/* 上报按键开关状态并触发回调：对 key/state 做合法性校验，成功后更新序号与有效位。 */
HOOCH_PROTOCOL_KeyStatusResult_t HOOCH_PROTOCOL_KeyStatus_ReportState(
    HOOCH_PROTOCOL_KeyStatusKey_t key,
    HOOCH_PROTOCOL_KeyStatusState_t state)
{
    if ((HOOCH_PROTOCOL_KeyStatus_IsValidKey(key) == 0U) ||
        (HOOCH_PROTOCOL_KeyStatus_IsValidState(state) == 0U))
    {
        return HOOCH_PROTOCOL_KEY_STATUS_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_key_status_report_frame.key = key;
    s_hooch_protocol_key_status_report_frame.state = state;
    s_hooch_protocol_key_status_report_frame.control_item =
        HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_STATE;
    s_hooch_protocol_key_status_report_frame.sequence++;
    s_hooch_protocol_key_status_report_frame.valid = 1U;

    HOOCH_PROTOCOL_KeyStatus_NotifyReportCallback();

    return HOOCH_PROTOCOL_KEY_STATUS_RESULT_OK;
}

/* 直接写入完整帧并触发回调：根据 control_item 仅更新对应字段。 */
HOOCH_PROTOCOL_KeyStatusResult_t HOOCH_PROTOCOL_KeyStatus_SetFrame(
    const HOOCH_PROTOCOL_KeyStatusFrame_t *frame)
{
    if (frame == 0)
    {
        return HOOCH_PROTOCOL_KEY_STATUS_RESULT_INVALID_PARAM;
    }

    if (HOOCH_PROTOCOL_KeyStatus_IsValidControlItem(frame->control_item) == 0U)
    {
        return HOOCH_PROTOCOL_KEY_STATUS_RESULT_INVALID_PARAM;
    }

    switch (frame->control_item)
    {
        case HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_ALL:
        case HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_STATE:
            s_hooch_protocol_key_status_report_frame.state = frame->state;
            break;

        default:
            return HOOCH_PROTOCOL_KEY_STATUS_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_key_status_report_frame.key = frame->key;
    s_hooch_protocol_key_status_report_frame.control_item = frame->control_item;
    s_hooch_protocol_key_status_report_frame.sequence = frame->sequence;
    s_hooch_protocol_key_status_report_frame.valid = 1U;

    HOOCH_PROTOCOL_KeyStatus_NotifyReportCallback();

    return HOOCH_PROTOCOL_KEY_STATUS_RESULT_OK;
}

/* 获取当前保存的按键状态数据帧（只读）。 */
const HOOCH_PROTOCOL_KeyStatusFrame_t *HOOCH_PROTOCOL_KeyStatus_GetFrame(void)
{
    return &s_hooch_protocol_key_status_report_frame;
}

/* 注册按键状态上报回调函数（允许覆盖已有回调）。 */
void HOOCH_PROTOCOL_KeyStatus_RegisterReportCallback(
    HOOCH_PROTOCOL_KeyStatusReportCallback_t callback)
{
    s_hooch_protocol_key_status_report_callback = callback;
}

/* 注销按键状态上报回调函数。 */
void HOOCH_PROTOCOL_KeyStatus_UnregisterReportCallback(void)
{
    s_hooch_protocol_key_status_report_callback = 0;
}
