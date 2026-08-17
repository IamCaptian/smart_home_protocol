/***********************************************************
 * @file     mcu_api.c
 * @brief    function define:   MCU SDK API PORT,User is 
 *           required to modify this file according to 
 *           the instructions.
 * @version  3.3.5
 * @date     2026.04.08
 * @copyright Copyright (c) tuya.inc 2024
 * 
 * Transplant Instructions 
 * 
 *  ———————————————————————————————README————————————————————————————————————
 * | User just need to change <mcu_api.c> <mcu_api.h>;Tuya developer platform|
 * | will automatically generate other source files;Users should try not to  |
 * | change other source files;                                              |
 * | User must complete the UART function of their own chip platform,user    |
 * | needs to complete four parts of work to implement the Tuya Zigbee uart  |
 * | protocol;                                                               |
 * | reference address:                                                      |
 * | https://developer.tuya.com/cn/docs/mcu-standard-protocol                |
 *  —————————————————————————————————————————————————————————————————————————
 * 
 * 
 * Part 1. uart data receive part;
 * —————————————————————————————————————————————————————————————————————————————
 * This part of the user needs to transfer the data received by the 
 * UART in a single-byte format into the function "uart_servive_rx_store()";
 * Then the user needs to periodically call the function "uart_service_parse()";
 * —————————————————————————————————————————————————————————————————————————————
 * 
 * 
 * Part 2. wakeup method part;
 * —————————————————————————————————————————————————————————————————————————————
 * “DEVICE_TYPE = ROUTER_DEVICE”:
 *     a. Users can ignore this part;
 * “DEVICE_TYPE = SCENE_SWITCH_DEVICE”:
 *     a. Users can ignore this part;
 * "DEVICE_TYPE = SLEEP_END_DEVICE":
 *     mcu wakeup zigbee module has two methods: low-level wakeup or low-level pulse;
 *     "MCU_WAKEUP_MODULE_METHOD == LOW_LEVEL_WAKE_UP":
 *         a. Related parameters and functions:,
 *            "g_wakeup_stay_time",
 *            "g_mcu_wait_zg_time",
 *            "mcu_wakeup_zg_level_method()";
 *         b. Please refer to "mcu_wakeup_zg_level_method()" note;
 *     "MCU_WAKEUP_MODULE_METHOD == LOW_PULSE_WAKE_UP":
 *         a. Related parameters and functions:
 *            "g_wakeup_stay_time",
 *            "g_mcu_wait_zg_time",
 *            "mcu_wakeup_zg_pulse_method()";
 *         b. Please refer to "mcu_wakeup_zg_pulse_method()" note;
 *     zigbee module wakeup mcu only has one method: low-level;
 *         a. Related parameters and functions:
 *            "g_zg_wait_mcu_time";
 * —————————————————————————————————————————————————————————————————————————————
 * 
 * 
 * Part 3. uart data send part;
 * —————————————————————————————————————————————————————————————————————————————
 * This part of the user needs to transfer the data send by the 
 * UART in a single-byte format into the function "uart_send_byte()";
* —————————————————————————————————————————————————————————————————————————————
 * 
 * 
 * Part 4. frame receive handle part;
 * —————————————————————————————————————————————————————————————————————————————
 * This part of the user needs to complete the frame receive handle
 * —————————————————————————————————————————————————————————————————————————————
 * 
 * 
 * Part 5. check device info part;
 * —————————————————————————————————————————————————————————————————————————————
 * This part of the user needs to check development device info, 
 * Users can search for the keyword "USER_CHECK_MSG" to check;
 *     a. check user product key, such as:        #define PRODUCT_KEY "xn7xzluu"
 *     b. check user dp id define, such as:       #define DPID_SWITCH_1 1...
 *     c. check user dp id list, such as:         const DOWNLOAD_CMD_S download_cmd[] = {...};
 *     d. check user dp handle function, such as: case DPID_SWITCH_1:...
 *     e. user achieves the reporting function of DP through the two functions 
 *        'all_data_update()' and 'specific_dp_update()';
 * —————————————————————————————————————————————————————————————————————————————
 **********************************************************/

