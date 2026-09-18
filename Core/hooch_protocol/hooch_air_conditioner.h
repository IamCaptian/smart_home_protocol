#ifndef HOOCH_AIR_CONDITIONER_H
#define HOOCH_AIR_CONDITIONER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include "hooch_protocol_common.h"

/* ========================== 类型定义 ========================== */

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

/* 空调类型 */
typedef enum
{
    HOOCH_PROTOCOL_AIR_CONDITIONER_TYPE_INVALID = 0U,               /* 无效类型 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_TYPE_WATER_UNIT = 1U,            /* 水机 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_TYPE_HF_WIRED_CONTROLLER,        /* HF线控 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_TYPE_HF_WIRED_CONTROLLER_485,    /* HF线控485 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_TYPE_HF_CENTRAL_CONTROL_485,     /* HF集控485 */
} HOOCH_PROTOCOL_AirConditionerType_t;

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
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ENABLE_BIT,         /* [7] 仅更新使能位(1 byte 位图, bit0:使能 bit1:开关 bit2:模式 bit3:风速 bit4:当前温度) */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_DEVICE_DESC,        /* [8] 设备描述字符串 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_DEFAULT_ICON,       /* [9] 默认图标 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_SELECTED_ICON,      /* [10] 选中图标 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMP_STEP,          /* [11] 温度设置步进 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMP_MIN,           /* [12] 最小设置温度 (预留)*/
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMP_MAX,           /* [13] 最大设置温度(预留) */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_TUYA,           /* [14] 全量控制涂鸦 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_XIAOMI,         /* [15] 全量控制小米 */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_KNX,            /* [16] 全量控制KNX */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_AC_TYPE,            /* [17] 空调类型（取值见 HOOCH_PROTOCOL_AirConditionerType_t，用 value 承接） */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_CENTRAL_CONTROL_ADDRESS, /* [18] 集控地址（用 value 承接） */
    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MAX,                /* [19] 哨兵值，新增枚举请插入到上方 */
} HOOCH_PROTOCOL_AirConditionerControlItem_t;

/* 空调数据帧描述（下发帧与上报帧共用同一结构，靠 channel/address/source 区分） */
typedef struct
{
    uint8_t channel;                                         /* 空调通道 */
    uint16_t address;                                        /* 空调地址 */
    HOOCH_PROTOCOL_AirConditionerPower_t power;              /* 开关状态 */
    HOOCH_PROTOCOL_AirConditionerMode_t mode;                /* 模式 */
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed;       /* 风速 */
    uint16_t temperature;                                    /* 设定温度 */
    uint16_t current_temperature;                            /* 当前温度（实际室温） */
    HOOCH_PROTOCOL_AirConditionerControlItem_t control_item; /* 本次控制项，支持只更新单一参数 */
    HOOCH_PROTOCOL_Source_t source;                          /* 消息来源 */
    uint8_t device_desc[24];                                 /* 设备描述符字符串 */
    uint8_t sequence;                                        /* 更新序号 */
    uint8_t valid;                                           /* 当前数据是否有效 */
    uint16_t value;                                          /* 参数值 */
} HOOCH_PROTOCOL_AirConditionerFrame_t;

/* 空调回调函数类型（下发回调与上报回调共用同一签名） */
typedef void (*HOOCH_PROTOCOL_AirConditionerCallback_t)(
    const HOOCH_PROTOCOL_AirConditionerFrame_t *frame);

/* ============== 公共接口（初始化 / 清空 / 参数校验） ============== */

/* 初始化空调模块 */
void HOOCH_AirConditioner_Init(void);

/* 清空当前空调数据（清空上报帧缓存） */
void HOOCH_PROTOCOL_AirConditioner_Clear(void);

/* 复位一帧空调数据（清零并置默认值），跨平台构造下发/上报帧前先调用 */
void HOOCH_PROTOCOL_AirConditioner_ClearFrame(HOOCH_PROTOCOL_AirConditionerFrame_t *frame);

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

/* ==================== 下发（平台 -> MCU） ==================== */

/* ---- 下发回调注册 ---- */

/* 注册空调下发回调函数（DispatchFrame 写入成功后回调） */
void HOOCH_PROTOCOL_AirConditioner_RegisterCallback(
    HOOCH_PROTOCOL_AirConditionerCallback_t callback);

