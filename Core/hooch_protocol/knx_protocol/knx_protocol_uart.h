#ifndef KNX_PROTOCOL_UART_H
#define KNX_PROTOCOL_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

#ifndef KNX_LOG_ENABLE
#define KNX_LOG_ENABLE 0
#endif

#if KNX_LOG_ENABLE
#ifndef KNX_LOG_PRINTF
#include "user_printf.h"
#define KNX_LOG_PRINTF(...) LOG_INFO(__VA_ARGS__)
#endif
#else
#ifndef KNX_LOG_PRINTF
#define KNX_LOG_PRINTF(...) ((void)0)
#endif
#endif

#ifndef KNX_LOG_RX_BYTE_ENABLE
#define KNX_LOG_RX_BYTE_ENABLE 1
#endif

#ifndef KNX_LOG_TX_HEX_ENABLE
#define KNX_LOG_TX_HEX_ENABLE 1
#endif

#if !KNX_LOG_ENABLE
#undef KNX_LOG_RX_BYTE_ENABLE
#define KNX_LOG_RX_BYTE_ENABLE 1
#undef KNX_LOG_TX_HEX_ENABLE
#define KNX_LOG_TX_HEX_ENABLE 0
#endif

#ifndef KNX_TX_SEND
#define KNX_TX_SEND(buffer, length) HAL_UART_Transmit(&huart2, (buffer), (length), 1000U)
#endif

#if KNX_LOG_ENABLE
#define KNX_LOG_INFO(...)  KNX_LOG_PRINTF(__VA_ARGS__)
#define KNX_LOG_WARN(...)  KNX_LOG_PRINTF(__VA_ARGS__)
#define KNX_LOG_DEBUG(...) KNX_LOG_PRINTF(__VA_ARGS__)
#define KNX_LOG_ERROR(...) KNX_LOG_PRINTF(__VA_ARGS__)
#else
#define KNX_LOG_INFO(...)  ((void)0)
#define KNX_LOG_WARN(...)  ((void)0)
#define KNX_LOG_DEBUG(...) ((void)0)
#define KNX_LOG_ERROR(...) ((void)0)
#endif

#if KNX_LOG_RX_BYTE_ENABLE
#define KNX_LOG_RX_BYTE(byte) KNX_LOG_INFO("R %02X\r\n", (unsigned int)(byte))
#else
#define KNX_LOG_RX_BYTE(byte) ((void)(byte))
#endif

#if KNX_LOG_TX_HEX_ENABLE
#define KNX_LOG_TX_HEX_BEGIN(len) KNX_LOG_INFO("T %u: ", (unsigned int)(len))
#define KNX_LOG_TX_HEX_BYTE(byte) KNX_LOG_INFO("%02X ", (unsigned int)(byte))
#define KNX_LOG_TX_HEX_END()      KNX_LOG_INFO("\r\n")
#else
#define KNX_LOG_TX_HEX_BEGIN(len) ((void)(len))
#define KNX_LOG_TX_HEX_BYTE(byte) ((void)(byte))
#define KNX_LOG_TX_HEX_END()      ((void)0)
#endif

/* RX 整帧打印（非逐字节，帧收完后一次性输出） */
#ifndef KNX_LOG_RX_FRAME_ENABLE
#define KNX_LOG_RX_FRAME_ENABLE 1
#endif

#if !KNX_LOG_ENABLE
#undef  KNX_LOG_RX_FRAME_ENABLE
#define KNX_LOG_RX_FRAME_ENABLE 0
#endif

#if KNX_LOG_RX_FRAME_ENABLE
#define KNX_LOG_RX_FRAME_BEGIN(len) KNX_LOG_INFO("R %u: ", (unsigned int)(len))
#define KNX_LOG_RX_FRAME_BYTE(byte) KNX_LOG_INFO("%02X ", (unsigned int)(byte))
#define KNX_LOG_RX_FRAME_END()      KNX_LOG_INFO("\r\n")
#else
#define KNX_LOG_RX_FRAME_BEGIN(len) ((void)(len))
#define KNX_LOG_RX_FRAME_BYTE(byte) ((void)(byte))
#define KNX_LOG_RX_FRAME_END()      ((void)0)
#endif

/*
 * 面板通信协议（文档：面板通信协议 V2.4）解析模块对外接口。
 *
 * 设计目标：
 * 1) 中断仅负责收字节：USART IRQ 调用 knx_uart_rxcallback(data)；
 * 2) 主循环负责拼帧/校验/分发：周期调用 knx_uart_process()；
 * 3) 解析出的业务事件映射到 HOOCH_PROTOCOL，从而触发 main.c 注册的回调。
 */

#define KNX_FRAME_HEADER                 0x55AAU
#define KNX_FRAME_HEADER_H               0x55U
#define KNX_FRAME_HEADER_L               0xAAU

