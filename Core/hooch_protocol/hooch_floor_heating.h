#ifndef HOOCH_FLOOR_HEATING_H
#define HOOCH_FLOOR_HEATING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include "hooch_protocol_common.h"
/* 地暖接口返回结果 */
typedef enum
{
    HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_OK = 0U,          /* 执行成功 */
    HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_ERROR,            /* 通用错误 */
    HOOCH_PROTOCOL_FLOOR_HEATING_RESULT_INVALID_PARAM,    /* 参数非法 */
} HOOCH_PROTOCOL_FloorHeatingResult_t;

/* 地暖开关状态 */
typedef enum
{
    HOOCH_PROTOCOL_FLOOR_HEATING_POWER_OFF = 0U,    /* 关闭 */
    HOOCH_PROTOCOL_FLOOR_HEATING_POWER_ON,          /* 开启 */
} HOOCH_PROTOCOL_FloorHeatingPower_t;

/* 地暖模式 */
typedef enum
{
    HOOCH_PROTOCOL_FLOOR_HEATING_MODE_INVALID = 0U,    /* 无效模式 */
    HOOCH_PROTOCOL_FLOOR_HEATING_MODE_MANUAL = 1U,     /* 手动 */
    HOOCH_PROTOCOL_FLOOR_HEATING_MODE_AUTO,            /* 自动 */
    HOOCH_PROTOCOL_FLOOR_HEATING_MODE_ECO,             /* 节能 */
} HOOCH_PROTOCOL_FloorHeatingMode_t;

/* 地暖控制项 */
typedef enum
{
    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_INVALID = 0U,         /* 无效控制项 */
    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL,                  /* 全量控制 */
    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_POWER,                /* 仅控制开关 */
    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_MODE,                 /* 仅控制模式 */
    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TARGET_TEMPERATURE,   /* 仅控制目标温度 */
    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_CURRENT_TEMPERATURE,  /* 仅控制当前温度 */
    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ENABLE_BIT,           /* 仅更新使能位(1 byte 位图, bit0:使能 bit1:开关 bit2:模式 bit3:目标温度 bit4:当前温度) */
    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_DEVICE_DESC,          /* 设备描述字符串 */
    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_DEFAULT_ICON,         /* 默认图标 */
    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_SELECTED_ICON,        /* 选中图标 */
    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TEMP_STEP,            /* 温度设置步进 */
    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TEMP_MIN,             /* 最小设置温度 (预留)*/
    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TEMP_MAX,             /* 最大设置温度(预留) */
    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL_TUYA,             /* 全量控制涂鸦 */
    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL_XIAOMI,           /* 全量控制小米 */
    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL_KNX,              /* 全量控制KNX */
    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_MAX,                  /* 哨兵值，新增枚举请插入到上方 */
} HOOCH_PROTOCOL_FloorHeatingControlItem_t;

/* 地暖数据帧描述 */
typedef struct
{
    uint8_t channel;                                              /* 地暖通道 */
    uint16_t address;                                             /* 地暖地址 */
    HOOCH_PROTOCOL_FloorHeatingPower_t power;                     /* 开关状态 */
    HOOCH_PROTOCOL_FloorHeatingMode_t mode;                       /* 模式 */
    uint16_t target_temperature;                                   /* 目标温度 */
    uint16_t current_temperature;                                  /* 当前温度 */
    HOOCH_PROTOCOL_FloorHeatingControlItem_t control_item;        /* 本次控制项，支持只更新单一参数 */
    HOOCH_PROTOCOL_Source_t source;                                /* 消息来源 */
    uint8_t device_desc[24];                                      /* 设备描述符字符串 */
    uint8_t sequence;                                             /* 更新序号 */
    uint8_t valid;                                                /* 当前数据是否有效 */
    uint16_t value;                                                /* 参数值 */
} HOOCH_PROTOCOL_FloorHeatingFrame_t;

/* 地暖回调函数类型 */
typedef void (*HOOCH_PROTOCOL_FloorHeatingCallback_t)(
    const HOOCH_PROTOCOL_FloorHeatingFrame_t *frame);

/* 初始化地暖模块 */
void HOOCH_FloorHeating_Init(void);

/* 清空当前地暖数据 */
void HOOCH_PROTOCOL_FloorHeating_Clear(void);

/* 校验地暖开关状态是否有效 */
uint8_t HOOCH_PROTOCOL_FloorHeating_IsValidPower(HOOCH_PROTOCOL_FloorHeatingPower_t power);

/* 校验地暖模式是否有效 */
uint8_t HOOCH_PROTOCOL_FloorHeating_IsValidMode(HOOCH_PROTOCOL_FloorHeatingMode_t mode);

