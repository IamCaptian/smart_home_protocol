#ifndef HOOCH_PAGE_ADJUST_H
#define HOOCH_PAGE_ADJUST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
/* 页面调节接口返回结果 */
typedef enum
{
    HOOCH_PROTOCOL_PAGE_ADJUST_RESULT_OK = 0U,          /* 执行成功 */
    HOOCH_PROTOCOL_PAGE_ADJUST_RESULT_ERROR,            /* 通用错误 */
    HOOCH_PROTOCOL_PAGE_ADJUST_RESULT_INVALID_PARAM,    /* 参数非法 */
} HOOCH_PROTOCOL_PageAdjustResult_t;

/* 页面调节数据帧描述 */
typedef struct
{
    uint8_t value;      /* 当前值 */
    uint8_t sequence;   /* 更新序号 */
    uint8_t valid;      /* 当前数据是否有效 */
} HOOCH_PROTOCOL_PageAdjustFrame_t;

/* 页面调节回调函数类型 */
typedef void (*HOOCH_PROTOCOL_PageAdjustCallback_t)(
    const HOOCH_PROTOCOL_PageAdjustFrame_t *frame);

/* 初始化页面调节模块 */
void HOOCH_PageAdjust_Init(void);

/* 清空当前页面调节数据 */
void HOOCH_PROTOCOL_PageAdjust_Clear(void);

/* 设置当前页面调节值 */
HOOCH_PROTOCOL_PageAdjustResult_t HOOCH_PROTOCOL_PageAdjust_SetValue(uint8_t value);

/* 直接写入完整的页面调节数据帧 */
HOOCH_PROTOCOL_PageAdjustResult_t HOOCH_PROTOCOL_PageAdjust_SetFrame(
    const HOOCH_PROTOCOL_PageAdjustFrame_t *frame);

/* 获取当前缓存的页面调节数据帧 */
const HOOCH_PROTOCOL_PageAdjustFrame_t *HOOCH_PROTOCOL_PageAdjust_GetFrame(void);

/* 注册页面调节回调函数 */
void HOOCH_PROTOCOL_PageAdjust_RegisterCallback(
    HOOCH_PROTOCOL_PageAdjustCallback_t callback);

/* 注销页面调节回调函数 */
void HOOCH_PROTOCOL_PageAdjust_UnregisterCallback(void);

/* 对外统一的页面调节入口 */
HOOCH_PROTOCOL_PageAdjustResult_t HOOCH_PROTOCOL_PageAdjust_Send(uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_PAGE_ADJUST_H */
