/***********************************************************
 * @file     mcu_api.h
 * @brief    define:            debug print macro, buffer macro, command related define.
 *           function declare:  uart service funtion, uart sending function, mcu wakeup zigbee module function, 
 *                              frame receive handle function, frame sending function.
 * @version  3.3.5
 * @date     2026.04.08
 * @copyright Copyright (c) tuya.inc 2024
 **********************************************************/

#ifndef __MCU_API_H_
#define __MCU_API_H_

#include "mcu_sdk_types.h"
#include "hooch_setting.h"   /* 统一天气码 HOOCH_PROTOCOL_SettingWeatherCode_t */

#ifdef __cplusplus
extern "C"
{
#endif

 /***********************************************************
 * Macro Definitions <USER MUST NOT CHANGE>
 **********************************************************/
///< <USER MUST NOT CHANGE>
#define ROUTER_DEVICE                           1               // router device;
#define SLEEP_END_DEVICE                        2               // sleep end device; uniquely support cmd: 0x2b
#define SCENE_SWITCH_DEVICE                     3               // scene switch device;uniquely support cmd: 0x0a 0x41 0x42 0x43    uniquely not support cmd : 0x25 0x26 0x2a

#define LOW_LEVEL_WAKE_UP                       1               // mcu Low-level wake up zigbee module;
#define LOW_PULSE_WAKE_UP                       2               // mcu Low-level pulse wake up zigbee module;
///< END <USER MUST NOT CHANGE>

 /***********************************************************
 * Macro Definitions <USER MUST CHECK AND CHANGE>
 * 
 * MCU_VER: 
 * please user set the 'MCU_VER' to your MCU firmware version;
 * 

 * 
 * DEVICE_TYPE:
 * please user please select ROUTER_DEVICE, SLEEP_END_DEVICE 
 * or SCENE_SWITCH_DEVICE;
 * 
 * MCU_WAKEUP_MODULE_METHOD:
 * please user select LOW_LEVEL_WAKE_UP or LOW_PULSE_WAKE_UP;
 **********************************************************/
///< <USER MUST CHECK AND CHANGE>
#define MCU_VER                                 "1.0.0"             // MAX 3.3.15   BIT 7~0   XX.XX.XXXX
#define DEVICE_TYPE                             SCENE_SWITCH_DEVICE  // this product is a zigbee scene switch panel (supports cmd: 0x0a 0x41 0x42 0x43)
#if (DEVICE_TYPE == SLEEP_END_DEVICE)
#define MCU_WAKEUP_MODULE_METHOD                LOW_LEVEL_WAKE_UP   // please select LOW_LEVEL_WAKE_UP or LOW_PULSE_WAKE_UP
#endif
///< END <USER MUST CHECK AND CHANGE>

/***********************************************************
 * Macro Definitions <USER OPTIONAL CHANGE>
 * 
 * user can change the value according to your needs.
 * 
 * SUPPORT_RECEIVE_BROADCAST_DATA: <default: 1>
 * please user set to 1 if your MCU can recognize the received 
 * broadcast msg, otherwise set to 0;
 *  
 * ZG_WAIT_MCU_TIME_DEFAULT: <default: 10ms>
 * zigbee module waits mcu time after zigbee module starting 
 * wakeuping mcu;(uart command id: 0x2b)
 * 
 * MCU_WAIT_ZG_TIME_DEFAULT:
 * mcu waits zigbee module time after mcu starting wakeuping
 * zigbee module;
 * 
 * OTA_PACKET_SIZE: <default: 0x30>
 * OTA single packet payload length (uart command id: 0x0c 0x0d 0x0e)
 * 
 * MCU_SDK_DEBUG: <default: 0>
 * user needs to implement their own printing function and replace 
 * "USER_PRINT_FUNC(fmt, ##__VA_ARGS__)"
 **********************************************************/
///< <USER OPTIONAL CHANGE>
#define SUPPORT_RECEIVE_BROADCAST_DATA          1               // 0: not support; 1: support
#if (DEVICE_TYPE == SLEEP_END_DEVICE)
#define ZG_WAIT_MCU_TIME_DEFAULT                10              // unit: ms
#define MCU_WAIT_ZG_TIME_DEFAULT                10              // unit: ms
#endif
#define UART_RX_BUF_LEN_LMT                     256
#define UART_TX_BUF_LEN_LMT                     256
#define UART_QUEUE_BUF_LEN_LMT                  512
#define OTA_PACKET_SIZE                         0x30            // max: 0x30

#ifndef MCU_SDK_DEBUG
#define MCU_SDK_DEBUG                           0
#endif

#if MCU_SDK_DEBUG
#define PRINT_DEBUG(fmt, ...)                   USER_PRINT_FUNC(fmt, ##__VA_ARGS__)
#else
#define PRINT_DEBUG(fmt, ...)
#endif

/* Legal detection of macro definitions */
#if ((DEVICE_TYPE != ROUTER_DEVICE)&&(DEVICE_TYPE != SLEEP_END_DEVICE)&&(DEVICE_TYPE != SCENE_SWITCH_DEVICE))
#error  "DEVICE_TYPE ERR, PLEASE CHOOSE ROUTER_DEVICE OR SLEEP_END_DEVICE OR SCENE_SWITCH_DEVICE"
#endif
#if (DEVICE_TYPE == SLEEP_END_DEVICE)&&(MCU_WAKEUP_MODULE_METHOD != LOW_LEVEL_WAKE_UP)&&(MCU_WAKEUP_MODULE_METHOD != LOW_PULSE_WAKE_UP)
#error  "MCU_WAKEUP_MODULE_METHOD ERR, PLEASE CHOOSE LOW_LEVEL_WAKE_UP OR LOW_PULSE_WAKE_UP"
#endif

/***********************************************************
 * Typedef Definitions
 **********************************************************/


/***********************************************************
 * Function Declarations
 **********************************************************/
/**
 * @brief  store the rx uart data (byte)
 * @param[in] value
 * @return none
 */
void uart_servive_rx_store(unsigned char value);

/**
 * @brief  parse frame
 * @param none
 * @return none
 */
void uart_service_parse(void);

/**
 * @brief  uart send uart protocol frame
 * @param[in] payload_len
 * @return none
 */
void uart_send_frame(unsigned short payload_len);

/**
 * @brief  uart send bytes
 * @param[in] data
 * @param[in] data_len
 * @return none
 */
void uart_send_bytes(unsigned char *data, unsigned short data_len);

/**
 * @brief  uart send a byte
 * @param[in] value
 * @return none
 */
void uart_send_byte(unsigned char value);

#if (DEVICE_TYPE == SLEEP_END_DEVICE)
/**
 * @brief  mcu wakeup zigbee module and send frame by level method
 * @param[in] data
 * @param[in] data_len
 * @return none
 */
void mcu_wakeup_zg_level_method(unsigned char *data, unsigned short data_len);

/**
 * @brief  mcu wakeup zigbee module and send frame by pulse method
 * @param[in] data
 * @param[in] data_len
 * @return none
 */
void mcu_wakeup_zg_pulse_method(unsigned char *data, unsigned short data_len);
#endif

/**
 * @brief  when mcu receive the factory recovery notify from zigbee module (uart command id 0x00), this function will be called.
 *         you can execute the factory recovery operation here.
 * @param none
 * @return none
 */
void mcu_recv_factory_recovery_cb(void);

/**
 * @brief  when mcu receive the zigbee network status notify (uart command id 0x02), this function will be called.
 *         you can handle the zigbee module network status here.
 * @param[in] nwk_status: ZG_NWK_STATUS_NOT_JOIN
 *                        ZG_NWK_STATUS_JOINED
 *                        ZG_NWK_STATUS_ERROR
 *                        ZG_NWK_STATUS_JOINING
 * @return none
 */
void mcu_recv_zg_nwk_status_notify_cb(unsigned char nwk_status);

/**
 * @brief  when mcu send dp msg to zigbee module, this function will be called.
 *         you can make some handle after sending dp msg.
 * @param[in] ret: SUCCESS
 *                 ERROR
 * @param[in] dp_id
 * @param[in] dp_type
 * @param[in] dp_len
 * @param[in] dp_data
 * @return none
 */
void mcu_send_dp_msg_cb(unsigned char ret, unsigned char dp_id, unsigned char dp_type, unsigned short dp_len, unsigned char *dp_data);

/**
 * @brief  when mcu receive zigbee module fireware version (uart command id 0x07), this function will be called.
 *         you can handle the version here.
 * @param[in] zg_sw_ver
 * @return none
 */
void mcu_recv_zg_sw_ver_cb(unsigned char zg_sw_ver);

/**
 * @brief  when mcu receive zigbee module authorization information (uart command id 0x07), this function will be called.
 *         you can handle the authorization information here.
 * @param[in] zg_auth
 * @return none
 */
void mcu_recv_zg_auth_cb(unsigned char zg_auth);

/**
 * @brief  when mcu receive zigbee module mac address (uart command id 0x07), this function will be called.
 *         you can handle the mac address here.
 * @param[in] mac_addr: 8 bytes, high byte first
 * @return none
 */
void mcu_recv_zg_mac_addr_cb(unsigned char *mac_addr);

/**
 * @brief  when mcu receive zigbee module rf test result (uart command id 0x08), this function will be called.
 *         you can handle the result here.
 * @param[in] result: SUCCESS
 *                    ERROR
 * @param[in] rssi: 0x00 ~ 0x64
 * @return none
 */
void mcu_recv_rf_test_result_cb(unsigned char result, unsigned char rssi);

/**
 * @brief  when mcu receive OTA notify and finish checking (uart command id 0x0c), this function will be called.
 *         1. you should start to request OTA data here.
 * @param[in] check_result: TRUE
 *                          FALSE
 * @return none
 */
void mcu_recv_ota_notify_cb(unsigned char check_result);

/**
 * @brief  when mcu receive OTA data (uart command id 0x0d), this function will be called.
 *         1. you should handle the OTA data here, like write the data to flash.
 *         2. for non-last packet,
 *            if you want mcu requests the next OTA packet after receving the current OTA packet automatically,
 *            you can call 'mcu_tx_request_ota_data()' herer at least 50ms later.
 * @param[in] last_packet_flag: TRUE is the last packet
 * @param[in] fw_offset
 * @param[in] data
 * @param[in] data_len
 * @return none
 */
void mcu_recv_ota_data_cb(unsigned char last_packet_flag, unsigned int fw_offset, unsigned char *data, unsigned char data_len);

/**
 * @brief  when mcu receive the zigbee network status response (uart command id 0x20), this function will be called.
 *         you can handle the zigbee module network status here.
 * @param[in] nwk_status: ZG_NWK_STATUS_NOT_JOIN
 *                        ZG_NWK_STATUS_JOINED
 *                        ZG_NWK_STATUS_ERROR
 *                        ZG_NWK_STATUS_JOINING
 * @return none
 */
void mcu_recv_zg_nwk_status_response_cb(unsigned char nwk_status);

/**
 * @brief  when mcu receive double dongle test notify (uart command id 0x21), this function will be called.
 * @param none
 * @return none
 */
void mcu_recv_double_dongle_test_notify_cb(void);

/**
 * @brief  when mcu receive double dongle test data (uart command id 0x21), this function will be called.
 *         1. you should handle the data according to its json format.
 *         2. if you want to report the test result, you can call 'mcu_tx_double_dongle_test_report_data()' here.
 * @param[in] test_data
 * @param[in] test_data_len
 * @return none
 */
void mcu_recv_double_dongle_test_data_cb(unsigned char *test_data, unsigned short test_data_len);

/**
 * @brief  when mcu receive time sync result (uart command id 0x24), this function will be called.
 *         you can handle the timestamp or canlendar here.
 * @param[in] std_timestamp: 4 bytes
 * @param[in] calendar: please refer to 'TIME_SYNC_CALENDAR_T'
 * @return none
 */
void mcu_recv_time_sync_cb(unsigned char *std_timestamp, TIME_SYNC_CALENDAR_T *calendar);

/**
 * @brief  when mcu receive gateway network status (uart command id 0x25), this function will be called.
 *         you can handle the gateway network status here.
 * @param[in] nwk_status: GW_NWK_STATUS_OFFLINE
 *                        GW_NWK_STATUS_ONLINE
 *                        GW_NWK_STATUS_RESPOND_TIMEOUT
 * @return none
 */
void mcu_recv_gw_nwk_status_cb(unsigned char nwk_status);

/**
 * @brief  when mcu receive beacon test notify from zigbee module (uart command id 0x29), this function will be called.
 *         you can execute your beacon test here.
 * @param none
 * @return test result: SUCCESS
 *                      ERROR
 */
unsigned char mcu_recv_beacon_notify_cb(void);

/**
 * @brief  when mcu receive zigbee gpio config result (uart command id 0x36), this function will be called.
 *         you can handle the result here.
 * @param[in] port
 * @param[in] pin
 * @param[in] result: 0x00 config success
 *                    0x01 config error
 * @return none
 */
void mcu_recv_gpio_config_result_cb(unsigned char port, unsigned char pin, unsigned char result);

/**
 * @brief  when mcu receive zigbee gpio read result (uart command id 0x37), this function will be called.
 *         you can handle the result here.
 * @param[in] port
 * @param[in] pin
 * @param[in] level: 0x00 low level
 *                   0x01 high level
 *                   0xff gpio not init yet
 * @return none
 */
void mcu_recv_gpio_read_result_cb(unsigned char port, unsigned char pin, unsigned char level);

/**
 * @brief  when mcu receive zigbee gpio write result (uart command id 0x38), this function will be called.
 *         you can handle the result here.
 * @param[in] port
 * @param[in] pin
 * @param[in] result: 0x00 write success
 *                    0x01 write error
 * @return none
 */
void mcu_recv_gpio_write_result_cb(unsigned char port, unsigned char pin, unsigned char result);

/**
 * @brief  when mcu receive zigbee gpio irq (uart command id 0x39), this function will be called.
 *         you can handle the irq information here.
 * @param[in] port
 * @param[in] pin
 * @param[in] level: 0x00 low level
 *                   0x01 high level
 * @return none
 */
void mcu_recv_gpio_irq_cb(unsigned char port, unsigned char pin, unsigned char level);

/**
 * @brief  when mcu receive weather information response (uart command id 0x3b), this function will be called.
 *         you can handle the weather information here.
 * @param[in] weather_info: please refer to 'WEATHER_INFO_T' and protocol document
 * @return none
 */
void mcu_recv_weather_response_cb(WEATHER_INFO_T *weather_info);

/**
 * @brief  when mcu receive city information response (uart command id 0x3b), this function will be called.
 *         you can handle the city information here.
 * @param[in] city_info: please refer to 'CITY_INFO_T' and protocol document
 * @return none
 */
void mcu_recv_city_response_cb(CITY_INFO_T *city_info);

/* 0x3b 天气响应经内容识别后的细分回调: SDK 负责解析, 用户直接读结构体即可 */
void mcu_recv_weather_forecast_cb(const TUYA_WEATHER_FORECAST_T *forecast);  /* 预报天气(温度+6天码) */
void mcu_recv_weather_wind_cb(const TUYA_WEATHER_WIND_T *wind);              /* 风向 */
void mcu_recv_weather_city_cb(const TUYA_WEATHER_TEXT_T *city);              /* 城市 */
void mcu_recv_weather_area_cb(const TUYA_WEATHER_TEXT_T *area);              /* 区县 */

/* 涂鸦天气码(conditionNum 原值) → hooch 统一天气码 (HOOCH_PROTOCOL_SettingWeatherCode_t) */
HOOCH_PROTOCOL_SettingWeatherCode_t mcu_weather_code_to_hooch(unsigned char condition_num);

#if (DEVICE_TYPE == SCENE_SWITCH_DEVICE)
/**
 * @brief  when mcu receive scene config (uart command id 0x41), this function will be called.
 *         you can handle the scene information here.
 * @param[in] key_id
 * @param[in] group_id: this group id should be stored for uart command id 0x42 0x43
 * @param[in] scene_id
 * @return handle result: SUCCESS
 *                        ERROR
 */
unsigned char mcu_recv_scene_config_cb(unsigned char key_id, unsigned short group_id, unsigned char scene_id);
#endif

/**
 * @brief  mcu let zigbee module reset
 *         cmd: 0x03
 * @param none
 * @return none
 */
void mcu_tx_let_zigbee_reset(void);
/**
 * @brief  mcu let zigbee module start join
 *         cmd: 0x03
 * @param none
 * @return none
 */
void mcu_tx_let_zigbee_start_join(void);


/**
 * @brief  mcu report dp linkage
 *         cmd: 0x06
 * @param[in] dp_id
 * @param[in] dp_type
 * @param[in] dp_len
 * @param[in] dp_value
 * @return none
 */
void mcu_tx_report_dp_linkage(unsigned char dp_id, unsigned char dp_type, unsigned short dp_len, unsigned char *dp_value);

/**
 * @brief  mcu request zigbee module info
 *         cmd: 0x07
 * @param[in] sw_ver_req
 * @param[in] auth_req
 * @param[in] mac_addr_req
 * @return none
 */
void mcu_tx_request_zg_info(unsigned char sw_ver_req, unsigned char auth_req, unsigned char mac_addr_req);

/**
 * @brief  mcu start zigbee RF test
 *         cmd: 0x08
 * @param[in] channel
 * @return none
 */
void mcu_tx_start_zigbee_rf_test(unsigned char channel);

#if (DEVICE_TYPE == SCENE_SWITCH_DEVICE)
/**
 * @brief  mcu report keyID
 *         cmd: 0x0a
 * @param[in] keyID
 * @return none
 */
void mcu_tx_scene_key_id(unsigned char keyID);
#endif

/**
 * @brief  mcu send mcu fw version to zigbee
 *         cmd: 0x0b
 * @param none
 * @return none
 */
void mcu_tx_fw_ver(void);

/**
 * @brief  mcu request OTA data
 *         cmd: 0x0d
 * @param none
 * @return 0 is OTAing; 1 is OTA finished; 2 is OTA notify failed
 */
unsigned char mcu_tx_request_ota_data(void);

/**
 * @brief  mcu send OTA result to zigbee module
 *         cmd: 0x0e
 * @param[in] status: OTA result
 *                    if send 0x00, the OTA process will end with success
 *                    however, if send 0x01, the OTA process won't end until OTA timeout
 * @return none
 */
void mcu_tx_ota_result(unsigned char status);

/**
 * @brief  mcu check zigbee module network status
 *         cmd: 0x20
 * @param none
 * @return none
 */
void mcu_tx_check_zg_nwk_status(void);

/**
 * @brief  mcu report 'double dongle test data'
 *         cmd: 0x22
 * @param[in] report_data
 * @param[in] report_data_len
 * @return none
 */
void mcu_tx_double_dongle_test_report_data(unsigned char *report_data, unsigned short report_data_len);

/**
 * @brief  mcu request time sync
 *         cmd: 0x24
 * @param none
 * @return none
 */
void mcu_tx_request_time_sync(void);

/**
 * @brief  mcu check gw network status
 *         cmd: 0x25
 * @param none
 * @return none
 */
void mcu_tx_check_gw_nwk_status(void);

/**
 * @brief  mcu set zigbee nwk parameter
 *         cmd: 0x26
 * @param[in] nwk_param: if NULL, set default nwk param
 *                       if sleep end device, you should set all nwk param
 *                       if not sleep end device, you just need to set 'rf_power' of the structure 
 * @return none
 */
void mcu_tx_set_zg_nwk_param(NWK_PARAM_T *nwk_param);

/**
 * @brief  mcu send dp broadcast
 *         cmd: 0x27
 * @param[in] dp_id
 * @param[in] dp_type
 * @param[in] dp_len
 * @param[in] dp_value
 * @return none
 */
void mcu_tx_send_dp_broadcast(unsigned char dp_id, unsigned char dp_type, unsigned short dp_len, unsigned char *dp_value);

#if (DEVICE_TYPE == SLEEP_END_DEVICE)
/**
 * @brief  mcu set zigbee wait mcu time
 *         cmd: 0x2b
 * @param[in] time_ms
 * @return none
 */
void mcu_tx_set_zg_wait_mcu_time(unsigned short time_ms);
#endif

/**
 * @brief  mcu report dp without linkage
 *         cmd: 0x2c
 * @param[in] dp_id
 * @param[in] dp_type
 * @param[in] dp_len
 * @param[in] dp_value
 * @return none
 */
void mcu_tx_report_dp_without_linkage(unsigned char dp_id, unsigned char dp_type, unsigned short dp_len, unsigned char *dp_value);

/**
 * @brief  mcu config zg gpio
 *         cmd: 0x36
 * @param[in] gpio_num
 * @param[in] gpio_param
 * @return none
 */
void mcu_tx_config_zg_gpio(unsigned char gpio_num, MCU_CONFIG_GPIO_T *gpio_param);

/**
 * @brief  mcu read zg gpio
 *         cmd: 0x37
 * @param[in] gpio_num
 * @param[in] gpio_param
 * @return none
 */
void mcu_tx_read_zg_gpio(unsigned char gpio_num, MCU_READ_GPIO_T *gpio_param);

/**
 * @brief  mcu write zg gpio
 *         cmd: 0x38
 * @param[in] gpio_num
 * @param[in] gpio_param
 * @return none
 */
void mcu_tx_write_zg_gpio(unsigned char gpio_num, MCU_WRITE_GPIO_T *gpio_param);

/**
 * @brief  mcu request weather data
 *         cmd: 0x3a
 * @param[in] addr_type
 * @param[in] weather_id_list
 * @param[in] weather_id_num
 * @param[in] forecast_days
 * @param[in] real_time_enable
 * @return none
 */
void mcu_tx_weather_request(unsigned char addr_type, unsigned char *weather_id_list, unsigned char weather_id_num, unsigned char forecast_days, unsigned char real_time_enable);

/**
 * @brief  mcu request city data
 *         cmd: 0x3a
 * @param[in] addr_type
 * @return none
 */
void mcu_tx_city_request(unsigned char addr_type);

#if (DEVICE_TYPE == SCENE_SWITCH_DEVICE)
/**
 * @brief  mcu send standard command to group
 *         cmd: 0x42
 * @param[in] standard_cmd
 * @return none
 */
void mcu_tx_send_cmd_group(MCU_SEND_CMD_GROUP_T *standard_cmd);

/**
 * @brief  mcu send dp to group
 *         cmd: 0x43
 * @param[in] group_id
 * @param[in] dp_id
 * @param[in] dp_type
 * @param[in] dp_len
 * @param[in] dp_data
 * @return none
 */
void mcu_tx_send_dp_group(unsigned short group_id, unsigned char dp_id, unsigned char dp_type, unsigned short dp_len, unsigned char *dp_data);
#endif

#ifdef __cplusplus
}
#endif

#endif
/* -END OF FILE-  */
