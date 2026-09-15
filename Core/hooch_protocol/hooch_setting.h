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
    HOOCH_PROTOCOL_SETTING_ITEM_BACKLIGHT_BRIGHTNESS,       /* [9]   rgb按键亮度[低亮、默认] */
    HOOCH_PROTOCOL_SETTING_ITEM_BACKLIGHT_BRIGHTNESS_HIGHLIGHT, /* [10] rgb按键亮度高亮 */
    HOOCH_PROTOCOL_SETTING_ITEM_INDICATOR_LIGHT_COLOR,             /* [11] RGB指示灯颜色 (value = RGB888) */
    HOOCH_PROTOCOL_SETTING_ITEM_INDICATOR_LIGHT_SWITCH,            /* [12] RGB指示灯开关 (0=off, 1=on) */
    HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_BRIGHTNESS,          /* [13] 屏幕背光亮度 */
    HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_OFF_TIME,            /* [14] 熄屏时间 */
    HOOCH_PROTOCOL_SETTING_ITEM_DO_NOT_DISTURB_MODE,        /* [15] 勿扰模式 */
    HOOCH_PROTOCOL_SETTING_ITEM_TIME_CALIBRATION,           /* [16] 时间校准（UTC） */
    HOOCH_PROTOCOL_SETTING_ITEM_ADD_PAGE,                   /* [17] 新增页面 */
    HOOCH_PROTOCOL_SETTING_ITEM_HIDE_PAGE,                  /* [18] 隐藏页面 */
    HOOCH_PROTOCOL_SETTING_ITEM_DELETE_PAGE,                /* [19] 删除页面 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_STATUS_LIGHT,           /* [20] 按键状态灯 */
    HOOCH_PROTOCOL_SETTING_ITEM_LOCK_PARAMETER,             /* [21] 锁定参数 */
    HOOCH_PROTOCOL_SETTING_ITEM_TYPE_IDENTIFICATION,        /* [22] 类型识别 */
    HOOCH_PROTOCOL_SETTING_ITEM_ENTER_PAIRING_CLEAR_INTERLOCK,  /* [23] 进入对码、清码、互控 */
    HOOCH_PROTOCOL_SETTING_ITEM_PAIRING_CLEAR_INTERLOCK_STATUS, /* [24] 对码、清码、互控状态 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_POWER_ON_STATUS,            /* [25] 按键通电状态 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_RELAY_MAPPING,              /* [26] 按键对应继电器 */
    HOOCH_PROTOCOL_SETTING_ITEM_DEVICE_REBOOT,                  /* [27] 设备重启 */
    HOOCH_PROTOCOL_SETTING_ITEM_FACTORY_RESET,                  /* [28] 恢复出厂设置 */
    HOOCH_PROTOCOL_SETTING_ITEM_AIR_CONDITIONER_TEMPERATURE_MAX,/* [29] 空调温度上限 */
    HOOCH_PROTOCOL_SETTING_ITEM_AIR_CONDITIONER_TEMPERATURE_MIN,/* [30] 空调温度下限 */
    HOOCH_PROTOCOL_SETTING_ITEM_SCREENSAVER_TYPE,              /* [31] 屏保类型 */
    HOOCH_PROTOCOL_SETTING_ITEM_THEME,                         /* [32] 主题 */
    HOOCH_PROTOCOL_SETTING_ITEM_BACKGROUND,                    /* [33] 背景 */
    HOOCH_PROTOCOL_SETTING_ITEM_DEVICE_LOCK,                   /* [34] 设备锁启用/禁用 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_CHILD_LOCK,                /* [35] 按键童锁 */
    HOOCH_PROTOCOL_SETTING_ITEM_AUTO_BRIGHTNESS_ENABLE,        /* [36] 自动亮度使能 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_BACKLIGHT_BRIGHTNESS_NIGHT,/* [37] 按键背光亮度(黑夜) */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_BRIGHTNESS_NIGHT,          /* [38] 按键亮度(黑夜) */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_BACKLIGHT_BRIGHTNESS_DAY,  /* [39] 按键背光亮度(白天) */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_BRIGHTNESS_DAY,            /* [40] 按键亮度(白天) */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_WAKEUP_BACKLIGHT_ENABLE,   /* [41] 按键唤醒背光使能 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_STATE_LINKED_BACKLIGHT,    /* [42] 按键状态关联背光 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_LONG_PRESS_TIME,           /* [43] 按键长按时间 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEYS_APPLY_SIMULTANEOUSLY,     /* [44] 按键是否同时生效 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_BACKLIGHT_TIME,            /* [45] 按键背光时间 */
    HOOCH_PROTOCOL_SETTING_ITEM_DEVICE_CONFIG_STATUS,          /* [46] 设备配置状态 */
    HOOCH_PROTOCOL_SETTING_ITEM_STANDBY_BRIGHTNESS,            /* [47] 待机亮度 */
    HOOCH_PROTOCOL_SETTING_ITEM_SCREENSAVER_ENABLE,            /* [48] 屏保启用 */
    HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_CHILD_LOCK,             /* [49] 屏体童锁 */
    HOOCH_PROTOCOL_SETTING_ITEM_BACK_TO_HOME_TIME,             /* [50] 返回主页时间 */
    HOOCH_PROTOCOL_SETTING_ITEM_ENTER_SCREENSAVER_TIME,        /* [51] 进入屏保时间 */
    HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_PASSWORD,               /* [52] 屏密码 */
    HOOCH_PROTOCOL_SETTING_ITEM_PAGE_KEY_LOADED_FUNCTION,      /* [53] 页面按键装载的功能 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_JOG_TIME,                  /* [54] 按键点动时间 */
    HOOCH_PROTOCOL_SETTING_ITEM_MASTER_SW,                     /* [55] 总开总关 */
    HOOCH_PROTOCOL_SETTING_ITEM_VIBRATION_SWITCH,              /* [56] 振动开关 */
    HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_COUNTDOWN,              /* [57] 人感倒计时 */
    HOOCH_PROTOCOL_SETTING_ITEM_MULTICAST_GROUP_ID,            /* [58] 组播ID */
    HOOCH_PROTOCOL_SETTING_ITEM_TIME,                          /* [59] 普通时间（时分秒） */
    HOOCH_PROTOCOL_SETTING_ITEM_DATE,                          /* [60] 日期 */
    HOOCH_PROTOCOL_SETTING_ITEM_LANGUAGE,                      /* [61] 语言选择 */
    HOOCH_PROTOCOL_SETTING_ITEM_CONFIG_STATUS,                 /* [62] 配置状态 */
    HOOCH_PROTOCOL_SETTING_ITEM_PANEL_UPDATE_STATUS,           /* [63] 面板更新状态 */
    HOOCH_PROTOCOL_SETTING_ITEM_VERSION,                       /* [64] 版本 */
    HOOCH_PROTOCOL_SETTING_ITEM_PROGRAMMING_MODE,              /* [65] 编程模式 */
    HOOCH_PROTOCOL_SETTING_ITEM_BLE_KNX_GET_CONFIG,               /* [66] BLE KNX获取配置 */
    HOOCH_PROTOCOL_SETTING_ITEM_BLE_KNX_GET_STATUS,               /* [67] BLE KNX获取状态 */
    HOOCH_PROTOCOL_SETTING_ITEM_MODULE_TYPE,                      /* [68] 模组类型 */
    HOOCH_PROTOCOL_SETTING_ITEM_INTERLOCK_FUNCTION,               /* [69] 互锁功能 */
    HOOCH_PROTOCOL_SETTING_ITEM_TEMPERATURE_UNIT,                 /* [70] 温度显示单位 */
    HOOCH_PROTOCOL_SETTING_ITEM_SOUND_ENABLE,                     /* [71] 声音使能 */
    HOOCH_PROTOCOL_SETTING_ITEM_KEY_BACKLIGHT_MODE,               /* [72] 按键背光模式[KNX] */
    HOOCH_PROTOCOL_SETTING_ITEM_FLOOR_HEATING_TEMPERATURE_MAX,    /* [73] 地暖温度上限 */
    HOOCH_PROTOCOL_SETTING_ITEM_FLOOR_HEATING_TEMPERATURE_MIN,    /* [74] 地暖温度下限 */
    HOOCH_PROTOCOL_SETTING_ITEM_WEATHER,                          /* [75] 天气 */
    HOOCH_PROTOCOL_SETTING_ITEM_NETWORK_STATUS,                    /* [76] 网络状态 */
    HOOCH_PROTOCOL_SETTING_ITEM_PAGE_ORDER,                        /* [77] 页面排序 (5byte "12345") */
    HOOCH_PROTOCOL_SETTING_ITEM_TEMPERATURE_CALIBRATION,           /* [78] 温度传感器矫正 (int8 -10~+10) */
    HOOCH_PROTOCOL_SETTING_ITEM_HUMIDITY_CALIBRATION,              /* [79] 湿度传感器矫正 (int8 -20~+20) */

} HOOCH_PROTOCOL_SettingItem_t;

