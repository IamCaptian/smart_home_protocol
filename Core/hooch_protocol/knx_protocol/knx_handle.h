#ifndef KNX_HANDLE_H
#define KNX_HANDLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "knx_protocol_uart.h"

/* Fun1 values defined by panel communication protocol V2.4. */
typedef enum
{
    KNX_FUN1_BASIC_FUNCTION = 1U,
    KNX_FUN1_DEVICE = 2U,
    KNX_FUN1_BASIC_SETTING = 3U,
} KNX_Fun1_t;

/* Fun2 for basic function (fun1 = 1). */
typedef enum
{
    KNX_BASIC_FUNCTION_DATE = 1U,
    KNX_BASIC_FUNCTION_TIME = 2U,
    KNX_BASIC_FUNCTION_VERSION = 3U,
    KNX_BASIC_FUNCTION_UPDATE_CONFIG = 4U,
    KNX_BASIC_FUNCTION_CONFIG_SYNC_STATUS = 5U,
    KNX_BASIC_FUNCTION_PANEL_UPDATE_STATUS = 6U,
    KNX_BASIC_FUNCTION_DEVICE_TYPE = 7U,
    KNX_BASIC_FUNCTION_UNIQUE_SERIAL = 8U,
    KNX_BASIC_FUNCTION_SUPPORT_PARAMETER = 9U,
    KNX_BASIC_FUNCTION_PROGRAM_MODE = 10U,
    KNX_BASIC_FUNCTION_DEVICE_REBOOT = 11U,
    KNX_BASIC_FUNCTION_RESTORE_FACTORY = 12U,
    KNX_BASIC_FUNCTION_DEVICE_LOCK = 13U,
    KNX_BASIC_FUNCTION_WORLD_TIME = 14U,
    KNX_BASIC_FUNCTION_TEMPERATURE_SENSOR = 15U,
    KNX_BASIC_FUNCTION_HUMIDITY_SENSOR = 16U,
    KNX_BASIC_FUNCTION_HUMAN_PRESENCE_SENSOR = 17U,
    KNX_BASIC_FUNCTION_BACKLIGHT_SCREEN_CONTROL = 18U,
} KNX_BasicFunction_t;

/* Fun2 for basic setting (fun1 = 3). */
typedef enum
{
    KNX_BASIC_SETTING_SCREENSAVER_TYPE = 1U,
    KNX_BASIC_SETTING_THEME = 2U,
    KNX_BASIC_SETTING_BACKGROUND = 3U,
    KNX_BASIC_SETTING_DEVICE_LOCK_CONTROL = 4U,
    KNX_BASIC_SETTING_KEY_CHILD_LOCK = 5U,
    KNX_BASIC_SETTING_SCREEN_BACKLIGHT_BRIGHTNESS = 6U,
    KNX_BASIC_SETTING_AUTO_BRIGHTNESS_ENABLE = 7U,
    KNX_BASIC_SETTING_KEY_BACKLIGHT_BRIGHTNESS_NIGHT = 8U,
    KNX_BASIC_SETTING_KEY_BRIGHTNESS_NIGHT = 9U,
    KNX_BASIC_SETTING_KEY_BACKLIGHT_MODE = 10U,        /* 按键背光模式: 0=常亮, 1=延时 */
    KNX_BASIC_SETTING_KEY_WAKEUP_BACKLIGHT_ENABLE = 11U,
    KNX_BASIC_SETTING_KEY_STATUS_LINK_BACKLIGHT = 12U,
    KNX_BASIC_SETTING_KEY_LONG_PRESS_TIME = 13U,
    KNX_BASIC_SETTING_KEY_MULTI_EFFECTIVE = 14U,
    KNX_BASIC_SETTING_KEY_BACKLIGHT_TIME = 15U,
    KNX_BASIC_SETTING_PRESENCE_SENSOR_ENABLE = 16U,
    KNX_BASIC_SETTING_PRESENCE_SENSOR_SENSITIVITY = 17U,
    KNX_BASIC_SETTING_KEY_BACKLIGHT_BRIGHTNESS_DAY = 18U,  /* 按键背光亮度(白天)(K->P) */
    KNX_BASIC_SETTING_KEY_BRIGHTNESS_DAY = 19U,            /* 按键亮度(白天)(K->P) */
    KNX_BASIC_SETTING_SCREEN_STANDBY_BRIGHTNESS = 20U,
    KNX_BASIC_SETTING_SCREEN_SCREENSAVER_ENABLE = 22U,
    KNX_BASIC_SETTING_SCREEN_CHILD_LOCK = 23U,
    KNX_BASIC_SETTING_SCREEN_OFF_TIME = 24U,
    KNX_BASIC_SETTING_RETURN_HOME_TIME = 25U,
    KNX_BASIC_SETTING_ENTER_SCREENSAVER_TIME = 26U,
    KNX_BASIC_SETTING_SCREEN_PASSWORD = 27U,
    KNX_BASIC_SETTING_TEMPERATURE_UNIT = 28U,
    KNX_BASIC_SETTING_PAGE_BUTTON_FUNCTION = 29U,
    KNX_BASIC_SETTING_LANGUAGE_SELECT = 30U,
    KNX_BASIC_SETTING_SOUND_ENABLE = 31U,
    KNX_BASIC_SETTING_VIBRATION_ENABLE = 32U,
    KNX_BASIC_SETTING_PAGE_MANAGEMENT = 33U,               /* 页面管理(K->P) */
} KNX_BasicSetting_t;

