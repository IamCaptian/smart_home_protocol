/*
 * Xiaomi 86 Box — 设备处理公共层
 */
#include "xiaomi_86box_device_common.h"
#include "hooch_protocol.h"
#include "hooch_protocol_common.h"
#include <string.h>

static xiaomi_86box_state_t s_light[XIAOMI_86BOX_LIGHT_COUNT];
static xiaomi_86box_state_t s_switch[XIAOMI_86BOX_SWITCH_COUNT];
static xiaomi_86box_state_t s_curtain[XIAOMI_86BOX_CURTAIN_COUNT];
static xiaomi_86box_state_t s_air_conditioner;
static xiaomi_86box_state_t s_fresh_air;
static xiaomi_86box_state_t s_floor_heating;

/* 天气码缓存(由 0x11 上报天气更新, 0x0B 获取天气时回填) */
static uint32_t s_weather_code;

/* ============================ 通用工具 ============================ */

uint16_t xiaomi_86box_read_be16(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[0] << 8) | (uint16_t)data[1]);
}

uint32_t xiaomi_86box_read_be32(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) | (uint32_t)data[3];
}

uint8_t xiaomi_86box_is_light_type(uint8_t device_type)
{
    return (device_type == XIAOMI_86BOX_DEV_WY_LIGHT ||
            device_type == XIAOMI_86BOX_DEV_RGB_LIGHT ||
            device_type == XIAOMI_86BOX_DEV_SINGLE_LIGHT ||
            device_type == XIAOMI_86BOX_DEV_WY_RGB_LIGHT) ? 1U : 0U;
}

uint8_t xiaomi_86box_is_switch_type(uint8_t device_type)
{
    return (device_type == XIAOMI_86BOX_DEV_RELAY_SWITCH ||
            device_type == XIAOMI_86BOX_DEV_86_SWITCH) ? 1U : 0U;
}

xiaomi_86box_state_t *xiaomi_86box_state_get(uint8_t device_type, uint8_t device_id)
{
    if (xiaomi_86box_is_light_type(device_type) != 0U)
    {
        if ((device_id >= 1U) && (device_id <= XIAOMI_86BOX_LIGHT_COUNT))
        {
            return &s_light[device_id - 1U];
        }
        return NULL;
    }

    if (xiaomi_86box_is_switch_type(device_type) != 0U)
    {
        if ((device_id >= 1U) && (device_id <= XIAOMI_86BOX_SWITCH_COUNT))
        {
            return &s_switch[device_id - 1U];
        }
        return NULL;
    }

    if (device_type == XIAOMI_86BOX_DEV_CURTAIN)
    {
        if ((device_id >= 1U) && (device_id <= XIAOMI_86BOX_CURTAIN_COUNT))
        {
            return &s_curtain[device_id - 1U];
        }
        return NULL;
    }

    if (device_type == XIAOMI_86BOX_DEV_AIR_CONDITIONER)
    {
        return &s_air_conditioner;
    }

    if (device_type == XIAOMI_86BOX_DEV_FRESH_AIR)
    {
        return &s_fresh_air;
    }

    if (device_type == XIAOMI_86BOX_DEV_FLOOR_HEATING)
    {
        return &s_floor_heating;
    }

    return NULL;
}

void xiaomi_86box_cache_set(uint8_t device_type,
                            uint8_t device_id,
                            uint8_t field,
                            uint16_t value)
{
    xiaomi_86box_state_t *state = xiaomi_86box_state_get(device_type, device_id);

    if (state == NULL)
    {
        return;
    }

    switch (field)
    {
    case XIAOMI_86BOX_CACHE_SWITCH:         state->switch_state = (uint8_t)value;  break;
    case XIAOMI_86BOX_CACHE_BRIGHTNESS:     state->brightness = (uint8_t)value;    break;
    case XIAOMI_86BOX_CACHE_COLOR_TEMP:     state->color_temp = value;             break;
    case XIAOMI_86BOX_CACHE_PERCENT:        state->percent = (uint8_t)value;       break;
    case XIAOMI_86BOX_CACHE_SET_TRAVEL:     state->set_travel = (uint8_t)value;    break;
    case XIAOMI_86BOX_CACHE_CURRENT_TRAVEL: state->current_travel = (uint8_t)value;break;
    case XIAOMI_86BOX_CACHE_POWER:          state->power = (uint8_t)value;         break;
    case XIAOMI_86BOX_CACHE_MODE:           state->mode = (uint8_t)value;          break;
    case XIAOMI_86BOX_CACHE_FAN_SPEED:      state->fan_speed = (uint8_t)value;     break;
    case XIAOMI_86BOX_CACHE_TEMPERATURE:    state->temperature = value;            break;
    case XIAOMI_86BOX_CACHE_POWER_ON:       state->power_on = (uint8_t)value;      break;
    default: break;
    }
}

