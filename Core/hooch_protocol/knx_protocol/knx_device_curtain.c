/*
 * KNX Curtain Device — Control & Config Summary
 *
 * Panel Communication Protocol V2.4, fun1=2(DEVICE), fun3=7(CURTAIN)
 */
#include "knx_internal.h"
#include "hooch_curtain.h"
#include "hooch_key_mode.h"
#include "hooch_key_name.h"
static HOOCH_PROTOCOL_CurtainFrame_t curtain_frame;
/*窗帘控制处理*/
void knx_summary_curtain_control(const KNX_Frame_t *frame)
{
    const char *item_desc;

    if ((frame == NULL) || (frame->fun_count < 5U))
    {
        return;
    }

    (void)memset(&curtain_frame, 0, sizeof(curtain_frame));
    curtain_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;
    item_desc = KNX_DESC("Unknown Control Item");

    /* 窗帘控制项：item -> 功能描述 */
    switch (frame->fun[4])
    {
    case 1U: item_desc = KNX_DESC("Switch/UpDown (P->K)");
    curtain_frame.key = (HOOCH_PROTOCOL_CurtainKey_t)(frame->fun[1] + 1U);
    curtain_frame.control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_SWITCH;
    curtain_frame.switch_status.value = frame->data[0];
    curtain_frame.valid = 1U;
    break;
    case 2U: item_desc = KNX_DESC("Stop (P->K)");
    curtain_frame.key = (HOOCH_PROTOCOL_CurtainKey_t)(frame->fun[1] + 1U);
    curtain_frame.control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_STOP;
    curtain_frame.stop.value = frame->data[0];
    curtain_frame.valid = 1U;
    break;
    case 3U: item_desc = KNX_DESC("Position Percent (P->K)");
    curtain_frame.key = (HOOCH_PROTOCOL_CurtainKey_t)(frame->fun[1] + 1U);
    curtain_frame.control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_PERCENT;
    curtain_frame.percent.value = (uint8_t)(((uint32_t)frame->data[0] * 100U) / 255U);
    curtain_frame.valid = 1U;
    break;
    case 4U: item_desc = KNX_DESC("Position Percent State (K->P)");
    curtain_frame.key = (HOOCH_PROTOCOL_CurtainKey_t)(frame->fun[1] + 1U);
    curtain_frame.control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_PERCENT;
    curtain_frame.percent.value = (uint8_t)(((uint32_t)frame->data[0] * 100U) / 255U);
    curtain_frame.valid = 1U;
    break;
    case 5U: item_desc = KNX_DESC("Angle Open/Close (P->K)");
    curtain_frame.key = (HOOCH_PROTOCOL_CurtainKey_t)(frame->fun[1] + 1U);
    curtain_frame.control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_SWITCH;
    curtain_frame.switch_status.value = frame->data[0];
    curtain_frame.valid = 1U;
    break;
    case 6U: item_desc = KNX_DESC("Angle Stop (P->K)");
    curtain_frame.key = (HOOCH_PROTOCOL_CurtainKey_t)(frame->fun[1] + 1U);
    curtain_frame.control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_STOP;
    curtain_frame.stop.value = frame->data[0];
    curtain_frame.valid = 1U;
    break;
    case 7U: item_desc = KNX_DESC("Angle Percent (P->K)");
    curtain_frame.key = (HOOCH_PROTOCOL_CurtainKey_t)(frame->fun[1] + 1U);
    curtain_frame.control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ANGLE;
    curtain_frame.angle.value = (uint8_t)(((uint32_t)frame->data[0] * 100U) / 255U);
    curtain_frame.valid = 1U;
    break;
    case 8U: item_desc = KNX_DESC("Angle Percent State (K->P)");
    break;
    default: break;
    }
    
    (void)HOOCH_PROTOCOL_Curtain_DispatchFrame(&curtain_frame);
    
    KNX_SUMMARY_EMIT_INDEXED("Curtain", frame->fun[1], item_desc, frame);
}

