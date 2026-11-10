/*
 * Xiaomi 86 Box — 灯光设备处理
 */
#include "xiaomi_86box_device_light.h"
#include "hooch_protocol.h"

static uint8_t xiaomi_86box_light_handle_switch(const xiaomi_86box_device_header_t *header,
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
        /* 切换: 按内部缓存状态取反 */
        state = ((cache != NULL) && (cache->switch_state != 0U)) ? 0U : 1U;
    }

    (void)HOOCH_PROTOCOL_DimmerLight_DispatchSwitch(
        (HOOCH_PROTOCOL_DimmerLightKey_t)key,
        (state != 0U) ? HOOCH_PROTOCOL_DIMMER_LIGHT_SWITCH_ON
                      : HOOCH_PROTOCOL_DIMMER_LIGHT_SWITCH_OFF);
    xiaomi_86box_cache_set(header->device_type, key, XIAOMI_86BOX_CACHE_SWITCH, state);

    return 1U;
}

/* 0x41 上报所有状态: 开关 | 亮度 | 色温(2B, 高字节在前) | R | G | B */
static uint8_t xiaomi_86box_light_handle_report_all(
    const xiaomi_86box_device_header_t *header,
    uint8_t key,
    const uint8_t *params,
    uint16_t params_len)
{
    uint16_t color_temp;

    if (params_len < 7U)
    {
        return 1U;
    }

    color_temp = xiaomi_86box_read_be16(&params[2]);

    (void)HOOCH_PROTOCOL_DimmerLight_DispatchSwitch(
        (HOOCH_PROTOCOL_DimmerLightKey_t)key,
        (params[0] != 0U) ? HOOCH_PROTOCOL_DIMMER_LIGHT_SWITCH_ON
                          : HOOCH_PROTOCOL_DIMMER_LIGHT_SWITCH_OFF);
    (void)HOOCH_PROTOCOL_DimmerLight_DispatchBrightness(
        (HOOCH_PROTOCOL_DimmerLightKey_t)key, params[1]);
    (void)HOOCH_PROTOCOL_DimmerLight_DispatchColorTemperature(
        (HOOCH_PROTOCOL_DimmerLightKey_t)key, color_temp);

    xiaomi_86box_cache_set(header->device_type, key, XIAOMI_86BOX_CACHE_SWITCH, params[0]);
    xiaomi_86box_cache_set(header->device_type, key, XIAOMI_86BOX_CACHE_BRIGHTNESS, params[1]);
    xiaomi_86box_cache_set(header->device_type, key, XIAOMI_86BOX_CACHE_COLOR_TEMP, color_temp);

    /* params[4..6] 为 R/G/B, 暂无对应 HOOCH 接口, 暂不处理 */
    return 1U;
}

uint8_t xiaomi_86box_device_light_handle(const xiaomi_86box_device_header_t *header,
                                         uint8_t subcmd,
                                         const uint8_t *params,
                                         uint16_t params_len)
{
    uint8_t key = header->device_id;

    switch (subcmd)
    {
    case XIAOMI_86BOX_SUBCMD_REPORT_ALL:
        return xiaomi_86box_light_handle_report_all(header, key, params, params_len);

    case XIAOMI_86BOX_LIGHT_SWITCH:
        return xiaomi_86box_light_handle_switch(header, key, params, params_len);

    case XIAOMI_86BOX_LIGHT_BRIGHTNESS:
        if (params_len >= 1U)
        {
            (void)HOOCH_PROTOCOL_DimmerLight_DispatchBrightness(
                (HOOCH_PROTOCOL_DimmerLightKey_t)key, params[0]);
            xiaomi_86box_cache_set(header->device_type, key, XIAOMI_86BOX_CACHE_BRIGHTNESS, params[0]);
        }
        break;

    case XIAOMI_86BOX_LIGHT_COLOR_TEMP:
        if (params_len >= 2U)
        {
            uint16_t color_temp = xiaomi_86box_read_be16(params);
            (void)HOOCH_PROTOCOL_DimmerLight_DispatchColorTemperature(
                (HOOCH_PROTOCOL_DimmerLightKey_t)key, color_temp);
            xiaomi_86box_cache_set(header->device_type, key, XIAOMI_86BOX_CACHE_COLOR_TEMP, color_temp);
        }
        break;

    case XIAOMI_86BOX_LIGHT_RGB:
        /* HOOCH 暂无 RGB 抽象, 仅记录日志 */
        XIAOMI_86BOX_LOG_INFO("[86BOX] Light RGB: %u/%u/%u\r\n",
                              (unsigned int)((params_len > 0U) ? params[0] : 0U),
                              (unsigned int)((params_len > 1U) ? params[1] : 0U),
                              (unsigned int)((params_len > 2U) ? params[2] : 0U));
        break;

    case XIAOMI_86BOX_LIGHT_PAIRING:
        if (params_len >= 1U)
        {
            xiaomi_86box_dispatch_pairing(HOOCH_PROTOCOL_CODE_MATCH_PAGE_LIGHT,
                                          key, params[0]);
        }
        break;

    case XIAOMI_86BOX_LIGHT_EVENT:
        /* 参数1=情景编号, 参数2=单击/双击/长按; 作为按键点击事件下发 */
        if (params_len >= 2U)
        {
            xiaomi_86box_dispatch_key_click(key, params[1]);
        }
        break;

    case XIAOMI_86BOX_LIGHT_POWER_ON:
        if (params_len >= 1U)
        {
            xiaomi_86box_cache_set(header->device_type, key, XIAOMI_86BOX_CACHE_POWER_ON, params[0]);
            xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_KEY_POWER_ON_STATUS,
                                          params[0], HOOCH_PROTOCOL_SETTING_PAGE_INVALID);
        }
        break;

    default:
        return 0U;  /* 未知子命令, 不回复 */
    }

    return 1U;
}

uint8_t xiaomi_86box_device_light_report_all(const xiaomi_86box_device_header_t *header,
                                             uint8_t *params,
                                             uint16_t *params_len)
{
    xiaomi_86box_state_t *state = xiaomi_86box_state_get(header->device_type,
                                                         header->device_id);
    if (state == NULL)
    {
        return 0U;
    }

    /* 开关 + 亮度 + 色温(2字节) + R + G + B */
    params[0] = state->switch_state;
    params[1] = state->brightness;
    params[2] = (uint8_t)(state->color_temp >> 8);
    params[3] = (uint8_t)(state->color_temp);
    params[4] = 0U;
    params[5] = 0U;
    params[6] = 0U;
    *params_len = 7U;

    return 1U;
}
