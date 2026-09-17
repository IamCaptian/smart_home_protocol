#ifndef HOOCH_FRESH_AIR_H
#define HOOCH_FRESH_AIR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include "hooch_protocol_common.h"
/* 新风接口返回结果 */
typedef enum
{
    HOOCH_PROTOCOL_FRESH_AIR_RESULT_OK = 0U,          /* 执行成功 */
    HOOCH_PROTOCOL_FRESH_AIR_RESULT_ERROR,            /* 通用错误 */
    HOOCH_PROTOCOL_FRESH_AIR_RESULT_INVALID_PARAM,    /* 参数非法 */
} HOOCH_PROTOCOL_FreshAirResult_t;

/* 新风开关状态 */
typedef enum
{
    HOOCH_PROTOCOL_FRESH_AIR_POWER_OFF = 0U,    /* 关闭 */
    HOOCH_PROTOCOL_FRESH_AIR_POWER_ON,          /* 开启 */
    HOOCH_PROTOCOL_FRESH_AIR_POWER_INVALID = 0xFFU, /* 无效/保持当前值（仅内部填充层使用） */
} HOOCH_PROTOCOL_FreshAirPower_t;

/* 新风模式 */
typedef enum
{
    HOOCH_PROTOCOL_FRESH_AIR_MODE_INVALID = 0U,    /* 无效模式 */
    HOOCH_PROTOCOL_FRESH_AIR_MODE_AUTO = 1U,       /* 自动 */
    HOOCH_PROTOCOL_FRESH_AIR_MODE_MANUAL,          /* 手动 */
    HOOCH_PROTOCOL_FRESH_AIR_MODE_SLEEP,           /* 睡眠 */
    HOOCH_PROTOCOL_FRESH_AIR_MODE_BOOST,           /* 强劲 */
} HOOCH_PROTOCOL_FreshAirMode_t;

/* 新风风速 */
typedef enum
{
    HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID = 0U,    /* 无效风速 */
    HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_LOW = 1U,        /* 低风 */
    HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_MEDIUM,          /* 中风 */
    HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_HIGH,            /* 高风 */
    HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_AUTO,            /* 自动风 */
} HOOCH_PROTOCOL_FreshAirFanSpeed_t;

/* 新风控制项 */
typedef enum
{
    HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_INVALID = 0U,       /* 无效控制项 */
    HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL,                /* 全量控制 */
    HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_POWER,              /* 仅控制开关 */
    HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_MODE,               /* 仅控制模式 */
    HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED,          /* 仅控制风速 */
    HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_CURRENT_TEMPERATURE,/* 当前温度（实际室温） */
    HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ENABLE_BIT,         /* 仅更新使能位(1 byte 位图, bit0:使能 bit1:开关 bit2:模式 bit3:风速 bit4:当前温度) */
    HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_DEVICE_DESC,        /* 设备描述字符串 */
    HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_DEFAULT_ICON,       /* 默认图标 */
    HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_SELECTED_ICON,      /* 选中图标 */
    HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL_TUYA,           /* 全量控制涂鸦 */
    HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL_XIAOMI,         /* 全量控制小米 */
    HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL_KNX,            /* 全量控制KNX */
    HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_MAX,                /* 哨兵值，新增枚举请插入到上方 */
} HOOCH_PROTOCOL_FreshAirControlItem_t;

/* 新风数据帧描述 */
typedef struct
{
    uint8_t channel;                                         /* 新风通道 */
    uint16_t address;                                        /* 新风地址 */
    HOOCH_PROTOCOL_FreshAirPower_t power;                    /* 开关状态 */
    HOOCH_PROTOCOL_FreshAirMode_t mode;                      /* 模式 */
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed;             /* 风速 */
    uint16_t current_temperature;                            /* 当前温度（实际室温） */
    HOOCH_PROTOCOL_FreshAirControlItem_t control_item;       /* 本次控制项，支持只更新单一参数 */
    HOOCH_PROTOCOL_Source_t source;                           /* 消息来源 */
    uint8_t device_desc[24];                                 /* 设备描述符字符串 */
    uint8_t sequence;                                        /* 更新序号 */
    uint8_t valid;                                           /* 当前数据是否有效 */
    uint16_t value;                                          /* 参数值 */
} HOOCH_PROTOCOL_FreshAirFrame_t;

/* 新风回调函数类型 */
typedef void (*HOOCH_PROTOCOL_FreshAirCallback_t)(
    const HOOCH_PROTOCOL_FreshAirFrame_t *frame);

/* 初始化新风模块 */
void HOOCH_FreshAir_Init(void);

