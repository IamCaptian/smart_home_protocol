/*
 * Tuya DP Dispatch — Scene (DP 1-8)
 *
 * 场景 DP 下发处理：将涂鸦场景枚举转换为 HOOCH SceneDispatch 分发。
 */
#ifndef TUYA_DEVICE_SCENE_H
#define TUYA_DEVICE_SCENE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  处理场景相关的涂鸦 DP 下发 (DPID_SCENE_1 ~ DPID_SCENE_8)
 * @param  dp_id   涂鸦 DP ID
 * @param  dp_type 涂鸦 DP 类型
 * @param  dp_len  DP 数据长度
 * @param  dp_data DP 数据指针
 * @return 1=已处理, 0=未匹配到
 */
unsigned char tuya_dp_dispatch_scene(unsigned char dp_id, unsigned char dp_type,
                                     unsigned short dp_len, unsigned char *dp_data);

#ifdef __cplusplus
}
#endif

#endif /* TUYA_DEVICE_SCENE_H */
