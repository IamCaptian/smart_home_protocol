/***********************************************************
 * @file     protocol.c
 * @brief    define:            command related define.
 *           function declare:  frame receive handle funtion, DP msg recv & send function.
 *           function define:   DP recv & send function, frame receive handle funtion, frame sending function,
 *                              DP msg recv & send function.
 * @version  3.3.5
 * @date     2026.04.08
 * @copyright Copyright (c) tuya.inc 2024
 **********************************************************/

#include "tuya_protocol_uart.h"
#include "zigbee.h"
#include "hooch_protocol_common.h"


/***********************************************************
 * Macro Definitions
 **********************************************************/

#define DP_TOTAL_NUM                        (sizeof(download_cmd) / sizeof(download_cmd[0]))

/***********************************************************
 * Typedef Definitions
 **********************************************************/

typedef struct {
    unsigned char dp_id;
    unsigned char dp_type;
} DOWNLOAD_CMD_S;

typedef struct {
    unsigned short frame_hdr;
    unsigned char frame_ver;
    unsigned short frame_seq;
    unsigned char frame_cmd;
    unsigned short frame_payload_len;
    unsigned char *frame_payload;
} UART_FRAME_T;

typedef struct {
    unsigned char ota_pid[8];               // mcu PID
    unsigned char ota_fw_ver;               // new mcu fw version
    unsigned int ota_fw_size;               // mcu fw total size
    unsigned int ota_current_offset;        // mcu fw current offset
    unsigned int ota_expect_checksum;       // mcu fw expective checksum
} OTA_FW_INFO_T;

/***********************************************************
 * Variable Declarations
 **********************************************************/


//< USER_CHECK_MSG
/*----------------------------------------------------------
 *                        DP list
 * 1. please check the ID and type of each DP.
 *--------------------------------------------------------*/
const DOWNLOAD_CMD_S download_cmd[] =
{
  {DPID_SCENE_1, DP_TYPE_ENUM},
  {DPID_SCENE_2, DP_TYPE_ENUM},
  {DPID_SCENE_3, DP_TYPE_ENUM},
  {DPID_SCENE_4, DP_TYPE_ENUM},
  {DPID_SCENE_5, DP_TYPE_ENUM},
  {DPID_SCENE_6, DP_TYPE_ENUM},
  {DPID_SCENE_7, DP_TYPE_ENUM},
  {DPID_SCENE_8, DP_TYPE_ENUM},
  {DPID_SWITCH_1, DP_TYPE_BOOL},
  {DPID_SWITCH_2, DP_TYPE_BOOL},
  {DPID_SWITCH_3, DP_TYPE_BOOL},
  {DPID_SWITCH_4, DP_TYPE_BOOL},
  {DPID_SWITCH_5, DP_TYPE_BOOL},
  {DPID_SWITCH_6, DP_TYPE_BOOL},
  {DPID_FAN_DIRECTION, DP_TYPE_ENUM},
  {DPID_FAN_SPEED_ENUM, DP_TYPE_ENUM},
  {DPID_MODE, DP_TYPE_ENUM},
  {DPID_TEMP_SET, DP_TYPE_VALUE},
  {DPID_SWITCH, DP_TYPE_BOOL},
  {DPID_LOOP_MODE, DP_TYPE_ENUM},
  {DPID_SUPPLY_FAN_SPEED, DP_TYPE_ENUM},
  {DPID_EXHAUST_FAN_SPEED, DP_TYPE_ENUM},
  {DPID_FRESH_AIR_VALVE, DP_TYPE_BOOL},
  {DPID_PIR_STATE, DP_TYPE_ENUM},
  {DPID_FLOOR_SW, DP_TYPE_BOOL},
  {DPID_FLOOR_TEMP, DP_TYPE_VALUE},
  {DPID_HEAT_LIMIT, DP_TYPE_RAW},
  {DPID_AC_LIMIT, DP_TYPE_RAW},
  {DPID_AC_INFO, DP_TYPE_RAW},
  {DPID_FAN_INFO, DP_TYPE_RAW},
  {DPID_FLOOR_INFO, DP_TYPE_RAW},
  {DPID_SWITCH_7, DP_TYPE_BOOL},
  {DPID_SWITCH_8, DP_TYPE_BOOL},
  {DPID_KEY_MODE, DP_TYPE_RAW},
  {DPID_PAGE_MGR, DP_TYPE_RAW},
  {DPID_JOG_TIME, DP_TYPE_RAW},
  {DPID_BASE_SET, DP_TYPE_RAW},
  {DPID_ADV_SET, DP_TYPE_RAW},
  {DPID_THEME, DP_TYPE_ENUM},
  {DPID_KEY_NAME, DP_TYPE_RAW},
  {DPID_CHILD_LOCK, DP_TYPE_BOOL},
  {DPID_DISP_PARAM, DP_TYPE_RAW},
  {DPID_PAGE_SYNC, DP_TYPE_ENUM},
  {DPID_MASTER_SW, DP_TYPE_BOOL},
  {DPID_FRESH_AIR_SPEED, DP_TYPE_ENUM},
};

//< USER_CHECK_MSG END

extern volatile unsigned char g_uart_tx_buf[UART_TX_BUF_LEN_LMT];
extern volatile unsigned char g_uart_rx_buf[UART_RX_BUF_LEN_LMT];

// OTA (uart command id: 0x0c, 0x0d, 0x0e)
static unsigned char sg_ota_data_request_lock = FALSE;      // in these situation, mcu is not allowed to request OTA data:
                                                            //   1. mcu doesn't receive OTA notify yet.
                                                            //   2. the result of OTA info checking is wrong.
                                                            //   3. mcu has requested OTA data, but hasn't received the OTA data response yet.
static unsigned char sg_native_pid[8] = {0};                // native stored pid
static OTA_FW_INFO_T sg_ota_fw_info = {0};

// time sync (uart command id: 0x24)
static unsigned char sg_timestamp[4] = {0};
static TIME_SYNC_CALENDAR_T sg_time_calendar = {0};
static NWK_PARAM_T sg_default_nwk_param = NWK_PARAM_DEFAULT_SET; // zigbee module network param (uart command id: 0x26)


#if (DEVICE_TYPE == SLEEP_END_DEVICE)
unsigned short g_zg_wait_mcu_time = ZG_WAIT_MCU_TIME_DEFAULT;    // zigbee module wait mcu time (uart command id: 0x2b)
unsigned short g_mcu_wait_zg_time = MCU_WAIT_ZG_TIME_DEFAULT;    // mcu wait zigbee module time
#if (MCU_WAKEUP_MODULE_METHOD == LOW_LEVEL_WAKE_UP)
const unsigned short g_wakeup_stay_time = 120;                   // mcu wakeup stay time must be less than 2 minutes
#endif
#endif

#if SUPPORT_RECEIVE_BROADCAST_DATA
unsigned short g_group_id_send_dp_msg = 0;                       // mcu receives group id (uart command id: 0x41 0x42 0x43)
#endif

/***********************************************************
 * Variable Definitions
 **********************************************************/

DP_SEND_TYPE_E g_dp_send_type = DP_SEND_TYPE_NOT_SEND;

/***********************************************************
 * Function Declarations
 **********************************************************/

/*----------------------------------------------------------
 *              frame receive handle function
 *--------------------------------------------------------*/

#if TUYA_LOG_ENABLE

static const char *tuya_get_cmd_name(unsigned char cmd_id)
{
    switch (cmd_id) {
        case 0x00: return "FACTORY_RECOVERY";
        case 0x01: return "PRODUCT_INFO";
        case 0x02: return "ZG_NWK_STATUS";
        case 0x03: return "RESET_OR_JOIN";
        case 0x04: return "DP_MSG";
        case 0x05: return "RESPOND_DP";
        case 0x06: return "REPORT_DP_LINKAGE";
        case 0x07: return "ZG_INFO";
        case 0x08: return "RF_TEST";
        case 0x0A: return "SCENE_INFO";
        case 0x0B: return "MCU_VER";
        case 0x0C: return "OTA_NOTIFY";
        case 0x0D: return "OTA_DATA";
        case 0x0E: return "OTA_RESULT";
        case 0x20: return "ZG_NWK_STATUS_REQUEST";
        case 0x21: return "DOUBLE_DONGLE_NOTIFY";
        case 0x22: return "DOUBLE_DONGLE_REPORT";
        case 0x24: return "TIME_SYNC";
        case 0x25: return "MCU_CHECK_GW_NWK";
        case 0x26: return "MCU_SET_ZG_NWK_PARAM";
        case 0x27: return "SEND_DP_BROADCAST";
        case 0x28: return "GW_CHECK_DP";
        case 0x29: return "BEACON_TEST";
        case 0x2A: return "RX_DP_GROUP";
        case 0x2B: return "ZG_WAIT_MCU_TIME";
        case 0x2C: return "REPORT_DP_NOT_LINKAGE";
        case 0x36: return "CONFIG_ZG_GPIO";
        case 0x37: return "READ_ZG_GPIO";
        case 0x38: return "WRITE_ZG_GPIO";
        case 0x39: return "ZG_GPIO_IRQ";
        case 0x3A: return "WEATHER_REQUEST";
        case 0x3B: return "WEATHER_RESPONSE";
        case 0x41: return "SCENE_CONFIG";
        case 0x42: return "SEND_CMD_GROUP";
        case 0x43: return "SEND_DP_GROUP";
        default:  return "UNKNOWN";
    }
}

static void tuya_log_payload_hex(unsigned char *data, unsigned short len)
{
    TUYA_LOG_INFO("[Tuya]   payload[%u]: ", len);
    for (unsigned short i = 0; i < len; i++) {
        TUYA_LOG_INFO("%02X ", data[i]);
    }
    TUYA_LOG_INFO("\r\n");
}

static const char *tuya_get_dp_type_name(unsigned char dp_type)
{
    switch (dp_type) {
        case DP_TYPE_RAW:   return "RAW";
        case DP_TYPE_BOOL:  return "BOOL";
        case DP_TYPE_VALUE: return "VALUE";
        case DP_TYPE_STRING:   return "STRING";
        case DP_TYPE_ENUM:  return "ENUM";
        case DP_TYPE_FAULT: return "FAULT";
        default:            return "?";
    }
}

static const char *tuya_get_dp_name(unsigned char dp_id)
{
    switch (dp_id) {
        case DPID_SCENE_1:           return "SCENE_1";
        case DPID_SCENE_2:           return "SCENE_2";
        case DPID_SCENE_3:           return "SCENE_3";
        case DPID_SCENE_4:           return "SCENE_4";
        case DPID_SCENE_5:           return "SCENE_5";
        case DPID_SCENE_6:           return "SCENE_6";
        case DPID_SCENE_7:           return "SCENE_7";
        case DPID_SCENE_8:           return "SCENE_8";
        case DPID_SWITCH_1:          return "SWITCH_1";
        case DPID_SWITCH_2:          return "SWITCH_2";
        case DPID_SWITCH_3:          return "SWITCH_3";
        case DPID_SWITCH_4:          return "SWITCH_4";
        case DPID_SWITCH_5:          return "SWITCH_5";
        case DPID_SWITCH_6:          return "SWITCH_6";
        case DPID_FAN_DIRECTION:     return "FAN_DIRECTION";
        case DPID_FAN_SPEED_ENUM:    return "FAN_SPEED_ENUM";
        case DPID_MODE:              return "MODE";
        case DPID_TEMP_SET:          return "TEMP_SET";
        case DPID_SWITCH:            return "SWITCH";
        case DPID_LOOP_MODE:         return "LOOP_MODE";
        case DPID_SUPPLY_FAN_SPEED:  return "SUPPLY_FAN_SPEED";
        case DPID_EXHAUST_FAN_SPEED: return "EXHAUST_FAN_SPEED";
        case DPID_FRESH_AIR_VALVE:   return "FRESH_AIR_VALVE";
        case DPID_PIR_STATE:         return "PIR_STATE";
        case DPID_FLOOR_SW:          return "FLOOR_SW";
        case DPID_FLOOR_TEMP:        return "FLOOR_TEMP";
        case DPID_HEAT_LIMIT:        return "HEAT_LIMIT";
        case DPID_AC_LIMIT:          return "AC_LIMIT";
        case DPID_AC_INFO:           return "AC_INFO";
        case DPID_FAN_INFO:          return "FAN_INFO";
        case DPID_FLOOR_INFO:        return "FLOOR_INFO";
        case DPID_SWITCH_7:          return "SWITCH_7";
        case DPID_SWITCH_8:          return "SWITCH_8";
        case DPID_KEY_MODE:          return "KEY_MODE";
        case DPID_PAGE_MGR:          return "PAGE_MGR";
        case DPID_JOG_TIME:          return "JOG_TIME";
        case DPID_BASE_SET:          return "BASE_SET";
        case DPID_ADV_SET:           return "ADV_SET";
        case DPID_THEME:             return "THEME";
        case DPID_KEY_NAME:          return "KEY_NAME";
        case DPID_CHILD_LOCK:        return "CHILD_LOCK";
        case DPID_DISP_PARAM:        return "DISP_PARAM";
        case DPID_PAGE_SYNC:         return "PAGE_SYNC";
        case DPID_MASTER_SW:         return "MASTER_SW";
        case DPID_FRESH_AIR_SPEED:   return "FRESH_AIR_SPEED";
        default:                     return "UNKNOWN";
    }
}
#endif

