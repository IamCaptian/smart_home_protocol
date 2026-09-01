#ifndef __XIAONI_SMART_SCREEN_UART_H
#define __XIAONI_SMART_SCREEN_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <string.h>

/*
 * Logging configuration for Xiaomi smart screen module.
 *
 * Default behavior keeps the current project compatible with user_printf().
 * When porting this module, you only need to replace the macro below or
 * disable the logs here instead of editing all source files.
 */
#ifndef XIAOMI_SMART_SCREEN_LOG_PRINTF
#include "user_printf.h"
#define XIAOMI_SMART_SCREEN_LOG_PRINTF(...) LOG_INFO(__VA_ARGS__)
#endif


#ifndef XIAOMI_SMART_SCREEN_LOG_ENABLE
#define XIAOMI_SMART_SCREEN_LOG_ENABLE      0
#endif

#ifndef XIAOMI_SMART_SCREEN_LOG_RX_BYTE_ENABLE
#define XIAOMI_SMART_SCREEN_LOG_RX_BYTE_ENABLE 0
#endif

#ifndef XIAOMI_SMART_SCREEN_LOG_TX_HEX_ENABLE
#define XIAOMI_SMART_SCREEN_LOG_TX_HEX_ENABLE  0
#endif

/*
 * TX interface selection for Xiaomi smart screen module.
 *
 * Default uses blocking HAL UART transmit on USART2.
 * To switch to another interface, only override this macro here.
 */
#ifndef XIAOMI_SMART_SCREEN_TX_SEND
#define XIAOMI_SMART_SCREEN_TX_SEND(buffer, length) \
    HAL_UART_Transmit(&huart2, (buffer), (length), 1000U)
#endif

#if XIAOMI_SMART_SCREEN_LOG_ENABLE
#define XIAOMI_SMART_SCREEN_LOG_INFO(...)  XIAOMI_SMART_SCREEN_LOG_PRINTF(__VA_ARGS__)
#define XIAOMI_SMART_SCREEN_LOG_WARN(...)  XIAOMI_SMART_SCREEN_LOG_PRINTF(__VA_ARGS__)
#define XIAOMI_SMART_SCREEN_LOG_DEBUG(...) XIAOMI_SMART_SCREEN_LOG_PRINTF(__VA_ARGS__)
#define XIAOMI_SMART_SCREEN_LOG_ERROR(...) XIAOMI_SMART_SCREEN_LOG_PRINTF(__VA_ARGS__)
#else
#define XIAOMI_SMART_SCREEN_LOG_INFO(...)  ((void)0)
#define XIAOMI_SMART_SCREEN_LOG_WARN(...)  ((void)0)
#define XIAOMI_SMART_SCREEN_LOG_DEBUG(...) ((void)0)
#define XIAOMI_SMART_SCREEN_LOG_ERROR(...) ((void)0)
#endif

#if XIAOMI_SMART_SCREEN_LOG_RX_BYTE_ENABLE
#define XIAOMI_SMART_SCREEN_LOG_RX_BYTE(byte) \
    XIAOMI_SMART_SCREEN_LOG_INFO(" %02X\r\n", (unsigned int)(byte))
#else
#define XIAOMI_SMART_SCREEN_LOG_RX_BYTE(byte) ((void)(byte))
#endif

#if XIAOMI_SMART_SCREEN_LOG_TX_HEX_ENABLE
#define XIAOMI_SMART_SCREEN_LOG_TX_HEX_BEGIN(len) \
    XIAOMI_SMART_SCREEN_LOG_INFO("TX [%u bytes]: ", (unsigned int)(len))
#define XIAOMI_SMART_SCREEN_LOG_TX_HEX_BYTE(byte) \
    Log_Printf(LOG_LEVEL_INFO, "%02X ", (unsigned int)(byte))
#define XIAOMI_SMART_SCREEN_LOG_TX_HEX_END() \
    Log_Printf(LOG_LEVEL_INFO, "\r\n")
#else
#define XIAOMI_SMART_SCREEN_LOG_TX_HEX_BEGIN(len) ((void)(len))
#define XIAOMI_SMART_SCREEN_LOG_TX_HEX_BYTE(byte) ((void)(byte))
#define XIAOMI_SMART_SCREEN_LOG_TX_HEX_END()      ((void)0)
#endif

