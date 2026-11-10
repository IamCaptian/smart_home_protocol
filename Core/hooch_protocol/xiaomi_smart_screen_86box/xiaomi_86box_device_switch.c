/*
 * Xiaomi 86 Box — 开关设备处理
 */
#include "xiaomi_86box_device_switch.h"
#include "hooch_protocol.h"

/* 单路开关控制: 参数1 = 0-关 1-开 2-切换 */
static uint8_t xiaomi_86box_switch_handle_switch(const xiaomi_86box_device_header_t *header,
                                                 uint8_t key,
                                                 const uint8_t *params,
                                                 uint16_t params_len)
{
    uint8_t state;
    xiaomi_86box_state_t *cache = xiaomi_86box_state_get(header->device_type, key);

    if (params_len < 1U)
    {
        return 1U;
    }

    state = params[0];
    if (state == 2U)
    {
        state = ((cache != NULL) && (cache->switch_state != 0U)) ? 0U : 1U;
    }

    xiaomi_86box_dispatch_key_state(key, state);
    xiaomi_86box_cache_set(header->device_type, key, XIAOMI_86BOX_CACHE_SWITCH, state);

    return 1U;
}

/* 各路开关控制: 参数1 = 0~8(0 为总控), 参数2 = 状态位图 */
static uint8_t xiaomi_86box_switch_handle_channel(const xiaomi_86box_device_header_t *header,
                                                  const uint8_t *params,
                                                  uint16_t params_len)
{
    uint8_t channel;
    uint8_t bits;
    uint8_t key_index;

    if (params_len < 2U)
    {
        return 1U;
    }

    channel = params[0];
    bits = params[1];

    if (channel == 0U)
    {
        /* 总控: 位图对应 1~8 路 */
        for (key_index = 1U; key_index <= XIAOMI_86BOX_SWITCH_COUNT; key_index++)
        {
            uint8_t state = (uint8_t)((bits >> (key_index - 1U)) & 0x01U);
            xiaomi_86box_dispatch_key_state(key_index, state);
            xiaomi_86box_cache_set(header->device_type, key_index,
                                   XIAOMI_86BOX_CACHE_SWITCH, state);
        }
    }
    else
    {
        uint8_t state = (uint8_t)((bits >> (channel - 1U)) & 0x01U);
        xiaomi_86box_dispatch_key_state(channel, state);
        xiaomi_86box_cache_set(header->device_type, channel,
                               XIAOMI_86BOX_CACHE_SWITCH, state);
    }

    return 1U;
}

/* 上报所有状态: 参数1 = 1~8 路状态位图(部分模组只回 1 字节) */
static uint8_t xiaomi_86box_switch_handle_report_all(const xiaomi_86box_device_header_t *header,
                                                     const uint8_t *params,
                                                     uint16_t params_len)
{
    uint8_t bits;
    uint8_t index;

    if (params_len < 1U)
    {
        return 1U;
    }

    bits = params[0];

    /*
     * 实测模组按设备逐帧上报: 设备ID 即按键号, 参数1 的 bit0 为该键状态。
     * 例如按下按键 2 收到 01 B1 02 xx 41 01 01 00, 只应更新按键 2。
     */
    if (header->device_id != 0U)
    {
        uint8_t state = (uint8_t)(bits & 0x01U);

        /* 单键上报一律下发: 上电首帧可能是全关(0), 与缓存默认值相同也要通知一次 */
        xiaomi_86box_dispatch_key_state(header->device_id, state);
        xiaomi_86box_cache_set(header->device_type, header->device_id,
                               XIAOMI_86BOX_CACHE_SWITCH, state);
        return 1U;
    }

    /* 设备ID=0: 参数1 视为 1~8 路总状态位图 */
    for (index = 1U; index <= XIAOMI_86BOX_SWITCH_COUNT; index++)
    {
        uint8_t state = (uint8_t)((bits >> (index - 1U)) & 0x01U);
        xiaomi_86box_state_t *cache = xiaomi_86box_state_get(header->device_type, index);

        /* 只对状态发生变化的按键下发, 避免整帧上报时刷出 8 路事件 */
        if ((cache != NULL) && (cache->switch_state == state))
        {
            continue;
        }

        xiaomi_86box_dispatch_key_state(index, state);
        xiaomi_86box_cache_set(header->device_type, index,
                               XIAOMI_86BOX_CACHE_SWITCH, state);
    }

    return 1U;
}

