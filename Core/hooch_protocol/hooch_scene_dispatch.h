#ifndef HOOCH_SCENE_DISPATCH_H
#define HOOCH_SCENE_DISPATCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
/* 场景下发模块支持的场景总数 */
#define HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_COUNT    16U

/* 场景下发接口返回结果 */
typedef enum
{
    HOOCH_PROTOCOL_SCENE_DISPATCH_RESULT_OK = 0U,          /* 执行成功 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_RESULT_ERROR,            /* 通用错误 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_RESULT_INVALID_PARAM,    /* 参数非法 */
} HOOCH_PROTOCOL_SceneDispatchResult_t;

/* 场景下发模块的场景编号 */
typedef enum
{
    HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_INVALID = 0U,    /* 无效场景 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_1 = 1U,          /* 场景1 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_2,               /* 场景2 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_3,               /* 场景3 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_4,               /* 场景4 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_5,               /* 场景5 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_6,               /* 场景6 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_7,               /* 场景7 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_8,               /* 场景8 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_9,               /* 场景9 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_10,              /* 场景10 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_11,              /* 场景11 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_12,              /* 场景12 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_13,              /* 场景13 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_14,              /* 场景14 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_15,              /* 场景15 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_16,              /* 场景16 */
} HOOCH_PROTOCOL_SceneDispatchScene_t;

/* 场景下发控制项 */
typedef enum
{
    HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_INVALID = 0U,     /* 无效控制项 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_ALL,              /* 全量控制（触发展示） */
    HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_ENABLE_BIT,       /* 仅更新使能位(1 byte 位图, bit0:使能 bit1:场景) */
    HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_DEVICE_DESC,      /* 设备描述字符串 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_DEFAULT_ICON,     /* 默认图标 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_SELECTED_ICON,    /* 选中图标 */
    HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_LEARN,            /* 学习(P->K, 1 byte, 1=学习) */
    HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_MAX,              /* 哨兵值，新增枚举请插入到上方 */
} HOOCH_PROTOCOL_SceneDispatchControlItem_t;

/* 场景下发帧描述 */
typedef struct
{
    HOOCH_PROTOCOL_SceneDispatchScene_t scene;                /* 场景编号 */
    HOOCH_PROTOCOL_SceneDispatchControlItem_t control_item;   /* 本次控制项，支持只更新单一参数 */
    uint8_t device_desc[24];                                  /* 设备描述符字符串 */
    uint8_t value;                                            /* 参数值（图标等） */
    uint8_t sequence;                                         /* 更新序号 */
    uint8_t valid;                                            /* 当前数据是否有效 */
} HOOCH_PROTOCOL_SceneDispatchFrame_t;

/* 场景下发回调函数类型 */
typedef void (*HOOCH_PROTOCOL_SceneDispatchCallback_t)(
    const HOOCH_PROTOCOL_SceneDispatchFrame_t *frame);

void HOOCH_SceneDispatch_Init(void);
void HOOCH_PROTOCOL_SceneDispatch_Clear(void);
uint8_t HOOCH_PROTOCOL_SceneDispatch_IsValidScene(HOOCH_PROTOCOL_SceneDispatchScene_t scene);
uint8_t HOOCH_PROTOCOL_SceneDispatch_IsValidControlItem(
    HOOCH_PROTOCOL_SceneDispatchControlItem_t control_item);
HOOCH_PROTOCOL_SceneDispatchResult_t HOOCH_PROTOCOL_SceneDispatch_Set(
    HOOCH_PROTOCOL_SceneDispatchScene_t scene);
HOOCH_PROTOCOL_SceneDispatchResult_t HOOCH_PROTOCOL_SceneDispatch_SetFrame(
    const HOOCH_PROTOCOL_SceneDispatchFrame_t *frame);
const HOOCH_PROTOCOL_SceneDispatchFrame_t *HOOCH_PROTOCOL_SceneDispatch_GetFrame(void);
void HOOCH_PROTOCOL_SceneDispatch_RegisterCallback(
    HOOCH_PROTOCOL_SceneDispatchCallback_t callback);
void HOOCH_PROTOCOL_SceneDispatch_UnregisterCallback(void);
HOOCH_PROTOCOL_SceneDispatchResult_t HOOCH_PROTOCOL_SceneDispatch_Send(
    HOOCH_PROTOCOL_SceneDispatchScene_t scene);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_SCENE_DISPATCH_H */
