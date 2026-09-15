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
#include "hooch_air_conditioner.h"
#include "hooch_fresh_air.h"
#include "hooch_floor_heating.h"

/* 涂鸦 SDK 内部变量：DP 上报发送类型控制 */
extern DP_SEND_TYPE_E g_dp_send_type;

/* ---- 前向声明：涂鸦群组控制 / 天气查询 helper 函数 ---- */
void tuya_dimmer_light_switch(unsigned short GID, unsigned char State);
void tuya_dimmer_light_adjust(unsigned short GID, unsigned char value);
void tuya_dimmer_light_color_temp(unsigned short GID, unsigned char color_temp);
void tuya_curtain_switch(unsigned short GID, unsigned char State);
void tuya_curtain_pause(unsigned short GID);
void tuya_curtain_adjust(unsigned short GID, unsigned char value);
void tuya_air_conditioner_control(unsigned char type, unsigned char state,
                                  unsigned char temp, unsigned char mode,
                                  unsigned char wind, unsigned short gid);
void tuya_query_weather(unsigned char type);

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
 *     STOP    → 组播下发暂停帧 (dpid1 ENUM control = 0x01)
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
        /* 暂停：dpid1 ENUM control 末字节固定 0x01 */
        tuya_curtain_pause(frame->address);
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
 * 设置上报回调 (配网/时间/天气/人体存在等设置项上报)
 *   event    : 设置上报事件类型
 *   data     : 事件载荷（原始字节，与 KNX/Xiaomi 适配器约定一致）
 *   data_len : 载荷长度
 **********************************************************/
