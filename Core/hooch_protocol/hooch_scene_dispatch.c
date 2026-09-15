#include "hooch_scene_dispatch.h"
#include <string.h>

static HOOCH_PROTOCOL_SceneDispatchFrame_t s_hooch_protocol_scene_dispatch_frame;
static HOOCH_PROTOCOL_SceneDispatchCallback_t s_hooch_protocol_scene_dispatch_callback;

static void HOOCH_PROTOCOL_SceneDispatch_NotifyCallback(void)
{
    if (s_hooch_protocol_scene_dispatch_callback != 0)
    {
        s_hooch_protocol_scene_dispatch_callback(&s_hooch_protocol_scene_dispatch_frame);
    }
}

void HOOCH_SceneDispatch_Init(void)
{
    s_hooch_protocol_scene_dispatch_callback = 0;
    HOOCH_PROTOCOL_SceneDispatch_Clear();
}

void HOOCH_PROTOCOL_SceneDispatch_Clear(void)
{
    (void)memset(&s_hooch_protocol_scene_dispatch_frame, 0,
                 sizeof(s_hooch_protocol_scene_dispatch_frame));
    s_hooch_protocol_scene_dispatch_frame.scene = HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_INVALID;
    s_hooch_protocol_scene_dispatch_frame.control_item =
        HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_INVALID;
}

uint8_t HOOCH_PROTOCOL_SceneDispatch_IsValidScene(HOOCH_PROTOCOL_SceneDispatchScene_t scene)
{
    return (uint8_t)((scene >= HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_1) &&
                     (scene <= HOOCH_PROTOCOL_SCENE_DISPATCH_SCENE_16));
}

uint8_t HOOCH_PROTOCOL_SceneDispatch_IsValidControlItem(
    HOOCH_PROTOCOL_SceneDispatchControlItem_t control_item)
{
    return (uint8_t)((control_item >= HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_ALL) &&
                     (control_item < HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_MAX));
}

HOOCH_PROTOCOL_SceneDispatchResult_t HOOCH_PROTOCOL_SceneDispatch_Set(
    HOOCH_PROTOCOL_SceneDispatchScene_t scene)
{
    if (HOOCH_PROTOCOL_SceneDispatch_IsValidScene(scene) == 0U)
    {
        return HOOCH_PROTOCOL_SCENE_DISPATCH_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_scene_dispatch_frame.scene = scene;
    s_hooch_protocol_scene_dispatch_frame.control_item =
        HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_ALL;
    s_hooch_protocol_scene_dispatch_frame.sequence++;
    s_hooch_protocol_scene_dispatch_frame.valid = 1U;
    HOOCH_PROTOCOL_SceneDispatch_NotifyCallback();
    return HOOCH_PROTOCOL_SCENE_DISPATCH_RESULT_OK;
}

/* 统一按 control_item 写入场景下发帧。 */
static HOOCH_PROTOCOL_SceneDispatchResult_t HOOCH_PROTOCOL_SceneDispatch_SetFrameInternal(
    const HOOCH_PROTOCOL_SceneDispatchFrame_t *frame)
{
    if (frame == 0)
    {
        return HOOCH_PROTOCOL_SCENE_DISPATCH_RESULT_INVALID_PARAM;
    }

    if (HOOCH_PROTOCOL_SceneDispatch_IsValidControlItem(frame->control_item) == 0U)
    {
        return HOOCH_PROTOCOL_SCENE_DISPATCH_RESULT_INVALID_PARAM;
    }

    switch (frame->control_item)
    {
        case HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_ALL:
            if (HOOCH_PROTOCOL_SceneDispatch_IsValidScene(frame->scene) == 0U)
            {
                return HOOCH_PROTOCOL_SCENE_DISPATCH_RESULT_INVALID_PARAM;
            }
            s_hooch_protocol_scene_dispatch_frame.scene = frame->scene;
            break;

        case HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_ENABLE_BIT:
            /* 使能位原始位图(bit0:使能 bit1:场景) */
            s_hooch_protocol_scene_dispatch_frame.scene = frame->scene;
            s_hooch_protocol_scene_dispatch_frame.value = frame->value;
            break;

        case HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_DEVICE_DESC:
            (void)memcpy(s_hooch_protocol_scene_dispatch_frame.device_desc,
                         frame->device_desc,
                         sizeof(s_hooch_protocol_scene_dispatch_frame.device_desc));
            break;

        case HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_DEFAULT_ICON:
            s_hooch_protocol_scene_dispatch_frame.value = frame->value;
            break;

        case HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_SELECTED_ICON:
            s_hooch_protocol_scene_dispatch_frame.value = frame->value;
            break;

        case HOOCH_PROTOCOL_SCENE_DISPATCH_CONTROL_ITEM_LEARN:
            /* 学习模式：value=1 表示触发学习 */
            s_hooch_protocol_scene_dispatch_frame.scene = frame->scene;
            s_hooch_protocol_scene_dispatch_frame.value = frame->value;
            break;

        default:
            return HOOCH_PROTOCOL_SCENE_DISPATCH_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_scene_dispatch_frame.control_item = frame->control_item;
    s_hooch_protocol_scene_dispatch_frame.sequence++;
    s_hooch_protocol_scene_dispatch_frame.valid = 1U;
    HOOCH_PROTOCOL_SceneDispatch_NotifyCallback();
    return HOOCH_PROTOCOL_SCENE_DISPATCH_RESULT_OK;
}

HOOCH_PROTOCOL_SceneDispatchResult_t HOOCH_PROTOCOL_SceneDispatch_SetFrame(
    const HOOCH_PROTOCOL_SceneDispatchFrame_t *frame)
{
    return HOOCH_PROTOCOL_SceneDispatch_SetFrameInternal(frame);
}

const HOOCH_PROTOCOL_SceneDispatchFrame_t *HOOCH_PROTOCOL_SceneDispatch_GetFrame(void)
{
    return &s_hooch_protocol_scene_dispatch_frame;
}

void HOOCH_PROTOCOL_SceneDispatch_RegisterCallback(
    HOOCH_PROTOCOL_SceneDispatchCallback_t callback)
{
    s_hooch_protocol_scene_dispatch_callback = callback;
}

void HOOCH_PROTOCOL_SceneDispatch_UnregisterCallback(void)
{
    s_hooch_protocol_scene_dispatch_callback = 0;
}

HOOCH_PROTOCOL_SceneDispatchResult_t HOOCH_PROTOCOL_SceneDispatch_Send(
    HOOCH_PROTOCOL_SceneDispatchScene_t scene)
{
    return HOOCH_PROTOCOL_SceneDispatch_Set(scene);
}
