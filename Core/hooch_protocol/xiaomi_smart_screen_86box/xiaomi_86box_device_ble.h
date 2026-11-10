/*
 * Xiaomi 86 Box — BLE 基本资料/屏显参数处理
 *
 * 设备类型 0xFF: 童锁、屏显参数、天气、蜂鸣器、背光等模组级设置。
 */
#ifndef XIAOMI_86BOX_DEVICE_BLE_H
#define XIAOMI_86BOX_DEVICE_BLE_H

#include "xiaomi_86box_device_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 处理 BLE 基本资料子命令: 返回 1=已处理, 0=未知子命令 */
uint8_t xiaomi_86box_device_ble_handle(const xiaomi_86box_device_header_t *header,
                                       uint8_t subcmd,
                                       const uint8_t *params,
                                       uint16_t params_len);

#ifdef __cplusplus
}
#endif

#endif /* XIAOMI_86BOX_DEVICE_BLE_H */