/**
 * @brief  mcu rx factory recovery notify
 *         cmd: 0x00
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_factory_recovery_notify(UART_FRAME_T *uart_frame);

/**
 * @brief  mcu rx product info request
 *         cmd: 0x01
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_product_info_request(UART_FRAME_T *uart_frame);

/**
 * @brief  mcu rx zigbee network status (mcu need to respond)
 *         cmd: 0x02
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_zg_nwk_status_notify(UART_FRAME_T *uart_frame);

/**
 * @brief  mcu rx dp msg
 *         this function is different frome the group dp msg (command id: 0x2a)
 *         cmd: 0x04
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_common_dp_msg(UART_FRAME_T *uart_frame);

/**
 * @brief  mcu rx zigbee module info
 *         cmd: 0x07
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_zg_info(UART_FRAME_T *uart_frame);

/**
 * @brief  mcu rx RF test result
 *         cmd: 0x08
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_rf_test_result(UART_FRAME_T *uart_frame);

/**
 * @brief  mcu rx mcu version request
 *         cmd: 0x0b
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_mcu_version_request(UART_FRAME_T *uart_frame);
/**
 * @brief  send mcu fw version to zigbee module
 * @param[in] uart_frame_seq
 * @return none
 */
static void __send_mcu_fw_ver(unsigned short *uart_frame_seq);
/**
 * @brief  get native mcu fw version
 * @param none
 * @return  current mcu fw version
 */
static unsigned char __get_native_mcu_fw_ver(void);

/**
 * @brief  mcu rx zigbee ota notify
 *         cmd: 0x0c
 * @param[in] uart_frame 
 * @return none
 */
static void mcu_rx_ota_notify(UART_FRAME_T *uart_frame);
/**
 * @brief  init native pid
 * @param none
 * @return  none
 */
static void __init_native_pid(void);

/**
 * @brief  mcu rx OTA data
 *         cmd: 0x0d
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_ota_data(UART_FRAME_T *uart_frame);
/**
 * @brief  check the OTA data
 * @param[in] rx_ota_packet
 * @param[out] rx_ota_offset: the offset of current packet in the whole OTA data
 * @return check result
 */
static unsigned char __check_ota_data(unsigned char *rx_ota_packet, unsigned int *rx_ota_offset);

/**
 * @brief  mcu rx check zg network status response
 *         cmd: 0x20
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_check_zg_nwk_status_response(UART_FRAME_T *uart_frame);

/**
 * @brief  mcu rx double dongle test notify
 *         cmd: 0x21
 * @param none
 * @return none
 */
static void mcu_rx_double_dongle_test_notify(void);
/**
 * @brief  mcu rx double dongle test data
 *         cmd: 0x21
 * @param[in] test_info
 * @param[in] test_info_len
 * @return none
 */
static void mcu_rx_double_dongle_test_data(unsigned char *test_info, unsigned short test_info_len);

/**
 * @brief  mcu rx timestamp
 *         cmd: 0x24
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_time_sync_timestamp(UART_FRAME_T *uart_frame);
/**
 * @brief  calculate timestamp to time format (week and time)
 * @param none
 * @return none
 */
static void __convert_timestamp_to_calendar(void);
/**
 * @brief  calculate week and date
 * @param[in] timestamp
 * @param[in] calendar: use a globel variable now 
 * @return none
 */
static void __calculate_week_and_date(unsigned int timestamp, TIME_SYNC_CALENDAR_T *calendar);
/**
 * @brief  check whether the year is leap year
 * @param none
 * @return 1 is leap year
 */
static unsigned char __is_leap_year(unsigned int year);

/**
 * @brief  mcu rx gw network status
 *         cmd: 0x25
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_gw_nwk_status(UART_FRAME_T *uart_frame);

/**
 * @brief  mcu rx gw check dp
 *         cmd: 0x28
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_gw_check_dp(UART_FRAME_T *uart_frame);

/**
 * @brief  mcu rx beacon test notify
 *         cmd: 0x29
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_beacon_test_notify(UART_FRAME_T *uart_frame);

#if SUPPORT_RECEIVE_BROADCAST_DATA
/**
 * @brief  mcu rx group dp msg
 *         cmd: 0x2a
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_group_dp_msg(UART_FRAME_T *uart_frame);
#endif

/**
 * @brief  mcu rx config gpio result
 *         cmd: 0x36
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_config_gpio_result(UART_FRAME_T *uart_frame);

/**
 * @brief  mcu rx read gpio result
 *         cmd: 0x37
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_read_gpio_result(UART_FRAME_T *uart_frame);

/**
 * @brief  mcu rx write gpio result
 *         cmd: 0x38
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_write_gpio_result(UART_FRAME_T *uart_frame);

/**
 * @brief  mcu rx input gpio irq
 *         cmd: 0x39
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_gpio_irq(UART_FRAME_T *uart_frame);

/**
 * @brief  mcu rx weather data
 *         cmd: 0x3b
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_weather_data(UART_FRAME_T *uart_frame);

/**
 * @brief  mcu rx city data
 *         cmd: 0x3b
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_city_data(UART_FRAME_T *uart_frame);

#if (DEVICE_TYPE == SCENE_SWITCH_DEVICE)
/**
 * @brief  mcu rx scene config
 *         cmd: 0x41
 * @param[in] uart_frame
 * @return none
 */
static void mcu_rx_scene_config(UART_FRAME_T *uart_frame);
#endif

/*----------------------------------------------------------
 *                    dp sending function
 *--------------------------------------------------------*/

/**
 * @brief  send dp msg in specific way
 * @param[in] payload_len
 * @return none
 */
static void __dp_send_type_handle(unsigned short payload_len);

/*----------------------------------------------------------
 *              dp msg received handle function
 *--------------------------------------------------------*/

/**
 * @brief  dp recv handle
 * @param[in] dp_id
 * @param[in] value: dp data
 * @param[in] length: dp data len
 * @return operate result
 */
static unsigned char dp_msg_handle(unsigned char dp_id, const unsigned char *value, unsigned short length);

/*----------------------------------------------------------
                    dp msg report function
 *--------------------------------------------------------*/

/**
 * @brief  report all dp data
 * @param none
 * @return none
 */
static void all_data_update(void);

/**
 * @brief  report specific dp data
 * @param[in] dp_id
 * @return none
 */
static void specific_dp_update(unsigned char dp_id);

/***********************************************************
 * Function Definitions
 **********************************************************/

/*----------------------------------------------------------
 *              frame receive handle function
 *--------------------------------------------------------*/

unsigned char frame_rx_handle(unsigned short cur_frame_start_offset)
{
    unsigned char ret = 0;

    ///< 1. get rx frame info
    unsigned short seq_num = g_uart_rx_buf[cur_frame_start_offset + FRAME_FIELD_SEQ_HIGH] * 0x100;
    seq_num += g_uart_rx_buf[cur_frame_start_offset + FRAME_FIELD_SEQ_LOW];
    unsigned char cmd_id = g_uart_rx_buf[cur_frame_start_offset + FRAME_FIELD_CMD_ID];
    unsigned short payload_len = g_uart_rx_buf[cur_frame_start_offset + FRAME_FIELD_PAYLOAD_LEN_HIGH] * 0x100;
    payload_len += g_uart_rx_buf[cur_frame_start_offset + FRAME_FIELD_PAYLOAD_LEN_LOW];
    unsigned char *payload = (unsigned char *)g_uart_rx_buf + cur_frame_start_offset + FRAME_FIELD_PAYLOAD_START;

    UART_FRAME_T uart_frame = {
        .frame_hdr = FRAME_VALUE_HDR,
        .frame_ver = FRAME_VALUE_PROT_VER,
        .frame_seq = seq_num,
        .frame_cmd = cmd_id,
        .frame_payload_len = payload_len,
        .frame_payload = payload,
    };

#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] RX cmd=0x%02X(%s) seq=%u len=%u\r\n",
            cmd_id, tuya_get_cmd_name(cmd_id), seq_num, payload_len);
    if (payload_len > 0) {
        tuya_log_payload_hex(payload, payload_len);
    }
#endif
hooch_cpu1_set_detected_protocol(1);
    ///< 2. handle rx frame
    switch (cmd_id) {
        case UART_CMD_FACTORY_RECOVERY: {
            mcu_rx_factory_recovery_notify(&uart_frame);
            break;
        }

        case UART_CMD_PRODUCT_INFO: {
            mcu_rx_product_info_request(&uart_frame);
            break;
        }

        case UART_CMD_ZG_NWK_STATUS_NOTIFY: {
            mcu_rx_zg_nwk_status_notify(&uart_frame);
            break;
        }

        case UART_CMD_RESET_OR_JOIN: {
            // zigbee module sends a frame with no payload to mcu,
            // but mcu doesn't need to handle it.
            break;
        }

        case UART_CMD_RX_DP: {
            mcu_rx_common_dp_msg(&uart_frame);
            break;
        }

        case UART_CMD_RESPOND_DP: {
            // zigbee module sends a frame with dp response result to mcu,
            // but mcu doesn't need to handle it.
            ret = uart_frame.frame_payload[0];
            break;
        }

        case UART_CMD_REPORT_DP_LINKAGE: {
            // zigbee module sends a frame with dp report result to mcu,
            // but mcu doesn't need to handle it.
            ret = uart_frame.frame_payload[0];
            break;
        }

        case UART_CMD_ZG_INFO: {
            mcu_rx_zg_info(&uart_frame);
            break;
        }

        case UART_CMD_RF_TEST: {
            mcu_rx_rf_test_result(&uart_frame);
            break;
        }

#if (DEVICE_TYPE == SCENE_SWITCH_DEVICE)
        case UART_CMD_SCENE_INFO: {
            // zigbee module sends a frame with key_id report result to mcu,
            // but mcu doesn't need to handle it.
            ret = uart_frame.frame_payload[0];
            break;
        }
#endif

        case UART_CMD_MCU_VER: {
            mcu_rx_mcu_version_request(&uart_frame);
            break;
        }

        case UART_CMD_OTA_NOTIFY: {
            mcu_rx_ota_notify(&uart_frame);
            break;
        }

        case UART_CMD_OTA_DATA: {
            mcu_rx_ota_data(&uart_frame);
            break;
        }

        case UART_CMD_OTA_RESULT: {
            // zigbee module sends a frame with 0x00 to mcu,
            // but mcu doesn't need to handle it.
            ret = uart_frame.frame_payload[0];
            break;
        }

        case UART_CMD_ZG_NWK_STATUS_REQUEST: {
            mcu_rx_check_zg_nwk_status_response(&uart_frame);
            break;
        }

        case UART_CMD_DOUBLE_DONGLE_DATA_NOTIFY: {
            if (0 == payload_len) {
                mcu_rx_double_dongle_test_notify();
            } else {
                mcu_rx_double_dongle_test_data(payload, payload_len);
            }
            break;
        }

        case UART_CMD_DOUBLE_DONGLE_REPORT: {
            // zigbee module sends a frame with double dongle test data report to gateway result to mcu,
            // but mcu doesn't need to handle it.
            ret = uart_frame.frame_payload[0];
            break;
        }

        case UART_CMD_TIME_SYNC: {
            mcu_rx_time_sync_timestamp(&uart_frame);
            break;
        }

        case UART_CMD_MCU_CHECK_GW_NWK_STAUTS: {
            mcu_rx_gw_nwk_status(&uart_frame);
        }

        case UART_CMD_MCU_SET_ZG_NWK_PARAM: {
            // zigbee module sends a frame with zigbee module setting result to mcu,
            // but mcu doesn't need to handle it.
            ret = uart_frame.frame_payload[0];
            break;
        }

        case UART_CMD_SEND_DP_BROADCAST: {
            // zigbee module sends a frame with dp sending in broadcast result to mcu,
            // but mcu doesn't need to handle it.
            ret = uart_frame.frame_payload[0];
            break;
        }

        case UART_CMD_GW_CHECK_DP: {
            mcu_rx_gw_check_dp(&uart_frame);
            break;
        }

        case UART_CMD_BEACON_TEST: {
            mcu_rx_beacon_test_notify(&uart_frame);
            break;
        }

#if SUPPORT_RECEIVE_BROADCAST_DATA
        case UART_CMD_RX_DP_GROUP: {
            mcu_rx_group_dp_msg(&uart_frame);
            break;
        }
#endif

#if (DEVICE_TYPE == SLEEP_END_DEVICE)
        case UART_CMD_ZG_WAIT_MCU_TIME: {
            // zigbee module sends a frame with wait time set result to mcu,
            // but mcu doesn't need to handle it.
            ret = uart_frame.frame_payload[0];
            break;
        }
#endif

        case UART_CMD_REPORT_DP_NOT_LINKAGE: {
            // zigbee module sends a frame with dp report result to mcu,
            // but mcu doesn't need to handle it.
            ret = uart_frame.frame_payload[0];
            break;
        }

        case UART_CMD_CONFIG_ZG_GPIO: {
            mcu_rx_config_gpio_result(&uart_frame);
            break;
        }

        case UART_CMD_READ_ZG_GPIO: {
            mcu_rx_read_gpio_result(&uart_frame);
            break;
        }

        case UART_CMD_WRITE_ZG_GPIO: {
            mcu_rx_write_gpio_result(&uart_frame);
            break;
        }

        case UART_CMD_ZG_GPIO_IRQ: {
            mcu_rx_gpio_irq(&uart_frame);
            break;
        }

        case UART_CMD_WEATHER_REQUEST: {
            // when zigbee module request weather/city info to gateway failed, zigbee module will send nak to mcu (uart command id 0x3a)
            // but mcu doesn't need to handle it.
            ret = uart_frame.frame_payload[0];
            break;
        }

        case UART_CMD_WEATHER_RESPONSE: {
            unsigned char weather_flag = uart_frame.frame_payload[2];
            if (0x12 == weather_flag) {
                mcu_rx_weather_data(&uart_frame);
            } else {
                mcu_rx_city_data(&uart_frame);
            }
            break;
        }

#if (DEVICE_TYPE == SCENE_SWITCH_DEVICE)
        case UART_CMD_SCENE_CONFIG: {
            mcu_rx_scene_config(&uart_frame);
            break;
        }

        case UART_CMD_SEND_CMD_GROUP: {
            // zigbee module sends a frame with standard command sending to group result to mcu,
            // but mcu doesn't need to handle it.
            ret = uart_frame.frame_payload[0];
            break;
        }

        case UART_CMD_SEND_DP_GROUP: {
            // zigbee module sends a frame with dp sending to group result to mcu,
            // but mcu doesn't need to handle it.
            ret = uart_frame.frame_payload[0];
            break;
        }
#endif

        default: {
            return 0;
        }
    }

    return ret;
}

