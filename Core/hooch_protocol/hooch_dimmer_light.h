#ifndef HOOCH_DIMMER_LIGHT_H
#define HOOCH_DIMMER_LIGHT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
/* 支持的调光灯路数。 */
#define HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_COUNT    8U

/* 调光灯接口返回结果 */
typedef enum
{
    HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_OK = 0U,          /* 执行成功 */
    HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_ERROR,            /* 通用错误 */
    HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_INVALID_PARAM,    /* 参数非法 */
} HOOCH_PROTOCOL_DimmerLightResult_t;

/* 调光灯编号枚举。 */
typedef enum
{
    HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_INVALID = 0U, /* 无效调光灯 */
    HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_1 = 1U,       /* 调光灯1 */
    HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_2,
    HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_3,
    HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_4,
    HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_5,
    HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_6,
    HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_7,
    HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_8,
} HOOCH_PROTOCOL_DimmerLightKey_t;

typedef enum
{
    HOOCH_PROTOCOL_DIMMER_LIGHT_SWITCH_OFF = 0U,
    HOOCH_PROTOCOL_DIMMER_LIGHT_SWITCH_ON = 1U,
} HOOCH_PROTOCOL_DimmerLightSwitchState_t;

/* 调光灯控制项 */
typedef enum
{
    HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_INVALID = 0U,    /* 无效控制项 */
    HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_ALL,             /* 全量更新 */
    HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_BRIGHTNESS,      /* 仅更新亮度 */
    HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_COLOR_TEMP,      /* 仅更新色温 */
    HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_SWITCH,          /* 仅更新开关 */
    HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_ENABLE_BIT,      /* 仅更新使能位(1 byte 位图, bit0:使能 bit1:开关 bit2:亮度 bit3:色温 bit4:RGBW) */
    HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_DEVICE_DESCRIPTOR, /* 仅更新设备描述字符串(24 byte, UTF-8) */
    HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_DEFAULT_ICON,      /* 仅更新默认图标(1 byte, 0-255) */
    HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_SELECTED_ICON,     /* 仅更新选中图标(1 byte, 0-255) */
    HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_COLOR_TEMP_STEP,   /* 仅更新色温步进(1 byte: 0:50k 1:100k 2:200k 3:300k 4:500k) */
    HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_MIN_COLOR_TEMP,    /* 仅更新最小色温(2 byte, 0-65535k) */
    HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_MAX_COLOR_TEMP,    /* 仅更新最大色温(2 byte, 0-65535k) */
} HOOCH_PROTOCOL_DimmerLightControlItem_t;

/* 调光灯数据帧描述 */
typedef struct
{
    HOOCH_PROTOCOL_DimmerLightKey_t key;                     /* 调光灯编号 */
    uint8_t page;                                          /* 页面编号（上报类型2使用） */
    uint16_t address;                                      /* 寄存器地址（上报类型3使用） */
    uint8_t brightness;                                    /* 当前亮度值 */
    uint8_t color_temperature;                             /* 当前色温值 */
    uint8_t switch_state;                                  /* 当前开关状态 */
    HOOCH_PROTOCOL_DimmerLightControlItem_t control_item;  /* 本次控制项 */
    uint8_t sequence;                                      /* 更新序号 */
    uint8_t valid;                                         /* 当前数据是否有效 */
    uint8_t device_desc[24];                           /* 设备描述符 */
    int value;                            /* 通用配置值(图标/步进/使能位 1 byte；最小/最大色温 2 byte) */
} HOOCH_PROTOCOL_DimmerLightFrame_t;

/* 调光灯下发回调函数类型 */
typedef void (*HOOCH_PROTOCOL_DimmerLightCallback_t)(
    const HOOCH_PROTOCOL_DimmerLightFrame_t *frame);

/* 调光灯上报回调函数类型1：仅通道+数值 */
typedef void (*HOOCH_PROTOCOL_DimmerLightReportCallback1_t)(
    const HOOCH_PROTOCOL_DimmerLightFrame_t *frame);

/* 调光灯上报回调函数类型2：页面+通道+数值 */
typedef void (*HOOCH_PROTOCOL_DimmerLightReportCallback2_t)(
    const HOOCH_PROTOCOL_DimmerLightFrame_t *frame);

/* 调光灯上报回调函数类型3：地址+数值 */
typedef void (*HOOCH_PROTOCOL_DimmerLightReportCallback3_t)(
    const HOOCH_PROTOCOL_DimmerLightFrame_t *frame);