static void tuya_setting_report_callback(
    HOOCH_PROTOCOL_SettingReportEvent_t event,
    const void *data,
    uint16_t data_len)
{
    const unsigned char *payload = (const unsigned char *)data;

    (void)data_len;

    switch (event) {
    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_NETWORK_JOIN:
        /* 配网 → 让 Zigbee 模组开始配网 (0x03, 数据 0x01) */
        TUYA_LOG_INFO("tuya_setting_report_callback: let zigbee start join\n");
        mcu_tx_let_zigbee_start_join();
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_TIME:
        /* 时间 → 向 Zigbee 模组请求时间同步 (0x24) */
        TUYA_LOG_INFO("tuya_setting_report_callback: request time sync\n");
        mcu_tx_request_time_sync();
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_GET_WEATHER:
        /* 获取天气数据（温度/湿度/天气概况/7天预报）→ 天气查询 type 0 (0x3A) */
        TUYA_LOG_INFO("tuya_setting_report_callback: query weather data\n");
        tuya_query_weather(0U);
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_GET_WIND:
        /* 获取风向 → 天气查询 type 1 (0x3A, 仅限涂鸦) */
        TUYA_LOG_INFO("tuya_setting_report_callback: query wind\n");
        tuya_query_weather(1U);
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_GET_CITY:
        /* 获取城市 → 天气查询 type 2 (0x3A, 仅限涂鸦) */
        TUYA_LOG_INFO("tuya_setting_report_callback: query city\n");
        tuya_query_weather(2U);
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_GET_AREA:
        /* 获取区县 → 天气查询 type 3 (0x3A, 仅限涂鸦) */
        TUYA_LOG_INFO("tuya_setting_report_callback: query area\n");
        tuya_query_weather(3U);
        break;

    case HOOCH_PROTOCOL_SETTING_REPORT_EVENT_HUMAN_PRESENCE:
        /* 人体存在 → 上报 DPID_PIR_STATE (人体感应状态) [TODO] 后续可并入 BASE_SET raw */
        g_dp_send_type = DP_SEND_TYPE_REPORT_LINKAGE;
        (void)mcu_dp_enum_update(DPID_PIR_STATE,
                                 (unsigned char)(((payload != NULL) && (payload[0] != 0U)) ? 1U : 0U));
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

/* Hooch 空调模式 → 涂鸦模式枚举 (cold=0, hot=1, dry=2, fan=3, auto=4) */
static unsigned char tuya_air_mode_from_hooch(HOOCH_PROTOCOL_AirConditionerMode_t mode)
{
    switch (mode) {
    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_COOL: return 0U;
    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_HEAT: return 1U;
    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_DRY:  return 2U;
    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_FAN:  return 3U;
    case HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_AUTO: return 4U;
    default:                                       return 0xFFU;
    }
}

/* Hooch 空调风速 → 涂鸦风速枚举 (low=0, mid=1, high=2, auto=3) */
static unsigned char tuya_air_fan_speed_from_hooch(
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed)
{
    switch (fan_speed) {
    case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_LOW:    return 0U;
    case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_MEDIUM: return 1U;
    case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_HIGH:   return 2U;
    case HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_AUTO:   return 3U;
    default:                                              return 0xFFU;
    }
}

/***********************************************************
 * 空调上报回调 (开关/模式/风速/温度上报)
 *   frame->power        : 开关状态
 *   frame->mode         : 模式
 *   frame->fan_speed    : 风速
 *   frame->temperature  : 设定温度
 *   frame->control_item : 本次变化的控制项
 *   → 根据 control_item 上报对应 DP:
 *     POWER       → mcu_dp_bool_update(DPID_AIR_SWITCH, state)
 *     MODE        → mcu_dp_enum_update(DPID_MODE, mode)
 *     FAN_SPEED   → mcu_dp_enum_update(DPID_FAN_SPEED_ENUM, speed)
 *     TEMPERATURE → mcu_dp_value_update(DPID_TEMP_SET, temperature)
 *     ALL*        → 上述各项全量上报
 **********************************************************/
static void tuya_air_conditioner_report_callback(
    const HOOCH_PROTOCOL_AirConditionerFrame_t *frame)
{
    unsigned char mode;
    unsigned char fan_speed;

    if (frame == NULL) {
        return;
    }

    TUYA_LOG_INFO("tuya_air_conditioner_report_callback: power=%d, mode=%d, fan=%d, temp=%d, item=%d\n",
                  frame->power, frame->mode, frame->fan_speed, frame->temperature, frame->control_item);

    mode = tuya_air_mode_from_hooch(frame->mode);
    fan_speed = tuya_air_fan_speed_from_hooch(frame->fan_speed);

    g_dp_send_type = DP_SEND_TYPE_REPORT_LINKAGE;

    switch (frame->control_item) {
    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_POWER:
        (void)mcu_dp_bool_update(DPID_AIR_SWITCH, (unsigned char)frame->power);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MODE:
        if (mode != 0xFFU) {
            (void)mcu_dp_enum_update(DPID_MODE, mode);
        }
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_FAN_SPEED:
        if (fan_speed != 0xFFU) {
            (void)mcu_dp_enum_update(DPID_FAN_SPEED_ENUM, fan_speed);
        }
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMPERATURE:
        (void)mcu_dp_value_update(DPID_TEMP_SET, (unsigned long)frame->temperature);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL:
    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_TUYA:
    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_XIAOMI:
    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_KNX:
        (void)mcu_dp_bool_update(DPID_AIR_SWITCH, (unsigned char)frame->power);
        if (mode != 0xFFU) {
            (void)mcu_dp_enum_update(DPID_MODE, mode);
        }
        if (fan_speed != 0xFFU) {
            (void)mcu_dp_enum_update(DPID_FAN_SPEED_ENUM, fan_speed);
        }
        (void)mcu_dp_value_update(DPID_TEMP_SET, (unsigned long)frame->temperature);
        break;

    default:
        break;
    }
}

/***********************************************************
 * 空调上报回调（地址版，转 Zigbee 组 DP 控制）
 *   frame->address : 目标空调绑定组号(GID)，为 0 时忽略
 *   → 以 GID 组播下发 tuya_air_conditioner_control(type, ...):
 *     POWER       → Type 0 开关
 *     MODE        → Type 1 模式
 *     FAN_SPEED   → Type 2 风速
 *     TEMPERATURE → Type 3 温度
 *     ALL*        → 上述各项逐条下发
 **********************************************************/
static void tuya_air_conditioner_report_callback2(
    const HOOCH_PROTOCOL_AirConditionerFrame_t *frame)
{
    unsigned char mode;
    unsigned char fan_speed;

    if (frame == NULL || frame->address == 0U) {
        return;
    }

    TUYA_LOG_INFO("tuya_air_conditioner_report_callback2: gid=%d, power=%d, mode=%d, fan=%d, temp=%d, item=%d\n",
                  frame->address, frame->power, frame->mode, frame->fan_speed,
                  frame->temperature, frame->control_item);

    mode = tuya_air_mode_from_hooch(frame->mode);
    fan_speed = tuya_air_fan_speed_from_hooch(frame->fan_speed);

    switch (frame->control_item) {
    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_POWER:
        tuya_air_conditioner_control(0U, (unsigned char)frame->power, 0U, 0U, 0U,
                                     frame->address);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_MODE:
        if (mode != 0xFFU) {
            tuya_air_conditioner_control(1U, 0U, 0U, mode, 0U, frame->address);
        }
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_FAN_SPEED:
        if (fan_speed != 0xFFU) {
            tuya_air_conditioner_control(2U, 0U, 0U, 0U, fan_speed, frame->address);
        }
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_TEMPERATURE:
        tuya_air_conditioner_control(3U, 0U, frame->temperature, 0U, 0U, frame->address);
        break;

    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL:
    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_TUYA:
    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_XIAOMI:
    case HOOCH_PROTOCOL_AIR_CONDITIONER_CONTROL_ITEM_ALL_KNX:
        tuya_air_conditioner_control(0U, (unsigned char)frame->power, 0U, 0U, 0U,
                                     frame->address);
        if (mode != 0xFFU) {
            tuya_air_conditioner_control(1U, 0U, 0U, mode, 0U, frame->address);
        }
        if (fan_speed != 0xFFU) {
            tuya_air_conditioner_control(2U, 0U, 0U, 0U, fan_speed, frame->address);
        }
        tuya_air_conditioner_control(3U, 0U, frame->temperature, 0U, 0U, frame->address);
        break;

    default:
        break;
    }
}

/* Hooch 新风模式 → 涂鸦循环模式枚举 (auto=0, indoor_loop=1) */
static unsigned char tuya_fresh_air_mode_from_hooch(HOOCH_PROTOCOL_FreshAirMode_t mode)
{
    switch (mode) {
    case HOOCH_PROTOCOL_FRESH_AIR_MODE_AUTO:   return 0U;
    case HOOCH_PROTOCOL_FRESH_AIR_MODE_MANUAL: return 1U;
    default:                                   return 0xFFU;
    }
}

/* Hooch 新风风速 → 涂鸦风速枚举 (low=0, mid=1, high=2; auto 由循环模式 DP 表达) */
static unsigned char tuya_fresh_air_speed_from_hooch(
    HOOCH_PROTOCOL_FreshAirFanSpeed_t fan_speed)
{
    switch (fan_speed) {
    case HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_LOW:    return 0U;
    case HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_MEDIUM: return 1U;
    case HOOCH_PROTOCOL_FRESH_AIR_FAN_SPEED_HIGH:   return 2U;
    default:                                        return 0xFFU;
    }
}

/***********************************************************
 * 新风上报回调 (开关/循环模式/风速上报)
 *   frame->power        : 开关状态
 *   frame->mode         : 循环模式
 *   frame->fan_speed    : 风速
 *   frame->control_item : 本次变化的控制项
 *   → 根据 control_item 上报对应 DP:
 *     POWER     → mcu_dp_bool_update(DPID_FRESH_AIR_SWITCH, state)
 *     MODE      → mcu_dp_enum_update(DPID_LOOP_MODE, mode)
 *     FAN_SPEED → mcu_dp_enum_update(DPID_FRESH_AIR_SPEED, speed)
 *     ALL*      → 上述各项全量上报
 **********************************************************/
static void tuya_fresh_air_report_callback(
    const HOOCH_PROTOCOL_FreshAirFrame_t *frame)
{
    unsigned char mode;
    unsigned char fan_speed;

    if (frame == NULL) {
        return;
    }

    TUYA_LOG_INFO("tuya_fresh_air_report_callback: power=%d, mode=%d, fan=%d, item=%d\n",
                  frame->power, frame->mode, frame->fan_speed, frame->control_item);

    mode = tuya_fresh_air_mode_from_hooch(frame->mode);
    fan_speed = tuya_fresh_air_speed_from_hooch(frame->fan_speed);

    g_dp_send_type = DP_SEND_TYPE_REPORT_LINKAGE;

    switch (frame->control_item) {
    case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_POWER:
        (void)mcu_dp_bool_update(DPID_FRESH_AIR_SWITCH, (unsigned char)frame->power);
        break;

    case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_MODE:
        if (mode != 0xFFU) {
            (void)mcu_dp_enum_update(DPID_LOOP_MODE, mode);
        }
        break;

    case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_FAN_SPEED:
        if (fan_speed != 0xFFU) {
            (void)mcu_dp_enum_update(DPID_FRESH_AIR_SPEED, fan_speed);
        }
        break;

    case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL:
    case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL_TUYA:
    case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL_XIAOMI:
    case HOOCH_PROTOCOL_FRESH_AIR_CONTROL_ITEM_ALL_KNX:
        (void)mcu_dp_bool_update(DPID_FRESH_AIR_SWITCH, (unsigned char)frame->power);
        if (mode != 0xFFU) {
            (void)mcu_dp_enum_update(DPID_LOOP_MODE, mode);
        }
        if (fan_speed != 0xFFU) {
            (void)mcu_dp_enum_update(DPID_FRESH_AIR_SPEED, fan_speed);
        }
        break;

    default:
        break;
    }
}

/***********************************************************
 * 地暖上报回调 (开关/目标温度上报)
 *   frame->power              : 开关状态
 *   frame->target_temperature : 目标温度
 *   frame->control_item       : 本次变化的控制项
 *   → 根据 control_item 上报对应 DP:
 *     POWER              → mcu_dp_bool_update(DPID_FLOOR_SW, state)
 *     TARGET_TEMPERATURE → mcu_dp_value_update(DPID_FLOOR_TEMP, temperature)
 *     ALL*               → 上述各项全量上报
 *   注: 地暖模式无对应涂鸦 DP，暂不上报
 **********************************************************/
static void tuya_floor_heating_report_callback(
    const HOOCH_PROTOCOL_FloorHeatingFrame_t *frame)
{
    if (frame == NULL) {
        return;
    }

    TUYA_LOG_INFO("tuya_floor_heating_report_callback: power=%d, target_temp=%d, item=%d\n",
                  frame->power, frame->target_temperature, frame->control_item);

    g_dp_send_type = DP_SEND_TYPE_REPORT_LINKAGE;

    switch (frame->control_item) {
    case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_POWER:
        (void)mcu_dp_bool_update(DPID_FLOOR_SW, (unsigned char)frame->power);
        break;

    case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_TARGET_TEMPERATURE:
        (void)mcu_dp_value_update(DPID_FLOOR_TEMP, (unsigned long)frame->target_temperature);
        break;

    case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL:
    case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL_TUYA:
    case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL_XIAOMI:
    case HOOCH_PROTOCOL_FLOOR_HEATING_CONTROL_ITEM_ALL_KNX:
        (void)mcu_dp_bool_update(DPID_FLOOR_SW, (unsigned char)frame->power);
        (void)mcu_dp_value_update(DPID_FLOOR_TEMP, (unsigned long)frame->target_temperature);
        break;

    default:
        break;
    }
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
    HOOCH_PROTOCOL_AirConditioner_RegisterReportCallback(tuya_air_conditioner_report_callback);   /*直接上报*/
    HOOCH_PROTOCOL_AirConditioner_RegisterReportCallback2(tuya_air_conditioner_report_callback2); /*地址版：组播下发*/
    HOOCH_PROTOCOL_FreshAir_RegisterReportCallback(tuya_fresh_air_report_callback);
    HOOCH_PROTOCOL_FloorHeating_RegisterReportCallback(tuya_floor_heating_report_callback);
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
 * 组包格式 (UART_CMD_SEND_CMD_GROUP), 实测确认所有 2 字节字段统一高字节在前:
 *   Byte[0-1]: GID (Big-Endian, 群组地址)
 *   Byte[2-3]: Cluster ID 0x0300 (Big-Endian, Color Control)
 *   Byte[4]  : 0x0A  (MoveToColorTemperature)
 *   Byte[5-6]: ColorTemperatureMireds (Big-Endian, uint16)
 *   Byte[7-8]: Transition Time (Big-Endian = 0x0015)
 *
 * color_temp 为 0~100 归一化百分比: 0=1800K(最暖), 100=6500K(最冷),
 * 内部按 K 线性插值后换算 mireds = 1,000,000 / K。
 */
void tuya_dimmer_light_color_temp(unsigned short GID, unsigned char color_temp)
{
    unsigned short payload_len = 0;

    /* 0~100 百分比 → 色温 K (1800K ~ 6500K 线性) */
    unsigned short kelvin = (unsigned short)(1800U + (4700U * color_temp) / 100U);
    /* K → mireds (四舍五入) */
    unsigned short mireds = (unsigned short)((1000000U + kelvin / 2U) / kelvin);

    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID >> 8));     /* GID high */
    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID & 0x00FF)); /* GID low  */
    uart_framing_fill_payload_byte(&payload_len, 0x03U);   /* cluster 0x0300 high */
    uart_framing_fill_payload_byte(&payload_len, 0x00U);   /* cluster 0x0300 low  */
    uart_framing_fill_payload_byte(&payload_len, 0x0AU);   /* MoveToColorTemperature */
    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(mireds >> 8));   /* mireds high */
    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(mireds & 0xFF)); /* mireds low  */
    uart_framing_fill_payload_byte(&payload_len, 0x00U);   /* transition time */
    uart_framing_fill_payload_byte(&payload_len, 0x15U);   /* transition time */

    uart_framing_fill_protocol_field(UART_CMD_SEND_CMD_GROUP, payload_len, NULL);
    uart_send_frame(payload_len);
}