static void mcu_rx_factory_recovery_notify(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }
    
    ///< 1. send frame
    unsigned short payload_len = 0;
    uart_framing_fill_payload_byte(&payload_len, 0x01);
    uart_framing_fill_protocol_field(UART_CMD_FACTORY_RECOVERY, payload_len, &uart_frame->frame_seq);
    uart_send_frame(payload_len);

    ///< 2. factory recover
    mcu_recv_factory_recovery_cb();
}

static void mcu_rx_product_info_request(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }
    
    unsigned short payload_len = 0;
    // a. pid
    uart_framing_fill_payload_buff(&payload_len, (unsigned char *)"{\"p\":\"", my_strlen((unsigned char *)"{\"p\":\""));
    uart_framing_fill_payload_buff(&payload_len, (unsigned char *)PRODUCT_KEY, 8);      // must be 8, or zigbee module's json parse will fail

    // b. mcu version
    uart_framing_fill_payload_buff(&payload_len, (unsigned char *)"\",\"v\":\"", my_strlen((unsigned char *)"\",\"v\":\""));
    uart_framing_fill_payload_buff(&payload_len, (unsigned char *)MCU_VER, my_strlen((unsigned char *)MCU_VER));

    // c. support broadcast
    uart_framing_fill_payload_buff(&payload_len, (unsigned char *)"\",\"g\":", my_strlen((unsigned char *)"\",\"g\":"));
#if SUPPORT_RECEIVE_BROADCAST_DATA
    uart_framing_fill_payload_byte(&payload_len, '1');
#else
    uart_framing_fill_payload_byte(&payload_len, '0');
#endif

    // d. scene switch
    uart_framing_fill_payload_buff(&payload_len, (unsigned char *)",\"s\":", my_strlen((unsigned char *)",\"s\":"));
#if (DEVICE_TYPE == SCENE_SWITCH_DEVICE)
    uart_framing_fill_payload_byte(&payload_len, '1');
#else
    uart_framing_fill_payload_byte(&payload_len, '0');
#endif

    uart_framing_fill_payload_buff(&payload_len, (unsigned char *)"}", my_strlen((unsigned char *)"}"));
    uart_framing_fill_protocol_field(UART_CMD_PRODUCT_INFO, payload_len, &uart_frame->frame_seq);
    uart_send_frame(payload_len);
}

static void mcu_rx_zg_nwk_status_notify(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }
    
    ///< 1. send frame
    unsigned short payload_len = 0;
    uart_framing_fill_protocol_field(UART_CMD_ZG_NWK_STATUS_NOTIFY, payload_len, &uart_frame->frame_seq);
    uart_send_frame(payload_len);
    
    ///< 2. handle zigbee nwk status
    unsigned char zg_nwk_status = uart_frame->frame_payload[0];
    mcu_recv_zg_nwk_status_notify_cb(zg_nwk_status);
}

static void mcu_rx_common_dp_msg(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }
    
    ///< 1. dp report type (must respond with 0x05)
    g_dp_send_type = DP_SEND_TYPE_RESPOND;

    ///< 2. dp handle and report
    unsigned char ret = 0;
    unsigned char dp_id = 0;
    unsigned char dp_type = 0;
    unsigned short dp_len = 0;
    unsigned char *dp_data = NULL;

    for (unsigned short i = 0; i < uart_frame->frame_payload_len;) {
        dp_id = uart_frame->frame_payload[i];
        dp_type = uart_frame->frame_payload[i + 1]; 
        dp_len = uart_frame->frame_payload[i + 2] * 0x100 + uart_frame->frame_payload[i + 3];
        dp_data = uart_frame->frame_payload + i + 4;

#if TUYA_LOG_ENABLE
        TUYA_LOG_INFO("[Tuya] DP id=0x%02X(%u) type=%s len=%u\r\n",
                dp_id, dp_id, tuya_get_dp_type_name(dp_type), dp_len);
        if (dp_len > 0) {
            tuya_log_payload_hex(dp_data, dp_len);
        }
#endif

        if (TRUE == dp_info_check(dp_id, dp_type)) {
            ret = dp_msg_handle(dp_id, dp_data, dp_len);
        } else {
            ret = FALSE;
        }
        mcu_send_dp_msg_cb(ret, dp_id, dp_type, dp_len, dp_data);

        i += (dp_len + 4);
    }
}

static void mcu_rx_zg_info(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }

    unsigned char *zg_info = uart_frame->frame_payload;
    unsigned char info_type = 0;
    for (unsigned short i = 0; i < uart_frame->frame_payload_len;) {
        ///< 1. get info type
        info_type = zg_info[i];
        i++;

        ///< 2. get info
        switch (info_type) {
            case ZG_INFO_TYPE_SW_VER: {
                mcu_recv_zg_sw_ver_cb(zg_info[i]);
                i++;
                break;
            }

            case ZG_INFO_TYPE_AUTH: {
                mcu_recv_zg_auth_cb(zg_info[i]);
                i++;
                break;
            }

            case ZG_INFO_TYPE_MAC_ADDR: {
                unsigned char mac_addr[8] = {0};
                for (unsigned char j = 0; j < 8; j++) {
                    mac_addr[j] = zg_info[i + j];       // high byte first
                }
                mcu_recv_zg_mac_addr_cb(mac_addr);
                i += 8;
                break;
            }
            
            default: {
                break;
            }
        }
    }
}

static void mcu_rx_rf_test_result(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }

    unsigned char rf_result = uart_frame->frame_payload[0];
    unsigned char rssi = uart_frame->frame_payload[1];

    mcu_recv_rf_test_result_cb(rf_result, rssi);
}

static void mcu_rx_mcu_version_request(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }

    __send_mcu_fw_ver(&uart_frame->frame_seq);
}
static void __send_mcu_fw_ver(unsigned short *uart_frame_seq)
{
    unsigned short payload_len = 0;
    uart_framing_fill_payload_byte(&payload_len, __get_native_mcu_fw_ver());
    uart_framing_fill_protocol_field(UART_CMD_MCU_VER, payload_len, uart_frame_seq);
    uart_send_frame(payload_len);
}

static unsigned char __get_native_mcu_fw_ver(void)
{
    unsigned char *fw_ver = (unsigned char *)MCU_VER; // current mcu fw version
    unsigned char native_mcu_fw_ver = 0;

    if (ascii_to_hex(fw_ver[5]) == (char)(-1)) {
        // 3 number     eg. 1.0.2
        native_mcu_fw_ver = (ascii_to_hex(fw_ver[0]) & 0x03) << 6;
        native_mcu_fw_ver |= (ascii_to_hex(fw_ver[2]) & 0x03) << 4;
        native_mcu_fw_ver |= (ascii_to_hex(fw_ver[4]) & 0x0F);
    } else {
        // 4 number     eg. 1.0.12
        native_mcu_fw_ver = (ascii_to_hex(fw_ver[0]) & 0x03) << 6;
        native_mcu_fw_ver |= (ascii_to_hex(fw_ver[2]) & 0x03) << 4;
        native_mcu_fw_ver |= ((ascii_to_hex(fw_ver[4]) * 10 + ascii_to_hex(fw_ver[5])) & 0x0F);
    }

    return native_mcu_fw_ver;
}

static void mcu_rx_ota_notify(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }

    unsigned char *rx_ota_info = uart_frame->frame_payload;
    unsigned char i = 0;
    unsigned short payload_len = 0;

    ///< 1. init native pid
    __init_native_pid();

    ///< 2. get OTA info
    my_memset(&sg_ota_fw_info, 0, sizeof(OTA_FW_INFO_T));
    for (i = 0; i < 8; i++) {
        sg_ota_fw_info.ota_pid[i] = rx_ota_info[i];
    }
    sg_ota_fw_info.ota_fw_ver = rx_ota_info[8];
    for (i = 0; i < 4; i++) {
        sg_ota_fw_info.ota_fw_size <<= 8;
        sg_ota_fw_info.ota_fw_size += rx_ota_info[9 + i];
    }
    for (i = 0; i < 4; i++) {
        sg_ota_fw_info.ota_expect_checksum <<= 8;
        sg_ota_fw_info.ota_expect_checksum += rx_ota_info[13 + i];
    }

    ///< 3. check OTA info
    unsigned char check_result = FALSE;
    if ((0 == my_strncmp(sg_ota_fw_info.ota_pid, sg_native_pid, 8)) && (sg_ota_fw_info.ota_fw_ver > __get_native_mcu_fw_ver()) && (sg_ota_fw_info.ota_fw_size > 0)) {
        payload_len = 0;
        uart_framing_fill_payload_byte(&payload_len, 0x01);     // OK
        uart_framing_fill_protocol_field(UART_CMD_OTA_NOTIFY, payload_len, &uart_frame->frame_seq);
        uart_send_frame(payload_len);

        // unlock
        sg_ota_data_request_lock = TRUE;       // TRUE is can request
        check_result = TRUE;
    } else {
        payload_len = 0;
        uart_framing_fill_payload_byte(&payload_len, 0x00);     // ERROR
        uart_framing_fill_protocol_field(UART_CMD_OTA_NOTIFY, payload_len, &uart_frame->frame_seq);
        uart_send_frame(payload_len);

        // lock
        sg_ota_data_request_lock = FALSE;
        check_result = FALSE;
    }

    mcu_recv_ota_notify_cb(check_result);
}
static void __init_native_pid(void)
{
    unsigned char *fw_pid = (unsigned char *)PRODUCT_KEY;
    unsigned char i = 0;

    for (i = 0; i < 8; i++) {
        sg_native_pid[i] = fw_pid[i];
    }
}

static void mcu_rx_ota_data(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }

    unsigned char i = 0;

    unsigned char *rx_ota_packet = uart_frame->frame_payload;
    unsigned int rx_ota_offset = 0;
    static unsigned int s_cur_checksum = 0;
    unsigned char rx_ota_data[OTA_PACKET_SIZE] = {0};

    ///< 1. check OTA info
    if (FALSE == __check_ota_data(rx_ota_packet, &rx_ota_offset)) {
        return;
    }

    ///< 2. the first packet
    if (sg_ota_fw_info.ota_current_offset == 0) {
        s_cur_checksum = 0;
    }

    ///< 3. non-last packet
    if ((sg_ota_fw_info.ota_fw_size - rx_ota_offset) > OTA_PACKET_SIZE) {
        // a. checksum
        for (i = 0; i < OTA_PACKET_SIZE; i++) {
            rx_ota_data[i] = rx_ota_packet[14 + i];
            s_cur_checksum += rx_ota_data[i];
        }

        // b. handle
        sg_ota_fw_info.ota_current_offset += OTA_PACKET_SIZE;
        mcu_recv_ota_data_cb(FALSE, rx_ota_offset, rx_ota_data, OTA_PACKET_SIZE);
    } else {
    ///< 4. the last packet
        unsigned char last_package_len = sg_ota_fw_info.ota_fw_size - rx_ota_offset;
        // a. checksum
        for (i = 0; i < last_package_len; i++) {
            rx_ota_data[i] = rx_ota_packet[14 + i];
            s_cur_checksum += rx_ota_data[i];
        }

        
        // b. handle
        sg_ota_fw_info.ota_current_offset += last_package_len;
        mcu_recv_ota_data_cb(TRUE, rx_ota_offset, rx_ota_data, last_package_len);

        // c. send OTA result
        if (sg_ota_fw_info.ota_expect_checksum != s_cur_checksum) {
            mcu_tx_ota_result(1);       // failed, but the OTA process won't end until OTA timeout
        } else {
            mcu_tx_ota_result(0);       // success, the OTA process will end with success
        }
        // s_cur_checksum = 0;
    }

    ///< 5. unlock
    sg_ota_data_request_lock = TRUE;    // TRUE is can request
}
static unsigned char __check_ota_data(unsigned char *rx_ota_packet, unsigned int *rx_ota_offset)
{
    unsigned char i = 0;
    if (NULL == rx_ota_packet || NULL == rx_ota_offset) {
        return FALSE;
    }
    
    ///< 1. result
    if (0x01 == rx_ota_packet[0]) {
        return FALSE;
    }

    ///< 2. pid
    for (i = 0; i < 8; i++) {
        if (sg_native_pid[i] != rx_ota_packet[1 + i]) {
            return FALSE;
        }
    }

    ///< 3. mcu fw version
    if (sg_ota_fw_info.ota_fw_ver != rx_ota_packet[9]) {
        return FALSE;
    }

    ///< 4. OTA data offset
    unsigned int temp_offset = 0;
    for (i = 0; i < 4; i++) {
        temp_offset <<= 8;
        temp_offset += rx_ota_packet[10 + i];
    }

    // wrong offset
    if (sg_ota_fw_info.ota_current_offset != temp_offset) {
        // when zigbee module not recv the new OTA data request in 3s, zigbee module will resend at most 2 times to request the previous OTA data
        // when mcu recv the new OTA data response, mcu's offset will be updated and not equal to the previous offset (the 'resend' offset)
        PRINT_DEBUG("[ERROR] cur offset %d, rx offset %d\r\n", sg_ota_fw_info.ota_current_offset, temp_offset);
        return FALSE;
    }

    *rx_ota_offset = temp_offset;
    return TRUE;
}

