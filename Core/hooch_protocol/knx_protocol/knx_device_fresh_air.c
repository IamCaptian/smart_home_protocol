/*
 * KNX Fresh Air Device — Control & Config Summary
 *
 * Panel Communication Protocol V2.4, fun1=2(DEVICE), fun3=3(FRESH_AIR)
 */
#include "knx_internal.h"

static HOOCH_PROTOCOL_FreshAirFrame_t fresh_air_frame;
static HOOCH_PROTOCOL_SettingFrame_t setting_frame;

/* KNX 风速字节 → HOOCH 风速枚举
 * KNX: 0=关闭 1=低风 2=中风 3=高风 4=自动 */
static HOOCH_PROTOCOL_FreshAirFanSpeed_t knx_fresh_air_speed_to_hooch(uint8_t knx_speed)
{
    switch (knx_speed)
    {
    case 0U: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_OFF;
    case 1U: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_LOW;
    case 2U: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_MEDIUM;
    case 3U: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_HIGH;
    case 4U: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_AUTO;
    default: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID;
    }
}



/*新风控制处理*/
void knx_summary_fresh_air_control(const KNX_Frame_t *frame)
{
    const char *item_desc;
    uint8_t need_set;
    uint16_t fan_speed_raw;
    uint8_t has_fan_speed_raw;
    uint16_t temperature_raw;
    uint8_t has_temperature_raw;

    if ((frame == NULL) || (frame->fun_count < 5U))
    {
        return;
    }

    (void)memset(&fresh_air_frame, 0, sizeof(fresh_air_frame));
    fresh_air_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;
    fresh_air_frame.channel = frame->fun[1] + 1;
    fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_INVALID;
    item_desc = KNX_DESC("Unknown Control Item");
    need_set = 0U;
    fan_speed_raw = 0U;
    has_fan_speed_raw = 0U;
    temperature_raw = 0U;
    has_temperature_raw = 0U;

    switch (frame->fun[4])
    {
        case 1U: /* SWITCH P->K */
            item_desc = KNX_DESC("Power (P->K)");
            break;

        case 2U: /* SWITCH_STATUS K->P */
            item_desc = KNX_DESC("Power State (K->P)");
            if (frame->data_len > 0U)
            {
                fresh_air_frame.power = (HOOCH_PROTOCOL_FreshAirPower_t)frame->data[0];
                fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_POWER;
                need_set = 1U;
            }
            break;

        case 3U: /* FAN_SPEED_MODE P->K：0=关闭/1=低风/2=中风/3=高风/4=自动 */
            item_desc = KNX_DESC("Fan Speed/Mode (P->K)");
            break;

        case 4U: /* FAN_SPEED_MODE_STATUS K->P：0=关闭/1=低风/2=中风/3=高风/4=自动 */
            item_desc = KNX_DESC("Fan Speed/Mode State (K->P)");
            if (frame->data_len > 0U)
            {
                /* 先按风速枚举下发一次(0=关闭/1=低风/2=中风/3=高风/4=自动) */
                fresh_air_frame.fan_speed = knx_fresh_air_speed_to_hooch(frame->data[0]);
                if (fresh_air_frame.fan_speed != HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID)
                {
                    fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED;
                    need_set = 1U;
                }

                /* 原始值再透传一次 */
                fan_speed_raw = frame->data[0];
                has_fan_speed_raw = 1U;
            }
            break;

        case 5U: /* ACTUAL_FAN_SPEED K<->P：0=关闭/1=低风/2=中风/3=高风 */
            item_desc = KNX_DESC("Actual Fan Speed (K<->P)");
            if (frame->data_len > 0U)
            {
                /* 先按风速枚举下发一次(0=关闭/1=低风/2=中风/3=高风) */
                fresh_air_frame.fan_speed = knx_fresh_air_speed_to_hooch(frame->data[0]);
                if (fresh_air_frame.fan_speed != HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID)
                {
                    fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED;
                    need_set = 1U;
                }

                /* 原始值再透传一次 */
                fan_speed_raw = frame->data[0];
                has_fan_speed_raw = 1U;
            }
            break;

        case 6U: /* ACTUAL_TEMPERATURE：映射到 HOOCH current_temperature */
            item_desc = KNX_DESC("Actual Temperature (K<->P)");
            if (frame->data_len >= 2U)
            {
                /* 温度值为 0~500，实际温度 = value * 0.1℃ (四舍五入到整数℃) */
                fresh_air_frame.current_temperature = (uint8_t)((knx_read_be_u16(frame->data) + 5U) / 10U);
                fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_CURRENT_TEMPERATURE;
                need_set = 1U;

                /* 同时记录温度原始值(0~500)透传下去 */
                temperature_raw = knx_read_be_u16(frame->data);
                has_temperature_raw = 1U;
            }
            break;

        default:
            break;
    }

    if (need_set != 0U)
    {
        (void)HOOCH_PROTOCOL_FreshAir_DispatchFrame(&fresh_air_frame);
    }

    /* 风速字节原值额外透传一次(0=关闭/1=低风/2=中风/3=高风/4=自动)，供下游按原值处理 */
    if (has_fan_speed_raw != 0U)
    {
        fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED_RAW;
        fresh_air_frame.value = fan_speed_raw;
        (void)HOOCH_PROTOCOL_FreshAir_DispatchFrame(&fresh_air_frame);
    }

    /* 温度原始值额外透传一次(0~500)，供下游按原值处理 */
    if (has_temperature_raw != 0U)
    {
        fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_CURRENT_TEMPERATURE_RAW;
        fresh_air_frame.value = temperature_raw;
        (void)HOOCH_PROTOCOL_FreshAir_DispatchFrame(&fresh_air_frame);
    }

    KNX_SUMMARY_EMIT_INDEXED("FA", frame->fun[1], item_desc, frame);
}

