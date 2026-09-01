#ifndef HOOCH_SETTING_H
#define HOOCH_SETTING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
/* 设置接口返回结果 */
typedef enum
{
    HOOCH_PROTOCOL_SETTING_RESULT_OK = 0U,          /* 执行成功 */
    HOOCH_PROTOCOL_SETTING_RESULT_ERROR,            /* 通用错误 */
    HOOCH_PROTOCOL_SETTING_RESULT_INVALID_PARAM,    /* 参数非法 */
} HOOCH_PROTOCOL_SettingResult_t;

/* 设置项枚举 */
typedef enum
{
    HOOCH_PROTOCOL_SETTING_ITEM_INVALID = 0U,               /* [0]  无效设置项 */
    HOOCH_PROTOCOL_SETTING_ITEM_DEFAULT_MAIN_PAGE,          /* [1]  默认主页面 */
    HOOCH_PROTOCOL_SETTING_ITEM_AVAILABLE_PAGE,             /* [2]  可用页面 */
    HOOCH_PROTOCOL_SETTING_ITEM_CURRENT_DISPLAY_PAGE,       /* [3]  屏幕当前显示页面 */
    HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_OFF_EFFECT,          /* [4]  熄屏效果 */
    HOOCH_PROTOCOL_SETTING_ITEM_BACKLIGHT_SWITCH,           /* [5]  背光灯开关 */
    HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_SWITCH,              /* [6]  感应开关 */
    HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_REPORT,              /* [7]  感应上报 */
    HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_SENSITIVITY,         /* [8]  感应灵敏度 */
    HOOCH_PROTOCOL_SETTING_ITEM_BACKLIGHT_BRIGHTNESS,       /* [9]   rgb按键亮度 */
    HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_BRIGHTNESS,          /* [10] 屏幕背光亮度 */
    HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_OFF_TIME,            /* [11] 熄屏时间 */
    HOOCH_PROTOCOL_SETTING_ITEM_DO_NOT_DISTURB_MODE,        /* [12] 勿扰模式 */
    HOOCH_PROTOCOL_SETTING_ITEM_TIME_CALIBRATION,           /* [13] 时间校准（UTC） */
    HOOCH_PROTOCOL_SETTING_ITEM_ADD_PAGE,                   /* [14] 新增页面 */
    HOOCH_PROTOCOL_SETTING_ITEM_HIDE_PAGE,                  /* [15] 隐藏页面 */
    HOOCH_PROTOCOL_SETTING_ITEM_DELETE_PAGE,                /* [16] 删除页面 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_STATUS_LIGHT,           /* [17] 按键状态灯 */
    HOOCH_PROTOCOL_SETTING_ITEM_LOCK_PARAMETER,             /* [18] 锁定参数 */
    HOOCH_PROTOCOL_SETTING_ITEM_TYPE_IDENTIFICATION,        /* [19] 类型识别 */
    HOOCH_PROTOCOL_SETTING_ITEM_ENTER_PAIRING_CLEAR_INTERLOCK,  /* [20] 进入对码、清码、互控 */
    HOOCH_PROTOCOL_SETTING_ITEM_PAIRING_CLEAR_INTERLOCK_STATUS, /* [21] 对码、清码、互控状态 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_POWER_ON_STATUS,            /* [22] 按键通电状态 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_RELAY_MAPPING,              /* [23] 按键对应继电器 */
    HOOCH_PROTOCOL_SETTING_ITEM_DEVICE_REBOOT,                  /* [24] 设备重启 */
    HOOCH_PROTOCOL_SETTING_ITEM_FACTORY_RESET,                  /* [25] 恢复出厂设置 */
    HOOCH_PROTOCOL_SETTING_ITEM_AIR_CONDITIONER_TEMPERATURE_MAX,/* [26] 空调温度上限 */
    HOOCH_PROTOCOL_SETTING_ITEM_AIR_CONDITIONER_TEMPERATURE_MIN,/* [27] 空调温度下限 */
    HOOCH_PROTOCOL_SETTING_ITEM_SCREENSAVER_TYPE,              /* [28] 屏保类型 */
    HOOCH_PROTOCOL_SETTING_ITEM_THEME,                         /* [29] 主题 */
    HOOCH_PROTOCOL_SETTING_ITEM_BACKGROUND,                    /* [30] 背景 */
    HOOCH_PROTOCOL_SETTING_ITEM_DEVICE_LOCK,                   /* [31] 设备锁启用/禁用 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_CHILD_LOCK,                /* [32] 按键童锁 */
    HOOCH_PROTOCOL_SETTING_ITEM_AUTO_BRIGHTNESS_ENABLE,        /* [33] 自动亮度使能 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_BACKLIGHT_BRIGHTNESS_NIGHT,/* [34] 按键背光亮度(黑夜) */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_BRIGHTNESS_NIGHT,          /* [35] 按键亮度(黑夜) */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_BACKLIGHT_BRIGHTNESS_DAY,  /* [36] 按键背光亮度(白天) */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_BRIGHTNESS_DAY,            /* [37] 按键亮度(白天) */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_WAKEUP_BACKLIGHT_ENABLE,   /* [38] 按键唤醒背光使能 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_STATE_LINKED_BACKLIGHT,    /* [39] 按键状态关联背光 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_LONG_PRESS_TIME,           /* [40] 按键长按时间 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEYS_APPLY_SIMULTANEOUSLY,     /* [41] 按键是否同时生效 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_BACKLIGHT_TIME,            /* [42] 按键背光时间 */
    HOOCH_PROTOCOL_SETTING_ITEM_DEVICE_CONFIG_STATUS,          /* [43] 设备配置状态 */
    HOOCH_PROTOCOL_SETTING_ITEM_STANDBY_BRIGHTNESS,            /* [44] 待机亮度 */
    HOOCH_PROTOCOL_SETTING_ITEM_SCREENSAVER_ENABLE,            /* [45] 屏保启用 */
    HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_CHILD_LOCK,             /* [46] 屏体童锁 */
    HOOCH_PROTOCOL_SETTING_ITEM_BACK_TO_HOME_TIME,             /* [47] 返回主页时间 */
    HOOCH_PROTOCOL_SETTING_ITEM_ENTER_SCREENSAVER_TIME,        /* [48] 进入屏保时间 */
    HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_PASSWORD,               /* [49] 屏密码 */
    HOOCH_PROTOCOL_SETTING_ITEM_PAGE_KEY_LOADED_FUNCTION,      /* [50] 页面按键装载的功能 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_JOG_TIME,                  /* [51] 按键点动时间 */
    HOOCH_PROTOCOL_SETTING_ITEM_MASTER_SW,                     /* [52] 总开总关 */
    HOOCH_PROTOCOL_SETTING_ITEM_VIBRATION_SWITCH,              /* [53] 振动开关 */
    HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_COUNTDOWN,              /* [54] 人感倒计时 */
    HOOCH_PROTOCOL_SETTING_ITEM_MULTICAST_GROUP_ID,            /* [55] 组播ID */
    HOOCH_PROTOCOL_SETTING_ITEM_TIME,                          /* [56] 普通时间（时分秒） */
    HOOCH_PROTOCOL_SETTING_ITEM_DATE,                          /* [57] 日期 */
    HOOCH_PROTOCOL_SETTING_ITEM_LANGUAGE,                      /* [58] 语言选择 */
    HOOCH_PROTOCOL_SETTING_ITEM_CONFIG_STATUS,                 /* [59] 配置状态 */
    HOOCH_PROTOCOL_SETTING_ITEM_PANEL_UPDATE_STATUS,           /* [60] 面板更新状态 */
    HOOCH_PROTOCOL_SETTING_ITEM_VERSION,                       /* [61] 版本 */
    HOOCH_PROTOCOL_SETTING_ITEM_PROGRAMMING_MODE,              /* [62] 编程模式 */
    HOOCH_PROTOCOL_SETTING_ITEM_BLE_KNX_GET_CONFIG,               /* [63] BLE KNX获取配置 */
    HOOCH_PROTOCOL_SETTING_ITEM_BLE_KNX_GET_STATUS,               /* [64] BLE KNX获取状态 */
    HOOCH_PROTOCOL_SETTING_ITEM_MODULE_TYPE,                      /* [65] 模组类型 */
    HOOCH_PROTOCOL_SETTING_ITEM_INTERLOCK_FUNCTION,               /* [66] 互锁功能 */
    HOOCH_PROTOCOL_SETTING_ITEM_TEMPERATURE_UNIT,                 /* [67] 温度显示单位 */
    HOOCH_PROTOCOL_SETTING_ITEM_SOUND_ENABLE,                     /* [68] 声音使能 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_BACKLIGHT_MODE,               /* [69] 按键背光模式 */
    HOOCH_PROTOCOL_SETTING_ITEM_FLOOR_HEATING_TEMPERATURE_MAX,    /* [70] 地暖温度上限 */
    HOOCH_PROTOCOL_SETTING_ITEM_FLOOR_HEATING_TEMPERATURE_MIN,    /* [71] 地暖温度下限 */
    HOOCH_PROTOCOL_SETTING_ITEM_WEATHER,                          /* [72] 天气 */

} HOOCH_PROTOCOL_SettingItem_t;