static void mcu_rx_check_zg_nwk_status_response(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }

    unsigned char zg_nwk_status = uart_frame->frame_payload[0];
    mcu_recv_zg_nwk_status_response_cb(zg_nwk_status);
}

static void mcu_rx_double_dongle_test_notify(void)
{
    unsigned short payload_len = 0;
    uart_framing_fill_protocol_field(UART_CMD_DOUBLE_DONGLE_DATA_NOTIFY, payload_len, NULL);
    uart_send_frame(payload_len);

    mcu_recv_double_dongle_test_notify_cb();
}
static void mcu_rx_double_dongle_test_data(unsigned char *test_info, unsigned short test_info_len)
{
    if (NULL == test_info) {
        return;
    }

    ///< 1. send frame
    unsigned short payload_len = 0;
    uart_framing_fill_protocol_field(UART_CMD_DOUBLE_DONGLE_DATA_NOTIFY, payload_len, NULL);
    uart_send_frame(payload_len);

    ///< 3. handle test data
    mcu_recv_double_dongle_test_data_cb(test_info, test_info_len);
}

static void mcu_rx_time_sync_timestamp(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }

    ///< 1. calculate timestamp to time format
    unsigned char *__timestamp = uart_frame->frame_payload;
    // __timestamp[0]~__timestamp[3]：standard timestamp
    // __timestamp[4]~__timestamp[7]: local timestamp
    my_memcpy((void *)sg_timestamp, (const char *)__timestamp, 4);      // standard timestamp
    __convert_timestamp_to_calendar();

    mcu_recv_time_sync_cb(sg_timestamp, &sg_time_calendar);
}
static void __convert_timestamp_to_calendar(void)
{
    unsigned int time_stamp = byte_to_int(sg_timestamp);
    __calculate_week_and_date(time_stamp, &sg_time_calendar);
}
static void __calculate_week_and_date(unsigned int timestamp, TIME_SYNC_CALENDAR_T *calendar)
{
    static unsigned int total_days = 0;
    unsigned int temp_day = 0;
    unsigned int temp_month = 0;
    unsigned int temp_year = 0;
    if (NULL == calendar) {
        return;
    }
    
    temp_day = timestamp / 86400;
    if (total_days != temp_day) {
        // for the next day
        total_days = temp_day;
        temp_year = 1970;       // year is based on the 1970

        // calculate years
        while (temp_day >= 365) {
            if (__is_leap_year(temp_year)) {
                if (temp_day >= 366) {
                    temp_day -= 366;
                } else {
                    break;
                }
            } else {
                temp_day -= 365;
            }
            temp_year++;
        }
        calendar->w_year = temp_year;

        // calculate months and days
        const unsigned char mon_table[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        while (temp_day >= 28)
        {
            if (__is_leap_year(calendar->w_year) && temp_month == 1) {
                if (temp_day >= 29) {
                    temp_day -= 29;
                } else {
                    break;
                }
            } else {
                if (temp_day >= mon_table[temp_month]) {
                    temp_day -= mon_table[temp_month];
                } else {
                    break;
                }
            }
            temp_month++;
        }
        calendar->w_month = temp_month + 1;
        calendar->w_day = temp_day + 1;
    }

    // calculate date
    temp_day = timestamp % 86400;
    calendar->hour = temp_day / 3600;
    calendar->min = (temp_day % 3600) / 60;
    calendar->sec = (temp_day % 3600) % 60;
}
static unsigned char __is_leap_year(unsigned int year)
{
    if (0 != year % 4) {
        return 0;
    }
    if (0 != year % 100) {
        return 1;
    }
    if (0 == (year % 400)) {
        return 1;
    }
    return 0;
}

static void mcu_rx_gw_nwk_status(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }

    unsigned char gw_nwk_status = uart_frame->frame_payload[0];
    mcu_recv_gw_nwk_status_cb(gw_nwk_status);
}

static void mcu_rx_gw_check_dp(UART_FRAME_T *uart_frame)
{
    ///< 1. send 0x28+blank
    unsigned short __length = 0;
    uart_framing_fill_protocol_field(UART_CMD_GW_CHECK_DP, __length, &uart_frame->frame_seq);
    uart_send_frame(__length);

    ///< 2. dp report type (linkage trigger or not)
    g_dp_send_type = DP_SEND_TYPE_REPORT_LINKAGE;       //  default<0x06>

    ///< 3. dp report
    unsigned short report_dp_num = uart_frame->frame_payload_len;
    if (report_dp_num == 0) {       // report all dp
        all_data_update();
    } else {                        // report list of dp
        unsigned char report_dp_id = 0;
        for (unsigned char i = 0; i < report_dp_num; i++) {
            report_dp_id = uart_frame->frame_payload[i];
            specific_dp_update(report_dp_id);
        }
    }
}

static void mcu_rx_beacon_test_notify(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }

    ///< 1. lock
    static unsigned char s_beacon_start_flag = FALSE;
    if (TRUE == s_beacon_start_flag) {
        return;
    }
    s_beacon_start_flag = TRUE;
    
    ///< 2. self-test
    unsigned char ret = mcu_recv_beacon_notify_cb();

    ///< 3. send result to zigbee module
    unsigned short payload_len = 0;
    uart_framing_fill_payload_byte(&payload_len, ret);
    uart_framing_fill_protocol_field(UART_CMD_BEACON_TEST, payload_len, &uart_frame->frame_seq);
    uart_send_frame(payload_len);
}

#if SUPPORT_RECEIVE_BROADCAST_DATA
static void mcu_rx_group_dp_msg(UART_FRAME_T *uart_frame)
{
    ///< 1. dp report type
    g_dp_send_type = DP_SEND_TYPE_NOT_SEND;

    ///< 2. dp handle and report
    unsigned char ret = 0;
    unsigned char dp_id = 0;
    unsigned char dp_type = 0;
    unsigned short dp_len = 0;
    unsigned char *dp_data = NULL;
    
    for (unsigned short i = 0; i < uart_frame->frame_payload_len;) {
        dp_id = uart_frame->frame_payload[i];
        dp_type = uart_frame->frame_payload[i + 1]; 
        dp_len = uart_frame->frame_payload[i + 2] * 0x100 + uart_frame->frame_payload[i + 3];
        dp_data = uart_frame->frame_payload + i + 4;

        if (TRUE == dp_info_check(dp_id, dp_type)) {
            ret = dp_msg_handle(dp_id, dp_data, dp_len);
        } else {
            ret = FALSE;
        }
        mcu_send_dp_msg_cb(ret, dp_id, dp_type, dp_len, dp_data);

        i += (dp_len + 4);
    }
}
#endif

static void mcu_rx_config_gpio_result(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }

    unsigned char gpio_num = uart_frame->frame_payload[0];
    unsigned char *gpio_info = uart_frame->frame_payload + 1;

    unsigned char gpio_port;
    unsigned char gpio_pin;
    unsigned char gpio_config_result;
    for (unsigned char i = 0; i < gpio_num; i++) {
        ///< 1. get result
        gpio_port = gpio_info[i * 3];
        gpio_pin = gpio_info[i * 3 + 1];
        gpio_config_result = gpio_info[i * 3 + 2];

        ///< 2. handle result
        mcu_recv_gpio_config_result_cb(gpio_port, gpio_pin, gpio_config_result);
    }
}

static void mcu_rx_read_gpio_result(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }

    unsigned char gpio_num = uart_frame->frame_payload[0];
    unsigned char *gpio_info = uart_frame->frame_payload + 1;

    unsigned char gpio_port;
    unsigned char gpio_pin;
    unsigned char gpio_level;
    for (unsigned char i = 0; i < gpio_num; i++) {
        ///< 1. get result
        gpio_port = gpio_info[i * 3];
        gpio_pin = gpio_info[i * 3 + 1];
        gpio_level = gpio_info[i * 3 + 2];

        ///< 2. handle result
        mcu_recv_gpio_read_result_cb(gpio_port, gpio_pin, gpio_level);
    }
}

static void mcu_rx_write_gpio_result(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }

    unsigned char gpio_num = uart_frame->frame_payload[0];
    unsigned char *gpio_info = uart_frame->frame_payload + 1;

    unsigned char gpio_port;
    unsigned char gpio_pin;
    unsigned char gpio_write_result;
    for (unsigned char i = 0; i < gpio_num; i++) {
        ///< 1. get result
        gpio_port = gpio_info[i * 3];
        gpio_pin = gpio_info[1 + i * 3];
        gpio_write_result = gpio_info[2 + i * 3];

        ///< 2. handle result
        mcu_recv_gpio_write_result_cb(gpio_port, gpio_pin, gpio_write_result);
    }
}

static void mcu_rx_gpio_irq(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }

    ///< 1. send to zigbee module
    unsigned short payload_len = 0;
    uart_framing_fill_protocol_field(UART_CMD_ZG_GPIO_IRQ, payload_len, &uart_frame->frame_seq);
    uart_send_frame(payload_len);

    ///< 2. handle
    unsigned char gpio_port = uart_frame->frame_payload[0];
    unsigned char gpio_pin = uart_frame->frame_payload[1];
    unsigned char gpio_level = uart_frame->frame_payload[2];
    mcu_recv_gpio_irq_cb(gpio_port, gpio_pin, gpio_level);
}

static void mcu_rx_weather_data(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }

    WEATHER_INFO_T weather_info = {
        .version = uart_frame->frame_payload[0],                // 0x11
        .addr_type = uart_frame->frame_payload[1],
        .forecast_flag = uart_frame->frame_payload[2],          // 0x12
        .forecast_days = uart_frame->frame_payload[3],
        .real_time_weather_flag = uart_frame->frame_payload[4], // 0x13
        .real_time_enable = uart_frame->frame_payload[5],
        .weather_detail = uart_frame->frame_payload + 6,
        .weather_detail_len = uart_frame->frame_payload_len - 6,
    };
    mcu_recv_weather_response_cb(&weather_info);
}

static void mcu_rx_city_data(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }

    CITY_INFO_T city_info = {
        .version = uart_frame->frame_payload[0],                // 0x11
        .addr_type = uart_frame->frame_payload[1],
        .city_detail = uart_frame->frame_payload + 2,
        .city_detail_len = uart_frame->frame_payload_len - 2,
    };
    mcu_recv_city_response_cb(&city_info);
}

#if (DEVICE_TYPE == SCENE_SWITCH_DEVICE)
static void mcu_rx_scene_config(UART_FRAME_T *uart_frame)
{
    if (NULL == uart_frame || NULL == uart_frame->frame_payload) {
        return;
    }

    ///< 1. handle scene info
    unsigned char key_id = uart_frame->frame_payload[0];
    /* 组播ID: 大端序 uint16 (payload[1]=高字节, payload[2]=低字节) */
    unsigned short group_id = ((unsigned short)uart_frame->frame_payload[1] << 8)
                            |  uart_frame->frame_payload[2];
    unsigned char scene_id = uart_frame->frame_payload[3];
    
    unsigned char ret = mcu_recv_scene_config_cb(key_id, group_id, scene_id);

    ///< 2. send frame
    unsigned short payload_len = 0;
    uart_framing_fill_payload_byte(&payload_len, ret);
    uart_framing_fill_protocol_field(UART_CMD_SCENE_CONFIG, payload_len, &uart_frame->frame_seq);
    uart_send_frame(payload_len);
}
#endif

/*----------------------------------------------------------
 *                  frame sending function
 * 1. you can call these functions in your project to send frame to zigbee module.
 *--------------------------------------------------------*/

void mcu_tx_let_zigbee_reset(void)
{
    unsigned short payload_len = 0;
    uart_framing_fill_payload_byte(&payload_len, 0x00);
    uart_framing_fill_protocol_field(UART_CMD_RESET_OR_JOIN, payload_len, NULL);
    uart_send_frame(payload_len);
}
void mcu_tx_let_zigbee_start_join(void)
{
    unsigned short payload_len = 0;
    uart_framing_fill_payload_byte(&payload_len, 0x01);
    uart_framing_fill_protocol_field(UART_CMD_RESET_OR_JOIN, payload_len, NULL);
    uart_send_frame(payload_len);
}

void mcu_tx_report_dp_linkage(unsigned char dp_id, unsigned char dp_type, unsigned short dp_len, unsigned char *dp_data)
{
    if (NULL == dp_data) {
        return;
    }
    
    ///< 1. dp report type
    g_dp_send_type = DP_SEND_TYPE_REPORT_LINKAGE;

    ///< 2. dp report
    unsigned char ret = 0;
    if (TRUE == dp_info_check(dp_id, dp_type)) {
        ret = dp_msg_handle(dp_id, dp_data, dp_len);
    } else {
        ret = FALSE;
    }
    mcu_send_dp_msg_cb(ret, dp_id, dp_type, dp_len, dp_data);
}

