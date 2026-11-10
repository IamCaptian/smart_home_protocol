/*
 * Tuya DP Dispatch — Relay (DP 24-27, 153-156)
 *
 * 继电器1-4：DPID_SWITCH_1 ~ DPID_SWITCH_4 (24-27)
 * 继电器5-8：DPID_RELAY_5 ~ DPID_RELAY_8 (153-156)
 * 注意：DPID_SWITCH_01 ~ DPID_SWITCH_04 (157-160) 为开关1-4，走按键状态分发。
 */
#include "tuya_device_relay.h"
#include "protocol.h"
#include "hooch_relay.h"
#include <stddef.h>

/* 单个继电器通道的 bool DP 下发处理。 */
static unsigned char handle_relay_switch(HOOCH_PROTOCOL_RelayChannel_t channel,
                                         unsigned short dp_len, unsigned char *dp_data)
{
    HOOCH_PROTOCOL_RelaySwitchState_t state =
        mcu_get_dp_download_bool(dp_data, dp_len)
            ? HOOCH_PROTOCOL_RELAY_SWITCH_ON
            : HOOCH_PROTOCOL_RELAY_SWITCH_OFF;

    return (HOOCH_PROTOCOL_Relay_DispatchSwitch(channel, state) ==
            HOOCH_PROTOCOL_RELAY_RESULT_OK) ? 1U : 0U;
}

unsigned char tuya_dp_dispatch_relay(unsigned char dp_id, unsigned char dp_type,
                                     unsigned short dp_len, unsigned char *dp_data)
{
    (void)dp_type;

    switch (dp_id) {
    case DPID_SWITCH_1:  return handle_relay_switch(HOOCH_PROTOCOL_RELAY_CHANNEL_1, dp_len, dp_data);
    case DPID_SWITCH_2:  return handle_relay_switch(HOOCH_PROTOCOL_RELAY_CHANNEL_2, dp_len, dp_data);
    case DPID_SWITCH_3:  return handle_relay_switch(HOOCH_PROTOCOL_RELAY_CHANNEL_3, dp_len, dp_data);
    case DPID_SWITCH_4:  return handle_relay_switch(HOOCH_PROTOCOL_RELAY_CHANNEL_4, dp_len, dp_data);
    case DPID_RELAY_5:   return handle_relay_switch(HOOCH_PROTOCOL_RELAY_CHANNEL_5, dp_len, dp_data);
    case DPID_RELAY_6:   return handle_relay_switch(HOOCH_PROTOCOL_RELAY_CHANNEL_6, dp_len, dp_data);
    case DPID_RELAY_7:   return handle_relay_switch(HOOCH_PROTOCOL_RELAY_CHANNEL_7, dp_len, dp_data);
    case DPID_RELAY_8:   return handle_relay_switch(HOOCH_PROTOCOL_RELAY_CHANNEL_8, dp_len, dp_data);
    default: break;
    }
    return 0U;
}