/* Device category stored in fun4 when fun1 = 2. */
typedef enum
{
    KNX_DEVICE_CATEGORY_CONTROL = 0U,
    KNX_DEVICE_CATEGORY_CONFIG = 1U,
} KNX_DeviceCategory_t;

/* Device type stored in fun3 when fun1 = 2. */
typedef enum
{
    KNX_DEVICE_TYPE_AIR_CONDITIONER = 1U,
    KNX_DEVICE_TYPE_FLOOR_HEATING = 2U,
    KNX_DEVICE_TYPE_FRESH_AIR = 3U,
    KNX_DEVICE_TYPE_HVAC = 4U,
    KNX_DEVICE_TYPE_KEY = 5U,
    KNX_DEVICE_TYPE_DIMMING = 6U,
    KNX_DEVICE_TYPE_CURTAIN = 7U,
    KNX_DEVICE_TYPE_SCENE = 8U,
    KNX_DEVICE_TYPE_AV = 9U,
    KNX_DEVICE_TYPE_VALUE_DISPLAY = 10U,
    KNX_DEVICE_TYPE_TOUCH_KEY = 11U,
    KNX_DEVICE_TYPE_PUBLIC_DISPLAY = 12U,
} KNX_DeviceType_t;

/* Basic setting function type used by item 4. */
typedef enum
{
    KNX_SETTING_FUNCTION_TYPE_AIR_CONDITIONER = 1U,
    KNX_SETTING_FUNCTION_TYPE_FLOOR_HEATING = 2U,
    KNX_SETTING_FUNCTION_TYPE_FRESH_AIR = 3U,
    KNX_SETTING_FUNCTION_TYPE_DIMMING = 4U,
    KNX_SETTING_FUNCTION_TYPE_CURTAIN = 5U,
} KNX_BasicSettingFunctionType_t;

/* Air conditioner items. */
typedef enum
{
    KNX_AIR_ITEM_SWITCH = 1U,
    KNX_AIR_ITEM_SWITCH_STATUS = 2U,
    KNX_AIR_ITEM_MODE = 3U,
    KNX_AIR_ITEM_MODE_STATUS = 4U,
    KNX_AIR_ITEM_FAN_SPEED = 5U,
    KNX_AIR_ITEM_FAN_SPEED_STATUS = 6U,
    KNX_AIR_ITEM_SET_TEMPERATURE = 7U,
    KNX_AIR_ITEM_SET_TEMPERATURE_STATUS = 8U,
    KNX_AIR_ITEM_ACTUAL_TEMPERATURE = 9U,
} KNX_AirItem_t;

