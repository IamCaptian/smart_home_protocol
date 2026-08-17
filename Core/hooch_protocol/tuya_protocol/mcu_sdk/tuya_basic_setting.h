/*
 * Tuya DP Dispatch — Basic Settings (DP 132, 133, 140, 141, 142, 144, 146, 148, 149)
 *
 * 各类设置项 DP 下发处理 → HOOCH Setting。
 */
#ifndef TUYA_BASIC_SETTING_H
#define TUYA_BASIC_SETTING_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  处理基础设置相关的涂鸦 DP 下发
 * @param  dp_id   涂鸦 DP ID
 * @param  dp_type 涂鸦 DP 类型
 * @param  dp_len  DP 数据长度
 * @param  dp_data DP 数据指针
 * @return 1=已处理, 0=未匹配到
 */
unsigned char tuya_dp_dispatch_setting(unsigned char dp_id, unsigned char dp_type,
                                       unsigned short dp_len, unsigned char *dp_data);

#ifdef __cplusplus
}
#endif

#endif /* TUYA_BASIC_SETTING_H */
