/**
 * @file hooch_lcd_probe.c
 * @brief LCD 面板寄存器探测（STM32F103C8T6 移植版）
 *
 * 原实现基于 Beken BK7258 SDK 的 LCD(QSPI) GPIO 位操作读法
 * （bk_idk/middleware/driver/lcd/lcd_qspi_driver.c 里的 bk_lcd_qspi_read_data_gpio），
 * 这里移植为纯 STM32 HAL 的 GPIO bit-bang：
 *   - 用 HAL_GPIO_WritePin / HAL_GPIO_ReadPin 直接翻转 CLK/CSN/IO0 三根线；
 *   - 引脚选在 PA5/PA6/PA7，避开本工程已占用的串口引脚
 *     （USART1=PA9/PA10、USART2=PA2/PA3、USART3=PB10/PB11）；
 *   - 微秒延时用 Cortex-M3 的 DWT 周期计数器实现，不依赖主频写死的循环。
 *
 * 读时序与 SDK 一致：
 *   CS 拉低 → 读命令 + 0x00 + 寄存器地址 + dummy → IO0 切输入 → 读回数据 → CS 拉高
 */
#include "hooch_lcd_probe.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "user_printf.h"

/* reg 之后补发的 dummy 字节数（与 SDK 的读法保持一致） */
#define LCD_PROBE_DUMMY_BYTES       (1U)

/* 数据传输线格式：命令后的格式字节，0x00 表示单线(single line)数据模式 */
#define LCD_PROBE_DATA_FMT_1WIRE    (0x00U)

/* 写寄存器/写初始化命令所用的写命令 */
#define LCD_PROBE_WRITE_CMD         (0x02U)

/**
 * 探测表：新增一块屏幕只加一条。
 *
 * st77903_h0165y008t：写命令 0xDE / 读命令 0xDD，寄存器 0x0A 的期望值 0x94。
 * jd9855：写命令 0x02 / 读命令 0x0B。
 *         期望值尚未实测标定，expect=0 时该条目只打印原始读值、不参与判定。
 */
static const hooch_lcd_probe_item_t s_lcd_probe_items[] =
{
    {"st77903_h0165y008t", HOOCH_LCD_PROBE_PPI(400, 400), 0xDE, 0xDD, 0x0A, 0x94},
    {"jd9855",             HOOCH_LCD_PROBE_PPI(360, 360), 0x02, 0x0B, 0x0A, 0x00},
};

#define LCD_PROBE_ITEM_NUM  (sizeof(s_lcd_probe_items) / sizeof(s_lcd_probe_items[0]))

/* ------------------------------- 初始化命令表 ------------------------------- */

/* len 特殊值：本条不是命令，而是延时 data[0] 毫秒 */
#define LCD_PROBE_INIT_CMD_DELAY    (0xFFU)

typedef struct
{
    uint8_t cmd;       /**< 寄存器/命令 */
    uint8_t data[32];  /**< 数据字节 */
    uint8_t len;       /**< 数据字节数；LCD_PROBE_INIT_CMD_DELAY 表示 data[0] 为延时 ms */
} lcd_probe_init_cmd_t;

/**
 * 面板上电初始化表（单线写，写命令 0xDE）
 *
 * 每条 {寄存器, 数据, 数据长度}；长度为 LCD_PROBE_INIT_CMD_DELAY(0xff) 的条目
 * 表示延时 data[0] 毫秒。
 */