/* Air conditioner config items. */
typedef enum
{
    KNX_AIR_CONFIG_ENABLE_BITMAP = 1U,
    KNX_AIR_CONFIG_DESCRIPTION = 2U,
    KNX_AIR_CONFIG_DEFAULT_ICON = 3U,
    KNX_AIR_CONFIG_SELECTED_ICON = 4U,
    KNX_AIR_CONFIG_TEMP_STEP = 5U,
    KNX_AIR_CONFIG_TEMP_MIN = 6U,
    KNX_AIR_CONFIG_TEMP_MAX = 7U,
} KNX_AirConfigItem_t;

/* Floor heating items. */
typedef enum
{
    KNX_FLOOR_ITEM_SWITCH = 1U,
    KNX_FLOOR_ITEM_SWITCH_STATUS = 2U,
    KNX_FLOOR_ITEM_SET_TEMPERATURE = 3U,
    KNX_FLOOR_ITEM_SET_TEMPERATURE_STATUS = 4U,
    KNX_FLOOR_ITEM_ACTUAL_TEMPERATURE = 5U,
    KNX_FLOOR_ITEM_MANUAL_AUTO = 6U,
    KNX_FLOOR_ITEM_MANUAL_AUTO_STATUS = 7U,
    KNX_FLOOR_ITEM_RELAY_SWITCH = 8U,
    KNX_FLOOR_ITEM_RELAY_SWITCH_STATUS = 9U,
} KNX_FloorHeatingItem_t;

/* Floor heating config items. */
typedef enum
{
    KNX_FLOOR_CONFIG_ENABLE_BITMAP = 1U,
    KNX_FLOOR_CONFIG_DESCRIPTION = 2U,
    KNX_FLOOR_CONFIG_DEFAULT_ICON = 3U,
    KNX_FLOOR_CONFIG_SELECTED_ICON = 4U,
    KNX_FLOOR_CONFIG_TEMP_STEP = 5U,
    KNX_FLOOR_CONFIG_TEMP_MIN = 6U,
    KNX_FLOOR_CONFIG_TEMP_MAX = 7U,
} KNX_FloorHeatingConfigItem_t;

/* Fresh air items. */
typedef enum
{
    KNX_FRESH_AIR_ITEM_SWITCH = 1U,
    KNX_FRESH_AIR_ITEM_SWITCH_STATUS = 2U,
    KNX_FRESH_AIR_ITEM_FAN_SPEED_MODE = 3U,
    KNX_FRESH_AIR_ITEM_FAN_SPEED_MODE_STATUS = 4U,
    KNX_FRESH_AIR_ITEM_ACTUAL_FAN_SPEED = 5U,
    KNX_FRESH_AIR_ITEM_ACTUAL_TEMPERATURE = 6U,
} KNX_FreshAirItem_t;

/* Fresh air config items. */
typedef enum
{
    KNX_FRESH_AIR_CONFIG_ENABLE_BITMAP = 1U,
    KNX_FRESH_AIR_CONFIG_DESCRIPTION = 2U,
    KNX_FRESH_AIR_CONFIG_DEFAULT_ICON = 3U,
    KNX_FRESH_AIR_CONFIG_SELECTED_ICON = 4U,
} KNX_FreshAirConfigItem_t;

/* Light key items. */
typedef enum
{
    KNX_KEY_ITEM_SWITCH = 1U,
    KNX_KEY_ITEM_SWITCH_STATUS = 2U,
} KNX_KeyItem_t;

/* Light key config items. */
typedef enum
{
    KNX_KEY_CONFIG_ENABLE_BITMAP = 1U,
    KNX_KEY_CONFIG_DESCRIPTION = 2U,
    KNX_KEY_CONFIG_DEFAULT_ICON = 3U,
    KNX_KEY_CONFIG_SELECTED_ICON = 4U,
} KNX_KeyConfigItem_t;

