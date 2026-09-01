/**
  ******************************************************************************
  * @file    hooch_cli_driver.c
  * @brief   基于UART1的CLI命令行驱动，支持退网/组网命令
  ******************************************************************************
  */

#include "main.h"
#include "hooch_protocol.h"
#include "usart.h"
#include "user_printf.h"
#include "xiaomi_smart_screen_handle.h"
#include "xiaomi_smart_screen_circular_bufferc.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/* ======================== 配置定义 ======================== */
#define CLI_RX_BUF_SIZE       128     /* CLI接收缓冲区大小 */
#define CLI_MAX_ARGS          8       /* 最大参数个数 */
#define CLI_PROMPT            "hooch> "

/* 小米模组网络命令定义 */
#define CMD_NET_JOIN          0x81    /* 组网命令 */
#define CMD_NET_LEAVE         0x04    /* 退网命令 */
#define CMD_NET_STATUS        0x05    /* 查询网络状态命令 */

/* ======================== 私有变量 ======================== */
static uint8_t cli_rx_buf[CLI_RX_BUF_SIZE];  /* 命令行缓冲区 */
static uint16_t cli_rx_index = 0;             /* 命令行索引 */
static uint8_t cli_cmd_ready = 0;             /* 命令就绪标志 */

/* 中断接收FIFO */
#define CLI_FIFO_SIZE  64
static volatile uint8_t cli_fifo[CLI_FIFO_SIZE];
static volatile uint8_t cli_fifo_head = 0;
static volatile uint8_t cli_fifo_tail = 0;

/* ======================== 命令表定义 ======================== */
typedef void (*cli_cmd_func_t)(int argc, char *argv[]);

typedef struct {
    const char *name;         /* 命令名称 */
    const char *help;         /* 帮助信息 */
    cli_cmd_func_t handler;   /* 命令处理函数 */
} cli_cmd_t;

/* ======================== 命令处理函数声明 ======================== */
static void cli_cmd_help(int argc, char *argv[]);
static void cli_cmd_join(int argc, char *argv[]);
static void cli_cmd_leave(int argc, char *argv[]);
static void cli_cmd_status(int argc, char *argv[]);
static void cli_cmd_version(int argc, char *argv[]);
static void cli_cmd_reboot(int argc, char *argv[]);
static void cli_cmd_reset(int argc, char *argv[]);
static void cli_cmd_report(int argc, char *argv[]);
static void cli_cmd_mi_switch(int argc, char *argv[]);
static void cli_cmd_mi_brightness(int argc, char *argv[]);
static void cli_cmd_mi_color_temp(int argc, char *argv[]);
static void cli_cmd_mi_curtain_switch(int argc, char *argv[]);
static void cli_cmd_mi_curtain_stop(int argc, char *argv[]);
static void cli_cmd_mi_curtain_percent(int argc, char *argv[]);
static void cli_cmd_mi_curtain_angle(int argc, char *argv[]);
static void cli_cmd_mi_key_status(int argc, char *argv[]);
static void cli_cmd_mi_key_click(int argc, char *argv[]);
static void cli_cmd_mi_setting_report(int argc, char *argv[]);
static void cli_cmd_mi_human_presence(int argc, char *argv[]);
static void cli_cmd_mi_get_weather(int argc, char *argv[]);
static void cli_cmd_mi_air_power(int argc, char *argv[]);
static void cli_cmd_mi_air_mode(int argc, char *argv[]);
static void cli_cmd_mi_air_fan_speed(int argc, char *argv[]);
static void cli_cmd_mi_air_temperature(int argc, char *argv[]);
static void cli_cmd_mi_air_frame(int argc, char *argv[]);
static void cli_cmd_mi_air_frame_addr(int argc, char *argv[]);
static void cli_cmd_mi_code_switch(int argc, char *argv[]);
static void cli_cmd_mi_code_page(int argc, char *argv[]);
static void cli_cmd_mi_scene_report(int argc, char *argv[]);
static void cli_cmd_mi_scene_report_xiaomi(int argc, char *argv[]);
static void cli_cmd_mi_light_switch(int argc, char *argv[]);
static void cli_cmd_mi_light_brightness(int argc, char *argv[]);
static void cli_cmd_mi_light_color_temp(int argc, char *argv[]);
static void cli_cmd_mi_curtain_switch_page(int argc, char *argv[]);
static void cli_cmd_mi_curtain_stop_page(int argc, char *argv[]);
static void cli_cmd_mi_curtain_percent_page(int argc, char *argv[]);
static void cli_cmd_mi_curtain_angle_page(int argc, char *argv[]);
static void cli_cmd_mi_screen_read(int argc, char *argv[]);

/* ======================== 命令表 ======================== */
static const cli_cmd_t cli_cmd_table[] = {
    {"help",    "Show help info",                    cli_cmd_help},         /* Usage: help */
    {"join",    "Join network - send join cmd",      cli_cmd_join},         /* Usage: join */
    {"leave",   "Leave network - send leave cmd",    cli_cmd_leave},        /* Usage: leave */
    {"status",  "Query network status",              cli_cmd_status},       /* Usage: status */
    {"version", "Query module version",              cli_cmd_version},      /* Usage: version */
    {"reboot",  "Soft reset MCU",                    cli_cmd_reboot},       /* Usage: reboot */
    {"reset",   "Reset module to factory",           cli_cmd_reset},        /* Usage: reset */
    {"report",  "Report frame: report cmd data",     cli_cmd_report},       /* Usage: report <cmd_hex> <data_hex> */
    {"mi_scene_report", "Scene report: mi_scene_report page key type", cli_cmd_mi_scene_report_xiaomi}, /* Usage: mi_scene_report <page> <key:1~8> <type:1=click,2=double,3=long> */
    // {"mi_switch", "Dimmer switch: mi_switch key 0|1", cli_cmd_mi_switch},   /* Usage: mi_switch <key> <state> */
    // {"mi_brightness", "Dimmer brightness: mi_brightness key value", cli_cmd_mi_brightness}, /* Usage: mi_brightness <key> <brightness> */
    // {"mi_color_temp", "Dimmer color temp: mi_color_temp key value", cli_cmd_mi_color_temp},  /* Usage: mi_color_temp <key> <color_temp> */
    // {"mi_light_switch", "Light page switch: mi_light_switch page key state", cli_cmd_mi_light_switch}, /* Usage: mi_light_switch <page> <key> <state> */
    // {"mi_light_brightness", "Light page brightness: mi_light_brightness page key value", cli_cmd_mi_light_brightness}, /* Usage: mi_light_brightness <page> <key> <brightness> */
    // {"mi_light_color_temp", "Light page color temp: mi_light_color_temp page key value", cli_cmd_mi_light_color_temp}, /* Usage: mi_light_color_temp <page> <key> <color_temp> */
    // {"mi_curtain_switch", "Curtain switch: mi_curtain_switch key value", cli_cmd_mi_curtain_switch}, /* Usage: mi_curtain_switch <key> <value> */
    // {"mi_curtain_stop", "Curtain stop: mi_curtain_stop key value", cli_cmd_mi_curtain_stop}, /* Usage: mi_curtain_stop <key> <value> */
    // {"mi_curtain_percent", "Curtain percent: mi_curtain_percent key value", cli_cmd_mi_curtain_percent}, /* Usage: mi_curtain_percent <key> <percent> */
    // {"mi_curtain_angle", "Curtain angle: mi_curtain_angle key value", cli_cmd_mi_curtain_angle}, /* Usage: mi_curtain_angle <key> <angle> */
    // {"mi_curtain_switch_page", "Curtain page switch: mi_curtain_switch_page page key state", cli_cmd_mi_curtain_switch_page}, /* Usage: mi_curtain_switch_page <page> <key> <state> */
    // {"mi_curtain_stop_page", "Curtain page stop: mi_curtain_stop_page page key value", cli_cmd_mi_curtain_stop_page}, /* Usage: mi_curtain_stop_page <page> <key> <value> */
    // {"mi_curtain_percent_page", "Curtain page percent: mi_curtain_percent_page page key value", cli_cmd_mi_curtain_percent_page}, /* Usage: mi_curtain_percent_page <page> <key> <percent> */
    // {"mi_curtain_angle_page", "Curtain page angle: mi_curtain_angle_page page key value", cli_cmd_mi_curtain_angle_page}, /* Usage: mi_curtain_angle_page <page> <key> <angle> */
    // {"mi_key_status", "Key status: mi_key_status key 0|1", cli_cmd_mi_key_status}, /* Usage: mi_key_status <key> <state> */
    // {"mi_key_click", "Key click: mi_key_click key event state", cli_cmd_mi_key_click}, /* Usage: mi_key_click <key> <event> <state> */
     {"mi_setting_report", "Setting report: mi_setting_report event [value]", cli_cmd_mi_setting_report}, /* Usage: mi_setting_report <event> [value] */
    // {"mi_human_presence", "Human presence: mi_human_presence 0|1", cli_cmd_mi_human_presence}, /* Usage: mi_human_presence <state> */
    // {"mi_get_weather", "Get weather: mi_get_weather [value]", cli_cmd_mi_get_weather}, /* Usage: mi_get_weather [value] */
    // {"mi_air_power", "Air power: mi_air_power channel 0|1", cli_cmd_mi_air_power}, /* Usage: mi_air_power <channel> <power> */
    // {"mi_air_mode", "Air mode: mi_air_mode channel mode", cli_cmd_mi_air_mode}, /* Usage: mi_air_mode <channel> <mode> */
    // {"mi_air_fan_speed", "Air fan: mi_air_fan_speed channel speed", cli_cmd_mi_air_fan_speed}, /* Usage: mi_air_fan_speed <channel> <fan_speed> */
    // {"mi_air_temperature", "Air temp: mi_air_temperature channel temp", cli_cmd_mi_air_temperature}, /* Usage: mi_air_temperature <channel> <temperature> */
    // {"mi_air_frame", "Air frame: mi_air_frame channel power mode fan_speed temp", cli_cmd_mi_air_frame}, /* Usage: mi_air_frame <channel> <power> <mode> <fan_speed> <temperature> */
    // {"mi_air_frame_addr", "Air frame(addr): mi_air_frame_addr addr power mode fan_speed temp", cli_cmd_mi_air_frame_addr}, /* Usage: mi_air_frame_addr <address> <power> <mode> <fan_speed> <temperature> */
    // {"mi_code_switch", "Code match switch page: mi_code_switch key mode action", cli_cmd_mi_code_switch}, /* Usage: mi_code_switch <key:1-4> <mode:0=dimmer,1=curtain> <action:0=clear,1=match> */
    // {"mi_code_page", "Code match page: mi_code_page page index action", cli_cmd_mi_code_page}, /* Usage: mi_code_page <page:2=light,3=curtain,4=ac> <index> <action:0=clear,1=match> */
    // {"tuya_scene", "Scene report: tuya_scene key", cli_cmd_mi_scene_report}, /* Usage: tuya_scene <key:1~8> */
    // {"mi_screen_read", "Read screen status: mi_screen_read dp", cli_cmd_mi_screen_read}, /* Usage: mi_screen_read <dp_decimal> */
};