/* 窗帘开关
 *
 * 实测：窗帘必须走 DP 组命令 UART_CMD_SEND_DP_GROUP(0x43)，用 0x42 (标准命令组) 无效！
 * 负载与金威利原版 mcu_api.c Broadcast_Command(Type=1) 逐字节一致，勿改动顺序。
 *
 * 帧1 (仅开/关; 按 DP 组解析: dpid=0x01 ENUM control):
 *   Byte[0-1]: GID (Big-Endian, 群组地址)
 *   Byte[2]  : dpid 0x01 (control)
 *   Byte[3]  : dp type 0x04 (ENUM)
 *   Byte[4-5]: dp len 0x0001
 *   Byte[6]  : 0x00=开, 0x02=关
 *
 * 帧2 (仅开时发送; 按 DP 组解析: dpid=0x02 VALUE percent):
 *   Byte[0-1]: GID
 *   Byte[2]  : dpid 0x02 (percent)
 *   Byte[3]  : dp type 0x02 (VALUE)
 *   Byte[4-5]: dp len 0x0004
 *   Byte[6-9]: 0x00,0x00,0x00,value (32bit 大端, 0~100)
 *   Byte[9]  : 0x64 (开合百分比, 固定 100 = 全开)
 */
void tuya_curtain_switch(unsigned short GID, unsigned char State)
{
    unsigned short payload_len = 0;
    unsigned char last_byte = (State != 0U) ? 0x00U : 0x02U;

    /* 帧1: dpid1 ENUM control, 开=0x00 / 关=0x02 */
    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID >> 8));
    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID & 0x00FF));
    uart_framing_fill_payload_byte(&payload_len, 0x01U);
    uart_framing_fill_payload_byte(&payload_len, 0x04U);
    uart_framing_fill_payload_byte(&payload_len, 0x00U);
    uart_framing_fill_payload_byte(&payload_len, 0x01U);
    uart_framing_fill_payload_byte(&payload_len, last_byte);
    uart_framing_fill_protocol_field(UART_CMD_SEND_DP_GROUP, payload_len, NULL);
    uart_send_frame(payload_len);

    // /* 帧2: 仅窗帘开时发送, dpid2 VALUE percent = 100 */
    // if (State != 0U)
    // {
    //     payload_len = 0;
    //     uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID >> 8));
    //     uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID & 0x00FF));
    //     uart_framing_fill_payload_byte(&payload_len, 0x02U);
    //     uart_framing_fill_payload_byte(&payload_len, 0x02U);
    //     uart_framing_fill_payload_byte(&payload_len, 0x00U);
    //     uart_framing_fill_payload_byte(&payload_len, 0x04U);
    //     uart_framing_fill_payload_byte(&payload_len, 0x00U);
    //     uart_framing_fill_payload_byte(&payload_len, 0x00U);
    //     uart_framing_fill_payload_byte(&payload_len, 0x00U);
    //     uart_framing_fill_payload_byte(&payload_len, 0x64U);
    //     uart_framing_fill_protocol_field(UART_CMD_SEND_DP_GROUP, payload_len, NULL);
    //     uart_send_frame(payload_len);
    // }
}

