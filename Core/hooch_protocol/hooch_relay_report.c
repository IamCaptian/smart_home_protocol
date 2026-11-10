#include "hooch_relay.h"

/* 继电器上报功能暂未实现，此处仅保留接口占位，后续实现时补充上报逻辑。 */

void HOOCH_Relay_Init(void)
{
    HOOCH_PROTOCOL_Relay_UnregisterCallback();
    HOOCH_PROTOCOL_Relay_UnregisterReportCallback();
    HOOCH_PROTOCOL_Relay_Clear();
}

void HOOCH_PROTOCOL_Relay_RegisterReportCallback(
    HOOCH_PROTOCOL_RelayCallback_t callback)
{
    (void)callback;
}

void HOOCH_PROTOCOL_Relay_UnregisterReportCallback(void)
{
}

HOOCH_PROTOCOL_RelayResult_t HOOCH_PROTOCOL_Relay_SendSwitch(
    HOOCH_PROTOCOL_RelayChannel_t channel,
    HOOCH_PROTOCOL_RelaySwitchState_t switch_state)
{
    (void)channel;
    (void)switch_state;

    return HOOCH_PROTOCOL_RELAY_RESULT_ERROR;
}

HOOCH_PROTOCOL_RelayResult_t HOOCH_PROTOCOL_Relay_SendFrame(
    const HOOCH_PROTOCOL_RelayFrame_t *frame)
{
    (void)frame;

    return HOOCH_PROTOCOL_RELAY_RESULT_ERROR;
}
