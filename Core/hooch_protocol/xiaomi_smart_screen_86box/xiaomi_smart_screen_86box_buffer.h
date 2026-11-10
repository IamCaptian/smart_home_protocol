#ifndef __XIAOMI_SMART_SCREEN_86BOX_BUFFER_H__
#define __XIAOMI_SMART_SCREEN_86BOX_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <string.h>

/*
 * 乐式 86 盒子全面屏模组串口协议。
 * 帧格式: 55 AA | 版本 | 命令 | 数据长度(2字节,高字节在前) | 数据 | 检验和
 * 检验和: 从帧头开始按字节求和, 对 256 求余。
 */

#ifndef XIAOMI_86BOX_LOG_PRINTF
#include "user_printf.h"
/* 直接调用 Log_Printf, 避免逐字节/逐帧打印时每行都带 "[INFO]" 前缀 */
#define XIAOMI_86BOX_LOG_PRINTF(...) Log_Printf(LOG_LEVEL_INFO, __VA_ARGS__)
#endif

#ifndef XIAOMI_86BOX_LOG_ENABLE
#define XIAOMI_86BOX_LOG_ENABLE             0
#endif

#ifndef XIAOMI_86BOX_LOG_RX_BYTE_ENABLE
#define XIAOMI_86BOX_LOG_RX_BYTE_ENABLE     1
#endif

#ifndef XIAOMI_86BOX_LOG_TX_HEX_ENABLE
#define XIAOMI_86BOX_LOG_TX_HEX_ENABLE      1
#endif

/* 接收帧十六进制打印开关: 每收到一帧完整数据就打印整帧原始字节 */
#ifndef XIAOMI_86BOX_LOG_RX_FRAME_ENABLE
#define XIAOMI_86BOX_LOG_RX_FRAME_ENABLE    1
#endif

/* 发送接口: 默认使用 USART2 阻塞发送, 移植时只改这一处即可 */
#ifndef XIAOMI_86BOX_TX_SEND
#define XIAOMI_86BOX_TX_SEND(buffer, length) \
    HAL_UART_Transmit(&huart2, (buffer), (length), 1000U)
#endif

#if XIAOMI_86BOX_LOG_ENABLE
#define XIAOMI_86BOX_LOG_INFO(...)  XIAOMI_86BOX_LOG_PRINTF(__VA_ARGS__)
#define XIAOMI_86BOX_LOG_WARN(...)  XIAOMI_86BOX_LOG_PRINTF(__VA_ARGS__)
#define XIAOMI_86BOX_LOG_DEBUG(...) XIAOMI_86BOX_LOG_PRINTF(__VA_ARGS__)
#define XIAOMI_86BOX_LOG_ERROR(...) XIAOMI_86BOX_LOG_PRINTF(__VA_ARGS__)
#else
#define XIAOMI_86BOX_LOG_INFO(...)  ((void)0)
#define XIAOMI_86BOX_LOG_WARN(...)  ((void)0)
#define XIAOMI_86BOX_LOG_DEBUG(...) ((void)0)
#define XIAOMI_86BOX_LOG_ERROR(...) ((void)0)
#endif

#if XIAOMI_86BOX_LOG_RX_BYTE_ENABLE
#define XIAOMI_86BOX_LOG_RX_BYTE(byte) \
    XIAOMI_86BOX_LOG_PRINTF("BYTE %02X\r\n", (unsigned int)(byte))
#else
#define XIAOMI_86BOX_LOG_RX_BYTE(byte) ((void)(byte))
#endif

#if XIAOMI_86BOX_LOG_RX_FRAME_ENABLE
#define XIAOMI_86BOX_LOG_RX_FRAME_BEGIN(len) \
    XIAOMI_86BOX_LOG_PRINTF("RX [%u]: ", (unsigned int)(len))
#define XIAOMI_86BOX_LOG_RX_FRAME_BYTE(byte) \
    XIAOMI_86BOX_LOG_PRINTF("%02X ", (unsigned int)(byte))
#define XIAOMI_86BOX_LOG_RX_FRAME_END() \
    XIAOMI_86BOX_LOG_PRINTF("\r\n")
#else
#define XIAOMI_86BOX_LOG_RX_FRAME_BEGIN(len) ((void)(len))
#define XIAOMI_86BOX_LOG_RX_FRAME_BYTE(byte) ((void)(byte))
#define XIAOMI_86BOX_LOG_RX_FRAME_END()      ((void)0)
#endif

#if XIAOMI_86BOX_LOG_TX_HEX_ENABLE
#define XIAOMI_86BOX_LOG_TX_HEX_BEGIN(len) \
    XIAOMI_86BOX_LOG_PRINTF("TX [%u]: ", (unsigned int)(len))