/* 帧格式定义 */
#define FRAME_HEADER          0x55AA    /* 帧头 */
#define FRAME_HEADER_LEN      2         /* 帧头长度 */
#define FRAME_VERSION_LEN     1         /* 版本长度 */
#define FRAME_CMD_LEN         1         /* 命令长度 */
#define FRAME_DATA_LEN_LEN    2         /* 数据长度字段长度 */
#define FRAME_CHECKSUM_LEN    1         /* 检验和长度 */
#define FRAME_MIN_LEN         (FRAME_HEADER_LEN + FRAME_VERSION_LEN + FRAME_CMD_LEN + FRAME_DATA_LEN_LEN + FRAME_CHECKSUM_LEN)  /* 最小帧长度 */

/* 环形缓冲区配置 */
#define RING_BUFFER_SIZE      256       /* 环形缓冲区大小 */
#define MAX_FRAME_DATA_LEN    128       /* 最大数据长度 */
#define MAX_FRAME_LEN         (FRAME_MIN_LEN + MAX_FRAME_DATA_LEN)  /* 最大帧长度 */

/* 帧结构体 */
typedef struct {
    uint16_t header;        /* 帧头 0x55AA */
    uint8_t version;        /* 版本 */
    uint8_t command;        /* 命令 */
    uint16_t data_len;      /* 数据长度 */
    uint8_t data[MAX_FRAME_DATA_LEN];  /* 数据 */
    uint8_t checksum;       /* 检验和 */
} Frame_t;

/* 环形缓冲区结构体 */
typedef struct {
    uint8_t buffer[RING_BUFFER_SIZE];
    uint16_t head;          /* 写入位置 */
    uint16_t tail;          /* 读取位置 */
    uint16_t count;         /* 缓冲区中的数据数量 */
} RingBuffer_t;

/* 帧解析器状态 */
typedef enum {
    FRAME_STATE_IDLE,           /* 空闲状态，等待帧头 */
    FRAME_STATE_HEADER_H,       /* 已接收到帧头高字节 0x55 */
    FRAME_STATE_HEADER_L,       /* 已接收到帧头低字节 0xAA */
    FRAME_STATE_VERSION,        /* 接收版本 */
    FRAME_STATE_CMD,            /* 接收命令 */
    FRAME_STATE_DATA_LEN_H,     /* 接收数据长度高字节 */
    FRAME_STATE_DATA_LEN_L,     /* 接收数据长度低字节 */
    FRAME_STATE_DATA,           /* 接收数据 */
    FRAME_STATE_CHECKSUM        /* 接收检验和 */
} FrameState_t;

/* 帧解析器结构体 */
typedef struct {
    FrameState_t state;         /* 当前状态 */
    Frame_t frame;              /* 当前解析的帧 */
    uint16_t data_index;        /* 数据索引 */
    uint8_t calc_checksum;      /* 计算中的检验和 */
    uint8_t frame_ready;        /* 帧就绪标志 */
} FrameParser_t;

/* 函数声明 */
void RingBuffer_Init(RingBuffer_t *rb);
uint8_t RingBuffer_Push(RingBuffer_t *rb, uint8_t data);
uint8_t RingBuffer_Pop(RingBuffer_t *rb, uint8_t *data);
uint16_t RingBuffer_GetCount(RingBuffer_t *rb);
uint8_t RingBuffer_IsEmpty(RingBuffer_t *rb);
uint8_t RingBuffer_IsFull(RingBuffer_t *rb);

void FrameParser_Init(FrameParser_t *parser);
void FrameParser_ProcessByte(FrameParser_t *parser, uint8_t byte, Frame_t *out_frame);
uint8_t FrameParser_CalcChecksum(Frame_t *frame);
uint8_t FrameParser_VerifyChecksum(Frame_t *frame);

void xiaoni_smart_screen_uart_init(void);
void xiaoni_smart_screen_uart_process(void);
uint8_t xiaoni_smart_screen_uart_sendframe(Frame_t *frame);
void xiaoni_smart_screen_uart_rxcallback(uint8_t data);
uint8_t xiaoni_smart_screen_get_version(void);

/* 帧处理回调函数声明 */
void xiaomi_smart_screen_handle_frame(Frame_t *frame);
void xiaomi_quan_reply_frame(unsigned char command);

#ifdef __cplusplus
}
#endif

#endif /* __XIAONI_SMART_SCREEN_UART_H */