/*新风配置处理*/
void knx_summary_fresh_air_config(const KNX_Frame_t *frame)
{
    const char *item_desc;
    uint8_t need_set = 0;

    knx_setting_frame_reset(&setting_frame);
    (void)memset(&fresh_air_frame, 0, sizeof(fresh_air_frame));
    fresh_air_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;
    fresh_air_frame.channel = frame->fun[1] + 1;
    setting_frame.channel = frame->fun[1] + 1;

    if ((frame == NULL) || (frame->fun_count < 5U))
    {
        return;
    }

    item_desc = KNX_DESC("Unknown Config Item");

    switch (frame->fun[4])
    {
    case 1U: /* ENABLE_BITMAP */
    {
        item_desc = KNX_DESC("Enable Bit (K->P)");
        if ((frame->data[0] & 0x01) == 0x01)
        {
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_ADD_PAGE;
            setting_frame.value = HOOCH_PROTOCOL_SETTING_PAGE_FRESH_AIR;
            setting_frame.page = HOOCH_PROTOCOL_SETTING_PAGE_FRESH_AIR;
        }
        else
        {
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_DELETE_PAGE;
            setting_frame.value = HOOCH_PROTOCOL_SETTING_PAGE_FRESH_AIR;
            setting_frame.page = HOOCH_PROTOCOL_SETTING_PAGE_FRESH_AIR;
        }
        /* 使能位原始位图同步下发(bit0:使能 bit1:开关 bit2:模式 bit3:风速 bit4:当前温度) */
        fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ENABLE_BIT;
        fresh_air_frame.value = frame->data[0];
        need_set = 1U;
    }
    break;

    case 2U: /* DESCRIPTION */
        item_desc = KNX_DESC("Device Description (K->P)");
        fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_DEVICE_DESC;
        memcpy(fresh_air_frame.device_desc, frame->data, frame->data_len);
        need_set = 2U;
        break;

    case 3U: /* DEFAULT_ICON */
        item_desc = KNX_DESC("Default Icon (K->P)");
        fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_DEFAULT_ICON;
        fresh_air_frame.value = frame->data[0];
        need_set = 2U;
        break;

    case 4U: /* SELECTED_ICON */
        item_desc = KNX_DESC("Selected Icon (K->P)");
        fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_SELECTED_ICON;
        fresh_air_frame.value = frame->data[0];
        need_set = 2U;
        break;

    default:
        break;
    }

    if (need_set == 1U)
    {
        (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
        (void)HOOCH_PROTOCOL_FreshAir_DispatchFrame(&fresh_air_frame);
    }
    else if (need_set == 2U)
    {
        (void)HOOCH_PROTOCOL_FreshAir_DispatchFrame(&fresh_air_frame);
    }

    KNX_SUMMARY_EMIT_INDEXED("FA", frame->fun[1], item_desc, frame);
}