/* 窗帘暂停
 *
 * 实测：走 DP 组命令 UART_CMD_SEND_DP_GROUP(0x43)，与 tuya_curtain_switch 帧1 同构。
 * dpid1 ENUM control 末字节固定 0x01（0=开, 1=暂停, 2=关）。
 *
 * 帧 (payload 7 字节):
 *   Byte[0-1]: GID (Big-Endian, 群组地址)
 *   Byte[2]  : dpid 0x01 (control)
 *   Byte[3]  : dp type 0x04 (ENUM)
 *   Byte[4-5]: dp len 0x0001
 *   Byte[6]  : 0x01 (暂停)
 */
void tuya_curtain_pause(unsigned short GID)
{
    unsigned short payload_len = 0;

    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID >> 8));
    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(GID & 0x00FF));
    uart_framing_fill_payload_byte(&payload_len, 0x01U);
    uart_framing_fill_payload_byte(&payload_len, 0x04U);
    uart_framing_fill_payload_byte(&payload_len, 0x00U);
    uart_framing_fill_payload_byte(&payload_len, 0x01U);
    uart_framing_fill_payload_byte(&payload_len, 0x01U);
    uart_framing_fill_protocol_field(UART_CMD_SEND_DP_GROUP, payload_len, NULL);
    uart_send_frame(payload_len);
}





