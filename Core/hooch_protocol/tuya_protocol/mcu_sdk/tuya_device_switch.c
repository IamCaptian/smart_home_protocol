/*
 * Tuya DP Dispatch — Switch / Key Status (DP 24-29, 137-138)
 */
#include "tuya_device_switch.h"
#include "protocol.h"
#include "hooch_key_status_dispatch.h"

unsigned char tuya_dp_dispatch_switch(unsigned char dp_id, unsigned char dp_type,
                                      unsigned short dp_len, unsigned char *dp_data)
{
    HOOCH_PROTOCOL_KeyStatusDispatchKey_t key;

    (void)dp_type;

    switch (dp_id) {
    case DPID_SWITCH_1: key = HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_1; break;
    case DPID_SWITCH_2: key = HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_2; break;
    case DPID_SWITCH_3: key = HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_3; break;
    case DPID_SWITCH_4: key = HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_4; break;
    case DPID_SWITCH_5: key = HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_5; break;
    case DPID_SWITCH_6: key = HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_6; break;
    case DPID_SWITCH_7: key = HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_7; break;
    case DPID_SWITCH_8: key = HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_KEY_8; break;
    default: return 0U;
    }

    HOOCH_PROTOCOL_KeyStatusDispatchFrame_t key_status_frame;
    key_status_frame.sequence = 0U;
    key_status_frame.valid   = 0U;
    key_status_frame.key     = key;
    key_status_frame.state   = mcu_get_dp_download_bool(dp_data, dp_len)
        ? HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_STATE_PRESSED
        : HOOCH_PROTOCOL_KEY_STATUS_DISPATCH_STATE_RELEASED;
    HOOCH_PROTOCOL_KeyStatusDispatch_SetFrame(&key_status_frame);

    return 1U;
}
