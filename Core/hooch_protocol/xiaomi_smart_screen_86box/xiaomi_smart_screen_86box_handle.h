#ifndef __XIAOMI_SMART_SCREEN_86BOX_HANDLE_H__
#define __XIAOMI_SMART_SCREEN_86BOX_HANDLE_H__

#include "xiaomi_smart_screen_86box_buffer.h"
#include "hooch_setting.h"

/* 主命令 */
typedef enum
{
    XIAOMI_86BOX_CMD_DEVICE_CONTROL        = 0x44,  /* 设备控制/设置/获取 */
    XIAOMI_86BOX_CMD_PAIRING_CONTROL       = 0x81,  /* WiFi/Mesh 配对设置 */
    XIAOMI_86BOX_CMD_PAIRING_STATUS        = 0x82,  /* WiFi/Mesh 上报配对状态 */
    XIAOMI_86BOX_CMD_RESET_CONTROL         = 0x83,  /* 重置模块 */
    XIAOMI_86BOX_CMD_PRODUCTION_TEST       = 0x84,  /* 设置产测 */
    XIAOMI_86BOX_CMD_PRODUCTION_STATUS     = 0x85,  /* 上报产测状态 */
    XIAOMI_86BOX_CMD_RESET_STATUS          = 0x86,  /* 上报重置状态 */
} xiaomi_86box_command_t;

/* 设备类型(参数2), 见协议附表三 */
typedef enum
{
    XIAOMI_86BOX_DEV_WY_LIGHT          = 0x01,  /* WY 灯 */
    XIAOMI_86BOX_DEV_RGB_LIGHT         = 0x03,  /* RGB 灯 */
    XIAOMI_86BOX_DEV_SINGLE_LIGHT      = 0x05,  /* 单色灯 */
    XIAOMI_86BOX_DEV_WY_RGB_LIGHT      = 0x08,  /* WY_RGB 灯 */
    XIAOMI_86BOX_DEV_FAN               = 0x16,  /* 风扇 */
    XIAOMI_86BOX_DEV_AIR_CONDITIONER   = 0x21,  /* 空调 */
    XIAOMI_86BOX_DEV_SCENE_SWITCH      = 0x22,  /* 情景开关 */
    XIAOMI_86BOX_DEV_FRESH_AIR         = 0x23,  /* 新风 */
    XIAOMI_86BOX_DEV_FLOOR_HEATING     = 0x24,  /* 地暖 */
    XIAOMI_86BOX_DEV_BATH_HEATER       = 0x25,  /* 浴霸 */
    XIAOMI_86BOX_DEV_MUSIC             = 0x26,  /* 音乐 */
    XIAOMI_86BOX_DEV_86PIR             = 0xA3,  /* 86 人存在式感应器 */
    XIAOMI_86BOX_DEV_CURTAIN           = 0xB0,  /* 窗帘 */
    XIAOMI_86BOX_DEV_RELAY_SWITCH      = 0xB1,  /* 智能开关(继电器) */
    XIAOMI_86BOX_DEV_86_SWITCH         = 0xB2,  /* 86 面板开关(无线) */
    XIAOMI_86BOX_DEV_INDOOR_SENSOR     = 0xC2,  /* 室内传感器 */
    XIAOMI_86BOX_DEV_BLE_BASIC         = 0xFF,  /* BLE 基本资料 */
} xiaomi_86box_device_type_t;

/* 0x44 帧的设备头: 上报类型 | 设备类型 | 设备ID | 序号ID */
typedef struct
{
    uint8_t report_type;    /* 参数1: 0=本地设备, 1=米家设备 */
    uint8_t device_type;    /* 参数2 */
    uint8_t device_id;      /* 参数3 */
    uint8_t seq_id;         /* 参数4: 屏幕上设备排列顺序(遥控分组) */
} xiaomi_86box_device_header_t;