/* fun 字段最大数量（协议表中 Fun1..Funx）。 */
#define KNX_MAX_FUN_COUNT                8U

/* 配置写(C1)最大数据长度。 */
#define KNX_MAX_DATA_LEN                 256U

/* 接收环形缓冲区大小。 */
#define KNX_RING_BUFFER_SIZE             256U

typedef enum
{
    /* 写数据（带 fun + dataLen + data）。 */
    KNX_CMD_WRITE_DATA = 0xA1U,
    /* 查询数据（带 fun，不带 data）。 */
    KNX_CMD_QUERY_DATA = 0xA2U,
    /* 请求配置数据（带 fun，不带 data，V2.4 交互说明中暂预留）。 */
    KNX_CMD_REQUEST_CONFIG = 0xA3U,
    /* 写配置数据（带 address + dataLen + data，V2.4 交互说明中暂预留）。 */
    KNX_CMD_WRITE_CONFIG = 0xC1U,
    /* 请求配置数据（带 address + dataLen，不带 data，V2.4 交互说明中暂预留）。 */
    KNX_CMD_READ_CONFIG = 0xC2U,
    /* 应答命令，格式：55 AA C3 resp_type checksum。 */
    KNX_CMD_RESPONSE = 0xC3U,
} KNX_Command_t;

typedef enum
{
    /* 收到 A1/A2 后的正常应答：55 AA C3 C3 checksum。 */
    KNX_RESPONSE_TYPE_ACK = 0xC3U,
    /* 心跳包：55 AA C3 CA checksum。 */
    KNX_RESPONSE_TYPE_HEARTBEAT = 0xCAU,
    /* 繁忙应答：55 AA C3 CB checksum。 */
    KNX_RESPONSE_TYPE_BUSY = 0xCBU,
    /* 错误应答：55 AA C3 CE checksum。 */
    KNX_RESPONSE_TYPE_ERROR = 0xCEU,
} KNX_ResponseType_t;

typedef struct
{
    /* 帧头：固定 0x55AA。 */
    uint16_t header;
    /* 命令字：A1/A2/A3/C1/C2/C3。 */
    uint8_t command;

    /* fun 数量：后续 fun[] 的有效长度。 */
    uint8_t fun_count;
    /* fun 字段（Fun1..Funx）。 */
    uint8_t fun[KNX_MAX_FUN_COUNT];

    /* 地址字段（仅 C1/C2 类帧使用）。 */
    uint16_t address;

    /* 数据长度：
     * - A1：1字节长度，解析时放入 data_len（uint16_t）
     * - C1/C2：2字节长度（高字节在前）
     */
    uint16_t data_len;
    /* 数据区（A1/C1 才会填充）。 */
    uint8_t data[KNX_MAX_DATA_LEN];

    /* 应答类型（仅 C3 类帧使用，典型值：C3/CA/CB/CE）。 */
    uint8_t resp_type;

    /* 校验和：8bit 累加和（对除 checksum 外所有字节求和 mod256）。 */
    uint8_t checksum;
} KNX_Frame_t;

/* 初始化 KNX 协议模块（清空环形缓冲区/解析状态）。 */
void knx_uart_init(void);

/* 主循环处理：从环形缓冲区取字节，状态机拼帧，校验通过后调用 knx_handle_frame。 */
void knx_uart_process(void);

/* 中断喂入单字节（应在 USART RXNE 中断中调用）。 */
void knx_uart_rxcallback(uint8_t data);

/* 发送应答帧：
 * 协议格式：55 AA C3 resp_type checksum
 * resp_type 参考 KNX_ResponseType_t。
 */
uint8_t knx_uart_send_ack(uint8_t resp_type);

/* 发送业务写帧（A1）：
 * 格式：55 AA A1 fun_count fun[] data_len(1B) data checksum
 */
uint8_t knx_uart_send_write_frame(uint8_t fun_count,
                                  const uint8_t *fun,
                                  uint16_t data_len,
                                  const uint8_t *data);

/* 发送业务查询帧（A2）：
 * 格式：55 AA A2 fun_count fun[] checksum
 */
uint8_t knx_uart_send_query_frame(uint8_t fun_count,
                                  const uint8_t *fun);

/* 发送业务配置请求帧（A3）：
 * 格式：55 AA A3 fun_count fun[] checksum
 */
uint8_t knx_uart_send_request_config_frame(uint8_t fun_count,
                                           const uint8_t *fun);

/* 发送配置写帧（C1）。 */
uint8_t knx_uart_send_write_config(uint16_t address,
                                   uint16_t data_len,
                                   const uint8_t *data);

/* 发送配置读帧（C2）。 */
uint8_t knx_uart_send_read_config(uint16_t address,
                                  uint16_t data_len);

#ifdef __cplusplus
}
#endif

#endif /* KNX_PROTOCOL_UART_H */
