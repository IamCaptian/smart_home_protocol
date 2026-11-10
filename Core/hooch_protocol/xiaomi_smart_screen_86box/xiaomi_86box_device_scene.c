/*
 * Xiaomi 86 Box — 情景开关设备处理
 */
#include "xiaomi_86box_device_scene.h"

uint8_t xiaomi_86box_device_scene_handle(const xiaomi_86box_device_header_t *header,
                                         uint8_t subcmd,
                                         const uint8_t *params,
                                         uint16_t params_len)
{
    switch (subcmd)
    {
    case XIAOMI_86BOX_SCENE_SWITCH_STATE:
        /* MCU 主动上报方向, 收到时仅记录 */
        XIAOMI_86BOX_LOG_INFO("[86BOX] Scene switch state: scene=%u state=%u\r\n",
                              (unsigned int)((params_len > 0U) ? params[0] : 0U),
                              (unsigned int)((params_len > 1U) ? params[1] : 0U));
        break;

    case XIAOMI_86BOX_SCENE_SWITCH_EVENT:
        if (params_len >= 1U)
        {
            xiaomi_86box_dispatch_key_click(header->device_id, params[0]);
        }
        break;

    default:
        return 0U;
    }

    return 1U;
}
