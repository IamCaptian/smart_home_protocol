#ifndef HOOCH_KEY_STATUS_H
#define HOOCH_KEY_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include "hooch_protocol_common.h"
/* 支持的按键数量。 */
#define HOOCH_PROTOCOL_KEY_STATUS_KEY_COUNT    16U

/* 按键状态接口返回结果 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_STATUS_RESULT_OK = 0U,          /* 执行成功 */
    HOOCH_PROTOCOL_KEY_STATUS_RESULT_ERROR,            /* 通用错误 */
    HOOCH_PROTOCOL_KEY_STATUS_RESULT_INVALID_PARAM,    /* 参数非法 */
} HOOCH_PROTOCOL_KeyStatusResult_t;

/* 按键编号枚举。 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_STATUS_KEY_INVALID = 0U, /* 无效按键 */
    HOOCH_PROTOCOL_KEY_STATUS_KEY_1 = 1U,       /* 按键1 */
    HOOCH_PROTOCOL_KEY_STATUS_KEY_2,
    HOOCH_PROTOCOL_KEY_STATUS_KEY_3,
    HOOCH_PROTOCOL_KEY_STATUS_KEY_4,
    HOOCH_PROTOCOL_KEY_STATUS_KEY_5,
    HOOCH_PROTOCOL_KEY_STATUS_KEY_6,
    HOOCH_PROTOCOL_KEY_STATUS_KEY_7,
    HOOCH_PROTOCOL_KEY_STATUS_KEY_8,
    HOOCH_PROTOCOL_KEY_STATUS_KEY_9,
    HOOCH_PROTOCOL_KEY_STATUS_KEY_10,
    HOOCH_PROTOCOL_KEY_STATUS_KEY_11,
    HOOCH_PROTOCOL_KEY_STATUS_KEY_12,
    HOOCH_PROTOCOL_KEY_STATUS_KEY_13,
    HOOCH_PROTOCOL_KEY_STATUS_KEY_14,
    HOOCH_PROTOCOL_KEY_STATUS_KEY_15,
    HOOCH_PROTOCOL_KEY_STATUS_KEY_16,
} HOOCH_PROTOCOL_KeyStatusKey_t;

/* 按键状态枚举（关/开）。 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_STATUS_STATE_OFF = 0U, /* 关/松开 */
    HOOCH_PROTOCOL_KEY_STATUS_STATE_ON  = 1U, /* 开/按下 */
} HOOCH_PROTOCOL_KeyStatusState_t;

/* 按键状态控制项 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_INVALID = 0U,      /* 无效控制项 */
    HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_ALL,               /* 全量更新 */
    HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_STATE,             /* 仅更新开关状态 */
    HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_ENABLE_BIT,        /* 仅更新使能位(1 byte 位图, bit0:使能 bit1:开关) */
    HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_DEVICE_DESCRIPTOR, /* 仅更新设备描述字符串(24 byte, UTF-8) */
    HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_DEFAULT_ICON,      /* 仅更新默认图标(1 byte, 0-255) */
    HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_SELECTED_ICON,     /* 仅更新选中图标(1 byte, 0-255) */
    HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_KEY_MODE,          /* 仅更新按键模式(1 byte, 0-OFF/1-ON/2-Toggle) */
} HOOCH_PROTOCOL_KeyStatusControlItem_t;

/* 按键状态数据帧描述 */
typedef struct
{
    HOOCH_PROTOCOL_KeyStatusKey_t key;              /* 按键编号 */
    uint8_t page;                                   /* 页面编号（上报类型2使用） */
    uint16_t address;                               /* 寄存器地址（上报类型3使用） */
    HOOCH_PROTOCOL_KeyStatusState_t state;          /* 当前按键状态 */
    HOOCH_PROTOCOL_KeyStatusControlItem_t control_item; /* 本次控制项，支持只更新单一状态 */
    HOOCH_PROTOCOL_Source_t source;                       /* 消息来源 */
    uint8_t sequence;                               /* 更新序号 */
    uint8_t valid;                                  /* 当前数据是否有效 */
    uint8_t device_desc[24];                        /* 设备描述符 */
    int value;                                      /* 通用配置值(图标/使能位 1 byte) */
} HOOCH_PROTOCOL_KeyStatusFrame_t;

/* 按键状态下发回调函数类型 */
typedef void (*HOOCH_PROTOCOL_KeyStatusCallback_t)(
    const HOOCH_PROTOCOL_KeyStatusFrame_t *frame);

/* 按键状态上报回调函数类型 */
typedef void (*HOOCH_PROTOCOL_KeyStatusReportCallback_t)(
    const HOOCH_PROTOCOL_KeyStatusFrame_t *frame);

/* 初始化按键状态模块 */
void HOOCH_KeyStatus_Init(void);

/* 清空当前按键状态数据 */
void HOOCH_PROTOCOL_KeyStatus_Clear(void);

/* 获取当前缓存的按键状态数据帧 */
const HOOCH_PROTOCOL_KeyStatusFrame_t *HOOCH_PROTOCOL_KeyStatus_GetFrame(void);

/* 上报按键开关状态（仅通道+数值） */
HOOCH_PROTOCOL_KeyStatusResult_t HOOCH_PROTOCOL_KeyStatus_ReportState(
    HOOCH_PROTOCOL_KeyStatusKey_t key,
    HOOCH_PROTOCOL_KeyStatusState_t state);

/* 直接写入完整的按键状态上报帧 */
HOOCH_PROTOCOL_KeyStatusResult_t HOOCH_PROTOCOL_KeyStatus_SetFrame(
    const HOOCH_PROTOCOL_KeyStatusFrame_t *frame);

/* 注册按键状态上报回调函数 */
void HOOCH_PROTOCOL_KeyStatus_RegisterReportCallback(
    HOOCH_PROTOCOL_KeyStatusReportCallback_t callback);

/* 注销按键状态上报回调函数 */
void HOOCH_PROTOCOL_KeyStatus_UnregisterReportCallback(void);

/* 下发按键开关状态 */
HOOCH_PROTOCOL_KeyStatusResult_t HOOCH_PROTOCOL_KeyStatus_DispatchState(
    HOOCH_PROTOCOL_KeyStatusKey_t key,
    HOOCH_PROTOCOL_KeyStatusState_t state);

/* 写入完整的按键状态下发数据帧 */
HOOCH_PROTOCOL_KeyStatusResult_t HOOCH_PROTOCOL_KeyStatus_DispatchFrame(
    const HOOCH_PROTOCOL_KeyStatusFrame_t *frame);

/* 注册按键状态下发回调函数 */
void HOOCH_PROTOCOL_KeyStatus_RegisterCallback(
    HOOCH_PROTOCOL_KeyStatusCallback_t callback);

/* 注销按键状态下发回调函数 */
void HOOCH_PROTOCOL_KeyStatus_UnregisterCallback(void);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_KEY_STATUS_H */