/* 窗帘调整行程
 *
 * 实测：必须走 DP 组命令 UART_CMD_SEND_DP_GROUP(0x43)，与 tuya_curtain_switch 同理。
 *
 * 组包格式 (按 DP 组解析: dpid=0x02 VALUE percent):
 *   Byte[0-1]: GID (Big-Endian, 群组地址)
 *   Byte[2]  : dpid 0x02 (percent)
 *   Byte[3]  : dp type 0x02 (VALUE)
 *   Byte[4-5]: dp len 0x0004
 *   Byte[6-9]: 0x00,0x00,0x00,value (32bit 大端, 0~100)
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

    uart_framing_fill_protocol_field(UART_CMD_SEND_DP_GROUP, payload_len, NULL);
    uart_send_frame(payload_len);
}

/* 空调控制 (Zigbee 组 DP 控制)
 *
 * 移植自金威利原版 Air_Zigbee_Superman_Controls (ctrl_sys.c)，命令号
 * UART_CMD_SEND_DP_GROUP(0x43)，负载与原版逐字节一致，勿改动顺序。
 * GID 为目标空调的绑定组号。Type 决定控制项 (按原版语义):
 *   Type 0: 开关   dpid 0x01  BOOL,  state&0x01
 *   Type 1: 模式   dpid 0x04  ENUM,  mode (0=制冷 1=制热 2=除湿 3=送风)
 *   Type 2: 风速   dpid 0x05  ENUM,  wind (0=低风 1=中风 2=高风 3=自动)
 *   Type 3: 温度   dpid 0x02  VALUE, temp (℃, 32bit 大端)
 */
