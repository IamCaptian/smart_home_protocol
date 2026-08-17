#ifndef HOOCH_CURTAIN_H
#define HOOCH_CURTAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
/* 支持的窗帘路数。 */
#define HOOCH_PROTOCOL_CURTAIN_KEY_COUNT    8U

/* 窗帘接口返回结果 */
typedef enum
{
    HOOCH_PROTOCOL_CURTAIN_RESULT_OK = 0U,          /* 执行成功 */
    HOOCH_PROTOCOL_CURTAIN_RESULT_ERROR,            /* 通用错误 */
    HOOCH_PROTOCOL_CURTAIN_RESULT_INVALID_PARAM,    /* 参数非法 */
} HOOCH_PROTOCOL_CurtainResult_t;

/* 窗帘编号枚举。 */
typedef enum
{
    HOOCH_PROTOCOL_CURTAIN_KEY_INVALID = 0U, /* 无效窗帘 */
    HOOCH_PROTOCOL_CURTAIN_KEY_1 = 1U,       /* 窗帘1 */
    HOOCH_PROTOCOL_CURTAIN_KEY_2,
    HOOCH_PROTOCOL_CURTAIN_KEY_3,
    HOOCH_PROTOCOL_CURTAIN_KEY_4,
    HOOCH_PROTOCOL_CURTAIN_KEY_5,
    HOOCH_PROTOCOL_CURTAIN_KEY_6,
    HOOCH_PROTOCOL_CURTAIN_KEY_7,
    HOOCH_PROTOCOL_CURTAIN_KEY_8,
} HOOCH_PROTOCOL_CurtainKey_t;

/* 窗帘开关状态枚举 */
typedef enum
{
    HOOCH_PROTOCOL_CURTAIN_SWITCH_OFF = 0U,
    HOOCH_PROTOCOL_CURTAIN_SWITCH_ON  = 1U,
} HOOCH_PROTOCOL_CurtainSwitchState_t;

/* 窗帘单项数据描述 */
typedef struct
{
    uint8_t value;      /* 当前值 */
    uint8_t sequence;   /* 更新序号 */
    uint8_t valid;      /* 当前数据是否有效 */
} HOOCH_PROTOCOL_CurtainItem_t;

/* 窗帘控制项 */
typedef enum
{
    HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_INVALID = 0U, /* 无效控制项 */
    HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ALL,          /* 全量更新 */
    HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_SWITCH,       /* 仅更新开关状态 */
    HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_STOP,         /* 仅更新停止状态 */
    HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_PERCENT,      /* 仅更新开合百分比 */
    HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ANGLE,        /* 仅更新开合角度 */
    HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_DEVICE_DESCRIPTOR, /* 仅更新设备描述符 */
    HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_DEFAULT_ICON,      /* 仅更新默认图标 */
    HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_SELECTED_ICON,     /* 仅更新选中图标 */
    HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_CURTAIN_TYPE,      /* 仅更新窗帘类型 */
    HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_MAX,                /* 哨兵值，新增枚举请插入到上方 */
} HOOCH_PROTOCOL_CurtainControlItem_t;

/* 窗帘数据帧描述 */
typedef struct
{
    HOOCH_PROTOCOL_CurtainKey_t key;                     /* 窗帘编号 */
    uint8_t page;                                      /* 页面编号（上报类型2使用） */
    uint16_t address;                                  /* 寄存器地址（上报类型3使用） */
    HOOCH_PROTOCOL_CurtainItem_t switch_status;        /* 开关状态 */
    HOOCH_PROTOCOL_CurtainItem_t stop;                 /* 停止控制 */
    HOOCH_PROTOCOL_CurtainItem_t percent;              /* 开合百分比 */
    HOOCH_PROTOCOL_CurtainItem_t angle;                /* 开合角度 */
    HOOCH_PROTOCOL_CurtainControlItem_t control_item;  /* 本次控制项，支持只更新单一状态 */
    uint8_t sequence;                                  /* 更新序号 */
    uint8_t valid;                                     /* 当前数据是否有效 */
    uint8_t device_desc[24];                           /* 设备描述符 */
    uint8_t value;                              /* 当前值 */
} HOOCH_PROTOCOL_CurtainFrame_t;

/* 窗帘下发回调函数类型 */
typedef void (*HOOCH_PROTOCOL_CurtainCallback_t)(
    const HOOCH_PROTOCOL_CurtainFrame_t *frame);

/* 窗帘上报回调函数类型1：仅通道+数值 */
typedef void (*HOOCH_PROTOCOL_CurtainReportCallback1_t)(
    const HOOCH_PROTOCOL_CurtainFrame_t *frame);

/* 窗帘上报回调函数类型2：页面+通道+数值 */
typedef void (*HOOCH_PROTOCOL_CurtainReportCallback2_t)(
    const HOOCH_PROTOCOL_CurtainFrame_t *frame);

/* 窗帘上报回调函数类型3：地址+数值 */
typedef void (*HOOCH_PROTOCOL_CurtainReportCallback3_t)(
    const HOOCH_PROTOCOL_CurtainFrame_t *frame);

