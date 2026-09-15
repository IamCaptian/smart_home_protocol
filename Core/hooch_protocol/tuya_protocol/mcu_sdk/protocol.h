/***********************************************************
 * @file     protocol.h
 * @brief    define:            device information, DP ID, frame field, command id, command related define.
 *           function declare:  DP recv & send function.
 * @version  3.3.5
 * @date     2026.04.08
 * @copyright Copyright (c) tuya.inc 2024
 **********************************************************/

#ifndef __PROTOCOL_H_
#define __PROTOCOL_H_

#include "mcu_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

/***********************************************************
 * Macro Definitions
 **********************************************************/
//< USER_CHECK_MSG 
///<product key(pid)  the same as yours in the Tuya IoT Platform
#define PRODUCT_KEY "az0mzbm4"    //开发平台创建产品后生成的16位字符产品唯一标识

//< USER_CHECK_MSG END

//< USER_CHECK_MSG 
///< dp id define
//场景1(可下发可上报)
//备注:
#define DPID_SCENE_1 1
//场景2(可下发可上报)
//备注:
#define DPID_SCENE_2 2
//场景3(可下发可上报)
//备注:
#define DPID_SCENE_3 3
//场景4(可下发可上报)
//备注:
#define DPID_SCENE_4 4
//场景5(可下发可上报)
//备注:
#define DPID_SCENE_5 5
//场景6(可下发可上报)
//备注:
#define DPID_SCENE_6 6
//场景7(可下发可上报)
//备注:
#define DPID_SCENE_7 7
//场景8(可下发可上报)
//备注:
#define DPID_SCENE_8 8
//继电器1(可下发可上报)
//备注:
#define DPID_SWITCH_1 24
//继电器2(可下发可上报)
//备注:
#define DPID_SWITCH_2 25
//继电器3(可下发可上报)
//备注:
#define DPID_SWITCH_3 26
//继电器4(可下发可上报)
//备注:
#define DPID_SWITCH_4 27
//开关5(可下发可上报)
//备注:
#define DPID_SWITCH_5 28
//开关6(可下发可上报)
//备注:
#define DPID_SWITCH_6 29
//风向(可下发可上报)
//备注:
#define DPID_FAN_DIRECTION 101
//风速（枚举）(可下发可上报)
//备注:
#define DPID_FAN_SPEED_ENUM 102
//模式(可下发可上报)
//备注:
#define DPID_MODE 105
//温度设置(可下发可上报)
//备注:
#define DPID_TEMP_SET 106
//空调开关(可下发可上报)
//备注:云端 DP 点由 112 调整为 152
#define DPID_AIR_SWITCH 152
//循环模式(可下发可上报)
//备注:
#define DPID_LOOP_MODE 116
//送风风速(可下发可上报)
//备注:
#define DPID_SUPPLY_FAN_SPEED 123
//排风风速(可下发可上报)
//备注:
#define DPID_EXHAUST_FAN_SPEED 124
//新风开关(可下发可上报)
//备注:BOOL
#define DPID_FRESH_AIR_SWITCH 125
//人体感应状态(只上报)
//备注:
#define DPID_PIR_STATE 126
//地暖开关(可下发可上报)
//备注:
#define DPID_FLOOR_SW 130
//地暖温度(可下发可上报)
//备注:
#define DPID_FLOOR_TEMP 131
//地暖温度上下限设置(可下发可上报)
//备注:上下限(1byte)：0/1 上限/下限
//数值(1byte)：5-35, 间距: 1, 倍数: 0, 单位: ℃
#define DPID_HEAT_LIMIT 132
//空调温度上下限设置(可下发可上报)
//备注:上下限(1byte)：0/1 上限/下限
//数值(1byte)：5-35, 间距: 1, 倍数: 0, 单位: ℃
#define DPID_AC_LIMIT 133
//空调信息(可下发可上报)
//备注:Enum从0开始
//开关(1byte)：1/0 开/关
//模式(1byte)：cold, hot, dry, fan, auto
//风速(1byte)：low, middle, high, auto
//风向(1byte)：horizontal, vertical, auto
//温度(1byte)：数值范围: 5-35, 间距: 1, 倍数: 0, 单位: ℃
#define DPID_AC_INFO 134
//新风信息(可下发可上报)
//备注:
//开关(1byte)：1/0 开/关
//模式(1byte)：auto, indoor_loop, outdoor_loop
//风速(1byte)：off, low, mid, high
#define DPID_FAN_INFO 135
//地暖信息(可下发可上报)
//备注:
//开关(1byte)：1/0 开/关
//温度(1byte)：数值范围: 5-35, 间距: 1, 倍数: 0, 单位: ℃
//
#define DPID_FLOOR_INFO 136
//开关7(可下发可上报)
//备注:
#define DPID_SWITCH_7 137
//开关8(可下发可上报)
//备注:
#define DPID_SWITCH_8 138
//按键模式(可下发可上报)
//备注:
//按键通道(1byte)：switch_1 ~ switch_8
//按键模式(1byte)：switch_1, switch_2, switch_3, switch_4, unreal_switch_1, unreal_switch_2, unreal_switch_3, unreal_switch_4, jog_1, jog_2, jog_3, jog_4, light_?, curtain_?, scene_?
#define DPID_KEY_MODE 139
//页面管理(可下发可上报)
//备注:页面一(1byte)：0为删除  1-4为页面元素个数
//页面二(1byte)：0为删除  1-4为页面元素个数
//空调(1byte)：0为删除  1为添加
//新风(1byte)：0为删除  1为添加
//地暖(1byte)：0为删除  1为添加
#define DPID_PAGE_MGR 140
//点动时间(可下发可上报)
//备注:
//按键通道(1byte)：switch_1 ~ switch_8
//点动开or关(1byte)：0为关   1为开
//时间(1byte)：1~10s
//
#define DPID_JOG_TIME 141
//基础设置功能(可下发可上报)
//备注:上电状态(1byte)：0：off , 1：on, 2：memory
//振动开关(1byte)：0：off , 1：on
//人感上报开关(1byte)：0：off , 1：on
//人感开关(1byte)：0：off , 1：on
//人感倒计时(1byte)：10~60s
//人感灵敏度(1byte)：1~10
//指示灯颜色(3byte)：RGB888  例如红色0xFF0000
//指示灯亮度（默认）(1byte)：0~100
//背光亮度(1byte)：10~100
//关闭指示灯：0：off , 1：on
//屏幕待机样式：NULL
#define DPID_BASE_SET 142
//高级设置功能(只下发)
//备注:指示灯高亮(1byte)：0~100
//页面排序(5byte)：12345（默认）
//温度传感器矫正(1byte char)：-10~+10
//湿度传感器矫正(1byte char)：-20%~+20%
#define DPID_ADV_SET 143
//主题切换(可下发可上报)
//备注:NULL
#define DPID_THEME 144
//按键名称(可下发可上报)
//备注:Enum从0开始
//按键通道(1byte)：switch_1 ~ switch_8
//对应图标(1byte)：NULL
//字符总字节(含\0)(1byte)：？
//utf-8字符串(?字节)：????  
#define DPID_KEY_NAME 145
//童锁(可下发可上报)
//备注:
#define DPID_CHILD_LOCK 146
//展示参数(可下发可上报)
//备注:温度(2byte short)：-32,768 到 32,767
//示例：21.5度  2150
//湿度(1byte char)：0-100
//空气质量(2byte ushort)：0-65535
#define DPID_DISP_PARAM 147
//页面同步(可下发可上报)
//备注:
#define DPID_PAGE_SYNC 148
//总开总关(可下发可上报)
//备注:
#define DPID_MASTER_SW 149
//新风风速(可下发可上报)
//备注:
#define DPID_FRESH_AIR_SPEED 150