void mcu_tx_request_zg_info(unsigned char sw_ver_req, unsigned char auth_req, unsigned char mac_addr_req)
{
    if ((0 == sw_ver_req) && (0 == auth_req) && (0 == mac_addr_req)) {
        return;
    }

    unsigned short payload_len = 0;

    if (1 == sw_ver_req) {
        uart_framing_fill_payload_byte(&payload_len, ZG_INFO_TYPE_SW_VER);
    }
    if (1 == auth_req) {
        uart_framing_fill_payload_byte(&payload_len, ZG_INFO_TYPE_AUTH);
    }
    if (1 == mac_addr_req) {
        uart_framing_fill_payload_byte(&payload_len, ZG_INFO_TYPE_MAC_ADDR);
    }
    
    uart_framing_fill_protocol_field(UART_CMD_ZG_INFO, payload_len, NULL);
    uart_send_frame(payload_len);
}

void mcu_tx_start_zigbee_rf_test(unsigned char channel)
{
    unsigned short payload_len = 0;

    if (channel < 11 || channel > 26) {
        channel = 11;
    }
    uart_framing_fill_payload_byte(&payload_len, channel);

    uart_framing_fill_protocol_field(UART_CMD_RF_TEST, payload_len, NULL);
    uart_send_frame(payload_len);
}

#if (DEVICE_TYPE == SCENE_SWITCH_DEVICE)
void mcu_tx_scene_key_id(unsigned char keyID)
{
    unsigned short payload_len = 0;
    uart_framing_fill_payload_byte(&payload_len, keyID);

    uart_framing_fill_protocol_field(UART_CMD_SCENE_INFO, payload_len, NULL);
    uart_send_frame(payload_len);
}
#endif

void mcu_tx_fw_ver(void)
{
    __send_mcu_fw_ver(NULL);
}

unsigned char mcu_tx_request_ota_data(void)
{
    unsigned char i = 0;
    unsigned short payload_len = 0;

    ///< 1. check the lock
    if (FALSE == sg_ota_data_request_lock) {
        return 2;       // not allowed to request OTA data
    }

    ///< 2. check if OTA finishes
    if (sg_ota_fw_info.ota_current_offset >= sg_ota_fw_info.ota_fw_size) {
        return 1;       // OTA finish
    }

    ///< 3. send frame to request OTA data
    for (i = 0; i < 8; i++) {
        uart_framing_fill_payload_byte(&payload_len, sg_ota_fw_info.ota_pid[i]); 
    }
    uart_framing_fill_payload_byte(&payload_len, sg_ota_fw_info.ota_fw_ver);
    for (i = 0; i < 4; i++) {
        uart_framing_fill_payload_byte(&payload_len, sg_ota_fw_info.ota_current_offset >> (24 - i * 8));
    }
    uart_framing_fill_payload_byte(&payload_len, OTA_PACKET_SIZE);
    uart_framing_fill_protocol_field(UART_CMD_OTA_DATA, payload_len, NULL);
    uart_send_frame(payload_len);

    ///< 4. lock
    sg_ota_data_request_lock = FALSE;

    return 0;           // OTAing
}

void mcu_tx_ota_result(unsigned char status)
{
    unsigned char i = 0;
    unsigned short payload_len = 0;

    ///< 1. send frame
    // a. result (0x00: success; 0x01: failed)
    uart_framing_fill_payload_byte(&payload_len, status);
    // b. pid
    for (i = 0; i < 8; i++) {
        uart_framing_fill_payload_byte(&payload_len, sg_ota_fw_info.ota_pid[i]);
    }
    // c. new fw version
    uart_framing_fill_payload_byte(&payload_len, sg_ota_fw_info.ota_fw_ver);

    uart_framing_fill_protocol_field(UART_CMD_OTA_RESULT, payload_len, NULL);
    uart_send_frame(payload_len);
}

void mcu_tx_check_zg_nwk_status(void)
{
    unsigned short payload_len = 0;

    uart_framing_fill_protocol_field(UART_CMD_ZG_NWK_STATUS_REQUEST, payload_len, NULL);
    uart_send_frame(payload_len);
}

void mcu_tx_double_dongle_test_report_data(unsigned char *report_data, unsigned short report_data_len)
{
    if (NULL == report_data) {
        return;
    }

    ///< 1. send frame
    unsigned short payload_len = 0;
    uart_framing_fill_payload_buff(&payload_len, report_data, report_data_len);
    uart_framing_fill_protocol_field(UART_CMD_DOUBLE_DONGLE_REPORT, payload_len, NULL);
    uart_send_frame(payload_len);
}

void mcu_tx_request_time_sync(void)
{
    ///< 1. send frame
    unsigned short payload_len = 0;
    uart_framing_fill_protocol_field(UART_CMD_TIME_SYNC, payload_len, NULL);
    uart_send_frame(payload_len);
}

void mcu_tx_check_gw_nwk_status(void)
{
    unsigned short payload_len = 0;

    uart_framing_fill_protocol_field(UART_CMD_MCU_CHECK_GW_NWK_STAUTS, payload_len, NULL);
    uart_send_frame(payload_len);
}

void mcu_tx_set_zg_nwk_param(NWK_PARAM_T *nwk_param)
{
    ///< 1. if use default nwk param
    if (NULL == nwk_param){
        nwk_param = &sg_default_nwk_param;
    }

    ///< 2. send frame
    unsigned short payload_len = 0;
    uart_framing_fill_payload_byte(&payload_len, (nwk_param->heart_period >> 8));
    uart_framing_fill_payload_byte(&payload_len, (nwk_param->heart_period));
    uart_framing_fill_payload_byte(&payload_len, (nwk_param->join_timeout >> 8));
    uart_framing_fill_payload_byte(&payload_len, (nwk_param->join_timeout));
    uart_framing_fill_payload_byte(&payload_len, (nwk_param->rejoin_interval >> 8));
    uart_framing_fill_payload_byte(&payload_len, (nwk_param->rejoin_interval));
    uart_framing_fill_payload_byte(&payload_len, (nwk_param->poll_interval >> 8));
    uart_framing_fill_payload_byte(&payload_len, (nwk_param->poll_interval));
    uart_framing_fill_payload_byte(&payload_len, (nwk_param->fast_poll_period >> 8));
    uart_framing_fill_payload_byte(&payload_len, (nwk_param->fast_poll_period));
    uart_framing_fill_payload_byte(&payload_len, (nwk_param->poll_fail_times));
    uart_framing_fill_payload_byte(&payload_len, (nwk_param->app_data_trig_rejoin));
    uart_framing_fill_payload_byte(&payload_len, (nwk_param->rejoin_try_times));
    uart_framing_fill_payload_byte(&payload_len, (nwk_param->rf_power));

    uart_framing_fill_protocol_field(UART_CMD_MCU_SET_ZG_NWK_PARAM, payload_len, NULL);
    uart_send_frame(payload_len);
}

void mcu_tx_send_dp_broadcast(unsigned char dp_id, unsigned char dp_type, unsigned short dp_len, unsigned char *dp_data)
{
    if (NULL == dp_data) {
        return;
    }

    ///< 1. dp report type
    g_dp_send_type = DP_SEND_TYPE_SEND_BROADCAST;

    ///< 2. dp report
    unsigned char ret = 0;
    if (TRUE == dp_info_check(dp_id, dp_type)) {
        ret = dp_msg_handle(dp_id, dp_data, dp_len);
    } else {
        ret = FALSE;
    }
    mcu_send_dp_msg_cb(ret, dp_id, dp_type, dp_len, dp_data);
}

#if (DEVICE_TYPE == SLEEP_END_DEVICE)
void mcu_tx_set_zg_wait_mcu_time(unsigned short time_ms)
{
    ///< 1. send frame
    unsigned short payload_len = 0;
    uart_framing_fill_payload_byte(&payload_len, time_ms >> 8);
    uart_framing_fill_payload_byte(&payload_len, time_ms);

    uart_framing_fill_protocol_field(UART_CMD_ZG_WAIT_MCU_TIME, payload_len, NULL);
    uart_send_frame(payload_len);

    ///< 2. set time
    g_zg_wait_mcu_time = time_ms;
}
#endif

void mcu_tx_report_dp_without_linkage(unsigned char dp_id, unsigned char dp_type, unsigned short dp_len, unsigned char *dp_data)
{
    if (NULL == dp_data) {
        return;
    }

    ///< 1. dp report type
    g_dp_send_type = DP_SEND_TYPE_REPORT_NOT_LINKAGE;

    ///< 2. dp report
    unsigned char ret = 0;
    if (TRUE == dp_info_check(dp_id, dp_type)) {
        ret = dp_msg_handle(dp_id, dp_data, dp_len);
    } else {
        ret = FALSE;
    }
    mcu_send_dp_msg_cb(ret, dp_id, dp_type, dp_len, dp_data);
}

void mcu_tx_config_zg_gpio(unsigned char gpio_num, MCU_CONFIG_GPIO_T *gpio_param)
{
    unsigned char i = 0;
    unsigned short payload_len = 0;
    if (NULL == gpio_param) {
        return;
    }

    uart_framing_fill_payload_byte(&payload_len, gpio_num);
    for (i = 0; i < gpio_num; i++) {
        uart_framing_fill_payload_byte(&payload_len, gpio_param[i].port_num);
        uart_framing_fill_payload_byte(&payload_len, gpio_param[i].pin_num);
        uart_framing_fill_payload_byte(&payload_len, gpio_param[i].gpio_mode);
        uart_framing_fill_payload_byte(&payload_len, gpio_param[i].gpio_level);
    }

    uart_framing_fill_protocol_field(UART_CMD_CONFIG_ZG_GPIO, payload_len, NULL);
    uart_send_frame(payload_len);
}

void mcu_tx_read_zg_gpio(unsigned char gpio_num, MCU_READ_GPIO_T *gpio_param)
{
    unsigned char i = 0;
    unsigned short payload_len = 0;
    if (NULL == gpio_param) {
        return;
    }

    uart_framing_fill_payload_byte(&payload_len, gpio_num);
    for (i = 0; i < gpio_num; i++) {
        uart_framing_fill_payload_byte(&payload_len, gpio_param[i].port_num);
        uart_framing_fill_payload_byte(&payload_len, gpio_param[i].pin_num);
    }

    uart_framing_fill_protocol_field(UART_CMD_READ_ZG_GPIO, payload_len, NULL);
    uart_send_frame(payload_len);
}

void mcu_tx_write_zg_gpio(unsigned char gpio_num, MCU_WRITE_GPIO_T *gpio_param)
{
    unsigned char i = 0;
    unsigned short payload_len = 0;
    if (NULL == gpio_param) {
        return;
    }

    uart_framing_fill_payload_byte(&payload_len, gpio_num);
    for (i = 0; i < gpio_num; i++) {
        uart_framing_fill_payload_byte(&payload_len, gpio_param[i].port_num);
        uart_framing_fill_payload_byte(&payload_len, gpio_param[i].pin_num);
        uart_framing_fill_payload_byte(&payload_len, gpio_param[i].gpio_level);
    }

    uart_framing_fill_protocol_field(UART_CMD_WRITE_ZG_GPIO, payload_len, NULL);
    uart_send_frame(payload_len);
}

void mcu_tx_weather_request(unsigned char addr_type, unsigned char *weather_id_list, unsigned char weather_id_num, unsigned char forecast_days, unsigned char real_time_enable)
{
    unsigned short payload_len = 0;
    if (NULL == weather_id_list || 0 == weather_id_num || 7 < forecast_days) {
        return;
    }

    // at least one weather information request type
    if (0 == forecast_days && 0 == real_time_enable) {
        return;
    }

    uart_framing_fill_payload_byte(&payload_len, 0x11);                              // version 0x11
    uart_framing_fill_payload_byte(&payload_len, addr_type);                         // 0x00 device address; 0x01 home address;
    uart_framing_fill_payload_buff(&payload_len, weather_id_list, weather_id_num);   // weather id list
    uart_framing_fill_payload_byte(&payload_len, 0x12);
    uart_framing_fill_payload_byte(&payload_len, forecast_days);
    uart_framing_fill_payload_byte(&payload_len, 0x13);
    if (0 != real_time_enable) {
        real_time_enable = 1;
    }
    uart_framing_fill_payload_byte(&payload_len, real_time_enable);

    uart_framing_fill_protocol_field(UART_CMD_WEATHER_REQUEST, payload_len, NULL);
    uart_send_frame(payload_len);
}

void mcu_tx_city_request(unsigned char addr_type)
{
    unsigned short payload_len = 0;
    unsigned char area_flag[6] = {0x63, 0x2E, 0x61, 0x72, 0x65, 0x61};
    unsigned char city_flag[6] = {0x63, 0x2E, 0x63, 0x69, 0x74, 0x79};

    uart_framing_fill_payload_byte(&payload_len, 0x11);                              // version 0x11
    uart_framing_fill_payload_byte(&payload_len, addr_type);                         // 0x00 device address; 0x01 home address;
    uart_framing_fill_payload_buff(&payload_len, area_flag, 6);
    uart_framing_fill_payload_buff(&payload_len, city_flag, 6);

    uart_framing_fill_protocol_field(UART_CMD_WEATHER_REQUEST, payload_len, NULL);
    uart_send_frame(payload_len);
}

#if (DEVICE_TYPE == SCENE_SWITCH_DEVICE)
void mcu_tx_send_cmd_group(MCU_SEND_CMD_GROUP_T *standard_cmd)
{
    unsigned short payload_len = 0;
    if (NULL == standard_cmd) {
        return;
    }

    uart_framing_fill_payload_byte(&payload_len, (standard_cmd->group_id >> 8));
    uart_framing_fill_payload_byte(&payload_len, standard_cmd->group_id);
    uart_framing_fill_payload_byte(&payload_len, (standard_cmd->cluster_id >> 8));
    uart_framing_fill_payload_byte(&payload_len, standard_cmd->cluster_id);
    uart_framing_fill_payload_byte(&payload_len, standard_cmd->command_id);
    uart_framing_fill_payload_buff(&payload_len, standard_cmd->payload, standard_cmd->payload_len);

    uart_framing_fill_protocol_field(UART_CMD_SEND_CMD_GROUP, payload_len, NULL);
    uart_send_frame(payload_len);
}