/* 页面枚举 */
typedef enum
{
    HOOCH_PROTOCOL_SETTING_PAGE_INVALID = 0U,    /* 无效页面 */
    HOOCH_PROTOCOL_SETTING_PAGE_MEMORY,          /* 记忆页面 */
    HOOCH_PROTOCOL_SETTING_PAGE_SWITCH,          /* 开关页面 */
    HOOCH_PROTOCOL_SETTING_PAGE_SCENE,           /* 场景页面 */
    HOOCH_PROTOCOL_SETTING_PAGE_LIGHT,           /* 灯光页面 */
    HOOCH_PROTOCOL_SETTING_PAGE_CURTAIN,         /* 窗帘页面 */
    HOOCH_PROTOCOL_SETTING_PAGE_AIR_CONDITIONER, /* 空调页面 */
    HOOCH_PROTOCOL_SETTING_PAGE_PAGE1,           /* 页面一 */
    HOOCH_PROTOCOL_SETTING_PAGE_PAGE2,           /* 页面二 */
    HOOCH_PROTOCOL_SETTING_PAGE_LOCAL_SCENE,     /* 本地情景页面 */
    HOOCH_PROTOCOL_SETTING_PAGE_FLOOR_HEATING,   /* 地暖页面 */
    HOOCH_PROTOCOL_SETTING_PAGE_FRESH_AIR,       /* 新风页面 */
} HOOCH_PROTOCOL_SettingPage_t;

