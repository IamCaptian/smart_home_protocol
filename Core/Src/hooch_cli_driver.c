/**
  ******************************************************************************
  * @file    hooch_cli_driver.c
  * @brief   基于UART1的CLI命令行驱动，仅保留涂鸦(Tuya)配网与情景上报测试命令
  *            tuya_join  : 发起配网（HOOCH_PROTOCOL_SETTING_REPORT_EVENT_NETWORK_JOIN）
  *            tuya_scene : 情景上报（HOOCH_PROTOCOL_SceneReport_Send，1~8）
  *            tuya_key   : 按键开关上报（HOOCH_PROTOCOL_KeyStatus_ReportState，1~8 / 0或1）
  *            tuya_unsup : 不支持属性上报（HOOCH_PROTOCOL_UnsupportedAttribute_SendAttributes，位掩码 bit0~bit9）
  ******************************************************************************
  */

#include "main.h"
#include "hooch_protocol.h"
#include "xiaomi_smart_screen_86box_buffer.h"
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
static void cli_cmd_tuya_join(int argc, char *argv[]);
static void cli_cmd_tuya_scene(int argc, char *argv[]);
static void cli_cmd_tuya_key(int argc, char *argv[]);
static void cli_cmd_tuya_unsup(int argc, char *argv[]);
static void cli_cmd_xm86_reset(int argc, char *argv[]);
static void cli_cmd_xm86_reset_module(int argc, char *argv[]);

/* ======================== 命令表 ======================== */
static const cli_cmd_t cli_cmd_table[] = {
    // {"tuya_join",          "Tuya start network join",                        cli_cmd_tuya_join},
    // {"tuya_scene",         "Scene report: tuya_scene key (1~8)",             cli_cmd_tuya_scene},
    // {"tuya_key",           "Key switch report: tuya_key key(1~8) state(0/1)", cli_cmd_tuya_key},
    // {"tuya_unsup",         "Unsupported attr report: tuya_unsup mask(bit0~bit9)", cli_cmd_tuya_unsup},
    {"xm86_reset",         "86box report reset status (0x86)",               cli_cmd_xm86_reset},
    {"xm86_reset_module",  "86box reset module (0x83, clears pairing)",       cli_cmd_xm86_reset_module},
};
/*
bit0 不支持空调 0x001 `tuya_unsup 0x001` 
bit1 不支持新风 0x002 `tuya_unsup 0x002` 
bit2 不支持地暖 0x004 `tuya_unsup 0x004` 
bit3 不支持温度 0x008 `tuya_unsup 0x008` 
bit4 不支持湿度 0x010 `tuya_unsup 0x010` 
bit5 不支持人感灵敏度 0x020 `tuya_unsup 0x020` 
bit6 不支持震动 0x040 `tuya_unsup 0x040` 
bit7 不支持换壁纸 0x080 `tuya_unsup 0x080` 
bit8 不支持 5-8 路继电器 0x100 `tuya_unsup 
0x100` bit9 不支持氛围灯 0x200 `tuya_unsup 0x200`*/

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
 * @brief 涂鸦配网：发送 NETWORK_JOIN 设置上报事件
 */
static void cli_cmd_tuya_join(int argc, char *argv[])
{
    HOOCH_PROTOCOL_SettingResult_t result;
    char buf[64];

    (void)argc;
    (void)argv;

    result = HOOCH_PROTOCOL_SettingReport_Send(
        HOOCH_PROTOCOL_SETTING_REPORT_EVENT_NETWORK_JOIN,
        NULL,
        0U);

    snprintf(buf, sizeof(buf), "[CLI] tuya_join result: %u\r\n", (unsigned int)result);
    cli_print(buf);
}

/**
 * @brief 86 盒子模组：上报重置状态(0x86)
 * @note  触发 HOOCH_PROTOCOL_SETTING_REPORT_EVENT_RESET,
 *        由 86box setting_report 回调发送 0x86 重置状态帧给全面屏
 */
static void cli_cmd_xm86_reset(int argc, char *argv[])
{
    HOOCH_PROTOCOL_SettingResult_t result;
    char buf[64];

    (void)argc;
    (void)argv;

    result = HOOCH_PROTOCOL_SettingReport_Send(
        HOOCH_PROTOCOL_SETTING_REPORT_EVENT_RESET,
        NULL,
        0U);

    snprintf(buf, sizeof(buf), "[CLI] xm86_reset result: %u\r\n", (unsigned int)result);
    cli_print(buf);
}

/**
 * @brief 86 盒子模组：重置模块(0x83)
 * @note  直接向模组下发 0x83(数据 1), 模组会清掉配网信息并上报 0x86
 */
static void cli_cmd_xm86_reset_module(int argc, char *argv[])
{
    uint8_t result;
    char buf[64];

    (void)argc;
    (void)argv;

    result = xiaomi_86box_send_reset_module();

    snprintf(buf, sizeof(buf), "[CLI] xm86_reset_module result: %u\r\n", (unsigned int)result);
    cli_print(buf);
}