uint8_t xiaomi_86box_device_switch_handle(const xiaomi_86box_device_header_t *header,
                                          uint8_t subcmd,
                                          const uint8_t *params,
                                          uint16_t params_len)
{
    uint8_t key = header->device_id;

    switch (subcmd)
    {
    case XIAOMI_86BOX_SWITCH_SWITCH:
        return xiaomi_86box_switch_handle_switch(header, key, params, params_len);

    case XIAOMI_86BOX_SUBCMD_REPORT_ALL:
        return xiaomi_86box_switch_handle_report_all(header, params, params_len);

    case XIAOMI_86BOX_SWITCH_CHANNEL:
        return xiaomi_86box_switch_handle_channel(header, params, params_len);

    case XIAOMI_86BOX_SWITCH_SETTING:
    {
        uint8_t power_on;
        uint8_t mode;
        uint8_t first_key;
        uint8_t last_key;
        uint8_t index;

        if (params_len < 2U)
        {
            break;
        }

        power_on = params[0];
        mode = params[1];

        if (key == 0U)
        {
            first_key = 1U;
            last_key = XIAOMI_86BOX_SWITCH_KEY_COUNT;
        }
        else
        {
            first_key = key;
            last_key = key;
        }

        for (index = first_key; index <= last_key; index++)
        {
            (void)HOOCH_PROTOCOL_KeyMode_Send(
                (HOOCH_PROTOCOL_KeyModeKey_t)index,
                (HOOCH_PROTOCOL_KeyModePowerOnStatus_t)power_on,
                (HOOCH_PROTOCOL_KeyModeType_t)mode);
            xiaomi_86box_cache_set(header->device_type, index,
                                   XIAOMI_86BOX_CACHE_POWER_ON, power_on);
        }
        break;
    }

    case XIAOMI_86BOX_SWITCH_PAIRING:
        if (params_len >= 2U)
        {
            uint8_t channel = params[0];
            uint8_t action = params[1];

            if (channel == 0U)
            {
                uint8_t index;
                for (index = 1U; index <= XIAOMI_86BOX_SWITCH_KEY_COUNT; index++)
                {
                    /* 开关页 channel: 高 4bit 为按键索引(0-based), 低 4bit 为模式(0=调光) */
                    xiaomi_86box_dispatch_pairing(HOOCH_PROTOCOL_CODE_MATCH_PAGE_SWITCH,
                                                  (uint8_t)((index - 1U) << 4), action);
                }
            }
            else
            {
                xiaomi_86box_dispatch_pairing(HOOCH_PROTOCOL_CODE_MATCH_PAGE_SWITCH,
                                              (uint8_t)((channel - 1U) << 4), action);
            }
        }
        break;

    case XIAOMI_86BOX_SWITCH_INTERLOCK:
        if (params_len >= 1U)
        {
            xiaomi_86box_dispatch_interlock(params[0]);
        }
        break;

    case XIAOMI_86BOX_SWITCH_EVENT:
        if (params_len >= 1U)
        {
            xiaomi_86box_dispatch_key_click(key, params[0]);
        }
        break;

    case XIAOMI_86BOX_SWITCH_INTERLOCK_STAT:
        XIAOMI_86BOX_LOG_INFO("[86BOX] Switch interlock status: %u\r\n",
                              (unsigned int)((params_len > 0U) ? params[0] : 0U));
        break;

    default:
        return 0U;
    }

    return 1U;
}

uint8_t xiaomi_86box_device_switch_report_all(const xiaomi_86box_device_header_t *header,
                                              uint8_t *params,
                                              uint16_t *params_len)
{
    xiaomi_86box_state_t *first = xiaomi_86box_state_get(header->device_type, 1U);
    uint8_t bits = 0U;
    uint8_t index;

    for (index = 0U; index < XIAOMI_86BOX_SWITCH_COUNT; index++)
    {
        xiaomi_86box_state_t *state = xiaomi_86box_state_get(header->device_type,
                                                             (uint8_t)(index + 1U));
        if ((state != NULL) && (state->switch_state != 0U))
        {
            bits |= (uint8_t)(1U << index);
        }
    }

    params[0] = bits;
    params[1] = (first != NULL) ? first->power_on : 0U;
    params[2] = (first != NULL) ? first->mode : 0U;
    *params_len = 3U;

    return 1U;
}