/* 网络状态 (HOOCH_PROTOCOL_SETTING_ITEM_NETWORK_STATUS 的取值) */
typedef enum
{
    HOOCH_PROTOCOL_SETTING_NETWORK_STATUS_INVALID = 0xFFU,  /* 无效状态 */
    HOOCH_PROTOCOL_SETTING_NETWORK_STATUS_OFFLINE   = 0U,   /* 未入网 */
    HOOCH_PROTOCOL_SETTING_NETWORK_STATUS_JOINED    = 1U,   /* 已入网 */
    HOOCH_PROTOCOL_SETTING_NETWORK_STATUS_ERROR     = 2U,   /* 入网异常 */
    HOOCH_PROTOCOL_SETTING_NETWORK_STATUS_JOINING   = 3U,   /* 配网中 */
} HOOCH_PROTOCOL_SettingNetworkStatus_t;

/* 统一天气码 ([75] WEATHER 的取值; 数值与小米屏 WEATHER_CODE 线格式保持一致,
 * 各协议接入方把自家天气码转换到此枚举, 不对应的类型归入最近似项或 UNKNOWN) */
typedef enum
{
    HOOCH_PROTOCOL_SETTING_WEATHER_SUNNY                       = 0U,   /* 晴 */
    HOOCH_PROTOCOL_SETTING_WEATHER_CLOUDY                      = 1U,   /* 多云 */
    HOOCH_PROTOCOL_SETTING_WEATHER_OVERCAST                    = 2U,   /* 阴 */
    HOOCH_PROTOCOL_SETTING_WEATHER_SHOWER                      = 3U,   /* 阵雨 */
    HOOCH_PROTOCOL_SETTING_WEATHER_THUNDERSHOWER               = 4U,   /* 雷阵雨 */
    HOOCH_PROTOCOL_SETTING_WEATHER_THUNDERSHOWER_WITH_HAIL     = 5U,   /* 雷阵雨伴有冰雹 */
    HOOCH_PROTOCOL_SETTING_WEATHER_SLEET                       = 6U,   /* 雨夹雪 */
    HOOCH_PROTOCOL_SETTING_WEATHER_LIGHT_RAIN                  = 7U,   /* 小雨 */
    HOOCH_PROTOCOL_SETTING_WEATHER_MODERATE_RAIN               = 8U,   /* 中雨 */
    HOOCH_PROTOCOL_SETTING_WEATHER_HEAVY_RAIN                  = 9U,   /* 大雨 */
    HOOCH_PROTOCOL_SETTING_WEATHER_STORM                       = 10U,  /* 暴雨 */
    HOOCH_PROTOCOL_SETTING_WEATHER_HEAVY_STORM                 = 11U,  /* 大暴雨 */
    HOOCH_PROTOCOL_SETTING_WEATHER_SEVERE_STORM                = 12U,  /* 特大暴雨 */
    HOOCH_PROTOCOL_SETTING_WEATHER_SNOW_FLURRY                 = 13U,  /* 阵雪 */
    HOOCH_PROTOCOL_SETTING_WEATHER_LIGHT_SNOW                  = 14U,  /* 小雪 */
    HOOCH_PROTOCOL_SETTING_WEATHER_MODERATE_SNOW               = 15U,  /* 中雪 */
    HOOCH_PROTOCOL_SETTING_WEATHER_HEAVY_SNOW                  = 16U,  /* 大雪 */
    HOOCH_PROTOCOL_SETTING_WEATHER_SNOWSTORM                   = 17U,  /* 暴雪 */
    HOOCH_PROTOCOL_SETTING_WEATHER_FOGGY                       = 18U,  /* 雾 */
    HOOCH_PROTOCOL_SETTING_WEATHER_ICE_RAIN                    = 19U,  /* 冻雨 */
    HOOCH_PROTOCOL_SETTING_WEATHER_DUSTSTORM                   = 20U,  /* 沙尘暴 */
    HOOCH_PROTOCOL_SETTING_WEATHER_LIGHT_TO_MODERATE_RAIN      = 21U,  /* 小到中雨 */
    HOOCH_PROTOCOL_SETTING_WEATHER_MODERATE_TO_HEAVY_RAIN      = 22U,  /* 中到大雨 */
    HOOCH_PROTOCOL_SETTING_WEATHER_HEAVY_RAIN_TO_STORM         = 23U,  /* 大到暴雨 */
    HOOCH_PROTOCOL_SETTING_WEATHER_STORM_TO_HEAVY_STORM        = 24U,  /* 暴雨到大暴雨 */
    HOOCH_PROTOCOL_SETTING_WEATHER_HEAVY_TO_SEVERE_STORM       = 25U,  /* 大暴雨到特大暴雨 */
    HOOCH_PROTOCOL_SETTING_WEATHER_LIGHT_TO_MODERATE_SNOW      = 26U,  /* 小到中雪 */
    HOOCH_PROTOCOL_SETTING_WEATHER_MODERATE_TO_HEAVY_SNOW      = 27U,  /* 中到大雪 */
    HOOCH_PROTOCOL_SETTING_WEATHER_HEAVY_SNOW_TO_SNOWSTORM     = 28U,  /* 大到暴雪 */
    HOOCH_PROTOCOL_SETTING_WEATHER_DUST                        = 29U,  /* 浮尘 */
    HOOCH_PROTOCOL_SETTING_WEATHER_SAND                        = 30U,  /* 扬沙 */
    HOOCH_PROTOCOL_SETTING_WEATHER_SANDSTORM                   = 31U,  /* 强沙尘暴 */
    HOOCH_PROTOCOL_SETTING_WEATHER_DENSE_FOGGY                 = 32U,  /* 浓雾 */
    HOOCH_PROTOCOL_SETTING_WEATHER_SNOW                        = 33U,  /* 雪 */
    HOOCH_PROTOCOL_SETTING_WEATHER_HEAVY_DENSE_FOG             = 49U,  /* 强浓雾 */
    HOOCH_PROTOCOL_SETTING_WEATHER_HAZE                        = 53U,  /* 霾 */
    HOOCH_PROTOCOL_SETTING_WEATHER_MODERATE_HAZE               = 54U,  /* 中度霾 */
    HOOCH_PROTOCOL_SETTING_WEATHER_SEVERE_HAZE                 = 55U,  /* 重度霾 */
    HOOCH_PROTOCOL_SETTING_WEATHER_HAZARDOUS_HAZE              = 56U,  /* 危险霾 */
    HOOCH_PROTOCOL_SETTING_WEATHER_HEAVY_FOG                   = 57U,  /* 大雾 */
    HOOCH_PROTOCOL_SETTING_WEATHER_EXTRA_HEAVY_DENSE_FOG       = 58U,  /* 超强浓雾 */
    HOOCH_PROTOCOL_SETTING_WEATHER_RAIN                        = 301U, /* 雨 */
    HOOCH_PROTOCOL_SETTING_WEATHER_UNKNOWN                     = 99U,  /* 未知 */
} HOOCH_PROTOCOL_SettingWeatherCode_t;




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
