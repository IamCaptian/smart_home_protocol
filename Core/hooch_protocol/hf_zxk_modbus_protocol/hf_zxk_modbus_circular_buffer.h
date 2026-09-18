#ifndef __HF_ZXK_MODBUS_CIRCULAR_BUFFER_H
#define __HF_ZXK_MODBUS_CIRCULAR_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <string.h>

/*
 * 线控器 MODBUS-RTU 协议 —— 传输层（环形缓冲 + 接收状态机 + 帧解析）
 *
 * 物理层：USART2（9600 8N1），与小米/KNX/Tuya 等面板协议共享同一字节流；
 *         在 USART2 中断里把每个字节喂给 hf_zxk_modbus_uart_rxcallback()，
 *         在主循环里调用 hf_zxk_modbus_uart_process() 完成解析。
 *
 * 本机角色：MODBUS 主机(轮询方)，因此本模块接收到的都是空调网关(从机)的回复帧：
 *           0x03 读保持寄存器回复 / 0x06 写单寄存器回显 / 0x10 写多寄存器回显 / 异常帧
 */

/* ---------------- 日志配置（仿 xiaomi_smart_screen 结构） ---------------- */
/* 与工程内其它日志保持一致的输出通道，移植时只需替换此宏 */
#ifndef HF_ZXK_MODBUS_LOG_PRINTF
#include "user_printf.h"
#define HF_ZXK_MODBUS_LOG_PRINTF(...) LOG_INFO(__VA_ARGS__)
#endif

/* 置 0 可在编译期彻底裁剪掉本模块所有日志字符串 */
#ifndef HF_ZXK_MODBUS_LOG_ENABLE
#define HF_ZXK_MODBUS_LOG_ENABLE 1
#endif

/* 逐字节接收日志（调试接收状态机时打开） */
#ifndef HF_ZXK_MODBUS_LOG_RX_BYTE_ENABLE
#define HF_ZXK_MODBUS_LOG_RX_BYTE_ENABLE 0
#endif

/* 发送原始字节十六进制日志 */
#ifndef HF_ZXK_MODBUS_LOG_TX_HEX_ENABLE
#define HF_ZXK_MODBUS_LOG_TX_HEX_ENABLE 0
#endif

/* ---------------- TX 接口选择 ---------------- */
/* 默认与其他面板协议共用 USART2 阻塞发送，切换传输方式只需覆盖此宏 */
#ifndef HF_ZXK_MODBUS_TX_SEND
#define HF_ZXK_MODBUS_TX_SEND(buffer, length) \
    HAL_UART_Transmit(&huart2, (buffer), (length), 1000U)
#endif

#if HF_ZXK_MODBUS_LOG_ENABLE
#define HF_ZXK_MODBUS_LOG_INFO(...)  HF_ZXK_MODBUS_LOG_PRINTF(__VA_ARGS__)
#define HF_ZXK_MODBUS_LOG_WARN(...)  HF_ZXK_MODBUS_LOG_PRINTF(__VA_ARGS__)
#define HF_ZXK_MODBUS_LOG_DEBUG(...) HF_ZXK_MODBUS_LOG_PRINTF(__VA_ARGS__)
#else
#define HF_ZXK_MODBUS_LOG_INFO(...)  ((void)0)
#define HF_ZXK_MODBUS_LOG_WARN(...)  ((void)0)
#define HF_ZXK_MODBUS_LOG_DEBUG(...) ((void)0)
#endif

#if HF_ZXK_MODBUS_LOG_RX_BYTE_ENABLE
#define HF_ZXK_MODBUS_LOG_RX_BYTE(byte) \
    HF_ZXK_MODBUS_LOG_INFO(" %02X", (unsigned int)(byte))
#else
#define HF_ZXK_MODBUS_LOG_RX_BYTE(byte) ((void)(byte))
#endif

#if HF_ZXK_MODBUS_LOG_TX_HEX_ENABLE
#define HF_ZXK_MODBUS_LOG_TX_HEX_BEGIN(len) \
    HF_ZXK_MODBUS_LOG_INFO("[MODBUS] TX [%u bytes]:", (unsigned int)(len))
#define HF_ZXK_MODBUS_LOG_TX_HEX_BYTE(byte) \
    HF_ZXK_MODBUS_LOG_INFO(" %02X", (unsigned int)(byte))
#define HF_ZXK_MODBUS_LOG_TX_HEX_END() \
    HF_ZXK_MODBUS_LOG_INFO("\r\n")
#else
#define HF_ZXK_MODBUS_LOG_TX_HEX_BEGIN(len) ((void)(len))
#define HF_ZXK_MODBUS_LOG_TX_HEX_BYTE(byte) ((void)(byte))
#define HF_ZXK_MODBUS_LOG_TX_HEX_END()      ((void)0)
#endif

