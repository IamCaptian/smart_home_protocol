/*
 * Tuya DP Dispatch — Switch / Key Status (DP 24-29, 137-138)
 *
 * 继电器/开关 DP 下发处理：将涂鸦 bool 值转换为 HOOCH KeyStatusDispatch。
 */
#ifndef TUYA_DEVICE_SWITCH_H
#define TUYA_DEVICE_SWITCH_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  处理开关/继电器相关的涂鸦 DP 下发 (DPID_SWITCH_1 ~ DPID_SWITCH_8)
 * @param  dp_id   涂鸦 DP ID
 * @param  dp_type 涂鸦 DP 类型
 * @param  dp_len  DP 数据长度
 * @param  dp_data DP 数据指针
 * @return 1=已处理, 0=未匹配到
 */
unsigned char tuya_dp_dispatch_switch(unsigned char dp_id, unsigned char dp_type,
                                      unsigned short dp_len, unsigned char *dp_data);

#ifdef __cplusplus
}
#endif

#endif /* TUYA_DEVICE_SWITCH_H */