#include "tuya_protocol_uart.h"
#include "zigbee.h"
#include "hooch_setting.h"
#include "tuya_device_scene.h"
#include "tuya_device_switch.h"
#include "tuya_device_air.h"
#include "tuya_device_floor.h"
#include "tuya_device_freshair.h"
#include "tuya_device_key.h"
#include "tuya_basic_setting.h"

/***********************************************************
 * Macro Definitions
 **********************************************************/


/***********************************************************
 * Typedef Definitions
 **********************************************************/


/***********************************************************
 * Variable Declarations
 **********************************************************/

extern volatile unsigned char g_uart_tx_buf[UART_TX_BUF_LEN_LMT];
extern volatile unsigned char g_uart_rx_buf[UART_RX_BUF_LEN_LMT];

extern DP_SEND_TYPE_E g_dp_send_type;

/***********************************************************
 * Variable Definitions
 **********************************************************/


/***********************************************************
 * Function Declarations
 **********************************************************/


/***********************************************************
 * Function Definitions
 **********************************************************/
void uart_servive_rx_store(unsigned char value)
{
    queue_enqueue_byte(value);
}

void uart_service_parse(void)
{
    unsigned char temp = 0;

    static unsigned short s_uart_rx_buf_end_offset = 0;
    unsigned short cur_frame_start_offset = 0;

    unsigned short expect_payload_len = 0;
    unsigned short expect_frame_len = 0;
    unsigned char expect_checksum = 0;
    unsigned char cur_checksum = 0;

    ///< 1. dequeue 'whole' queue_buf to rx_buf (maybe more than 1 frame)
    while (s_uart_rx_buf_end_offset < sizeof(g_uart_rx_buf)) {
        if (FALSE == queue_dequeue_byte(&temp)) {
            break;
        }
        
        g_uart_rx_buf[s_uart_rx_buf_end_offset] = temp;
        s_uart_rx_buf_end_offset++;
    }

    ///< 2. check if rx_buf has at least one complete frame
    if (s_uart_rx_buf_end_offset < FRAME_LEN_WITHOUT_PAYLOAD) {
        return;
    }

    ///< 3. handle each complete frame
    while (FRAME_LEN_WITHOUT_PAYLOAD <= (s_uart_rx_buf_end_offset - cur_frame_start_offset)) {
        ///< a. check frame hdr
        if ((g_uart_rx_buf[cur_frame_start_offset + FRAME_FIELD_HDR_HIGH] != FRAME_VALUE_HDR_HIGH) || (g_uart_rx_buf[cur_frame_start_offset + FRAME_FIELD_HDR_LOW] != FRAME_VALUE_HDR_LOW)) {
            cur_frame_start_offset++;       // skip 1 byte
            continue;
        }

        ///< b. check frame ver
        if (g_uart_rx_buf[cur_frame_start_offset + FRAME_FIELD_PROT_VER] != FRAME_VALUE_PROT_VER) {
            cur_frame_start_offset += 2;    // skip 2 byte: abandon the front correct 2 bytes (hdr)
            continue;
        }

        ///< c. get expective payload len
        expect_payload_len = (unsigned short)g_uart_rx_buf[cur_frame_start_offset + FRAME_FIELD_PAYLOAD_LEN_HIGH] << 8;
        expect_payload_len += g_uart_rx_buf[cur_frame_start_offset + FRAME_FIELD_PAYLOAD_LEN_LOW];

        ///< d. get expective frame length
        expect_frame_len = expect_payload_len + FRAME_LEN_WITHOUT_PAYLOAD;

        ///< e. wrong frame check
        if (sizeof(g_uart_rx_buf) < expect_frame_len) {
            cur_frame_start_offset += 3;    // skip 3 byte: abandon the front correct 3 bytes (hdr, ver)
            continue;
        }

        ///< f. uncomplete frame check
        if ((s_uart_rx_buf_end_offset - cur_frame_start_offset) < expect_frame_len) {
            PRINT_DEBUG("[ERROR]start_offset: %d, end_offset: %d, expect_len: %d\r\n", cur_frame_start_offset, s_uart_rx_buf_end_offset, expect_frame_len);
            break;                          // skip this turn of parsing, because the frame is correct but uncomplete
                                            // in next turn of parsing, this frame will get the rest data
        }

        ///< g. checksum check
        expect_checksum = g_uart_rx_buf[cur_frame_start_offset + expect_frame_len - 1];
        if (FALSE == get_u8_checksum((unsigned char *)g_uart_rx_buf + cur_frame_start_offset, expect_frame_len - 1, &cur_checksum)) {
            return;
        }

        if (cur_checksum == expect_checksum) {
            ///< h. handle this correct complete frame
            frame_rx_handle(cur_frame_start_offset);
            ///< i. parse next frame
            cur_frame_start_offset += expect_frame_len;
        } else {
            cur_frame_start_offset += 3;    // skip 3 byte: abandon the front correct 3 bytes (hdr, ver)
            PRINT_DEBUG("[ERROR]actual_checksum: %d, expect_checksum: %d\r\n", cur_checksum, expect_checksum);
        }
    }    

    ///< 4. check the rest data, clear the useless data
    s_uart_rx_buf_end_offset -= cur_frame_start_offset;         // rest data length
    if (0 < s_uart_rx_buf_end_offset) {
        if ((cur_frame_start_offset == 0) && (s_uart_rx_buf_end_offset >= sizeof(g_uart_rx_buf))) {
            cur_frame_start_offset = 1;
            s_uart_rx_buf_end_offset--;
        }
        my_memcpy((unsigned char *)g_uart_rx_buf, (const unsigned char *)g_uart_rx_buf + cur_frame_start_offset, s_uart_rx_buf_end_offset);
    }
}

