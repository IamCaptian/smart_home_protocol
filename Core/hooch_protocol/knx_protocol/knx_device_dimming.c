/*
 * KNX Dimming Device — Control & Config Summary
 *
 * Panel Communication Protocol V2.4, fun1=2(DEVICE), fun3=6(DIMMING)
 */
#include "knx_internal.h"
#include "hooch_dimmer_light.h"
#include "hooch_key_mode.h"
#include "hooch_key_name.h"
HOOCH_PROTOCOL_DimmerLightFrame_t dimmer_light_frame;
/*调光控制处理*/
void knx_summary_dimming_control(const KNX_Frame_t *frame)
{
    const char *item_desc;
    uint8_t should_update_dimmer_light;
    if ((frame == NULL) || (frame->fun_count < 5U))
    {
        return;
    }

    (void)memset(&dimmer_light_frame, 0, sizeof(dimmer_light_frame));
    should_update_dimmer_light = 0U;
    item_desc = KNX_DESC("Unknown Control Item");

    /* 调光控制项：item -> 功能描述 */
    switch (frame->fun[4])
    {
    case 1U: item_desc = KNX_DESC("Switch (P->K)"); break;
    case 2U: item_desc = KNX_DESC("Switch State (K->P)");
    {
    dimmer_light_frame.key = (HOOCH_PROTOCOL_DimmerLightKey_t)(frame->fun[1] + 1U);
    dimmer_light_frame.control_item = HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_SWITCH;
    dimmer_light_frame.switch_state = frame->data[0];
    dimmer_light_frame.valid = 1U;
    should_update_dimmer_light = 1U;
    break;
    }
    case 3U: item_desc = KNX_DESC("Brightness (P->K)"); break;
    case 4U: item_desc = KNX_DESC("Brightness State (K->P)");
    {
    dimmer_light_frame.key = (HOOCH_PROTOCOL_DimmerLightKey_t)(frame->fun[1] + 1U);
    dimmer_light_frame.control_item = HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_BRIGHTNESS;
    dimmer_light_frame.brightness = (uint8_t)(((uint32_t)frame->data[0] * 100U) / 255U);
    dimmer_light_frame.valid = 1U;
    should_update_dimmer_light = 1U;
    break;
    }
    case 5U: item_desc = KNX_DESC("Color Temp (P->K)"); break;
    case 6U: item_desc = KNX_DESC("Color Temp State (K->P)");
    {
    dimmer_light_frame.key = (HOOCH_PROTOCOL_DimmerLightKey_t)(frame->fun[1] + 1U);
    dimmer_light_frame.control_item = HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_COLOR_TEMP;
    dimmer_light_frame.color_temperature = (frame->data_len >= 2U)
                                               ? (uint8_t)(((uint32_t)knx_read_be_u16(frame->data) * 100U) / 65535U)
                                               : 0U;
    dimmer_light_frame.valid = 1U;
    should_update_dimmer_light = 1U;
    break;
    }
    case 7U: item_desc = KNX_DESC("RGBW(P->K)"); break;
    case 8U: item_desc = KNX_DESC("RGBW State (K->P)"); break;
    case 9U: item_desc = KNX_DESC("Color Temp Percent (P->K)"); break;
    case 10U: item_desc = KNX_DESC("Color Temp Percent State (K->P)");
    {
    dimmer_light_frame.key = (HOOCH_PROTOCOL_DimmerLightKey_t)(frame->fun[1] + 1U);
    dimmer_light_frame.control_item = HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_COLOR_TEMP;
    dimmer_light_frame.color_temperature = (uint8_t)(((uint32_t)frame->data[0] * 100U) / 255U);
    dimmer_light_frame.valid = 1U;
    should_update_dimmer_light = 1U;
    break;
    }
    default: break;
    }

    if (should_update_dimmer_light != 0U)
    {
        (void)HOOCH_PROTOCOL_DimmerLight_DispatchFrame(&dimmer_light_frame);
    }
    
    KNX_SUMMARY_EMIT_INDEXED("Dimming", frame->fun[1], item_desc, frame);
}

