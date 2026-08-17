/*
 * Tuya DP Dispatch — Key Mode & Key Name (DP 139, 145)
 *
 * 按键模式/按键名称 DP 下发处理 → HOOCH KeyMode / KeyName / Setting。
 */
#ifndef TUYA_DEVICE_KEY_H
#define TUYA_DEVICE_KEY_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  处理按键模式/名称相关的涂鸦 DP 下发
 * @param  dp_id   涂鸦 DP ID
 * @param  dp_type 涂鸦 DP 类型
 * @param  dp_len  DP 数据长度
 * @param  dp_data DP 数据指针
 * @return 1=已处理, 0=未匹配到
 */
unsigned char tuya_dp_dispatch_key(unsigned char dp_id, unsigned char dp_type,
                                   unsigned short dp_len, unsigned char *dp_data);

#ifdef __cplusplus
}
#endif

#endif /* TUYA_DEVICE_KEY_H */
