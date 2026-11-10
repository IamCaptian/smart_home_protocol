/*
 * Tuya DP Dispatch — Relay (DP 24-27, 153-156)
 *
 * 继电器 1-8 控制：
 *   继电器1-4 → DPID_SWITCH_1 ~ DPID_SWITCH_4 (24-27)
 *   继电器5-8 → DPID_RELAY_5 ~ DPID_RELAY_8 (153-156)
 *
 * 下发语义：App/网关下发的 DP 指令 → Hooch Relay 下发回调(Dispatch)。
 * 注意：DPID_SWITCH_01 ~ DPID_SWITCH_04 (157-160) 为开关1-4，
 *       DPID_SWITCH_5 ~ DPID_SWITCH_8 (28-29/137/138) 为开关5-8，均走按键状态，不在此处理。
 */
#ifndef TUYA_DEVICE_RELAY_H
#define TUYA_DEVICE_RELAY_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  处理继电器相关的涂鸦 DP 下发 (继电器1-8)
 * @param  dp_id   涂鸦 DP ID
 * @param  dp_type 涂鸦 DP 类型
 * @param  dp_len  DP 数据长度
 * @param  dp_data DP 数据指针
 * @return 1=已处理, 0=未匹配到
 */
unsigned char tuya_dp_dispatch_relay(unsigned char dp_id, unsigned char dp_type,
                                     unsigned short dp_len, unsigned char *dp_data);

#ifdef __cplusplus
}
#endif

#endif /* TUYA_DEVICE_RELAY_H */