/* 注销空调下发回调函数 */
void HOOCH_PROTOCOL_AirConditioner_UnregisterCallback(void);

/* ---- 下发入口 ---- */

/* 写入空调下发数据帧（跨平台唯一入口，调用方需在 frame->source 标明来源）
 * 支持的控制项：ALL / POWER / MODE / FAN_SPEED / TEMPERATURE / CURRENT_TEMPERATURE /
 * ENABLE_BIT / DEVICE_DESC / DEFAULT_ICON / SELECTED_ICON / TEMP_STEP / TEMP_MIN / TEMP_MAX /
 * AC_TYPE / CENTRAL_CONTROL_ADDRESS */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_DispatchFrame(
    const HOOCH_PROTOCOL_AirConditionerFrame_t *frame);

/* ==================== 上报（MCU -> 平台） ==================== */

/* 三路上报接口与回调一一对应，投递互斥（同一帧只走一路，不会串到别的回调）：
 *   Send*                通道版 -> RegisterReportCallback
 *   Send*Addr            地址版 -> RegisterReportCallback2（address 作为标识/GID）
 *   Send*AddrHfWired485  线控网关 -> RegisterReportCallbackHfWired485
 */

/* ---- 通道版上报 ---- */

/* 注册空调上报回调函数（通道版：写上报帧后触发） */
void HOOCH_PROTOCOL_AirConditioner_RegisterReportCallback(
    HOOCH_PROTOCOL_AirConditionerCallback_t callback);

/* 注销空调上报回调函数（通道版） */
void HOOCH_PROTOCOL_AirConditioner_UnregisterReportCallback(void);

/* 对外统一的空调入口：全量上报，通道由 channel 指定（control_item = ALL） */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_Send(
    uint8_t channel,
    HOOCH_PROTOCOL_AirConditionerPower_t power,
    HOOCH_PROTOCOL_AirConditionerMode_t mode,
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed,
    uint16_t temperature);

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
    uint16_t temperature);

/* ---- 地址版上报 ---- */

/* 注册空调上报回调函数（地址版：frame->address 非 0 时触发，用于组播/地址上报） */
void HOOCH_PROTOCOL_AirConditioner_RegisterReportCallback2(
    HOOCH_PROTOCOL_AirConditionerCallback_t callback);

/* 注销空调上报回调函数（地址版） */
void HOOCH_PROTOCOL_AirConditioner_UnregisterReportCallback2(void);

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
    uint16_t temperature);

/* ---- HF线控485 网关上报（地址版，专属回调） ---- */

/* 注册 HF线控485 网关上报回调（仅承接下面 4 个接口发起的上报） */
void HOOCH_PROTOCOL_AirConditioner_RegisterReportCallbackHfWired485(
    HOOCH_PROTOCOL_AirConditionerCallback_t callback);

/* 注销 HF线控485 网关上报回调 */
void HOOCH_PROTOCOL_AirConditioner_UnregisterReportCallbackHfWired485(void);

/* HF线控485 网关单独上报空调开关（地址） */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendPowerAddrHfWired485(
    uint16_t address,
    HOOCH_PROTOCOL_AirConditionerPower_t power);

/* HF线控485 网关单独上报空调模式（地址） */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendModeAddrHfWired485(
    uint16_t address,
    HOOCH_PROTOCOL_AirConditionerMode_t mode);

/* HF线控485 网关单独上报空调风速（地址） */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendFanSpeedAddrHfWired485(
    uint16_t address,
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed);

/* HF线控485 网关单独上报空调温度（地址） */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SendTemperatureAddrHfWired485(
    uint16_t address,
    uint16_t temperature);

/* ---- 上报帧写入与查询（历史接口，写入/读取的都是上报帧缓存） ---- */








/* 写入空调上报数据帧，可按 control_item 只更新单一参数
 * 支持的控制项：ALL / ALL_KNX / ALL_XIAOMI（全量），POWER / MODE / FAN_SPEED / TEMPERATURE（单项）；
 * 其余控制项仅下发行支持，此处会返回 INVALID_PARAM */
HOOCH_PROTOCOL_AirConditionerResult_t HOOCH_PROTOCOL_AirConditioner_SetFrame(
    const HOOCH_PROTOCOL_AirConditionerFrame_t *frame);



#ifdef __cplusplus
}
#endif

#endif /* HOOCH_AIR_CONDITIONER_H */