//< USER_CHECK_MSG END

///< frame field id
#define FRAME_FIELD_HDR_HIGH                    0
#define FRAME_FIELD_HDR_LOW                     1
#define FRAME_FIELD_PROT_VER                    2
#define FRAME_FIELD_SEQ_HIGH                    3
#define FRAME_FIELD_SEQ_LOW                     4
#define FRAME_FIELD_CMD_ID                      5
#define FRAME_FIELD_PAYLOAD_LEN_HIGH            6
#define FRAME_FIELD_PAYLOAD_LEN_LOW             7
#define FRAME_FIELD_PAYLOAD_START               8

#define FRAME_LEN_WITHOUT_PAYLOAD               9           // the shortest frame length (no payload)

///< frame field value
#define FRAME_VALUE_HDR_HIGH                    0x55
#define FRAME_VALUE_HDR_LOW                     0xaa
#define FRAME_VALUE_HDR                         0x55aa
#define FRAME_VALUE_PROT_VER                    0x02

///< uart protocol command id
#define UART_CMD_FACTORY_RECOVERY               0x00
#define UART_CMD_PRODUCT_INFO                   0x01
#define UART_CMD_ZG_NWK_STATUS_NOTIFY           0x02
#define UART_CMD_RESET_OR_JOIN                  0x03
#define UART_CMD_RX_DP                          0x04
#define UART_CMD_RESPOND_DP                     0x05
#define UART_CMD_REPORT_DP_LINKAGE              0x06
#define UART_CMD_ZG_INFO                        0x07
#define UART_CMD_RF_TEST                        0x08
#define UART_CMD_SCENE_INFO                     0x0A
#define UART_CMD_MCU_VER                        0x0B
#define UART_CMD_OTA_NOTIFY                     0x0C
#define UART_CMD_OTA_DATA                       0x0D
#define UART_CMD_OTA_RESULT                     0x0E
#define UART_CMD_ZG_NWK_STATUS_REQUEST          0x20
#define UART_CMD_DOUBLE_DONGLE_DATA_NOTIFY      0x21
#define UART_CMD_DOUBLE_DONGLE_REPORT           0x22
#define UART_CMD_TIME_SYNC                      0x24
#define UART_CMD_MCU_CHECK_GW_NWK_STAUTS        0x25
#define UART_CMD_MCU_SET_ZG_NWK_PARAM           0x26
#define UART_CMD_SEND_DP_BROADCAST              0x27
#define UART_CMD_GW_CHECK_DP                    0x28
#define UART_CMD_BEACON_TEST                    0x29
#define UART_CMD_RX_DP_GROUP                    0x2A
#define UART_CMD_ZG_WAIT_MCU_TIME               0x2B
#define UART_CMD_REPORT_DP_NOT_LINKAGE          0x2C
#define UART_CMD_CONFIG_ZG_GPIO                 0x36
#define UART_CMD_READ_ZG_GPIO                   0x37
#define UART_CMD_WRITE_ZG_GPIO                  0x38
#define UART_CMD_ZG_GPIO_IRQ                    0x39
#define UART_CMD_WEATHER_REQUEST                0x3A
#define UART_CMD_WEATHER_RESPONSE               0x3B
#define UART_CMD_SCENE_CONFIG                   0x41
#define UART_CMD_SEND_CMD_GROUP                 0x42
#define UART_CMD_SEND_DP_GROUP                  0x43

