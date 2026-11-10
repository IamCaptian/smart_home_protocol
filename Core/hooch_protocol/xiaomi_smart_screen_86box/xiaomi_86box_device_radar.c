/*
 * Xiaomi 86 Box — 人存在雷达传感器处理
 */
#include "xiaomi_86box_device_radar.h"

uint8_t xiaomi_86box_device_radar_handle(const xiaomi_86box_device_header_t *header,
                                         uint8_t subcmd,
                                         const uint8_t *params,
                                         uint16_t params_len)
{
    (void)header;

    switch (subcmd)
    {
    case XIAOMI_86BOX_RADAR_PARAM_SET:
        if (params_len >= 2U)
        {
            if (params[0] == 0U)    /* 感应功能使能 */
            {
                xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_SWITCH,
                                              params[1], HOOCH_PROTOCOL_SETTING_PAGE_INVALID);
            }
            else if (params[0] == 6U)   /* 人体传感器灵敏度 */
            {
                xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_SENSITIVITY,
                                              params[1], HOOCH_PROTOCOL_SETTING_PAGE_INVALID);
            }
        }
        break;

    case XIAOMI_86BOX_RADAR_STATUS_REPORT:
        if (params_len >= 1U)
        {
            xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_SENSOR_REPORT,
                                          params[0], HOOCH_PROTOCOL_SETTING_PAGE_INVALID);
        }
        break;

    case XIAOMI_86BOX_RADAR_PARAM_GET:
        /* 查询类请求, 由上层按需补全, 此处仅应答 */
        break;

    default:
        return 0U;
    }

    return 1U;
}
