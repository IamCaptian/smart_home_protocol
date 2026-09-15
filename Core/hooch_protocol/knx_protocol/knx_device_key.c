/*
 * KNX Key/Switch Device — Control & Config Summary
 *
 * Panel Communication Protocol V2.4, fun1=2(DEVICE), fun3=5(KEY)
 */
#include "knx_internal.h"
#include "hooch_key_status.h"
#include "hooch_key_name.h"
static HOOCH_PROTOCOL_KeyNameFrame_t key_name_frame;
static HOOCH_PROTOCOL_KeyStatusFrame_t key_status_frame;
/*按键控制处理*/
void knx_summary_key_control(const KNX_Frame_t *frame)
{
    const char *item_desc;
    HOOCH_PROTOCOL_KeyStatusFrame_t key_status_frame;
    (void)memset(&key_status_frame, 0, sizeof(key_status_frame));
    if ((frame == NULL) || (frame->fun_count < 5U))
    {
        return;
    }

    item_desc = KNX_DESC("Unknown Control Item");

    /* 按键控制项：item -> 功能描述 */
    switch (frame->fun[4])
    {
    case 1U: item_desc = KNX_DESC("Switch (P->K)"); break;
    case 2U: item_desc = KNX_DESC("Switch State (K->P)");
    key_status_frame.key = (HOOCH_PROTOCOL_KeyStatusKey_t)(frame->fun[1] + 1U);
    key_status_frame.state = (HOOCH_PROTOCOL_KeyStatusState_t)frame->data[0];
    key_status_frame.control_item = HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_STATE;
    break;
    default: break;
    }

    if (key_status_frame.control_item != HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_INVALID)
    {
        (void)HOOCH_PROTOCOL_KeyStatus_DispatchFrame(&key_status_frame);
    }

    KNX_SUMMARY_EMIT_INDEXED("Key", frame->fun[1], item_desc, frame);
}

