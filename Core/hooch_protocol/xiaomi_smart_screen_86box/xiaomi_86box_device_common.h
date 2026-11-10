/*
 * Xiaomi 86 Box — 设备处理公共层
 *
 * 各设备模块(灯/开关/窗帘/空调/新风/地暖等)共用的状态缓存、HOOCH 下发封装
 * 与参数换算工具。设备模块只负责解析自己的子命令, 不再重复实现这些逻辑。
 */
#ifndef XIAOMI_86BOX_DEVICE_COMMON_H
#define XIAOMI_86BOX_DEVICE_COMMON_H

#include "xiaomi_smart_screen_86box_handle.h"
#include "hooch_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 各设备支持的最大路数 */
#define XIAOMI_86BOX_LIGHT_COUNT          4U
#define XIAOMI_86BOX_SWITCH_COUNT         8U
#define XIAOMI_86BOX_SWITCH_KEY_COUNT     4U
#define XIAOMI_86BOX_CURTAIN_COUNT        3U

/* 单设备状态缓存, 用于应答 0x43 获取所有状态 */
typedef struct
{
    uint8_t  power;
    uint8_t  switch_state;
    uint8_t  brightness;
    uint16_t color_temp;
    uint8_t  percent;
    uint8_t  set_travel;
    uint8_t  current_travel;
    uint8_t  fan_speed;
    uint8_t  mode;
    uint16_t temperature;
    uint8_t  power_on;
} xiaomi_86box_state_t;

/* 大端读取 */
uint16_t xiaomi_86box_read_be16(const uint8_t *data);
uint32_t xiaomi_86box_read_be32(const uint8_t *data);

/* 设备类型判定 */
uint8_t xiaomi_86box_is_light_type(uint8_t device_type);
uint8_t xiaomi_86box_is_switch_type(uint8_t device_type);

/* 按设备类型/ID 取状态缓存 */
xiaomi_86box_state_t *xiaomi_86box_state_get(uint8_t device_type, uint8_t device_id);

/* HOOCH 下发封装 */
void xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SettingItem_t item,
                                   uint32_t value,
                                   HOOCH_PROTOCOL_SettingPage_t page);
void xiaomi_86box_dispatch_screen_off_time(uint32_t seconds);
void xiaomi_86box_dispatch_interlock(uint8_t value);
void xiaomi_86box_dispatch_pairing(uint8_t page, uint8_t channel, uint8_t raw_action);
void xiaomi_86box_dispatch_key_state(uint8_t key, uint8_t state);
void xiaomi_86box_dispatch_key_click(uint8_t key, uint8_t raw_click);

/* 协议原始值 -> HOOCH 语义值 */
HOOCH_PROTOCOL_AirConditionerFanSpeed_t xiaomi_86box_fan_speed_from_raw(uint8_t raw);
uint16_t xiaomi_86box_ac_temperature_from_raw(uint8_t raw);
uint16_t xiaomi_86box_floor_temperature_from_raw(uint8_t raw);

/* 设备类型 -> HOOCH 页面 */
HOOCH_PROTOCOL_SettingPage_t xiaomi_86box_page_from_device_type(uint8_t device_type);
HOOCH_PROTOCOL_KeyNamePage_t xiaomi_86box_name_page_from_device_type(uint8_t device_type);

/* 天气码缓存(由 0x11 上报天气更新) */
void xiaomi_86box_set_weather_code(uint32_t weather_code);

#ifdef __cplusplus
}
#endif

#endif /* XIAOMI_86BOX_DEVICE_COMMON_H */