/* 所有设备通用子命令 */
typedef enum
{
    XIAOMI_86BOX_SUBCMD_REPORT_ALL     = 0x41,  /* 上报所有状态 */
    XIAOMI_86BOX_SUBCMD_GET_ALL        = 0x43,  /* 获取所有状态 */
    XIAOMI_86BOX_SUBCMD_REQUEST_NAME   = 0x44,  /* 请求名称信息 */
    XIAOMI_86BOX_SUBCMD_REPORT_NAME    = 0x45,  /* 上报名称信息 */
} xiaomi_86box_common_subcmd_t;

/* 灯类设备子命令 */
typedef enum
{
    XIAOMI_86BOX_LIGHT_SWITCH          = 0x01,  /* 0-关 1-开 2-切换 */
    XIAOMI_86BOX_LIGHT_BRIGHTNESS      = 0x02,  /* 亮度 1-100 */
    XIAOMI_86BOX_LIGHT_COLOR_TEMP      = 0x03,  /* 色温 2 字节, 高字节在前 */
    XIAOMI_86BOX_LIGHT_RGB             = 0x04,  /* R/G/B 三字节 */
    XIAOMI_86BOX_LIGHT_PAIRING         = 0x05,  /* 0-清码 1-对码 */
    XIAOMI_86BOX_LIGHT_EVENT           = 0x06,  /* 情景编号 + 单击/双击/长按 */
    XIAOMI_86BOX_LIGHT_POWER_ON        = 0x07,  /* 上电状态 */
} xiaomi_86box_light_subcmd_t;

/* 开关类设备子命令 */
typedef enum
{
    XIAOMI_86BOX_SWITCH_SWITCH         = 0x01,  /* 单路开关 */
    XIAOMI_86BOX_SWITCH_CHANNEL        = 0x02,  /* 各路开关: 通道 + 状态位图 */
    XIAOMI_86BOX_SWITCH_SETTING        = 0x03,  /* 上电状态 + 开关模式 */
    XIAOMI_86BOX_SWITCH_PAIRING        = 0x05,  /* 通道 + 对码/清码 */
    XIAOMI_86BOX_SWITCH_INTERLOCK      = 0x06,  /* 0-解控 1-互控 2-取消互控 */
    XIAOMI_86BOX_SWITCH_EVENT          = 0x07,  /* 单击/双击/长按 */
    XIAOMI_86BOX_SWITCH_INTERLOCK_STAT = 0x08,  /* 上报互控状态 */
} xiaomi_86box_switch_subcmd_t;

/* 窗帘设备子命令 */
typedef enum
{
    XIAOMI_86BOX_CURTAIN_SWITCH        = 0x01,  /* 0-打开 1-暂停 2-关闭 */
    XIAOMI_86BOX_CURTAIN_TRAVEL        = 0x02,  /* 行程百分比 */
    XIAOMI_86BOX_CURTAIN_PAIRING       = 0x03,  /* 0-清码 1-对码 */
} xiaomi_86box_curtain_subcmd_t;

/* 空调设备子命令 */
typedef enum
{
    XIAOMI_86BOX_AC_POWER              = 0x01,  /* 开关 */
    XIAOMI_86BOX_AC_MODE               = 0x02,  /* 1-制冷 2-送风 3-除湿 4-制热 */
    XIAOMI_86BOX_AC_FAN_SPEED          = 0x03,  /* 1-自动 2-低风 3-中风 4-高风 */
    XIAOMI_86BOX_AC_TEMPERATURE        = 0x04,  /* 温度设定 */
    XIAOMI_86BOX_AC_PAIRING            = 0x05,  /* 0-清码 1-对码 */
    XIAOMI_86BOX_AC_INTERLOCK          = 0x06,  /* 0-解控 1-互控 2-取消互控 */
    XIAOMI_86BOX_AC_INTERLOCK_STATUS   = 0x07,  /* 上报互控状态 */
} xiaomi_86box_ac_subcmd_t;

