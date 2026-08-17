/***********************************************************
 * @file     mcu_sdk_types.h
 * @brief    shared type definitions for the MCU SDK headers.
 * @version  3.3.5
 * @date     2026.04.08
 * @copyright Copyright (c) tuya.inc 2024
 **********************************************************/

#ifndef __MCU_SDK_TYPES_H_
#define __MCU_SDK_TYPES_H_

#ifdef __cplusplus
extern "C"
{
#endif

/***********************************************************
 * Typedef Definitions
 **********************************************************/

///< time sync (uart command id: 0x24)
typedef struct TIME_SYNC_CALENDAR {
    unsigned short w_year;
    unsigned char w_month;
    unsigned char w_day;
    unsigned char hour;         // + 8 for BeiJing time
    unsigned char min;
    unsigned char sec;
} TIME_SYNC_CALENDAR_T;

///< zigbee module network parameter (uart command id: 0x26)
typedef struct NWK_PARAM {
    unsigned short heart_period;
    unsigned short join_timeout;
    unsigned short rejoin_interval;
    unsigned short poll_interval;
    unsigned short fast_poll_period;            // power on poll period
    unsigned char poll_fail_times;
    unsigned char app_data_trig_rejoin;
    unsigned char rejoin_try_times;
    unsigned char rf_power;
} NWK_PARAM_T;

///< zigbee module gpio (uart command id: 0x36 0x37 0x38)
///< for low level output, use GPIO_MODE_OUTPUT_OD_PULL_UP   and GPIO_DOUT_LOW
///< for high level output, use GPIO_MODE_OUTPUT_OD_PULL_DOWN and GPIO_DOUT_HIGH
typedef struct MCU_CONFIG_GPIO {
    unsigned char port_num;
    unsigned char pin_num;
    unsigned char gpio_mode;        // GPIO_MODE_INPUT_HIGH_IMPEDANCE = 0,      ///< input mode: high impedance
                                    // GPIO_MODE_INPUT_PULL,                    ///< input mode: pull(up or down)
                                    // GPIO_MODE_OUTPUT_PP,                     ///< output mode: Push-pull
                                    // GPIO_MODE_OUTPUT_OD,                     ///< output mode: Open drain
                                    // GPIO_MODE_OUTPUT_OD_PULL_UP,
                                    // GPIO_MODE_OUTPUT_OD_PULL_DOWN,
    unsigned char gpio_level;       // for input:
                                    // GPIO_LEVEL_LOW = 0,                      ///< Drop edge triggers interrupt
                                    // GPIO_LEVEL_HIGH,                         ///< Rising edge triggers interrupt
                                    // GPIO_LEVEL_ALL,                          ///< Rising and Drop edge triggers interrupt
                                    // for output:
                                    // GPIO_DOUT_LOW = 0,                       ///< output low or input pull down
                                    // GPIO_DOUT_HIGH
} MCU_CONFIG_GPIO_T;

typedef struct MCU_READ_GPIO {
    unsigned char port_num;
    unsigned char pin_num;
} MCU_READ_GPIO_T;

typedef struct MCU_WRITE_GPIO {
    unsigned char port_num;
    unsigned char pin_num;
    unsigned char gpio_level;
} MCU_WRITE_GPIO_T;

///< weather/city info (uart command id: 0x3b)
typedef struct WEATHER_INFO {
    unsigned char version;                  // 0x11
    unsigned char addr_type;
    unsigned char forecast_flag;            // 0x12
    unsigned char forecast_days;
    unsigned char real_time_weather_flag;   // 0x13
    unsigned char real_time_enable;
    unsigned char *weather_detail;
    unsigned char weather_detail_len;
} WEATHER_INFO_T;

typedef struct CITY_INFO {
    unsigned char version;                  // 0x11
    unsigned char addr_type;
    unsigned char *city_detail;
    unsigned char city_detail_len;
} CITY_INFO_T;

///< standard command group sending (uart command id: 0x42)
typedef struct MCU_SEND_CMD_GROUP {
    unsigned short group_id;
    unsigned short cluster_id;
    unsigned char command_id;
    unsigned char *payload;
    unsigned char payload_len;
} MCU_SEND_CMD_GROUP_T;

#ifdef __cplusplus
}
#endif

#endif
/* -END OF FILE-  */