/* 语言选择 */
typedef enum
{
    HOOCH_PROTOCOL_SETTING_LANGUAGE_INVALID = 0xFFU, /* 无效 */
    HOOCH_PROTOCOL_SETTING_LANGUAGE_CHINESE  = 0U,   /* 中文 */
    HOOCH_PROTOCOL_SETTING_LANGUAGE_ENGLISH  = 1U,   /* 英文 */
    HOOCH_PROTOCOL_SETTING_LANGUAGE_RUSSIAN  = 2U,   /* 俄语 */
    HOOCH_PROTOCOL_SETTING_LANGUAGE_GERMAN   = 3U,   /* 德语 */
    HOOCH_PROTOCOL_SETTING_LANGUAGE_FRENCH   = 4U,   /* 法语 */
    HOOCH_PROTOCOL_SETTING_LANGUAGE_SPANISH  = 5U,   /* 西班牙语 */
    HOOCH_PROTOCOL_SETTING_LANGUAGE_ITALIAN  = 6U,   /* 意大利语 */
    HOOCH_PROTOCOL_SETTING_LANGUAGE_JAPANESE = 7U,   /* 日语 */
    HOOCH_PROTOCOL_SETTING_LANGUAGE_KOREAN   = 8U,   /* 韩语 */
    HOOCH_PROTOCOL_SETTING_LANGUAGE_ARABIC   = 9U,   /* 阿拉伯语 */
} HOOCH_PROTOCOL_SettingLanguage_t;

typedef enum
{
    HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_INVALID = 0xFFU, /* 无效类型 */
    HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_NEVER = 0U,      /* 永不熄屏 */
    HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_AUTO = 1U,       /* 自动熄屏 */
    HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_VALUE = 2U,      /* 数值熄屏 */
    HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_BACKLIGHT_OFF = 3U, /* 关闭背光 */
    
} HOOCH_PROTOCOL_SettingScreenOffTime_t;

