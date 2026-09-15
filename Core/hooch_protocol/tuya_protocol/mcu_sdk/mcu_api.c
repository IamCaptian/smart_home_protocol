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
    /* 恢复出厂设置 (cmd 0x00) → Hooch 设置项下发 */
    HOOCH_PROTOCOL_SettingFrame_t setting_frame;
    setting_frame.item                 = HOOCH_PROTOCOL_SETTING_ITEM_FACTORY_RESET;
    setting_frame.value                = 1U;
    setting_frame.screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_INVALID;
    setting_frame.page                 = HOOCH_PROTOCOL_SETTING_PAGE_INVALID;
    setting_frame.param1               = 0U;
    setting_frame.param2               = 0U;
    setting_frame.param3               = 0U;
    setting_frame.param4               = 0U;
    setting_frame.sequence             = 0U;
    setting_frame.valid                = 0U;
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
}

void mcu_recv_zg_nwk_status_notify_cb(unsigned char nwk_status)
{
    /* Zigbee 模组入网状态 (cmd 0x02) → Hooch 网络状态设置项
     * 值域 0~3 与 ZG_NWK_STATUS_E / HOOCH_PROTOCOL_SETTING_NETWORK_STATUS_* 一致 */
    if (nwk_status > (unsigned char)HOOCH_PROTOCOL_SETTING_NETWORK_STATUS_JOINING) {
        return;
    }

    HOOCH_PROTOCOL_SettingFrame_t setting_frame;
    setting_frame.item                 = HOOCH_PROTOCOL_SETTING_ITEM_NETWORK_STATUS;
    setting_frame.value               = nwk_status;
    setting_frame.screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_INVALID;
    setting_frame.page                = HOOCH_PROTOCOL_SETTING_PAGE_INVALID;
    setting_frame.param1              = 0U;
    setting_frame.param2              = 0U;
    setting_frame.param3              = 0U;
    setting_frame.param4              = 0U;
    setting_frame.sequence            = 0U;
    setting_frame.valid               = 0U;
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
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
     * - DPID_EXHAUST_FAN_SPEED(124): Hooch FreshAirFrame 仅一个 fan_speed 字段
     * - DPID_PIR_STATE        (126): 仅上报，无需分发
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
    if ((std_timestamp == NULL) || (calendar == NULL)) {
        return;
    }

    /* payload[0..3] = 标准(UTC) 时间戳, 大端 */
    unsigned int utc_sec = ((unsigned int)std_timestamp[0] << 24)
                         | ((unsigned int)std_timestamp[1] << 16)
                         | ((unsigned int)std_timestamp[2] << 8)
                         |  (unsigned int)std_timestamp[3];

#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] Time sync: utc=%u, %04u-%02u-%02u %02u:%02u:%02u\r\n",
            (unsigned int)utc_sec,
            (unsigned int)calendar->w_year, (unsigned int)calendar->w_month,
            (unsigned int)calendar->w_day, (unsigned int)calendar->hour,
            (unsigned int)calendar->min, (unsigned int)calendar->sec);
#endif

    /* 与小米屏口径一致: [13] 时间校准(UTC) value = 大端 UTC 秒 */
    HOOCH_PROTOCOL_SettingFrame_t setting_frame;
    setting_frame.item                 = HOOCH_PROTOCOL_SETTING_ITEM_TIME_CALIBRATION;
    setting_frame.value               = utc_sec;
    setting_frame.screen_off_time_type = HOOCH_PROTOCOL_SETTING_SCREEN_OFF_TIME_INVALID;
    setting_frame.page                = HOOCH_PROTOCOL_SETTING_PAGE_INVALID;
    setting_frame.param1              = 0U;
    setting_frame.param2              = 0U;
    setting_frame.param3              = 0U;
    setting_frame.param4              = 0U;
    setting_frame.sequence            = 0U;
    setting_frame.valid               = 0U;
    (void)HOOCH_PROTOCOL_Setting_SetFrame(&setting_frame);
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

/* ============================================================
 * 天气私有协议 (0x3B) 解析与分发
 * 内容识别与字节偏移沿用金威利原版 receive_weather 语义
 * (帧头 8 字节, 本 SDK 回调收到的 detail 即 payload+偏移):
 *   weather_info->weather_detail = payload + 6
 *   city_info->city_detail       = payload + 2
 * 用户直接在 mcu_recv_weather_*_cb 里读结构体即可。
 * ------------------------------------------------------------ */

