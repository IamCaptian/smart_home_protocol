#include "hooch_key_mode.h"

static HOOCH_PROTOCOL_KeyModeFrame_t s_hooch_protocol_key_mode_frame;
static HOOCH_PROTOCOL_KeyModeCallback_t s_hooch_protocol_key_mode_callback;
static HOOCH_PROTOCOL_KeyModeType_t s_hooch_protocol_key_mode_type_table[HOOCH_PROTOCOL_KEY_MODE_KEY_COUNT];
static HOOCH_PROTOCOL_KeyModePowerOnStatus_t s_hooch_protocol_key_mode_power_on_status_table[HOOCH_PROTOCOL_KEY_MODE_KEY_COUNT];

static uint8_t HOOCH_PROTOCOL_KeyMode_KeyToIndex(HOOCH_PROTOCOL_KeyModeKey_t key)
{
    return (uint8_t)((uint8_t)key - 1U);
}

static void HOOCH_PROTOCOL_KeyMode_NotifyCallback(void)
{
    if (s_hooch_protocol_key_mode_callback != 0)
    {
        s_hooch_protocol_key_mode_callback(&s_hooch_protocol_key_mode_frame);
    }
}

void HOOCH_KeyMode_Init(void)
{
    s_hooch_protocol_key_mode_callback = 0;
    HOOCH_PROTOCOL_KeyMode_Clear();
}

void HOOCH_PROTOCOL_KeyMode_Clear(void)
{
    uint8_t i;

    s_hooch_protocol_key_mode_frame.key = HOOCH_PROTOCOL_KEY_MODE_KEY_INVALID;
    s_hooch_protocol_key_mode_frame.power_on_status = HOOCH_PROTOCOL_KEY_MODE_POWER_ON_INVALID;
    s_hooch_protocol_key_mode_frame.type = HOOCH_PROTOCOL_KEY_MODE_TYPE_INVALID;
    s_hooch_protocol_key_mode_frame.sequence = 0U;
    s_hooch_protocol_key_mode_frame.valid = 0U;

    for (i = 0U; i < HOOCH_PROTOCOL_KEY_MODE_KEY_COUNT; i++)
    {
        s_hooch_protocol_key_mode_type_table[i] = HOOCH_PROTOCOL_KEY_MODE_TYPE_INVALID;
        s_hooch_protocol_key_mode_power_on_status_table[i] = HOOCH_PROTOCOL_KEY_MODE_POWER_ON_INVALID;
    }
}

uint8_t HOOCH_PROTOCOL_KeyMode_IsValidKey(HOOCH_PROTOCOL_KeyModeKey_t key)
{
    return (uint8_t)((key >= HOOCH_PROTOCOL_KEY_MODE_KEY_1) &&
                     (key <= HOOCH_PROTOCOL_KEY_MODE_KEY_16));
}

uint8_t HOOCH_PROTOCOL_KeyMode_IsValidType(HOOCH_PROTOCOL_KeyModeType_t type)
{
    return (uint8_t)(type <= HOOCH_PROTOCOL_KEY_MODE_TYPE_INVALID);
}

uint8_t HOOCH_PROTOCOL_KeyMode_IsValidPowerOnStatus(HOOCH_PROTOCOL_KeyModePowerOnStatus_t status)
{
    return (uint8_t)((status == HOOCH_PROTOCOL_KEY_MODE_POWER_ON_MEMORY) ||
                     (status == HOOCH_PROTOCOL_KEY_MODE_POWER_ON_OFF) ||
                     (status == HOOCH_PROTOCOL_KEY_MODE_POWER_ON_ON));
}

const char *HOOCH_PROTOCOL_KeyMode_TypeToString(HOOCH_PROTOCOL_KeyModeType_t type)
{
    switch (type)
    {
    case HOOCH_PROTOCOL_KEY_MODE_TYPE_WIRED_WIRELESS_SWITCH:
        return "WIRED_WIRELESS_SWITCH";
    case HOOCH_PROTOCOL_KEY_MODE_TYPE_WIRELESS_SWITCH:
        return "WIRELESS_SWITCH";
    case HOOCH_PROTOCOL_KEY_MODE_TYPE_DYNAMIC_SWITCH:
        return "DYNAMIC_SWITCH";
    case HOOCH_PROTOCOL_KEY_MODE_TYPE_WIRELESS_REMOTE_SWITCH:
        return "WIRELESS_REMOTE_SWITCH/VIRTUAL_SWITCH";
    case HOOCH_PROTOCOL_KEY_MODE_TYPE_DIMMER_SWITCH:
        return "DIMMER_SWITCH";
    case HOOCH_PROTOCOL_KEY_MODE_TYPE_CURTAIN_SWITCH:
        return "CURTAIN_SWITCH";
    case HOOCH_PROTOCOL_KEY_MODE_TYPE_LOCK_SWITCH:
        return "LOCK_SWITCH";
    case HOOCH_PROTOCOL_KEY_MODE_TYPE_MOMENTARY_SWITCH:
        return "MOMENTARY_SWITCH";
    case HOOCH_PROTOCOL_KEY_MODE_TYPE_NORMAL_SWITCH:
        return "NORMAL_SWITCH";
    case HOOCH_PROTOCOL_KEY_MODE_TYPE_NORMAL_SWITCH_TOGGLE:
        return "NORMAL_SWITCH_TOGGLE";
    case HOOCH_PROTOCOL_KEY_MODE_TYPE_ALWAYS_ON_SWITCH:
        return "ALWAYS_ON_SWITCH";
    case HOOCH_PROTOCOL_KEY_MODE_TYPE_ALWAYS_OFF_SWITCH:
        return "ALWAYS_OFF_SWITCH";
    case HOOCH_PROTOCOL_KEY_MODE_TYPE_INVALID:
        return "INVALID";
    default:
        return "UNKNOWN";
    }
}