static const lcd_probe_init_cmd_t s_lcd_probe_init_cmds[] =
{
	{0xff, {0xa5}, 1},
	{0x40, {0x04}, 1},

	{0x55, {0x00, 0x00, 0xd3}, 3},

	{0x57, {0x08}, 1},
	{0x82, {0x05}, 1},
	{0x83, {0x69}, 1},
	{0x84, {0xba}, 1},
	{0x85, {0x28}, 1},
	{0x8a, {0x3f}, 1},
	{0x8b, {0x80}, 1},
	{0x8f, {0x53, 0x03}, 2},
	{0x9a, {0x08}, 1},
	{0x9b, {0x08}, 1},
	{0x9d, {0x04}, 1},
	{0x9e, {0xc2}, 1},

	{0x56, {0x05, 0x05, 0x40, 0x2c}, 4}, // porch

	{0xa8, {0x00, 0x01}, 2},
	{0xa9, {0x00, 0x05}, 2},

	{0xb4, {0x00, 0x04}, 2},
	{0xb5, {0x03, 0x5a}, 2},

	// change_add
	{0x87, {0xf3, 0x55}, 2},
	{0xd3, {0x14}, 1},
	{0xd4, {0x03, 0x00}, 2},
	{0xcf, {0xfd}, 1},
	{0xd0, {0xfd}, 1},
	// change_add end

	{0xb6, {0xe4}, 1},
	{0xb7, {0xe3}, 1},

	{0xb9, {0x14}, 1},
	{0xba, {0x03}, 1},

	{0xa2, {0x20, 0x2a}, 2},
	{0xa4, {0x2a, 0x20}, 2},
	{0xa6, {0x20, 0xa6}, 2},
	{0xa7, {0x99, 0x92}, 2},
	{0xce, {0x00, 0x01}, 2},

	{0xd5, {0x20, 0xca, 0x0e}, 3},
	{0xd6, {0xeb, 0x78}, 2},
	{0xd7, {0x31, 0xdb, 0x1f}, 3},
	{0xd8, {0xeb, 0x78}, 2},
	{0xd9, {0x0b, 0x33, 0x0b}, 3},

	// source
	{0xa0, {0xf2}, 1},
	{0xe1, {0x05}, 1},
	{0xe2, {0x07, 0x0b}, 2},
	{0xe3, {0x01, 0x50}, 2},
	{0xe4, {0xf8, 0x02, 0xfb, 0x29}, 4},
	{0xe5, {0x01, 0x10}, 2},
	{0xe6, {0x00, 0x00}, 2},
	{0xe7, {0x04, 0x28}, 2},
	{0xe8, {0x00, 0x00, 0xf4, 0x00}, 4},
	{0xe9, {0x60}, 1},
	{0xea, {0x2f, 0x10}, 2},
	{0xeb, {0x00, 0x00, 0x16}, 3},
	{0xec, {0x15, 0x40}, 2},
	{0xed, {0x1a, 0x14}, 2},
	{0xee, {0x00}, 1},
	{0xef, {0x17, 0xf2}, 2},
	{0xf0, {0xfc, 0x2a, 0xfc, 0xf4}, 4},
	{0xf1, {0xf3}, 1},
	{0xf2, {0x00, 0x00}, 2},
	{0xf6, {0x15, 0x64, 0x12, 0x15}, 4},

	{0x60, {0x00}, 1}, // vrp0 V63
	{0x70, {0x00}, 1}, // vrn0 V63
	{0x61, {0x08}, 1}, // vrp1 V62
	{0x71, {0x08}, 1}, // vrn1 V62
	{0x62, {0x0d}, 1}, // vrp2 V61
	{0x72, {0x0d}, 1}, // vrn2 V61
	{0x63, {0x0a}, 1}, // vrp4 V59
	{0x73, {0x0a}, 1}, // vrn4 V59
	{0x64, {0x09}, 1}, // vrp6 V57
	{0x74, {0x0b}, 1}, // vrn6 V57
	{0x65, {0x06}, 1}, // vrp13 V50
	{0x75, {0x07}, 1}, // vrn13 V50
	{0x66, {0x33}, 1}, // vrp20 V43
	{0x76, {0x38}, 1}, // vrn20 V43
	{0x67, {0x33}, 1}, // vrp27 V36
	{0x77, {0x33}, 1}, // vrn27 V36
	{0x68, {0x48}, 1}, // vrp43 V20
	{0x78, {0x48}, 1}, // vrn43 V20
	{0x69, {0x0a}, 1}, // vrp50 V13
	{0x79, {0x0a}, 1}, // vrn50 V13
	{0x6a, {0x14}, 1}, // vrp57 V6
	{0x7a, {0x17}, 1}, // vrn57 V6
	{0x6b, {0x13}, 1}, // vrp59 V4
	{0x7b, {0x15}, 1}, // vrn59 V4
	{0x6c, {0x2d}, 1}, // vrp61 V2
	{0x7c, {0x30}, 1}, // vrn61 V2
	{0x6d, {0x34}, 1}, // vrp62 V1
	{0x7d, {0x34}, 1}, // vrn62 V1
	{0x6e, {0x0f}, 1}, // vrp63 V0
	{0x7e, {0x0f}, 1}, // vrn63 V0

	{0xf2, {0x2e}, 1},
	{0xf9, {0x52, 0x9d}, 2},
	{0x80, {0x01, 0xa7}, 2},
	{0x81, {0x19}, 1},
	{0x46, {0x10}, 1},
	{0xff, {0x00}, 1},

	{0x3a, {0x55}, 1}, // 55=RGB565
	{0x35, {0x00}, 1}, // 00=FW; 03=BW
	{0x11, {0x00}, 0},
	{0x00, {0xc8}, 0xff}, // Delayms(200);
	{0x29, {0x00}, 0},
	{0x00, {0x14}, 0xff}, // Delayms(20);
	{0xf4, {0x01}, 1},
	{0xf5, {0x00,0x00,0x00}, 3},
};

