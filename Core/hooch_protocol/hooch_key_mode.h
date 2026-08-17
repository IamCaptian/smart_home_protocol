#ifndef HOOCH_KEY_MODE_H
#define HOOCH_KEY_MODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
/* 支持的按键数量。 */
#define HOOCH_PROTOCOL_KEY_MODE_KEY_COUNT    8U

/* 按键模式接口返回结果。 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_MODE_RESULT_OK = 0U,
    HOOCH_PROTOCOL_KEY_MODE_RESULT_ERROR,
    HOOCH_PROTOCOL_KEY_MODE_RESULT_INVALID_PARAM,
} HOOCH_PROTOCOL_KeyModeResult_t;

/* 按键编号枚举。 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_MODE_KEY_INVALID = 0U,    /* 无效按键 */
    HOOCH_PROTOCOL_KEY_MODE_KEY_1 = 1U,          /* 按键1 */
    HOOCH_PROTOCOL_KEY_MODE_KEY_2,
    HOOCH_PROTOCOL_KEY_MODE_KEY_3,
    HOOCH_PROTOCOL_KEY_MODE_KEY_4,
    HOOCH_PROTOCOL_KEY_MODE_KEY_5,
    HOOCH_PROTOCOL_KEY_MODE_KEY_6,
    HOOCH_PROTOCOL_KEY_MODE_KEY_7,
    HOOCH_PROTOCOL_KEY_MODE_KEY_8,
} HOOCH_PROTOCOL_KeyModeKey_t;

/* 按键模式枚举。 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_MODE_TYPE_WIRED_WIRELESS_SWITCH = 0U,    /* 有线和无线开关 */
    HOOCH_PROTOCOL_KEY_MODE_TYPE_WIRELESS_SWITCH = 1U,          /* 无线开关 */
    HOOCH_PROTOCOL_KEY_MODE_TYPE_DYNAMIC_SWITCH = 2U,           /* 灵动开关 */
    HOOCH_PROTOCOL_KEY_MODE_TYPE_WIRELESS_REMOTE_SWITCH = 3U,   /* 无线遥控开关 */
    HOOCH_PROTOCOL_KEY_MODE_TYPE_DIMMER_SWITCH = 4U,            /* 调光/调色开关 */
    HOOCH_PROTOCOL_KEY_MODE_TYPE_CURTAIN_SWITCH = 5U,           /* 窗帘开关 */
    HOOCH_PROTOCOL_KEY_MODE_TYPE_LOCK_SWITCH = 6U,              /* 锁定开关 */
    HOOCH_PROTOCOL_KEY_MODE_TYPE_MOMENTARY_SWITCH = 7U,         /* 点动开关 */
    HOOCH_PROTOCOL_KEY_MODE_TYPE_NORMAL_SWITCH = 8U,            /* 普通开关 */
    HOOCH_PROTOCOL_KEY_MODE_TYPE_SCENE_SWITCH = 9U,             /* 场景模式 */
    HOOCH_PROTOCOL_KEY_MODE_TYPE_SCENARIO_SWITCH = 10U,         /* 情景模式 */
    HOOCH_PROTOCOL_KEY_MODE_TYPE_VIRTUAL_SWITCH = 11U,           /* 虚拟开关 */
    HOOCH_PROTOCOL_KEY_MODE_TYPE_NORMAL_SWITCH_TOGGLE = 12U,    /* 普通开关反转模式 */
    HOOCH_PROTOCOL_KEY_MODE_TYPE_INVALID = 0xFFU,               /* 无效类型 */

} HOOCH_PROTOCOL_KeyModeType_t;

/* 上电状态枚举。 */
typedef enum
{
    HOOCH_PROTOCOL_KEY_MODE_POWER_ON_MEMORY = 0U,     /* 断电记忆 */
    HOOCH_PROTOCOL_KEY_MODE_POWER_ON_OFF = 1U,        /* 上电关闭 */
    HOOCH_PROTOCOL_KEY_MODE_POWER_ON_ON = 2U,         /* 上电打开 */
    HOOCH_PROTOCOL_KEY_MODE_POWER_ON_INVALID = 0xFFU, /* 无效类型 */
} HOOCH_PROTOCOL_KeyModePowerOnStatus_t;

/* 按键模式帧结构。 */
typedef struct
{
    HOOCH_PROTOCOL_KeyModeKey_t key;      /* 按键编号 */
    HOOCH_PROTOCOL_KeyModePowerOnStatus_t power_on_status; /* 上电状态 */
    HOOCH_PROTOCOL_KeyModeType_t type;    /* 按键模式 */
    uint8_t sequence;                     /* 更新序号 */
    uint8_t valid;                        /* 当前数据是否有效 */
} HOOCH_PROTOCOL_KeyModeFrame_t;

/* 按键模式回调函数类型。 */
typedef void (*HOOCH_PROTOCOL_KeyModeCallback_t)(
    const HOOCH_PROTOCOL_KeyModeFrame_t *frame);

void HOOCH_KeyMode_Init(void);
void HOOCH_PROTOCOL_KeyMode_Clear(void);
uint8_t HOOCH_PROTOCOL_KeyMode_IsValidKey(HOOCH_PROTOCOL_KeyModeKey_t key);
uint8_t HOOCH_PROTOCOL_KeyMode_IsValidType(HOOCH_PROTOCOL_KeyModeType_t type);
uint8_t HOOCH_PROTOCOL_KeyMode_IsValidPowerOnStatus(HOOCH_PROTOCOL_KeyModePowerOnStatus_t status);
const char *HOOCH_PROTOCOL_KeyMode_TypeToString(HOOCH_PROTOCOL_KeyModeType_t type);

HOOCH_PROTOCOL_KeyModeResult_t HOOCH_PROTOCOL_KeyMode_Set(
    HOOCH_PROTOCOL_KeyModeKey_t key,
    HOOCH_PROTOCOL_KeyModePowerOnStatus_t power_on_status,
    HOOCH_PROTOCOL_KeyModeType_t type);

HOOCH_PROTOCOL_KeyModeResult_t HOOCH_PROTOCOL_KeyMode_SetFrame(
    const HOOCH_PROTOCOL_KeyModeFrame_t *frame);

const HOOCH_PROTOCOL_KeyModeFrame_t *HOOCH_PROTOCOL_KeyMode_GetFrame(void);
HOOCH_PROTOCOL_KeyModeType_t HOOCH_PROTOCOL_KeyMode_GetType(HOOCH_PROTOCOL_KeyModeKey_t key);
HOOCH_PROTOCOL_KeyModePowerOnStatus_t HOOCH_PROTOCOL_KeyMode_GetPowerOnStatus(
    HOOCH_PROTOCOL_KeyModeKey_t key);

void HOOCH_PROTOCOL_KeyMode_RegisterCallback(
    HOOCH_PROTOCOL_KeyModeCallback_t callback);

void HOOCH_PROTOCOL_KeyMode_UnregisterCallback(void);

HOOCH_PROTOCOL_KeyModeResult_t HOOCH_PROTOCOL_KeyMode_Send(
    HOOCH_PROTOCOL_KeyModeKey_t key,
    HOOCH_PROTOCOL_KeyModePowerOnStatus_t power_on_status,
    HOOCH_PROTOCOL_KeyModeType_t type);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_KEY_MODE_H */
