/*
 * Tuya DP Dispatch — Fresh Air (DP 116, 123, 150, 135)
 *
 * 新风相关 DP 下发处理 → HOOCH FreshAir。
 */
#ifndef TUYA_DEVICE_FRESHAIR_H
#define TUYA_DEVICE_FRESHAIR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  处理新风相关的涂鸦 DP 下发
 * @param  dp_id   涂鸦 DP ID
 * @param  dp_type 涂鸦 DP 类型
 * @param  dp_len  DP 数据长度
 * @param  dp_data DP 数据指针
 * @return 1=已处理, 0=未匹配到
 */
unsigned char tuya_dp_dispatch_freshair(unsigned char dp_id, unsigned char dp_type,
                                        unsigned short dp_len, unsigned char *dp_data);

#ifdef __cplusplus
}
#endif

#endif /* TUYA_DEVICE_FRESHAIR_H */
