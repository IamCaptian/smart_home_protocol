/*
 * KNX Air Conditioner Device — Control & Config Summary
 *
 * Panel Communication Protocol V2.4, fun1=2(DEVICE), fun3=1(AIR_CONDITIONER)
 */
#include "knx_internal.h"
   static HOOCH_PROTOCOL_AirConditionerFrame_t air_conditioner_frame;
   static HOOCH_PROTOCOL_SettingFrame_t setting_frame;


/* KNX 模式值映射到 HOOCH 模式枚举
 * KNX:  1=制冷 2=制热 3=除湿 4=送风 5=自动
 * HOOCH: 1=COOL 2=FAN  3=DRY  4=HEAT 5=AUTO
 * 其中 FAN/HEAT 位置互换（2↔4），其余一一对应 */
static HOOCH_PROTOCOL_AirConditionerMode_t knx_air_mode_to_hooch(uint8_t knx_mode)
{
    switch (knx_mode)
    {
    case 1U: return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_COOL;
    case 2U: return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_HEAT;
    case 3U: return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_DRY;
    case 4U: return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_FAN;
    case 5U: return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_AUTO;
    default: return HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_INVALID;
    }
}

/*空调控制处理*/
void knx_summary_air_control(const KNX_Frame_t *frame)
{
    const char *item_desc;
    uint8_t need_set;

    if ((frame == NULL) || (frame->fun_count < 5U))
    {
        return;
    }

    (void)memset(&air_conditioner_frame, 0, sizeof(air_conditioner_frame));
    air_conditioner_frame.channel = frame->fun[1] + 1;
    air_conditioner_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_INVALID;
    item_desc = KNX_DESC("Unknown Control Item");
    need_set = 0U;

    /* 空调控制项：item -> 功能描述 */
    switch (frame->fun[4])
    {
        case 1U:
            item_desc = KNX_DESC("Power (P->K)");
            if (frame->data_len > 0U)
            {
                air_conditioner_frame.power = (HOOCH_PROTOCOL_AirConditionerPower_t)frame->data[0];
                air_conditioner_frame.control_item =
                    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_POWER;
                need_set = 1U;
            }
            break;

        case 2U:
            item_desc = KNX_DESC("Power State (K->P)");
                 
            if (frame->data_len > 0U)
            {
                air_conditioner_frame.power = (HOOCH_PROTOCOL_AirConditionerPower_t)frame->data[0];
                air_conditioner_frame.control_item =
                    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_POWER;
                need_set = 1U;
            }
            break;

        case 3U:
            item_desc = KNX_DESC("Mode (P->K)");
            if (frame->data_len > 0U)
            {
                air_conditioner_frame.mode = knx_air_mode_to_hooch(frame->data[0]);
                air_conditioner_frame.control_item =
                    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MODE;
                need_set = 1U;
            }
            break;

        case 4U:
            item_desc = KNX_DESC("Mode State (K->P)");
            if (frame->data_len > 0U)
            {
                air_conditioner_frame.mode = knx_air_mode_to_hooch(frame->data[0]);
                air_conditioner_frame.control_item =
                    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MODE;
                need_set = 1U;
            }
            break;

        case 5U:
            item_desc = KNX_DESC("Fan Speed (P->K)");
            if (frame->data_len > 0U)
            {
                air_conditioner_frame.fan_speed =
                    (HOOCH_PROTOCOL_AirConditionerFanSpeed_t)frame->data[0];
                air_conditioner_frame.control_item =
                    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_FAN_SPEED;
                need_set = 1U;
            }
            break;

        case 6U:
            item_desc = KNX_DESC("Fan Speed State (K->P)");
            if (frame->data_len > 0U)
            {
                air_conditioner_frame.fan_speed =
                    (HOOCH_PROTOCOL_AirConditionerFanSpeed_t)frame->data[0];
                air_conditioner_frame.control_item =
                    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_FAN_SPEED;
                need_set = 1U;
            }
            break;

        case 7U:
            item_desc = KNX_DESC("Set Temp (P->K)");
            if (frame->data_len >= 2U)
            {
                /* 温度值为 0~500，实际温度 = value * 0.1℃ */
                air_conditioner_frame.temperature = knx_read_be_u16(frame->data);
                air_conditioner_frame.control_item =
                    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMPERATURE;
                need_set = 1U;
            }
            break;

        case 8U:
            item_desc = KNX_DESC("Set Temp State (K->P)");
            if (frame->data_len >= 2U)
            {
                /* 温度值为 0~500，实际温度 = value * 0.1℃ */
                air_conditioner_frame.temperature = knx_read_be_u16(frame->data);
                air_conditioner_frame.control_item =
                    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMPERATURE;
                need_set = 1U;
            }
            break;

        case 9U:
            item_desc = KNX_DESC("Actual Temperature (K<->P)");
            if (frame->data_len >= 2U)
            {
                /* 温度值为 0~500，实际温度 = value * 0.1℃ */
                air_conditioner_frame.current_temperature = knx_read_be_u16(frame->data);
                air_conditioner_frame.control_item =
                    HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_CURRENT_TEMPERATURE;
                need_set = 1U;
            }
            break;

        default:
            break;
    }

    if (need_set != 0U)
    {
        (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&air_conditioner_frame);
    }

    KNX_SUMMARY_EMIT_INDEXED("AC", frame->fun[1], item_desc, frame);
}

