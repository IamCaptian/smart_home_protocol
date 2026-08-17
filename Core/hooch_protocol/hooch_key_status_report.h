#ifndef HOOCH_KEY_STATUS_REPORT_H
#define HOOCH_KEY_STATUS_REPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
/* 支持的按键数量。 */
#define HOOCH_PROTOCOL_KEY_STATUS_REPORT_KEY_COUNT    8U

/* 按键状态上报接口返回结果。 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_STATUS_REPORT_RESULT_OK = 0U,
    HOOCH_PROTOCOL_KEY_STATUS_REPORT_RESULT_ERROR,
    HOOCH_PROTOCOL_KEY_STATUS_REPORT_RESULT_INVALID_PARAM,
} HOOCH_PROTOCOL_KeyStatusReportResult_t;

/* 按键编号枚举。 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_STATUS_REPORT_KEY_INVALID = 0U,
    HOOCH_PROTOCOL_KEY_STATUS_REPORT_KEY_1 = 1U,
    HOOCH_PROTOCOL_KEY_STATUS_REPORT_KEY_2,
    HOOCH_PROTOCOL_KEY_STATUS_REPORT_KEY_3,
    HOOCH_PROTOCOL_KEY_STATUS_REPORT_KEY_4,
    HOOCH_PROTOCOL_KEY_STATUS_REPORT_KEY_5,
    HOOCH_PROTOCOL_KEY_STATUS_REPORT_KEY_6,
    HOOCH_PROTOCOL_KEY_STATUS_REPORT_KEY_7,
    HOOCH_PROTOCOL_KEY_STATUS_REPORT_KEY_8,
} HOOCH_PROTOCOL_KeyStatusReportKey_t;

/* 按键状态枚举（关/开）。 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_STATUS_REPORT_STATE_OFF = 0U,
    HOOCH_PROTOCOL_KEY_STATUS_REPORT_STATE_ON = 1U,
} HOOCH_PROTOCOL_KeyStatusReportState_t;


/* 按键状态上报帧结构。 */
typedef struct
{
    HOOCH_PROTOCOL_KeyStatusReportKey_t key;
    HOOCH_PROTOCOL_KeyStatusReportState_t state;
    uint8_t sequence;
    uint8_t valid;
} HOOCH_PROTOCOL_KeyStatusReportFrame_t;

/* 按键状态上报回调函数类型。 */
typedef void (*HOOCH_PROTOCOL_KeyStatusReportCallback_t)(
    const HOOCH_PROTOCOL_KeyStatusReportFrame_t *frame);

/* 初始化按键状态上报模块。 */
void HOOCH_KeyStatusReport_Init(void);

/* 清空当前保存的按键状态上报数据。 */
void HOOCH_PROTOCOL_KeyStatusReport_Clear(void);

/* 校验按键编号是否合法（目前支持 1~8）。 */
uint8_t HOOCH_PROTOCOL_KeyStatusReport_IsValidKey(HOOCH_PROTOCOL_KeyStatusReportKey_t key);

/* 设置一条按键状态并触发回调。 */
HOOCH_PROTOCOL_KeyStatusReportResult_t HOOCH_PROTOCOL_KeyStatusReport_Set(
    HOOCH_PROTOCOL_KeyStatusReportKey_t key,
    HOOCH_PROTOCOL_KeyStatusReportState_t state);

/* 直接写入完整帧并触发回调。 */
HOOCH_PROTOCOL_KeyStatusReportResult_t HOOCH_PROTOCOL_KeyStatusReport_SetFrame(
    const HOOCH_PROTOCOL_KeyStatusReportFrame_t *frame);

/* 获取当前保存的按键状态上报帧（只读）。 */
const HOOCH_PROTOCOL_KeyStatusReportFrame_t *HOOCH_PROTOCOL_KeyStatusReport_GetFrame(void);

/* 注册按键状态上报回调函数（允许覆盖已有回调）。 */
void HOOCH_PROTOCOL_KeyStatusReport_RegisterCallback(
    HOOCH_PROTOCOL_KeyStatusReportCallback_t callback);

/* 注销按键状态上报回调函数。 */
void HOOCH_PROTOCOL_KeyStatusReport_UnregisterCallback(void);

/* 统一的发送入口：当前版本等价于 Set(key, state)。 */
HOOCH_PROTOCOL_KeyStatusReportResult_t HOOCH_PROTOCOL_KeyStatusReport_Send(
    HOOCH_PROTOCOL_KeyStatusReportKey_t key,
    HOOCH_PROTOCOL_KeyStatusReportState_t state);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_KEY_STATUS_REPORT_H */
