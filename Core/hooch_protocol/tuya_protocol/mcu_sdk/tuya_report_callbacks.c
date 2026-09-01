/***********************************************************
 * @file     tuya_report_callbacks.c
 * @brief    涂鸦上报回调实现：将 Hooch 协议层上报数据转换为涂鸦 DP 更新
 * @version  1.0.0
 * @date     2026.07.21
 * @copyright Copyright (c) 2024
 **********************************************************/

#include <stddef.h>
#include "protocol.h"
#include "system.h"
#include "tuya_protocol_uart.h"
#include "hooch_key_status.h"
#include "hooch_key_click_report.h"
#include "hooch_dimmer_light.h"
#include "hooch_curtain.h"
#include "hooch_setting.h"
#include "hooch_setting_report.h"
#include "hooch_code_match_dispatch_report.h"
#include "hooch_scene_report.h"

/* 涂鸦 SDK 内部变量：DP 上报发送类型控制 */
extern DP_SEND_TYPE_E g_dp_send_type;

/* ---- 前向声明：涂鸦群组控制 helper 函数 ---- */
void tuya_dimmer_light_switch(unsigned short GID, unsigned char State);
void tuya_dimmer_light_adjust(unsigned short GID, unsigned char value);
void tuya_dimmer_light_color_temp(unsigned short GID, unsigned char color_temp);
void tuya_curtain_switch(unsigned short GID, unsigned char State);
void tuya_curtain_adjust(unsigned short GID, unsigned char value);

/***********************************************************
 * Helper: 将 Hooch 按键编号 (1~8) 映射到涂鸦 DPID_SWITCH_*
 *   Key: 1→DPID_SWITCH_1(24), 2→(25), 3→(26), 4→(27),
 *        5→(28), 6→(29), 7→DPID_SWITCH_7(137), 8→(138)
 **********************************************************/
static unsigned char switch_key_to_dp_id(unsigned char key)
{
    if (key >= 1U && key <= 6U) {
        return (unsigned char)(DPID_SWITCH_1 + key - 1U);
    } else if (key == 7U) {
        return DPID_SWITCH_7;
    } else if (key == 8U) {
        return DPID_SWITCH_8;
    }
    return 0U; /* invalid */
}

/***********************************************************
 * 按键状态上报回调 (继电器开关状态上报)
 *   frame->key   : 按键编号 1~8
 *   frame->state : 0=关, 1=开
 *   → mcu_dp_bool_update(DPID_SWITCH_x, state)
 **********************************************************/
static void tuya_key_status_report_callback(
    const HOOCH_PROTOCOL_KeyStatusFrame_t *frame)
{
    if (frame == NULL) {
        return;
    }

    TUYA_LOG_INFO("tuya_key_status_report_callback: key=%d, state=%d\n", frame->key, frame->state);

    unsigned char dp_id = switch_key_to_dp_id((unsigned char)frame->key);
    if (dp_id == 0U) {
        return;
    }
    
    unsigned char switch_val = (frame->state == HOOCH_PROTOCOL_KEY_STATUS_STATE_ON) ? 1U : 0U;
    g_dp_send_type = DP_SEND_TYPE_REPORT_LINKAGE;
    (void)mcu_dp_bool_update(dp_id, switch_val);
}



/***********************************************************
 * 调光灯上报回调 (开关/亮度/色温上报)
 *   frame->key          : 调光灯编号 1~8
 *   frame->switch_state : 开关状态 0/1
 *   frame->brightness   : 亮度值
 *   frame->control_item : 本次变化的控制项
 *   → 根据 control_item 选择上报对应 DP:
 *     SWITCH     → mcu_dp_bool_update(DPID_SWITCH_x, state)
 *     BRIGHTNESS → [TODO] 无独立亮度 DP，暂通过开关 DP 上报
 *     ALL        → 全量上报
 **********************************************************/
static void tuya_dimmer_light_callback(
    const HOOCH_PROTOCOL_DimmerLightFrame_t *frame)
{
    if (frame == NULL || frame->address == 0U) {
        return;
    }

    switch (frame->control_item) {
    case HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_SWITCH:
        tuya_dimmer_light_switch(frame->address, frame->switch_state);
        break;

    case HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_BRIGHTNESS:
        tuya_dimmer_light_adjust(frame->address, frame->brightness);
        break;

    case HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_COLOR_TEMP:
        tuya_dimmer_light_color_temp(frame->address, frame->color_temperature);
        break;

    case HOOCH_PROTOCOL_DIMMER_LIGHT_CONTROL_ITEM_ALL:
        tuya_dimmer_light_switch(frame->address, frame->switch_state);
        tuya_dimmer_light_adjust(frame->address, frame->brightness);
        break;

    default:
        break;
    }
}