/* 初始化窗帘模块 */
void HOOCH_Curtain_Init(void);

/* 清空当前窗帘数据 */
void HOOCH_PROTOCOL_Curtain_Clear(void);

/* 设置指定窗帘开关状态 */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SetValue(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value);

/* 设置指定窗帘停止状态 */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SetStop(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value);

/* 设置指定窗帘开合百分比 */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SetPercent(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value);

/* 设置指定窗帘开合角度 */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SetAngle(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value);

/* 直接写入完整的窗帘数据帧 */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SetFrame(
    const HOOCH_PROTOCOL_CurtainFrame_t *frame);

/* 获取当前缓存的窗帘数据帧 */
const HOOCH_PROTOCOL_CurtainFrame_t *HOOCH_PROTOCOL_Curtain_GetFrame(void);

/* 注册窗帘下发回调函数 */
void HOOCH_PROTOCOL_Curtain_RegisterCallback(
    HOOCH_PROTOCOL_CurtainCallback_t callback);

/* 注销窗帘下发回调函数 */
void HOOCH_PROTOCOL_Curtain_UnregisterCallback(void);

/* 注册窗帘上报回调函数（类型1：仅通道+数值） */
void HOOCH_PROTOCOL_Curtain_RegisterReportCallback(
    HOOCH_PROTOCOL_CurtainReportCallback1_t callback);

/* 注销窗帘上报回调函数（类型1） */
void HOOCH_PROTOCOL_Curtain_UnregisterReportCallback(void);

/* 注册窗帘上报回调函数（类型2：页面+通道+数值） */
void HOOCH_PROTOCOL_Curtain_RegisterReportCallback2(
    HOOCH_PROTOCOL_CurtainReportCallback2_t callback);

/* 注销窗帘上报回调函数（类型2） */
void HOOCH_PROTOCOL_Curtain_UnregisterReportCallback2(void);

/* 注册窗帘上报回调函数（类型3：地址+数值） */
void HOOCH_PROTOCOL_Curtain_RegisterReportCallback3(
    HOOCH_PROTOCOL_CurtainReportCallback3_t callback);

/* 注销窗帘上报回调函数（类型3） */
void HOOCH_PROTOCOL_Curtain_UnregisterReportCallback3(void);

/* ---- 统一入口 ---- */

/* 对外统一的窗帘入口，当前不支持无 key 的模糊发送 */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_Send(uint8_t value);

/* ---- 类型1 上报：仅通道+数值 ---- */

/* 上报窗帘开关状态（类型1：仅通道+数值） */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendSwitch(
    HOOCH_PROTOCOL_CurtainKey_t key,
    HOOCH_PROTOCOL_CurtainSwitchState_t state);

/* 上报窗帘停止状态（类型1：仅通道+数值） */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendStop(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value);

/* 上报窗帘开合百分比（类型1：仅通道+数值） */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendPercent(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value);

/* 上报窗帘开合角度（类型1：仅通道+数值） */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendAngle(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value);

/* ---- 类型2 上报：页面+通道+数值 ---- */

/* 上报窗帘开关状态（类型2：页面+通道） */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendSwitchPage(
    uint8_t page,
    HOOCH_PROTOCOL_CurtainKey_t key,
    HOOCH_PROTOCOL_CurtainSwitchState_t state);

/* 上报窗帘停止状态（类型2：页面+通道） */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendStopPage(
    uint8_t page,
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value);

/* 上报窗帘开合百分比（类型2：页面+通道） */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendPercentPage(
    uint8_t page,
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value);

/* 上报窗帘开合角度（类型2：页面+通道） */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendAnglePage(
    uint8_t page,
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value);

/* ---- 类型3 上报：地址+数值 ---- */

/* 上报窗帘开关状态（类型3：地址） */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendSwitchAddr(
    uint16_t address,
    HOOCH_PROTOCOL_CurtainSwitchState_t state);

/* 上报窗帘停止状态（类型3：地址） */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendStopAddr(
    uint16_t address,
    uint8_t value);

/* 上报窗帘开合百分比（类型3：地址） */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendPercentAddr(
    uint16_t address,
    uint8_t value);

/* 上报窗帘开合角度（类型3：地址） */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_SendAngleAddr(
    uint16_t address,
    uint8_t value);

/* ---- 下发接口 ---- */

/* 写入完整的窗帘下发数据帧 */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_DispatchFrame(
    const HOOCH_PROTOCOL_CurtainFrame_t *frame);

/* 下发窗帘开关状态 */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_DispatchSwitch(
    HOOCH_PROTOCOL_CurtainKey_t key,
    HOOCH_PROTOCOL_CurtainSwitchState_t state);

/* 下发窗帘停止状态 */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_DispatchStop(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value);

/* 下发窗帘开合百分比 */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_DispatchPercent(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value);

/* 下发窗帘开合角度 */
HOOCH_PROTOCOL_CurtainResult_t HOOCH_PROTOCOL_Curtain_DispatchAngle(
    HOOCH_PROTOCOL_CurtainKey_t key,
    uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_CURTAIN_H */
