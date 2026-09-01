#ifndef HOOCH_AIR_CONDITIONER_H
#define HOOCH_AIR_CONDITIONER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
/* 空调接口返回结果 */
typedef enum
{
    HOOCH_PROTOCOL_AIR_CONDITIONER_RESULT_OK = 0U,          /* 执行成功 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_RESULT_ERROR,            /* 通用错误 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_RESULT_INVALID_PARAM,    /* 参数非法 */
} HOOCH_PROTOCOL_AirConditionerResult_t;

/* 空调开关状态 */
typedef enum
{
    HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_OFF = 0U,    /* 关闭 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_ON,          /* 开启 */
} HOOCH_PROTOCOL_AirConditionerPower_t;

/* 空调模式 */
typedef enum
{
    HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_INVALID = 0U,    /* 无效模式 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_COOL = 1U,       /* 制冷 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_FAN,             /* 送风 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_DRY,             /* 除湿 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_HEAT,            /* 制热 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_AUTO,            /* 自动 */
} HOOCH_PROTOCOL_AirConditionerMode_t;

/* 空调风速 */
typedef enum
{
    HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_INVALID = 0U,    /* 无效风速 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_LOW = 1U,        /* 低风 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_MEDIUM,          /* 中风 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_HIGH,            /* 高风 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_AUTO,            /* 自动风 */
} HOOCH_PROTOCOL_AirConditionerFanSpeed_t;

/* 空调控制项 */
typedef enum
{
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_INVALID,            /* [0] 无效控制项 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL,                /* [1] 全量控制 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_POWER,              /* [2] 仅控制开关 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MODE,               /* [3] 仅控制模式 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_FAN_SPEED,          /* [4] 仅控制风速 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMPERATURE,        /* [5] 仅控制温度 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_CURRENT_TEMPERATURE,/* [6] 当前温度（实际室温） */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ENABLE,             /* [7] 空调使能 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_DEVICE_DESC,        /* [8] 设备描述字符串 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_DEFAULT_ICON,       /* [9] 默认图标 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_SELECTED_ICON,      /* [10] 选中图标 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMP_STEP,          /* [11] 温度设置步进 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMP_MIN,           /* [12] 最小设置温度 (预留)*/
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMP_MAX,           /* [13] 最大设置温度(预留) */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_TUYA,           /* [14] 全量控制涂鸦 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_XIAOMI,         /* [15] 全量控制小米 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_KNX,            /* [16] 全量控制KNX */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MAX,                /* [17] 哨兵值，新增枚举请插入到上方 */
} HOOCH_PROTOCOL_AirConditionerControlItem_t;

/* 空调数据帧描述 */
typedef struct
{
    uint8_t channel;                                         /* 空调通道 */
    uint16_t address;                                        /* 空调地址 */
    HOOCH_PROTOCOL_AirConditionerPower_t power;              /* 开关状态 */
    HOOCH_PROTOCOL_AirConditionerMode_t mode;                /* 模式 */
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed;       /* 风速 */
    uint8_t temperature;                                     /* 设定温度 */
    uint8_t current_temperature;                             /* 当前温度（实际室温） */
    HOOCH_PROTOCOL_AirConditionerControlItem_t control_item; /* 本次控制项，支持只更新单一参数 */
    uint8_t device_desc[24];                                 /* 设备描述符字符串 */
    uint8_t sequence;                                        /* 更新序号 */
    uint8_t valid;                                           /* 当前数据是否有效 */
    uint8_t value;                                           /* 参数值 */
} HOOCH_PROTOCOL_AirConditionerFrame_t;



/* 空调回调函数类型 */
typedef void (*HOOCH_PROTOCOL_AirConditionerCallback_t)(
    const HOOCH_PROTOCOL_AirConditionerFrame_t *frame);

/* 初始化空调模块 */
void HOOCH_AirConditioner_Init(void);

/* 清空当前空调数据 */
void HOOCH_PROTOCOL_AirConditioner_Clear(void);

/* 校验空调开关状态是否有效 */
uint8_t HOOCH_PROTOCOL_AirConditioner_IsValidPower(HOOCH_PROTOCOL_AirConditionerPower_t power);

/* 校验空调模式是否有效 */
uint8_t HOOCH_PROTOCOL_AirConditioner_IsValidMode(HOOCH_PROTOCOL_AirConditionerMode_t mode);