void mcu_recv_weather_response_cb(WEATHER_INFO_T *weather_info)
{
    if ((weather_info == NULL) || (weather_info->weather_detail == NULL)) {
        return;
    }

    unsigned char *p = weather_info->weather_detail;   /* = payload + 6 */
    unsigned short len = weather_info->weather_detail_len;

    /* ---- 风向信息: detail[0]=0x06 (查询 type 1: 06风速/07风向/08风级) ---- */
    if ((len >= 7U) && (p[0] == 0x06U)) {
        TUYA_WEATHER_WIND_T wind = {0U, 0U, 0U};
        wind.speed = (unsigned char)(p[1] + p[2]);   /* payload[7..8] 风速 */
        if (p[3] == 0x07U) {
            wind.dir = p[4];                         /* payload[10] 风向 */
        }
        if (p[5] == 0x08U) {
            wind.level = p[6];                       /* payload[12] 风级 */
        }
        mcu_recv_weather_wind_cb(&wind);
        return;
    }

    /* ---- 预报天气: detail[0]=0x01 (温度), 天气概况标志在 payload[40] ---- */
    if (len >= 5U) {
        TUYA_WEATHER_FORECAST_T forecast = {0, {0}};
        signed char today_temp = (signed char)p[4];  /* payload[10] 当天温度 */

        forecast.today_temp = ((today_temp >= 60) || (today_temp < -50)) ? 0 : today_temp;

        if ((len >= 42U) && (p[34] == 0x03U)) {      /* payload[40]=0x03 天气概况 */
            unsigned char i;
            for (i = 0U; i < 6U; i++) {
                forecast.day_code[i] = p[36 + i];    /* payload[42..47] 涂鸦 conditionNum 原值 */
            }
        }
        mcu_recv_weather_forecast_cb(&forecast);
    }
}

void mcu_recv_city_response_cb(CITY_INFO_T *city_info)
{
    if ((city_info == NULL) || (city_info->city_detail == NULL)) {
        return;
    }

    unsigned char *p = city_info->city_detail;       /* = payload + 2 */
    unsigned short len = city_info->city_detail_len;

    if (len < 7U) {
        return;
    }

    TUYA_WEATHER_TEXT_T text = {{0}};
    unsigned char copy_len = (len >= 6U) ? (unsigned char)(len - 6U) : 0U;  /* 文本从 payload[8] 起 */
    if (copy_len > (unsigned char)(sizeof(text.text) - 1U)) {
        copy_len = (unsigned char)(sizeof(text.text) - 1U);
    }
    unsigned char i;
    for (i = 0U; i < copy_len; i++) {
        text.text[i] = p[6U + i];
    }
    text.text[copy_len] = 0U;

    /* 内容识别: payload[2..4] = "c.a"(区县) / "c.c"(城市) */
    if ((p[0] == 0x63U) && (p[1] == 0x2EU) && (p[2] == 0x61U)) {
        mcu_recv_weather_area_cb(&text);
    } else {
        mcu_recv_weather_city_cb(&text);
    }
}

/* ---- 涂鸦天气码(conditionNum) → hooch 统一天气码 ----
 * conditionNum 语义对照涂鸦官方表(101~136, 120=晴 等);
 * 表中未列出的码(如 135/137+ 需真机日志确认)统一归 UNKNOWN。 */
