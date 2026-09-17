#ifndef HOOCH_SCENE_REPORT_H
#define HOOCH_SCENE_REPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include "hooch_protocol_common.h"
/* 场景上报模块支持的按键通道数 */
#define HOOCH_PROTOCOL_SCENE_REPORT_KEY_COUNT    16U

/* 场景上报接口返回结果 */
typedef enum
{
    HOOCH_PROTOCOL_SCENE_REPORT_RESULT_OK = 0U,          /* 执行成功 */
    HOOCH_PROTOCOL_SCENE_REPORT_RESULT_ERROR,            /* 通用错误 */
    HOOCH_PROTOCOL_SCENE_REPORT_RESULT_INVALID_PARAM,    /* 参数非法 */
} HOOCH_PROTOCOL_SceneReportResult_t;

/* 场景上报按键编号枚举 */
typedef enum
{
    HOOCH_PROTOCOL_SCENE_REPORT_KEY_INVALID = 0U, /* 无效按键 */
    HOOCH_PROTOCOL_SCENE_REPORT_KEY_1 = 1U,       /* 按键1 */
    HOOCH_PROTOCOL_SCENE_REPORT_KEY_2,
    HOOCH_PROTOCOL_SCENE_REPORT_KEY_3,
    HOOCH_PROTOCOL_SCENE_REPORT_KEY_4,
    HOOCH_PROTOCOL_SCENE_REPORT_KEY_5,
    HOOCH_PROTOCOL_SCENE_REPORT_KEY_6,
    HOOCH_PROTOCOL_SCENE_REPORT_KEY_7,
    HOOCH_PROTOCOL_SCENE_REPORT_KEY_8,
    HOOCH_PROTOCOL_SCENE_REPORT_KEY_9,
    HOOCH_PROTOCOL_SCENE_REPORT_KEY_10,
    HOOCH_PROTOCOL_SCENE_REPORT_KEY_11,
    HOOCH_PROTOCOL_SCENE_REPORT_KEY_12,
    HOOCH_PROTOCOL_SCENE_REPORT_KEY_13,
    HOOCH_PROTOCOL_SCENE_REPORT_KEY_14,
    HOOCH_PROTOCOL_SCENE_REPORT_KEY_15,
    HOOCH_PROTOCOL_SCENE_REPORT_KEY_16,
} HOOCH_PROTOCOL_SceneReportKey_t;

/* 场景上报按键类型（小米专用） */
typedef enum
{
    HOOCH_PROTOCOL_SCENE_REPORT_TYPE_INVALID = 0U,  /* 无效类型 */
    HOOCH_PROTOCOL_SCENE_REPORT_TYPE_SINGLE_CLICK,  /* 单击 */
    HOOCH_PROTOCOL_SCENE_REPORT_TYPE_DOUBLE_CLICK,  /* 双击 */
    HOOCH_PROTOCOL_SCENE_REPORT_TYPE_LONG_CLICK,    /* 长按 */
} HOOCH_PROTOCOL_SceneReportType_t;

/* 场景上报动作类型（类型1上报使用） */
typedef enum
{
    HOOCH_PROTOCOL_SCENE_REPORT_ACTION_TRIGGER = 0U,  /* 触发：1=触发 */
    HOOCH_PROTOCOL_SCENE_REPORT_ACTION_LEARN,         /* 学习：1=学习 */
} HOOCH_PROTOCOL_SceneReportAction_t;

/* 场景上报帧描述 */
typedef struct
{
    HOOCH_PROTOCOL_SceneReportKey_t key;        /* 按键编号（通道） */
    HOOCH_PROTOCOL_SceneReportAction_t action;  /* 上报动作（触发/学习） */
    HOOCH_PROTOCOL_SceneReportType_t type;      /* 按键类型（小米上报使用） */
    uint8_t page;                               /* 页面编号（上报类型2使用） */
    uint16_t address;                           /* 寄存器地址（上报类型3使用） */
    HOOCH_PROTOCOL_Source_t source;             /* 消息来源 */
    uint8_t sequence;                           /* 更新序号 */
    uint8_t valid;                              /* 当前数据是否有效 */
} HOOCH_PROTOCOL_SceneReportFrame_t;

/* 场景上报回调函数类型 */
typedef void (*HOOCH_PROTOCOL_SceneReportCallback_t)(
    const HOOCH_PROTOCOL_SceneReportFrame_t *frame);

/* 场景上报回调函数类型1：仅场景+数值 */
typedef void (*HOOCH_PROTOCOL_SceneReportReportCallback1_t)(
    const HOOCH_PROTOCOL_SceneReportFrame_t *frame);

/* 场景上报回调函数类型2：页面+场景+数值 */
typedef void (*HOOCH_PROTOCOL_SceneReportReportCallback2_t)(
    const HOOCH_PROTOCOL_SceneReportFrame_t *frame);