/* ---------------- MODBUS-RTU 帧格式 ----------------
 * +---------+---------+------------------+--------+--------+
 * | 从机地址 | 功能码  |      数据域       | CRC低   | CRC高  |
 * +---------+---------+------------------+--------+--------+
 * 地址/功能码各 1 字节；CRC 为 CRC-16/MODBUS（反射多项式 0xA001、初值 0xFFFF），
 * 低字节先发。各功能码的数据域长度：
 *   0x03 回复 : 1 字节字节数 N + N 字节寄存器数据(大端)
 *   0x06 回显 : 2 字节寄存器地址 + 2 字节寄存器值
 *   0x10 回显 : 2 字节起始寄存器地址 + 2 字节寄存器数量
 *   异常帧    : 功能码 | 0x80，后跟 1 字节异常码
 */
#define HF_ZXK_MODBUS_ADDR_LEN        1U
#define HF_ZXK_MODBUS_FUNC_LEN        1U
#define HF_ZXK_MODBUS_CRC_LEN         2U

/* 本协议单台内机最多查询 9 个寄存器(18 字节)，预留到 16 个寄存器 */
#define HF_ZXK_MODBUS_MAX_REG_COUNT   16U
#define HF_ZXK_MODBUS_MAX_DATA_LEN    (HF_ZXK_MODBUS_MAX_REG_COUNT * 2U)
#define HF_ZXK_MODBUS_MAX_FRAME_LEN   (HF_ZXK_MODBUS_MAX_DATA_LEN + \
                                       HF_ZXK_MODBUS_ADDR_LEN + HF_ZXK_MODBUS_FUNC_LEN + \
                                       HF_ZXK_MODBUS_CRC_LEN)

#define HF_ZXK_MODBUS_CRC_INIT        0xFFFFU
#define HF_ZXK_MODBUS_CRC_POLY        0xA001U

/* 485 从机地址范围：485_1 由拨码设置 0~15(无拨码固定 0)，485_2 固定 1 */
#define HF_ZXK_MODBUS_ADDR_PANEL_1_MIN   0U
#define HF_ZXK_MODBUS_ADDR_PANEL_1_MAX   15U
#define HF_ZXK_MODBUS_ADDR_PANEL_2       1U

/* CPU1 协议类型编号（1=涂鸦 2=小米 3=KNX），线控使用 4 */
#define HF_ZXK_MODBUS_DETECTED_PROTOCOL_CODE 4U

/* 功能码 */
typedef enum
{
    HF_ZXK_MODBUS_FUNC_READ_HOLDING   = 0x03,  /* 读保持寄存器 */
    HF_ZXK_MODBUS_FUNC_WRITE_SINGLE   = 0x06,  /* 写单个寄存器 */
    HF_ZXK_MODBUS_FUNC_WRITE_MULTIPLE = 0x10,  /* 写多个寄存器 */
} hf_zxk_modbus_func_t;

/* 异常帧的功能码 = 原功能码 | 0x80 */
#define HF_ZXK_MODBUS_FUNC_EXCEPTION_MASK 0x80U

/* 异常码 */
typedef enum
{
    HF_ZXK_MODBUS_EXCEPTION_ILLEGAL_FUNCTION = 0x01,  /* 功能码不支持 */
    HF_ZXK_MODBUS_EXCEPTION_ILLEGAL_ADDRESS  = 0x02,  /* 寄存器地址非法 */
    HF_ZXK_MODBUS_EXCEPTION_ILLEGAL_VALUE    = 0x03,  /* 数据值非法 */
    HF_ZXK_MODBUS_EXCEPTION_SLAVE_FAILURE    = 0x04,  /* 从机执行失败 */
} hf_zxk_modbus_exception_t;

/* 解析后的 MODBUS 帧（接收方向，即网关回复帧） */
typedef struct
{
    uint8_t  address;                          /* 从机地址 */
    uint8_t  function;                         /* 功能码（异常帧已去掉 0x80 位） */
    uint8_t  is_exception;                     /* 1: 异常帧 */
    uint8_t  exception_code;                   /* 异常码，仅异常帧有效 */
    uint16_t reg_addr;                         /* 0x06/0x10: 寄存器地址 */
    uint16_t reg_value;                        /* 0x06: 写入的寄存器值 */
    uint16_t reg_quantity;                     /* 0x10: 寄存器数量 */
    uint8_t  byte_count;                       /* 0x03: 数据字节数 */
    uint8_t  data[HF_ZXK_MODBUS_MAX_DATA_LEN]; /* 0x03: 寄存器数据(大端) */
    uint16_t crc;                              /* 收到的 CRC 值 */
} hf_zxk_modbus_frame_t;