void uart_send_byte(unsigned char value)
{
    TUYA_TX_SEND(&value, 1);
}

void uart_send_bytes(unsigned char *data, unsigned short data_len)
{
    if ((NULL == data)) {
        return;
    }

    ///< 1. uart sending
    for (unsigned short i = 0; i < data_len; i++) {
        uart_send_byte(data[i]);
    }
	
	///< 2. debug print
#if 0
    PRINT_DEBUG("\r\n[MCU-HEX]");
    for (unsigned short i = 0; i < data_len; i++) {
        PRINT_DEBUG("%02x ", data[i]);
    }
    PRINT_DEBUG("[END]\r\n");

    PRINT_DEBUG("[MCU-STR]");
    for (unsigned short i = 0; i < data_len; i++) {
        PRINT_DEBUG("%c", data[i]);
    }
    PRINT_DEBUG("[END]\r\n");
#endif
}

#if (DEVICE_TYPE == SLEEP_END_DEVICE)
/* 
 * "g_zg_wait_mcu_time" depends on the user's chip platform, users should design it by themselves,
 * users can set the "g_zg_wait_mcu_time" through the function "mcu_tx_set_zg_wait_mcu_time()".
 */
#if (MCU_WAKEUP_MODULE_METHOD == LOW_LEVEL_WAKE_UP)
void mcu_wakeup_zg_level_method(unsigned char *data, unsigned short data_len)
{
    /*
    *< USER TODO: wakeup zigbee module by low-level method end send data;
    *sequence chart:
    *                         ____                                      _____
    *  mcu wakeup module pin:     |<---------------t1----------------->|
    *                             |____________________________________|
    *  mcu uart tx pin      :     |<-t0->|<-send()->|                  |
    *                             |______|__________|__________________|
    *  mcu uart rx pin      :     |                 |<-recv()->|       |
    *                             |_________________|__________|_______|
    *  t0: "g_mcu_wait_zg_time" mcu wait module wakeup time, 1-10ms, ZT module >=10ms;
    *  t1: "g_wakeup_stay_time" mcu wakeup module pin stay time, t1 must < 120s;
    *  1. pull down the mcu wakeup module pin;
    *  2. wait t0;
    *  3. uart_send_bytes() send data;
    *  4. wait uart_servive_rx_store() finish receiving data and uart_service_parse() 
    *     finish analysing data;
    *  6. pull up the mcu wakeup module pin; then the module will enter sleep mode after x ms;
    */    

}
#elif (MCU_WAKEUP_MODULE_METHOD == LOW_PULSE_WAKE_UP)
void mcu_wakeup_zg_pulse_method(unsigned char *data, unsigned short data_len)
{
    /*
    *< USER TODO: wakeup zigbee module by low-level pulse method end send data;
    *sequence chart:
    *                         ____        ________________________________
    *  mcu wakeup module pin:     |<-t0->|                             |
    *                             |______|                             |
    *  mcu uart tx pin      :     |      |<-send()->|                  |
    *                             |______|__________|__________________|
    *  mcu uart rx pin      :     |                 |<-recv()->|       |
    *                             |_________________|__________|_______|
    *  t0: "g_mcu_wait_zg_time" mcu wakeup module pin pull down stay time;ZS module [1, 5]ms, ZT module >=10ms;
    *  1. mcu wakeup module pin generate a low-level pulse lasting for t0 ms;
    *  2. uart_send_bytes() send data;
    *  3. wait uart_servive_rx_store() finish receiving data and uart_service_parse() 
    *     finish analysing data;
    */  
}
#endif
#endif

