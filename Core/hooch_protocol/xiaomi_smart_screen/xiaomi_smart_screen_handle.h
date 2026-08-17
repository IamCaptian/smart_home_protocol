#ifndef __XIAOMI_QUAN_HANDLE_H__
#define __XIAOMI_QUAN_HANDLE_H__

#include "xiaomi_smart_screen_circular_bufferc.h"
#include "user_printf.h"

/*主命令*/
typedef enum
{
    XIAOMI_SMART_SCREEN_NET_STATUS_CONTROL = 0x81,  /*配网状态控制*/
    XIAOMI_SMART_SCREEN_RESET_CONTROL = 0x83,  /*重置模块控制*/
    XIAOMI_SMART_SCREEN_SET_PRODUCTION_CONTROL = 0x84,  /*设置产测模式*/
    XIAOMI_SMART_SCREEN_PRODUCTION_STATUS_CONTROL = 0x85,  /*上报产测状态*/
    XIAOMI_SMART_SCREEN_CUSTOM_CONFIG_CONTROL = 0x88,  /*设备自定义配置*/
} xiaomi_screen_cmd_t;



/*子命令-消息类型*/
typedef enum
{
    XIAOMI_SMART_SCREEN_SUBCMD_READ =  0x00,  /*读取*/
    XIAOMI_SMART_SCREEN_SUBCMD_SET =  0x01,  /*设置*/
    XIAOMI_SMART_SCREEN_SUBCMD_REPORT =  0x02,  /*上报*/
} xiaomi_screen_msg_type_t;

/*子命令-功能命令*/
typedef enum
{
    XIAOMI_SMART_SCREEN_SUBCMD_DEFAULT =  0x01,  /*默认命令*/
} xiaomi_screen_func_cmd_t;

