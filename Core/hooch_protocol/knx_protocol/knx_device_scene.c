/*
 * KNX Scene Device — Control & Config Summary
 *
 * Panel Communication Protocol V2.4, fun1=2(DEVICE), fun3=8(SCENE)
 */
#include "knx_internal.h"
#include "hooch_scene_dispatch.h"
#include "hooch_key_mode.h"
#include "hooch_key_name.h"
static HOOCH_PROTOCOL_KeyNameFrame_t key_name_frame;
static HOOCH_PROTOCOL_SceneDispatchFrame_t scene_dispatch_frame;
/*情景控制处理*/
void knx_summary_scene_control(const KNX_Frame_t *frame)
{
    const char *item_desc;

    if ((frame == NULL) || (frame->fun_count < 5U))
    {
        return;
    }

    item_desc = KNX_DESC("Unknown Control Item");

    /* 情景控制项：item -> 功能描述 */
    switch (frame->fun[4])
    {
    case KNX_SCENE_ITEM_TRIGGER:
        item_desc = KNX_DESC("Trigger (P->K)"); /* 触发：1=触发 */

        break;

    case KNX_SCENE_ITEM_STATUS:
        item_desc = KNX_DESC("Status (K->P)"); /* 状态：0=非触发, 1=触发 */
        if ((frame->data_len >= 1U) && (frame->data[0] == 1U))
        {
            /* 场景触发：全量控制，来源标注为 KNX */
            (void)memset(&scene_dispatch_frame, 0, sizeof(scene_dispatch_frame));
            /* 场景编号 = 设备编号 + 1 */
            scene_dispatch_frame.scene = (HOOCH_PROTOCOL_SceneDispatchScene_t)(frame->fun[1] + 1U);
            scene_dispatch_frame.control_item = HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_ALL;
            scene_dispatch_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;
            scene_dispatch_frame.valid = 1U;
            (void)HOOCH_PROTOCOL_SceneDispatch_SetFrame(&scene_dispatch_frame);
        }
        break;

    case KNX_SCENE_ITEM_LEARN:
        item_desc = KNX_DESC("Learn (P->K)"); /* 学习：1=学习 */
        // if ((frame->data_len >= 1U) && (frame->data[0] == 1U))
        // {
        //     scene_dispatch_frame.scene = (HOOCH_PROTOCOL_SceneDispatchScene_t)(frame->fun[1] + 1U);
        //     scene_dispatch_frame.control_item = HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_LEARN;
        //     scene_dispatch_frame.value = frame->data[0];
        //     scene_dispatch_frame.valid = 1U;
        //     (void)HOOCH_PROTOCOL_SceneDispatch_SetFrame(&scene_dispatch_frame);
        // }
        break;

    default:
        break;
    }

    KNX_SUMMARY_EMIT_INDEXED("Scene", frame->fun[1], item_desc, frame);
}

