/*
 * Xiaomi 86 Box — 灯光设备处理
 *
 * 覆盖 WY 灯 / RGB 灯 / 单色灯 / WY_RGB 灯(设备类型 0x01/0x03/0x05/0x08)。
 */
#ifndef XIAOMI_86BOX_DEVICE_LIGHT_H
#define XIAOMI_86BOX_DEVICE_LIGHT_H

#include "xiaomi_86box_device_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 处理灯类设备子命令: 返回 1=已处理, 0=未知子命令 */
uint8_t xiaomi_86box_device_light_handle(const xiaomi_86box_device_header_t *header,
                                         uint8_t subcmd,
                                         const uint8_t *params,
                                         uint16_t params_len);

/* 填充"上报所有状态"载荷: 返回 1=已填充 */
uint8_t xiaomi_86box_device_light_report_all(const xiaomi_86box_device_header_t *header,
                                             uint8_t *params,
                                             uint16_t *params_len);

#ifdef __cplusplus
}
#endif

#endif /* XIAOMI_86BOX_DEVICE_LIGHT_H */