typedef enum
{
    HOOCH_PROTOCOL_SETTING_PAIRING_CLEAR_INTERLOCK_OP_INVALID = 0xFFU, /* 无效类型 */
    HOOCH_PROTOCOL_SETTING_PAIRING_CLEAR_INTERLOCK_OP_PAIRING = 0U,    /* 对码 */
    HOOCH_PROTOCOL_SETTING_PAIRING_CLEAR_INTERLOCK_OP_CLEAR = 1U,      /* 清码 */
    HOOCH_PROTOCOL_SETTING_PAIRING_CLEAR_INTERLOCK_OP_INTERLOCK = 2U,  /* 互控 */
} HOOCH_PROTOCOL_SettingPairingClearInterlockOp_t;

typedef enum
{
    HOOCH_PROTOCOL_SETTING_PAIRING_CLEAR_RESULT_INVALID = 0xFFU, /* 无效类型 */
    HOOCH_PROTOCOL_SETTING_PAIRING_CLEAR_RESULT_FAIL = 0U,       /* 失败 */
    HOOCH_PROTOCOL_SETTING_PAIRING_CLEAR_RESULT_SUCCESS = 1U,    /* 成功 */
} HOOCH_PROTOCOL_SettingPairingClearResult_t;

typedef enum
{
    HOOCH_PROTOCOL_SETTING_INTERLOCK_STATUS_INVALID = 0xFFU,   /* 无效类型 */
    HOOCH_PROTOCOL_SETTING_INTERLOCK_STATUS_RELEASE = 0U,      /* 解控状态 */
    HOOCH_PROTOCOL_SETTING_INTERLOCK_STATUS_INTERLOCK = 1U,    /* 互控状态 */
} HOOCH_PROTOCOL_SettingInterlockStatus_t;






/* 设置数据帧描述 */
typedef struct
{
    HOOCH_PROTOCOL_SettingItem_t item;    /* 设置索引 */
    HOOCH_PROTOCOL_SettingScreenOffTime_t screen_off_time_type; /* 熄屏时间类型 */
    uint32_t value;                      /* 当前值 */
    HOOCH_PROTOCOL_SettingPage_t page;   /* 页面 */
    uint8_t param1;
    uint8_t param2;
    uint8_t param3;
    uint8_t param4;
    uint8_t data[10];                   /* 不定长原始数据缓冲区（时间、日期等） */
    uint8_t sequence;                   /* 更新序号 */
    uint8_t valid;                      /* 当前数据是否有效 */
    uint8_t channel;                    /* 通道号 */
} HOOCH_PROTOCOL_SettingFrame_t;

/* 设置回调函数类型 */
typedef void (*HOOCH_PROTOCOL_SettingCallback_t)(
    const HOOCH_PROTOCOL_SettingFrame_t *frame);

/* 初始化设置模块 */
void HOOCH_Setting_Init(void);

/* 清空当前设置数据 */
void HOOCH_PROTOCOL_Setting_Clear(void);

/* 设置当前设置值 */
HOOCH_PROTOCOL_SettingResult_t HOOCH_PROTOCOL_Setting_SetValue(uint8_t value);

/* 直接写入完整的设置数据帧 */
HOOCH_PROTOCOL_SettingResult_t HOOCH_PROTOCOL_Setting_SetFrame(
    const HOOCH_PROTOCOL_SettingFrame_t *frame);

/* 获取当前缓存的设置数据帧 */
const HOOCH_PROTOCOL_SettingFrame_t *HOOCH_PROTOCOL_Setting_GetFrame(void);

/* 注册设置回调函数 */
void HOOCH_PROTOCOL_Setting_RegisterCallback(
    HOOCH_PROTOCOL_SettingCallback_t callback);

/* 注销设置回调函数 */
void HOOCH_PROTOCOL_Setting_UnregisterCallback(void);

/* 对外统一的设置入口 */
HOOCH_PROTOCOL_SettingResult_t HOOCH_PROTOCOL_Setting_Send(uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_SETTING_H */
