/*
 * Xiaomi 86 Box — 情景开关设备处理
 *
 * 设备类型 0x22。
 */
#ifndef XIAOMI_86BOX_DEVICE_SCENE_H
#define XIAOMI_86BOX_DEVICE_SCENE_H

#include "xiaomi_86box_device_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 处理情景开关设备子命令: 返回 1=已处理, 0=未知子命令 */
uint8_t xiaomi_86box_device_scene_handle(const xiaomi_86box_device_header_t *header,
                                         uint8_t subcmd,
                                         const uint8_t *params,
                                         uint16_t params_len);

#ifdef __cplusplus
}
#endif

#endif /* XIAOMI_86BOX_DEVICE_SCENE_H */