#define XIAOMI_86BOX_LOG_TX_HEX_BYTE(byte) \
    XIAOMI_86BOX_LOG_PRINTF("%02X ", (unsigned int)(byte))
#define XIAOMI_86BOX_LOG_TX_HEX_END() \
    XIAOMI_86BOX_LOG_PRINTF("\r\n")
#else
#define XIAOMI_86BOX_LOG_TX_HEX_BEGIN(len) ((void)(len))
#define XIAOMI_86BOX_LOG_TX_HEX_BYTE(byte) ((void)(byte))
#define XIAOMI_86BOX_LOG_TX_HEX_END()      ((void)0)
#endif

/* 帧格式定义 */
#define XIAOMI_86BOX_FRAME_HEADER          0x55AAU
#define XIAOMI_86BOX_FRAME_HEADER_LEN      2U
#define XIAOMI_86BOX_FRAME_VERSION_LEN     1U
#define XIAOMI_86BOX_FRAME_CMD_LEN         1U
#define XIAOMI_86BOX_FRAME_DATA_LEN_LEN    2U
#define XIAOMI_86BOX_FRAME_CHECKSUM_LEN    1U
#define XIAOMI_86BOX_FRAME_MIN_LEN         (XIAOMI_86BOX_FRAME_HEADER_LEN + \
                                            XIAOMI_86BOX_FRAME_VERSION_LEN + \
                                            XIAOMI_86BOX_FRAME_CMD_LEN + \
                                            XIAOMI_86BOX_FRAME_DATA_LEN_LEN + \
                                            XIAOMI_86BOX_FRAME_CHECKSUM_LEN)

/* 环形缓冲区与帧长度配置 */
#define XIAOMI_86BOX_RING_BUFFER_SIZE      256U
#define XIAOMI_86BOX_MAX_FRAME_DATA_LEN    128U
#define XIAOMI_86BOX_MAX_FRAME_LEN         (XIAOMI_86BOX_FRAME_MIN_LEN + XIAOMI_86BOX_MAX_FRAME_DATA_LEN)

/* 帧结构体 */
typedef struct
{
    uint16_t header;                                /* 帧头 0x55AA */
    uint8_t  version;                               /* 版本 */
    uint8_t  command;                               /* 命令 */
    uint16_t data_len;                              /* 数据长度 */
    uint8_t  data[XIAOMI_86BOX_MAX_FRAME_DATA_LEN]; /* 数据 */
    uint8_t  checksum;                              /* 检验和 */
} XIAOMI_86BOX_Frame_t;

/* 环形缓冲区结构体 */
typedef struct
{
    uint8_t  buffer[XIAOMI_86BOX_RING_BUFFER_SIZE];
    uint16_t head;                                  /* 写入位置 */
    uint16_t tail;                                  /* 读取位置 */
    uint16_t count;                                 /* 数据数量 */
} XIAOMI_86BOX_RingBuffer_t;

/* 帧解析器状态 */
typedef enum
{
    XIAOMI_86BOX_FRAME_STATE_IDLE = 0,
    XIAOMI_86BOX_FRAME_STATE_HEADER_H,
    XIAOMI_86BOX_FRAME_STATE_HEADER_L,
    XIAOMI_86BOX_FRAME_STATE_VERSION,
    XIAOMI_86BOX_FRAME_STATE_CMD,
    XIAOMI_86BOX_FRAME_STATE_DATA_LEN_H,
    XIAOMI_86BOX_FRAME_STATE_DATA_LEN_L,
    XIAOMI_86BOX_FRAME_STATE_DATA,
    XIAOMI_86BOX_FRAME_STATE_CHECKSUM
} XIAOMI_86BOX_FrameState_t;

/* 帧解析器结构体 */
typedef struct
{
    XIAOMI_86BOX_FrameState_t state;
    XIAOMI_86BOX_Frame_t      frame;
    uint16_t                  data_index;
    uint8_t                   calc_checksum;
    uint8_t                   frame_ready;
} XIAOMI_86BOX_FrameParser_t;

/* 对外接口 */
void xiaomi_86box_uart_init(void);
void xiaomi_86box_uart_process(void);
uint8_t xiaomi_86box_uart_send_frame(XIAOMI_86BOX_Frame_t *frame);
void xiaomi_86box_uart_rx_callback(uint8_t data);
uint8_t xiaomi_86box_get_version(void);

/* 主动下发: 重置模块(0x83, 数据 1=重置模块), 用于 CLI/上层测试 */
uint8_t xiaomi_86box_send_reset_module(void);

#ifdef __cplusplus
}
#endif

#endif /* __XIAOMI_SMART_SCREEN_86BOX_BUFFER_H__ */
