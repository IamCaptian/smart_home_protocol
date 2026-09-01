#include "hooch_scene_report.h"

static HOOCH_PROTOCOL_SceneReportFrame_t s_hooch_protocol_scene_report_frame;
static HOOCH_PROTOCOL_SceneReportCallback_t s_hooch_protocol_scene_report_callback;

static HOOCH_PROTOCOL_SceneReportReportCallback1_t s_hooch_protocol_scene_report_report_callback1;
static HOOCH_PROTOCOL_SceneReportReportCallback2_t s_hooch_protocol_scene_report_report_callback2;
static HOOCH_PROTOCOL_SceneReportReportCallback3_t s_hooch_protocol_scene_report_report_callback3;
static HOOCH_PROTOCOL_SceneReportReportCallback4_t s_hooch_protocol_scene_report_report_callback4;

static void HOOCH_PROTOCOL_SceneReport_NotifyCallback(void)
{
    if (s_hooch_protocol_scene_report_callback != 0)
    {
        s_hooch_protocol_scene_report_callback(&s_hooch_protocol_scene_report_frame);
    }
}

static void HOOCH_PROTOCOL_SceneReport_NotifyReportCallback1(void)
{
    if (s_hooch_protocol_scene_report_report_callback1 != 0)
    {
        s_hooch_protocol_scene_report_report_callback1(&s_hooch_protocol_scene_report_frame);
    }
}

static void HOOCH_PROTOCOL_SceneReport_NotifyReportCallback2(void)
{
    if (s_hooch_protocol_scene_report_report_callback2 != 0)
    {
        s_hooch_protocol_scene_report_report_callback2(&s_hooch_protocol_scene_report_frame);
    }
}

static void HOOCH_PROTOCOL_SceneReport_NotifyReportCallback3(void)
{
    if (s_hooch_protocol_scene_report_report_callback3 != 0)
    {
        s_hooch_protocol_scene_report_report_callback3(&s_hooch_protocol_scene_report_frame);
    }
}

static void HOOCH_PROTOCOL_SceneReport_NotifyReportCallback4(void)
{
    if (s_hooch_protocol_scene_report_report_callback4 != 0)
    {
        s_hooch_protocol_scene_report_report_callback4(&s_hooch_protocol_scene_report_frame);
    }
}

void HOOCH_SceneReport_Init(void)
{
    s_hooch_protocol_scene_report_callback = 0;
    s_hooch_protocol_scene_report_report_callback1 = 0;
    s_hooch_protocol_scene_report_report_callback2 = 0;
    s_hooch_protocol_scene_report_report_callback3 = 0;
    s_hooch_protocol_scene_report_report_callback4 = 0;
    HOOCH_PROTOCOL_SceneReport_Clear();
}

void HOOCH_PROTOCOL_SceneReport_Clear(void)
{
    s_hooch_protocol_scene_report_frame.key = HOOCH_PROTOCOL_SCENE_REPORT_KEY_INVALID;
    s_hooch_protocol_scene_report_frame.type = HOOCH_PROTOCOL_SCENE_REPORT_TYPE_INVALID;
    s_hooch_protocol_scene_report_frame.page = 0U;
    s_hooch_protocol_scene_report_frame.address = 0U;
    s_hooch_protocol_scene_report_frame.sequence = 0U;
    s_hooch_protocol_scene_report_frame.valid = 0U;
}

uint8_t HOOCH_PROTOCOL_SceneReport_IsValidKey(HOOCH_PROTOCOL_SceneReportKey_t key)
{
    return (uint8_t)((key >= HOOCH_PROTOCOL_SCENE_REPORT_KEY_1) &&
                     (key <= HOOCH_PROTOCOL_SCENE_REPORT_KEY_8));
}

uint8_t HOOCH_PROTOCOL_SceneReport_IsValidType(HOOCH_PROTOCOL_SceneReportType_t type)
{
    return (uint8_t)((type >= HOOCH_PROTOCOL_SCENE_REPORT_TYPE_SINGLE_CLICK) &&
                     (type <= HOOCH_PROTOCOL_SCENE_REPORT_TYPE_LONG_CLICK));
}