/* 校验空调风速是否有效 */
uint8_t HOOCH_PROTOCOL_AirConditioner_IsValidFanSpeed(
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed);

/* 校验空调控制项是否有效 */
uint8_t HOOCH_PROTOCOL_AirConditioner_IsValidControlItem(
    HOOCH_PROTOCOL_AirConditionerControlItem_t control_item);

/* 设置当前空调状态 */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_Set(
    HOOCH_PROTOCOL_AirConditionerPower_t power,
    HOOCH_PROTOCOL_AirConditionerMode_t mode,
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed,
    uint8_t temperature);

/* 写入空调数据帧，可按 control_item 只更新单一参数 */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SetFrame(
    const HOOCH_PROTOCOL_AirConditionerFrame_t *frame);

/* 获取当前缓存的空调数据帧 */
const HOOCH_PROTOCOL_AirConditionerFrame_t *HOOCH_PROTOCOL_AirConditioner_GetFrame(void);

/* 注册空调下发回调函数 */
void HOOCH_PROTOCOL_AirConditioner_RegisterCallback(
    HOOCH_PROTOCOL_AirConditionerCallback_t callback);

/* 注销空调下发回调函数 */
void HOOCH_PROTOCOL_AirConditioner_UnregisterCallback(void);

/* 注册空调上报回调函数（通道版） */
void HOOCH_PROTOCOL_AirConditioner_RegisterReportCallback(
    HOOCH_PROTOCOL_AirConditionerCallback_t callback);

/* 注册空调上报回调函数（地址版） */
void HOOCH_PROTOCOL_AirConditioner_RegisterReportCallback2(
    HOOCH_PROTOCOL_AirConditionerCallback_t callback);

/* 注销空调上报回调函数（通道版） */
void HOOCH_PROTOCOL_AirConditioner_UnregisterReportCallback(void);

/* 注销空调上报回调函数（地址版） */
void HOOCH_PROTOCOL_AirConditioner_UnregisterReportCallback2(void);

/* 对外统一的空调入口 */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_Send(
    uint8_t channel,
    HOOCH_PROTOCOL_AirConditionerPower_t power,
    HOOCH_PROTOCOL_AirConditionerMode_t mode,
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed,
    uint8_t temperature);

/* 单独上报空调开关（通道） */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendPower(
    uint8_t channel,
    HOOCH_PROTOCOL_AirConditionerPower_t power);

/* 单独上报空调模式（通道） */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendMode(
    uint8_t channel,
    HOOCH_PROTOCOL_AirConditionerMode_t mode);

/* 单独上报空调风速（通道） */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendFanSpeed(
    uint8_t channel,
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed);

/* 单独上报空调温度（通道） */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendTemperature(
    uint8_t channel,
    uint8_t temperature);

/* 单独上报空调开关（地址） */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendPowerAddr(
    uint16_t address,
    HOOCH_PROTOCOL_AirConditionerPower_t power);

/* 单独上报空调模式（地址） */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendModeAddr(
    uint16_t address,
    HOOCH_PROTOCOL_AirConditionerMode_t mode);

/* 单独上报空调风速（地址） */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendFanSpeedAddr(
    uint16_t address,
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed);

/* 单独上报空调温度（地址） */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendTemperatureAddr(
    uint16_t address,
    uint8_t temperature);

/* 上报空调整帧（通道） */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendFrame(
    uint8_t channel,
    HOOCH_PROTOCOL_AirConditionerPower_t power,
    HOOCH_PROTOCOL_AirConditionerMode_t mode,
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed,
    uint8_t temperature);

/* 上报空调整帧（地址） */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendFrameAddr(
    uint16_t address,
    HOOCH_PROTOCOL_AirConditionerPower_t power,
    HOOCH_PROTOCOL_AirConditionerMode_t mode,
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed,
    uint8_t temperature);

/* 写入空调下发数据帧 */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_DispatchFrame(
    const HOOCH_PROTOCOL_AirConditionerFrame_t *frame);

/* 下发空调控制数据 单空调使用：小米、涂鸦 默认通道0*/
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_Dispatch(
    HOOCH_PROTOCOL_AirConditionerPower_t power,
    HOOCH_PROTOCOL_AirConditionerMode_t mode,
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed,
    uint8_t temperature);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_AIR_CONDITIONER_H */