/*子命令*/
typedef enum
{
    XIAOMI_SMART_SCREEN_SUBCMD_SWITCH_CONTROL =  0x01,  /*开关控制*/
    XIAOMI_SMART_SCREEN_SUBCMD_EVENT_STATUS =  0x02,  /*事件状态*/
    XIAOMI_SMART_SCREEN_SUBCMD_RELAY_KEY_FUNCTION_CONFIG =  0x03,  /*继电器、按键功能设置*/
    XIAOMI_SMART_SCREEN_SUBCMD_KEY_NAME_INFO =  0x04,  /*按键名称信息*/
    XIAOMI_SMART_SCREEN_SUBCMD_ENTER_PAIRING_CLEAR_INTERLOCK =  0x05,  /*进入对码、清码、互控*/
    XIAOMI_SMART_SCREEN_SUBCMD_PAIRING_CLEAR_INTERLOCK_STATUS =  0x06,  /*对码、清码、互控状态*/
    XIAOMI_SMART_SCREEN_SUBCMD_MOMENTARY_FUNCTION =  0x07,  /*点动功能*/
    XIAOMI_SMART_SCREEN_SUBCMD_INTERLOCK_FUNCTION =  0x08,  /*互锁功能*/
    XIAOMI_SMART_SCREEN_SUBCMD_FUNCTION_CONFIG =  0x09,  /*功能设置*/
    XIAOMI_SMART_SCREEN_SUBCMD_SENSOR_STATUS_REPORT =  0x0A,  /*感应状态上传*/
    XIAOMI_SMART_SCREEN_SUBCMD_BACKLIGHT_BRIGHTNESS_CONTROL =  0x0B,  /*背光灯亮度调节*/
    XIAOMI_SMART_SCREEN_SUBCMD_SWITCH_CONTROL_EXT =  0x0C,  /*开关控制*/
    XIAOMI_SMART_SCREEN_SUBCMD_CLEAR_ALL_REMOTE_CONTROL_FUNCTIONS =  0x0D,  /*清 除 所 有 遥 控器功能*/
    XIAOMI_SMART_SCREEN_SUBCMD_DIMMING_COLOR_CURTAIN_SWITCH_STATUS =  0x0E,  /*调光/调色/窗帘开关状态*/
    XIAOMI_SMART_SCREEN_SUBCMD_BRIGHTNESS_COLOR_TEMP_TRAVEL_STATUS =  0x0F,  /*亮度/色温/行程状态*/
    XIAOMI_SMART_SCREEN_SUBCMD_UTC_TIME_CONTROL =  0x10,  /*UTC 时间控制*/
    XIAOMI_SMART_SCREEN_SUBCMD_TIME_ZONE_CONFIG =  0x11,  /*设备时区设置*/
    XIAOMI_SMART_SCREEN_SUBCMD_WEATHER_CODE =  0x12,  /*天气码*/
    XIAOMI_SMART_SCREEN_SUBCMD_SCREEN_BRIGHTNESS_CONTROL =  0x13,  /*屏幕亮度调节*/
    XIAOMI_SMART_SCREEN_SUBCMD_SCREEN_OFF_MODE_CONTROL =  0x14,  /*熄屏模式控制*/
    XIAOMI_SMART_SCREEN_SUBCMD_RELAY_CONFIG =  0x15,  /*继电器配置*/
    XIAOMI_SMART_SCREEN_SUBCMD_MCU_VERSION_REPORT =  0x18,  /*MCU 版本上报*/
    XIAOMI_SMART_SCREEN_SUBCMD_DIMMER_DOUBLE_TAP_SWITCH_SETTING =  0x19,  /*调光模式双击切换下发*/
    XIAOMI_SMART_SCREEN_SUBCMD_SCREEN_SAVER =  0x1A,    /*屏保*/
    XIAOMI_SMART_SCREEN_SUBCMD_AVAILABLE_PAGE =  0x1B, /*默认页面*/ 
    XIAOMI_SMART_SCREEN_SUBCMD_TEXT_MESSAGE_DISPLAY_SWITCH =  0x1C,  /*文字留言显示开关*/
    XIAOMI_SMART_SCREEN_SUBCMD_SPEED_MODE_SWITCH =  0x1D,  /*极速模式开关*/
    XIAOMI_SMART_SCREEN_SUBCMD_SCENE_NAME_INFO =  0x1E,  /*场景名称信息*/
    XIAOMI_SMART_SCREEN_SUBCMD_LIGHT_NAME_INFO =  0x1F,  /*灯光名称信息*/
    XIAOMI_SMART_SCREEN_SUBCMD_CURTAIN_NAME_INFO =  0x20,  /*窗帘名称信息*/
    XIAOMI_SMART_SCREEN_SUBCMD_AC_NAME_INFO =  0x21,  /*空调名称信息*/
    XIAOMI_SMART_SCREEN_SUBCMD_FRESH_AIR_NAME_INFO =  0x22,  /*新风名称信息*/
    XIAOMI_SMART_SCREEN_SUBCMD_FLOOR_HEATING_NAME_INFO =  0x23,   /*地暖名称信息*/
    XIAOMI_SMART_SCREEN_SUBCMD_AC_STATUS =  0x24, /*空调开、关/温度/模式/风速状态*/
    XIAOMI_SMART_SCREEN_SUBCMD_FRESH_AIR_STATUS =  0x25,  /*新风开、关/模式/风速状态*/
    XIAOMI_SMART_SCREEN_SUBCMD_FLOOR_HEATING_STATUS =  0x26,   /*地暖开、关/模式/温度设置/温度上报*/
    XIAOMI_SMART_SCREEN_SUBCMD_CURRENT_PAGE_READ_SETTING =  0x27,  /*设置读取当前页面*/
    XIAOMI_SMART_SCREEN_SUBCMD_PAGE_DISPLAY_VISIBILITY =  0x28, /*显示/隐藏页面*/
    XIAOMI_SMART_SCREEN_SUBCMD_KEY_EVENT_STATUS =  0x29,  /*按键事件状态*/
    XIAOMI_SMART_SCREEN_SUBCMD_MULTI_FUNCTION_STATUS_CONFIG =  0x2A,  /*调光/窗帘/手势、感 应 功 能 开 关状态/字体大小/锁 定 无 线 开 关模式/手势传感器灵敏度*/
    XIAOMI_SMART_SCREEN_SUBCMD_TEXT_MESSAGE_DELIVERY =  0x2B,  /*文字留言下发*/
    XIAOMI_SMART_SCREEN_SUBCMD_BRIGHTNESS_COLOR_TEMP_TRAVEL_STATUS_EXT =  0x2C,  /*亮度/色温/行程状态*/
    XIAOMI_SMART_SCREEN_SUBCMD_PAIRING_CLEAR_STATUS =  0x2D,  /*对码、清码*/
    XIAOMI_SMART_SCREEN_SUBCMD_INDICATOR_COLOR_CONFIG =  0x2E,  /*指示灯颜色设置*/
    XIAOMI_SMART_SCREEN_SUBCMD_BACKLIGHT_COLOR_CONFIG =  0x2F,  /*背光灯颜色设置*/
    XIAOMI_SMART_SCREEN_SUBCMD_RADAR_SENSITIVITY_CONFIG =  0x30,  /*雷达灵敏度设置*/
    XIAOMI_SMART_SCREEN_SUBCMD_FREE_DEFINE =  0xFF,  /*自由定义*/

} xiaomi_screen_subcmd_t;