void tuya_air_conditioner_control(unsigned char type, unsigned char state,
                                  unsigned char temp, unsigned char mode,
                                  unsigned char wind, unsigned short gid)
{
    unsigned short payload_len = 0;

    /* GID (Big-Endian) */
    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(gid >> 8));
    uart_framing_fill_payload_byte(&payload_len, (unsigned char)(gid & 0x00FF));

    switch (type) {
    case 0U: /* 开关: dpid 0x01, BOOL */
        uart_framing_fill_payload_byte(&payload_len, 0x01U);
        uart_framing_fill_payload_byte(&payload_len, 0x01U);
        uart_framing_fill_payload_byte(&payload_len, 0x00U);
        uart_framing_fill_payload_byte(&payload_len, 0x01U);
        uart_framing_fill_payload_byte(&payload_len, (unsigned char)(state & 0x01U));
        break;

    case 1U: /* 模式: dpid 0x04, ENUM */
        uart_framing_fill_payload_byte(&payload_len, 0x04U);
        uart_framing_fill_payload_byte(&payload_len, 0x04U);
        uart_framing_fill_payload_byte(&payload_len, 0x00U);
        uart_framing_fill_payload_byte(&payload_len, 0x01U);
        uart_framing_fill_payload_byte(&payload_len, mode);
        break;

    case 2U: /* 风速: dpid 0x05, ENUM */
        uart_framing_fill_payload_byte(&payload_len, 0x05U);
        uart_framing_fill_payload_byte(&payload_len, 0x04U);
        uart_framing_fill_payload_byte(&payload_len, 0x00U);
        uart_framing_fill_payload_byte(&payload_len, 0x01U);
        uart_framing_fill_payload_byte(&payload_len, wind);
        break;

    case 3U: /* 温度: dpid 0x02, VALUE (4 byte 大端) */
        uart_framing_fill_payload_byte(&payload_len, 0x02U);
        uart_framing_fill_payload_byte(&payload_len, 0x02U);
        uart_framing_fill_payload_byte(&payload_len, 0x00U);
        uart_framing_fill_payload_byte(&payload_len, 0x04U);
        uart_framing_fill_payload_byte(&payload_len, 0x00U);
        uart_framing_fill_payload_byte(&payload_len, 0x00U);
        uart_framing_fill_payload_byte(&payload_len, 0x00U);
        uart_framing_fill_payload_byte(&payload_len, temp);
        break;

    default:
        return;
    }

    uart_framing_fill_protocol_field(UART_CMD_SEND_DP_GROUP, payload_len, NULL);
    uart_send_frame(payload_len);
}