HOOCH_PROTOCOL_KeyModeResult_t HOOCH_PROTOCOL_KeyMode_Set(
    HOOCH_PROTOCOL_KeyModeKey_t key,
    HOOCH_PROTOCOL_KeyModePowerOnStatus_t power_on_status,
    HOOCH_PROTOCOL_KeyModeType_t type)
{
    uint8_t index;

    if ((HOOCH_PROTOCOL_KeyMode_IsValidKey(key) == 0U) ||
        (HOOCH_PROTOCOL_KeyMode_IsValidPowerOnStatus(power_on_status) == 0U) ||
        (HOOCH_PROTOCOL_KeyMode_IsValidType(type) == 0U))
    {
        return HOOCH_PROTOCOL_KEY_MODE_RESULT_INVALID_PARAM;
    }

    index = HOOCH_PROTOCOL_KeyMode_KeyToIndex(key);
    s_hooch_protocol_key_mode_type_table[index] = type;
    s_hooch_protocol_key_mode_power_on_status_table[index] = power_on_status;

    s_hooch_protocol_key_mode_frame.key = key;
    s_hooch_protocol_key_mode_frame.power_on_status = power_on_status;
    s_hooch_protocol_key_mode_frame.type = type;
    s_hooch_protocol_key_mode_frame.sequence++;
    s_hooch_protocol_key_mode_frame.valid = 1U;

    HOOCH_PROTOCOL_KeyMode_NotifyCallback();

    return HOOCH_PROTOCOL_KEY_MODE_RESULT_OK;
}

HOOCH_PROTOCOL_KeyModeResult_t HOOCH_PROTOCOL_KeyMode_SetFrame(
    const HOOCH_PROTOCOL_KeyModeFrame_t *frame)
{
    if (frame == 0)
    {
        return HOOCH_PROTOCOL_KEY_MODE_RESULT_INVALID_PARAM;
    }

    if ((HOOCH_PROTOCOL_KeyMode_IsValidKey(frame->key) == 0U) ||
        (HOOCH_PROTOCOL_KeyMode_IsValidPowerOnStatus(frame->power_on_status) == 0U) ||
        (HOOCH_PROTOCOL_KeyMode_IsValidType(frame->type) == 0U))
    {
        return HOOCH_PROTOCOL_KEY_MODE_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_key_mode_frame = *frame;
    s_hooch_protocol_key_mode_frame.valid = 1U;

    s_hooch_protocol_key_mode_type_table[HOOCH_PROTOCOL_KeyMode_KeyToIndex(frame->key)] = frame->type;
    s_hooch_protocol_key_mode_power_on_status_table[HOOCH_PROTOCOL_KeyMode_KeyToIndex(frame->key)] =
        frame->power_on_status;

    HOOCH_PROTOCOL_KeyMode_NotifyCallback();

    return HOOCH_PROTOCOL_KEY_MODE_RESULT_OK;
}

const HOOCH_PROTOCOL_KeyModeFrame_t *HOOCH_PROTOCOL_KeyMode_GetFrame(void)
{
    return &s_hooch_protocol_key_mode_frame;
}

HOOCH_PROTOCOL_KeyModeType_t HOOCH_PROTOCOL_KeyMode_GetType(HOOCH_PROTOCOL_KeyModeKey_t key)
{
    if (HOOCH_PROTOCOL_KeyMode_IsValidKey(key) == 0U)
    {
        return HOOCH_PROTOCOL_KEY_MODE_TYPE_INVALID;
    }

    return s_hooch_protocol_key_mode_type_table[HOOCH_PROTOCOL_KeyMode_KeyToIndex(key)];
}

HOOCH_PROTOCOL_KeyModePowerOnStatus_t HOOCH_PROTOCOL_KeyMode_GetPowerOnStatus(
    HOOCH_PROTOCOL_KeyModeKey_t key)
{
    if (HOOCH_PROTOCOL_KeyMode_IsValidKey(key) == 0U)
    {
        return HOOCH_PROTOCOL_KEY_MODE_POWER_ON_INVALID;
    }

    return s_hooch_protocol_key_mode_power_on_status_table[HOOCH_PROTOCOL_KeyMode_KeyToIndex(key)];
}

void HOOCH_PROTOCOL_KeyMode_RegisterCallback(
    HOOCH_PROTOCOL_KeyModeCallback_t callback)
{
    s_hooch_protocol_key_mode_callback = callback;
}

void HOOCH_PROTOCOL_KeyMode_UnregisterCallback(void)
{
    s_hooch_protocol_key_mode_callback = 0;
}

HOOCH_PROTOCOL_KeyModeResult_t HOOCH_PROTOCOL_KeyMode_Send(
    HOOCH_PROTOCOL_KeyModeKey_t key,
    HOOCH_PROTOCOL_KeyModePowerOnStatus_t power_on_status,
    HOOCH_PROTOCOL_KeyModeType_t type)
{
    return HOOCH_PROTOCOL_KeyMode_Set(key, power_on_status, type);
}