/* 场景上报回调函数类型3：地址+数值 */
typedef void (*HOOCH_PROTOCOL_SceneReportReportCallback3_t)(
    const HOOCH_PROTOCOL_SceneReportFrame_t *frame);

/* 场景上报回调函数类型4：页面+通道+类型（小米专用） */
typedef void (*HOOCH_PROTOCOL_SceneReportReportCallback4_t)(
    const HOOCH_PROTOCOL_SceneReportFrame_t *frame);

void HOOCH_SceneReport_Init(void);
void HOOCH_PROTOCOL_SceneReport_Clear(void);
uint8_t HOOCH_PROTOCOL_SceneReport_IsValidKey(HOOCH_PROTOCOL_SceneReportKey_t key);
uint8_t HOOCH_PROTOCOL_SceneReport_IsValidType(HOOCH_PROTOCOL_SceneReportType_t type);
HOOCH_PROTOCOL_SceneReportResult_t HOOCH_PROTOCOL_SceneReport_Set(
    HOOCH_PROTOCOL_SceneReportKey_t key);
HOOCH_PROTOCOL_SceneReportResult_t HOOCH_PROTOCOL_SceneReport_SetFrame(
    const HOOCH_PROTOCOL_SceneReportFrame_t *frame);
const HOOCH_PROTOCOL_SceneReportFrame_t *HOOCH_PROTOCOL_SceneReport_GetFrame(void);

/* 注册场景下发回调函数 */
void HOOCH_PROTOCOL_SceneReport_RegisterCallback(
    HOOCH_PROTOCOL_SceneReportCallback_t callback);

/* 注销场景下发回调函数 */
void HOOCH_PROTOCOL_SceneReport_UnregisterCallback(void);

/* 注册场景上报回调函数（类型1：仅场景+数值） */
void HOOCH_PROTOCOL_SceneReport_RegisterReportCallback(
    HOOCH_PROTOCOL_SceneReportReportCallback1_t callback);

/* 注销场景上报回调函数（类型1） */
void HOOCH_PROTOCOL_SceneReport_UnregisterReportCallback(void);

/* 注册场景上报回调函数（类型2：页面+场景+数值） */
void HOOCH_PROTOCOL_SceneReport_RegisterReportCallback2(
    HOOCH_PROTOCOL_SceneReportReportCallback2_t callback);

/* 注销场景上报回调函数（类型2） */
void HOOCH_PROTOCOL_SceneReport_UnregisterReportCallback2(void);

/* 注册场景上报回调函数（类型3：地址+数值） */
void HOOCH_PROTOCOL_SceneReport_RegisterReportCallback3(
    HOOCH_PROTOCOL_SceneReportReportCallback3_t callback);

/* 注销场景上报回调函数（类型3） */
void HOOCH_PROTOCOL_SceneReport_UnregisterReportCallback3(void);

/* 注册场景上报回调函数（类型4：页面+通道+类型，小米专用） */
void HOOCH_PROTOCOL_SceneReport_RegisterReportCallback4(
    HOOCH_PROTOCOL_SceneReportReportCallback4_t callback);

/* 注销场景上报回调函数（类型4） */
void HOOCH_PROTOCOL_SceneReport_UnregisterReportCallback4(void);

/* ---- 类型1 上报：仅key ---- */

/* 上报场景触发（类型1：仅key） */
HOOCH_PROTOCOL_SceneReportResult_t HOOCH_PROTOCOL_SceneReport_Send(
    HOOCH_PROTOCOL_SceneReportKey_t key);

/* 上报场景学习（类型1：仅key） */
HOOCH_PROTOCOL_SceneReportResult_t HOOCH_PROTOCOL_SceneReport_SendLearn(
    HOOCH_PROTOCOL_SceneReportKey_t key);

/* ---- 类型2 上报：页面+key ---- */

/* 上报场景按键（类型2：页面+key） */
HOOCH_PROTOCOL_SceneReportResult_t HOOCH_PROTOCOL_SceneReport_SendPage(
    uint8_t page,
    HOOCH_PROTOCOL_SceneReportKey_t key);

/* ---- 类型3 上报：地址+key ---- */

/* 上报场景按键（类型3：地址+key） */
HOOCH_PROTOCOL_SceneReportResult_t HOOCH_PROTOCOL_SceneReport_SendAddr(
    uint16_t address,
    HOOCH_PROTOCOL_SceneReportKey_t key);

/* ---- 类型4 上报：页面+通道+类型（小米专用） ---- */

/* 上报场景按键（类型4：页面+通道+类型） */
HOOCH_PROTOCOL_SceneReportResult_t HOOCH_PROTOCOL_SceneReport_SendXiaomi(
    uint8_t page,
    HOOCH_PROTOCOL_SceneReportKey_t key,
    HOOCH_PROTOCOL_SceneReportType_t type);




    
#ifdef __cplusplus
}
#endif

#endif /* HOOCH_SCENE_REPORT_H */