/***********************************************************
 * 窗帘上报回调 (开关/停止/百分比/角度上报)
 *   frame->key           : 窗帘编号 1~8
 *   frame->switch_status : 开关状态
 *   frame->stop          : 停止控制
 *   frame->percent       : 开合百分比
 *   frame->angle         : 开合角度
 *   frame->control_item  : 本次变化的控制项
 *   → 根据 control_item 选择上报:
 *     SWITCH  → mcu_dp_bool_update(DPID_SWITCH_x, state)
 *     PERCENT → [TODO] 无独立百分比 DP
 *     STOP    → [TODO] 无独立停止 DP
 *     ANGLE   → [TODO] 无独立角度 DP
 **********************************************************/
static void tuya_curtain_report_callback(
    const HOOCH_PROTOCOL_CurtainFrame_t *frame)
{
    if (frame == NULL || frame->address == 0U) {
        return;
    }

    switch (frame->control_item) {
    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_SWITCH:
        tuya_curtain_switch(frame->address, frame->switch_status.value);
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_PERCENT:
        tuya_curtain_adjust(frame->address, frame->percent.value);
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_STOP:
        /* 停止为瞬时动作，无对应持续性 DP，暂不处理 */
        break;

    case HOOCH_PROTOCOL_CURTAIN_CONTROL_ITEM_ALL:
        tuya_curtain_switch(frame->address, frame->switch_status.value);
        tuya_curtain_adjust(frame->address, frame->percent.value);
        break;

    default:
        break;
    }
}

/***********************************************************
 * 设置上报回调 (主题/童锁/屏保/背光等设置项上报)
 *   event    : 设置上报事件类型
 *   data     : 事件关联数据（通常为 HOOCH_PROTOCOL_SettingFrame_t*）
 *   data_len : 数据长度
 **********************************************************/
static void tuya_setting_report_callback(
    HOOCH_PROTOCOL_SettingReportEvent_t event,
    const void *data,
    uint16_t data_len)
{
    const HOOCH_PROTOCOL_SettingFrame_t *sf = (const HOOCH_PROTOCOL_SettingFrame_t *)data;

    if (sf == NULL) {
        return;
    }

    switch (event) {
    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_HUMAN_PRESENCE:
        /* 人体存在 → 作为 BASE_SET raw 的一部分 [TODO] 需拼接完整帧 */
        g_dp_send_type = DP_SEND_TYPE_REPORT_LINKAGE;
        (void)mcu_dp_enum_update(DPID_PIR_STATE, (unsigned char)(sf->value != 0U ? 1U : 0U));
        break;
    default:
        break;
    }
}

/***********************************************************
 * 情景上报回调
 *   frame->key : 情景按键编号 1~8
 *   → mcu_dp_enum_update(DPID_SCENE_x, key)
 **********************************************************/
static void tuya_scene_report_callback(
    const HOOCH_PROTOCOL_SceneReportFrame_t *frame)
{
    if (frame == NULL) {
        return;
    }

    unsigned char key = (unsigned char)frame->key;
    if (key < 1U || key > 8U) {
        return;
    }

    g_dp_send_type = DP_SEND_TYPE_REPORT_LINKAGE;
    (void)mcu_dp_enum_update(
        (unsigned char)(DPID_SCENE_1 + key - 1U),
        key);
}

/***********************************************************
 * 涂鸦 UART 初始化 & 回调注册
 **********************************************************/
void tuya_uart_init(void)
{
    /* 环形缓冲区在 system.c 中已通过静态初始化完成，此处无需额外操作 */
    HOOCH_PROTOCOL_KeyStatus_RegisterReportCallback(tuya_key_status_report_callback);
    HOOCH_PROTOCOL_DimmerLight_RegisterReportCallback3(tuya_dimmer_light_callback);
    HOOCH_PROTOCOL_Curtain_RegisterReportCallback3(tuya_curtain_report_callback);
    HOOCH_PROTOCOL_SettingReport_RegisterCallback(tuya_setting_report_callback);
    HOOCH_PROTOCOL_SceneReport_RegisterReportCallback(tuya_scene_report_callback);
}




