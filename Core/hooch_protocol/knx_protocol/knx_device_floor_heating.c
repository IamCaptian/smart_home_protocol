/*
 * KNX Floor Heating Device — Control & Config Summary
 *
 * Panel Communication Protocol V2.4, fun1=2(DEVICE), fun3=2(FLOOR_HEATING)
 */
#include "knx_internal.h"

static HOOCH_PROTOCOL_FloorHeatingFrame_t floor_heating_frame;
static HOOCH_PROTOCOL_SettingFrame_t setting_frame;

/* KNX 手动/自动映射到 HOOCH 地暖模式
 * KNX:   0=手动 1=自动
 * HOOCH: 1=MANUAL 2=AUTO (3=ECO 不参与 KNX 两态映射) */
static HOOCH_PROTOCOL_FloorHeatingMode_t knx_floor_mode_to_hooch(uint8_t knx_mode)
{
    switch (knx_mode)
    {
    case 0U: return HOOCH_PROTOCOL_FLOOR_HEATING_MODE_MANUAL;
    case 1U: return HOOCH_PROTOCOL_FLOOR_HEATING_MODE_AUTO;
    default: return HOOCH_PROTOCOL_FLOOR_HEATING_MODE_INVALID;
    }
}

/*地暖控制处理*/
void knx_summary_floor_heating_control(const KNX_Frame_t *frame)
{
    const char *item_desc;
    uint8_t need_set;

    if ((frame == NULL) || (frame->fun_count < 5U))
    {
        return;
    }

    (void)memset(&floor_heating_frame, 0, sizeof(floor_heating_frame));
    floor_heating_frame.channel = frame->fun[1] + 1;
    floor_heating_frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_INVALID;
    item_desc = KNX_DESC("Unknown Control Item");
    need_set = 0U;

    switch (frame->fun[4])
    {
        case 1U: /* SWITCH P->K */
            item_desc = KNX_DESC("Power (P->K)");
            if (frame->data_len > 0U)
            {
                floor_heating_frame.power = (HOOCH_PROTOCOL_FloorHeatingPower_t)frame->data[0];
                floor_heating_frame.control_item =
                    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_POWER;
                need_set = 1U;
            }
            break;

        case 2U: /* SWITCH_STATUS K->P */
            item_desc = KNX_DESC("Power State (K->P)");
            if (frame->data_len > 0U)
            {
                floor_heating_frame.power = (HOOCH_PROTOCOL_FloorHeatingPower_t)frame->data[0];
                floor_heating_frame.control_item =
                    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_POWER;
                need_set = 1U;
            }
            break;

        case 3U: /* SET_TEMPERATURE P->K */
            item_desc = KNX_DESC("Set Temp (P->K)");
            if (frame->data_len >= 2U)
            {
                /* 温度值为 0~500，实际温度 = value * 0.1℃ */
                floor_heating_frame.target_temperature = (uint8_t)(knx_read_be_u16(frame->data) / 10U);
                floor_heating_frame.control_item =
                    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TARGET_TEMPERATURE;
                need_set = 1U;
            }
            break;

        case 4U: /* SET_TEMPERATURE_STATUS K->P */
            item_desc = KNX_DESC("Set Temp State (K->P)");
            if (frame->data_len >= 2U)
            {
                /* 温度值为 0~500，实际温度 = value * 0.1℃ */
                floor_heating_frame.target_temperature = (uint8_t)(knx_read_be_u16(frame->data) / 10U);
                floor_heating_frame.control_item =
                    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TARGET_TEMPERATURE;
                need_set = 1U;
            }
            break;

        case 5U: /* ACTUAL_TEMPERATURE K<->P */
            item_desc = KNX_DESC("Actual Temperature (K<->P)");
            if (frame->data_len >= 2U)
            {
                /* 温度值为 0~500，实际温度 = value * 0.1℃ */
                floor_heating_frame.current_temperature = (uint8_t)(knx_read_be_u16(frame->data) / 10U);
                floor_heating_frame.control_item =
                    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_CURRENT_TEMPERATURE;
                need_set = 1U;
            }
            break;

        case 6U: /* MANUAL_AUTO P->K */
            item_desc = KNX_DESC("Manual/Auto (P->K)");
            if (frame->data_len > 0U)
            {
                floor_heating_frame.mode = knx_floor_mode_to_hooch(frame->data[0]);
                floor_heating_frame.control_item =
                    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_MODE;
                need_set = 1U;
            }
            break;

        case 7U: /* MANUAL_AUTO_STATUS K->P */
            item_desc = KNX_DESC("Manual/Auto State (K->P)");
            if (frame->data_len > 0U)
            {
                floor_heating_frame.mode = knx_floor_mode_to_hooch(frame->data[0]);
                floor_heating_frame.control_item =
                    HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_MODE;
                need_set = 1U;
            }
            break;

        case 8U: /* RELAY_SWITCH P->K：无对应 HOOCH 字段，暂不处理 */
            item_desc = KNX_DESC("Relay Switch (P->K)");
            break;

        case 9U: /* RELAY_SWITCH_STATUS K->P：无对应 HOOCH 字段，暂不处理 */
            item_desc = KNX_DESC("Relay Switch State (K->P)");
            break;

        default:
            break;
    }

    if (need_set != 0U)
    {
        (void)HOOCH_PROTOCOL_FloorHeating_DispatchFrame(&floor_heating_frame);
    }

    KNX_SUMMARY_EMIT_INDEXED("FH", frame->fun[1], item_desc, frame);
}