/*自由定义功能*/
typedef enum
{
    XIAOMI_SMART_SCREEN_FREE_DEFINE_MESSAGE_BOARD  = 0x00,  /*留言板功能*/
    XIAOMI_SMART_SCREEN_FREE_DEFINE_LOCAL_SCENE    = 0x01,  /*本地情景*/
    XIAOMI_SMART_SCREEN_FREE_DEFINE_VIBRATION      = 0x02,  /*振动功能*/
    XIAOMI_SMART_SCREEN_FREE_DEFINE_AC_PARAM       = 0x03,  /*空调参数*/
} xiaomi_screen_free_define_t;



/*事件状态*/
typedef enum
{
    XIAOMI_SMART_SCREEN_EVENT_SINGLE_CLICK = 0,          /*单击*/
    XIAOMI_SMART_SCREEN_EVENT_DOUBLE_CLICK = 1,          /*双击*/
    XIAOMI_SMART_SCREEN_EVENT_LONG_CLICK = 2,            /*长按*/
    XIAOMI_SMART_SCREEN_EVENT_LONG_CLICK_1P5S = 3,       /*长按1.5秒，转遥控用*/
    XIAOMI_SMART_SCREEN_EVENT_DYNAMIC_SINGLE_CLICK = 4,  /*灵动单击事件*/
    XIAOMI_SMART_SCREEN_EVENT_DYNAMIC_DOUBLE_CLICK = 5,  /*灵动双击事件*/
    XIAOMI_SMART_SCREEN_EVENT_DYNAMIC_LONG_CLICK = 6,    /*灵动长按事件*/
    XIAOMI_SMART_SCREEN_EVENT_REMOTE_SINGLE_CLICK = 7,   /*遥控单击事件*/
    XIAOMI_SMART_SCREEN_EVENT_REMOTE_DOUBLE_CLICK = 8,   /*遥控双击事件*/
    XIAOMI_SMART_SCREEN_EVENT_REMOTE_LONG_CLICK = 9,     /*遥控长按事件*/
    XIAOMI_SMART_SCREEN_EVENT_MOMENTARY_SINGLE_CLICK = 10, /*点动单击事件*/
    XIAOMI_SMART_SCREEN_EVENT_MOMENTARY_DOUBLE_CLICK = 11, /*点动双击事件*/
    XIAOMI_SMART_SCREEN_EVENT_MOMENTARY_LONG_CLICK = 12,   /*点动长按事件*/
} xiaomi_screen_event_status_t;

/*开关路数*/
typedef enum
{
    XIAOMI_SMART_SCREEN_SWITCH_ALL = 0,  /*总控*/
    XIAOMI_SMART_SCREEN_SWITCH_1 = 1,    /*开关1路*/
    XIAOMI_SMART_SCREEN_SWITCH_2 = 2,    /*开关2路*/
    XIAOMI_SMART_SCREEN_SWITCH_3 = 3,    /*开关3路*/
    XIAOMI_SMART_SCREEN_SWITCH_4 = 4,    /*开关4路*/
    XIAOMI_SMART_SCREEN_SWITCH_5 = 5,    /*开关5路*/
    XIAOMI_SMART_SCREEN_SWITCH_6 = 6,    /*开关6路*/
    XIAOMI_SMART_SCREEN_SWITCH_7 = 7,    /*开关7路*/
    XIAOMI_SMART_SCREEN_SWITCH_8 = 8,    /*开关8路*/
} xiaomi_screen_switch_channel_t;

/*上电状态*/
typedef enum
{
    XIAOMI_SMART_SCREEN_POWER_ON_MEMORY = 0,  /*断电记忆*/
    XIAOMI_SMART_SCREEN_POWER_ON_OFF = 1,     /*上电关闭*/
    XIAOMI_SMART_SCREEN_POWER_ON_ON = 2,      /*上电打开*/
} xiaomi_screen_power_on_status_t;