/*调光配置处理*/
void knx_summary_dimming_config(const KNX_Frame_t *frame)
{
    const char *item_desc;
    uint8_t need_set = 0U;
    if ((frame == NULL) || (frame->fun_count < 5U))
    {
        return;
    }

    item_desc = KNX_DESC("Unknown Config Item");
    (void)memset(&dimmer_light_frame, 0, sizeof(dimmer_light_frame));
    /* 调光配置项：item -> 功能描述 */
    switch (frame->fun[4])
    {
    case 1U: item_desc = KNX_DESC("Enable Bit (K->P)");
        {
        HOOCH_PROTOCOL_KeyModeFrame_t key_mode_frame;
        (void)memset(&key_mode_frame, 0, sizeof(key_mode_frame));
        key_mode_frame.key = (HOOCH_PROTOCOL_KeyModeKey_t)(frame->fun[1] + 1U);
        if ((frame->data[0] & 0x01U) != 0U)
        {
            /* bit0=1-使能：按调光开关处理 */
            key_mode_frame.type = HOOCH_PROTOCOL_KEY_MODE_TYPE_DIMMER_SWITCH;
            key_mode_frame.valid = 1U;

        }else
        {
            /* bit0=0-不使能：按普通开关处理 */
            key_mode_frame.type = HOOCH_PROTOCOL_KEY_MODE_TYPE_NORMAL_SWITCH;
            key_mode_frame.valid = 1U;
        }
        (void)HOOCH_PROTOCOL_KeyMode_SetFrame(&key_mode_frame);
        /* 使能位原始位图同步下发 */
        dimmer_light_frame.key = (HOOCH_PROTOCOL_DimmerLightKey_t)(frame->fun[1] + 1U);
        dimmer_light_frame.control_item = HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_ENABLE_BIT;
        dimmer_light_frame.value = frame->data[0];
        dimmer_light_frame.valid = 1U;
        need_set = 3U;
        }
        break;
    case 2U: item_desc = KNX_DESC("Device Description (K->P)");
    {
        /* 设备描述字符串：24 byte UTF-8，通过 KeyName 接口下发 */
        HOOCH_PROTOCOL_KeyNameFrame_t key_name_frame;
        uint16_t copy_len;
        (void)memset(&key_name_frame, 0, sizeof(key_name_frame));
        key_name_frame.key = (HOOCH_PROTOCOL_KeyNameKey_t)(frame->fun[1] + 1U);
        key_name_frame.delivery = HOOCH_PROTOCOL_KEY_NAME_DELIVERY_KEY;
        copy_len = (frame->data_len > HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH)
                       ? HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH
                       : frame->data_len;
        if (copy_len > 0U)
        {
            (void)memcpy(key_name_frame.name, frame->data, copy_len);
            key_name_frame.name[copy_len] = '\0';
        }
        key_name_frame.valid = 1U;
        need_set = 2U;
        (void)HOOCH_PROTOCOL_KeyName_SetFrame(&key_name_frame);

        /* 同步通过调光灯接口下发设备描述(24 byte, UTF-8) */
        dimmer_light_frame.key = (HOOCH_PROTOCOL_DimmerLightKey_t)(frame->fun[1] + 1U);
        dimmer_light_frame.control_item = HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_DEVICE_DESCRIPTOR;
        copy_len = (copy_len > sizeof(dimmer_light_frame.device_desc))
                       ? (uint16_t)sizeof(dimmer_light_frame.device_desc)
                       : copy_len;
        if (copy_len > 0U)
        {
            (void)memcpy(dimmer_light_frame.device_desc, frame->data, copy_len);
        }
        dimmer_light_frame.valid = 1U;
        need_set = 3U;
    }
    break;

    
    case 3U: item_desc = KNX_DESC("Default Icon (K->P)"); 
    dimmer_light_frame.key = (HOOCH_PROTOCOL_DimmerLightKey_t)(frame->fun[1] + 1U);
    dimmer_light_frame.control_item = HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_DEFAULT_ICON;
    dimmer_light_frame.value = frame->data[0];
    dimmer_light_frame.valid = 1U;
    need_set = 3U;
    
    break;
    case 4U: item_desc = KNX_DESC("Selected Icon (K->P)"); 
    dimmer_light_frame.key = (HOOCH_PROTOCOL_DimmerLightKey_t)(frame->fun[1] + 1U);
    dimmer_light_frame.control_item = HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_SELECTED_ICON;
    dimmer_light_frame.value = frame->data[0];
    dimmer_light_frame.valid = 1U;
    need_set = 3U;
    break;
    case 5U: item_desc = KNX_DESC("Color Temp Step (K->P)"); 
    dimmer_light_frame.key = (HOOCH_PROTOCOL_DimmerLightKey_t)(frame->fun[1] + 1U);
    dimmer_light_frame.control_item = HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_COLOR_TEMP_STEP;
    dimmer_light_frame.value = frame->data[0];
    dimmer_light_frame.valid = 1U;
    need_set = 3U;
    break;
    case 6U: item_desc = KNX_DESC("Min Color Temp (K->P)");
    dimmer_light_frame.key = (HOOCH_PROTOCOL_DimmerLightKey_t)(frame->fun[1] + 1U);
    dimmer_light_frame.control_item = HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_MIN_COLOR_TEMP;
    dimmer_light_frame.value = (frame->data_len >= 2U) ? (int)knx_read_be_u16(frame->data) : 0;
    dimmer_light_frame.valid = 1U;
    need_set = 3U;
    break;
    case 7U: item_desc = KNX_DESC("Max Color Temp (K->P)");
    dimmer_light_frame.key = (HOOCH_PROTOCOL_DimmerLightKey_t)(frame->fun[1] + 1U);
    dimmer_light_frame.control_item = HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_MAX_COLOR_TEMP;
    dimmer_light_frame.value = (frame->data_len >= 2U) ? (int)knx_read_be_u16(frame->data) : 0;
    dimmer_light_frame.valid = 1U;
    need_set = 3U;
    break;
    case 8U: item_desc = KNX_DESC("Switch Mode (K->P)");
    {
        HOOCH_PROTOCOL_KeyModeFrame_t key_mode_frame;
        (void)memset(&key_mode_frame, 0, sizeof(key_mode_frame));
        key_mode_frame.key = (HOOCH_PROTOCOL_KeyModeKey_t)(frame->fun[1] + 1U);
        if ((frame->data_len >= 1U) && (frame->data[0] == 2U))
        {
            /* 2=Toggle(ON/OFF)：普通开关反转模式 */
            key_mode_frame.type = HOOCH_PROTOCOL_KEY_MODE_TYPE_NORMAL_SWITCH_TOGGLE;
        }
        else
        {
            /* 0=OFF / 1=ON：普通开关 */
            key_mode_frame.type = HOOCH_PROTOCOL_KEY_MODE_TYPE_NORMAL_SWITCH;
        }
        key_mode_frame.valid = 1U;
        (void)HOOCH_PROTOCOL_KeyMode_SetFrame(&key_mode_frame);
    }
    break;
    default: break;
    }
    if(need_set == 3U)
    {
        (void)HOOCH_PROTOCOL_DimmerLight_DispatchFrame(&dimmer_light_frame);
    }




    
    KNX_SUMMARY_EMIT_INDEXED("Dimming", frame->fun[1], item_desc, frame);
}
