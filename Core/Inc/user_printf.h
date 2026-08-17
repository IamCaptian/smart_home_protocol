/**
  ******************************************************************************
  * @file    user_printf.h
  * @brief   基于UART1的日志输出驱动头文件
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USER_PRINTF_H
#define __USER_PRINTF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Exported defines ----------------------------------------------------------*/
/* 日志级别定义 */
#define LOG_LEVEL_ERROR     0   ///< 错误日志
#define LOG_LEVEL_WARN      1   ///< 警告日志
#define LOG_LEVEL_INFO      2   ///< 信息日志
#define LOG_LEVEL_DEBUG     3   ///< 调试日志

/* 当前日志级别(可通过修改此值控制日志输出) */
#define LOG_CURRENT_LEVEL   LOG_LEVEL_DEBUG

/* 日志输出宏定义 */
#define LOG_ERROR(fmt, ...)  Log_Printf(LOG_LEVEL_ERROR, "[ERROR] " fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)   Log_Printf(LOG_LEVEL_WARN, "[WARN]  " fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)   Log_Printf(LOG_LEVEL_INFO, "[INFO]  " fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)  Log_Printf(LOG_LEVEL_DEBUG, "[DEBUG] " fmt, ##__VA_ARGS__)

/* 日志颜色控制(可选) */
#define LOG_COLOR_ENABLE    0   ///< 0:关闭颜色, 1:开启颜色

/* Exported function prototypes ----------------------------------------------*/
/**
  * @brief 初始化日志模块
  */
void Log_Init(void);

/**
  * @brief 打印日志
  * @param level: 日志级别
  * @param fmt: 格式化字符串
  * @param ...: 可变参数
  */
void Log_Printf(uint8_t level, const char *fmt, ...);

/**
  * @brief 直接通过UART1发送数据
  * @param data: 数据指针
  * @param len: 数据长度
  */
void UART1_SendData(uint8_t *data, uint16_t len);

/**
  * @brief 通过UART1发送字符串
  * @param str: 字符串指针
  */
void UART1_SendString(const char *str);

/**
  * @brief 通过UART1发送单个字符
  * @param ch: 字符
  */
void UART1_SendChar(uint8_t ch);

#ifdef __cplusplus
}
#endif

#endif /* __USER_PRINTF_H */
