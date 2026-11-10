/*
 * Xiaomi 86 Box — 窗帘设备处理
 *
 * 设备类型 0xB0。
 */
#ifndef XIAOMI_86BOX_DEVICE_CURTAIN_H
#define XIAOMI_86BOX_DEVICE_CURTAIN_H

#include "xiaomi_86box_device_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 处理窗帘设备子命令: 返回 1=已处理, 0=未知子命令 */
uint8_t xiaomi_86box_device_curtain_handle(const xiaomi_86box_device_header_t *header,
                                           uint8_t subcmd,
                                           const uint8_t *params,
                                           uint16_t params_len);

/* 填充"上报所有状态"载荷: 返回 1=已填充 */
uint8_t xiaomi_86box_device_curtain_report_all(const xiaomi_86box_device_header_t *header,
                                               uint8_t *params,
                                               uint16_t *params_len);

#ifdef __cplusplus
}
#endif

#endif /* XIAOMI_86BOX_DEVICE_CURTAIN_H */