/*
mi_code_switch 1 0 1   → 按键1, 调光, 对码
mi_code_switch 1 1 0   → 按键1, 窗帘, 清码
mi_code_switch 3 0 1   → 按键3, 调光, 对码
mi_code_switch 4 1 0   → 按键4, 窗帘, 清码

mi_code_page 2 0 1     → 灯光页, 索引0, 对码
mi_code_page 3 1 0     → 窗帘页, 索引1, 清码
mi_code_page 4 0 1     → 空调页, 索引0, 对码
*/
#define CLI_CMD_COUNT  (sizeof(cli_cmd_table) / sizeof(cli_cmd_table[0]))

/* ======================== 私有函数 ======================== */

/**
 * @brief 通过UART1打印CLI输出
 */
static void cli_print(const char *str)
{
    UART1_SendString(str);
}

/**
 * @brief 发送网络命令帧到小米模组 (通过UART2)
 * @param cmd 命令字节
 * @param data 数据指针(可为NULL)
 * @param data_len 数据长度
 * @return 0成功，1失败
 */
static uint8_t cli_send_net_cmd(uint8_t cmd, uint8_t *data, uint16_t data_len)
{
    Frame_t frame;

    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();
    frame.command = cmd;
    frame.data_len = data_len;

    if (data != NULL && data_len > 0) {
        memcpy(frame.data, data, data_len);
    }

    return xiaoni_smart_screen_uart_sendframe(&frame);
}

static uint8_t cli_hex_char_to_nibble(char ch, uint8_t *nibble)
{
    if (ch >= '0' && ch <= '9') {
        *nibble = (uint8_t)(ch - '0');
        return 1;
    }

    if (ch >= 'a' && ch <= 'f') {
        *nibble = (uint8_t)(ch - 'a' + 10U);
        return 1;
    }

    if (ch >= 'A' && ch <= 'F') {
        *nibble = (uint8_t)(ch - 'A' + 10U);
        return 1;
    }

    return 0;
}

static uint8_t cli_parse_hex_byte(const char *str, uint8_t *value)
{
    uint8_t high;
    uint8_t low;

    if (str == NULL || value == NULL) {
        return 0;
    }

    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
    }

    if (str[0] == '\0' || str[1] == '\0' || str[2] != '\0') {
        return 0;
    }

    if (!cli_hex_char_to_nibble(str[0], &high) || !cli_hex_char_to_nibble(str[1], &low)) {
        return 0;
    }

    *value = (uint8_t)((high << 4) | low);
    return 1;
}

static uint8_t cli_parse_hex_data(const char *hex_str, uint8_t *data, uint16_t *data_len)
{
    uint16_t hex_len;
    uint16_t i;

    if (hex_str == NULL || data == NULL || data_len == NULL) {
        return 0;
    }

    hex_len = (uint16_t)strlen(hex_str);
    if (hex_len == 0U || (hex_len & 0x01U) != 0U) {
        return 0;
    }

    *data_len = (uint16_t)(hex_len / 2U);
    if (*data_len > MAX_FRAME_DATA_LEN) {
        return 0;
    }

    for (i = 0U; i < *data_len; i++) {
        uint8_t high;
        uint8_t low;

        if (!cli_hex_char_to_nibble(hex_str[i * 2U], &high)
            || !cli_hex_char_to_nibble(hex_str[i * 2U + 1U], &low)) {
            return 0;
        }

        data[i] = (uint8_t)((high << 4) | low);
    }

    return 1;
}

static uint8_t cli_parse_u8_arg(const char *str, uint8_t *value)
{
    char *endptr;
    unsigned long parsed_value;

    if (str == NULL || value == NULL) {
        return 0;
    }

    parsed_value = strtoul(str, &endptr, 0);
    if (str[0] == '\0' || endptr == NULL || *endptr != '\0' || parsed_value > 0xFFUL) {
        return 0;
    }

    *value = (uint8_t)parsed_value;
    return 1;
}

static uint8_t cli_parse_dimmer_key(
    const char *str,
    HOOCH_PROTOCOL_DimmerLightKey_t *key)
{
    uint8_t key_value;

    if (!cli_parse_u8_arg(str, &key_value)) {
        return 0;
    }

    if (key_value < (uint8_t)HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_1 ||
        key_value > HOOCH_PROTOCOL_DIMMER_LIGHT_KEY_COUNT) {
        return 0;
    }

    *key = (HOOCH_PROTOCOL_DimmerLightKey_t)key_value;
    return 1;
}

static uint8_t cli_parse_dimmer_switch_state(
    const char *str,
    HOOCH_PROTOCOL_DimmerLightSwitchState_t *state)
{
    uint8_t state_value;

    if (!cli_parse_u8_arg(str, &state_value)) {
        return 0;
    }

    if (state_value > (uint8_t)HOOCH_PROTOCOL_DIMMER_LIGHT_SWITCH_ON) {
        return 0;
    }

    *state = (HOOCH_PROTOCOL_DimmerLightSwitchState_t)state_value;
    return 1;
}

