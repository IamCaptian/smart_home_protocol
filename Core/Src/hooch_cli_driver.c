/**
  ******************************************************************************
  * @file    hooch_cli_driver.c
  * @brief   基于UART1的CLI命令行驱动，仅保留涂鸦(Tuya)相关功能测试命令
  *          保留范围与 tuya_uart_init() 注册的 5 类上报一一对应：
  *            按键状态 / 调光灯 / 窗帘 / 设置上报 / 情景上报
  ******************************************************************************
  */

#include "main.h"
#include "hooch_protocol.h"
#include "usart.h"
#include "user_printf.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/* ======================== 配置定义 ======================== */
#define CLI_RX_BUF_SIZE       128     /* CLI接收缓冲区大小 */
#define CLI_MAX_ARGS          8       /* 最大参数个数 */
#define CLI_PROMPT            "hooch> "

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
static void cli_cmd_reboot(int argc, char *argv[]);
static void cli_cmd_mi_switch(int argc, char *argv[]);
static void cli_cmd_mi_brightness(int argc, char *argv[]);
static void cli_cmd_mi_color_temp(int argc, char *argv[]);
static void cli_cmd_mi_curtain_switch(int argc, char *argv[]);
static void cli_cmd_mi_curtain_stop(int argc, char *argv[]);
static void cli_cmd_mi_curtain_percent(int argc, char *argv[]);
static void cli_cmd_mi_curtain_angle(int argc, char *argv[]);
static void cli_cmd_mi_key_status(int argc, char *argv[]);
static void cli_cmd_mi_setting_report(int argc, char *argv[]);
static void cli_cmd_mi_human_presence(int argc, char *argv[]);
static void cli_cmd_mi_get_weather(int argc, char *argv[]);
static void cli_cmd_mi_scene_report(int argc, char *argv[]);

/* ======================== 命令表 ======================== */
static const cli_cmd_t cli_cmd_table[] = {
    {"help",               "Show help info",                          cli_cmd_help},
    {"reboot",             "Soft reset MCU",                          cli_cmd_reboot},
    {"mi_switch",          "Dimmer switch: mi_switch key 0|1",        cli_cmd_mi_switch},
    {"mi_brightness",      "Dimmer brightness: mi_brightness key value", cli_cmd_mi_brightness},
    {"mi_color_temp",      "Dimmer color temp: mi_color_temp key value(0-65535)", cli_cmd_mi_color_temp},
    {"mi_curtain_switch",  "Curtain switch: mi_curtain_switch key value", cli_cmd_mi_curtain_switch},
    {"mi_curtain_stop",    "Curtain stop: mi_curtain_stop key value", cli_cmd_mi_curtain_stop},
    {"mi_curtain_percent", "Curtain percent: mi_curtain_percent key percent", cli_cmd_mi_curtain_percent},
    {"mi_curtain_angle",   "Curtain angle: mi_curtain_angle key angle", cli_cmd_mi_curtain_angle},
    {"mi_key_status",      "Key status: mi_key_status key 0|1",       cli_cmd_mi_key_status},
    {"mi_setting_report",  "Setting report: mi_setting_report event [value]", cli_cmd_mi_setting_report},
    {"mi_human_presence",  "Human presence: mi_human_presence 0|1",   cli_cmd_mi_human_presence},
    {"mi_get_weather",     "Get weather data: mi_get_weather",        cli_cmd_mi_get_weather},
    {"mi_scene_report",    "Scene report: mi_scene_report key:1~8",   cli_cmd_mi_scene_report},
};

#define CLI_CMD_COUNT  (sizeof(cli_cmd_table) / sizeof(cli_cmd_table[0]))

/* ======================== 私有函数 ======================== */

/**
 * @brief 通过UART1打印CLI输出
 */