/* 新风设备子命令 */
typedef enum
{
    XIAOMI_86BOX_FRESH_AIR_POWER       = 0x01,  /* 开关 */
    XIAOMI_86BOX_FRESH_AIR_FAN_SPEED   = 0x02,  /* 1-自动 2-低风 3-中风 4-高风 */
    XIAOMI_86BOX_FRESH_AIR_INTERLOCK   = 0x03,  /* 0-解控 1-互控 2-取消互控 */
    XIAOMI_86BOX_FRESH_AIR_INTERLOCK_STATUS = 0x04, /* 上报互控状态 */
    XIAOMI_86BOX_FRESH_AIR_MODE        = 0x05,  /* 0-送风 1-排风 */
} xiaomi_86box_fresh_air_subcmd_t;

/* 地暖设备子命令 */
typedef enum
{
    XIAOMI_86BOX_FLOOR_HEATING_POWER   = 0x01,  /* 开关 */
    XIAOMI_86BOX_FLOOR_HEATING_TEMP    = 0x02,  /* 温度设定 */
    XIAOMI_86BOX_FLOOR_HEATING_INTERLOCK = 0x03,/* 0-解控 1-互控 2-取消互控 */
    XIAOMI_86BOX_FLOOR_HEATING_INTERLOCK_STATUS = 0x04, /* 上报互控状态 */
} xiaomi_86box_floor_heating_subcmd_t;

/* 情景开关设备子命令 */
typedef enum
{
    XIAOMI_86BOX_SCENE_SWITCH_STATE    = 0x01,  /* 情景序号 + 开关状态 */
    XIAOMI_86BOX_SCENE_SWITCH_EVENT    = 0x07,  /* 单击/双击/长按 */
} xiaomi_86box_scene_switch_subcmd_t;

/* 人存在雷达传感器子命令 */
typedef enum
{
    XIAOMI_86BOX_RADAR_PARAM_SET       = 0x01,  /* 功能类型 + 参数值 */
    XIAOMI_86BOX_RADAR_PARAM_GET       = 0x02,  /* 功能类型 */
    XIAOMI_86BOX_RADAR_STATUS_REPORT   = 0x03,  /* 0-无感应 1-触发无人 2-触发有人 */
} xiaomi_86box_radar_subcmd_t;

/* BLE 基本资料子命令 */
typedef enum
{
    XIAOMI_86BOX_BLE_TEMP_HUMIDITY     = 0x01,  /* 温度 + 湿度 */
    XIAOMI_86BOX_BLE_PRODUCT_ID        = 0x02,  /* 产品 ID */
    XIAOMI_86BOX_BLE_TIME_REPORT       = 0x03,  /* 上报时间戳 */
    XIAOMI_86BOX_BLE_TIME_GET          = 0x04,  /* 获取时间戳 */
    XIAOMI_86BOX_BLE_CODE_LIBRARY      = 0x05,  /* 设置码库 */
    XIAOMI_86BOX_BLE_BACKLIGHT_SET     = 0x06,  /* 设置背光灯 */
    XIAOMI_86BOX_BLE_BACKLIGHT_GET     = 0x07,  /* 获取背光灯 */
    XIAOMI_86BOX_BLE_BACKLIGHT_REPORT  = 0x08,  /* 上报背光灯 */
    XIAOMI_86BOX_BLE_MODEL_PID_SET     = 0x09,  /* 设置 model-pid */
    XIAOMI_86BOX_BLE_MODEL_PID_GET     = 0x0A,  /* 获取 model-pid */
    XIAOMI_86BOX_BLE_WEATHER_GET       = 0x0B,  /* 获取天气 */
    XIAOMI_86BOX_BLE_CHILD_LOCK_SET    = 0x0C,  /* 设置童锁 */
    XIAOMI_86BOX_BLE_CHILD_LOCK_GET    = 0x0D,  /* 获取童锁 */
    XIAOMI_86BOX_BLE_SCREEN_PARAM_SET  = 0x0E,  /* 屏显参数设置 */
    XIAOMI_86BOX_BLE_SCREEN_PARAM_GET  = 0x0F,  /* 屏显参数获取 */
    XIAOMI_86BOX_BLE_FAULT_REPORT      = 0x10,  /* 上报设备故障 */
    XIAOMI_86BOX_BLE_WEATHER_REPORT    = 0x11,  /* 上报天气 */
    XIAOMI_86BOX_BLE_BUZZER_SET        = 0x12,  /* 设置蜂鸣器 */
    XIAOMI_86BOX_BLE_BUZZER_GET        = 0x13,  /* 获取蜂鸣器 */
    XIAOMI_86BOX_BLE_PAGE_HIDE_SET     = 0x14,  /* 设置屏显隐藏参数 */
    XIAOMI_86BOX_BLE_PAGE_HIDE_GET     = 0x15,  /* 获取屏显隐藏参数 */
    XIAOMI_86BOX_BLE_PAGE_TYPE_SET     = 0x16,  /* 三合一当前工作界面类型 */
    XIAOMI_86BOX_BLE_PAGE_TYPE_REPORT  = 0x17,  /* 上报三合一工作界面 */
    XIAOMI_86BOX_BLE_BACKLIGHT_SWITCH  = 0x18,  /* 三合一背光开关 */
    XIAOMI_86BOX_BLE_BRIGHTNESS_SET    = 0x1A,  /* 三合一背光亮度 */
    XIAOMI_86BOX_BLE_SCREEN_BRIGHTNESS = 0x1C,  /* 三合一屏幕亮度 */
    XIAOMI_86BOX_BLE_SCREEN_OFF_TIME   = 0x1E,  /* 三合一熄屏时间 */
    XIAOMI_86BOX_BLE_SCREEN_OFF_EFFECT = 0x20,  /* 三合一熄屏效果 */
} xiaomi_86box_ble_subcmd_t;