void uart_send_frame(unsigned short payload_len)
{
#if (DEVICE_TYPE == SLEEP_END_DEVICE)
#if (MCU_WAKEUP_MODULE_METHOD == LOW_LEVEL_WAKE_UP)
    mcu_wakeup_zg_level_method((unsigned char *)g_uart_tx_buf, payload_len + FRAME_LEN_WITHOUT_PAYLOAD);
#elif (MCU_WAKEUP_MODULE_METHOD == LOW_PULSE_WAKE_UP)
    mcu_wakeup_zg_pulse_method((unsigned char *)g_uart_tx_buf, payload_len + FRAME_LEN_WITHOUT_PAYLOAD);
#endif
#else
    uart_send_bytes((unsigned char *)g_uart_tx_buf, payload_len + FRAME_LEN_WITHOUT_PAYLOAD);
#endif
}

void mcu_recv_factory_recovery_cb(void)
{
    //< USER TODO
}

void mcu_recv_zg_nwk_status_notify_cb(unsigned char nwk_status)
{
    //< USER TODO
}

void mcu_send_dp_msg_cb(unsigned char ret, unsigned char dp_id, unsigned char dp_type, unsigned short dp_len, unsigned char *dp_data)
{
    if (ret != SUCCESS || dp_data == NULL) {
#if TUYA_LOG_ENABLE
        if (ret != SUCCESS) {
            TUYA_LOG_INFO("[Tuya] DP_CB: dp_id=%u FAILED ret=%u\r\n", dp_id, ret);
        }
#endif
        return;
    }

#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] DP_CB: dp_id=%u type=%u len=%u\r\n", dp_id, dp_type, dp_len);
#endif

    /* 按功能模块依次尝试分发 */
    if (tuya_dp_dispatch_scene(dp_id, dp_type, dp_len, dp_data))   return;
    if (tuya_dp_dispatch_switch(dp_id, dp_type, dp_len, dp_data))  return;
    if (tuya_dp_dispatch_air(dp_id, dp_type, dp_len, dp_data))     return;
    if (tuya_dp_dispatch_floor(dp_id, dp_type, dp_len, dp_data))   return;
    if (tuya_dp_dispatch_freshair(dp_id, dp_type, dp_len, dp_data)) return;
    if (tuya_dp_dispatch_key(dp_id, dp_type, dp_len, dp_data))     return;
    if (tuya_dp_dispatch_setting(dp_id, dp_type, dp_len, dp_data)) return;

    /* 以下 DP 暂无对应 Hooch 接口或数据结构不足以分发:
     * - DPID_FAN_DIRECTION    (101): Hooch AirConditionerFrame 无 wind_direction 字段
     * - DPID_FRESH_AIR_VALVE  (125): Hooch FreshAirFrame 无 valve 字段
     * - DPID_EXHAUST_FAN_SPEED(124): Hooch FreshAirFrame 仅一个 fan_speed 字段
     * - DPID_PIR_STATE        (126): 仅上报，无需分发
     * - DPID_ADV_SET          (143): raw 多字段，需单独解析
     * - DPID_DISP_PARAM       (147): raw 3字段(5bytes)，需单独解析
     */
}

