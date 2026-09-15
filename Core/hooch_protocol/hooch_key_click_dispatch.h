#ifndef HOOCH_KEY_CLICK_DISPATCH_H
#define HOOCH_KEY_CLICK_DISPATCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
/* 按键下发模块支持的按键总数 */
#define HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_COUNT    16U

/* 按键下发接口返回结果 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_RESULT_OK = 0U,          /* 执行成功 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_RESULT_ERROR,            /* 通用错误 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_RESULT_INVALID_PARAM,    /* 参数非法 */
} HOOCH_PROTOCOL_KeyClickDispatchResult_t;

/* `Core/hooch_protocol` 目录下按键点击下发模块的按键编号 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_INVALID = 0U,    /* 无效按键 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_1 = 1U,          /* 按键1 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_2,               /* 按键2 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_3,               /* 按键3 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_4,               /* 按键4 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_5,               /* 按键5 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_6,               /* 按键6 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_7,               /* 按键7 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_8,               /* 按键8 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_9,               /* 按键9 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_10,              /* 按键10 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_11,              /* 按键11 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_12,              /* 按键12 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_13,              /* 按键13 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_14,              /* 按键14 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_15,              /* 按键15 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_KEY_16,              /* 按键16 */
} HOOCH_PROTOCOL_KeyClickDispatchKey_t;

/* 按键点击下发事件类型 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_EVENT_NONE = 0U,            /* 无事件 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_EVENT_SINGLE_CLICK,         /* 单击 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_EVENT_DOUBLE_CLICK,         /* 双击 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_EVENT_LONG_CLICK,           /* 长按 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_EVENT_LONG_CLICK_1P5S,      /* 长按 1.5 秒 */
} HOOCH_PROTOCOL_KeyClickDispatchEvent_t;

/* 按键状态 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_STATE_OFF = 0U,     /* 关闭状态 */
    HOOCH_PROTOCOL_KEY_CLICK_DISPATCH_STATE_ON,           /* 开启状态 */
} HOOCH_PROTOCOL_KeyClickDispatchState_t;

/* 按键点击下发帧描述 */
typedef struct
{
    HOOCH_PROTOCOL_KeyClickDispatchKey_t key;         /* 按键编号 */
    HOOCH_PROTOCOL_KeyClickDispatchEvent_t event;     /* 点击事件 */
    HOOCH_PROTOCOL_KeyClickDispatchState_t state;     /* 开关状态 */
    uint8_t sequence;                                 /* 下发序号 */
    uint8_t valid;                                    /* 当前数据是否有效 */
} HOOCH_PROTOCOL_KeyClickDispatchFrame_t;

/* 按键点击下发回调函数类型 */
typedef void (*HOOCH_PROTOCOL_KeyClickDispatchCallback_t)(
    const HOOCH_PROTOCOL_KeyClickDispatchFrame_t *frame);

/* 初始化按键点击下发模块 */
void HOOCH_KeyClickDispatch_Init(void);

/* 清空当前按键点击下发数据 */
void HOOCH_PROTOCOL_KeyClickDispatch_Clear(void);

/* 校验按键编号是否有效 */
uint8_t HOOCH_PROTOCOL_KeyClickDispatch_IsValidKey(HOOCH_PROTOCOL_KeyClickDispatchKey_t key);

/* 根据按键、事件和状态设置当前下发数据 */
HOOCH_PROTOCOL_KeyClickDispatchResult_t HOOCH_PROTOCOL_KeyClickDispatch_Set(
    HOOCH_PROTOCOL_KeyClickDispatchKey_t key,
    HOOCH_PROTOCOL_KeyClickDispatchEvent_t event,
    HOOCH_PROTOCOL_KeyClickDispatchState_t state);

/* 直接写入完整的按键点击下发帧 */
HOOCH_PROTOCOL_KeyClickDispatchResult_t HOOCH_PROTOCOL_KeyClickDispatch_SetFrame(
    const HOOCH_PROTOCOL_KeyClickDispatchFrame_t *frame);

/* 获取当前缓存的按键点击下发帧 */
const HOOCH_PROTOCOL_KeyClickDispatchFrame_t *HOOCH_PROTOCOL_KeyClickDispatch_GetFrame(void);

/* 注册按键点击下发回调函数 */
void HOOCH_PROTOCOL_KeyClickDispatch_RegisterCallback(
    HOOCH_PROTOCOL_KeyClickDispatchCallback_t callback);

/* 注销按键点击下发回调函数 */
void HOOCH_PROTOCOL_KeyClickDispatch_UnregisterCallback(void);

/* 对外统一的按键点击下发入口 */
HOOCH_PROTOCOL_KeyClickDispatchResult_t HOOCH_PROTOCOL_KeyClickDispatch_Send(
    HOOCH_PROTOCOL_KeyClickDispatchKey_t key,
    HOOCH_PROTOCOL_KeyClickDispatchEvent_t event,
    HOOCH_PROTOCOL_KeyClickDispatchState_t state);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_KEY_CLICK_DISPATCH_H */
