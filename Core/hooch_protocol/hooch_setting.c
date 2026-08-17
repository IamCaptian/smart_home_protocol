#include "hooch_setting.h"

/* 保存当前一次设置数据。 */
static HOOCH_PROTOCOL_SettingFrame_t s_hooch_protocol_setting_frame;

/* 保存外部注册的设置通知回调。 */
static HOOCH_PROTOCOL_SettingCallback_t s_hooch_protocol_setting_callback;

/* 有回调注册时，把最新数据通知出去。 */
static void HOOCH_PROTOCOL_Setting_NotifyCallback(void)
{
    if (s_hooch_protocol_setting_callback != 0)
    {
        s_hooch_protocol_setting_callback(&s_hooch_protocol_setting_frame);
    }
}

void HOOCH_Setting_Init(void)
{
    /* 初始化时先清掉回调，避免使用野指针。 */
    s_hooch_protocol_setting_callback = 0;
    HOOCH_PROTOCOL_Setting_Clear();
}

void HOOCH_PROTOCOL_Setting_Clear(void)
{
    s_hooch_protocol_setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_INVALID;
    s_hooch_protocol_setting_frame.page = HOOCH_PROTOCOL_SETTING_PAGE_INVALID;
    s_hooch_protocol_setting_frame.screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_INVALID;
    s_hooch_protocol_setting_frame.param1 = 0xFFU;
    s_hooch_protocol_setting_frame.param2 = 0xFFU;
    s_hooch_protocol_setting_frame.param3 = 0xFFU;
    s_hooch_protocol_setting_frame.param4 = 0xFFU;
    s_hooch_protocol_setting_frame.value = 0U;
    s_hooch_protocol_setting_frame.sequence = 0U;
    s_hooch_protocol_setting_frame.valid = 0U;
}

HOOCH_PROTOCOL_SettingResult_t HOOCH_PROTOCOL_Setting_SetValue(uint8_t value)
{
    s_hooch_protocol_setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_INVALID;
    s_hooch_protocol_setting_frame.page = HOOCH_PROTOCOL_SETTING_PAGE_INVALID;
    s_hooch_protocol_setting_frame.screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_INVALID;
    s_hooch_protocol_setting_frame.param1 = 0xFFU;
    s_hooch_protocol_setting_frame.param2 = 0xFFU;
    s_hooch_protocol_setting_frame.param3 = 0xFFU;
    s_hooch_protocol_setting_frame.param4 = 0xFFU;
    s_hooch_protocol_setting_frame.value = value;
    s_hooch_protocol_setting_frame.sequence++;
    s_hooch_protocol_setting_frame.valid = 1U;

    /* 当前帧更新成功后，通知外部业务层。 */
    HOOCH_PROTOCOL_Setting_NotifyCallback();

    return HOOCH_PROTOCOL_SETTING_RESULT_OK;
}

HOOCH_PROTOCOL_SettingResult_t HOOCH_PROTOCOL_Setting_SetFrame(
    const HOOCH_PROTOCOL_SettingFrame_t *frame)
{
    if (frame == 0)
    {
        /* 空指针不允许写入。 */
        return HOOCH_PROTOCOL_SETTING_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_setting_frame = *frame;
    s_hooch_protocol_setting_frame.valid = 1U;

    /* 直接写帧成功后，也同步回调一次。 */
    HOOCH_PROTOCOL_Setting_NotifyCallback();

    return HOOCH_PROTOCOL_SETTING_RESULT_OK;
}

const HOOCH_PROTOCOL_SettingFrame_t *HOOCH_PROTOCOL_Setting_GetFrame(void)
{
    return &s_hooch_protocol_setting_frame;
}

void HOOCH_PROTOCOL_Setting_RegisterCallback(
    HOOCH_PROTOCOL_SettingCallback_t callback)
{
    /* 允许外部替换为新的回调函数。 */
    s_hooch_protocol_setting_callback = callback;
}

void HOOCH_PROTOCOL_Setting_UnregisterCallback(void)
{
    /* 注销后不再向外通知。 */
    s_hooch_protocol_setting_callback = 0;
}

HOOCH_PROTOCOL_SettingResult_t HOOCH_PROTOCOL_Setting_Send(uint8_t value)
{
    /* 这里先保留为统一入口，后续可接真实协议流程。 */
    return HOOCH_PROTOCOL_Setting_SetValue(value);
}