/* 初始化调光灯模块 */
void HOOCH_DimmerLight_Init(void);

/* 清空当前调光灯数据 */
void HOOCH_PROTOCOL_DimmerLight_Clear(void);

/* 设置当前调光灯值 */
HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SetValue(uint8_t value);

/* 直接写入完整的调光灯数据帧 */
HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SetFrame(
    const HOOCH_PROTOCOL_DimmerLightFrame_t *frame);

/* 获取当前缓存的调光灯数据帧 */
const HOOCH_PROTOCOL_DimmerLightFrame_t *HOOCH_PROTOCOL_DimmerLight_GetFrame(void);

/* 注册调光灯下发回调函数 */
void HOOCH_PROTOCOL_DimmerLight_RegisterCallback(
    HOOCH_PROTOCOL_DimmerLightCallback_t callback);

/* 注销调光灯下发回调函数 */
void HOOCH_PROTOCOL_DimmerLight_UnregisterCallback(void);

/* 注册调光灯上报回调函数（类型1：仅通道+数值） */
void HOOCH_PROTOCOL_DimmerLight_RegisterReportCallback(
    HOOCH_PROTOCOL_DimmerLightReportCallback1_t callback);

/* 注销调光灯上报回调函数（类型1） */
void HOOCH_PROTOCOL_DimmerLight_UnregisterReportCallback(void);

/* 注册调光灯上报回调函数（类型2：页面+通道+数值） */
void HOOCH_PROTOCOL_DimmerLight_RegisterReportCallback2(
    HOOCH_PROTOCOL_DimmerLightReportCallback2_t callback);

/* 注销调光灯上报回调函数（类型2） */
void HOOCH_PROTOCOL_DimmerLight_UnregisterReportCallback2(void);

/* 注册调光灯上报回调函数（类型3：地址+数值） */
void HOOCH_PROTOCOL_DimmerLight_RegisterReportCallback3(
    HOOCH_PROTOCOL_DimmerLightReportCallback3_t callback);

/* 注销调光灯上报回调函数（类型3） */
void HOOCH_PROTOCOL_DimmerLight_UnregisterReportCallback3(void);

/* 对外统一的调光灯入口（无指定通道，兼容旧接口） */
HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_Send(uint8_t value);

/* 上报调光灯开关状态（类型1：仅通道+数值） */
HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendSwitch(
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    HOOCH_PROTOCOL_DimmerLightSwitchState_t state);

/* 上报调光灯亮度值（类型1：仅通道+数值） */
HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendBrightness(
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    uint8_t brightness);

/* 上报调光灯色温值（类型1：仅通道+数值） */
HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendColorTemperature(
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    uint8_t color_temperature);

/* ---- 类型2 上报：页面+通道+数值 ---- */

/* 上报调光灯开关状态（类型2：页面+通道） */
HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendSwitchPage(
    uint8_t page,
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    HOOCH_PROTOCOL_DimmerLightSwitchState_t state);

/* 上报调光灯亮度值（类型2：页面+通道） */
HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendBrightnessPage(
    uint8_t page,
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    uint8_t brightness);

/* 上报调光灯色温值（类型2：页面+通道） */
HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendColorTemperaturePage(
    uint8_t page,
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    uint8_t color_temperature);

/* ---- 类型3 上报：地址+数值 ---- */

/* 上报调光灯开关状态（类型3：地址） */
HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendSwitchAddr(
    uint16_t address,
    HOOCH_PROTOCOL_DimmerLightSwitchState_t state);

/* 上报调光灯亮度值（类型3：地址） */
HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendBrightnessAddr(
    uint16_t address,
    uint8_t brightness);

/* 上报调光灯色温值（类型3：地址） */
HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_SendColorTemperatureAddr(
    uint16_t address,
    uint8_t color_temperature);

/* 下发调光灯开关控制 */
HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_DispatchSwitch(
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    HOOCH_PROTOCOL_DimmerLightSwitchState_t state);

/* 下发调光灯亮度控制 */
HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_DispatchBrightness(
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    uint8_t brightness);

/* 下发调光灯色温控制 */
HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_DispatchColorTemperature(
    HOOCH_PROTOCOL_DimmerLightKey_t key,
    uint8_t color_temperature);

/* 下发完整调光灯数据帧（全量更新） */
HOOCH_PROTOCOL_DimmerLightResult_t HOOCH_PROTOCOL_DimmerLight_DispatchFrame(
    const HOOCH_PROTOCOL_DimmerLightFrame_t *frame);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_DIMMER_LIGHT_H */
