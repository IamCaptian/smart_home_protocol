/*
 * Xiaomi 86 Box — 窗帘设备处理
 */
#include "xiaomi_86box_device_curtain.h"
#include "hooch_protocol.h"

/* 窗帘开关控制: 参数1 = 0-打开 1-暂停 2-关闭 */
static uint8_t xiaomi_86box_curtain_handle_switch(const xiaomi_86box_device_header_t *header,
                                                  uint8_t key,
                                                  const uint8_t *params,
                                                  uint16_t params_len)
{
    if (params_len < 1U)
    {
        return 1U;
    }

    if (params[0] == 0U)        /* 打开 */
    {
        (void)HOOCH_PROTOCOL_Curtain_DispatchSwitch(
            (HOOCH_PROTOCOL_CurtainKey_t)key, HOOCH_PROTOCOL_CURTAIN_SWITCH_ON);
        xiaomi_86box_cache_set(header->device_type, key, XIAOMI_86BOX_CACHE_SWITCH, 1U);
    }
    else if (params[0] == 2U)   /* 关闭 */
    {
        (void)HOOCH_PROTOCOL_Curtain_DispatchSwitch(
            (HOOCH_PROTOCOL_CurtainKey_t)key, HOOCH_PROTOCOL_CURTAIN_SWITCH_OFF);
        xiaomi_86box_cache_set(header->device_type, key, XIAOMI_86BOX_CACHE_SWITCH, 0U);
    }
    else                        /* 暂停 */
    {
        (void)HOOCH_PROTOCOL_Curtain_DispatchStop(
            (HOOCH_PROTOCOL_CurtainKey_t)key, 1U);
    }

    return 1U;
}

/* 0x41 上报所有状态: 状态 | 设定行程 | 当前行程 (0-打开 1-暂停 2-关闭) */
static uint8_t xiaomi_86box_curtain_handle_report_all(
    const xiaomi_86box_device_header_t *header,
    uint8_t key,
    const uint8_t *params,
    uint16_t params_len)
{
    if (params_len < 3U)
    {
        return 1U;
    }

    switch (params[0])
    {
    case 0U:    /* 打开 */
        (void)HOOCH_PROTOCOL_Curtain_DispatchSwitch(
            (HOOCH_PROTOCOL_CurtainKey_t)key, HOOCH_PROTOCOL_CURTAIN_SWITCH_ON);
        xiaomi_86box_cache_set(header->device_type, key, XIAOMI_86BOX_CACHE_SWITCH, 1U);
        break;

    case 2U:    /* 关闭 */
        (void)HOOCH_PROTOCOL_Curtain_DispatchSwitch(
            (HOOCH_PROTOCOL_CurtainKey_t)key, HOOCH_PROTOCOL_CURTAIN_SWITCH_OFF);
        xiaomi_86box_cache_set(header->device_type, key, XIAOMI_86BOX_CACHE_SWITCH, 0U);
        break;

    default:    /* 1-暂停 */
        (void)HOOCH_PROTOCOL_Curtain_DispatchStop(
            (HOOCH_PROTOCOL_CurtainKey_t)key, 1U);
        break;
    }

    (void)HOOCH_PROTOCOL_Curtain_DispatchPercent(
        (HOOCH_PROTOCOL_CurtainKey_t)key, params[2]);

    xiaomi_86box_cache_set(header->device_type, key, XIAOMI_86BOX_CACHE_SET_TRAVEL, params[1]);
    xiaomi_86box_cache_set(header->device_type, key, XIAOMI_86BOX_CACHE_CURRENT_TRAVEL, params[2]);
    xiaomi_86box_cache_set(header->device_type, key, XIAOMI_86BOX_CACHE_PERCENT, params[2]);

    return 1U;
}

uint8_t xiaomi_86box_device_curtain_handle(const xiaomi_86box_device_header_t *header,
                                           uint8_t subcmd,
                                           const uint8_t *params,
                                           uint16_t params_len)
{
    uint8_t key = header->device_id;

    switch (subcmd)
    {
    case XIAOMI_86BOX_SUBCMD_REPORT_ALL:
        return xiaomi_86box_curtain_handle_report_all(header, key, params, params_len);

    case XIAOMI_86BOX_CURTAIN_SWITCH:
        return xiaomi_86box_curtain_handle_switch(header, key, params, params_len);

    case XIAOMI_86BOX_CURTAIN_TRAVEL:
        if (params_len >= 1U)
        {
            (void)HOOCH_PROTOCOL_Curtain_DispatchPercent(
                (HOOCH_PROTOCOL_CurtainKey_t)key, params[0]);
            xiaomi_86box_cache_set(header->device_type, key, XIAOMI_86BOX_CACHE_PERCENT, params[0]);
        }
        break;

    case XIAOMI_86BOX_CURTAIN_PAIRING:
        if (params_len >= 1U)
        {
            xiaomi_86box_dispatch_pairing(HOOCH_PROTOCOL_CODE_MATCH_PAGE_CURTAIN,
                                          key, params[0]);
        }
        break;

    default:
        return 0U;
    }

    return 1U;
}

uint8_t xiaomi_86box_device_curtain_report_all(const xiaomi_86box_device_header_t *header,
                                               uint8_t *params,
                                               uint16_t *params_len)
{
    xiaomi_86box_state_t *state = xiaomi_86box_state_get(header->device_type,
                                                         header->device_id);
    if (state == NULL)
    {
        return 0U;
    }

    params[0] = state->switch_state;
    params[1] = state->set_travel;
    params[2] = state->current_travel;
    *params_len = 3U;

    return 1U;
}