/*按键类型*/
typedef enum
{
    XIAOMI_SMART_SCREEN_KEY_TYPE_WIRED_WIRELESS_SWITCH = 0,  /*有线和无线开关*/
    XIAOMI_SMART_SCREEN_KEY_TYPE_WIRELESS_SWITCH = 1,        /*无线开关*/
    XIAOMI_SMART_SCREEN_KEY_TYPE_DYNAMIC_SWITCH = 2,         /*灵动开关*/
    XIAOMI_SMART_SCREEN_KEY_TYPE_WIRELESS_REMOTE_SWITCH = 3, /*无线遥控开关*/
    XIAOMI_SMART_SCREEN_KEY_TYPE_DIMMER_SWITCH = 4,          /*调光/调色开关*/
    XIAOMI_SMART_SCREEN_KEY_TYPE_CURTAIN_SWITCH = 5,         /*窗帘开关*/
    XIAOMI_SMART_SCREEN_KEY_TYPE_LOCK_SWITCH = 6,            /*锁定开关*/
    XIAOMI_SMART_SCREEN_KEY_TYPE_MOMENTARY_SWITCH = 7,       /*点动开关*/
} xiaomi_screen_key_type_t;

/*色温、亮度、行程状态*/
typedef enum
{
    XIAOMI_SMART_SCREEN_STATUS_BRIGHTNESS = 1,              /*亮度*/
    XIAOMI_SMART_SCREEN_STATUS_COLOR_TEMP = 2,              /*色温*/
    XIAOMI_SMART_SCREEN_STATUS_TRAVEL = 3,                  /*行程*/
    XIAOMI_SMART_SCREEN_STATUS_CURTAIN_OPEN_PERCENT = 4,    /*窗帘开百分比*/
    XIAOMI_SMART_SCREEN_STATUS_CURTAIN_CLOSE_PERCENT = 5,   /*窗帘关百分比*/
} xiaomi_screen_brightness_color_temp_travel_status_t;

/*熄屏模式*/
typedef enum
{
    XIAOMI_SMART_SCREEN_OFF_MODE_CLOCK_SCREEN = 1,      /*Clock Screen*/
    XIAOMI_SMART_SCREEN_OFF_MODE_OFF_SCREEN = 2,        /*Off Screen*/
    XIAOMI_SMART_SCREEN_OFF_MODE_MICRO_BRIGHT = 3,      /*Micro Bright*/
    XIAOMI_SMART_SCREEN_OFF_MODE_HALF_BRIGHT = 4,       /*Half Bright*/
    XIAOMI_SMART_SCREEN_OFF_MODE_STEADY_ON = 5,         /*Steady On*/
    XIAOMI_SMART_SCREEN_OFF_MODE_RANDOM = 6,            /*Random*/
    XIAOMI_SMART_SCREEN_OFF_MODE_STARDUST_PAINT = 7,    /*Stardust Paint*/
    XIAOMI_SMART_SCREEN_OFF_MODE_DYNAMIC_LEAP = 8,      /*Dynamic Leap*/
    XIAOMI_SMART_SCREEN_OFF_MODE_DEERLIGHT = 9,         /*Deerlight*/
    XIAOMI_SMART_SCREEN_OFF_MODE_RHINOCEROS = 10,       /*Rhinoceros*/
    XIAOMI_SMART_SCREEN_OFF_MODE_REALM_OF_DAWN = 11,    /*Realm Of Dawn*/
    XIAOMI_SMART_SCREEN_OFF_MODE_HEART_LAKE_MOON = 12,  /*Heart Lake Moon*/
    XIAOMI_SMART_SCREEN_OFF_MODE_XINYU_ASTRONAUT = 13,  /*Xinyu Astronaut*/
    XIAOMI_SMART_SCREEN_OFF_MODE_AZURE_FOAM = 14,       /*Azure Foam*/
} xiaomi_screen_off_mode_t;