/* 天气查询 (Zigbee 模块, 命令号 0x3A)
 *
 * 移植自金威利原版 Zigbee_Query_Weather (ctrl_sys.c)，命令号
 * UART_CMD_WEATHER_REQUEST(0x3A)。Type 决定请求内容:
 *   Type 0: 温度/湿度/天气概况 + 7 天预报
 *   Type 1: 风向/风况等 (06/07/08), 原版未请求预报与实时
 *   Type 2: 城市 (负载为 c.city 文本标志, 与原版逐字节一致)
 *   Type 3: 区县 (负载为 c.area/c.city 文本标志, 与原版逐字节一致)
 * 注: Type 2/3 文本标志段原版写法较乱但模块可识别，勿改动；
 *     若新模块按官方协议解析, 可改用 SDK 自带 mcu_tx_city_request(0x00)。
 */
void tuya_query_weather(unsigned char type)
{
    unsigned short payload_len = 0;

    uart_framing_fill_payload_byte(&payload_len, 0x11U);   /* 协议版本 0x11 */
    uart_framing_fill_payload_byte(&payload_len, 0x00U);   /* 取网关设备地址 */

    switch (type) {
    case 0U:
        uart_framing_fill_payload_byte(&payload_len, 0x01U);   /* 温度 */
        uart_framing_fill_payload_byte(&payload_len, 0x02U);   /* 湿度 */
        uart_framing_fill_payload_byte(&payload_len, 0x03U);   /* 天气概况 */
        uart_framing_fill_payload_byte(&payload_len, 0x12U);   /* 天气预报标志 */
        uart_framing_fill_payload_byte(&payload_len, 0x07U);   /* 7 天 */
        uart_framing_fill_payload_byte(&payload_len, 0x13U);   /* 实时天气标志 */
        uart_framing_fill_payload_byte(&payload_len, 0x00U);   /* 不需要实时 */
        break;

    case 1U:
        uart_framing_fill_payload_byte(&payload_len, 0x06U);
        uart_framing_fill_payload_byte(&payload_len, 0x07U);
        uart_framing_fill_payload_byte(&payload_len, 0x08U);
        uart_framing_fill_payload_byte(&payload_len, 0x12U);
        uart_framing_fill_payload_byte(&payload_len, 0x00U);
        uart_framing_fill_payload_byte(&payload_len, 0x13U);
        uart_framing_fill_payload_byte(&payload_len, 0x00U);
        break;

    case 2U: /* 城市: "c.city" "c.city" 截断段 */
        uart_framing_fill_payload_byte(&payload_len, 0x63U);
        uart_framing_fill_payload_byte(&payload_len, 0x2EU);
        uart_framing_fill_payload_byte(&payload_len, 0x63U);
        uart_framing_fill_payload_byte(&payload_len, 0x69U);
        uart_framing_fill_payload_byte(&payload_len, 0x74U);
        uart_framing_fill_payload_byte(&payload_len, 0x79U);
        uart_framing_fill_payload_byte(&payload_len, 0x63U);
        uart_framing_fill_payload_byte(&payload_len, 0x2EU);
        uart_framing_fill_payload_byte(&payload_len, 0x63U);
        uart_framing_fill_payload_byte(&payload_len, 0x69U);
        uart_framing_fill_payload_byte(&payload_len, 0x74U);
        break;

    case 3U: /* 区县: "c.area" "c.city" 截断段 */
        uart_framing_fill_payload_byte(&payload_len, 0x63U);
        uart_framing_fill_payload_byte(&payload_len, 0x2EU);
        uart_framing_fill_payload_byte(&payload_len, 0x61U);
        uart_framing_fill_payload_byte(&payload_len, 0x72U);
        uart_framing_fill_payload_byte(&payload_len, 0x65U);
        uart_framing_fill_payload_byte(&payload_len, 0x61U);
        uart_framing_fill_payload_byte(&payload_len, 0x63U);
        uart_framing_fill_payload_byte(&payload_len, 0x2EU);
        uart_framing_fill_payload_byte(&payload_len, 0x63U);
        uart_framing_fill_payload_byte(&payload_len, 0x69U);
        uart_framing_fill_payload_byte(&payload_len, 0x74U);
        break;

    default:
        return;
    }

    uart_framing_fill_protocol_field(UART_CMD_WEATHER_REQUEST, payload_len, NULL);
    uart_send_frame(payload_len);
}