void mcu_tx_send_dp_group(unsigned short group_id, unsigned char dp_id, unsigned char dp_type, unsigned short dp_len, unsigned char *dp_data)
{
    if (NULL == dp_data) {
        return;
    }

    ///< 1. dp report type
    g_dp_send_type = DP_SEND_TYPE_SEND_GROUP;
    g_group_id_send_dp_msg = group_id;

    ///< 2. dp report
    unsigned char ret = 0;
    if (TRUE == dp_info_check(dp_id, dp_type)) {
        ret = dp_msg_handle(dp_id, dp_data, dp_len);
    } else {
        ret = FALSE;
    }
    mcu_send_dp_msg_cb(ret, dp_id, dp_type, dp_len, dp_data);
}
#endif

/*----------------------------------------------------------
 *                dp receive handle function
 *--------------------------------------------------------*/
unsigned char dp_info_check(unsigned char dp_id, unsigned char dp_type)
{
    for (unsigned char index = 0; index < DP_TOTAL_NUM; index++) {
        if (download_cmd[index].dp_id == dp_id) {
            if (download_cmd[index].dp_type == dp_type) {
                return TRUE;        // correct
            } else {
                return FALSE;       // wrong
            }
        }
    }
    return FALSE;                   // not found
}

unsigned char mcu_get_dp_download_bool(const unsigned char *dp_data, unsigned short dp_data_len)
{
    if (NULL == dp_data) {
        return 0;
    }

    return dp_data[0];
}

unsigned char mcu_get_dp_download_enum(const unsigned char *dp_data, unsigned short dp_data_len)
{
    if (NULL == dp_data) {
        return 0;
    }

    return dp_data[0];
}

unsigned long mcu_get_dp_download_value(const unsigned char *dp_data, unsigned short dp_data_len)
{
    if (NULL == dp_data) {
        return 0;
    }

    return byte_to_int(dp_data);
}

/*----------------------------------------------------------
 *                    dp report function
 *--------------------------------------------------------*/

unsigned char mcu_dp_raw_update(unsigned char dp_id, const unsigned char *dp_data, unsigned short dp_data_len)
{
    unsigned short payload_len = 0;

    uart_framing_fill_payload_byte(&payload_len, dp_id);
    uart_framing_fill_payload_byte(&payload_len, DP_TYPE_RAW);
    uart_framing_fill_payload_byte(&payload_len, dp_data_len / 0x100);
    uart_framing_fill_payload_byte(&payload_len, dp_data_len % 0x100);
    uart_framing_fill_payload_buff(&payload_len, (unsigned char *)dp_data, dp_data_len);    // can be NULL

    __dp_send_type_handle(payload_len);

    return SUCCESS;
}

unsigned char mcu_dp_bool_update(unsigned char dp_id, unsigned char dp_data)
{
    unsigned short payload_len = 0;

    uart_framing_fill_payload_byte(&payload_len, dp_id);
    uart_framing_fill_payload_byte(&payload_len, DP_TYPE_BOOL);
    uart_framing_fill_payload_byte(&payload_len, 0);
    uart_framing_fill_payload_byte(&payload_len, 1);
    if (dp_data == FALSE) {
        uart_framing_fill_payload_byte(&payload_len, FALSE);
    } else {
        uart_framing_fill_payload_byte(&payload_len, TRUE);
    }

    __dp_send_type_handle(payload_len);

    return SUCCESS;
}

unsigned char mcu_dp_value_update(unsigned char dp_id, unsigned long dp_data)
{
    unsigned short payload_len = 0;

    uart_framing_fill_payload_byte(&payload_len, dp_id);
    uart_framing_fill_payload_byte(&payload_len, DP_TYPE_VALUE);
    uart_framing_fill_payload_byte(&payload_len, 0);
    uart_framing_fill_payload_byte(&payload_len, 4);
    uart_framing_fill_payload_byte(&payload_len, dp_data >> 24);
    uart_framing_fill_payload_byte(&payload_len, dp_data >> 16);
    uart_framing_fill_payload_byte(&payload_len, dp_data >> 8);
    uart_framing_fill_payload_byte(&payload_len, dp_data);

    __dp_send_type_handle(payload_len);

    return SUCCESS;
}

unsigned char mcu_dp_string_update(unsigned char dp_id, const unsigned char *dp_data, unsigned short dp_data_len)
{
    if (NULL == dp_data) {
        return ERROR;
    }
    
    unsigned short payload_len = 0;

    uart_framing_fill_payload_byte(&payload_len, dp_id);
    uart_framing_fill_payload_byte(&payload_len, DP_TYPE_STRING);
    uart_framing_fill_payload_byte(&payload_len, dp_data_len / 0x100);
    uart_framing_fill_payload_byte(&payload_len, dp_data_len % 0x100);
    uart_framing_fill_payload_buff(&payload_len, (unsigned char *)dp_data, dp_data_len);

    __dp_send_type_handle(payload_len);

    return SUCCESS;
}

unsigned char mcu_dp_enum_update(unsigned char dp_id, unsigned char dp_data)
{
    unsigned short payload_len = 0;

    uart_framing_fill_payload_byte(&payload_len, dp_id);
    uart_framing_fill_payload_byte(&payload_len, DP_TYPE_ENUM);
    uart_framing_fill_payload_byte(&payload_len, 0);
    uart_framing_fill_payload_byte(&payload_len, 1);
    uart_framing_fill_payload_byte(&payload_len, dp_data);

    __dp_send_type_handle(payload_len);

    return SUCCESS;
}

unsigned char mcu_dp_bitmap_update(unsigned char dp_id, unsigned long dp_data)
{
    unsigned short payload_len = 0;

    uart_framing_fill_payload_byte(&payload_len, dp_id);
    uart_framing_fill_payload_byte(&payload_len, DP_TYPE_BITMAP);
    uart_framing_fill_payload_byte(&payload_len, 0);
    if ((dp_data | 0xff) == 0xff) {                                     // 8 bit - 1 byte
        uart_framing_fill_payload_byte(&payload_len, 1);
        uart_framing_fill_payload_byte(&payload_len, dp_data);
    } else if ((dp_data | 0xffff) == 0xffff) {                          // 16 bit - 2 bytes
        uart_framing_fill_payload_byte(&payload_len, 2);
        uart_framing_fill_payload_byte(&payload_len, dp_data >> 8);
        uart_framing_fill_payload_byte(&payload_len, dp_data);
    } else {                                                            // 32 bit - 4 bytes
        uart_framing_fill_payload_byte(&payload_len, 4);
        uart_framing_fill_payload_byte(&payload_len, dp_data >> 24);
        uart_framing_fill_payload_byte(&payload_len, dp_data >> 16);
        uart_framing_fill_payload_byte(&payload_len, dp_data >> 8);
        uart_framing_fill_payload_byte(&payload_len, dp_data);
    }

    __dp_send_type_handle(payload_len);

    return SUCCESS;
}

unsigned char mcu_dp_fault_update(unsigned char dp_id, unsigned long dp_data)
{
    return mcu_dp_bitmap_update(dp_id, dp_data);
}

static void __dp_send_type_handle(unsigned short payload_len)
{
    switch (g_dp_send_type) {
        case DP_SEND_TYPE_NOT_SEND: {
            PRINT_DEBUG("[OK] dp msg not send.\r\n");
            break;
        }

        case DP_SEND_TYPE_RESPOND: {
            uart_framing_fill_protocol_field(UART_CMD_RESPOND_DP, payload_len, NULL);
            uart_send_frame(payload_len);
            PRINT_DEBUG("[OK] dp msg respond.\r\n");
            break;
        }

        case DP_SEND_TYPE_REPORT_LINKAGE: {
            uart_framing_fill_protocol_field(UART_CMD_REPORT_DP_LINKAGE, payload_len, NULL);
            uart_send_frame(payload_len);
            PRINT_DEBUG("[OK] dp msg report with linkage.\r\n");
            break;
        }

        case DP_SEND_TYPE_REPORT_NOT_LINKAGE: {
            uart_framing_fill_protocol_field(UART_CMD_REPORT_DP_NOT_LINKAGE, payload_len, NULL);
            uart_send_frame(payload_len);
            PRINT_DEBUG("[OK] dp msg report without linkage.\r\n");
            break;
        }

        case DP_SEND_TYPE_SEND_BROADCAST: {
            uart_framing_fill_protocol_field(UART_CMD_SEND_DP_BROADCAST, payload_len, NULL);
            uart_send_frame(payload_len);
            PRINT_DEBUG("[OK] dp msg send broadcast.\r\n");
            break;
        }

#if (DEVICE_TYPE == SCENE_SWITCH_DEVICE)
        case DP_SEND_TYPE_SEND_GROUP: {
            uart_framing_fill_payload_add_group_id(&payload_len);      // group id
            uart_framing_fill_protocol_field(UART_CMD_SEND_DP_GROUP, payload_len, NULL);
            uart_send_frame(payload_len);
            PRINT_DEBUG("[OK] dp msg send to group.\r\n");
            break;
        }
#endif
        
        default: {
            PRINT_DEBUG("[ERROR] dp msg send type not found.\r\n");
            break;
        }
    }
}

/*----------------------------------------------------------
 *              dp msg received handle function
 * 1. please complete these functions to handle the received dp data.
 *--------------------------------------------------------*/
