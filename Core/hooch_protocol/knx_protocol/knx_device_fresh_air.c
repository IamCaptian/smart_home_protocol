/*
 * KNX Fresh Air Device — Control & Config Summary
 *
 * Panel Communication Protocol V2.4, fun1=2(DEVICE), fun3=3(FRESH_AIR)
 */
#include "knx_internal.h"

static HOOCH_PROTOCOL_FreshAirFrame_t fresh_air_frame;
static HOOCH_PROTOCOL_SettingFrame_t setting_frame;

/* KNX 风速值 → HOOCH 风速枚举
 * KNX: 1=低风 2=中风 3=高风；0=关闭由调用方按 power 关闭处理 */
static HOOCH_PROTOCOL_FreshAirFanSpeed_t knx_fresh_air_speed_to_hooch(uint8_t knx_speed)
{
    switch (knx_speed)
    {
    case 1U: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_LOW;
    case 2U: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_MEDIUM;
    case 3U: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_HIGH;
    default: return HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_INVALID;
    }
}

/* KNX 风速模式值 → HOOCH 模式枚举
 * KNX: 4=自动 映射为自动模式，其余非自动值返回无效 */
static HOOCH_PROTOCOL_FreshAirMode_t knx_fresh_air_mode_to_hooch(uint8_t knx_mode)
{
    switch (knx_mode)
    {
    case 4U: return HOOCH_PROTOCOL_FRESH_AIR_MODE_AUTO;
    default: return HOOCH_PROTOCOL_FRESH_AIR_MODE_INVALID;
    }
}

/*新风控制处理*/
void knx_summary_fresh_air_control(const KNX_Frame_t *frame)
{
    const char *item_desc;
    uint8_t need_set;

    if ((frame == NULL) || (frame->fun_count < 5U))
    {
        return;
    }

    (void)memset(&fresh_air_frame, 0, sizeof(fresh_air_frame));
    fresh_air_frame.channel = frame->fun[1] + 1;
    fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_INVALID;
    item_desc = KNX_DESC("Unknown Control Item");
    need_set = 0U;

    switch (frame->fun[4])
    {
        case 1U: /* SWITCH P->K */
            item_desc = KNX_DESC("Power (P->K)");
            if (frame->data_len > 0U)
            {
                fresh_air_frame.power = (HOOCH_PROTOCOL_FreshAirPower_t)frame->data[0];
                fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_POWER;
                need_set = 1U;
            }
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
            if (frame->data_len > 0U)
            {
                if (frame->data[0] == 0U)
                {
                    fresh_air_frame.power = HOOCH_PROTOCOL_FRESH_AIR_POWER_OFF;
                    fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_POWER;
                    need_set = 1U;
                }
                else if (frame->data[0] == 4U)
                {
                    fresh_air_frame.mode = knx_fresh_air_mode_to_hooch(frame->data[0]);
                    fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_MODE;
                    need_set = 1U;
                }
                else
                {
                    fresh_air_frame.fan_speed = knx_fresh_air_speed_to_hooch(frame->data[0]);
                    fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED;
                    need_set = 1U;
                }
            }
            break;

        case 4U: /* FAN_SPEED_MODE_STATUS K->P：0=关闭/1=低风/2=中风/3=高风/4=自动 */
            item_desc = KNX_DESC("Fan Speed/Mode State (K->P)");
            if (frame->data_len > 0U)
            {
                if (frame->data[0] == 0U)
                {
                    fresh_air_frame.power = HOOCH_PROTOCOL_FRESH_AIR_POWER_OFF;
                    fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_POWER;
                    need_set = 1U;
                }
                else if (frame->data[0] == 4U)
                {
                    fresh_air_frame.mode = knx_fresh_air_mode_to_hooch(frame->data[0]);
                    fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_MODE;
                    need_set = 1U;
                }
                else
                {
                    fresh_air_frame.fan_speed = knx_fresh_air_speed_to_hooch(frame->data[0]);
                    fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED;
                    need_set = 1U;
                }
            }
            break;

        case 5U: /* ACTUAL_FAN_SPEED K<->P：0=关闭/1=低风/2=中风/3=高风 */
            item_desc = KNX_DESC("Actual Fan Speed (K<->P)");
            if (frame->data_len > 0U)
            {
                if (frame->data[0] == 0U)
                {
                    fresh_air_frame.power = HOOCH_PROTOCOL_FRESH_AIR_POWER_OFF;
                    fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_POWER;
                    need_set = 1U;
                }
                else
                {
                    fresh_air_frame.fan_speed = knx_fresh_air_speed_to_hooch(frame->data[0]);
                    fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED;
                    need_set = 1U;
                }
            }
            break;

        case 6U: /* ACTUAL_TEMPERATURE：映射到 HOOCH current_temperature */
            item_desc = KNX_DESC("Actual Temperature (K<->P)");
            if (frame->data_len >= 2U)
            {
                /* 温度值为 0~500，实际温度 = value * 0.1℃ */
                fresh_air_frame.current_temperature = (uint8_t)(knx_read_be_u16(frame->data) / 10U);
                fresh_air_frame.control_item = HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_CURRENT_TEMPERATURE;
                need_set = 1U;
            }
            break;

        default:
            break;
    }

    if (need_set != 0U)
    {
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
    }
    else if (need_set == 2U)
    {
        (void)HOOCH_PROTOCOL_FreshAir_DispatchFrame(&fresh_air_frame);
    }

    KNX_SUMMARY_EMIT_INDEXED("FA", frame->fun[1], item_desc, frame);
}
