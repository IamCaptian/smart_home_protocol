#ifndef HOOCH_SETTING_REPORT_H
#define HOOCH_SETTING_REPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hooch_setting.h"
#include <string.h>
typedef enum
{
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_INVALID = 0U,          /* [0]  无效事件 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_NETWORK_JOIN,          /* [1]  配网 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_NETWORK_LEAVE,         /* [2]  退网 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_RESET,                 /* [3]  重置 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_PRODUCTION_TEST,       /* [4]  产测 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_DATE,                  /* [5]  日期 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_TIME,                  /* [6]  时间 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_VERSION_INFO,          /* [7]  版本信息 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_KNX_UPDATE_CONFIG,     /* [8]  KNX拉取配置 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_HUMAN_PRESENCE,       /* [9]  人体存在 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_KNX_HEARTBEAT,         /* [10] KNX心跳 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_GET_WEATHER,           /* [11] 获取天气 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_UPDATE_CONFIG,         /* [12] 更新配置 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_PANEL_UPDATE_STATUS,   /* [13] 面板更新状态 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_DEVICE_TYPE,           /* [14] 设备类型 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_PROGRAMMING_MODE,      /* [15] 编程模式 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_TEMPERATURE_SENSOR,    /* [16] 温度传感器 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_HUMIDITY_SENSOR,       /* [17] 湿度传感器 */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_GET_WIND,              /* [18] 获取风向（仅限涂鸦） */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_GET_CITY,              /* [19] 获取城市（仅限涂鸦） */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_GET_AREA,              /* [20] 获取区县（仅限涂鸦） */
    HOOCH_PROTOCOL_SETTING_REPORT_EVENT_COUNT                   /* [21] 枚举数量（非有效事件） */
} HOOCH_PROTOCOL_SettingReportEvent_t;

typedef void (*HOOCH_PROTOCOL_SettingReportCallback_t)(
    HOOCH_PROTOCOL_SettingReportEvent_t event,
    const void *data,
    uint16_t data_len);

void HOOCH_SettingReport_Init(void);

void HOOCH_PROTOCOL_SettingReport_Clear(void);

HOOCH_PROTOCOL_SettingResult_t HOOCH_PROTOCOL_SettingReport_SetEvent(
    HOOCH_PROTOCOL_SettingReportEvent_t event,
    const void *data,
    uint16_t data_len);

HOOCH_PROTOCOL_SettingReportEvent_t HOOCH_PROTOCOL_SettingReport_GetEvent(void);

const void *HOOCH_PROTOCOL_SettingReport_GetData(void);

uint16_t HOOCH_PROTOCOL_SettingReport_GetDataLen(void);

void HOOCH_PROTOCOL_SettingReport_RegisterCallback(
    HOOCH_PROTOCOL_SettingReportCallback_t callback);

void HOOCH_PROTOCOL_SettingReport_UnregisterCallback(void);

HOOCH_PROTOCOL_SettingResult_t HOOCH_PROTOCOL_SettingReport_Send(
    HOOCH_PROTOCOL_SettingReportEvent_t event,
    const void *data,
    uint16_t data_len);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_SETTING_REPORT_H */