/*窗帘配置处理*/
void knx_summary_curtain_config(const KNX_Frame_t *frame)
{
    const char *item_desc;
    uint8_t need_set = 0U;
    if ((frame == NULL) || (frame->fun_count < 5U))
    {
        return;
    }

    item_desc = KNX_DESC("Unknown Config Item");
    (void)memset(&curtain_frame, 0, sizeof(curtain_frame));
    curtain_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;

    /* 窗帘配置项：item -> 功能描述 */
    switch (frame->fun[4])
    {
    case 1U: item_desc = KNX_DESC("Enable Bit (K->P)");
    {
        HOOCH_PROTOCOL_KeyModeFrame_t key_mode_frame;
        (void)memset(&key_mode_frame, 0, sizeof(key_mode_frame));
        key_mode_frame.key = (HOOCH_PROTOCOL_KeyModeKey_t)(frame->fun[1] + 1U);
        if(frame->data[0] != 0)
        {
            key_mode_frame.type = HOOCH_PROTOCOL_KEY_MODE_TYPE_CURTAIN_SWITCH;
            key_mode_frame.valid = 1U;
        }else
        {
            key_mode_frame.type = HOOCH_PROTOCOL_KEY_MODE_TYPE_NORMAL_SWITCH;
            key_mode_frame.valid = 1U;
        }
        (void)HOOCH_PROTOCOL_KeyMode_SetFrame(&key_mode_frame);

        /* 同步通过窗帘接口下发使能位原始位图(bit0:使能 bit1:开关 bit2:停止 bit3:百分比 bit4:角度) */
        curtain_frame.key = (HOOCH_PROTOCOL_CurtainKey_t)(frame->fun[1] + 1U);
        curtain_frame.control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ENABLE_BIT;
        curtain_frame.value = frame->data[0];
        curtain_frame.valid = 1U;
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
    (void)HOOCH_PROTOCOL_KeyName_SetFrame(&key_name_frame);
        need_set = 2U;

        /* 同步通过窗帘接口下发设备描述(24 byte, UTF-8) */
        curtain_frame.key = (HOOCH_PROTOCOL_CurtainKey_t)(frame->fun[1] + 1U);
        curtain_frame.control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_DEVICE_DESCRIPTOR;
        copy_len = (copy_len > sizeof(curtain_frame.device_desc))
                       ? (uint16_t)sizeof(curtain_frame.device_desc)
                       : copy_len;
        if (copy_len > 0U)
        {
            (void)memcpy(curtain_frame.device_desc, frame->data, copy_len);
        }
        curtain_frame.valid = 1U;
        need_set = 3U;
    }
    break;

    
    case 3U: item_desc = KNX_DESC("Default Icon (K->P)");
    curtain_frame.key = (HOOCH_PROTOCOL_CurtainKey_t)(frame->fun[1] + 1U);
    curtain_frame.control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_DEFAULT_ICON;
    curtain_frame.value = frame->data[0];
    curtain_frame.valid = 1U;
    need_set = 3U;
    break;
    case 4U: item_desc = KNX_DESC("Selected Icon (K->P)"); 
    curtain_frame.key = (HOOCH_PROTOCOL_CurtainKey_t)(frame->fun[1] + 1U);
    curtain_frame.control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_SELECTED_ICON;
    curtain_frame.value = frame->data[0];
    curtain_frame.valid = 1U;
    need_set = 3U;
    break;
    case 5U: item_desc = KNX_DESC("Curtain Type (K->P)");
    curtain_frame.key = (HOOCH_PROTOCOL_CurtainKey_t)(frame->fun[1] + 1U);
    curtain_frame.control_item = HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_CURTAIN_TYPE;
    curtain_frame.value = frame->data[0];
    curtain_frame.valid = 1U;
    need_set = 3U;
    break;
    default: break;
    }
    
 if(need_set == 3)
    {
        (void)HOOCH_PROTOCOL_Curtain_DispatchFrame(&curtain_frame);
    }


    KNX_SUMMARY_EMIT_INDEXED("Curtain", frame->fun[1], item_desc, frame);
}
