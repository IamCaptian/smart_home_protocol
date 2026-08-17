#ifndef TUYA_PROTOCOL_UART_H
#define TUYA_PROTOCOL_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "usart.h"
#include <stdint.h>

/*
 * Tuya Zigbee UART 协议 — 硬件抽象与日志控制
 *
 * 参考 knx_protocol_uart.h 的设计模式：
 * 1) TUYA_TX_SEND — UART 发送宏，可更换物理串口
 * 2) TUYA_LOG_ENABLE — 日志总开关，关闭时字符串不参与编译
 * 3) 子开关（RX_BYTE / TX_HEX）— 精细控制单字节/整帧 hex 日志
 *
 * 日志输出口（调试口）：USART1（通过 user_printf.h → LOG_INFO）
 * 协议数据口（发送口）：默认 USART2（通过 TUYA_TX_SEND 宏）
 */

/*----------------------------------------------------------
 *              日志总开关
 *--------------------------------------------------------*/
#ifndef TUYA_LOG_ENABLE
#define TUYA_LOG_ENABLE                     1
#endif

#if TUYA_LOG_ENABLE
#ifndef TUYA_LOG_PRINTF
#include "user_printf.h"
#define TUYA_LOG_PRINTF(...)                LOG_INFO(__VA_ARGS__)
#endif
#else
#ifndef TUYA_LOG_PRINTF
#define TUYA_LOG_PRINTF(...)                ((void)0)
#endif
#endif

/*----------------------------------------------------------
 *              日志子开关
 *--------------------------------------------------------*/
#ifndef TUYA_LOG_RX_BYTE_ENABLE
#define TUYA_LOG_RX_BYTE_ENABLE             0
#endif

#ifndef TUYA_LOG_TX_HEX_ENABLE
#define TUYA_LOG_TX_HEX_ENABLE              0
#endif

#if !TUYA_LOG_ENABLE
#undef  TUYA_LOG_RX_BYTE_ENABLE
#define TUYA_LOG_RX_BYTE_ENABLE             0
#undef  TUYA_LOG_TX_HEX_ENABLE
#define TUYA_LOG_TX_HEX_ENABLE              0
#endif

/*----------------------------------------------------------
 *              UART 发送硬件抽象
 *   默认 USART2，可在此修改为其他串口（如 huart3）
 *--------------------------------------------------------*/
#ifndef TUYA_TX_SEND
#define TUYA_TX_SEND(buffer, length)        HAL_UART_Transmit(&huart2, (buffer), (length), 1000U)
#endif

/*----------------------------------------------------------
 *              日志宏定义
 *--------------------------------------------------------*/
#if TUYA_LOG_ENABLE
#define TUYA_LOG_INFO(...)                  TUYA_LOG_PRINTF(__VA_ARGS__)
#define TUYA_LOG_WARN(...)                  TUYA_LOG_PRINTF(__VA_ARGS__)
#define TUYA_LOG_DEBUG(...)                 TUYA_LOG_PRINTF(__VA_ARGS__)
#define TUYA_LOG_ERROR(...)                 TUYA_LOG_PRINTF(__VA_ARGS__)
#else
#define TUYA_LOG_INFO(...)                  ((void)0)
#define TUYA_LOG_WARN(...)                  ((void)0)
#define TUYA_LOG_DEBUG(...)                 ((void)0)
#define TUYA_LOG_ERROR(...)                 ((void)0)
#endif

/*----------------------------------------------------------
 *              RX 逐字节日志
 *--------------------------------------------------------*/
#if TUYA_LOG_RX_BYTE_ENABLE
#define TUYA_LOG_RX_BYTE(byte)              TUYA_LOG_INFO("R %02X\r\n", (unsigned int)(byte))
#else
#define TUYA_LOG_RX_BYTE(byte)              ((void)(byte))
#endif

/*----------------------------------------------------------
 *              TX 整帧 HEX 日志
 *--------------------------------------------------------*/
#if TUYA_LOG_TX_HEX_ENABLE
#define TUYA_LOG_TX_HEX_BEGIN(len)          TUYA_LOG_INFO("T %u: ", (unsigned int)(len))
#define TUYA_LOG_TX_HEX_BYTE(byte)          TUYA_LOG_INFO("%02X ", (unsigned int)(byte))
#define TUYA_LOG_TX_HEX_END()               TUYA_LOG_INFO("\r\n")
#else
#define TUYA_LOG_TX_HEX_BEGIN(len)          ((void)(len))
#define TUYA_LOG_TX_HEX_BYTE(byte)          ((void)(byte))
#define TUYA_LOG_TX_HEX_END()               ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* TUYA_PROTOCOL_UART_H */