#define LCD_PROBE_INIT_CMD_NUM  (sizeof(s_lcd_probe_init_cmds) / sizeof(s_lcd_probe_init_cmds[0]))

/* ----------------------------- 微秒延时（DWT） ----------------------------- */

static uint8_t s_lcd_probe_dwt_ready = 0U;

/**
 * @brief 使能 DWT 周期计数器，用于精确微秒延时（只需初始化一次）
 */
static void lcd_probe_dwt_init(void)
{
    if (s_lcd_probe_dwt_ready != 0U) {
        return;
    }

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    s_lcd_probe_dwt_ready = 1U;
}

/**
 * @brief 忙等微秒延时（基于 DWT 周期计数，72MHz 下 1us = 72 周期）
 */
static void lcd_probe_delay_us(uint32_t us)
{
    uint32_t start  = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000U);

    while ((uint32_t)(DWT->CYCCNT - start) < cycles) {
        /* busy wait */
    }
}

/* ------------------------------- GPIO 位操作 ------------------------------- */

/**
 * @brief 配置单根引脚方向（输出推挽 / 输入浮空）
 */
static void lcd_probe_pin_mode(GPIO_TypeDef *port, uint16_t pin, uint32_t mode)
{
    GPIO_InitTypeDef cfg = {0};

    cfg.Pin   = pin;
    cfg.Mode  = mode;
    cfg.Pull  = GPIO_NOPULL;
    cfg.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(port, &cfg);
}

static void lcd_probe_bb_clk(bool high)
{
    HAL_GPIO_WritePin(HOOCH_LCD_PROBE_CLK_PORT, HOOCH_LCD_PROBE_CLK_PIN,
                      high ? GPIO_PIN_SET : GPIO_PIN_RESET);
    lcd_probe_delay_us(HOOCH_LCD_PROBE_BB_DELAY_US);
    lcd_probe_delay_us(HOOCH_LCD_PROBE_BB_DELAY_US);
}