void mcu_recv_zg_sw_ver_cb(unsigned char zg_sw_ver)
{
    //< USER TODO
}

void mcu_recv_zg_auth_cb(unsigned char zg_auth)
{
    //< USER TODO
}

void mcu_recv_zg_mac_addr_cb(unsigned char *mac_addr)
{
    //< USER TODO
}

void mcu_recv_rf_test_result_cb(unsigned char result, unsigned char rssi)
{
    //< USER TODO
}

void mcu_recv_ota_notify_cb(unsigned char check_result)
{
    //< USER TODO
}

void mcu_recv_ota_data_cb(unsigned char last_packet_flag, unsigned int fw_offset, unsigned char *data, unsigned char data_len)
{
    //< USER TODO
}

void mcu_recv_zg_nwk_status_response_cb(unsigned char nwk_status)
{
    //< USER TODO
}

void mcu_recv_double_dongle_test_notify_cb(void)
{
    //< USER TODO
}

void mcu_recv_double_dongle_test_data_cb(unsigned char *test_data, unsigned short test_data_len)
{
    //< USER TODO
}

void mcu_recv_time_sync_cb(unsigned char *std_timestamp, TIME_SYNC_CALENDAR_T *calendar)
{
    //< USER TODO
}

void mcu_recv_gw_nwk_status_cb(unsigned char nwk_status)
{
    //< USER TODO
}

unsigned char mcu_recv_beacon_notify_cb(void)
{
    //< USER TODO
    return 0;
}

void mcu_recv_gpio_config_result_cb(unsigned char port, unsigned char pin, unsigned char result)
{
    //< USER TODO
}

void mcu_recv_gpio_read_result_cb(unsigned char port, unsigned char pin, unsigned char level)
{
    //< USER TODO
}

void mcu_recv_gpio_write_result_cb(unsigned char port, unsigned char pin, unsigned char result)
{
    //< USER TODO
}

void mcu_recv_gpio_irq_cb(unsigned char port, unsigned char pin, unsigned char level)
{
    //< USER TODO
}

void mcu_recv_weather_response_cb(WEATHER_INFO_T *weather_info)
{
    //< USER TODO
}

void mcu_recv_city_response_cb(CITY_INFO_T *city_info)
{
    //< USER TODO
}

#if (DEVICE_TYPE == SCENE_SWITCH_DEVICE)
unsigned char mcu_recv_scene_config_cb(unsigned char key_id, unsigned short group_id, unsigned char scene_id)
{
    /* key_id: Tuya 按键通道 0-based (0~7) → Hooch 1-based (1~8) */
    HOOCH_PROTOCOL_SettingFrame_t setting_frame;
    setting_frame.item                 = HOOCH_PROTOCOL_SETTING_ITEM_MULTICAST_GROUP_ID;
    setting_frame.value               = key_id + 1U;           /* 按键通道(1-based) */
    setting_frame.screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_NEVER;
    setting_frame.page                = HOOCH_PROTOCOL_SETTING_PAGE_SWITCH;
    setting_frame.param1              = (unsigned char)(group_id >> 8);   /* 组播ID高字节 */
    setting_frame.param2              = (unsigned char)(group_id & 0xFF); /* 组播ID低字节 */
    setting_frame.param3              = 0U;
    setting_frame.param4              = 0U;
    setting_frame.sequence            = 0U;
    setting_frame.valid               = 0U;
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);

    return 0U;
}
#endif

/* -END OF FILE-  */