//< USER_CHECK_MSG
/*****************************************************************************
函数名称 : dp_download_scene_1_handle
功能描述 : 针对DPID_SCENE_1的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_scene_1_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为ENUM
    unsigned char ret;
    unsigned char scene_1;
    
    scene_1 = mcu_get_dp_download_enum(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SCENE_1", scene_1);
#endif
    switch(scene_1) {
        case 0:
        break;
        
        default:
    
        break;
    }
    
    //There should be a report after processing the DP
    ret = mcu_dp_enum_update(DPID_SCENE_1, scene_1);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_scene_2_handle
功能描述 : 针对DPID_SCENE_2的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_scene_2_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为ENUM
    unsigned char ret;
    unsigned char scene_2;
    
    scene_2 = mcu_get_dp_download_enum(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SCENE_2", scene_2);
#endif
    switch(scene_2) {
        case 0:
        break;
        
        default:
    
        break;
    }
    
    //There should be a report after processing the DP
    ret = mcu_dp_enum_update(DPID_SCENE_2, scene_2);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_scene_3_handle
功能描述 : 针对DPID_SCENE_3的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_scene_3_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为ENUM
    unsigned char ret;
    unsigned char scene_3;
    
    scene_3 = mcu_get_dp_download_enum(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SCENE_3", scene_3);
#endif
    switch(scene_3) {
        case 0:
        break;
        
        default:
    
        break;
    }
    
    //There should be a report after processing the DP
    ret = mcu_dp_enum_update(DPID_SCENE_3, scene_3);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_scene_4_handle
功能描述 : 针对DPID_SCENE_4的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_scene_4_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为ENUM
    unsigned char ret;
    unsigned char scene_4;
    
    scene_4 = mcu_get_dp_download_enum(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SCENE_4", scene_4);
#endif
    switch(scene_4) {
        case 0:
        break;
        
        default:
    
        break;
    }
    
    //There should be a report after processing the DP
    ret = mcu_dp_enum_update(DPID_SCENE_4, scene_4);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_scene_5_handle
功能描述 : 针对DPID_SCENE_5的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_scene_5_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为ENUM
    unsigned char ret;
    unsigned char scene_5;
    
    scene_5 = mcu_get_dp_download_enum(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SCENE_5", scene_5);
#endif
    switch(scene_5) {
        case 0:
        break;
        
        default:
    
        break;
    }
    
    //There should be a report after processing the DP
    ret = mcu_dp_enum_update(DPID_SCENE_5, scene_5);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_scene_6_handle
功能描述 : 针对DPID_SCENE_6的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_scene_6_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为ENUM
    unsigned char ret;
    unsigned char scene_6;
    
    scene_6 = mcu_get_dp_download_enum(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SCENE_6", scene_6);
#endif
    switch(scene_6) {
        case 0:
        break;
        
        default:
    
        break;
    }
    
    //There should be a report after processing the DP
    ret = mcu_dp_enum_update(DPID_SCENE_6, scene_6);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_scene_7_handle
功能描述 : 针对DPID_SCENE_7的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_scene_7_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为ENUM
    unsigned char ret;
    unsigned char scene_7;
    
    scene_7 = mcu_get_dp_download_enum(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SCENE_7", scene_7);
#endif
    switch(scene_7) {
        case 0:
        break;
        
        default:
    
        break;
    }
    
    //There should be a report after processing the DP
    ret = mcu_dp_enum_update(DPID_SCENE_7, scene_7);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_scene_8_handle
功能描述 : 针对DPID_SCENE_8的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_scene_8_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为ENUM
    unsigned char ret;
    unsigned char scene_8;
    
    scene_8 = mcu_get_dp_download_enum(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SCENE_8", scene_8);
#endif
    switch(scene_8) {
        case 0:
        break;
        
        default:
    
        break;
    }
    
    //There should be a report after processing the DP
    ret = mcu_dp_enum_update(DPID_SCENE_8, scene_8);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_switch_1_handle
功能描述 : 针对DPID_SWITCH_1的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_switch_1_handle(const unsigned char value[], unsigned short length)
{
    unsigned char ret;
    unsigned char switch_1;
    
    switch_1 = mcu_get_dp_download_bool(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SWITCH_1", switch_1);
#endif
    
    ret = mcu_dp_bool_update(DPID_SWITCH_1, switch_1);
    if (ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_switch_2_handle
功能描述 : 针对DPID_SWITCH_2的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_switch_2_handle(const unsigned char value[], unsigned short length)
{
    unsigned char ret;
    unsigned char switch_2;
    
    switch_2 = mcu_get_dp_download_bool(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SWITCH_2", switch_2);
#endif
    
    ret = mcu_dp_bool_update(DPID_SWITCH_2, switch_2);
    if (ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_switch_3_handle
功能描述 : 针对DPID_SWITCH_3的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_switch_3_handle(const unsigned char value[], unsigned short length)
{
    unsigned char ret;
    unsigned char switch_3;
    
    switch_3 = mcu_get_dp_download_bool(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SWITCH_3", switch_3);
#endif
    
    ret = mcu_dp_bool_update(DPID_SWITCH_3, switch_3);
    if (ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_switch_4_handle
功能描述 : 针对DPID_SWITCH_4的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_switch_4_handle(const unsigned char value[], unsigned short length)
{
    unsigned char ret;
    unsigned char switch_4;
    
    switch_4 = mcu_get_dp_download_bool(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SWITCH_4", switch_4);
#endif
    
    ret = mcu_dp_bool_update(DPID_SWITCH_4, switch_4);
    if (ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_switch_5_handle
功能描述 : 针对DPID_SWITCH_5的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_switch_5_handle(const unsigned char value[], unsigned short length)
{
    unsigned char ret;
    unsigned char switch_5;
    
    switch_5 = mcu_get_dp_download_bool(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SWITCH_5", switch_5);
#endif
    
    ret = mcu_dp_bool_update(DPID_SWITCH_5, switch_5);
    if (ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_switch_6_handle
功能描述 : 针对DPID_SWITCH_6的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_switch_6_handle(const unsigned char value[], unsigned short length)
{
    unsigned char ret;
    unsigned char switch_6;
    
    switch_6 = mcu_get_dp_download_bool(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SWITCH_6", switch_6);
#endif
    
    ret = mcu_dp_bool_update(DPID_SWITCH_6, switch_6);
    if (ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_fan_direction_handle
功能描述 : 针对DPID_FAN_DIRECTION的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_fan_direction_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为ENUM
    unsigned char ret;
    unsigned char fan_direction;
    
    fan_direction = mcu_get_dp_download_enum(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "FAN_DIRECTION", fan_direction);
#endif
    switch(fan_direction) {
        case 0:
        break;
        
        case 1:
        break;
        
        case 2:
        break;
        
        default:
    
        break;
    }
    
    //There should be a report after processing the DP
    ret = mcu_dp_enum_update(DPID_FAN_DIRECTION, fan_direction);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_fan_speed_enum_handle
功能描述 : 针对DPID_FAN_SPEED_ENUM的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_fan_speed_enum_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为ENUM
    unsigned char ret;
    unsigned char fan_speed_enum;
    
    fan_speed_enum = mcu_get_dp_download_enum(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "FAN_SPEED_ENUM", fan_speed_enum);
#endif
    switch(fan_speed_enum) {
        case 0:
        break;
        
        case 1:
        break;
        
        case 2:
        break;
        
        case 3:
        break;
        
        default:
    
        break;
    }
    
    //There should be a report after processing the DP
    ret = mcu_dp_enum_update(DPID_FAN_SPEED_ENUM, fan_speed_enum);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_mode_handle
功能描述 : 针对DPID_MODE的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_mode_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为ENUM
    unsigned char ret;
    unsigned char mode;
    
    mode = mcu_get_dp_download_enum(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "MODE", mode);
#endif
    switch(mode) {
        case 0:
        break;
        
        case 1:
        break;
        
        case 2:
        break;
        
        case 3:
        break;
        
        case 4:
        break;
        
        default:
    
        break;
    }
    
    //There should be a report after processing the DP
    ret = mcu_dp_enum_update(DPID_MODE, mode);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_temp_set_handle
功能描述 : 针对DPID_TEMP_SET的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_temp_set_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为VALUE
    unsigned char ret;
    unsigned long temp_set;
    
    temp_set = mcu_get_dp_download_value(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "TEMP_SET", temp_set);
#endif
    /*
    //VALUE type data processing
    
    */
    
    //There should be a report after processing the DP
    ret = mcu_dp_value_update(DPID_TEMP_SET,temp_set);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_switch_handle