/*按键配置处理*/
void knx_summary_key_config(const KNX_Frame_t *frame)
{
    const char *item_desc;
    uint8_t need_set = 0U;
    if ((frame == NULL) || (frame->fun_count < 5U))
    {
        return;
    }

    item_desc = KNX_DESC("Unknown Config Item");
    (void)memset(&key_status_frame, 0, sizeof(key_status_frame));

    /* 按键配置项：item -> 功能描述 */
    switch (frame->fun[4])
    {
    case 1U: item_desc = KNX_DESC("Enable Bit (K<->P)"); 
          {
            need_set = 1U;
        HOOCH_PROTOCOL_KeyModeFrame_t key_mode_frame;
        (void)memset(&key_mode_frame, 0, sizeof(key_mode_frame));
        key_mode_frame.key = (HOOCH_PROTOCOL_KeyModeKey_t)(frame->fun[1] + 1U);
        if(frame->data[0] != 0)
        {
            key_mode_frame.type = HOOCH_PROTOCOL_KEY_MODE_TYPE_NORMAL_SWITCH;
            key_mode_frame.valid = 1U;

        }else
        {
            key_mode_frame.type = HOOCH_PROTOCOL_KEY_MODE_TYPE_NORMAL_SWITCH;
            key_mode_frame.valid = 1U;
        }
                HOOCH_PROTOCOL_KeyMode_SetFrame(&key_mode_frame);
        /* 使能位原始位图同步下发 */
        key_status_frame.key = (HOOCH_PROTOCOL_KeyStatusKey_t)(frame->fun[1] + 1U);
        key_status_frame.control_item = HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_ENABLE_BIT;
        key_status_frame.value = frame->data[0];
        key_status_frame.valid = 1U;
        need_set = 3U;
        } 
        
        
    break;
    case 2U: item_desc = KNX_DESC("Device Description (K<->P)");
    {
        /* 设备描述字符串：24 byte UTF-8，通过 KeyName 接口下发 */
        
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
        key_name_frame.content_type = HOOCH_PROTOCOL_KEY_NAME_CONTENT_TYPE_NAME;
        key_name_frame.valid = 1U;
        (void)HOOCH_PROTOCOL_KeyName_SetFrame(&key_name_frame);

        /* 同步通过按键状态接口下发设备描述(24 byte, UTF-8) */
        key_status_frame.key = (HOOCH_PROTOCOL_KeyStatusKey_t)(frame->fun[1] + 1U);
        key_status_frame.control_item = HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_DEVICE_DESCRIPTOR;
        copy_len = (copy_len > sizeof(key_status_frame.device_desc))
                       ? (uint16_t)sizeof(key_status_frame.device_desc)
                       : copy_len;
        if (copy_len > 0U)
        {
            (void)memcpy(key_status_frame.device_desc, frame->data, copy_len);
        }
        key_status_frame.valid = 1U;
        need_set = 3U;
    }
    break;
    case 3U: item_desc = KNX_DESC("Default Icon (K<->P)"); 
    key_name_frame.key = (HOOCH_PROTOCOL_KeyNameKey_t)(frame->fun[1] + 1U);
    key_name_frame.content_type = HOOCH_PROTOCOL_KEY_NAME_CONTENT_TYPE_DEFAULT_ICON;
    key_name_frame.value = frame->data[0];
    key_name_frame.valid = 1U;
    need_set = 2U;
    break;
    case 4U: item_desc = KNX_DESC("Selected Icon (K<->P)"); 
        key_name_frame.key = (HOOCH_PROTOCOL_KeyNameKey_t)(frame->fun[1] + 1U);
    key_name_frame.content_type = HOOCH_PROTOCOL_KEY_NAME_CONTENT_TYPE_SELECTED_ICON;
    key_name_frame.value = frame->data[0];
    key_name_frame.valid = 1U;
    need_set = 2U;  
    break;
    case 5U: item_desc = KNX_DESC("Switch Mode (K->P)");
    {
        HOOCH_PROTOCOL_KeyModeFrame_t key_mode_frame;
        (void)memset(&key_mode_frame, 0, sizeof(key_mode_frame));
        key_mode_frame.key = (HOOCH_PROTOCOL_KeyModeKey_t)(frame->fun[1] + 1U);
        /*兜底*/
        if (frame->data_len < 1U)
        {
            /* 无数据：按普通开关处理 */
            key_mode_frame.type = HOOCH_PROTOCOL_KEY_MODE_TYPE_NORMAL_SWITCH;
        }
        else
        {
            switch (frame->data[0])
            {
            case 0U:
                /* 0=OFF：常闭模式（怎么点都是关） */
                key_mode_frame.type = HOOCH_PROTOCOL_KEY_MODE_TYPE_ALWAYS_OFF_SWITCH;
                break;
            case 1U:
                /* 1=ON：常开模式（怎么点都是开） */
                key_mode_frame.type = HOOCH_PROTOCOL_KEY_MODE_TYPE_ALWAYS_ON_SWITCH;
                break;
            case 2U:
                /* 2=Toggle(ON/OFF)：普通开关反转模式 */
                key_mode_frame.type = HOOCH_PROTOCOL_KEY_MODE_TYPE_NORMAL_SWITCH_TOGGLE;
                break;
            default:
                /* 无效值：按普通开关处理 */
                key_mode_frame.type = HOOCH_PROTOCOL_KEY_MODE_TYPE_NORMAL_SWITCH;
                break;
            }
        }
        key_mode_frame.valid = 1U;
        (void)HOOCH_PROTOCOL_KeyMode_SetFrame(&key_mode_frame);

        /* 按键模式原始值同步下发(0-OFF/1-ON/2-Toggle) */
        if (frame->data_len >= 1U)
        {
            key_status_frame.key = (HOOCH_PROTOCOL_KeyStatusKey_t)(frame->fun[1] + 1U);
            key_status_frame.control_item = HOOCH_PROTOCOL_KEY_STATUS_CONTROL_ITEM_KEY_MODE;
            key_status_frame.value = frame->data[0];
            key_status_frame.valid = 1U;
            need_set = 3U;
        }
    }
    break;
    default: break;
    }
  if(need_set == 2U)
  {
(void)HOOCH_PROTOCOL_KeyName_SetFrame(&key_name_frame);
  }
    if (need_set == 3U)
    {
        (void)HOOCH_PROTOCOL_KeyStatus_DispatchFrame(&key_status_frame);
    }
    KNX_SUMMARY_EMIT_INDEXED("Key", frame->fun[1], item_desc, frame);
}