/* 校验地暖控制项是否有效 */
uint8_t HOOCH_PROTOCOL_FloorHeating_IsValidControlItem(
    HOOCH_PROTOCOL_FloorHeatingControlItem_t control_item);

/* 设置当前地暖状态 */
HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_Set(
    HOOCH_PROTOCOL_FloorHeatingPower_t power,
    HOOCH_PROTOCOL_FloorHeatingMode_t mode,
    uint16_t target_temperature,
    uint16_t current_temperature);

/* 写入地暖数据帧，可按 control_item 只更新单一参数 */
HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SetFrame(
    const HOOCH_PROTOCOL_FloorHeatingFrame_t *frame);

/* 获取当前缓存的地暖数据帧 */
const HOOCH_PROTOCOL_FloorHeatingFrame_t *HOOCH_PROTOCOL_FloorHeating_GetFrame(void);

/* 注册地暖下发回调函数 */
void HOOCH_PROTOCOL_FloorHeating_RegisterCallback(
    HOOCH_PROTOCOL_FloorHeatingCallback_t callback);

/* 注销地暖下发回调函数 */
void HOOCH_PROTOCOL_FloorHeating_UnregisterCallback(void);

/* 注册地暖上报回调函数（通道版） */
void HOOCH_PROTOCOL_FloorHeating_RegisterReportCallback(
    HOOCH_PROTOCOL_FloorHeatingCallback_t callback);

/* 注册地暖上报回调函数（地址版） */
void HOOCH_PROTOCOL_FloorHeating_RegisterReportCallback2(
    HOOCH_PROTOCOL_FloorHeatingCallback_t callback);

/* 注销地暖上报回调函数（通道版） */
void HOOCH_PROTOCOL_FloorHeating_UnregisterReportCallback(void);

/* 注销地暖上报回调函数（地址版） */
void HOOCH_PROTOCOL_FloorHeating_UnregisterReportCallback2(void);

/* 对外统一的地暖入口 */
HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_Send(
    uint8_t channel,
    HOOCH_PROTOCOL_FloorHeatingPower_t power,
    HOOCH_PROTOCOL_FloorHeatingMode_t mode,
    uint16_t target_temperature,
    uint16_t current_temperature);

/* 单独上报地暖开关（通道） */
HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendPower(
    uint8_t channel,
    HOOCH_PROTOCOL_FloorHeatingPower_t power);

/* 单独上报地暖模式（通道） */
HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendMode(
    uint8_t channel,
    HOOCH_PROTOCOL_FloorHeatingMode_t mode);

/* 单独上报地暖目标温度（通道） */
HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendTargetTemperature(
    uint8_t channel,
    uint16_t target_temperature);

/* 单独上报地暖当前温度（通道）废弃 */
HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendCurrentTemperature(
    uint8_t channel,
    uint16_t current_temperature);

/* 单独上报地暖开关（地址） */
HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendPowerAddr(
    uint16_t address,
    HOOCH_PROTOCOL_FloorHeatingPower_t power);

/* 单独上报地暖模式（地址） */
HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendModeAddr(
    uint16_t address,
    HOOCH_PROTOCOL_FloorHeatingMode_t mode);

/* 单独上报地暖目标温度（地址） */
HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendTargetTemperatureAddr(
    uint16_t address,
    uint16_t target_temperature);

/* 单独上报地暖当前温度（地址） */
HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendCurrentTemperatureAddr(
    uint16_t address,
    uint16_t current_temperature);

/* 上报地暖整帧（通道） */
HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendFrame(
    uint8_t channel,
    HOOCH_PROTOCOL_FloorHeatingPower_t power,
    HOOCH_PROTOCOL_FloorHeatingMode_t mode,
    uint16_t target_temperature,
    uint16_t current_temperature);

/* 上报地暖整帧（地址） */
HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_SendFrameAddr(
    uint16_t address,
    HOOCH_PROTOCOL_FloorHeatingPower_t power,
    HOOCH_PROTOCOL_FloorHeatingMode_t mode,
    uint16_t target_temperature,
    uint16_t current_temperature);

/* 写入地暖下发数据帧 */
HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_DispatchFrame(
    const HOOCH_PROTOCOL_FloorHeatingFrame_t *frame);

/* 下发地暖控制数据 单地暖使用：小米、涂鸦 默认通道0 */
HOOCH_PROTOCOL_FloorHeatingResult_t HOOCH_PROTOCOL_FloorHeating_Dispatch(
    HOOCH_PROTOCOL_FloorHeatingPower_t power,
    HOOCH_PROTOCOL_FloorHeatingMode_t mode,
    uint16_t target_temperature,
    uint16_t current_temperature);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_FLOOR_HEATING_H */