HOOCH_PROTOCOL_SceneReportResult_t HOOCH_PROTOCOL_SceneReport_Set(
    HOOCH_PROTOCOL_SceneReportKey_t key)
{
    if (HOOCH_PROTOCOL_SceneReport_IsValidKey(key) == 0U)
    {
        return HOOCH_PROTOCOL_SCENE_REPORT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_scene_report_frame.key = key;
    s_hooch_protocol_scene_report_frame.sequence++;
    s_hooch_protocol_scene_report_frame.valid = 1U;
    HOOCH_PROTOCOL_SceneReport_NotifyCallback();
    HOOCH_PROTOCOL_SceneReport_NotifyReportCallback1();
    return HOOCH_PROTOCOL_SCENE_REPORT_RESULT_OK;
}



HOOCH_PROTOCOL_SceneReportResult_t HOOCH_PROTOCOL_SceneReport_SetFrame(
    const HOOCH_PROTOCOL_SceneReportFrame_t *frame)
{
    if ((frame == 0) || (HOOCH_PROTOCOL_SceneReport_IsValidKey(frame->key) == 0U))
    {
        return HOOCH_PROTOCOL_SCENE_REPORT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_scene_report_frame = *frame;
    s_hooch_protocol_scene_report_frame.valid = 1U;
    HOOCH_PROTOCOL_SceneReport_NotifyCallback();
    HOOCH_PROTOCOL_SceneReport_NotifyReportCallback1();
    return HOOCH_PROTOCOL_SCENE_REPORT_RESULT_OK;
}

const HOOCH_PROTOCOL_SceneReportFrame_t *HOOCH_PROTOCOL_SceneReport_GetFrame(void)
{
    return &s_hooch_protocol_scene_report_frame;
}

void HOOCH_PROTOCOL_SceneReport_RegisterCallback(
    HOOCH_PROTOCOL_SceneReportCallback_t callback)
{
    s_hooch_protocol_scene_report_callback = callback;
}

void HOOCH_PROTOCOL_SceneReport_UnregisterCallback(void)
{
    s_hooch_protocol_scene_report_callback = 0;
}

void HOOCH_PROTOCOL_SceneReport_RegisterReportCallback(
    HOOCH_PROTOCOL_SceneReportReportCallback1_t callback)
{
    s_hooch_protocol_scene_report_report_callback1 = callback;
}

void HOOCH_PROTOCOL_SceneReport_UnregisterReportCallback(void)
{
    s_hooch_protocol_scene_report_report_callback1 = 0;
}

void HOOCH_PROTOCOL_SceneReport_RegisterReportCallback2(
    HOOCH_PROTOCOL_SceneReportReportCallback2_t callback)
{
    s_hooch_protocol_scene_report_report_callback2 = callback;
}

void HOOCH_PROTOCOL_SceneReport_UnregisterReportCallback2(void)
{
    s_hooch_protocol_scene_report_report_callback2 = 0;
}

void HOOCH_PROTOCOL_SceneReport_RegisterReportCallback3(
    HOOCH_PROTOCOL_SceneReportReportCallback3_t callback)
{
    s_hooch_protocol_scene_report_report_callback3 = callback;
}

void HOOCH_PROTOCOL_SceneReport_UnregisterReportCallback3(void)
{
    s_hooch_protocol_scene_report_report_callback3 = 0;
}

void HOOCH_PROTOCOL_SceneReport_RegisterReportCallback4(
    HOOCH_PROTOCOL_SceneReportReportCallback4_t callback)
{
    s_hooch_protocol_scene_report_report_callback4 = callback;
}

void HOOCH_PROTOCOL_SceneReport_UnregisterReportCallback4(void)
{
    s_hooch_protocol_scene_report_report_callback4 = 0;
}

/* ======================== 类型1 上报：仅key ======================== */

HOOCH_PROTOCOL_SceneReportResult_t HOOCH_PROTOCOL_SceneReport_Send(
    HOOCH_PROTOCOL_SceneReportKey_t key)
{
    return HOOCH_PROTOCOL_SceneReport_Set(key);
}

/* ======================== 类型2 上报：页面+key ======================== */

HOOCH_PROTOCOL_SceneReportResult_t HOOCH_PROTOCOL_SceneReport_SendPage(
    uint8_t page,
    HOOCH_PROTOCOL_SceneReportKey_t key)
{
    if (HOOCH_PROTOCOL_SceneReport_IsValidKey(key) == 0U)
    {
        return HOOCH_PROTOCOL_SCENE_REPORT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_scene_report_frame.page = page;
    s_hooch_protocol_scene_report_frame.key = key;
    s_hooch_protocol_scene_report_frame.sequence++;
    s_hooch_protocol_scene_report_frame.valid = 1U;

    HOOCH_PROTOCOL_SceneReport_NotifyReportCallback2();

    return HOOCH_PROTOCOL_SCENE_REPORT_RESULT_OK;
}

/* ======================== 类型3 上报：地址+key ======================== */

HOOCH_PROTOCOL_SceneReportResult_t HOOCH_PROTOCOL_SceneReport_SendAddr(
    uint16_t address,
    HOOCH_PROTOCOL_SceneReportKey_t key)
{
    if (HOOCH_PROTOCOL_SceneReport_IsValidKey(key) == 0U)
    {
        return HOOCH_PROTOCOL_SCENE_REPORT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_scene_report_frame.address = address;
    s_hooch_protocol_scene_report_frame.key = key;
    s_hooch_protocol_scene_report_frame.sequence++;
    s_hooch_protocol_scene_report_frame.valid = 1U;

    HOOCH_PROTOCOL_SceneReport_NotifyReportCallback3();

    return HOOCH_PROTOCOL_SCENE_REPORT_RESULT_OK;
}

/* ======================== 类型4 上报：页面+通道+类型（小米专用） ======================== */

HOOCH_PROTOCOL_SceneReportResult_t HOOCH_PROTOCOL_SceneReport_SendXiaomi(
    uint8_t page,
    HOOCH_PROTOCOL_SceneReportKey_t key,
    HOOCH_PROTOCOL_SceneReportType_t type)
{
    if ((HOOCH_PROTOCOL_SceneReport_IsValidKey(key) == 0U) ||
        (HOOCH_PROTOCOL_SceneReport_IsValidType(type) == 0U))
    {
        return HOOCH_PROTOCOL_SCENE_REPORT_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_scene_report_frame.page = page;
    s_hooch_protocol_scene_report_frame.key = key;
    s_hooch_protocol_scene_report_frame.type = type;
    s_hooch_protocol_scene_report_frame.sequence++;
    s_hooch_protocol_scene_report_frame.valid = 1U;

    HOOCH_PROTOCOL_SceneReport_NotifyReportCallback4();

    return HOOCH_PROTOCOL_SCENE_REPORT_RESULT_OK;
}