功能描述 : 针对DPID_SWITCH的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_switch_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为BOOL
    unsigned char ret;
    //0:off/1:on
    unsigned char switch_1;
    
    switch_1 = mcu_get_dp_download_bool(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SWITCH", switch_1);
#endif
    if(switch_1 == 0) {
        //bool off
    }else {
        //bool on
    }
  
    //There should be a report after processing the DP
    ret = mcu_dp_bool_update(DPID_SWITCH,switch_1);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_loop_mode_handle
功能描述 : 针对DPID_LOOP_MODE的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_loop_mode_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为ENUM
    unsigned char ret;
    unsigned char loop_mode;
    
    loop_mode = mcu_get_dp_download_enum(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "LOOP_MODE", loop_mode);
#endif
    switch(loop_mode) {
        case 0:
        break;
        
        case 1:
        break;
        
        case 2:
        break;
        
        default:
    
        break;
    }
    
    //There should be a report after processing the DP
    ret = mcu_dp_enum_update(DPID_LOOP_MODE, loop_mode);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_supply_fan_speed_handle
功能描述 : 针对DPID_SUPPLY_FAN_SPEED的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_supply_fan_speed_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为ENUM
    unsigned char ret;
    unsigned char supply_fan_speed;
    
    supply_fan_speed = mcu_get_dp_download_enum(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SUPPLY_FAN_SPEED", supply_fan_speed);
#endif
    switch(supply_fan_speed) {
        case 0:
        break;
        
        case 1:
        break;
        
        case 2:
        break;
        
        case 3:
        break;
        
        default:
    
        break;
    }
    
    //There should be a report after processing the DP
    ret = mcu_dp_enum_update(DPID_SUPPLY_FAN_SPEED, supply_fan_speed);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_exhaust_fan_speed_handle
功能描述 : 针对DPID_EXHAUST_FAN_SPEED的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_exhaust_fan_speed_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为ENUM
    unsigned char ret;
    unsigned char exhaust_fan_speed;
    
    exhaust_fan_speed = mcu_get_dp_download_enum(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "EXHAUST_FAN_SPEED", exhaust_fan_speed);
#endif
    switch(exhaust_fan_speed) {
        case 0:
        break;
        
        case 1:
        break;
        
        case 2:
        break;
        
        case 3:
        break;
        
        default:
    
        break;
    }
    
    //There should be a report after processing the DP
    ret = mcu_dp_enum_update(DPID_EXHAUST_FAN_SPEED, exhaust_fan_speed);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_fresh_air_valve_handle
功能描述 : 针对DPID_FRESH_AIR_VALVE的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_fresh_air_valve_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为BOOL
    unsigned char ret;
    //0:off/1:on
    unsigned char fresh_air_valve;
    
    fresh_air_valve = mcu_get_dp_download_bool(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "FRESH_AIR_VALVE", fresh_air_valve);
#endif
    if(fresh_air_valve == 0) {
        //bool off
    }else {
        //bool on
    }
  
    //There should be a report after processing the DP
    ret = mcu_dp_bool_update(DPID_FRESH_AIR_VALVE,fresh_air_valve);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_floor_sw_handle
功能描述 : 针对DPID_FLOOR_SW的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_floor_sw_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为BOOL
    unsigned char ret;
    //0:off/1:on
    unsigned char floor_sw;
    
    floor_sw = mcu_get_dp_download_bool(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "FLOOR_SW", floor_sw);
#endif
    if(floor_sw == 0) {
        //bool off
    }else {
        //bool on
    }
  
    //There should be a report after processing the DP
    ret = mcu_dp_bool_update(DPID_FLOOR_SW,floor_sw);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_floor_temp_handle
功能描述 : 针对DPID_FLOOR_TEMP的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_floor_temp_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为VALUE
    unsigned char ret;
    unsigned long floor_temp;
    
    floor_temp = mcu_get_dp_download_value(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "FLOOR_TEMP", floor_temp);
#endif
    /*
    //VALUE type data processing
    
    */
    
    //There should be a report after processing the DP
    ret = mcu_dp_value_update(DPID_FLOOR_TEMP,floor_temp);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_heat_limit_handle
功能描述 : 针对DPID_HEAT_LIMIT的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_heat_limit_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为RAW
    unsigned char ret;
    /*
    //RAW type data processing
    
    */
    
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s raw_data[%u]\r\n", "HEAT_LIMIT", length);
#endif
    //There should be a report after processing the DP
    ret = mcu_dp_raw_update(DPID_HEAT_LIMIT,value,length);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_ac_limit_handle
功能描述 : 针对DPID_AC_LIMIT的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_ac_limit_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为RAW
    unsigned char ret;
    /*
    //RAW type data processing
    
    */
    
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s raw_data[%u]\r\n", "AC_LIMIT", length);
#endif
    //There should be a report after processing the DP
    ret = mcu_dp_raw_update(DPID_AC_LIMIT,value,length);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_ac_info_handle
功能描述 : 针对DPID_AC_INFO的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_ac_info_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为RAW
    unsigned char ret;
    /*
    //RAW type data processing
    
    */
    
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s raw_data[%u]\r\n", "AC_INFO", length);
#endif
    //There should be a report after processing the DP
    ret = mcu_dp_raw_update(DPID_AC_INFO,value,length);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_fan_info_handle
功能描述 : 针对DPID_FAN_INFO的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_fan_info_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为RAW
    unsigned char ret;
    /*
    //RAW type data processing
    
    */
    
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s raw_data[%u]\r\n", "FAN_INFO", length);
#endif
    //There should be a report after processing the DP
    ret = mcu_dp_raw_update(DPID_FAN_INFO,value,length);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_floor_info_handle
功能描述 : 针对DPID_FLOOR_INFO的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_floor_info_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为RAW
    unsigned char ret;
    /*
    //RAW type data processing
    
    */
    
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s raw_data[%u]\r\n", "FLOOR_INFO", length);
#endif
    //There should be a report after processing the DP
    ret = mcu_dp_raw_update(DPID_FLOOR_INFO,value,length);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_switch_7_handle
功能描述 : 针对DPID_SWITCH_7的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_switch_7_handle(const unsigned char value[], unsigned short length)
{
    unsigned char ret;
    unsigned char switch_7;
    
    switch_7 = mcu_get_dp_download_bool(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SWITCH_7", switch_7);
#endif
    
    ret = mcu_dp_bool_update(DPID_SWITCH_7, switch_7);
    if (ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_switch_8_handle
功能描述 : 针对DPID_SWITCH_8的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_switch_8_handle(const unsigned char value[], unsigned short length)
{
    unsigned char ret;
    unsigned char switch_8;
    
    switch_8 = mcu_get_dp_download_bool(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "SWITCH_8", switch_8);
#endif
    
    ret = mcu_dp_bool_update(DPID_SWITCH_8, switch_8);
    if (ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_key_mode_handle
功能描述 : 针对DPID_KEY_MODE的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_key_mode_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为RAW
    unsigned char ret;
    /*
    //RAW type data processing
    
    */
    
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s raw_data[%u]\r\n", "KEY_MODE", length);
#endif
    //There should be a report after processing the DP
    ret = mcu_dp_raw_update(DPID_KEY_MODE,value,length);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_page_mgr_handle
功能描述 : 针对DPID_PAGE_MGR的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_page_mgr_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为RAW
    unsigned char ret;
    /*
    //RAW type data processing
    
    */
    
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s raw_data[%u]\r\n", "PAGE_MGR", length);
#endif
    //There should be a report after processing the DP
    ret = mcu_dp_raw_update(DPID_PAGE_MGR,value,length);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_jog_time_handle
功能描述 : 针对DPID_JOG_TIME的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_jog_time_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为RAW
    unsigned char ret;
    /*
    //RAW type data processing
    
    */
    
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s raw_data[%u]\r\n", "JOG_TIME", length);
#endif
    //There should be a report after processing the DP
    ret = mcu_dp_raw_update(DPID_JOG_TIME,value,length);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_base_set_handle
功能描述 : 针对DPID_BASE_SET的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_base_set_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为RAW
    unsigned char ret;
    /*
    //RAW type data processing
    
    */
    
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s raw_data[%u]\r\n", "BASE_SET", length);
#endif
    //There should be a report after processing the DP
    ret = mcu_dp_raw_update(DPID_BASE_SET,value,length);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_adv_set_handle
功能描述 : 针对DPID_ADV_SET的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_adv_set_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为RAW
    unsigned char ret;
    /*
    //RAW type data processing
    
    */
    
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s raw_data[%u]\r\n", "ADV_SET", length);
#endif
    //There should be a report after processing the DP
    ret = mcu_dp_raw_update(DPID_ADV_SET,value,length);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_theme_handle
功能描述 : 针对DPID_THEME的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_theme_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为ENUM
    unsigned char ret;
    unsigned char theme;
    
    theme = mcu_get_dp_download_enum(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "THEME", theme);
#endif
    switch(theme) {
        case 0:
        break;
        
        default:
    
        break;
    }
    
    //There should be a report after processing the DP
    ret = mcu_dp_enum_update(DPID_THEME, theme);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_key_name_handle
功能描述 : 针对DPID_KEY_NAME的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_key_name_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为RAW
    unsigned char ret;
    /*
    //RAW type data processing
    
    */
    
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s raw_data[%u]\r\n", "KEY_NAME", length);
#endif
    //There should be a report after processing the DP
    ret = mcu_dp_raw_update(DPID_KEY_NAME,value,length);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_child_lock_handle
功能描述 : 针对DPID_CHILD_LOCK的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_child_lock_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为BOOL
    unsigned char ret;
    //0:off/1:on
    unsigned char child_lock;
    
    child_lock = mcu_get_dp_download_bool(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "CHILD_LOCK", child_lock);
#endif
    if(child_lock == 0) {
        //bool off
    }else {
        //bool on
    }
  
    //There should be a report after processing the DP
    ret = mcu_dp_bool_update(DPID_CHILD_LOCK,child_lock);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_disp_param_handle
功能描述 : 针对DPID_DISP_PARAM的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_disp_param_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为RAW
    unsigned char ret;
    /*
    //RAW type data processing
    
    */
    
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s raw_data[%u]\r\n", "DISP_PARAM", length);
#endif
    //There should be a report after processing the DP
    ret = mcu_dp_raw_update(DPID_DISP_PARAM,value,length);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_page_sync_handle
功能描述 : 针对DPID_PAGE_SYNC的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_page_sync_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为ENUM
    unsigned char ret;
    unsigned char page_sync;
    
    page_sync = mcu_get_dp_download_enum(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "PAGE_SYNC", page_sync);
#endif
    switch(page_sync) {
        case 0:
        break;
        
        case 1:
        break;
        
        case 2:
        break;
        
        case 3:
        break;
        
        case 4:
        break;
        
        default:
    
        break;
    }
    
    //There should be a report after processing the DP
    ret = mcu_dp_enum_update(DPID_PAGE_SYNC, page_sync);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_master_sw_handle
功能描述 : 针对DPID_MASTER_SW的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_master_sw_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为BOOL
    unsigned char ret;
    //0:off/1:on
    unsigned char master_sw;
    
    master_sw = mcu_get_dp_download_bool(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "MASTER_SW", master_sw);
#endif
    if(master_sw == 0) {
        //bool off
    }else {
        //bool on
    }
  
    //There should be a report after processing the DP
    ret = mcu_dp_bool_update(DPID_MASTER_SW,master_sw);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_fresh_air_speed_handle
功能描述 : 针对DPID_FRESH_AIR_SPEED的处理函数
输入参数 : value:数据源数据
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : 可下发可上报类型,需要在处理完数据后上报处理结果至app
*****************************************************************************/
static unsigned char dp_download_fresh_air_speed_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为ENUM
    unsigned char ret;
    unsigned char fresh_air_speed;
    
    fresh_air_speed = mcu_get_dp_download_enum(value,length);
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE_RESULT: %s val=%u\r\n", "FRESH_AIR_SPEED", fresh_air_speed);
#endif
    switch(fresh_air_speed) {
        case 0:
        break;
        
        case 1:
        break;
        
        case 2:
        break;
        
        default:
    
        break;
    }
    
    //There should be a report after processing the DP
    ret = mcu_dp_enum_update(DPID_FRESH_AIR_SPEED, fresh_air_speed);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}

//< USER_CHECK_MSG END
static unsigned char dp_msg_handle(unsigned char dp_id, const unsigned char *value, unsigned short length)
{
    unsigned char ret;
    if (NULL == value) {
        return ERROR;
    }
    
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_HANDLE: %s data[%u]=", tuya_get_dp_name(dp_id), length);
    for (unsigned short _i = 0; _i < length; _i++) {
        TUYA_LOG_INFO("%02X ", value[_i]);
    }
    TUYA_LOG_INFO("\r\n");
#endif
    
    switch (dp_id) {
        case DPID_SCENE_1:
            //场景1处理函数
            ret = dp_download_scene_1_handle(value,length);
        break;
        case DPID_SCENE_2:
            //场景2处理函数
            ret = dp_download_scene_2_handle(value,length);
        break;
        case DPID_SCENE_3:
            //场景3处理函数
            ret = dp_download_scene_3_handle(value,length);
        break;
        case DPID_SCENE_4:
            //场景4处理函数
            ret = dp_download_scene_4_handle(value,length);
        break;
        case DPID_SCENE_5:
            //场景5处理函数
            ret = dp_download_scene_5_handle(value,length);
        break;
        case DPID_SCENE_6:
            //场景6处理函数
            ret = dp_download_scene_6_handle(value,length);
        break;
        case DPID_SCENE_7:
            //场景7处理函数
            ret = dp_download_scene_7_handle(value,length);
        break;
        case DPID_SCENE_8:
            //场景8处理函数
            ret = dp_download_scene_8_handle(value,length);
        break;
        case DPID_SWITCH_1:
            //继电器1处理函数
            ret = dp_download_switch_1_handle(value,length);
        break;
        case DPID_SWITCH_2:
            //继电器2处理函数
            ret = dp_download_switch_2_handle(value,length);
        break;
        case DPID_SWITCH_3:
            //继电器3处理函数
            ret = dp_download_switch_3_handle(value,length);
        break;
        case DPID_SWITCH_4:
            //继电器4处理函数
            ret = dp_download_switch_4_handle(value,length);
        break;
        case DPID_SWITCH_5:
            //开关5处理函数
            ret = dp_download_switch_5_handle(value,length);
        break;
        case DPID_SWITCH_6:
            //开关6处理函数
            ret = dp_download_switch_6_handle(value,length);
        break;
        case DPID_FAN_DIRECTION:
            //风向处理函数
            ret = dp_download_fan_direction_handle(value,length);
        break;
        case DPID_FAN_SPEED_ENUM:
            //风速（枚举）处理函数
            ret = dp_download_fan_speed_enum_handle(value,length);
        break;
        case DPID_MODE:
            //模式处理函数
            ret = dp_download_mode_handle(value,length);
        break;
        case DPID_TEMP_SET:
            //温度设置处理函数
            ret = dp_download_temp_set_handle(value,length);
        break;
        case DPID_SWITCH:
            //开关处理函数
            ret = dp_download_switch_handle(value,length);
        break;
        case DPID_LOOP_MODE:
            //循环模式处理函数
            ret = dp_download_loop_mode_handle(value,length);
        break;
        case DPID_SUPPLY_FAN_SPEED:
            //送风风速处理函数
            ret = dp_download_supply_fan_speed_handle(value,length);
        break;
        case DPID_EXHAUST_FAN_SPEED:
            //排风风速处理函数
            ret = dp_download_exhaust_fan_speed_handle(value,length);
        break;
        case DPID_FRESH_AIR_VALVE:
            //新风阀处理函数
            ret = dp_download_fresh_air_valve_handle(value,length);
        break;
        case DPID_FLOOR_SW:
            //地暖开关处理函数
            ret = dp_download_floor_sw_handle(value,length);
        break;
        case DPID_FLOOR_TEMP:
            //地暖温度处理函数
            ret = dp_download_floor_temp_handle(value,length);
        break;
        case DPID_HEAT_LIMIT:
            //地暖温度上下限设置处理函数
            ret = dp_download_heat_limit_handle(value,length);
        break;
        case DPID_AC_LIMIT:
            //空调温度上下限设置处理函数
            ret = dp_download_ac_limit_handle(value,length);
        break;
        case DPID_AC_INFO:
            //空调信息处理函数
            ret = dp_download_ac_info_handle(value,length);
        break;
        case DPID_FAN_INFO:
            //新风信息处理函数
            ret = dp_download_fan_info_handle(value,length);
        break;
        case DPID_FLOOR_INFO:
            //地暖信息处理函数
            ret = dp_download_floor_info_handle(value,length);
        break;
        case DPID_SWITCH_7:
            //开关7处理函数
            ret = dp_download_switch_7_handle(value,length);
        break;
        case DPID_SWITCH_8:
            //开关8处理函数
            ret = dp_download_switch_8_handle(value,length);
        break;
        case DPID_KEY_MODE:
            //按键模式处理函数
            ret = dp_download_key_mode_handle(value,length);
        break;
        case DPID_PAGE_MGR:
            //页面管理处理函数
            ret = dp_download_page_mgr_handle(value,length);
        break;
        case DPID_JOG_TIME:
            //点动时间处理函数
            ret = dp_download_jog_time_handle(value,length);
        break;
        case DPID_BASE_SET:
            //基础设置功能处理函数
            ret = dp_download_base_set_handle(value,length);
        break;
        case DPID_ADV_SET:
            //高级设置功能处理函数
            ret = dp_download_adv_set_handle(value,length);
        break;
        case DPID_THEME:
            //主题切换处理函数
            ret = dp_download_theme_handle(value,length);
        break;
        case DPID_KEY_NAME:
            //按键名称处理函数
            ret = dp_download_key_name_handle(value,length);
        break;
        case DPID_CHILD_LOCK:
            //童锁处理函数
            ret = dp_download_child_lock_handle(value,length);
        break;
        case DPID_DISP_PARAM:
            //展示参数处理函数
            ret = dp_download_disp_param_handle(value,length);
        break;
        case DPID_PAGE_SYNC:
            //页面同步处理函数
            ret = dp_download_page_sync_handle(value,length);
        break;
        case DPID_MASTER_SW:
            //总开总关处理函数
            ret = dp_download_master_sw_handle(value,length);
        break;
        case DPID_FRESH_AIR_SPEED:
            //新风风速处理函数
            ret = dp_download_fresh_air_speed_handle(value,length);
        break;

        default :
            break;
    }
    return ret;
}

/*----------------------------------------------------------
 * The module needs to query the DP status of the MCU at certain times 
 * (such as during a reboot or re-configuration of the network), so the 
 * function of reporting DP status by the MCU must be implemented in 
 * cooperation with the user.
 * 
 * How to pass parameters to the reporting function of the DP data?
 * |:--------:|:--------------:|:-------------------------------:|
 * | DP Type  |   Data Type    |      Reporting Function         |
 * |:--------:|:--------------:|:-------------------------------:|
 * | bool     | Boolean        | mcu_dp_bool_update()            |
 * | enum     | Enumeration    | mcu_dp_enum_update()            |
 * | value    | Value          | mcu_dp_value_update()           |
 * | string   | String         | mcu_dp_string_update()          |
 * | raw      | Raw Data       | mcu_dp_raw_update()             | 
 * | fault    | Bitmap         | mcu_dp_bitmap_update()          |
 * |:--------:|:--------------:|:-------------------------------:|
 * bool DP: 
 * --------------------------------------------------------
 * Usually, it refers to the switch type DP, such as switches, 
 * ECO and display screens.
 * ex: 
 * If the user has a switch-type DP with a DPID of DPID_SWITCH, 
 * this DP has only two values, 0 and 1. 0 means off and 1 means on.
 * 'mcu_dp_bool_update(DPID_SWITCH, 1)' // this means to report the switch DP with value "1" (ON).
 * --------------------------------------------------------
 * 
 * enum DP:
 * --------------------------------------------------------
 * It is usually used as a DP with multiple states, such as working mode, 
 * wind speed and wind swing position.
 * ex:
 * If the user has a tricolor light DP with a DPID of DPID_LIGHT_COLOR, 
 * this DP has only three values. 0 means red、1 means green and 2 means blue. 
 * 'mcu_dp_enum_update(DPID_MODE, 2)' // this means to report the light DP with value "2" (blue light).
 * --------------------------------------------------------
 * 
 * value DP: 
 * --------------------------------------------------------
 * It is usually used as a DP with a range of values, such as battery 
 * percentage.
 * ex:
 * if the user has a battery percentage DP with a DPID of DPID_BATTERY_PERCENTAGE,
 * the value range of this DP is 0~100.
 * 'mcu_dp_value_update(DPID_BATTERY_PERCENTAGE, 75)' // this means to report the battery percentage DP with value "75%".
 * --------------------------------------------------------
 * 
 * fault DP:
 * --------------------------------------------------------
 * It is usually used for reporting faults, and the data is typically displayed in bitmap format.
 * ex:
 * If the user has a fault DP with a DPID of DPID_FAULT, bit 0 indicates a motor fault, 
 * bit 1 indicates a temperature fault.
 * 'mcu_dp_fault_update(DPID_FAULT, 0x03)' // this means to report both motor fault and temperature fault.
 * --------------------------------------------------------
 * 
 * string DP & raw DP:
 * --------------------------------------------------------
 * These two DP will not be elaborated on. The actual parameters passed are pointers and lengths. In essence,
 *  it is to transfer a piece of data. The data represents what the user and the platform have directly set 
 * in advance.
 * 
 *--------------------------------------------------------*/

static void all_data_update(void)
{
    //< USER_CHECK_MSG
    // TODO: fill in actual DP data to report on boot/network reconfigure
    // Example: mcu_dp_enum_update(DPID_SCENE_1, current_scene_1);
    (void)0;
    //< USER_CHECK_MSG END
}

static void specific_dp_update(unsigned char dp_id)
{
    switch (dp_id) {
        /** please refer the following example and 'all_data_update()'
         * 
         *     case DPID_SCENE_1: {
         *         mcu_dp_enum_update(DPID_SCENE_1,当前场景1); //枚举型数据上报;
         *         break;
         *     }
         * 
         *     case DPID_SCENE_2: {
         *         mcu_dp_enum_update(DPID_SCENE_2,当前场景2); //枚举型数据上报;
         *         break;
         *     }
         */
        default: {
            break;
        }
    }
}
/* -END OF FILE-  */
