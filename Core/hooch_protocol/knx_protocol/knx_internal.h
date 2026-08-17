/*
 * KNX Internal API Header
 *
 * Shared macros and declarations for the knx_protocol module.
 * Not for use outside of the knx_protocol layer.
 */
#ifndef KNX_INTERNAL_H
#define KNX_INTERNAL_H

#include "knx_handle.h"
#include "hooch_protocol.h"
#include <string.h>
#include <stdio.h>

#if KNX_LOG_ENABLE
#define KNX_DESC(text) (text)
#define KNX_SUMMARY_EMIT_ITEM(item_desc, frame_ptr) knx_summary_emit((item_desc), (frame_ptr))
#define KNX_SUMMARY_EMIT_INDEXED(prefix, index, item_desc, frame_ptr)                            \
    do                                                                                            \
    {                                                                                             \
        char desc[128];                                                                           \
        (void)snprintf(desc, sizeof(desc), "%s[%u] %s", (prefix), (unsigned int)(index),         \
                       (item_desc));                                                              \
        knx_summary_emit(desc, (frame_ptr));                                                      \
    } while (0)
void knx_summary_emit(const char *function_desc, const KNX_Frame_t *frame);
#else
#define KNX_DESC(text) ((const char *)0)
#define KNX_SUMMARY_EMIT_ITEM(item_desc, frame_ptr)                                              \
    do                                                                                            \
    {                                                                                             \
        (void)(item_desc);                                                                        \
        (void)(frame_ptr);                                                                        \
    } while (0)
#define KNX_SUMMARY_EMIT_INDEXED(prefix, index, item_desc, frame_ptr)                             \
    do                                                                                            \
    {                                                                                             \
        (void)(index);                                                                            \
        (void)(item_desc);                                                                        \
        (void)(frame_ptr);                                                                        \
    } while (0)
#endif

/* Endian helpers. */
uint16_t knx_read_be_u16(const uint8_t *data);
int16_t  knx_read_be_s16(const uint8_t *data);
uint32_t knx_read_be_u32(const uint8_t *data);
float    knx_read_be_float(const uint8_t *data);

/* Shared utility. */
void knx_setting_frame_reset(HOOCH_PROTOCOL_SettingFrame_t *setting_frame);

/* ===== Structured data overlays for SettingFrame_t.data[] ================ */

/* KNX TIME payload overlay: hour + minute + second + weekday (4 bytes). */
typedef struct
{
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t weekday;
} KNX_TimeData_t;

/* KNX DATE payload overlay: year(uint16 BE) + month + day (4 bytes). */
typedef struct
{
    uint16_t year;      /* big-endian */
    uint8_t  month;
    uint8_t  day;
} KNX_DateData_t;

/* KNX VERSION payload overlay: major + minor (2 bytes). */
typedef struct
{
    uint8_t major;
    uint8_t minor;
} KNX_VersionData_t;

/* Device summary functions — called from knx_handle_frame(). */
void knx_summary_air_control(const KNX_Frame_t *frame);
void knx_summary_air_config(const KNX_Frame_t *frame);
void knx_summary_key_control(const KNX_Frame_t *frame);
void knx_summary_key_config(const KNX_Frame_t *frame);
void knx_summary_dimming_control(const KNX_Frame_t *frame);
void knx_summary_dimming_config(const KNX_Frame_t *frame);
void knx_summary_curtain_control(const KNX_Frame_t *frame);
void knx_summary_curtain_config(const KNX_Frame_t *frame);
void knx_summary_floor_heating_control(const KNX_Frame_t *frame);
void knx_summary_floor_heating_config(const KNX_Frame_t *frame);
void knx_summary_fresh_air_control(const KNX_Frame_t *frame);
void knx_summary_fresh_air_config(const KNX_Frame_t *frame);
void knx_summary_scene_control(const KNX_Frame_t *frame);
void knx_summary_scene_config(const KNX_Frame_t *frame);
void knx_summary_basic_function(const KNX_Frame_t *frame);
void knx_summary_basic_setting(const KNX_Frame_t *frame);

#endif /* KNX_INTERNAL_H */
