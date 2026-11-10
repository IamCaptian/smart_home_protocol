#ifndef HOOCH_RELAY_H
#define HOOCH_RELAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include "hooch_protocol_common.h"

/* 支持的继电器路数（通道 1-8）。 */
#define HOOCH_PROTOCOL_RELAY_CHANNEL_COUNT    8U

/* 继电器接口返回结果 */
typedef enum
{
    HOOCH_PROTOCOL_RELAY_RESULT_OK = 0U,          /* 执行成功 */
    HOOCH_PROTOCOL_RELAY_RESULT_ERROR,            /* 通用错误 */
    HOOCH_PROTOCOL_RELAY_RESULT_INVALID_PARAM,    /* 参数非法 */
} HOOCH_PROTOCOL_RelayResult_t;

/* 继电器开关状态 */
typedef enum
{
    HOOCH_PROTOCOL_RELAY_SWITCH_OFF = 0U,    /* 断开 */
    HOOCH_PROTOCOL_RELAY_SWITCH_ON,          /* 吸合 */
} HOOCH_PROTOCOL_RelaySwitchState_t;

/* 继电器通道枚举，有效范围 1-8 */
typedef enum
{
    HOOCH_PROTOCOL_RELAY_CHANNEL_INVALID = 0U,    /* 无效通道 */
    HOOCH_PROTOCOL_RELAY_CHANNEL_1 = 1U,          /* 继电器1 */
    HOOCH_PROTOCOL_RELAY_CHANNEL_2,
    HOOCH_PROTOCOL_RELAY_CHANNEL_3,
    HOOCH_PROTOCOL_RELAY_CHANNEL_4,
    HOOCH_PROTOCOL_RELAY_CHANNEL_5,
    HOOCH_PROTOCOL_RELAY_CHANNEL_6,
    HOOCH_PROTOCOL_RELAY_CHANNEL_7,
    HOOCH_PROTOCOL_RELAY_CHANNEL_8,
} HOOCH_PROTOCOL_RelayChannel_t;

/* 继电器控制项 */
typedef enum
{
    HOOCH_PROTOCOL_RELAY_CONTROL_ITEM_INVALID = 0U,          /* 无效控制项 */
    HOOCH_PROTOCOL_RELAY_CONTROL_ITEM_ALL,                   /* 全量控制 */
    HOOCH_PROTOCOL_RELAY_CONTROL_ITEM_SWITCH,                /* 仅控制开关 */
    HOOCH_PROTOCOL_RELAY_CONTROL_ITEM_ENABLE_BIT,            /* 仅更新使能位(1 byte 位图, bit0:使能 bit1:开关) */
    HOOCH_PROTOCOL_RELAY_CONTROL_ITEM_DEVICE_DESC,           /* 设备描述字符串 */
    HOOCH_PROTOCOL_RELAY_CONTROL_ITEM_DEFAULT_ICON,          /* 默认图标 */
    HOOCH_PROTOCOL_RELAY_CONTROL_ITEM_SELECTED_ICON,         /* 选中图标 */
    HOOCH_PROTOCOL_RELAY_CONTROL_ITEM_MAX,                   /* 哨兵值，新增枚举请插入到上方 */
} HOOCH_PROTOCOL_RelayControlItem_t;

/* 继电器数据帧描述 */
typedef struct
{
    HOOCH_PROTOCOL_RelayChannel_t channel;                  /* 继电器通道(1-8) */
    HOOCH_PROTOCOL_RelaySwitchState_t switch_state;         /* 开关状态 */
    HOOCH_PROTOCOL_RelayControlItem_t control_item;         /* 本次控制项，支持只更新单一参数 */
    HOOCH_PROTOCOL_Source_t source;                         /* 消息来源 */
    uint8_t device_desc[24];                                /* 设备描述符字符串 */
    uint8_t sequence;                                       /* 更新序号 */
    uint8_t valid;                                          /* 当前数据是否有效 */
    uint16_t value;                                         /* 参数值(使能位/图标等) */
} HOOCH_PROTOCOL_RelayFrame_t;

/* 继电器接收回调函数类型 */
typedef void (*HOOCH_PROTOCOL_RelayCallback_t)(
    const HOOCH_PROTOCOL_RelayFrame_t *frame);

/* 初始化继电器模块 */
void HOOCH_Relay_Init(void);

/* 清空当前继电器数据 */
void HOOCH_PROTOCOL_Relay_Clear(void);

/* 校验继电器通道是否有效(1-8) */
uint8_t HOOCH_PROTOCOL_Relay_IsValidChannel(
    HOOCH_PROTOCOL_RelayChannel_t channel);

/* 校验继电器开关状态是否有效 */
uint8_t HOOCH_PROTOCOL_Relay_IsValidSwitchState(
    HOOCH_PROTOCOL_RelaySwitchState_t switch_state);

/* 获取当前缓存的继电器数据帧 */
const HOOCH_PROTOCOL_RelayFrame_t *HOOCH_PROTOCOL_Relay_GetFrame(void);

/* 注册继电器接收回调函数 */
void HOOCH_PROTOCOL_Relay_RegisterCallback(
    HOOCH_PROTOCOL_RelayCallback_t callback);

/* 注销继电器接收回调函数 */
void HOOCH_PROTOCOL_Relay_UnregisterCallback(void);

/* 写入继电器接收数据帧，可按 control_item 只更新单一参数 */
HOOCH_PROTOCOL_RelayResult_t HOOCH_PROTOCOL_Relay_DispatchFrame(
    const HOOCH_PROTOCOL_RelayFrame_t *frame);

/* 接收继电器开关控制 */
HOOCH_PROTOCOL_RelayResult_t HOOCH_PROTOCOL_Relay_DispatchSwitch(
    HOOCH_PROTOCOL_RelayChannel_t channel,
    HOOCH_PROTOCOL_RelaySwitchState_t switch_state);

/* ---------------- 上报接口（上报功能暂未实现，仅保留接口） ---------------- */

/* 注册继电器上报回调函数 */
void HOOCH_PROTOCOL_Relay_RegisterReportCallback(
    HOOCH_PROTOCOL_RelayCallback_t callback);

/* 注销继电器上报回调函数 */
void HOOCH_PROTOCOL_Relay_UnregisterReportCallback(void);

/* 上报继电器开关（暂未实现） */
HOOCH_PROTOCOL_RelayResult_t HOOCH_PROTOCOL_Relay_SendSwitch(
    HOOCH_PROTOCOL_RelayChannel_t channel,
    HOOCH_PROTOCOL_RelaySwitchState_t switch_state);

/* 上报继电器整帧（暂未实现） */
HOOCH_PROTOCOL_RelayResult_t HOOCH_PROTOCOL_Relay_SendFrame(
    const HOOCH_PROTOCOL_RelayFrame_t *frame);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_RELAY_H */