static void lcd_probe_bb_dat(bool high)
{
    HAL_GPIO_WritePin(HOOCH_LCD_PROBE_IO0_PORT, HOOCH_LCD_PROBE_IO0_PIN,
                      high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void lcd_probe_bb_write_byte(uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        lcd_probe_bb_clk(false);
        lcd_probe_bb_dat((byte & 0x80) != 0);
        lcd_probe_bb_clk(true);
        byte <<= 1;
    }
}

/* 从指定数据线读 1 字节（MSB 先） */
static uint8_t lcd_probe_bb_read_byte(GPIO_TypeDef *port, uint16_t pin)
{
    uint8_t byte = 0;

    for (int i = 0; i < 8; i++) {
        byte <<= 1;
        lcd_probe_bb_clk(false);
        lcd_probe_bb_clk(true);
        if (HAL_GPIO_ReadPin(port, pin) != GPIO_PIN_RESET) {
            byte |= 1;
        }
    }

    return byte;
}

/* 按接收线选择返回对应的端口/引脚 */
static void lcd_probe_rx_line_pins(uint8_t rx_line, GPIO_TypeDef **port, uint16_t *pin)
{
    if (rx_line == HOOCH_LCD_PROBE_RX_LINE_IO1) {
        *port = HOOCH_LCD_PROBE_IO1_PORT;
        *pin  = HOOCH_LCD_PROBE_IO1_PIN;
    } else {
        *port = HOOCH_LCD_PROBE_IO0_PORT;
        *pin  = HOOCH_LCD_PROBE_IO0_PIN;
    }
}

/* -------------------------------- 引脚控制 -------------------------------- */

/**
 * @brief 配置探测引脚为输出并置为空闲电平
 *
 * 全程由本驱动控制这几根引脚（不做“交出/归还”）：CLK/CSN/IO0/IO1/RST 均配置为
 * 推挽输出，RST 低电平复位、空闲保持高。
 */
static void lcd_probe_pins_config(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    lcd_probe_dwt_init();

    lcd_probe_pin_mode(HOOCH_LCD_PROBE_CLK_PORT, HOOCH_LCD_PROBE_CLK_PIN, GPIO_MODE_OUTPUT_PP);
    lcd_probe_pin_mode(HOOCH_LCD_PROBE_CSN_PORT, HOOCH_LCD_PROBE_CSN_PIN, GPIO_MODE_OUTPUT_PP);
    lcd_probe_pin_mode(HOOCH_LCD_PROBE_IO0_PORT, HOOCH_LCD_PROBE_IO0_PIN, GPIO_MODE_OUTPUT_PP);
    lcd_probe_pin_mode(HOOCH_LCD_PROBE_IO1_PORT, HOOCH_LCD_PROBE_IO1_PIN, GPIO_MODE_OUTPUT_PP);
    lcd_probe_pin_mode(HOOCH_LCD_PROBE_RST_PORT, HOOCH_LCD_PROBE_RST_PIN, GPIO_MODE_OUTPUT_PP);

    HAL_GPIO_WritePin(HOOCH_LCD_PROBE_CSN_PORT, HOOCH_LCD_PROBE_CSN_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(HOOCH_LCD_PROBE_CLK_PORT, HOOCH_LCD_PROBE_CLK_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(HOOCH_LCD_PROBE_IO0_PORT, HOOCH_LCD_PROBE_IO0_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(HOOCH_LCD_PROBE_IO1_PORT, HOOCH_LCD_PROBE_IO1_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(HOOCH_LCD_PROBE_RST_PORT, HOOCH_LCD_PROBE_RST_PIN, GPIO_PIN_SET);
    lcd_probe_delay_us(HOOCH_LCD_PROBE_BB_DELAY_US);
}

/**
 * @brief 复位脉冲：拉低 RST 保持一段时间后释放(拉高)，并等待面板内部启动
 *
 * 读寄存器前调用，确保面板从已知的复位后状态开始响应。
 */
static void lcd_probe_reset_pulse(void)
{
    HAL_GPIO_WritePin(HOOCH_LCD_PROBE_RST_PORT, HOOCH_LCD_PROBE_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(HOOCH_LCD_PROBE_RST_ASSERT_MS);
    HAL_GPIO_WritePin(HOOCH_LCD_PROBE_RST_PORT, HOOCH_LCD_PROBE_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(HOOCH_LCD_PROBE_RST_RELEASE_MS);
}

/* -------------------------------- 寄存器读取 -------------------------------- */

/**
 * @brief 读一个寄存器（调用前必须已 config 引脚）
 *
 * 时序：CS 拉低 → 读命令 + 0x00 + 寄存器地址 + 0x00 → 接收线切输入 → 读回数据 → CS 拉高
 * 命令/地址始终在 IO0 上发出；接收线由 rx_line 选择（IO0 或 IO1）。
 * 读回 HOOCH_LCD_PROBE_READ_LEN 个字节，按先收到的为高字节拼成 16 位值。
 */
static hooch_lcd_probe_err_t lcd_probe_read_reg_raw(uint8_t read_cmd, uint8_t reg,
                                                    uint8_t rx_line, uint16_t *value)
{
    uint16_t data = 0;
    GPIO_TypeDef *rx_port;
    uint16_t rx_pin;

    if (value == NULL) {
        return HOOCH_LCD_PROBE_ERR_NULL_PARAM;
    }

    lcd_probe_rx_line_pins(rx_line, &rx_port, &rx_pin);

    HAL_GPIO_WritePin(HOOCH_LCD_PROBE_CSN_PORT, HOOCH_LCD_PROBE_CSN_PIN, GPIO_PIN_SET);
    lcd_probe_bb_clk(false);
    lcd_probe_bb_dat(true);
    HAL_GPIO_WritePin(HOOCH_LCD_PROBE_CSN_PORT, HOOCH_LCD_PROBE_CSN_PIN, GPIO_PIN_RESET);

    lcd_probe_bb_write_byte(read_cmd);
    lcd_probe_bb_write_byte(0x00);
    lcd_probe_bb_write_byte(reg);
     lcd_probe_bb_write_byte(0x00);


    /* 接收线切输入读回数据 */
    HAL_GPIO_WritePin(rx_port, rx_pin, GPIO_PIN_RESET);
    lcd_probe_pin_mode(rx_port, rx_pin, GPIO_MODE_INPUT);

    for (uint32_t i = 0; i < HOOCH_LCD_PROBE_READ_LEN; i++) {
        data = (uint16_t)((data << 8) | lcd_probe_bb_read_byte(rx_port, rx_pin));
    }

    /* 接收线恢复输出 */
    lcd_probe_pin_mode(rx_port, rx_pin, GPIO_MODE_OUTPUT_PP);
    HAL_GPIO_WritePin(rx_port, rx_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(HOOCH_LCD_PROBE_CSN_PORT, HOOCH_LCD_PROBE_CSN_PIN, GPIO_PIN_SET);

    *value = data;

    return HOOCH_LCD_PROBE_OK;
}

hooch_lcd_probe_err_t hooch_lcd_probe_read_reg(const hooch_lcd_probe_item_t *item, uint16_t *value)
{
    hooch_lcd_probe_err_t ret;

    if (item == NULL) {
        return HOOCH_LCD_PROBE_ERR_NULL_PARAM;
    }

    lcd_probe_pins_config();
    ret = lcd_probe_read_reg_raw(item->read_cmd, item->reg, HOOCH_LCD_PROBE_RX_LINE_IO0, value);

    return ret;
}

/**
 * @brief 写一条命令及其数据（调用前必须已 config 引脚）
 *
 * 时序：CS 拉低 → opcode + 0x00 + 命令 + 0x00 + data[len] → CS 拉高。
 *      （与读时序 opcode + 0x00 + reg + 0x00 对称，全程单线数据模式）
 */
static void lcd_probe_write_cmd(uint8_t opcode, uint8_t cmd, const uint8_t *data, uint8_t len)
{
    HAL_GPIO_WritePin(HOOCH_LCD_PROBE_CSN_PORT, HOOCH_LCD_PROBE_CSN_PIN, GPIO_PIN_SET);
    lcd_probe_bb_clk(false);
    lcd_probe_bb_dat(true);
    HAL_GPIO_WritePin(HOOCH_LCD_PROBE_CSN_PORT, HOOCH_LCD_PROBE_CSN_PIN, GPIO_PIN_RESET);

    lcd_probe_bb_write_byte(opcode);
    lcd_probe_bb_write_byte(LCD_PROBE_DATA_FMT_1WIRE);
    lcd_probe_bb_write_byte(cmd);
    lcd_probe_bb_write_byte(LCD_PROBE_DATA_FMT_1WIRE);
    for (uint8_t i = 0; i < len; i++) {
        lcd_probe_bb_write_byte(data[i]);
    }

    HAL_GPIO_WritePin(HOOCH_LCD_PROBE_CSN_PORT, HOOCH_LCD_PROBE_CSN_PIN, GPIO_PIN_SET);
    lcd_probe_delay_us(HOOCH_LCD_PROBE_BB_DELAY_US);
}

/**
 * @brief 写一个寄存器（1 字节数据；调用前必须已 config 引脚）
 */
static void lcd_probe_write_reg_raw(uint8_t write_cmd, uint8_t reg, uint8_t value)
{
    lcd_probe_write_cmd(write_cmd, reg, &value, 1U);
}

/* --------------------------------- 初始化 --------------------------------- */

void hooch_lcd_probe_init(void)
{
    LOG_INFO("[LCDPROBE] init: clk=PA5 csn=PA6 io0=PA7 io1=PA8 rst=PA4\r\n");

    /* 面板上电后需要一点稳定时间 */
    HAL_Delay(HOOCH_LCD_PROBE_POWER_ON_DELAY_MS);

    /* 全程掌控引脚：配置为输出并置为空闲电平 */
    lcd_probe_pins_config();

    /* 复位面板，确保从已知状态开始 */
    lcd_probe_reset_pulse();

    /* 按初始化表下发（单线写，写命令 0xDE） */
    for (uint32_t i = 0; i < LCD_PROBE_INIT_CMD_NUM; i++) {
        const lcd_probe_init_cmd_t *c = &s_lcd_probe_init_cmds[i];

        if (c->len == LCD_PROBE_INIT_CMD_DELAY) {
            HAL_Delay(c->data[0]);
        } else {
            lcd_probe_write_cmd(LCD_PROBE_WRITE_CMD, c->cmd, c->data, c->len);
        }
    }

    LOG_INFO("[LCDPROBE] init done (%u cmds)\r\n", (unsigned int)LCD_PROBE_INIT_CMD_NUM);
}

/* --------------------------------- 探测 --------------------------------- */

const hooch_lcd_probe_item_t *hooch_lcd_probe_items(uint32_t *count)
{
    if (count != NULL) {
        *count = (uint32_t)LCD_PROBE_ITEM_NUM;
    }
    return s_lcd_probe_items;
}

const hooch_lcd_probe_item_t *hooch_lcd_probe_detect(void)
{
    const hooch_lcd_probe_item_t *hit = NULL;

    LOG_INFO("[LCDPROBE] start: clk=PA5 csn=PA6 io0=PA7 io1=PA8 rst=PA4\r\n");

    /* 面板上电后需要一点稳定时间 */
    HAL_Delay(HOOCH_LCD_PROBE_POWER_ON_DELAY_MS);

    /* 全程掌控引脚：配置为输出并置为空闲电平 */
    lcd_probe_pins_config();

    for (uint32_t i = 0; i < LCD_PROBE_ITEM_NUM; i++) {
        const hooch_lcd_probe_item_t *item = &s_lcd_probe_items[i];
        uint16_t value = 0;

        if (lcd_probe_read_reg_raw(item->read_cmd, item->reg, HOOCH_LCD_PROBE_RX_LINE_IO0, &value) != HOOCH_LCD_PROBE_OK) {
            LOG_INFO("[LCDPROBE] %s: read reg 0x%02X failed\r\n", item->name, item->reg);
            continue;
        }

        LOG_INFO("[LCDPROBE] %s: reg 0x%02X = 0x%04X (expect 0x%02X, read_cmd 0x%02X)\r\n",
                 item->name, item->reg, value, item->expect, item->read_cmd);

        if ((item->expect != 0) && (value == item->expect) && (hit == NULL)) {
            hit = item;
        }
    }

    if (hit != NULL) {
        LOG_INFO("[LCDPROBE] hit: %s\r\n", hit->name);
    } else {
        LOG_INFO("[LCDPROBE] no panel matched\r\n");
    }

    return hit;
}

/* ---------------------------------- CLI ---------------------------------- */

static uint8_t lcd_probe_cli_parse_u8(const char *str)
{
    if ((strncmp(str, "0x", 2) == 0) || (strncmp(str, "0X", 2) == 0)) {
        return (uint8_t)strtol(str, NULL, 16);
    }
    return (uint8_t)atoi(str);
}

static void lcd_probe_cli_dump_items(void)
{
    LOG_INFO("index name                 ppi        write read reg  expect\r\n");
    for (uint32_t i = 0; i < LCD_PROBE_ITEM_NUM; i++) {
        const hooch_lcd_probe_item_t *item = &s_lcd_probe_items[i];
        LOG_INFO("  %u   %-20s %u x %u    0x%02X  0x%02X 0x%02X 0x%02X\r\n",
                 (unsigned int)i,
                 item->name,
                 (unsigned int)(item->ppi >> 16),
                 (unsigned int)(item->ppi & 0xFFFF),
                 item->write_cmd, item->read_cmd, item->reg, item->expect);
    }
}

void hooch_lcd_probe_cli_cmd(int argc, char *argv[])
{
    if ((argc < 2) || (strcmp(argv[1], "detect") == 0)) {
        (void)hooch_lcd_probe_detect();
        return;
    }

    if (strcmp(argv[1], "items") == 0) {
        lcd_probe_cli_dump_items();
        return;
    }

    if (strcmp(argv[1], "init") == 0) {
        hooch_lcd_probe_init();
        return;
    }

    if (strcmp(argv[1], "reset") == 0) {
        lcd_probe_pins_config();
        lcd_probe_reset_pulse();

        LOG_INFO("[LCDPROBE] reset pulse done (rst=PA4)\r\n");
        return;
    }

    if (strcmp(argv[1], "read") == 0) {
        uint8_t read_cmd;
        uint8_t reg;
        uint8_t rx_line = HOOCH_LCD_PROBE_RX_LINE_IO0;
        uint16_t value = 0;

        if (argc < 4) {
            LOG_INFO("Usage: lcdprobe read <read_cmd> <reg> [line]   (e.g. read 0xDD 0x0A 0; line: 0=IO0 1=IO1)\r\n");
            return;
        }

        read_cmd = lcd_probe_cli_parse_u8(argv[2]);
        reg = lcd_probe_cli_parse_u8(argv[3]);

        if (argc >= 5) {
            rx_line = lcd_probe_cli_parse_u8(argv[4]);
        }

        lcd_probe_pins_config();
        lcd_probe_read_reg_raw(read_cmd, reg, rx_line, &value);

        LOG_INFO("[LCDPROBE] read: read_cmd 0x%02X reg 0x%02X line=%s = 0x%04X\r\n",
                 read_cmd, reg, (rx_line == HOOCH_LCD_PROBE_RX_LINE_IO1) ? "IO1" : "IO0", value);
        return;
    }

    if (strcmp(argv[1], "write") == 0) {
        uint8_t write_cmd;
        uint8_t reg;
        uint8_t value;

        if (argc < 5) {
            LOG_INFO("Usage: lcdprobe write <write_cmd> <reg> <value>   (e.g. write 0xDE 0x0A 0x01)\r\n");
            return;
        }

        write_cmd = lcd_probe_cli_parse_u8(argv[2]);
        reg = lcd_probe_cli_parse_u8(argv[3]);
        value = lcd_probe_cli_parse_u8(argv[4]);

        lcd_probe_pins_config();
        lcd_probe_write_reg_raw(write_cmd, reg, value);

        LOG_INFO("[LCDPROBE] write: write_cmd 0x%02X reg 0x%02X = 0x%02X (single line, 1 byte)\r\n",
                 write_cmd, reg, value);
        return;
    }

    LOG_INFO("Usage:\r\n");
    LOG_INFO("  lcdprobe                             - detect panel and print result\r\n");
    LOG_INFO("  lcdprobe init                        - reset + send panel init table (single line, cmd 0xDE)\r\n");
    LOG_INFO("  lcdprobe items                       - dump the probe table\r\n");
    LOG_INFO("  lcdprobe reset                       - pulse the panel reset pin (rst=PA4)\r\n");
    LOG_INFO("  lcdprobe read <read_cmd> <reg> [line] - read a panel register (line: 0=IO0 1=IO1)\r\n");
    LOG_INFO("  lcdprobe write <write_cmd> <reg> <value> - write a panel register (single line)\r\n");
}