/* 调光灯开关
 *
 * 组包格式 (UART_CMD_SEND_CMD_GROUP):
 *   Byte[0-1]: GID (Big-Endian, 群组地址)
 *   Byte[2-3]: Cluster ID 0x0006 (Little-Endian, On/Off)
 *   Byte[4]  : 开=0x01, 关=0x00
 */
void tuya_dimmer_light_switch(unsigned short GID, unsigned char State)
{
    unsigned short payload_len = 0;
    unsigned char cluster_val = (State != 0U) ? 0x01U : 0x00U;

    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID >> 8));     /* GID high */
    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID & 0x00FF)); /* GID low  */
    uart_framing_fill_payload_byte(&payload_len, 0x00U);   /* cluster */
    uart_framing_fill_payload_byte(&payload_len, 0x06U);   /* cluster */
    uart_framing_fill_payload_byte(&payload_len, cluster_val); /* 开=0x01, 关=0x00 */

    uart_framing_fill_protocol_field(UART_CMD_SEND_CMD_GROUP, payload_len, NULL);
    uart_send_frame(payload_len);
}

/* 调光灯调光
 *
 * 组包格式 (UART_CMD_SEND_CMD_GROUP):
 *   Byte[0-1]: GID (Big-Endian, 群组地址)
 *   Byte[2-3]: Cluster ID 0x0008 (Little-Endian, Level Control)
 *   Byte[4]  : 0x00  (MoveToLevel)
 *   Byte[5]  : value (亮度值 0~255)
 *   Byte[6-7]: 0x00, 0x15 (Transition Time, Little-Endian)
 */
void tuya_dimmer_light_adjust(unsigned short GID, unsigned char value)
{
    unsigned short payload_len = 0;

    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID >> 8));     /* GID high */
    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID & 0x00FF)); /* GID low  */
    uart_framing_fill_payload_byte(&payload_len, 0x00U);   /* cluster */
    uart_framing_fill_payload_byte(&payload_len, 0x08U);   /* cluster */
    uart_framing_fill_payload_byte(&payload_len, 0x00U);   /* cluster */
    uart_framing_fill_payload_byte(&payload_len, value);   /* brightness */
    uart_framing_fill_payload_byte(&payload_len, 0x00U);   /* cluster */
    uart_framing_fill_payload_byte(&payload_len, 0x15U);   /* cluster */

    uart_framing_fill_protocol_field(UART_CMD_SEND_CMD_GROUP, payload_len, NULL);
    uart_send_frame(payload_len);
}


/* 调光灯色温
 *
 * 组包格式 (UART_CMD_SEND_CMD_GROUP):
 *   Byte[0-1]: GID (Big-Endian, 群组地址)
 *   Byte[2-3]: Cluster ID 0x0300 (Little-Endian, Color Control)
 *   Byte[4]  : 0x0A  (MoveToColorTemperature)
 *   Byte[5-6]: Color Temperature Mireds (Little-Endian, uint16)
 *   Byte[7-8]: 0x00, 0x15 (Transition Time, Little-Endian)
 */
void tuya_dimmer_light_color_temp(unsigned short GID, unsigned char color_temp)
{
    unsigned short payload_len = 0;

    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID >> 8));     /* GID high */
    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID & 0x00FF)); /* GID low  */
    uart_framing_fill_payload_byte(&payload_len, 0x00U);   /* cluster 0x0300 */
    uart_framing_fill_payload_byte(&payload_len, 0x03U);   /* cluster */
    uart_framing_fill_payload_byte(&payload_len, 0x0AU);   /* MoveToColorTemperature */
    uart_framing_fill_payload_byte(&payload_len, color_temp);      /* mireds low */
    uart_framing_fill_payload_byte(&payload_len, 0x00U);           /* mireds high */
    uart_framing_fill_payload_byte(&payload_len, 0x00U);   /* transition time */
    uart_framing_fill_payload_byte(&payload_len, 0x15U);   /* transition time */

    uart_framing_fill_protocol_field(UART_CMD_SEND_CMD_GROUP, payload_len, NULL);
    uart_send_frame(payload_len);
}