HOOCH_PROTOCOL_SettingWeatherCode_t mcu_weather_code_to_hooch(unsigned char condition_num)
{
    switch (condition_num) {
    case 119U: return HOOCH_PROTOCOL_SETTING_WEATHER_SUNNY;                 /* 大部晴朗 */
    case 120U: return HOOCH_PROTOCOL_SETTING_WEATHER_SUNNY;                 /* 晴 */
    case 129U: return HOOCH_PROTOCOL_SETTING_WEATHER_CLOUDY;                /* 少云 */
    case 132U: return HOOCH_PROTOCOL_SETTING_WEATHER_OVERCAST;              /* 阴 */
    case 108U: return HOOCH_PROTOCOL_SETTING_WEATHER_SHOWER;                /* 局部阵雨 */
    case 111U: return HOOCH_PROTOCOL_SETTING_WEATHER_SHOWER;                /* 小阵雨 */
    case 122U: return HOOCH_PROTOCOL_SETTING_WEATHER_SHOWER;                /* 阵雨 */
    case 123U: return HOOCH_PROTOCOL_SETTING_WEATHER_SHOWER;                /* 强阵雨 */
    case 102U: return HOOCH_PROTOCOL_SETTING_WEATHER_THUNDERSHOWER;         /* 雷暴 */
    case 110U: return HOOCH_PROTOCOL_SETTING_WEATHER_THUNDERSHOWER;         /* 雷电 */
    case 127U: return HOOCH_PROTOCOL_SETTING_WEATHER_THUNDERSHOWER_WITH_HAIL; /* 冰雹 */
    case 136U: return HOOCH_PROTOCOL_SETTING_WEATHER_THUNDERSHOWER_WITH_HAIL; /* 雷阵雨伴有冰雹 */
    case 113U: return HOOCH_PROTOCOL_SETTING_WEATHER_SLEET;                 /* 雨夹雪 */
    case 118U: return HOOCH_PROTOCOL_SETTING_WEATHER_LIGHT_TO_MODERATE_RAIN;/* 小到中雨 */
    case 101U: return HOOCH_PROTOCOL_SETTING_WEATHER_HEAVY_RAIN;            /* 大雨 */
    case 112U: return HOOCH_PROTOCOL_SETTING_WEATHER_RAIN;                  /* 雨 */
    case 107U: return HOOCH_PROTOCOL_SETTING_WEATHER_STORM;                 /* 暴雨 */
    case 134U: return HOOCH_PROTOCOL_SETTING_WEATHER_HEAVY_STORM;           /* 大暴雨 */
    case 125U: return HOOCH_PROTOCOL_SETTING_WEATHER_SEVERE_STORM;          /* 特大暴雨 */
    case 115U: return HOOCH_PROTOCOL_SETTING_WEATHER_SNOW;                  /* 冰粒 */
    case 133U: return HOOCH_PROTOCOL_SETTING_WEATHER_SNOW;                  /* 冰针 */
    case 104U: return HOOCH_PROTOCOL_SETTING_WEATHER_LIGHT_SNOW;            /* 小雪 */
    case 131U: return HOOCH_PROTOCOL_SETTING_WEATHER_MODERATE_SNOW;         /* 中雪 */
    case 124U: return HOOCH_PROTOCOL_SETTING_WEATHER_HEAVY_SNOW;            /* 大雪 */
    case 126U: return HOOCH_PROTOCOL_SETTING_WEATHER_SNOWSTORM;             /* 暴雪 */
    case 105U: return HOOCH_PROTOCOL_SETTING_WEATHER_SNOW;                  /* 雪 */
    case 128U: return HOOCH_PROTOCOL_SETTING_WEATHER_LIGHT_TO_MODERATE_SNOW;/* 小到中雪 */
    case 130U: return HOOCH_PROTOCOL_SETTING_WEATHER_SNOW_FLURRY;           /* 小阵雪 */
    case 106U: return HOOCH_PROTOCOL_SETTING_WEATHER_FOGGY;                 /* 冻雾 */
    case 121U: return HOOCH_PROTOCOL_SETTING_WEATHER_FOGGY;                 /* 雾 */
    case 109U: return HOOCH_PROTOCOL_SETTING_WEATHER_DUST;                  /* 浮尘 */
    case 117U: return HOOCH_PROTOCOL_SETTING_WEATHER_SAND;                  /* 扬沙 */
    case 103U: return HOOCH_PROTOCOL_SETTING_WEATHER_DUSTSTORM;             /* 沙尘暴 */
    case 116U: return HOOCH_PROTOCOL_SETTING_WEATHER_SANDSTORM;             /* 强沙尘暴 */
    case 114U: return HOOCH_PROTOCOL_SETTING_WEATHER_DUST;                  /* 尘卷风(近似浮尘) */
    default:   return HOOCH_PROTOCOL_SETTING_WEATHER_UNKNOWN;               /* 未收录码 */
    }
}

/* ---- 用户层细分回调: 直接读结构体取天气数据 ---- */

void mcu_recv_weather_forecast_cb(const TUYA_WEATHER_FORECAST_T *forecast)
{
    if (forecast == NULL) {
        return;
    }
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] Weather forecast: temp=%d, tuya_code[6]=%u,%u,%u,%u,%u,%u, hooch_today=%u\r\n",
            (int)forecast->today_temp,
            (unsigned int)forecast->day_code[0], (unsigned int)forecast->day_code[1],
            (unsigned int)forecast->day_code[2], (unsigned int)forecast->day_code[3],
            (unsigned int)forecast->day_code[4], (unsigned int)forecast->day_code[5],
            (unsigned int)mcu_weather_code_to_hooch(forecast->day_code[0]));
#endif
}

void mcu_recv_weather_wind_cb(const TUYA_WEATHER_WIND_T *wind)
{
    if (wind == NULL) {
        return;
    }
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] Weather wind: speed=%u, dir=%u, level=%u\r\n",
            (unsigned int)wind->speed, (unsigned int)wind->dir, (unsigned int)wind->level);
#endif
}

void mcu_recv_weather_city_cb(const TUYA_WEATHER_TEXT_T *city)
{
    if (city == NULL) {
        return;
    }
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] Weather city: %s\r\n", (char *)city->text);
#endif
}

void mcu_recv_weather_area_cb(const TUYA_WEATHER_TEXT_T *area)
{
    if (area == NULL) {
        return;
    }
#if TUYA_LOG_ENABLE
    TUYA_LOG_INFO("[Tuya] Weather area: %s\r\n", (char *)area->text);
#endif
}

#if (DEVICE_TYPE == SCENE_SWITCH_DEVICE)
unsigned char mcu_recv_scene_config_cb(unsigned char key_id, unsigned short group_id, unsigned char scene_id)
{
    /* key_id: Tuya 0x41 按键通道 1-based (1~8) → Hooch 按键通道 1-based (1~8), 直接 1:1 */
    if (key_id < 1U || key_id > 8U) {
        return 0U;
    }

    HOOCH_PROTOCOL_SettingFrame_t setting_frame;
    setting_frame.item                 = HOOCH_PROTOCOL_SETTING_ITEM_MULTICAST_GROUP_ID;
    setting_frame.value               = key_id;                 /* 按键通道(1-based) */
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