void xiaomi_86box_set_weather_code(uint32_t weather_code)
{
    s_weather_code = weather_code;
}

uint32_t xiaomi_86box_get_weather_code(void)
{
    return s_weather_code;
}

/* ============================ HOOCH 下发 ============================ */

static void xiaomi_86box_setting_reset(HOOCH_PROTOCOL_SettingFrame_t *frame)
{
    if (frame == NULL)
    {
        return;
    }

    memset(frame, 0, sizeof(*frame));
    frame->page = HOOCH_PROTOCOL_SETTING_PAGE_INVALID;
    frame->screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_INVALID;
    frame->param1 = 0xFFU;
    frame->param2 = 0xFFU;
    frame->param3 = 0xFFU;
    frame->param4 = 0xFFU;
    frame->item = HOOCH_PROTOCOL_SETTING_ITEM_INVALID;
    frame->source = HOOCH_PROTOCOL_SOURCE_XIAOMI;
}

void xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SettingItem_t item,
                                   uint32_t value,
                                   HOOCH_PROTOCOL_SettingPage_t page)
{
    HOOCH_PROTOCOL_SettingFrame_t frame;

    xiaomi_86box_setting_reset(&frame);
    frame.item = item;
    frame.value = value;
    frame.page = page;
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&frame);
}

void xiaomi_86box_dispatch_screen_off_time(uint32_t seconds)
{
    HOOCH_PROTOCOL_SettingFrame_t frame;

    xiaomi_86box_setting_reset(&frame);
    frame.item = HOOCH_PROTOCOL_SETTING_ITEM_SCREEN_OFF_TIME;
    frame.value = seconds;

    if (seconds == 0xFFFFFFFFUL)
    {
        frame.screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_NEVER;
    }
    else if (seconds == 0U)
    {
        frame.screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_NEVER;
    }
    else
    {
        frame.screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_VALUE;
    }

    (void)HOOCH_PROTOCOL_Setting_SetFrame(&frame);
}

void xiaomi_86box_dispatch_interlock(uint8_t value)
{
    xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_INTERLOCK_FUNCTION,
                                  value,
                                  HOOCH_PROTOCOL_SETTING_PAGE_INVALID);
}

void xiaomi_86box_dispatch_pairing(uint8_t page,
                                   uint8_t channel,
                                   uint8_t raw_action)
{
    HOOCH_PROTOCOL_CodeMatchReportAction_t action =
        (raw_action != 0U) ? HOOCH_PROTOCOL_CODE_MATCH_REPORT_ACTION_MATCH
                           : HOOCH_PROTOCOL_CODE_MATCH_REPORT_ACTION_CLEAR;

    (void)HOOCH_PROTOCOL_CodeMatchDispatch_Dispatch(page, channel, action);
}

void xiaomi_86box_dispatch_key_state(uint8_t key, uint8_t state)
{
    HOOCH_PROTOCOL_KeyStatusFrame_t frame;

    if ((key == 0U) || (key > HOOCH_PROTOCOL_KEY_STATUS_KEY_COUNT))
    {
        return;
    }

    memset(&frame, 0, sizeof(frame));
    frame.key = (HOOCH_PROTOCOL_KeyStatusKey_t)key;
    frame.state = (state != 0U) ? HOOCH_PROTOCOL_KEY_STATUS_STATE_ON
                                : HOOCH_PROTOCOL_KEY_STATUS_STATE_OFF;
    frame.control_item = HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_STATE;
    frame.source = HOOCH_PROTOCOL_SOURCE_XIAOMI;

    (void)HOOCH_PROTOCOL_KeyStatus_DispatchFrame(&frame);
}

static HOOCH_PROTOCOL_KeyClickReportEvent_t xiaomi_86box_click_event(uint8_t raw)
{
    switch (raw)
    {
    case 0U: return HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_SINGLE_CLICK;
    case 1U: return HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_DOUBLE_CLICK;
    case 2U: return HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_LONG_CLICK;
    default: return HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_NONE;
    }
}