static uint8_t cli_parse_curtain_key(
    const char *str,
    HOOCH_PROTOCOL_CurtainKey_t *key)
{
    uint8_t key_value;

    if (!cli_parse_u8_arg(str, &key_value)) {
        return 0;
    }

    if (key_value < (uint8_t)HOOCH_PROTOCOL_CURTAIN_KEY_1 ||
        key_value > HOOCH_PROTOCOL_CURTAIN_KEY_COUNT) {
        return 0;
    }

    *key = (HOOCH_PROTOCOL_CurtainKey_t)key_value;
    return 1;
}

static uint8_t cli_parse_key_status_key(
    const char *str,
    HOOCH_PROTOCOL_KeyStatusKey_t *key)
{
    uint8_t key_value;

    if (!cli_parse_u8_arg(str, &key_value)) {
        return 0;
    }

    if (key_value < (uint8_t)HOOCH_PROTOCOL_KEY_STATUS_KEY_1 ||
        key_value > HOOCH_PROTOCOL_KEY_STATUS_KEY_COUNT) {
        return 0;
    }

    *key = (HOOCH_PROTOCOL_KeyStatusKey_t)key_value;
    return 1;
}

static uint8_t cli_parse_key_status_state(
    const char *str,
    HOOCH_PROTOCOL_KeyStatusState_t *state)
{
    uint8_t state_value;

    if (!cli_parse_u8_arg(str, &state_value)) {
        return 0;
    }

    if (state_value > (uint8_t)HOOCH_PROTOCOL_KEY_STATUS_STATE_ON) {
        return 0;
    }

    *state = (HOOCH_PROTOCOL_KeyStatusState_t)state_value;
    return 1;
}

static uint8_t cli_parse_key_click_key(
    const char *str,
    HOOCH_PROTOCOL_KeyClickReportKey_t *key)
{
    uint8_t key_value;

    if (!cli_parse_u8_arg(str, &key_value)) {
        return 0;
    }

    if (key_value < (uint8_t)HOOCH_PROTOCOL_KEY_CLICK_REPORT_KEY_1 ||
        key_value > HOOCH_PROTOCOL_KEY_CLICK_REPORT_KEY_COUNT) {
        return 0;
    }

    *key = (HOOCH_PROTOCOL_KeyClickReportKey_t)key_value;
    return 1;
}

static uint8_t cli_parse_key_click_event(
    const char *str,
    HOOCH_PROTOCOL_KeyClickReportEvent_t *event)
{
    uint8_t event_value;

    if (!cli_parse_u8_arg(str, &event_value)) {
        return 0;
    }

    if (event_value < (uint8_t)HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_SINGLE_CLICK ||
        event_value > (uint8_t)HOOCH_PROTOCOL_KEY_CLICK_REPORT_EVENT_LONG_CLICK_1P5S) {
        return 0;
    }

    *event = (HOOCH_PROTOCOL_KeyClickReportEvent_t)event_value;
    return 1;
}

static uint8_t cli_parse_key_click_state(
    const char *str,
    HOOCH_PROTOCOL_KeyClickReportState_t *state)
{
    uint8_t state_value;

    if (!cli_parse_u8_arg(str, &state_value)) {
        return 0;
    }

    if (state_value > (uint8_t)HOOCH_PROTOCOL_KEY_CLICK_REPORT_STATE_ON) {
        return 0;
    }

    *state = (HOOCH_PROTOCOL_KeyClickReportState_t)state_value;
    return 1;
}

static uint8_t cli_parse_setting_report_event(
    const char *str,
    HOOCH_PROTOCOL_SettingReportEvent_t *event)
{
    uint8_t event_value;

    if (!cli_parse_u8_arg(str, &event_value)) {
        return 0;
    }

    if (event_value <= (uint8_t)HOOCH_PROTOCOL_SETTING_REPORT_EVENT_INVALID ||
        event_value > (uint8_t)HOOCH_PROTOCOL_SETTING_REPORT_EVENT_KNX_UPDATE_CONFIG) {
        return 0;
    }

    *event = (HOOCH_PROTOCOL_SettingReportEvent_t)event_value;
    return 1;
}

static uint8_t cli_parse_air_channel(
    const char *str,
    uint8_t *channel)
{
    return cli_parse_u8_arg(str, channel);
}

static uint8_t cli_parse_air_address(
    const char *str,
    uint16_t *address)
{
    char *endptr;
    unsigned long parsed_value;

    if (str == NULL || address == NULL) {
        return 0;
    }

    parsed_value = strtoul(str, &endptr, 0);
    if (str[0] == '\0' || endptr == NULL || *endptr != '\0' || parsed_value > 0xFFFFUL) {
        return 0;
    }

    *address = (uint16_t)parsed_value;
    return 1;
}

static uint8_t cli_parse_air_power(
    const char *str,
    HOOCH_PROTOCOL_AirConditionerPower_t *power)
{
    uint8_t value;

    if (!cli_parse_u8_arg(str, &value)) {
        return 0;
    }

    if (value > (uint8_t)HOOCH_PROTOCOL_AIR_CONDITIONER_POWER_ON) {
        return 0;
    }

    *power = (HOOCH_PROTOCOL_AirConditionerPower_t)value;
    return 1;
}

static uint8_t cli_parse_air_mode(
    const char *str,
    HOOCH_PROTOCOL_AirConditionerMode_t *mode)
{
    uint8_t value;

    if (!cli_parse_u8_arg(str, &value)) {
        return 0;
    }

    if (value < (uint8_t)HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_COOL ||
        value > (uint8_t)HOOCH_PROTOCOL_AIR_CONDITIONER_MODE_AUTO) {
        return 0;
    }

    *mode = (HOOCH_PROTOCOL_AirConditionerMode_t)value;
    return 1;
}

static uint8_t cli_parse_air_fan_speed(
    const char *str,
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t *fan_speed)
{
    uint8_t value;

    if (!cli_parse_u8_arg(str, &value)) {
        return 0;
    }

    if (value < (uint8_t)HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_LOW ||
        value > (uint8_t)HOOCH_PROTOCOL_AIR_CONDITIONER_FAN_SPEED_AUTO) {
        return 0;
    }

    *fan_speed = (HOOCH_PROTOCOL_AirConditionerFanSpeed_t)value;
    return 1;
}

static uint8_t cli_parse_air_temperature(
    const char *str,
    uint8_t *temperature)
{
    uint8_t value;

    if (!cli_parse_u8_arg(str, &value)) {
        return 0;
    }

    if (value < 16U || value > 30U) {
        return 0;
    }

    *temperature = value;
    return 1;
}

static void cli_print_dimmer_send_result(
    const char *cmd_name,
    HOOCH_PROTOCOL_DimmerLightResult_t result)
{
    char buf[96];

    snprintf(buf, sizeof(buf), "[CLI] %s result: %u\r\n", cmd_name, (unsigned int)result);
    cli_print(buf);

    if (result == HOOCH_PROTOCOL_DIMMER_LIGHT_RESULT_OK) {
        LOG_INFO("[CLI] Dimmer light command sent\r\n");
    } else {
        LOG_ERROR("[CLI] Dimmer light command failed\r\n");
    }
}

static void cli_print_key_status_send_result(
    const char *cmd_name,
    HOOCH_PROTOCOL_KeyStatusResult_t result)
{
    char buf[96];

    snprintf(buf, sizeof(buf), "[CLI] %s result: %u\r\n", cmd_name, (unsigned int)result);
    cli_print(buf);

    if (result == HOOCH_PROTOCOL_KEY_STATUS_RESULT_OK) {
        LOG_INFO("[CLI] Key status report command sent\r\n");
    } else {
        LOG_ERROR("[CLI] Key status report command failed\r\n");
    }
}

static void cli_print_key_click_send_result(
    const char *cmd_name,
    HOOCH_PROTOCOL_KeyClickReportResult_t result)
{
    char buf[96];

    snprintf(buf, sizeof(buf), "[CLI] %s result: %u\r\n", cmd_name, (unsigned int)result);
    cli_print(buf);

    if (result == HOOCH_PROTOCOL_KEY_CLICK_REPORT_RESULT_OK) {
        LOG_INFO("[CLI] Key click report command sent\r\n");
    } else {
        LOG_ERROR("[CLI] Key click report command failed\r\n");
    }
}