/* 窗帘开关
 *
 * 帧1 组包格式 (UART_CMD_SEND_CMD_GROUP):
 *   Byte[0-1]: GID (Big-Endian, 群组地址)
 *   Byte[2-3]: Cluster ID 0x0104 (Little-Endian)
 *   Byte[4-5]: 0x00, 0x01
 *   Byte[6]  : 开=0x00, 关=0x02
 *
 * 帧2 组包格式 (仅开时发送):
 *   Byte[0-1]: GID
 *   Byte[2-3]: Cluster ID 0x0202 (Little-Endian)
 *   Byte[4]  : 0x00
 *   Byte[5]  : 0x04  (GoToLiftPercentage)
 *   Byte[6-8]: 0x00, 0x00, 0x00
 *   Byte[9]  : 0x64  (开合百分比, 固定100=全开)
 */
void tuya_curtain_switch(unsigned short GID, unsigned char State)
{
    unsigned short payload_len = 0;
    unsigned char last_byte = (State != 0U) ? 0x00U : 0x02U;

    /* 帧1: on/off cluster (0x0202? -> 0x0104, cmd data: 0x00, 0x01, last) */
    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID >> 8));
    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID & 0x00FF));
    uart_framing_fill_payload_byte(&payload_len, 0x01U);
    uart_framing_fill_payload_byte(&payload_len, 0x04U);
    uart_framing_fill_payload_byte(&payload_len, 0x00U);
    uart_framing_fill_payload_byte(&payload_len, 0x01U);
    uart_framing_fill_payload_byte(&payload_len, last_byte);  /* 开=0x00, 关=0x02 */
    uart_framing_fill_protocol_field(UART_CMD_SEND_CMD_GROUP, payload_len, NULL);
    uart_send_frame(payload_len);

    /* 帧2: 仅窗帘开时发送 level control cluster (0x0202) */
    if (State != 0U)
    {
        payload_len = 0;
        uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID >> 8));
        uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID & 0x00FF));
        uart_framing_fill_payload_byte(&payload_len, 0x02U);
        uart_framing_fill_payload_byte(&payload_len, 0x02U);
        uart_framing_fill_payload_byte(&payload_len, 0x00U);
        uart_framing_fill_payload_byte(&payload_len, 0x04U);
        uart_framing_fill_payload_byte(&payload_len, 0x00U);
        uart_framing_fill_payload_byte(&payload_len, 0x00U);
        uart_framing_fill_payload_byte(&payload_len, 0x00U);
        /* [TODO] value 未定义，需确认来源（开合百分比？） */
        uart_framing_fill_payload_byte(&payload_len, 0x64U);
        uart_framing_fill_protocol_field(UART_CMD_SEND_CMD_GROUP, payload_len, NULL);
        uart_send_frame(payload_len);
    }
}





/* 窗帘调整行程
 *
 * 组包格式 (UART_CMD_SEND_CMD_GROUP):
 *   Byte[0-1]: GID (Big-Endian, 群组地址)
 *   Byte[2-3]: Cluster ID 0x0202 (Little-Endian)
 *   Byte[4]  : 0x00
 *   Byte[5]  : 0x04  (GoToLiftPercentage)
 *   Byte[6-8]: 0x00, 0x00, 0x00
 *   Byte[9]  : value (开合百分比 0~100)
 */
void tuya_curtain_adjust(unsigned short GID, unsigned char value)
{
    unsigned short payload_len = 0;

    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID >> 8));
    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID & 0x00FF));
    uart_framing_fill_payload_byte(&payload_len, 0x02U);
    uart_framing_fill_payload_byte(&payload_len, 0x02U);
    uart_framing_fill_payload_byte(&payload_len, 0x00U);
    uart_framing_fill_payload_byte(&payload_len, 0x04U);
    uart_framing_fill_payload_byte(&payload_len, 0x00U);
    uart_framing_fill_payload_byte(&payload_len, 0x00U);
    uart_framing_fill_payload_byte(&payload_len, 0x00U);
    uart_framing_fill_payload_byte(&payload_len, value);   /* 开合百分比 */

    uart_framing_fill_protocol_field(UART_CMD_SEND_CMD_GROUP, payload_len, NULL);
    uart_send_frame(payload_len);
}