void xiaomi_86box_dispatch_key_click(uint8_t key, uint8_t raw_click)
{
    HOOCH_PROTOCOL_KeyClickReportFrame_t frame;
    HOOCH_PROTOCOL_KeyClickReportEvent_t event = xiaomi_86box_click_event(raw_click);

    if ((key == 0U) || (key > HOOCH_PROTOCOL_KEY_CLICK_REPORT_KEY_COUNT) ||
        (event == HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_NONE))
    {
        return;
    }

    memset(&frame, 0, sizeof(frame));
    frame.key = (HOOCH_PROTOCOL_KeyClickReportKey_t)key;
    frame.event = event;
    frame.state = HOOCH_PROTOCOL_KEY_CLICK_REPORT_STATE_ON;

    (void)HOOCH_PROTOCOL_KeyClickReport_SetFrame(&frame);
}

HOOCH_PROTOCOL_AirConditionerFanSpeed_t xiaomi_86box_fan_speed_from_raw(uint8_t raw)
{
    switch (raw)
    {
    case 1U: return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_AUTO;
    case 2U: return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_LOW;
    case 3U: return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_MEDIUM;
    case 4U: return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_HIGH;
    default: return HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_INVALID;
    }
}

/* 空调温度: 0x10~0x20 表示 16~32℃(步长 1℃), 0x2A~0x4A 表示 0.5℃ 步长 */
uint16_t xiaomi_86box_ac_temperature_from_raw(uint8_t raw)
{
    if (raw >= 0x2AU)
    {
        return (uint16_t)(((uint16_t)raw - 10U) / 2U);
    }
    return raw;
}

/* 地暖温度: 0x05~0x28 表示 5~40℃(步长 1℃), 0x32~0x78 表示 0.5℃ 步长 */
uint16_t xiaomi_86box_floor_temperature_from_raw(uint8_t raw)
{
    if (raw >= 0x32U)
    {
        return (uint16_t)(((uint16_t)raw - 40U) / 2U);
    }
    return raw;
}

HOOCH_PROTOCOL_SettingPage_t xiaomi_86box_page_from_device_type(uint8_t device_type)
{
    if (xiaomi_86box_is_light_type(device_type) != 0U)
    {
        return HOOCH_PROTOCOL_SETTING_PAGE_LIGHT;
    }

    switch (device_type)
    {
    case XIAOMI_86BOX_DEV_RELAY_SWITCH:
    case XIAOMI_86BOX_DEV_86_SWITCH:      return HOOCH_PROTOCOL_SETTING_PAGE_SWITCH;
    case XIAOMI_86BOX_DEV_SCENE_SWITCH:   return HOOCH_PROTOCOL_SETTING_PAGE_SCENE;
    case XIAOMI_86BOX_DEV_CURTAIN:        return HOOCH_PROTOCOL_SETTING_PAGE_CURTAIN;
    case XIAOMI_86BOX_DEV_AIR_CONDITIONER:return HOOCH_PROTOCOL_SETTING_PAGE_AIR_CONDITIONER;
    case XIAOMI_86BOX_DEV_FRESH_AIR:      return HOOCH_PROTOCOL_SETTING_PAGE_FRESH_AIR;
    case XIAOMI_86BOX_DEV_FLOOR_HEATING:  return HOOCH_PROTOCOL_SETTING_PAGE_FLOOR_HEATING;
    default:                              return HOOCH_PROTOCOL_SETTING_PAGE_INVALID;
    }
}

HOOCH_PROTOCOL_KeyNamePage_t xiaomi_86box_name_page_from_device_type(uint8_t device_type)
{
    if (xiaomi_86box_is_light_type(device_type) != 0U)
    {
        return HOOCH_PROTOCOL_KEY_NAME_PAGE_LIGHT;
    }

    switch (device_type)
    {
    case XIAOMI_86BOX_DEV_RELAY_SWITCH:
    case XIAOMI_86BOX_DEV_86_SWITCH:    return HOOCH_PROTOCOL_KEY_NAME_PAGE_SWITCH;
    case XIAOMI_86BOX_DEV_SCENE_SWITCH: return HOOCH_PROTOCOL_KEY_NAME_PAGE_SCENE;
    case XIAOMI_86BOX_DEV_CURTAIN:      return HOOCH_PROTOCOL_KEY_NAME_PAGE_CURTAIN;
    default:                            return HOOCH_PROTOCOL_KEY_NAME_PAGE_INVALID;
    }
}