static void cli_print_curtain_send_result(
    const char *cmd_name,
    HOOCH_PROTOCOL_CurtainResult_t result)
{
    char buf[96];

    snprintf(buf, sizeof(buf), "[CLI] %s result: %u\r\n", cmd_name, (unsigned int)result);
    cli_print(buf);

    if (result == HOOCH_PROTOCOL_CURTAIN_RESULT_OK) {
        LOG_INFO("[CLI] Curtain command sent\r\n");
    } else {
        LOG_ERROR("[CLI] Curtain command failed\r\n");
    }
}

static void cli_print_setting_report_send_result(
    const char *cmd_name,
    HOOCH_PROTOCOL_SettingResult_t result)
{
    char buf[96];

    snprintf(buf, sizeof(buf), "[CLI] %s result: %u\r\n", cmd_name, (unsigned int)result);
    cli_print(buf);

    if (result == HOOCH_PROTOCOL_SETTING_RESULT_OK) {
        LOG_INFO("[CLI] Setting report command sent\r\n");
    } else {
        LOG_ERROR("[CLI] Setting report command failed\r\n");
    }
}

static void cli_print_air_send_result(
    const char *cmd_name,
    HOOCH_PROTOCOL_AirConditionerResult_t result)
{
    char buf[96];

    snprintf(buf, sizeof(buf), "[CLI] %s result: %u\r\n", cmd_name, (unsigned int)result);
    cli_print(buf);

    if (result == HOOCH_PROTOCOL_AIR_CONDITIONER_RESULT_OK) {
        LOG_INFO("[CLI] Air conditioner report command sent\r\n");
    } else {
        LOG_ERROR("[CLI] Air conditioner report command failed\r\n");
    }
}

static void cli_print_code_match_send_result(
    const char *cmd_name,
    HOOCH_PROTOCOL_CodeMatchReportResult_t result)
{
    char buf[96];

    snprintf(buf, sizeof(buf), "[CLI] %s result: %u\r\n", cmd_name, (unsigned int)result);
    cli_print(buf);

    if (result == HOOCH_PROTOCOL_CODE_MATCH_REPORT_RESULT_OK) {
        LOG_INFO("[CLI] Code match report command sent\r\n");
    } else {
        LOG_ERROR("[CLI] Code match report command failed\r\n");
    }
}

/**
 * @brief 解析命令行，将字符串分割为参数
 * @param cmd_line 命令行字符串
 * @param argv 参数数组
 * @return 参数个数
 */
static int cli_parse_args(char *cmd_line, char *argv[])
{
    int argc = 0;
    char *p = cmd_line;

    while (*p && argc < CLI_MAX_ARGS) {
        /* 跳过空格 */
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;

        argv[argc++] = p;

        /* 找到下一个空格 */
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) {
            *p = '\0';
            p++;
        }
    }

    return argc;
}

/* ======================== 命令处理函数实现 ======================== */

/**
 * @brief help命令 - 显示所有可用命令
 */
static void cli_cmd_help(int argc, char *argv[])
{
    cli_print("\r\n===== Hooch CLI Commands =====\r\n");
    for (uint8_t i = 0; i < CLI_CMD_COUNT; i++) {
        char buf[80];
        snprintf(buf, sizeof(buf), "  %-10s - %s\r\n",
                 cli_cmd_table[i].name, cli_cmd_table[i].help);
        cli_print(buf);
    }
    cli_print("==============================\r\n");
}

/**
 * @brief join命令 - 发送组网命令
 */
static void cli_cmd_join(int argc, char *argv[])
{
    cli_print("[CLI] Sending join network cmd...\r\n");
    unsigned char join_status = 0;
    if (cli_send_net_cmd(CMD_NET_JOIN, &join_status, 1) == 0) {
        cli_print("[CLI] Join network cmd sent OK!\r\n");
        LOG_INFO("[CLI] Join network command sent\r\n");
    } else {
        cli_print("[CLI] Join network cmd FAILED!\r\n");
        LOG_ERROR("[CLI] Join network command failed\r\n");
    }
}

/**
 * @brief leave命令 - 发送退网命令
 */
static void cli_cmd_leave(int argc, char *argv[])
{
    cli_print("[CLI] Sending leave network cmd...\r\n");

    if (cli_send_net_cmd(CMD_NET_LEAVE, NULL, 0) == 0) {
        cli_print("[CLI] Leave network cmd sent OK!\r\n");
        LOG_INFO("[CLI] Leave network command sent\r\n");
    } else {
        cli_print("[CLI] Leave network cmd FAILED!\r\n");
        LOG_ERROR("[CLI] Leave network command failed\r\n");
    }
}

/**
 * @brief status命令 - 查询网络状态
 */
static void cli_cmd_status(int argc, char *argv[])
{
    cli_print("[CLI] Querying network status...\r\n");

    if (cli_send_net_cmd(CMD_NET_STATUS, NULL, 0) == 0) {
        cli_print("[CLI] Status query cmd sent OK!\r\n");
        LOG_INFO("[CLI] Status query command sent\r\n");
    } else {
        cli_print("[CLI] Status query cmd FAILED!\r\n");
        LOG_ERROR("[CLI] Status query command failed\r\n");
    }
}

/**
 * @brief version命令 - 查询模组版本
 */
static void cli_cmd_version(int argc, char *argv[])
{
    char buf[64];
    uint8_t ver = xiaoni_smart_screen_get_version();
    snprintf(buf, sizeof(buf), "[CLI] Module version: %d\r\n", ver);
    cli_print(buf);
}

/**
 * @brief reboot命令 - 软复位MCU
 */
static void cli_cmd_reboot(int argc, char *argv[])
{
    cli_print("[CLI] System rebooting...\r\n");
    HAL_Delay(100);
    NVIC_SystemReset();
}

/**
 * @brief reset命令 - 发送模组重置/恢复出厂设置命令
 */
#define CMD_NET_RESET  0x83    /* 模组重置命令 */
static void cli_cmd_reset(int argc, char *argv[])
{
    cli_print("[CLI] Sending module reset cmd...\r\n");
    unsigned char reset_status = 1;
    if (cli_send_net_cmd(CMD_NET_RESET, &reset_status, 1) == 0) {
        cli_print("[CLI] Module reset cmd sent OK!\r\n");
        LOG_INFO("[CLI] Module reset command sent\r\n");
    } else {
        cli_print("[CLI] Module reset cmd FAILED!\r\n");
        LOG_ERROR("[CLI] Module reset command failed\r\n");
    }
}

static void cli_cmd_report(int argc, char *argv[])
{
    Frame_t frame;
    char buf[96];

    if (argc != 3) {
        cli_print("[CLI] Usage: report cmd data\r\n");
        cli_print("[CLI] Example: report 88 02010101\r\n");
        return;
    }

    if (!cli_parse_hex_byte(argv[1], &frame.command)) {
        cli_print("[CLI] Invalid cmd, use 1 byte hex like 88\r\n");
        return;
    }

    if (!cli_parse_hex_data(argv[2], frame.data, &frame.data_len)) {
        cli_print("[CLI] Invalid data, use even-length hex string without 0x\r\n");
        return;
    }

    frame.header = FRAME_HEADER;
    frame.version = xiaoni_smart_screen_get_version();

    snprintf(buf, sizeof(buf), "[CLI] Reporting frame: CMD=0x%02X LEN=%u\r\n",
             frame.command, frame.data_len);
    cli_print(buf);

    xiaomi_smart_screen_report_frame(&frame);
}