/**
 * @brief 情景上报：发送指定按键(1~8)的场景触发
 */
static void cli_cmd_tuya_scene(int argc, char *argv[])
{
    uint8_t key_value;
    HOOCH_PROTOCOL_SceneReportResult_t result;
    char buf[64];

    if (argc != 2) {
        cli_print("[CLI] Usage: tuya_scene key (1~8)\r\n");
        cli_print("[CLI] Example: tuya_scene 1\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[1], &key_value) || key_value < 1U || key_value > 8U) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_SceneReport_Send((HOOCH_PROTOCOL_SceneReportKey_t)key_value);

    snprintf(buf, sizeof(buf), "[CLI] tuya_scene result: %u\r\n", (unsigned int)result);
    cli_print(buf);
}

/**
 * @brief 按键开关上报：上报指定按键(1~8)的开关状态(0/1)
 */
static void cli_cmd_tuya_key(int argc, char *argv[])
{
    uint8_t key_value;
    uint8_t state_value;
    HOOCH_PROTOCOL_KeyStatusResult_t result;
    char buf[64];

    if (argc != 3) {
        cli_print("[CLI] Usage: tuya_key key(1~8) state(0/1)\r\n");
        cli_print("[CLI] Example: tuya_key 1 1\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[1], &key_value) || key_value < 1U || key_value > 8U) {
        cli_print("[CLI] Invalid key, use 1~8\r\n");
        return;
    }

    if (!cli_parse_u8_arg(argv[2], &state_value) || state_value > 1U) {
        cli_print("[CLI] Invalid state, use 0/1\r\n");
        return;
    }

    result = HOOCH_PROTOCOL_KeyStatus_ReportState(
        (HOOCH_PROTOCOL_KeyStatusKey_t)key_value,
        (state_value == 1U) ? HOOCH_PROTOCOL_KEY_STATUS_STATE_ON
                            : HOOCH_PROTOCOL_KEY_STATUS_STATE_OFF);

    snprintf(buf, sizeof(buf), "[CLI] tuya_key result: %u\r\n", (unsigned int)result);
    cli_print(buf);
}

/**
 * @brief 不支持属性上报：按位掩码(bit0~bit9)上报自身不支持项
 * @note  掩码展开为属性列表(位号+1)后调用 SendAttributes，
 *        帧内其余字段由各通道上报回调自行填充
 */
static void cli_cmd_tuya_unsup(int argc, char *argv[])
{
    HOOCH_PROTOCOL_UnsupportedAttribute_t list[10];
    HOOCH_PROTOCOL_SettingResult_t result;
    char *endptr;
    unsigned long mask;
    uint8_t count = 0U;
    uint8_t i;
    char buf[80];

    if (argc != 2) {
        cli_print("[CLI] Usage: tuya_unsup mask(bit0~bit9, e.g. 0x41)\r\n");
        cli_print("[CLI] bit0 AC, bit1 fresh air, bit2 floor heating, bit3 temperature, bit4 humidity\r\n");
        cli_print("[CLI] bit5 presence sensitivity, bit6 vibration, bit7 wallpaper, bit8 relay 5-8, bit9 ambience light\r\n");
        return;
    }

    mask = strtoul(argv[1], &endptr, 0);
    if (argv[1][0] == '\0' || endptr == NULL || *endptr != '\0' || mask > 0x3FFUL) {
        cli_print("[CLI] Invalid mask, use 0x000~0x3FF\r\n");
        return;
    }

    /* 位号 -> 属性编号(位号+1)，其余槽位填 NONE 作为列表结束标志 */
    for (i = 0U; i < 10U; i++) {
        if ((mask & (1UL << i)) != 0UL) {
            list[count++] = (HOOCH_PROTOCOL_UnsupportedAttribute_t)(i + 1U);
        }
    }
    while (count < (uint8_t)(sizeof(list) / sizeof(list[0]))) {
        list[count++] = HOOCH_PROTOCOL_UNSUPPORTED_ATTRIBUTE_NONE;
    }

    result = HOOCH_PROTOCOL_UnsupportedAttribute_SendAttributes(
        list[0], list[1], list[2], list[3], list[4],
        list[5], list[6], list[7], list[8], list[9]);

    snprintf(buf, sizeof(buf), "[CLI] tuya_unsup mask=0x%03X result: %u\r\n",
             (unsigned int)mask, (unsigned int)result);
    cli_print(buf);
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
    cli_print("  Commands: tuya_join / tuya_scene key(1~8) / tuya_key key(1~8) state(0/1) / tuya_unsup mask(bit0~bit9)\r\n");
    cli_print("             xm86_reset / xm86_reset_module\r\n");
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
            cli_print("\r\n");
        }
    }

    /* 重置缓冲区，打印提示符 */
    cli_rx_index = 0;
    cli_cmd_ready = 0;
    memset(cli_rx_buf, 0, CLI_RX_BUF_SIZE);
    cli_print(CLI_PROMPT);
}
