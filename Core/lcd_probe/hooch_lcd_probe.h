#ifndef _HOOCH_LCD_PROBE_H_
#define _HOOCH_LCD_PROBE_H_

#include "main.h"
#include <stdint.h>

/**
 * @brief LCD 面板寄存器探测（STM32F103C8T6 移植版）
 *
 * 由 Beken BK7258 版本移植而来：原版用 SDK 的 gpio 位操作读法识别 QSPI 屏幕，
 * 这里改成纯 STM32 HAL 的 GPIO bit-bang，只依赖 HAL 的 GPIO + DWT 计数延时，
 * 不依赖任何 LCD/QSPI 控制器驱动。
 *
 * 时序：用 3 根 GPIO（CLK/CSN/IO0）位操作读面板寄存器，据此识别屏幕型号。
 *       引脚集中在下方宏里，选用的引脚需避开本工程已占用的串口引脚。
 *
 * 特点：
 *   - 只用 HAL_GPIO_* + DWT 延时，不依赖 STM32 的 SPI/FSMC 外设；
 *   - 引脚、读命令、期望值都集中在 hooch_lcd_probe.c 顶部的表与宏里。
 */

/* ---------------- 板级配置：bit-bang 引脚（避开已占用的串口引脚） ----------------
 * 本工程已占用（见 Core/Src/usart.c）：
 *   USART1 = PA9 (TX) / PA10 (RX)
 *   USART2 = PA2 (TX) / PA3  (RX)
 *   USART3 = PB10(TX) / PB11 (RX)
 * 下面选用 PA4/PA5/PA6/PA7/PA8，与上述串口互不冲突；如需改脚，改这几个宏即可。
 */
#ifndef HOOCH_LCD_PROBE_CLK_PORT
#define HOOCH_LCD_PROBE_CLK_PORT         GPIOA
#endif
#ifndef HOOCH_LCD_PROBE_CLK_PIN
#define HOOCH_LCD_PROBE_CLK_PIN          GPIO_PIN_5
#endif

#ifndef HOOCH_LCD_PROBE_CSN_PORT
#define HOOCH_LCD_PROBE_CSN_PORT         GPIOA
#endif
#ifndef HOOCH_LCD_PROBE_CSN_PIN
#define HOOCH_LCD_PROBE_CSN_PIN          GPIO_PIN_6
#endif

#ifndef HOOCH_LCD_PROBE_IO0_PORT
#define HOOCH_LCD_PROBE_IO0_PORT         GPIOA
#endif
#ifndef HOOCH_LCD_PROBE_IO0_PIN
#define HOOCH_LCD_PROBE_IO0_PIN          GPIO_PIN_7
#endif

/* 第二根数据线 IO1：部分面板用 IO1 回数据（MCU 在 IO1 上接收） */
#ifndef HOOCH_LCD_PROBE_IO1_PORT
#define HOOCH_LCD_PROBE_IO1_PORT         GPIOA
#endif
#ifndef HOOCH_LCD_PROBE_IO1_PIN
#define HOOCH_LCD_PROBE_IO1_PIN          GPIO_PIN_8
#endif

/* 接收数据线选择 */
#define HOOCH_LCD_PROBE_RX_LINE_IO0      (0U)   /**< 用 IO0 接收 */
#define HOOCH_LCD_PROBE_RX_LINE_IO1      (1U)   /**< 用 IO1 接收 */

/* 面板复位引脚（低电平复位，高电平释放运行） */
#ifndef HOOCH_LCD_PROBE_RST_PORT
#define HOOCH_LCD_PROBE_RST_PORT         GPIOA
#endif
#ifndef HOOCH_LCD_PROBE_RST_PIN
#define HOOCH_LCD_PROBE_RST_PIN          GPIO_PIN_4
#endif

/* 复位脉冲：拉低保持时间 / 释放后等待面板启动时间（ms） */
#ifndef HOOCH_LCD_PROBE_RST_ASSERT_MS
#define HOOCH_LCD_PROBE_RST_ASSERT_MS    (10U)
#endif
#ifndef HOOCH_LCD_PROBE_RST_RELEASE_MS
#define HOOCH_LCD_PROBE_RST_RELEASE_MS   (120U)
#endif

/* bit-bang 半周期延时（us） */
#ifndef HOOCH_LCD_PROBE_BB_DELAY_US
#define HOOCH_LCD_PROBE_BB_DELAY_US      (2U)
#endif

/* 面板上电后稳定等待（ms）：探测前留出面板内部复位时间 */
#ifndef HOOCH_LCD_PROBE_POWER_ON_DELAY_MS
#define HOOCH_LCD_PROBE_POWER_ON_DELAY_MS (150U)
#endif

/* 每个条目读取的数据长度 */
#define HOOCH_LCD_PROBE_READ_LEN         (2U)

/* 分辨率打包：高 16 位宽、低 16 位高（兼容原工程 ppi 的打印方式） */
#define HOOCH_LCD_PROBE_PPI(w, h)        ((((uint32_t)(w)) << 16) | ((uint32_t)(h) & 0xFFFFU))

/**
 * @brief 探测接口返回码（替代原 SDK 的 bk_err_t）
 */
typedef enum
{
    HOOCH_LCD_PROBE_OK            = 0,   /**< 成功 */
    HOOCH_LCD_PROBE_ERR_NULL_PARAM = -1, /**< 空指针参数 */
} hooch_lcd_probe_err_t;

/**
 * @brief 一个候选面板的探测描述
 */
typedef struct
{
    const char *name;      /**< 面板名 */
    uint32_t    ppi;       /**< 面板分辨率，高 16 位宽、低 16 位高 */
    uint8_t     write_cmd; /**< 面板写命令 */
    uint8_t     read_cmd;  /**< 面板读命令 */
    uint8_t     reg;       /**< 探测用寄存器地址 */
    uint8_t     expect;    /**< 期望读值；0 表示未标定（不参与判定，仅打印原始值） */
} hooch_lcd_probe_item_t;

/**
 * @brief 上电初始化面板：复位 + 按内置初始化表(单线写、写命令 0xDE)下发
 * @note  调用后即可用 hooch_lcd_probe_detect() 读寄存器识别面板
 */
void hooch_lcd_probe_init(void);

/**
 * @brief 探测面板；未识别返回 NULL
 */
const hooch_lcd_probe_item_t *hooch_lcd_probe_detect(void);

/**
 * @brief 获取探测条目表；count 返回条目个数
 */
const hooch_lcd_probe_item_t *hooch_lcd_probe_items(uint32_t *count);

/**
 * @brief 按条目读一个寄存器（内部自动配置并全程掌控引脚），返回 16 位值（先收高字节）
 */
hooch_lcd_probe_err_t hooch_lcd_probe_read_reg(const hooch_lcd_probe_item_t *item, uint16_t *value);

/**
 * @brief CLI 命令处理入口
 * @note  与本工程 Core/Src/hooch_cli_driver.c 的 cli_cmd_func_t 签名一致
 *        （void (*)(int argc, char *argv[])），加入其命令表即可使用：
 *          {"lcdprobe", "LCD panel probe", hooch_lcd_probe_cli_cmd},
 */
void hooch_lcd_probe_cli_cmd(int argc, char *argv[]);

#endif