/*情景配置处理*/
void knx_summary_scene_config(const KNX_Frame_t *frame)
{
    const char *item_desc;
    uint8_t need_set = 0U;
    if ((frame == NULL) || (frame->fun_count < 5U))
    {
        return;
    }
    (void)memset(&key_name_frame, 0, sizeof(key_name_frame));
    (void)memset(&scene_dispatch_frame, 0, sizeof(scene_dispatch_frame));
    scene_dispatch_frame.source = HOOCH_PROTOCOL_SOURCE_KNX;
    item_desc = KNX_DESC("Unknown Config Item");

    /* 情景配置项：item -> 功能描述 */
    switch (frame->fun[4])
    {
    case KNX_SCENE_CONFIG_ENABLE_BITMAP:
        item_desc = KNX_DESC("Enable Bit (K->P)"); /* 使能位 */
        {
            HOOCH_PROTOCOL_KeyModeFrame_t key_mode_frame;
            (void)memset(&key_mode_frame, 0, sizeof(key_mode_frame));
            key_mode_frame.key = (HOOCH_PROTOCOL_KeyModeKey_t)(frame->fun[1] + 1U);
            if (frame->data_len >= 1U)
            {
                /* 使能位控制：1=场景模式使能，0=普通开关 */
                if (frame->data[0] != 0)
                {
                    key_mode_frame.type = HOOCH_PROTOCOL_KEY_MODE_TYPE_SCENARIO_SWITCH;
                    key_mode_frame.valid = 1U;
                }
                else
                {
                    key_mode_frame.type = HOOCH_PROTOCOL_KEY_MODE_TYPE_NORMAL_SWITCH;
                    key_mode_frame.valid = 1U;
                }

                /* 使能位原始位图同步下发(bit0:使能 bit1:场景) */
                scene_dispatch_frame.scene = (HOOCH_PROTOCOL_SceneDispatchScene_t)(frame->fun[1] + 1U);
                scene_dispatch_frame.control_item = HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_ENABLE_BIT;
                scene_dispatch_frame.value = frame->data[0];
                need_set = 3U;
            }
            (void)HOOCH_PROTOCOL_KeyMode_SetFrame(&key_mode_frame);
        }
        break;

    case KNX_SCENE_CONFIG_DESCRIPTION:
        item_desc = KNX_DESC("Device Description (K->P)"); /* 设备描述字符串 */
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

            /* 同步通过场景下发接口下发设备描述(24 byte, UTF-8) */
            scene_dispatch_frame.scene = (HOOCH_PROTOCOL_SceneDispatchScene_t)(frame->fun[1] + 1U);
            scene_dispatch_frame.control_item = HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_DEVICE_DESC;
            copy_len = (copy_len > sizeof(scene_dispatch_frame.device_desc))
                           ? (uint16_t)sizeof(scene_dispatch_frame.device_desc)
                           : copy_len;
            if (copy_len > 0U)
            {
                (void)memcpy(scene_dispatch_frame.device_desc, frame->data, copy_len);
            }
            scene_dispatch_frame.valid = 1U;
            need_set = 3U;
        }
        break;

    case KNX_SCENE_CONFIG_DEFAULT_ICON:
        item_desc = KNX_DESC("Default Icon (K->P)"); /* 默认图标 */
    // key_name_frame.key = (HOOCH_PROTOCOL_KeyNameKey_t)(frame->fun[1] + 1U);
    // key_name_frame.content_type = HOOCH_PROTOCOL_KEY_NAME_CONTENT_TYPE_DEFAULT_ICON;
    // key_name_frame.value = frame->data[0];
    // key_name_frame.valid = 1U;
    scene_dispatch_frame.scene = (HOOCH_PROTOCOL_SceneDispatchScene_t)(frame->fun[1] + 1U);
    scene_dispatch_frame.control_item = HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_DEFAULT_ICON;
    scene_dispatch_frame.value = frame->data[0];
    need_set = 3U;
        break;

    case KNX_SCENE_CONFIG_SELECTED_ICON:
        item_desc = KNX_DESC("Selected Icon (K->P)"); /* 选中图标 */
    // key_name_frame.key = (HOOCH_PROTOCOL_KeyNameKey_t)(frame->fun[1] + 1U);
    // key_name_frame.content_type = HOOCH_PROTOCOL_KEY_NAME_CONTENT_TYPE_SELECTED_ICON;
    // key_name_frame.value = frame->data[0];
    // key_name_frame.valid = 1U;
    scene_dispatch_frame.scene = (HOOCH_PROTOCOL_SceneDispatchScene_t)(frame->fun[1] + 1U);
    scene_dispatch_frame.control_item = HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_SELECTED_ICON;
    scene_dispatch_frame.value = frame->data[0];
    need_set = 3U;

        break;

    default:
        break;
    }
  if(need_set == 2U)
  {
(void)HOOCH_PROTOCOL_KeyName_SetFrame(&key_name_frame);
  }else if(need_set == 3U)
  {
    (void)HOOCH_PROTOCOL_SceneDispatch_SetFrame(&scene_dispatch_frame);
  }
    KNX_SUMMARY_EMIT_INDEXED("Scene", frame->fun[1], item_desc, frame);
}