/*空调配置处理*/
void knx_summary_air_config(const KNX_Frame_t *frame)
{
    const char *item_desc;
    uint8_t need_set = 0;

    knx_setting_frame_reset(&setting_frame);
    (void)memset(&air_conditioner_frame, 0, sizeof(air_conditioner_frame));
    air_conditioner_frame.channel = frame->fun[1] + 1;
    setting_frame.channel = frame->fun[1] + 1;
    
    if ((frame == NULL) || (frame->fun_count < 5U))
    {
        return;
    }

    item_desc = KNX_DESC("Unknown Config Item");

    /* 空调配置项：item -> 功能描述 */
    switch (frame->fun[4])
    {
    case 1U:
    {
        item_desc = KNX_DESC("Enable Bit (K->P)");
        if ((frame->data[0] & 0x01) == 0x01)
        {
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_ADD_PAGE;
            setting_frame.value = HOOCH_PROTOCOL_SETTING_PAGE_AIR_CONDITIONER;
            setting_frame.page = HOOCH_PROTOCOL_SETTING_PAGE_AIR_CONDITIONER;
        }
        else
        {
            setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_DELETE_PAGE;
            setting_frame.value = HOOCH_PROTOCOL_SETTING_PAGE_AIR_CONDITIONER;
            setting_frame.page = HOOCH_PROTOCOL_SETTING_PAGE_AIR_CONDITIONER;
        }
    air_conditioner_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ENABLE_BIT;
    air_conditioner_frame.value = frame->data[0];
    need_set = 1U;
    }
    break;
    case 2U: item_desc = KNX_DESC("Device Description (K->P)"); 
    air_conditioner_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_DEVICE_DESC;
    memcpy(air_conditioner_frame.device_desc, frame->data, frame->data_len);
    need_set = 2U;
    break;

    case 3U: item_desc = KNX_DESC("Default Icon (K->P)"); 
    air_conditioner_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_DEFAULT_ICON;
    air_conditioner_frame.value = frame->data[0];
    need_set = 2U;
    break;
    case 4U: item_desc = KNX_DESC("Selected Icon (K->P)");
    air_conditioner_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_SELECTED_ICON;
    air_conditioner_frame.value = frame->data[0];
    need_set = 2U;
    break;
    case 5U: item_desc = KNX_DESC("Temp Step (K->P)");
    air_conditioner_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMP_STEP;
    air_conditioner_frame.value = frame->data[0];
    need_set = 2U;
    break;
    case 6U: item_desc = KNX_DESC("Min Set Temp (K->P)");
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_AIR_CONDITIONER_TEMPERATURE_MIN;/* 空调温度下限 */
        if (frame->data_len >= 2U)
        {
            /* 温度值为 0~500，实际温度 = value * 0.1℃ */
            setting_frame.value = knx_read_be_u16(frame->data);

            air_conditioner_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMP_MIN;
            air_conditioner_frame.value = knx_read_be_u16(frame->data);
        }
        need_set = 1U;
    break;
    case 7U: item_desc = KNX_DESC("Max Set Temp (K->P)");
        setting_frame.item = HOOCH_PROTOCOL_SETTING_ITEM_AIR_CONDITIONER_TEMPERATURE_MAX;/* 空调温度上限 */
        if (frame->data_len >= 2U)
        {
            /* 温度值为 0~500，实际温度 = value * 0.1℃ */
            setting_frame.value = knx_read_be_u16(frame->data);
            air_conditioner_frame.control_item = HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMP_MAX;
            air_conditioner_frame.value = knx_read_be_u16(frame->data);
        }
        need_set = 1U;
    break;
    default:
     break;
    }

    if (need_set == 1U)
    {
        (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
        (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&air_conditioner_frame);
    }else if (need_set == 2U)
    {
        (void)HOOCH_PROTOCOL_AirConditioner_DispatchFrame(&air_conditioner_frame);
    }

    KNX_SUMMARY_EMIT_INDEXED("AC", frame->fun[1], item_desc, frame);
}