/* Dimming items. */
typedef enum
{
    KNX_DIMMING_ITEM_SWITCH = 1U,
    KNX_DIMMING_ITEM_SWITCH_STATUS = 2U,
    KNX_DIMMING_ITEM_BRIGHTNESS = 3U,
    KNX_DIMMING_ITEM_BRIGHTNESS_STATUS = 4U,
    KNX_DIMMING_ITEM_COLOR_TEMPERATURE = 5U,
    KNX_DIMMING_ITEM_COLOR_TEMPERATURE_STATUS = 6U,
    KNX_DIMMING_ITEM_RGBW = 7U,
    KNX_DIMMING_ITEM_RGBW_STATUS = 8U,
    KNX_DIMMING_ITEM_COLOR_TEMP_PERCENT = 9U,
    KNX_DIMMING_ITEM_COLOR_TEMP_PERCENT_STATUS = 10U,
} KNX_DimmingItem_t;

/* Dimming config items. */
typedef enum
{
    KNX_DIMMING_CONFIG_ENABLE_BITMAP = 1U,
    KNX_DIMMING_CONFIG_DESCRIPTION = 2U,
    KNX_DIMMING_CONFIG_DEFAULT_ICON = 3U,
    KNX_DIMMING_CONFIG_SELECTED_ICON = 4U,
    KNX_DIMMING_CONFIG_COLOR_TEMP_STEP = 5U,
    KNX_DIMMING_CONFIG_COLOR_TEMP_MIN = 6U,
    KNX_DIMMING_CONFIG_COLOR_TEMP_MAX = 7U,
} KNX_DimmingConfigItem_t;

/* Curtain items. */
typedef enum
{
    KNX_CURTAIN_ITEM_POSITION_OPEN_CLOSE = 1U,
    KNX_CURTAIN_ITEM_POSITION_STOP = 2U,
    KNX_CURTAIN_ITEM_POSITION_PERCENT = 3U,
    KNX_CURTAIN_ITEM_POSITION_PERCENT_STATUS = 4U,
    KNX_CURTAIN_ITEM_ANGLE_OPEN_CLOSE = 5U,
    KNX_CURTAIN_ITEM_ANGLE_STOP = 6U,
    KNX_CURTAIN_ITEM_ANGLE_PERCENT = 7U,
    KNX_CURTAIN_ITEM_ANGLE_PERCENT_STATUS = 8U,
} KNX_CurtainItem_t;

/* Curtain config items. */
typedef enum
{
    KNX_CURTAIN_CONFIG_ENABLE_BITMAP = 1U,
    KNX_CURTAIN_CONFIG_DESCRIPTION = 2U,
    KNX_CURTAIN_CONFIG_DEFAULT_ICON = 3U,
    KNX_CURTAIN_CONFIG_SELECTED_ICON = 4U,
    KNX_CURTAIN_CONFIG_TYPE = 5U,
} KNX_CurtainConfigItem_t;

/* Scene items. */
typedef enum
{
    KNX_SCENE_ITEM_TRIGGER = 1U,
    KNX_SCENE_ITEM_STATUS = 2U,
    KNX_SCENE_ITEM_LEARN = 3U,
} KNX_SceneItem_t;

/* Scene config items. */
typedef enum
{
    KNX_SCENE_CONFIG_ENABLE_BITMAP = 1U,
    KNX_SCENE_CONFIG_DESCRIPTION = 2U,
    KNX_SCENE_CONFIG_DEFAULT_ICON = 3U,
    KNX_SCENE_CONFIG_SELECTED_ICON = 4U,
} KNX_SceneConfigItem_t;