///< dp data type
#define DP_TYPE_RAW                             0x00
#define DP_TYPE_BOOL                            0x01
#define DP_TYPE_VALUE                           0x02
#define DP_TYPE_STRING                          0x03
#define DP_TYPE_ENUM                            0x04
#define DP_TYPE_BITMAP                          0x05
#define DP_TYPE_FAULT                           DP_TYPE_BITMAP

///< zigbee module info (uart command id: 0x07)
#define ZG_INFO_TYPE_SW_VER                     0x01
#define ZG_INFO_TYPE_AUTH                       0x02
#define ZG_INFO_TYPE_MAC_ADDR                   0x03

///< zigbee module network parameter (uart command id: 0x26)
#define HEART_PERIOD_DEFAULT_SET                0xfffe
#define JOIN_TIMEOUT_DEFAULT_SET                0xfffe
#define REJOIN_INTERVAL_DEFAULT_SET             0xfffe
#define POLL_INTERVAL_DEFAULT_SET               0xfffe
#define FAST_POLL_PERIOD_DEFAULT_SET            0xfffe
#define POLL_FAIL_TIMES_DEFAULT_SET             0xfe
#define APP_DATA_TRIG_REJOIN_DEFAULT_SET        0xfe
#define REJOIN_TRY_TIMES_DEFAULT_SET            0xfe
#define RF_POWER_DEFAULT_SET                    0xfe
#define NWK_PARAM_DEFAULT_SET                   {HEART_PERIOD_DEFAULT_SET, JOIN_TIMEOUT_DEFAULT_SET, REJOIN_INTERVAL_DEFAULT_SET, POLL_INTERVAL_DEFAULT_SET, FAST_POLL_PERIOD_DEFAULT_SET, POLL_FAIL_TIMES_DEFAULT_SET, APP_DATA_TRIG_REJOIN_DEFAULT_SET, REJOIN_TRY_TIMES_DEFAULT_SET, RF_POWER_DEFAULT_SET}

/***********************************************************
 * Typedef Definitions
 **********************************************************/

///< zigbee module network status (uart command id: 0x02 0x20)
typedef enum {
    ZG_NWK_STATUS_NOT_JOIN = 0,
    ZG_NWK_STATUS_JOINED,
    ZG_NWK_STATUS_ERROR,
    ZG_NWK_STATUS_JOINING,
} ZG_NWK_STATUS_E;

///< gateway network status (uart command id: 0x25)
typedef enum {
    GW_NWK_STATUS_OFFLINE = 0,
    GW_NWK_STATUS_ONLINE,
    GW_NWK_STATUS_RESPOND_TIMEOUT,
} GW_NWK_STATUS_E;