/*地暖配置处理*/
void knx_summary_floor_heating_config(const KNX_Frame_t *frame)
{
    const char *item_desc;
    uint8_t need_set = 0;

    knx_setting_frame_reset(&setting_frame);
    (void)memset(&floor_heating_frame, 0, sizeof(floor_heating_frame));
    floor_heating_frame.channel = frame->fun[1] + 1;
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
            setting_frame.value = HOOCH_PROTOCOL_SETTING_PAGE_FLOOR_HEATING;
            setting_frame.page = HOOCH_PROTOCOL_SETTING_PAGE_FLOOR_HEATING;
        }
        else
        {
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_DELETE_PAGE;
            setting_frame.value = HOOCH_PROTOCOL_SETTING_PAGE_FLOOR_HEATING;
            setting_frame.page = HOOCH_PROTOCOL_SETTING_PAGE_FLOOR_HEATING;
        }
        need_set = 1U;
    }
    break;

    case 2U: /* DESCRIPTION */
        item_desc = KNX_DESC("Device Description (K->P)");
        floor_heating_frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_DEVICE_DESC;
        memcpy(floor_heating_frame.device_desc, frame->data, frame->data_len);
        need_set = 2U;
        break;

    case 3U: /* DEFAULT_ICON */
        item_desc = KNX_DESC("Default Icon (K->P)");
        floor_heating_frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_DEFAULT_ICON;
        floor_heating_frame.value = frame->data[0];
        need_set = 2U;
        break;

    case 4U: /* SELECTED_ICON */
        item_desc = KNX_DESC("Selected Icon (K->P)");
        floor_heating_frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_SELECTED_ICON;
        floor_heating_frame.value = frame->data[0];
        need_set = 2U;
        break;

    case 5U: /* TEMP_STEP */
        item_desc = KNX_DESC("Temp Step (K->P)");
        floor_heating_frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TEMP_STEP;
        floor_heating_frame.value = frame->data[0];
        need_set = 2U;
        break;

    case 6U: /* TEMP_MIN */
        item_desc = KNX_DESC("Min Set Temp (K->P)");
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_FLOOR_HEATING_TEMPERATURE_MIN;
        if (frame->data_len >= 2U)
        {
            /* 温度值为 0~500，实际温度 = value * 0.1℃ */
            setting_frame.value = (uint8_t)(knx_read_be_u16(frame->data) / 10U);

            floor_heating_frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TEMP_MIN;
            floor_heating_frame.value = (uint8_t)(knx_read_be_u16(frame->data) / 10U);
        }
        need_set = 3U;
        break;

    case 7U: /* TEMP_MAX */
        item_desc = KNX_DESC("Max Set Temp (K->P)");
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_FLOOR_HEATING_TEMPERATURE_MAX;
        if (frame->data_len >= 2U)
        {
            /* 温度值为 0~500，实际温度 = value * 0.1℃ */
            setting_frame.value = (uint8_t)(knx_read_be_u16(frame->data) / 10U);

            floor_heating_frame.control_item = HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TEMP_MAX;
            floor_heating_frame.value = (uint8_t)(knx_read_be_u16(frame->data) / 10U);
        }
        need_set = 3U;
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
        (void)HOOCH_PROTOCOL_FloorHeating_DispatchFrame(&floor_heating_frame);
    }
    else if (need_set == 3U)
    {
        /* 温度上下限：同时下发设置与地暖字段（与空调一致） */
        (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
        (void)HOOCH_PROTOCOL_FloorHeating_DispatchFrame(&floor_heating_frame);
    }

    KNX_SUMMARY_EMIT_INDEXED("FH", frame->fun[1], item_desc, frame);
}