/* 环形缓冲区配置 */
#define HF_ZXK_MODBUS_RING_BUFFER_SIZE 256U

/* 环形缓冲区结构体 */
typedef struct
{
    uint8_t  buffer[HF_ZXK_MODBUS_RING_BUFFER_SIZE];
    uint16_t head;   /* 写入位置 */
    uint16_t tail;   /* 读取位置 */
    uint16_t count;  /* 缓冲区中的数据数量 */
} hf_zxk_modbus_ring_buffer_t;

/* 接收状态机状态 */
typedef enum
{
    HF_ZXK_MODBUS_STATE_IDLE = 0,   /* 等待从机地址（帧起始） */
    HF_ZXK_MODBUS_STATE_FUNC,       /* 功能码 */
    HF_ZXK_MODBUS_STATE_BYTE_COUNT, /* 0x03: 数据字节数 */
    HF_ZXK_MODBUS_STATE_DATA,       /* 0x03: 寄存器数据 */
    HF_ZXK_MODBUS_STATE_REG_H,      /* 0x06/0x10: 寄存器地址高字节 */
    HF_ZXK_MODBUS_STATE_REG_L,      /* 0x06/0x10: 寄存器地址低字节 */
    HF_ZXK_MODBUS_STATE_VALUE_H,    /* 0x06: 寄存器值高字节 / 0x10: 寄存器数量高字节 */
    HF_ZXK_MODBUS_STATE_VALUE_L,    /* 0x06: 寄存器值低字节 / 0x10: 寄存器数量低字节 */
    HF_ZXK_MODBUS_STATE_EXCEPTION,  /* 异常码 */
    HF_ZXK_MODBUS_STATE_CRC_L,      /* CRC 低字节 */
    HF_ZXK_MODBUS_STATE_CRC_H,      /* CRC 高字节 */
} hf_zxk_modbus_frame_state_t;

/* 帧解析器结构体 */
typedef struct
{
    hf_zxk_modbus_frame_state_t state;       /* 当前状态 */
    hf_zxk_modbus_frame_t       frame;       /* 当前解析中的帧 */
    uint16_t                    data_index;  /* 数据域已收字节数 */
    uint16_t                    calc_crc;    /* 逐字节累积的 CRC */
    uint8_t                     frame_ready; /* 帧就绪标志 */
} hf_zxk_modbus_parser_t;

/* ---------------- 环形缓冲区接口 ---------------- */
void     hf_zxk_modbus_ring_buffer_init(hf_zxk_modbus_ring_buffer_t *rb);
uint8_t  hf_zxk_modbus_ring_buffer_push(hf_zxk_modbus_ring_buffer_t *rb, uint8_t data);
uint8_t  hf_zxk_modbus_ring_buffer_pop(hf_zxk_modbus_ring_buffer_t *rb, uint8_t *data);
uint16_t hf_zxk_modbus_ring_buffer_get_count(hf_zxk_modbus_ring_buffer_t *rb);
uint8_t  hf_zxk_modbus_ring_buffer_is_empty(hf_zxk_modbus_ring_buffer_t *rb);
uint8_t  hf_zxk_modbus_ring_buffer_is_full(hf_zxk_modbus_ring_buffer_t *rb);

/* ---------------- CRC 接口 ---------------- */
/* 逐字节累积计算 CRC（用于接收状态机流式计算） */
uint16_t hf_zxk_modbus_crc_update(uint16_t crc, uint8_t byte);
/* 一次性计算一段数据的 CRC */
uint16_t hf_zxk_modbus_crc16(const uint8_t *data, uint16_t len);

/* ---------------- 帧解析器接口 ---------------- */
void    hf_zxk_modbus_parser_init(hf_zxk_modbus_parser_t *parser);
void    hf_zxk_modbus_parser_process_byte(hf_zxk_modbus_parser_t *parser,
                                          uint8_t byte,
                                          hf_zxk_modbus_frame_t *out_frame);

/* ---------------- UART 接口 ---------------- */
void    hf_zxk_modbus_uart_init(void);
void    hf_zxk_modbus_uart_process(void);
uint8_t hf_zxk_modbus_uart_send_pdu(const uint8_t *pdu, uint16_t pdu_len);
void    hf_zxk_modbus_uart_rxcallback(uint8_t data);

/* 帧处理回调（业务层实现，见 hf_zxk_modbus_handle.h） */
void hf_zxk_modbus_handle_frame(const hf_zxk_modbus_frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif /* __HF_ZXK_MODBUS_CIRCULAR_BUFFER_H */