///< dp send type (used in mcu recv and send dp msg handle)
typedef enum {
    DP_SEND_TYPE_NOT_SEND,                      // not send                                                         : when mcu receives broadcast dp msg (uart command id: 0x2a), it doesn't need to respond.
    DP_SEND_TYPE_RESPOND,                       // respond dp msg (uart command id: 0x05)                           : when mcu receives dp msg (uart command id: 0x04), it needs to respond to zigbee module.
    DP_SEND_TYPE_REPORT_LINKAGE,                // report dp msg with linkage trigger (uart command id: 0x06)       :
    DP_SEND_TYPE_REPORT_NOT_LINKAGE,            // report dp msg without linkage trigger (uart command id: 0x2c)    :
    DP_SEND_TYPE_SEND_BROADCAST,                // send dp msg broadcast (uart command id: 0x27)                    :
    DP_SEND_TYPE_SEND_GROUP,                    // send dp msg to group (uart command id: 0x43)                     :
} DP_SEND_TYPE_E;

#if (DEVICE_TYPE == SLEEP_END_DEVICE)
extern unsigned short g_zg_wait_mcu_time;       // zigbee module wait mcu time (uart command id: 0x2b)
extern unsigned short g_mcu_wait_zg_time;       // mcu wait zigbee module time
#if (MCU_WAKEUP_MODULE_METHOD == LOW_LEVEL_WAKE_UP)
extern const unsigned short g_wakeup_stay_time; // mcu wakeup stay time must be less than 2 minutes
#endif
#endif

#if SUPPORT_RECEIVE_BROADCAST_DATA
extern unsigned short g_group_id_send_dp_msg;   // mcu receives group id (uart command id: 0x41 0x42 0x43)
#endif

/***********************************************************
 * Function Declarations
 **********************************************************/

/*----------------------------------------------------------
 *              frame receive handle function
 *--------------------------------------------------------*/

/**
 * @brief  frame rx handle entry point
 * @param[in] cur_frame_start_offset
 * @return operate result
 */
unsigned char frame_rx_handle(unsigned short cur_frame_start_offset);

/*----------------------------------------------------------
 *                dp receive handle function
 *--------------------------------------------------------*/

/**
 * @brief  check dp id and dp type
 * @param[in] dpid
 * @param[in] dpid
 * @return TRUE is correct, FALSE is wrong or not found
 */
unsigned char dp_info_check(unsigned char dp_id, unsigned char dp_type);

/**
 * @brief  get dp data in format (bool)
 * @param[in] dp_data
 * @param[in] dp_data_len: 1
 * @return dp data
 */
unsigned char mcu_get_dp_download_bool(const unsigned char *dp_data, unsigned short dp_data_len);

/**
 * @brief  get dp data in format (enum)
 * @param[in] dp_data
 * @param[in] dp_data_len: 1
 * @return dp data
 */
unsigned char mcu_get_dp_download_enum(const unsigned char *dp_data, unsigned short dp_data_len);

/**
 * @brief  get dp data in format (value)
 * @param[in] dp_data
 * @param[in] dp_data_len: 4
 * @return dp data
 */
unsigned long mcu_get_dp_download_value(const unsigned char *dp_data, unsigned short dp_data_len);

/*----------------------------------------------------------
 *                    dp report function
 *--------------------------------------------------------*/

/**
 * @brief  send dp msg (raw)
 * @param[in] dp_id
 * @param[in] dp_data
 * @param[in] dp_data_len
 * @return operate result
 */
unsigned char mcu_dp_raw_update(unsigned char dp_id, const unsigned char *dp_data, unsigned short dp_data_len);

/**
 * @brief  send dp msg (bool)
 * @param[in] dp_id
 * @param[in] dp_data
 * @return operate result
 */
unsigned char mcu_dp_bool_update(unsigned char dp_id, unsigned char dp_data);

/**
 * @brief  send dp msg (value)
 * @param[in] dp_id
 * @param[in] dp_data
 * @param[in] dp_data_len
 * @return operate result
 */
unsigned char mcu_dp_value_update(unsigned char dp_id, unsigned long dp_data);

/**
 * @brief  send dp msg (string)
 * @param[in] dp_id
 * @param[in] dp_data
 * @param[in] dp_data_len
 * @return operate result
 */
unsigned char mcu_dp_string_update(unsigned char dp_id, const unsigned char *dp_data, unsigned short dp_data_len);

/**
 * @brief  send dp msg (enum)
 * @param[in] dp_id
 * @param[in] dp_data
 * @return operate result
 */
unsigned char mcu_dp_enum_update(unsigned char dp_id, unsigned char dp_data);

/**
 * @brief  send dp msg (bitmap)
 * @param[in] dp_id
 * @param[in] dp_data
 * @return operate result
 */
unsigned char mcu_dp_bitmap_update(unsigned char dp_id, unsigned long dp_data);

/**
 * @brief  send dp msg (fault), the same as bitmap
 * @param[in] dp_id
 * @param[in] dp_data
 * @return operate result
 */
unsigned char mcu_dp_fault_update(unsigned char dp_id, unsigned long dp_data);

#ifdef __cplusplus
}
#endif

#endif
/* -END OF FILE-  */
