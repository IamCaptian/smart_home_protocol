/*
 * Tuya DP Dispatch — Switch / Key Status (DP 157-160, 28-29, 137-138)
 *
 * 开关1-4：DPID_SWITCH_01 ~ DPID_SWITCH_04 (157-160)
 * 开关5-8：DPID_SWITCH_5 ~ DPID_SWITCH_8 (28-29/137/138)
 * 注意：DPID_SWITCH_1 ~ DPID_SWITCH_4 (24-27) 为继电器1-4，走继电器分发。
 */
#include "tuya_device_switch.h"
#include "protocol.h"
#include "hooch_key_status.h"

unsigned char tuya_dp_dispatch_switch(unsigned char dp_id, unsigned char dp_type,
                                      unsigned short dp_len, unsigned char *dp_data)
{
    HOOCH_PROTOCOL_KeyStatusKey_t key;

    (void)dp_type;

    switch (dp_id) {
    case DPID_SWITCH_01: key = HOOCH_PROTOCOL_KEY_STATUS_KEY_1; break;
    case DPID_SWITCH_02: key = HOOCH_PROTOCOL_KEY_STATUS_KEY_2; break;
    case DPID_SWITCH_03: key = HOOCH_PROTOCOL_KEY_STATUS_KEY_3; break;
    case DPID_SWITCH_04: key = HOOCH_PROTOCOL_KEY_STATUS_KEY_4; break;
    case DPID_SWITCH_5: key = HOOCH_PROTOCOL_KEY_STATUS_KEY_5; break;
    case DPID_SWITCH_6: key = HOOCH_PROTOCOL_KEY_STATUS_KEY_6; break;
    case DPID_SWITCH_7: key = HOOCH_PROTOCOL_KEY_STATUS_KEY_7; break;
    case DPID_SWITCH_8: key = HOOCH_PROTOCOL_KEY_STATUS_KEY_8; break;
    default: return 0U;
    }

    HOOCH_PROTOCOL_KeyStatusFrame_t key_status_frame;
    (void)memset(&key_status_frame, 0, sizeof(key_status_frame));
    key_status_frame.key = key;
    key_status_frame.state = mcu_get_dp_download_bool(dp_data, dp_len)
        ? HOOCH_PROTOCOL_KEY_STATUS_STATE_ON
        : HOOCH_PROTOCOL_KEY_STATUS_STATE_OFF;
    key_status_frame.control_item = HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_STATE;
    key_status_frame.source = HOOCH_PROTOCOL_SOURCE_TUYA;
    HOOCH_PROTOCOL_KeyStatus_DispatchFrame(&key_status_frame);

    return 1U;
}