static void cli_print(const char *str)
{
    UART1_SendString(str);
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

static uint8_t cli_parse_u16_arg(const char *str, uint16_t *value)
{
    char *endptr;
    unsigned long parsed_value;

    if (str == NULL || value == NULL) {
        return 0;
    }

    parsed_value = strtoul(str, &endptr, 0);
    if (str[0] == '\0' || endptr == NULL || *endptr != '\0' || parsed_value > 0xFFFFUL) {
        return 0;
    }

    *value = (uint16_t)parsed_value;
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

static uint8_t cli_parse_setting_report_event(
    const char *str,
    HOOCH_PROTOCOL_SettingReportEvent_t *event)
{
    uint8_t event_value;

    if (!cli_parse_u8_arg(str, &event_value)) {
        return 0;
    }

    if (event_value <= (uint8_t)HOOCH_PROTOCOL_SETTING_REPORT_EVENT_INVALID ||
        event_value >= (uint8_t)HOOCH_PROTOCOL_SETTING_REPORT_EVENT_COUNT) {
        return 0;
    }

    *event = (HOOCH_PROTOCOL_SettingReportEvent_t)event_value;
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
 * @brief reboot命令 - 软复位MCU
 */
static void cli_cmd_reboot(int argc, char *argv[])
{
    cli_print("[CLI] System rebooting...\r\n");
    HAL_Delay(100);
    NVIC_SystemReset();
}

/* ---- 按键状态上报（涂鸦: 继电器/开关 DP） ---- */

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

/* ---- 调光灯上报（涂鸦: 群组 ZCL 开关/亮度/色温） ---- */

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
    uint16_t color_temp;
    HOOCH_PROTOCOL_DimmerLightResult_t result;
    const HOOCH_PROTOCOL_DimmerLightFrame_t *frame;
    char buf[96];

    if (argc != 3) {
        cli_print("[CLI] Usage: mi_color_temp key color_temp\r\n");
        cli_print("[CLI] Example: mi_color_temp 1 50\r\n");
        return;
    }

    if (!cli_parse_dimmer_key(argv[1], &key)) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_u16_arg(argv[2], &color_temp)) {
        cli_print("[CLI] Invalid color_temp, use 0~65535\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_DimmerLight_SendColorTemperature(key, color_temp);
    cli_print_dimmer_send_result("mi_color_temp", result);

    frame = HOOCH_PROTOCOL_DimmerLight_GetFrame();
    if (frame != 0) {
        snprintf(buf, sizeof(buf),
                 "[CLI] frame: key=%u ctemp=%u item=%u seq=%u valid=%u\r\n",
                 (unsigned int)frame->key,
                 (unsigned int)frame->color_temperature,
                 (unsigned int)frame->control_item,
                 (unsigned int)frame->sequence,
                 (unsigned int)frame->valid);
        cli_print(buf);
    }
}

/* ---- 窗帘上报（涂鸦: 群组 DP 开关/百分比） ---- */

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

/* ---- 设置上报（涂鸦: 配网/时间/天气/人体存在等设置事件） ---- */

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
        cli_print("[CLI] Tuya events: 1=join 2=leave 3=reset 6=time 11=weather 18=wind 19=city 20=area\r\n");
        return;
    }

    if (!cli_parse_setting_report_event(argv[1], &event)) {
        cli_print("[CLI] Invalid event, use 1~20\r\n");
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

    result = HOOCH_PROTOCOL_SettingReport_Send(
        HOOCH_PROTOCOL_SETTING_REPORT_EVENT_GET_WEATHER,
        NULL,
        0U);
    cli_print_setting_report_send_result("mi_get_weather", result);
}

/* ---- 情景上报（涂鸦: 场景1~8 DP） ---- */

static void cli_cmd_mi_scene_report(int argc, char *argv[])
{
    HOOCH_PROTOCOL_SceneReportKey_t key;
    HOOCH_PROTOCOL_SceneReportResult_t result;
    uint8_t key_val;

    if (argc != 2) {
        cli_print("[CLI] Usage: mi_scene_report key\r\n");
        cli_print("[CLI] Example: mi_scene_report 1\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[1], &key_val) || key_val < 1U || key_val > 8U) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    key = (HOOCH_PROTOCOL_SceneReportKey_t)key_val;
    result = HOOCH_PROTOCOL_SceneReport_Send(key);
    cli_print_scene_send_result("mi_scene_report", result);
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