/* AV items. */
typedef enum
{
    KNX_AV_ITEM_SWITCH = 1U,
    KNX_AV_ITEM_SWITCH_STATUS = 2U,
    KNX_AV_ITEM_UP_DOWN = 3U,
    KNX_AV_ITEM_LEFT_RIGHT = 4U,
    KNX_AV_ITEM_VOLUME_UP_DOWN = 5U,
    KNX_AV_ITEM_VOLUME_SET = 6U,
    KNX_AV_ITEM_VOLUME_STATUS = 7U,
    KNX_AV_ITEM_CONFIRM = 8U,
    KNX_AV_ITEM_BACK = 9U,
    KNX_AV_ITEM_MUTE = 10U,
    KNX_AV_ITEM_MUTE_STATUS = 11U,
    KNX_AV_ITEM_PLAY_PAUSE = 12U,
    KNX_AV_ITEM_PLAY_MODE_CONTROL = 13U,
    KNX_AV_ITEM_PLAY_MODE_STATUS = 14U,
    KNX_AV_ITEM_PREV_NEXT = 15U,
} KNX_AvItem_t;

/* AV config items. */
typedef enum
{
    KNX_AV_CONFIG_ENABLE_BITMAP = 1U,
    KNX_AV_CONFIG_DESCRIPTION = 2U,
    KNX_AV_CONFIG_DEFAULT_ICON = 3U,
    KNX_AV_CONFIG_SELECTED_ICON = 4U,
} KNX_AvConfigItem_t;

/* Value display items. */
typedef enum
{
    KNX_VALUE_DISPLAY_ITEM_VALUE = 1U,
} KNX_ValueDisplayItem_t;

/* Value display config items. */
typedef enum
{
    KNX_VALUE_DISPLAY_CONFIG_ENABLE_BITMAP = 1U,
    KNX_VALUE_DISPLAY_CONFIG_DESCRIPTION = 2U,
    KNX_VALUE_DISPLAY_CONFIG_DEFAULT_ICON = 3U,
    KNX_VALUE_DISPLAY_CONFIG_SELECTED_ICON = 4U,
    KNX_VALUE_DISPLAY_CONFIG_UNIT = 5U,
    KNX_VALUE_DISPLAY_CONFIG_ARRAY_SIZE = 6U,
    KNX_VALUE_DISPLAY_CONFIG_DATA_TYPE = 7U,
    KNX_VALUE_DISPLAY_CONFIG_DECIMAL_COUNT = 8U,
} KNX_ValueDisplayConfigItem_t;

/* Touch key items. */
typedef enum
{
    KNX_TOUCH_KEY_ITEM_KEY_STATUS = 1U,
    KNX_TOUCH_KEY_ITEM_LED_STATUS = 2U,
    KNX_TOUCH_KEY_ITEM_KEY_LOCK = 3U,
} KNX_TouchKeyItem_t;

/* Public display items. */
typedef enum
{
    KNX_PUBLIC_DISPLAY_ITEM_TEMPERATURE = 1U,
    KNX_PUBLIC_DISPLAY_ITEM_HUMIDITY = 2U,
    KNX_PUBLIC_DISPLAY_ITEM_ILLUMINANCE = 3U,
    KNX_PUBLIC_DISPLAY_ITEM_CO = 4U,
    KNX_PUBLIC_DISPLAY_ITEM_PM2_5 = 5U,
    KNX_PUBLIC_DISPLAY_ITEM_PM10 = 6U,
    KNX_PUBLIC_DISPLAY_ITEM_HCHO = 7U,
    KNX_PUBLIC_DISPLAY_ITEM_TVOC = 8U,
    KNX_PUBLIC_DISPLAY_ITEM_CO2 = 9U,
    KNX_PUBLIC_DISPLAY_ITEM_AQI = 10U,
    KNX_PUBLIC_DISPLAY_ITEM_TEMPERATURE_CALIBRATION = 11U,
    KNX_PUBLIC_DISPLAY_ITEM_HUMIDITY_CALIBRATION = 12U,
    KNX_PUBLIC_DISPLAY_ITEM_ILLUMINANCE_CALIBRATION = 13U,
} KNX_PublicDisplayItem_t;

