#ifndef HOOCH_KEY_STATUS_DISPATCH_H
#define HOOCH_KEY_STATUS_DISPATCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
/* 按键状态下发模块支持的按键总数 */
#define HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_COUNT    8U

/* 按键状态下发接口返回结果 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_RESULT_OK = 0U,          /* 执行成功 */
    HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_RESULT_ERROR,            /* 通用错误 */
    HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_RESULT_INVALID_PARAM,    /* 参数非法 */
} HOOCH_PROTOCOL_KeyStatusDispatchResult_t;

/* `Core/hooch_protocol` 目录下按键状态下发模块的按键编号 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_INVALID = 0U,    /* 无效按键 */
    HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_1 = 1U,          /* 按键1 */
    HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_2,               /* 按键2 */
    HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_3,               /* 按键3 */
    HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_4,               /* 按键4 */
    HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_5,               /* 按键5 */
    HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_6,               /* 按键6 */
    HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_7,               /* 按键7 */
    HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_8,               /* 按键8 */
} HOOCH_PROTOCOL_KeyStatusDispatchKey_t;

/* 按键状态 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_STATE_RELEASED = 0U,    /* 松开 */
    HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_STATE_PRESSED,          /* 按下 */
} HOOCH_PROTOCOL_KeyStatusDispatchState_t;

/* 按键状态下发帧描述 */
typedef struct
{
    HOOCH_PROTOCOL_KeyStatusDispatchKey_t key;         /* 按键编号 */
    HOOCH_PROTOCOL_KeyStatusDispatchState_t state;     /* 按键状态 */
    uint8_t sequence;                                  /* 下发序号 */
    uint8_t valid;                                     /* 当前数据是否有效 */
} HOOCH_PROTOCOL_KeyStatusDispatchFrame_t;

/* 按键状态下发回调函数类型 */
typedef void (*HOOCH_PROTOCOL_KeyStatusDispatchCallback_t)(
    const HOOCH_PROTOCOL_KeyStatusDispatchFrame_t *frame);

/* 初始化按键状态下发模块 */
void HOOCH_KeyStatusDispatch_Init(void);

/* 清空当前按键状态下发数据 */
void HOOCH_PROTOCOL_KeyStatusDispatch_Clear(void);

/* 校验按键编号是否有效 */
uint8_t HOOCH_PROTOCOL_KeyStatusDispatch_IsValidKey(HOOCH_PROTOCOL_KeyStatusDispatchKey_t key);

/* 根据按键和状态设置当前下发数据 */
HOOCH_PROTOCOL_KeyStatusDispatchResult_t HOOCH_PROTOCOL_KeyStatusDispatch_Set(
    HOOCH_PROTOCOL_KeyStatusDispatchKey_t key,
    HOOCH_PROTOCOL_KeyStatusDispatchState_t state);

/* 直接写入完整的按键状态下发帧 */
HOOCH_PROTOCOL_KeyStatusDispatchResult_t HOOCH_PROTOCOL_KeyStatusDispatch_SetFrame(
    const HOOCH_PROTOCOL_KeyStatusDispatchFrame_t *frame);

/* 获取当前缓存的按键状态下发帧 */
const HOOCH_PROTOCOL_KeyStatusDispatchFrame_t *HOOCH_PROTOCOL_KeyStatusDispatch_GetFrame(void);

/* 注册按键状态下发回调函数 */
void HOOCH_PROTOCOL_KeyStatusDispatch_RegisterCallback(
    HOOCH_PROTOCOL_KeyStatusDispatchCallback_t callback);

/* 注销按键状态下发回调函数 */
void HOOCH_PROTOCOL_KeyStatusDispatch_UnregisterCallback(void);

/* 对外统一的按键状态下发入口 */
HOOCH_PROTOCOL_KeyStatusDispatchResult_t HOOCH_PROTOCOL_KeyStatusDispatch_Send(
    HOOCH_PROTOCOL_KeyStatusDispatchKey_t key,
    HOOCH_PROTOCOL_KeyStatusDispatchState_t state);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_KEY_STATUS_DISPATCH_H */
