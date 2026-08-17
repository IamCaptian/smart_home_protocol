#ifndef HOOCH_CODE_MATCH_DISPATCH_DISPATCH_H
#define HOOCH_CODE_MATCH_DISPATCH_DISPATCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hooch_code_match_dispatch_report.h"

/* 对码下发回调函数类型（复用 report 帧结构，语义为"被下发的对码数据"） */
typedef void (*HOOCH_PROTOCOL_CodeMatchDispatchCallback_t)(
    const HOOCH_PROTOCOL_CodeMatchReportFrame_t *frame);

/* 初始化对码下发模块 */
void HOOCH_CodeMatchDispatch_Init(void);

/* 清空当前对码下发数据 */
void HOOCH_PROTOCOL_CodeMatchDispatch_Clear(void);

/* 写入对码下发数据，更新缓存并触发已注册的下发回调 */
HOOCH_PROTOCOL_CodeMatchReportResult_t HOOCH_PROTOCOL_CodeMatchDispatch_Set(
    uint8_t page,
    uint8_t channel,
    HOOCH_PROTOCOL_CodeMatchReportAction_t action);

/* 以帧方式写入对码下发数据，更新缓存并触发已注册的下发回调 */
HOOCH_PROTOCOL_CodeMatchReportResult_t HOOCH_PROTOCOL_CodeMatchDispatch_SetFrame(
    const HOOCH_PROTOCOL_CodeMatchReportFrame_t *frame);

/* 获取当前缓存的对码下发数据帧 */
const HOOCH_PROTOCOL_CodeMatchReportFrame_t *HOOCH_PROTOCOL_CodeMatchDispatch_GetFrame(void);

/* 注册对码下发回调函数（接收远程对码指令时触发） */
void HOOCH_PROTOCOL_CodeMatchDispatch_RegisterCallback(
    HOOCH_PROTOCOL_CodeMatchDispatchCallback_t callback);

/* 注销对码下发回调函数 */
void HOOCH_PROTOCOL_CodeMatchDispatch_UnregisterCallback(void);

/* 接收远程对码下发指令（协议层入口，内部调用 Set 更新缓存并通知回调） */
HOOCH_PROTOCOL_CodeMatchReportResult_t HOOCH_PROTOCOL_CodeMatchDispatch_Dispatch(
    uint8_t page,
    uint8_t channel,
    HOOCH_PROTOCOL_CodeMatchReportAction_t action);

/* 以帧方式接收远程对码下发指令（协议层入口，内部调用 SetFrame 更新缓存并通知回调） */
HOOCH_PROTOCOL_CodeMatchReportResult_t HOOCH_PROTOCOL_CodeMatchDispatch_DispatchFrame(
    const HOOCH_PROTOCOL_CodeMatchReportFrame_t *frame);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_CODE_MATCH_DISPATCH_DISPATCH_H */