typedef void (*KNX_SummaryCallback_t)(const char *function_desc, const uint8_t *data, uint16_t data_len);

void knx_handle_frame(const KNX_Frame_t *frame);

void knx_register_summary_callback(KNX_SummaryCallback_t callback);

/* 基础功能写入（fun1=1, A1）。 */
uint8_t knx_send_basic_function(KNX_BasicFunction_t item,
                                const uint8_t *data,
                                uint16_t data_len);

/* 基础功能查询（fun1=1, A2）。 */
uint8_t knx_query_basic_function(KNX_BasicFunction_t item);

/* 基础设置写入（fun1=3, A1）。
 * 对于协议中带扩展 fun3 的项目（如 4/16/27/29），通过 ext_fun3 传入；
 * 无扩展 fun3 时传 0。
 */
uint8_t knx_send_basic_setting(KNX_BasicSetting_t item,
                               uint8_t ext_fun3,
                               const uint8_t *data,
                               uint16_t data_len);

/* 基础设置查询（fun1=3, A3）。 */
uint8_t knx_query_basic_setting(KNX_BasicSetting_t item,
                                uint8_t ext_fun3);

/* 设备功能写入（fun1=2, A1）。 */
uint8_t knx_send_device_function(uint8_t device_no,
                                 KNX_DeviceType_t device_type,
                                 KNX_DeviceCategory_t category,
                                 uint8_t item,
                                 const uint8_t *data,
                                 uint16_t data_len);

/* 设备功能查询：
 * - CONTROL 使用 A2
 * - CONFIG 使用 A3
 */
uint8_t knx_query_device_function(uint8_t device_no,
                                  KNX_DeviceType_t device_type,
                                  KNX_DeviceCategory_t category,
                                  uint8_t item);

/* 空调控制便捷接口。1字节型字段：开关/模式/风速。 */
uint8_t knx_send_air_conditioner_control_u8(uint8_t device_no,
                                            KNX_AirItem_t item,
                                            uint8_t value);

/* 空调控制便捷接口。2字节大端字段：设定温度/状态温度/实际温度。 */
uint8_t knx_send_air_conditioner_control_u16(uint8_t device_no,
                                             KNX_AirItem_t item,
                                             uint16_t value);

/* 空调配置便捷接口。 */
uint8_t knx_send_air_conditioner_config_u8(uint8_t device_no,
                                           KNX_AirConfigItem_t item,
                                           uint8_t value);

uint8_t knx_send_air_conditioner_config_u16(uint8_t device_no,
                                            KNX_AirConfigItem_t item,
                                            uint16_t value);

/* 地暖控制便捷接口。1字节型字段：开关/手动自动。 */
uint8_t knx_send_floor_heating_control_u8(uint8_t device_no,
                                          KNX_FloorHeatingItem_t item,
                                          uint8_t value);

/* 地暖控制便捷接口。2字节大端字段：设定温度/实际温度。 */
uint8_t knx_send_floor_heating_control_u16(uint8_t device_no,
                                           KNX_FloorHeatingItem_t item,
                                           uint16_t value);

/* 新风控制便捷接口。1字节型字段：开关/风速模式。 */
uint8_t knx_send_fresh_air_control_u8(uint8_t device_no,
                                      KNX_FreshAirItem_t item,
                                      uint8_t value);

/* 调光控制便捷接口。 */
uint8_t knx_send_dimming_control_u8(uint8_t device_no,
                                    KNX_DimmingItem_t item,
                                    uint8_t value);

uint8_t knx_send_dimming_control_u16(uint8_t device_no,
                                     KNX_DimmingItem_t item,
                                     uint16_t value);

/* 窗帘控制便捷接口。 */
uint8_t knx_send_curtain_control_u8(uint8_t device_no,
                                    KNX_CurtainItem_t item,
                                    uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* KNX_HANDLE_H */