/* 清空当前新风数据 */
void HOOCH_PROTOCOL_FreshAir_Clear(void);

/* 校验新风开关状态是否有效 */
uint8_t HOOCH_PROTOCOL_FreshAir_IsValidPower(HOOCH_PROTOCOL_FreshAirPower_t power);

/* 校验新风模式是否有效 */
uint8_t HOOCH_PROTOCOL_FreshAir_IsValidMode(HOOCH_PROTOCOL_FreshAirMode_t mode);

/* 校验新风风速是否有效 */
uint8_t HOOCH_PROTOCOL_FreshAir_IsValidFanSpeed(HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed);

/* 校验新风控制项是否有效 */
uint8_t HOOCH_PROTOCOL_FreshAir_IsValidControlItem(
    HOOCH_PROTOCOL_FreshAirControlItem_t control_item);

/* 设置当前新风状态 */
HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_Set(
    HOOCH_PROTOCOL_FreshAirPower_t power,
    HOOCH_PROTOCOL_FreshAirMode_t mode,
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed);

/* 写入新风数据帧，可按 control_item 只更新单一参数 */
HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SetFrame(
    const HOOCH_PROTOCOL_FreshAirFrame_t *frame);

/* 获取当前缓存的新风数据帧 */
const HOOCH_PROTOCOL_FreshAirFrame_t *HOOCH_PROTOCOL_FreshAir_GetFrame(void);

/* 注册新风下发回调函数 */
void HOOCH_PROTOCOL_FreshAir_RegisterCallback(
    HOOCH_PROTOCOL_FreshAirCallback_t callback);

/* 注销新风下发回调函数 */
void HOOCH_PROTOCOL_FreshAir_UnregisterCallback(void);

/* 注册新风上报回调函数（通道版） */
void HOOCH_PROTOCOL_FreshAir_RegisterReportCallback(
    HOOCH_PROTOCOL_FreshAirCallback_t callback);

/* 注册新风上报回调函数（地址版） */
void HOOCH_PROTOCOL_FreshAir_RegisterReportCallback2(
    HOOCH_PROTOCOL_FreshAirCallback_t callback);

/* 注销新风上报回调函数（通道版） */
void HOOCH_PROTOCOL_FreshAir_UnregisterReportCallback(void);

/* 注销新风上报回调函数（地址版） */
void HOOCH_PROTOCOL_FreshAir_UnregisterReportCallback2(void);

/* 对外统一的新风入口 */
HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_Send(
    uint8_t channel,
    HOOCH_PROTOCOL_FreshAirPower_t power,
    HOOCH_PROTOCOL_FreshAirMode_t mode,
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed);

/* 单独上报新风开关（通道） */
HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SendPower(
    uint8_t channel,
    HOOCH_PROTOCOL_FreshAirPower_t power);

/* 单独上报新风模式（通道） */
HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SendMode(
    uint8_t channel,
    HOOCH_PROTOCOL_FreshAirMode_t mode);

/* 单独上报新风风速（通道） */
HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SendFanSpeed(
    uint8_t channel,
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed);

/* 单独上报新风开关（地址） */
HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SendPowerAddr(
    uint16_t address,
    HOOCH_PROTOCOL_FreshAirPower_t power);

/* 单独上报新风模式（地址） */
HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SendModeAddr(
    uint16_t address,
    HOOCH_PROTOCOL_FreshAirMode_t mode);

/* 单独上报新风风速（地址） */
HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SendFanSpeedAddr(
    uint16_t address,
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed);

/* 上报新风整帧（通道） */
HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SendFrame(
    uint8_t channel,
    HOOCH_PROTOCOL_FreshAirPower_t power,
    HOOCH_PROTOCOL_FreshAirMode_t mode,
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed);

/* 上报新风整帧（地址） */
HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_SendFrameAddr(
    uint16_t address,
    HOOCH_PROTOCOL_FreshAirPower_t power,
    HOOCH_PROTOCOL_FreshAirMode_t mode,
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed);

/* 写入新风下发数据帧 */
HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_DispatchFrame(
    const HOOCH_PROTOCOL_FreshAirFrame_t *frame);

/* 下发新风控制数据 单新风使用：小米、涂鸦 默认通道0 */
HOOCH_PROTOCOL_FreshAirResult_t HOOCH_PROTOCOL_FreshAir_Dispatch(
    HOOCH_PROTOCOL_FreshAirPower_t power,
    HOOCH_PROTOCOL_FreshAirMode_t mode,
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_FRESH_AIR_H */
