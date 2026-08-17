#include "hooch_key_name.h"

static HOOCH_PROTOCOL_KeyNameFrame_t s_hooch_protocol_key_name_frame;
static HOOCH_PROTOCOL_KeyNameCallback_t s_hooch_protocol_key_name_callback;

static void HOOCH_PROTOCOL_KeyName_CopyName(char *dst, const char *src)
{
    uint8_t index = 0U;

    while ((index < (HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH - 1U)) && (src[index] != '\0'))
    {
        dst[index] = src[index];
        index++;
    }

    dst[index] = '\0';
}

static void HOOCH_PROTOCOL_KeyName_NotifyCallback(void)
{
    if (s_hooch_protocol_key_name_callback != 0)
    {
        s_hooch_protocol_key_name_callback(&s_hooch_protocol_key_name_frame);
    }
}

void HOOCH_KeyName_Init(void)
{
    s_hooch_protocol_key_name_callback = 0;
    HOOCH_PROTOCOL_KeyName_Clear();
}

void HOOCH_PROTOCOL_KeyName_Clear(void)
{
    s_hooch_protocol_key_name_frame.key = HOOCH_PROTOCOL_KEY_NAME_KEY_INVALID;
    s_hooch_protocol_key_name_frame.icon_index = HOOCH_PROTOCOL_KEY_NAME_ICON_INDEX_INVALID;
    s_hooch_protocol_key_name_frame.page = HOOCH_PROTOCOL_KEY_NAME_PAGE_INVALID;
    s_hooch_protocol_key_name_frame.delivery = HOOCH_PROTOCOL_KEY_NAME_DELIVERY_INVALID;
    s_hooch_protocol_key_name_frame.name[0] = '\0';
    s_hooch_protocol_key_name_frame.sequence = 0U;
    s_hooch_protocol_key_name_frame.valid = 0U;
}

uint8_t HOOCH_PROTOCOL_KeyName_IsValidKey(HOOCH_PROTOCOL_KeyNameKey_t key)
{
    return (uint8_t)((key >= HOOCH_PROTOCOL_KEY_NAME_KEY_1) &&
                     (key <= HOOCH_PROTOCOL_KEY_NAME_KEY_8));
}

HOOCH_PROTOCOL_KeyNameResult_t HOOCH_PROTOCOL_KeyName_SetName(
    HOOCH_PROTOCOL_KeyNameKey_t key,
    const char *name)
{
    if ((HOOCH_PROTOCOL_KeyName_IsValidKey(key) == 0U) || (name == 0))
    {
        return HOOCH_PROTOCOL_KEY_NAME_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_key_name_frame.key = key;
    s_hooch_protocol_key_name_frame.icon_index = HOOCH_PROTOCOL_KEY_NAME_ICON_INDEX_INVALID;
    s_hooch_protocol_key_name_frame.page = HOOCH_PROTOCOL_KEY_NAME_PAGE_INVALID;
    s_hooch_protocol_key_name_frame.delivery = HOOCH_PROTOCOL_KEY_NAME_DELIVERY_KEY;
    HOOCH_PROTOCOL_KeyName_CopyName(s_hooch_protocol_key_name_frame.name, name);
    s_hooch_protocol_key_name_frame.sequence++;
    s_hooch_protocol_key_name_frame.valid = 1U;
    HOOCH_PROTOCOL_KeyName_NotifyCallback();
    return HOOCH_PROTOCOL_KEY_NAME_RESULT_OK;
}

HOOCH_PROTOCOL_KeyNameResult_t HOOCH_PROTOCOL_KeyName_SetFrame(
    const HOOCH_PROTOCOL_KeyNameFrame_t *frame)
{
    if ((frame == 0) || (HOOCH_PROTOCOL_KeyName_IsValidKey(frame->key) == 0U))
    {
        return HOOCH_PROTOCOL_KEY_NAME_RESULT_INVALID_PARAM;
    }

    s_hooch_protocol_key_name_frame.key = frame->key;
    s_hooch_protocol_key_name_frame.icon_index = frame->icon_index;
    s_hooch_protocol_key_name_frame.page = frame->page;
    s_hooch_protocol_key_name_frame.delivery = frame->delivery;
    HOOCH_PROTOCOL_KeyName_CopyName(s_hooch_protocol_key_name_frame.name, frame->name);
    s_hooch_protocol_key_name_frame.sequence = frame->sequence;
    s_hooch_protocol_key_name_frame.valid = 1U;
    HOOCH_PROTOCOL_KeyName_NotifyCallback();
    return HOOCH_PROTOCOL_KEY_NAME_RESULT_OK;
}

const HOOCH_PROTOCOL_KeyNameFrame_t *HOOCH_PROTOCOL_KeyName_GetFrame(void)
{
    return &s_hooch_protocol_key_name_frame;
}

const char *HOOCH_PROTOCOL_KeyName_GetName(void)
{
    return s_hooch_protocol_key_name_frame.name;
}

void HOOCH_PROTOCOL_KeyName_RegisterCallback(
    HOOCH_PROTOCOL_KeyNameCallback_t callback)
{
    s_hooch_protocol_key_name_callback = callback;
}

void HOOCH_PROTOCOL_KeyName_UnregisterCallback(void)
{
    s_hooch_protocol_key_name_callback = 0;
}
