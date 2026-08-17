/**
  ******************************************************************************
  * @file    user_printf.c
  * @brief   基于UART1的日志输出驱动实现
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "user_printf.h"
#include "usart.h"

/* Private defines -----------------------------------------------------------*/
#define LOG_BUFFER_SIZE     256     ///< 日志缓冲区大小

/* Private variables ---------------------------------------------------------*/
static char log_buffer[LOG_BUFFER_SIZE];  ///< 日志格式化缓冲区

/* Exported functions --------------------------------------------------------*/

/**
  * @brief 初始化日志模块
  */
void Log_Init(void)
{
    // UART1已在MX_USART1_UART_Init()中初始化
    // 这里可以添加额外的初始化操作
    LOG_INFO("Log module initialized, UART1 baudrate: 115200\r\n");
}

/**
  * @brief 通过UART1发送单个字符(用于printf重定向)
  * @param ch: 字符
  */
void UART1_SendChar(uint8_t ch)
{
    HAL_UART_Transmit(&huart1, &ch, 1, 100);
}

/**
  * @brief 通过UART1发送字符串
  * @param str: 字符串指针
  */
void UART1_SendString(const char *str)
{
    uint16_t len = strlen(str);
    if (len > 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)str, len, 1000);
    }
}

/**
  * @brief 直接通过UART1发送数据
  * @param data: 数据指针
  * @param len: 数据长度
  */
void UART1_SendData(uint8_t *data, uint16_t len)
{
    if (len > 0) {
        HAL_UART_Transmit(&huart1, data, len, 1000);
    }
}

/**
  * @brief 打印日志
  * @param level: 日志级别
  * @param fmt: 格式化字符串
  * @param ...: 可变参数
  */
void Log_Printf(uint8_t level, const char *fmt, ...)
{
    // 检查日志级别
    if (level > LOG_CURRENT_LEVEL) {
        return;
    }
    
    // 格式化日志内容
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(log_buffer, LOG_BUFFER_SIZE, fmt, args);
    va_end(args);
    
    // 确保不超过缓冲区大小
    if (len >= LOG_BUFFER_SIZE) {
        len = LOG_BUFFER_SIZE - 1;
    }
    
    // 发送日志
    UART1_SendString(log_buffer);
}

/* 以下函数用于重定向printf到UART1(可选) */

/**
  * @brief 重定向fputc到UART1(支持标准printf)
  * @note  需要在IDE中勾选"Use MicroLIB"(Keil)或添加链接选项
  */
#if 0  // 默认禁用，如需使用请改为 #if 1
int fputc(int ch, FILE *f)
{
    UART1_SendChar((uint8_t)ch);
    return ch;
}

/**
  * @brief 重定向fgetc从UART1(可选)
  */
int fgetc(FILE *f)
{
    uint8_t ch = 0;
    HAL_UART_Receive(&huart1, &ch, 1, 100);
    return ch;
}
#endif

/**
  * @brief 半主机模式支持(防止链接错误)
  */
void _sys_exit(int x)
{
    x = x;
    while (1);
}

/**
  * @brief 半主机模式支持
  */
struct __FILE 
{
    int handle;
};

FILE __stdout;
FILE __stdin;
