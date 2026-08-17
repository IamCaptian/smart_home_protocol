#ifndef HOOCH_CODE_MATCH_DISPATCH_REPORT_H
#define HOOCH_CODE_MATCH_DISPATCH_REPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 对码上报接口返回结果 */
typedef enum
{
    HOOCH_PROTOCOL_CODE_MATCH_REPORT_RESULT_OK = 0U,          /* 执行成功 */
    HOOCH_PROTOCOL_CODE_MATCH_REPORT_RESULT_ERROR,            /* 通用错误 */
    HOOCH_PROTOCOL_CODE_MATCH_REPORT_RESULT_INVALID_PARAM,    /* 参数非法 */
} HOOCH_PROTOCOL_CodeMatchReportResult_t;

/* 对码操作类型（与线值对齐：0=清码, 1=对码） */
typedef enum
{
    HOOCH_PROTOCOL_CODE_MATCH_REPORT_ACTION_CLEAR = 0U,    /* 清码 */
    HOOCH_PROTOCOL_CODE_MATCH_REPORT_ACTION_MATCH = 1U,    /* 对码 */
} HOOCH_PROTOCOL_CodeMatchReportAction_t;

/* 页面编号（与协议 Byte[0] 对齐） */
typedef enum
{
    HOOCH_PROTOCOL_CODE_MATCH_PAGE_SWITCH       = 0x00U,    /* 开关页面 */
    HOOCH_PROTOCOL_CODE_MATCH_PAGE_SCENE        = 0x01U,    /* 情景页面 */
    HOOCH_PROTOCOL_CODE_MATCH_PAGE_LIGHT        = 0x02U,    /* 灯光页面 */
    HOOCH_PROTOCOL_CODE_MATCH_PAGE_CURTAIN      = 0x03U,    /* 窗帘页面 */
    HOOCH_PROTOCOL_CODE_MATCH_PAGE_AC           = 0x04U,    /* 空调页面 */
    HOOCH_PROTOCOL_CODE_MATCH_PAGE_FRESH_AIR    = 0x05U,    /* 新风页面 */
    HOOCH_PROTOCOL_CODE_MATCH_PAGE_FLOOR_HEATING = 0x06U,   /* 地暖页面 */
    HOOCH_PROTOCOL_CODE_MATCH_PAGE_LOCAL_SCENE  = 0x07U,    /* 本地情景页面 */
} HOOCH_PROTOCOL_CodeMatchPage_t;

/*
 * 对码上报帧描述（与协议3字节直接映射）
 *
 *   Byte[0] = page    页面 (0x00~0x07)
 *   Byte[1] = channel 通道（编码规则因页面而异，由业务层自行组装）
 *   Byte[2] = action  操作 (0=清码, 1=对码)
 */
typedef struct
{
    uint8_t page;                                   /* 当前页面 */
    uint8_t channel;                                /* 通道 */
    HOOCH_PROTOCOL_CodeMatchReportAction_t action;  /* 对码/清码 */
    uint8_t sequence;                               /* 更新序号 */
    uint8_t valid;                                  /* 当前数据是否有效 */
} HOOCH_PROTOCOL_CodeMatchReportFrame_t;

/* 对码上报回调函数类型 */
typedef void (*HOOCH_PROTOCOL_CodeMatchReportCallback_t)(
    const HOOCH_PROTOCOL_CodeMatchReportFrame_t *frame);

/* 初始化 */
void HOOCH_CodeMatchReport_Init(void);

/* 清空当前数据 */
void HOOCH_PROTOCOL_CodeMatchReport_Clear(void);

/* 通用写入并通知回调（page/channel/action 直写，不编码） */
HOOCH_PROTOCOL_CodeMatchReportResult_t HOOCH_PROTOCOL_CodeMatchReport_Set(
    uint8_t page,
    uint8_t channel,
    HOOCH_PROTOCOL_CodeMatchReportAction_t action);

/* 帧方式写入 */
HOOCH_PROTOCOL_CodeMatchReportResult_t HOOCH_PROTOCOL_CodeMatchReport_SetFrame(
    const HOOCH_PROTOCOL_CodeMatchReportFrame_t *frame);

/* 获取当前缓存帧 */
const HOOCH_PROTOCOL_CodeMatchReportFrame_t *HOOCH_PROTOCOL_CodeMatchReport_GetFrame(void);

/* 注册/注销回调 */
void HOOCH_PROTOCOL_CodeMatchReport_RegisterCallback(
    HOOCH_PROTOCOL_CodeMatchReportCallback_t callback);
    
void HOOCH_PROTOCOL_CodeMatchReport_UnregisterCallback(void);

/* 通用上报入口（= Set，业务层自行组装 channel 后调用） */
HOOCH_PROTOCOL_CodeMatchReportResult_t HOOCH_PROTOCOL_CodeMatchReport_Send(
    uint8_t page,
    uint8_t channel,
    HOOCH_PROTOCOL_CodeMatchReportAction_t action);

/*
 * ======================== 开关页面对码上报（语义化接口） ========================
 * 开关页面(0x00)的 item 编码: 高4bit=按键索引(0~3), 低4bit=模式(0=调光,1=窗帘)
 * 调用方无需手算 channel 字节，直接传按键/模式/动作即可。
 */

/* 开关页面对码回调：专用于开关页面事件，传入已解码的 key/mode */
typedef void (*HOOCH_PROTOCOL_CodeMatchReportSwitchCallback_t)(
    uint8_t page,
    uint8_t key,
    uint8_t mode,
    HOOCH_PROTOCOL_CodeMatchReportAction_t action);

void HOOCH_PROTOCOL_CodeMatchReport_RegisterSwitchCallback(
    HOOCH_PROTOCOL_CodeMatchReportSwitchCallback_t callback);

void HOOCH_PROTOCOL_CodeMatchReport_UnregisterSwitchCallback(void);

/* 开关页面对码上报: page=页面, key=按键(1~4), mode=0(调光)/1(窗帘), action=对码/清码 */
HOOCH_PROTOCOL_CodeMatchReportResult_t HOOCH_PROTOCOL_CodeMatchReport_SendSwitch(
    uint8_t page,
    uint8_t key,
    uint8_t mode,
    HOOCH_PROTOCOL_CodeMatchReportAction_t action);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_CODE_MATCH_DISPATCH_REPORT_H */
