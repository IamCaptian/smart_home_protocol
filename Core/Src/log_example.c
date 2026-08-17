/**
  ******************************************************************************
  * @file    log_example.c
  * @brief   日志模块使用示例
  ******************************************************************************
  */

/* 
 * 使用示例：
 * 
 * 在 main.c 中包含头文件：
 * #include "user_printf.h"
 * 
 * 在 main() 函数中初始化：
 * Log_Init();
 * 
 * 在代码中使用日志：
 */

/* 示例代码（仅供参考，不要编译此文件）*/

#if 0

#include "user_printf.h"

void example_function(void)
{
    // 1. 使用宏定义打印不同级别的日志
    LOG_ERROR("System error code: %d\r\n", 0x01);
    LOG_WARN("Temperature warning: %d C\r\n", 85);
    LOG_INFO("System running, count: %d\r\n", 100);
    LOG_DEBUG("Debug variable: 0x%04X\r\n", 0xABCD);
    
    // 2. 直接发送字符串
    UART1_SendString("Hello UART1!\r\n");
    
    // 3. 发送数据
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    UART1_SendData(data, 4);
    
    // 4. 发送单个字符
    UART1_SendChar('A');
    
    // 5. 使用标准printf（需要启用fputc重定向）
    // printf("Standard printf: %d\r\n", 123);
}

/* 
 * 日志级别控制：
 * 在 user_printf.h 中修改 LOG_CURRENT_LEVEL
 * 
 * #define LOG_CURRENT_LEVEL   LOG_LEVEL_DEBUG   // 输出所有日志
 * #define LOG_CURRENT_LEVEL   LOG_LEVEL_INFO    // 只输出 ERROR, WARN, INFO
 * #define LOG_CURRENT_LEVEL   LOG_LEVEL_WARN    // 只输出 ERROR, WARN
 * #define LOG_CURRENT_LEVEL   LOG_LEVEL_ERROR   // 只输出 ERROR
 */

#endif
