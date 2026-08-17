/*
 * Tuya DP Dispatch — Scene (DP 1-8)
 */
#include "tuya_device_scene.h"
#include "protocol.h"
#include "hooch_scene_dispatch.h"

unsigned char tuya_dp_dispatch_scene(unsigned char dp_id, unsigned char dp_type,
                                     unsigned short dp_len, unsigned char *dp_data)
{
    (void)dp_type;
    (void)dp_len;
    (void)dp_data;

    switch (dp_id) {
    case DPID_SCENE_1: HOOCH_PROTOCOL_SceneDispatch_Send(HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_1); return 1U;
    case DPID_SCENE_2: HOOCH_PROTOCOL_SceneDispatch_Send(HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_2); return 1U;
    case DPID_SCENE_3: HOOCH_PROTOCOL_SceneDispatch_Send(HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_3); return 1U;
    case DPID_SCENE_4: HOOCH_PROTOCOL_SceneDispatch_Send(HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_4); return 1U;
    case DPID_SCENE_5: HOOCH_PROTOCOL_SceneDispatch_Send(HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_5); return 1U;
    case DPID_SCENE_6: HOOCH_PROTOCOL_SceneDispatch_Send(HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_6); return 1U;
    case DPID_SCENE_7: HOOCH_PROTOCOL_SceneDispatch_Send(HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_7); return 1U;
    case DPID_SCENE_8: HOOCH_PROTOCOL_SceneDispatch_Send(HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_8); return 1U;
    default: break;
    }
    return 0U;
}