static void cli_cmd_mi_switch(int argc, char *argv[])
{
    HOOCH_PROTOCOL_DimmerLightKey_t key;
    HOOCH_PROTOCOL_DimmerLightSwitchState_t state;
    HOOCH_PROTOCOL_DimmerLightResult_t result;

    if (argc != 3) {
        cli_print("[CLI] Usage: mi_switch key state\r\n");
        cli_print("[CLI] Example: mi_switch 1 1\r\n");
        return;
    }

    if (!cli_parse_dimmer_key(argv[1], &key)) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_dimmer_switch_state(argv[2], &state)) {
        cli_print("[CLI] Invalid state, use 0(off) or 1(on)\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_DimmerLight_SendSwitch(key, state);
    cli_print_dimmer_send_result("mi_switch", result);
}

static void cli_cmd_mi_brightness(int argc, char *argv[])
{
    HOOCH_PROTOCOL_DimmerLightKey_t key;
    uint8_t brightness;
    HOOCH_PROTOCOL_DimmerLightResult_t result;

    if (argc != 3) {
        cli_print("[CLI] Usage: mi_brightness key brightness\r\n");
        cli_print("[CLI] Example: mi_brightness 1 80\r\n");
        return;
    }

    if (!cli_parse_dimmer_key(argv[1], &key)) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[2], &brightness)) {
        cli_print("[CLI] Invalid brightness, use 0~255\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_DimmerLight_SendBrightness(key, brightness);
    cli_print_dimmer_send_result("mi_brightness", result);
}

static void cli_cmd_mi_color_temp(int argc, char *argv[])
{
    HOOCH_PROTOCOL_DimmerLightKey_t key;
    uint8_t color_temp;
    HOOCH_PROTOCOL_DimmerLightResult_t result;

    if (argc != 3) {
        cli_print("[CLI] Usage: mi_color_temp key color_temp\r\n");
        cli_print("[CLI] Example: mi_color_temp 1 50\r\n");
        return;
    }

    if (!cli_parse_dimmer_key(argv[1], &key)) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[2], &color_temp)) {
        cli_print("[CLI] Invalid color temp, use 0~255\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_DimmerLight_SendColorTemperature(key, color_temp);
    cli_print_dimmer_send_result("mi_color_temp", result);
}

static void cli_cmd_mi_curtain_switch(int argc, char *argv[])
{
    HOOCH_PROTOCOL_CurtainKey_t key;
    uint8_t value;
    HOOCH_PROTOCOL_CurtainResult_t result;

    if (argc != 3) {
        cli_print("[CLI] Usage: mi_curtain_switch key value\r\n");
        cli_print("[CLI] Example: mi_curtain_switch 1 1\r\n");
        return;
    }

    if (!cli_parse_curtain_key(argv[1], &key)) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[2], &value)) {
        cli_print("[CLI] Invalid value, use 0~255\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_Curtain_SendSwitch(key, (HOOCH_PROTOCOL_CurtainSwitchState_t)value);
    cli_print_curtain_send_result("mi_curtain_switch", result);
}

static void cli_cmd_mi_curtain_stop(int argc, char *argv[])
{
    HOOCH_PROTOCOL_CurtainKey_t key;
    uint8_t value;
    HOOCH_PROTOCOL_CurtainResult_t result;

    if (argc != 3) {
        cli_print("[CLI] Usage: mi_curtain_stop key value\r\n");
        cli_print("[CLI] Example: mi_curtain_stop 1 1\r\n");
        return;
    }

    if (!cli_parse_curtain_key(argv[1], &key)) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[2], &value)) {
        cli_print("[CLI] Invalid value, use 0~255\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_Curtain_SendStop(key, value);
    cli_print_curtain_send_result("mi_curtain_stop", result);
}

static void cli_cmd_mi_curtain_percent(int argc, char *argv[])
{
    HOOCH_PROTOCOL_CurtainKey_t key;
    uint8_t percent;
    HOOCH_PROTOCOL_CurtainResult_t result;

    if (argc != 3) {
        cli_print("[CLI] Usage: mi_curtain_percent key percent\r\n");
        cli_print("[CLI] Example: mi_curtain_percent 1 80\r\n");
        return;
    }

    if (!cli_parse_curtain_key(argv[1], &key)) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[2], &percent)) {
        cli_print("[CLI] Invalid percent, use 0~255\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_Curtain_SendPercent(key, percent);
    cli_print_curtain_send_result("mi_curtain_percent", result);
}

static void cli_cmd_mi_curtain_angle(int argc, char *argv[])
{
    HOOCH_PROTOCOL_CurtainKey_t key;
    uint8_t angle;
    HOOCH_PROTOCOL_CurtainResult_t result;

    if (argc != 3) {
        cli_print("[CLI] Usage: mi_curtain_angle key angle\r\n");
        cli_print("[CLI] Example: mi_curtain_angle 1 50\r\n");
        return;
    }

    if (!cli_parse_curtain_key(argv[1], &key)) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[2], &angle)) {
        cli_print("[CLI] Invalid angle, use 0~255\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_Curtain_SendAngle(key, angle);
    cli_print_curtain_send_result("mi_curtain_angle", result);
}

static void cli_cmd_mi_key_status(int argc, char *argv[])
{
    HOOCH_PROTOCOL_KeyStatusKey_t key;
    HOOCH_PROTOCOL_KeyStatusState_t state;
    HOOCH_PROTOCOL_KeyStatusResult_t result;

    if (argc != 3) {
        cli_print("[CLI] Usage: mi_key_status key state\r\n");
        cli_print("[CLI] Example: mi_key_status 1 1\r\n");
        return;
    }

    if (!cli_parse_key_status_key(argv[1], &key)) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_key_status_state(argv[2], &state)) {
        cli_print("[CLI] Invalid state, use 0(off) or 1(on)\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_KeyStatus_ReportState(key, state);
    cli_print_key_status_send_result("mi_key_status", result);
}

static void cli_cmd_mi_key_click(int argc, char *argv[])
{
    HOOCH_PROTOCOL_KeyClickReportKey_t key;
    HOOCH_PROTOCOL_KeyClickReportEvent_t event;
    HOOCH_PROTOCOL_KeyClickReportState_t state;
    HOOCH_PROTOCOL_KeyClickReportResult_t result;

    if (argc != 4) {
        cli_print("[CLI] Usage: mi_key_click key event state\r\n");
        cli_print("[CLI] Example: mi_key_click 1 1 1\r\n");
        cli_print("[CLI] Event: 1(single) 2(double) 3(long) 4(long_1p5s)\r\n");
        return;
    }

    if (!cli_parse_key_click_key(argv[1], &key)) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_key_click_event(argv[2], &event)) {
        cli_print("[CLI] Invalid event, use 1(single)~4(long_1p5s)\r\n");
        return;
    }

    if (!cli_parse_key_click_state(argv[3], &state)) {
        cli_print("[CLI] Invalid state, use 0(off) or 1(on)\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_KeyClickReport_Send(key, event, state);
    cli_print_key_click_send_result("mi_key_click", result);
}

static void cli_cmd_mi_setting_report(int argc, char *argv[])
{
    HOOCH_PROTOCOL_SettingReportEvent_t event;
    HOOCH_PROTOCOL_SettingResult_t result;
    uint8_t value;
    const void *data;
    uint16_t data_len;

    if (argc != 2 && argc != 3) {
        cli_print("[CLI] Usage: mi_setting_report event [value]\r\n");
        cli_print("[CLI] Example: mi_setting_report 8 1\r\n");
        cli_print("[CLI] Event 8 means update_config\r\n");
        return;
    }

    if (!cli_parse_setting_report_event(argv[1], &event)) {
        cli_print("[CLI] Invalid event, use 1~24\r\n");
        return;
    }

    data = 0;
    data_len = 0U;

    if (argc == 3) {
        if (!cli_parse_u8_arg(argv[2], &value)) {
            cli_print("[CLI] Invalid value, use 0~255\r\n");
            return;
        }

        data = &value;
        data_len = 1U;
    }

    result = HOOCH_PROTOCOL_SettingReport_Send(event, data, data_len);
    cli_print_setting_report_send_result("mi_setting_report", result);
}

static void cli_cmd_mi_human_presence(int argc, char *argv[])
{
    HOOCH_PROTOCOL_SettingResult_t result;
    uint8_t state;

    if (argc != 2) {
        cli_print("[CLI] Usage: mi_human_presence <state:0|1>\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[1], &state)) {
        cli_print("[CLI] Invalid state, use 0 or 1\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_SettingReport_Send(
        HOOCH_PROTOCOL_SETTING_REPORT_EVENT_HUMAN_PRESENCE,
        &state,
        sizeof(state));
    cli_print_setting_report_send_result("mi_human_presence", result);
}

static void cli_cmd_mi_get_weather(int argc, char *argv[])
{
    HOOCH_PROTOCOL_SettingResult_t result;

    (void)argc;
    (void)argv;
   int sssss = 0;
    result = HOOCH_PROTOCOL_SettingReport_Send(
        HOOCH_PROTOCOL_SETTING_REPORT_EVENT_GET_WEATHER,
        &sssss,
        1);
    cli_print_setting_report_send_result("mi_get_weather", result);
}

static void cli_cmd_mi_air_power(int argc, char *argv[])
{
    uint8_t channel;
    HOOCH_PROTOCOL_AirConditionerPower_t power;
    HOOCH_PROTOCOL_AirConditionerResult_t result;

    if (argc != 3) {
        cli_print("[CLI] Usage: mi_air_power channel power\r\n");
        cli_print("[CLI] Example: mi_air_power 1 1\r\n");
        return;
    }

    if (!cli_parse_air_channel(argv[1], &channel)) {
        cli_print("[CLI] Invalid channel, use 0~255\r\n");
        return;
    }

    if (!cli_parse_air_power(argv[2], &power)) {
        cli_print("[CLI] Invalid power, use 0(off) or 1(on)\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_AirConditioner_SendPower(channel, power);
    cli_print_air_send_result("mi_air_power", result);
}

static void cli_cmd_mi_air_mode(int argc, char *argv[])
{
    uint8_t channel;
    HOOCH_PROTOCOL_AirConditionerMode_t mode;
    HOOCH_PROTOCOL_AirConditionerResult_t result;

    if (argc != 3) {
        cli_print("[CLI] Usage: mi_air_mode channel mode\r\n");
        cli_print("[CLI] Example: mi_air_mode 1 1\r\n");
        cli_print("[CLI] Mode: 1(cool) 2(fan) 3(dry) 4(heat) 5(auto)\r\n");
        return;
    }

    if (!cli_parse_air_channel(argv[1], &channel)) {
        cli_print("[CLI] Invalid channel, use 0~255\r\n");
        return;
    }

    if (!cli_parse_air_mode(argv[2], &mode)) {
        cli_print("[CLI] Invalid mode, use 1(cool)~5(auto)\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_AirConditioner_SendMode(channel, mode);
    cli_print_air_send_result("mi_air_mode", result);
}

static void cli_cmd_mi_air_fan_speed(int argc, char *argv[])
{
    uint8_t channel;
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed;
    HOOCH_PROTOCOL_AirConditionerResult_t result;

    if (argc != 3) {
        cli_print("[CLI] Usage: mi_air_fan_speed channel fan_speed\r\n");
        cli_print("[CLI] Example: mi_air_fan_speed 1 1\r\n");
        cli_print("[CLI] Fan: 1(low) 2(medium) 3(high) 4(auto)\r\n");
        return;
    }

    if (!cli_parse_air_channel(argv[1], &channel)) {
        cli_print("[CLI] Invalid channel, use 0~255\r\n");
        return;
    }

    if (!cli_parse_air_fan_speed(argv[2], &fan_speed)) {
        cli_print("[CLI] Invalid fan speed, use 1(low)~4(auto)\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_AirConditioner_SendFanSpeed(channel, fan_speed);
    cli_print_air_send_result("mi_air_fan_speed", result);
}

static void cli_cmd_mi_air_temperature(int argc, char *argv[])
{
    uint8_t channel;
    uint8_t temperature;
    HOOCH_PROTOCOL_AirConditionerResult_t result;

    if (argc != 3) {
        cli_print("[CLI] Usage: mi_air_temperature channel temperature\r\n");
        cli_print("[CLI] Example: mi_air_temperature 1 26\r\n");
        return;
    }

    if (!cli_parse_air_channel(argv[1], &channel)) {
        cli_print("[CLI] Invalid channel, use 0~255\r\n");
        return;
    }

    if (!cli_parse_air_temperature(argv[2], &temperature)) {
        cli_print("[CLI] Invalid temperature, use 16~30\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_AirConditioner_SendTemperature(channel, temperature);
    cli_print_air_send_result("mi_air_temperature", result);
}

static void cli_cmd_mi_air_frame(int argc, char *argv[])
{
    uint8_t channel;
    HOOCH_PROTOCOL_AirConditionerPower_t power;
    HOOCH_PROTOCOL_AirConditionerMode_t mode;
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed;
    uint8_t temperature;
    HOOCH_PROTOCOL_AirConditionerResult_t result;

    if (argc < 6) {
        cli_print("[CLI] Usage: mi_air_frame channel power mode fan_speed temperature\r\n");
        cli_print("[CLI] Example: mi_air_frame 1 1 1 1 26\r\n");
        cli_print("[CLI] Power: 0(off) 1(on), Mode: 1(cool)~5(auto), Fan: 1(low)~4(auto), Temp: 16~30\r\n");
        return;
    }

    if (!cli_parse_air_channel(argv[1], &channel)) {
        cli_print("[CLI] Invalid channel, use 0~255\r\n");
        return;
    }

    if (!cli_parse_air_power(argv[2], &power)) {
        cli_print("[CLI] Invalid power, use 0(off) or 1(on)\r\n");
        return;
    }

    if (!cli_parse_air_mode(argv[3], &mode)) {
        cli_print("[CLI] Invalid mode, use 1(cool)~5(auto)\r\n");
        return;
    }

    if (!cli_parse_air_fan_speed(argv[4], &fan_speed)) {
        cli_print("[CLI] Invalid fan speed, use 1(low)~4(auto)\r\n");
        return;
    }

    if (!cli_parse_air_temperature(argv[5], &temperature)) {
        cli_print("[CLI] Invalid temperature, use 16~30\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_AirConditioner_SendFrame(channel, power, mode, fan_speed, temperature);
    cli_print_air_send_result("mi_air_frame", result);
}

static void cli_cmd_mi_air_frame_addr(int argc, char *argv[])
{
    uint16_t address;
    HOOCH_PROTOCOL_AirConditionerPower_t power;
    HOOCH_PROTOCOL_AirConditionerMode_t mode;
    HOOCH_PROTOCOL_AirConditionerFanSpeed_t fan_speed;
    uint8_t temperature;
    HOOCH_PROTOCOL_AirConditionerResult_t result;

    if (argc < 6) {
        cli_print("[CLI] Usage: mi_air_frame_addr address power mode fan_speed temperature\r\n");
        cli_print("[CLI] Example: mi_air_frame_addr 0x1234 1 1 1 26\r\n");
        cli_print("[CLI] Power: 0(off) 1(on), Mode: 1(cool)~5(auto), Fan: 1(low)~4(auto), Temp: 16~30\r\n");
        return;
    }

    if (!cli_parse_air_address(argv[1], &address)) {
        cli_print("[CLI] Invalid address, use 0~65535\r\n");
        return;
    }

    if (!cli_parse_air_power(argv[2], &power)) {
        cli_print("[CLI] Invalid power, use 0(off) or 1(on)\r\n");
        return;
    }

    if (!cli_parse_air_mode(argv[3], &mode)) {
        cli_print("[CLI] Invalid mode, use 1(cool)~5(auto)\r\n");
        return;
    }

    if (!cli_parse_air_fan_speed(argv[4], &fan_speed)) {
        cli_print("[CLI] Invalid fan speed, use 1(low)~4(auto)\r\n");
        return;
    }

    if (!cli_parse_air_temperature(argv[5], &temperature)) {
        cli_print("[CLI] Invalid temperature, use 16~30\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_AirConditioner_SendFrameAddr(address, power, mode, fan_speed, temperature);
    cli_print_air_send_result("mi_air_frame_addr", result);
}

/* ---- 调光页面上报（页面+通道+数值） ---- */

static void cli_cmd_mi_light_switch(int argc, char *argv[])
{
    uint8_t page;
    HOOCH_PROTOCOL_DimmerLightKey_t key;
    HOOCH_PROTOCOL_DimmerLightSwitchState_t state;
    HOOCH_PROTOCOL_DimmerLightResult_t result;

    if (argc != 4) {
        cli_print("[CLI] Usage: mi_light_switch page key state\r\n");
        cli_print("[CLI] Example: mi_light_switch 2 1 1\r\n");
        cli_print("[CLI] page: 2=light, key: 1~8, state: 0=off, 1=on\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[1], &page)) {
        cli_print("[CLI] Invalid page\r\n");
        return;
    }

    if (!cli_parse_dimmer_key(argv[2], &key)) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_dimmer_switch_state(argv[3], &state)) {
        cli_print("[CLI] Invalid state, use 0(off) or 1(on)\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_DimmerLight_SendSwitchPage(page, key, state);
    cli_print_dimmer_send_result("mi_light_switch", result);
}

static void cli_cmd_mi_light_brightness(int argc, char *argv[])
{
    uint8_t page;
    HOOCH_PROTOCOL_DimmerLightKey_t key;
    uint8_t brightness;
    HOOCH_PROTOCOL_DimmerLightResult_t result;

    if (argc != 4) {
        cli_print("[CLI] Usage: mi_light_brightness page key brightness\r\n");
        cli_print("[CLI] Example: mi_light_brightness 2 1 80\r\n");
        cli_print("[CLI] page: 2=light, key: 1~8, brightness: 0~255\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[1], &page)) {
        cli_print("[CLI] Invalid page\r\n");
        return;
    }

    if (!cli_parse_dimmer_key(argv[2], &key)) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[3], &brightness)) {
        cli_print("[CLI] Invalid brightness, use 0~255\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_DimmerLight_SendBrightnessPage(page, key, brightness);
    cli_print_dimmer_send_result("mi_light_brightness", result);
}

static void cli_cmd_mi_light_color_temp(int argc, char *argv[])
{
    uint8_t page;
    HOOCH_PROTOCOL_DimmerLightKey_t key;
    uint8_t color_temp;
    HOOCH_PROTOCOL_DimmerLightResult_t result;

    if (argc != 4) {
        cli_print("[CLI] Usage: mi_light_color_temp page key color_temp\r\n");
        cli_print("[CLI] Example: mi_light_color_temp 2 1 50\r\n");
        cli_print("[CLI] page: 2=light, key: 1~8, color_temp: 0~255\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[1], &page)) {
        cli_print("[CLI] Invalid page\r\n");
        return;
    }

    if (!cli_parse_dimmer_key(argv[2], &key)) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[3], &color_temp)) {
        cli_print("[CLI] Invalid color_temp, use 0~255\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_DimmerLight_SendColorTemperaturePage(page, key, color_temp);
    cli_print_dimmer_send_result("mi_light_color_temp", result);
}

/* ---- 窗帘页面上报（页面+通道+数值） ---- */

static void cli_cmd_mi_curtain_switch_page(int argc, char *argv[])
{
    uint8_t page;
    HOOCH_PROTOCOL_CurtainKey_t key;
    uint8_t value;
    HOOCH_PROTOCOL_CurtainResult_t result;

    if (argc != 4) {
        cli_print("[CLI] Usage: mi_curtain_switch_page page key state\r\n");
        cli_print("[CLI] Example: mi_curtain_switch_page 3 1 1\r\n");
        cli_print("[CLI] page: 3=curtain, key: 1~8, state: 0=off, 1=on\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[1], &page)) {
        cli_print("[CLI] Invalid page\r\n");
        return;
    }

    if (!cli_parse_curtain_key(argv[2], &key)) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[3], &value)) {
        cli_print("[CLI] Invalid state, use 0(off) or 1(on)\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_Curtain_SendSwitchPage(page, key, (HOOCH_PROTOCOL_CurtainSwitchState_t)value);
    cli_print_curtain_send_result("mi_curtain_switch_page", result);
}

static void cli_cmd_mi_curtain_stop_page(int argc, char *argv[])
{
    uint8_t page;
    HOOCH_PROTOCOL_CurtainKey_t key;
    uint8_t value;
    HOOCH_PROTOCOL_CurtainResult_t result;

    if (argc != 4) {
        cli_print("[CLI] Usage: mi_curtain_stop_page page key value\r\n");
        cli_print("[CLI] Example: mi_curtain_stop_page 3 1 1\r\n");
        cli_print("[CLI] page: 3=curtain, key: 1~8, value: 0=stop, 1=running\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[1], &page)) {
        cli_print("[CLI] Invalid page\r\n");
        return;
    }

    if (!cli_parse_curtain_key(argv[2], &key)) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[3], &value)) {
        cli_print("[CLI] Invalid value, use 0~255\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_Curtain_SendStopPage(page, key, value);
    cli_print_curtain_send_result("mi_curtain_stop_page", result);
}

static void cli_cmd_mi_curtain_percent_page(int argc, char *argv[])
{
    uint8_t page;
    HOOCH_PROTOCOL_CurtainKey_t key;
    uint8_t percent;
    HOOCH_PROTOCOL_CurtainResult_t result;

    if (argc != 4) {
        cli_print("[CLI] Usage: mi_curtain_percent_page page key percent\r\n");
        cli_print("[CLI] Example: mi_curtain_percent_page 3 1 80\r\n");
        cli_print("[CLI] page: 3=curtain, key: 1~8, percent: 0~255\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[1], &page)) {
        cli_print("[CLI] Invalid page\r\n");
        return;
    }

    if (!cli_parse_curtain_key(argv[2], &key)) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[3], &percent)) {
        cli_print("[CLI] Invalid percent, use 0~255\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_Curtain_SendPercentPage(page, key, percent);
    cli_print_curtain_send_result("mi_curtain_percent_page", result);
}

static void cli_cmd_mi_curtain_angle_page(int argc, char *argv[])
{
    uint8_t page;
    HOOCH_PROTOCOL_CurtainKey_t key;
    uint8_t angle;
    HOOCH_PROTOCOL_CurtainResult_t result;

    if (argc != 4) {
        cli_print("[CLI] Usage: mi_curtain_angle_page page key angle\r\n");
        cli_print("[CLI] Example: mi_curtain_angle_page 3 1 50\r\n");
        cli_print("[CLI] page: 3=curtain, key: 1~8, angle: 0~255\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[1], &page)) {
        cli_print("[CLI] Invalid page\r\n");
        return;
    }

    if (!cli_parse_curtain_key(argv[2], &key)) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[3], &angle)) {
        cli_print("[CLI] Invalid angle, use 0~255\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_Curtain_SendAnglePage(page, key, angle);
    cli_print_curtain_send_result("mi_curtain_angle_page", result);
}

static void cli_cmd_mi_code_switch(int argc, char *argv[])
{
    uint8_t key;
    uint8_t mode;
    HOOCH_PROTOCOL_CodeMatchReportAction_t action;
    HOOCH_PROTOCOL_CodeMatchReportResult_t result;
    uint8_t val;

    if (argc != 4) {
        cli_print("[CLI] Usage: mi_code_switch key mode action\r\n");
        cli_print("[CLI] Example: mi_code_switch 1 0 1\r\n");
        cli_print("[CLI] key: 1~4, mode: 0(dimmer) 1(curtain), action: 0(clear) 1(match)\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[1], &val) || val < 1U || val > 4U) {
        cli_print("[CLI] Invalid key, use 1~4\r\n");
        return;
    }
    key = val;

    if (!cli_parse_u8_arg(argv[2], &val) || val > 1U) {
        cli_print("[CLI] Invalid mode, use 0(dimmer) or 1(curtain)\r\n");
        return;
    }
    mode = val;

    if (!cli_parse_u8_arg(argv[3], &val) || val > 1U) {
        cli_print("[CLI] Invalid action, use 0(clear) or 1(match)\r\n");
        return;
    }
    action = (HOOCH_PROTOCOL_CodeMatchReportAction_t)val;

    result = HOOCH_PROTOCOL_CodeMatchReport_SendSwitch(
        HOOCH_PROTOCOL_CODE_MATCH_PAGE_SWITCH, key, mode, action);
    cli_print_code_match_send_result("mi_code_switch", result);
}

static void cli_cmd_mi_code_page(int argc, char *argv[])
{
    HOOCH_PROTOCOL_CodeMatchPage_t page;
    HOOCH_PROTOCOL_CodeMatchReportAction_t action;
    HOOCH_PROTOCOL_CodeMatchReportResult_t result;
    uint8_t val;
    uint8_t index;

    if (argc != 4) {
        cli_print("[CLI] Usage: mi_code_page page index action\r\n");
        cli_print("[CLI] Example: mi_code_page 2 0 1\r\n");
        cli_print("[CLI] page: 2(light) 3(curtain) 4(ac), action: 0(clear) 1(match)\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[1], &val) || val < 2U || val > 4U) {
        cli_print("[CLI] Invalid page, use 2(light) 3(curtain) 4(ac)\r\n");
        return;
    }
    page = (HOOCH_PROTOCOL_CodeMatchPage_t)val;

    if (!cli_parse_u8_arg(argv[2], &index)) {
        cli_print("[CLI] Invalid index\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[3], &val) || val > 1U) {
        cli_print("[CLI] Invalid action, use 0(clear) or 1(match)\r\n");
        return;
    }
    action = (HOOCH_PROTOCOL_CodeMatchReportAction_t)val;

    result = HOOCH_PROTOCOL_CodeMatchReport_Send((uint8_t)page, index, action);
    cli_print_code_match_send_result("mi_code_page", result);
}

/* ======================== 情景上报 ======================== */

static void cli_print_scene_send_result(
    const char *cmd_name,
    HOOCH_PROTOCOL_SceneReportResult_t result)
{
    char buf[96];

    snprintf(buf, sizeof(buf), "[CLI] %s result: %u\r\n", cmd_name, (unsigned int)result);
    cli_print(buf);

    if (result == HOOCH_PROTOCOL_SCENE_REPORT_RESULT_OK) {
        LOG_INFO("[CLI] Scene report command sent\r\n");
    } else {
        LOG_ERROR("[CLI] Scene report command failed\r\n");
    }
}

static void cli_cmd_mi_scene_report(int argc, char *argv[])
{
    HOOCH_PROTOCOL_SceneReportKey_t key;
    HOOCH_PROTOCOL_SceneReportResult_t result;
    uint8_t key_val;

    if (argc != 2) {
        cli_print("[CLI] Usage: tuya_scene key\r\n");
        cli_print("[CLI] Example: tuya_scene 1\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[1], &key_val) || key_val < 1U || key_val > 8U) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    key = (HOOCH_PROTOCOL_SceneReportKey_t)key_val;
    result = HOOCH_PROTOCOL_SceneReport_Send(key);
    cli_print_scene_send_result("tuya_scene", result);
}

static void cli_cmd_mi_scene_report_xiaomi(int argc, char *argv[])
{
    HOOCH_PROTOCOL_SceneReportKey_t key;
    HOOCH_PROTOCOL_SceneReportType_t type;
    HOOCH_PROTOCOL_SceneReportResult_t result;
    uint8_t page_val;
    uint8_t key_val;
    uint8_t type_val;

    if (argc != 4) {
        cli_print("[CLI] Usage: mi_scene_report page key type\r\n");
        cli_print("[CLI] Example: mi_scene_report 1 1 1\r\n");
        cli_print("[CLI] type: 1=click, 2=double, 3=long\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[1], &page_val)) {
        cli_print("[CLI] Invalid page value\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[2], &key_val) || key_val < 1U || key_val > 8U) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[3], &type_val) ||
        type_val < (uint8_t)HOOCH_PROTOCOL_SCENE_REPORT_TYPE_SINGLE_CLICK ||
        type_val > (uint8_t)HOOCH_PROTOCOL_SCENE_REPORT_TYPE_LONG_CLICK) {
        cli_print("[CLI] Invalid type, use 1=click, 2=double, 3=long\r\n");
        return;
    }

    key = (HOOCH_PROTOCOL_SceneReportKey_t)key_val;
    type = (HOOCH_PROTOCOL_SceneReportType_t)type_val;
    result = HOOCH_PROTOCOL_SceneReport_SendXiaomi(page_val, key, type);
    cli_print_scene_send_result("mi_scene_report", result);
}

static void cli_cmd_mi_screen_read(int argc, char *argv[])
{
    uint8_t dp_val;

    if (argc != 2) {
        cli_print("[CLI] Usage: mi_screen_read dp\r\n");
        cli_print("[CLI] Example: mi_screen_read 2\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[1], &dp_val)) {
        cli_print("[CLI] Invalid dp value\r\n");
        return;
    }

    xiaoni_smart_screen_send_report_status((xiaomi_screen_subcmd_t)dp_val);
    cli_print("[CLI] mi_screen_read sent\r\n");
}

/* ======================== 公共接口函数 ======================== */

/**
 * @brief CLI初始化
 */
void Hooch_CLI_Init(void)
{
    cli_rx_index = 0;
    cli_cmd_ready = 0;
    memset(cli_rx_buf, 0, CLI_RX_BUF_SIZE);

    /* 使能UART1接收中断 */
    __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_RXNE | UART_FLAG_ORE | UART_FLAG_FE | UART_FLAG_NE);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);

    cli_print("\r\n");
    cli_print("========================================\r\n");
    cli_print("  Hooch CLI v1.0\r\n");
    cli_print("  Type 'help' for commands\r\n");
    cli_print("========================================\r\n");
    cli_print(CLI_PROMPT);
}

/**
 * @brief CLI接收字节回调(在USART1中断中调用)
 * @note  中断中只做FIFO缓存，不做任何发送操作，避免阻塞丢字符
 * @param data 接收到的字节
 */
void Hooch_CLI_RxCallback(uint8_t data)
{
    uint8_t next_head = (cli_fifo_head + 1) % CLI_FIFO_SIZE;
    if (next_head != cli_fifo_tail) {
        cli_fifo[cli_fifo_head] = data;
        cli_fifo_head = next_head;
    }
}

/**
 * @brief 从FIFO取一个字节
 * @param data 输出字节指针
 * @return 1取到数据，0无数据
 */
static uint8_t cli_fifo_pop(uint8_t *data)
{
    if (cli_fifo_head == cli_fifo_tail) return 0;
    *data = cli_fifo[cli_fifo_tail];
    cli_fifo_tail = (cli_fifo_tail + 1) % CLI_FIFO_SIZE;
    return 1;
}

/**
 * @brief CLI命令处理(在主循环中调用)
 * @note  负责从FIFO取字节、回显、组装命令行、执行命令
 */
void Hooch_CLI_Process(void)
{
    uint8_t ch;

    /* 从FIFO逐字节取出并处理 */
    while (cli_fifo_pop(&ch)) {
        if (cli_cmd_ready) continue;  /* 命令待执行，丢弃新输入 */

        if (ch == '\r' || ch == '\n') {
            if (cli_rx_index > 0) {
                cli_rx_buf[cli_rx_index] = '\0';
                cli_cmd_ready = 1;
            }
            UART1_SendChar('\r');
            UART1_SendChar('\n');
        } else if (ch == '\b' || ch == 0x7F) {
            if (cli_rx_index > 0) {
                cli_rx_index--;
                UART1_SendChar('\b');
                UART1_SendChar(' ');
                UART1_SendChar('\b');
            }
        } else {
            if (cli_rx_index < CLI_RX_BUF_SIZE - 1) {
                cli_rx_buf[cli_rx_index++] = ch;
                UART1_SendChar(ch);
            }
        }
    }

    /* 如果有命令就绪，执行 */
    if (!cli_cmd_ready) return;

    char *argv[CLI_MAX_ARGS];
    int argc;
    uint8_t cmd_found = 0;

    argc = cli_parse_args((char *)cli_rx_buf, argv);

    if (argc > 0) {
        for (uint8_t i = 0; i < CLI_CMD_COUNT; i++) {
            if (strcmp(argv[0], cli_cmd_table[i].name) == 0) {
                cli_cmd_table[i].handler(argc, argv);
                cmd_found = 1;
                break;
            }
        }

        if (!cmd_found) {
            cli_print("[CLI] Unknown cmd: ");
            cli_print(argv[0]);
            cli_print("\r\n[CLI] Type 'help' for commands\r\n");
        }
    }

    /* 重置缓冲区，打印提示符 */
    cli_rx_index = 0;
    cli_cmd_ready = 0;
    memset(cli_rx_buf, 0, CLI_RX_BUF_SIZE);
    cli_print(CLI_PROMPT);
}
