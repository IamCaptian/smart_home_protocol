#include "hooch_relay.h"

/* 保存当前一次继电器接收数据。 */
static HOOCH_PROTOCOL_RelayFrame_t s_hooch_protocol_relay_dispatch_frame;

/* 保存外部注册的继电器接收回调。 */
static HOOCH_PROTOCOL_RelayCallback_t s_hooch_protocol_relay_dispatch_callback;

/* 统一清空一帧继电器数据。 */
static void HOOCH_PROTOCOL_Relay_ClearFrame(HOOCH_PROTOCOL_RelayFrame_t *frame)
{
    if (frame == 0)
    {
        return;
    }

    frame->channel = HOOCH_PROTOCOL_RELAY_CHANNEL_INVALID;
    frame->switch_state = HOOCH_PROTOCOL_RELAY_SWITCH_OFF;
    frame->control_item = HOOCH_PROTOCOL_RELAY_CONTROL_ITEM_INVALID;
    frame->source = HOOCH_PROTOCOL_SOURCE_INVALID;
    frame->sequence = 0U;
    frame->valid = 0U;
    frame->value = 0U;
    (void)memset(frame->device_desc, 0, sizeof(frame->device_desc));
}

/* 有接收回调注册时，把最新数据通知出去。 */
static void HOOCH_PROTOCOL_Relay_NotifyDispatchCallback(void)
{
    if (s_hooch_protocol_relay_dispatch_callback != 0)
    {
        s_hooch_protocol_relay_dispatch_callback(
            &s_hooch_protocol_relay_dispatch_frame);
    }
}

/* 统一按 control_item 写入继电器接收帧。 */
static HOOCH_PROTOCOL_RelayResult_t HOOCH_PROTOCOL_Relay_SetDispatchFrameInternal(
    const HOOCH_PROTOCOL_RelayFrame_t *frame)
{
    if (frame == 0)
    {
        return HOOCH_PROTOCOL_RELAY_RESULT_INVALID_PARAM;
    }

    if (HOOCH_PROTOCOL_Relay_IsValidChannel(frame->channel) == 0U)
    {
        return HOOCH_PROTOCOL_RELAY_RESULT_INVALID_PARAM;
    }

    switch (frame->control_item)
    {
        case HOOCH_PROTOCOL_RELAY_CONTROL_ITEM_ALL:
        case HOOCH_PROTOCOL_RELAY_CONTROL_ITEM_SWITCH:
            if (HOOCH_PROTOCOL_Relay_IsValidSwitchState(frame->switch_state) == 0U)
            {
                return HOOCH_PROTOCOL_RELAY_RESULT_INVALID_PARAM;
            }

            s_hooch_protocol_relay_dispatch_frame.switch_state = frame->switch_state;
            break;

        case HOOCH_PROTOCOL_RELAY_CONTROL_ITEM_ENABLE_BIT:
            /* 使能位原始位图(bit0:使能 bit1:开关) */
            s_hooch_protocol_relay_dispatch_frame.value = frame->value;
            break;

        case HOOCH_PROTOCOL_RELAY_CONTROL_ITEM_DEVICE_DESC:
            (void)memcpy(s_hooch_protocol_relay_dispatch_frame.device_desc,
                         frame->device_desc,
                         sizeof(s_hooch_protocol_relay_dispatch_frame.device_desc));
            break;

        case HOOCH_PROTOCOL_RELAY_CONTROL_ITEM_DEFAULT_ICON:
        case HOOCH_PROTOCOL_RELAY_CONTROL_ITEM_SELECTED_ICON:
            s_hooch_protocol_relay_dispatch_frame.value = frame->value;
            break;

        default:
            return HOOCH_PROTOCOL_RELAY_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_relay_dispatch_frame.channel = frame->channel;
    s_hooch_protocol_relay_dispatch_frame.control_item = frame->control_item;
    s_hooch_protocol_relay_dispatch_frame.source = frame->source;
    s_hooch_protocol_relay_dispatch_frame.sequence++;
    s_hooch_protocol_relay_dispatch_frame.valid = 1U;

    HOOCH_PROTOCOL_Relay_NotifyDispatchCallback();

    return HOOCH_PROTOCOL_RELAY_RESULT_OK;
}

uint8_t HOOCH_PROTOCOL_Relay_IsValidChannel(
    HOOCH_PROTOCOL_RelayChannel_t channel)
{
    return (uint8_t)((channel >= HOOCH_PROTOCOL_RELAY_CHANNEL_1) &&
                     (channel <= HOOCH_PROTOCOL_RELAY_CHANNEL_8));
}

uint8_t HOOCH_PROTOCOL_Relay_IsValidSwitchState(
    HOOCH_PROTOCOL_RelaySwitchState_t switch_state)
{
    return (uint8_t)((switch_state == HOOCH_PROTOCOL_RELAY_SWITCH_OFF) ||
                     (switch_state == HOOCH_PROTOCOL_RELAY_SWITCH_ON));
}

const HOOCH_PROTOCOL_RelayFrame_t *HOOCH_PROTOCOL_Relay_GetFrame(void)
{
    return &s_hooch_protocol_relay_dispatch_frame;
}

void HOOCH_PROTOCOL_Relay_Clear(void)
{
    HOOCH_PROTOCOL_Relay_ClearFrame(&s_hooch_protocol_relay_dispatch_frame);
}

void HOOCH_PROTOCOL_Relay_RegisterCallback(
    HOOCH_PROTOCOL_RelayCallback_t callback)
{
    s_hooch_protocol_relay_dispatch_callback = callback;
}

void HOOCH_PROTOCOL_Relay_UnregisterCallback(void)
{
    s_hooch_protocol_relay_dispatch_callback = 0;
}

HOOCH_PROTOCOL_RelayResult_t HOOCH_PROTOCOL_Relay_DispatchFrame(
    const HOOCH_PROTOCOL_RelayFrame_t *frame)
{
    return HOOCH_PROTOCOL_Relay_SetDispatchFrameInternal(frame);
}

HOOCH_PROTOCOL_RelayResult_t HOOCH_PROTOCOL_Relay_DispatchSwitch(
    HOOCH_PROTOCOL_RelayChannel_t channel,
    HOOCH_PROTOCOL_RelaySwitchState_t switch_state)
{
    HOOCH_PROTOCOL_RelayFrame_t frame;

    HOOCH_PROTOCOL_Relay_ClearFrame(&frame);
    frame.channel = channel;
    frame.switch_state = switch_state;
    frame.control_item = HOOCH_PROTOCOL_RELAY_CONTROL_ITEM_SWITCH;

    return HOOCH_PROTOCOL_Relay_SetDispatchFrameInternal(&frame);
}