/* 缓存字段: 用于应答 0x43 获取所有状态 */
typedef enum
{
    XIAOMI_86BOX_CACHE_SWITCH = 0,      /* 开关状态 */
    XIAOMI_86BOX_CACHE_BRIGHTNESS,      /* 亮度 */
    XIAOMI_86BOX_CACHE_COLOR_TEMP,      /* 色温 */
    XIAOMI_86BOX_CACHE_PERCENT,         /* 百分比/行程 */
    XIAOMI_86BOX_CACHE_SET_TRAVEL,      /* 设置行程 */
    XIAOMI_86BOX_CACHE_CURRENT_TRAVEL,  /* 当前行程 */
    XIAOMI_86BOX_CACHE_POWER,           /* 电源 */
    XIAOMI_86BOX_CACHE_MODE,            /* 模式 */
    XIAOMI_86BOX_CACHE_FAN_SPEED,       /* 风速 */
    XIAOMI_86BOX_CACHE_TEMPERATURE,     /* 温度 */
    XIAOMI_86BOX_CACHE_POWER_ON,        /* 上电状态 */
} xiaomi_86box_cache_field_t;

/* 帧处理入口 */
void xiaomi_86box_handle_frame(XIAOMI_86BOX_Frame_t *frame);

/* 应答接口: 原样回显请求的设备头 + 指定子命令与参数 */
uint8_t xiaomi_86box_reply_frame(const XIAOMI_86BOX_Frame_t *request,
                                 uint8_t subcmd,
                                 const uint8_t *params,
                                 uint16_t params_len);

/* 无参数应答(主命令级) */
void xiaomi_86box_reply_no_param_frame(uint8_t command);

/* 主动上报: 自动拼 0x44 设备头, 供 buffer 层回调使用 */
uint8_t xiaomi_86box_send_report(uint8_t device_type,
                                 uint8_t device_id,
                                 uint8_t seq_id,
                                 uint8_t subcmd,
                                 const uint8_t *params,
                                 uint16_t params_len);

/* 更新内部状态缓存(供 0x43 获取所有状态使用) */
void xiaomi_86box_cache_set(uint8_t device_type,
                            uint8_t device_id,
                            uint8_t field,
                            uint16_t value);

/* 天气码缓存(4 字节, 高字节在前) */
uint32_t xiaomi_86box_get_weather_code(void);

#endif /* __XIAOMI_SMART_SCREEN_86BOX_HANDLE_H__ */