/*天气码*/
typedef enum
{
    WEA_SUNNY = 0,                        /*晴*/
    WEA_CLOUDY = 1,                       /*多云*/
    WEA_OVERCAST = 2,                     /*阴*/
    WEA_SHOWER = 3,                       /*阵雨*/
    WEA_THUNDERSHOWER = 4,                /*雷阵雨*/
    WEA_THUNDERSHOWER_WITH_HAIL = 5,      /*雷阵雨伴有冰雹*/
    WEA_SLEET = 6,                        /*雨夹雪*/
    WEA_LIGHT_RAIN = 7,                   /*小雨*/
    WEA_MODERATE_RAIN = 8,                /*中雨*/
    WEA_HEAVY_RAIN = 9,                   /*大雨*/
    WEA_STORM = 10,                       /*暴雨*/
    WEA_HEAVY_STORM = 11,                 /*大暴雨*/
    WEA_SEVERE_STORM = 12,                /*特大暴雨*/
    WEA_SNOW_FLURRY = 13,                 /*阵雪*/
    WEA_LIGHT_SNOW = 14,                  /*小雪*/
    WEA_MODERATE_SNOW = 15,               /*中雪*/
    WEA_HEAVY_SNOW = 16,                  /*大雪*/
    WEA_SNOWSTORM = 17,                   /*暴雪*/
    WEA_FOGGY = 18,                       /*雾*/
    WEA_ICE_RAIN = 19,                    /*冻雨*/
    WEA_DUSTSTORM = 20,                   /*沙尘暴*/
    WEA_LIGHT_TO_MODERATE_RAIN = 21,      /*小到中雨*/
    WEA_MODERATE_TO_HEAVY_RAIN = 22,      /*中到大雨*/
    WEA_HEAVY_RAIN_TO_STORM = 23,         /*大到暴雨*/
    WEA_STORM_TO_HEAVY_STORM = 24,        /*暴雨到大暴雨*/
    WEA_HEAVY_TO_SEVERE_STORM = 25,       /*大暴雨到特大暴雨*/
    WEA_LIGHT_TO_MODERATE_SNOW = 26,      /*小到中雪*/
    WEA_MODERATE_TO_HEAVY_SNOW = 27,      /*中到大雪*/
    WEA_HEAVY_SNOW_TO_SNOWSTORM = 28,     /*大到暴雪*/
    WEA_DUST = 29,                        /*浮尘*/
    WEA_SAND = 30,                        /*扬沙*/
    WEA_SANDSTORM = 31,                   /*强沙尘暴*/
    WEA_DENSE_FOGGY = 32,                 /*浓雾*/
    WEA_SNOW = 33,                        /*雪*/
    WEA_HEAVY_DENSE_FOG = 49,             /*强浓雾*/
    WEA_HAZE = 53,                        /*霾*/
    WEA_MODERATE_HAZE = 54,               /*中度霾*/
    WEA_SEVERE_HAZE = 55,                 /*重度霾*/
    WEA_HAZARDOUS_HAZE = 56,              /*危险霾*/
    WEA_HEAVY_FOG = 57,                   /*大雾*/
    WEA_EXTRA_HEAVY_DENSE_FOG = 58,       /*超强浓雾*/
    WEA_RAIN = 301,                       /*雨*/
    WEA_UNKNOWN = 99,                     /*未知*/
} xiaomi_screen_weather_code_t;

/*空调模式*/
typedef enum
{
    XIAOMI_SMART_SCREEN_AC_MODE_COOL = 1,    /*制冷*/
    XIAOMI_SMART_SCREEN_AC_MODE_FAN = 2,     /*送风*/
    XIAOMI_SMART_SCREEN_AC_MODE_DRY = 3,     /*除湿*/
    XIAOMI_SMART_SCREEN_AC_MODE_HEAT = 4,    /*制热*/
    XIAOMI_SMART_SCREEN_AC_MODE_AUTO = 5,    /*自动*/
} xiaomi_screen_ac_mode_t;

/* 按键点动控制帧 (Len=5 Byte) */
typedef struct
{
    uint8_t  channel;       /* Byte[0]: 0=总控, 1~8=各路 */
    uint8_t  switch_;       /* Byte[1]: 点动开关, 0=关闭, 1=打开 */
    uint16_t duration;      /* Byte[2-3]: 点动时间, 1~3600 单位:秒 */
    uint8_t  state;         /* Byte[4]: 点动状态, 0=关闭, 1=开启 */
} xiaomi_screen_jog_ctrl_t;

void xiaomi_smart_screen_handle_frame(Frame_t *frame);
void xiaomi_smart_screen_reply_no_param_frame(unsigned char command);
void xiaomi_smart_screen_reply_frame(Frame_t *frame);
void xiaomi_smart_screen_report_frame(Frame_t *frame);
static void xiaomi_smart_screen_handle_custom_config(Frame_t *frame);
static void xiaomi_smart_screen_handle_custom_config_2(Frame_t *frame);
static void xiaomi_smart_screen_handle_custom_config_3(Frame_t *frame);
uint8_t xiaoni_smart_screen_send_report_status(xiaomi_screen_subcmd_t dp);
void xiaoni_smart_screen_switch_control_update(uint8_t switch_bits);

#endif
