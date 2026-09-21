#ifndef __HF_ZJK_MODBUS_CIRCULAR_BUFFER_H
#define __HF_ZJK_MODBUS_CIRCULAR_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <string.h>

/*
 * 集控器 自定义 485 协议 —— 传输层（环形缓冲 + 接收状态机 + 帧解析）
 *
 * 物理层：USART2，协议要求 9600bps / 8 位数据 / 偶校验 / 1 位停止位；
 *         与小米、KNX、Tuya、线控器等面板协议共享同一字节流：
 *         在 USART2 中断里把每个字节喂给 hf_zjk_modbus_uart_rxcallback()，
 *         在主循环里调用 hf_zjk_modbus_uart_process() 完成解析。
 *
 * 本机角色：集成系统(主机)。协议规定"网关不主动发出数据，只响应"，
 *           因此本模块接收到的都是网关的回复帧：
 *             0x50/0x53 状态查询回复 / 控制指令回显 / 0x90 性能查询回复
 */

/* ---------------- 日志配置（仿 hf_zxk_modbus 结构） ---------------- */
/* 与工程内其它日志保持一致的输出通道，移植时只需替换此宏 */
#ifndef HF_ZJK_MODBUS_LOG_PRINTF
#include "user_printf.h"
#define HF_ZJK_MODBUS_LOG_PRINTF(...) LOG_INFO(__VA_ARGS__)
#endif

/* 置 0 可在编译期彻底裁剪掉本模块所有日志字符串 */
#ifndef HF_ZJK_MODBUS_LOG_ENABLE
#define HF_ZJK_MODBUS_LOG_ENABLE 1
#endif

/* 逐字节接收日志（调试接收状态机时打开） */
#ifndef HF_ZJK_MODBUS_LOG_RX_BYTE_ENABLE
#define HF_ZJK_MODBUS_LOG_RX_BYTE_ENABLE 0
#endif

/* 发送原始字节十六进制日志 */
#ifndef HF_ZJK_MODBUS_LOG_TX_HEX_ENABLE
#define HF_ZJK_MODBUS_LOG_TX_HEX_ENABLE 0
#endif

/* ---------------- TX 接口选择 ---------------- */
/* 默认与其他面板协议共用 USART2 阻塞发送，切换传输方式只需覆盖此宏 */
#ifndef HF_ZJK_MODBUS_TX_SEND
#define HF_ZJK_MODBUS_TX_SEND(buffer, length) \
    HAL_UART_Transmit(&huart2, (buffer), (length), 1000U)
#endif

/* ---------------- 模块使能开关 ---------------- */
/* 上电默认使能状态，运行期可用 hf_zjk_modbus_set_enable() 随时开关 */
#ifndef HF_ZJK_MODBUS_ENABLE_DEFAULT
#define HF_ZJK_MODBUS_ENABLE_DEFAULT 0
#endif

#if HF_ZJK_MODBUS_LOG_ENABLE
#define HF_ZJK_MODBUS_LOG_INFO(...)  HF_ZJK_MODBUS_LOG_PRINTF(__VA_ARGS__)
#define HF_ZJK_MODBUS_LOG_WARN(...)  HF_ZJK_MODBUS_LOG_PRINTF(__VA_ARGS__)
#define HF_ZJK_MODBUS_LOG_DEBUG(...) HF_ZJK_MODBUS_LOG_PRINTF(__VA_ARGS__)
#else
#define HF_ZJK_MODBUS_LOG_INFO(...)  ((void)0)
#define HF_ZJK_MODBUS_LOG_WARN(...)  ((void)0)
#define HF_ZJK_MODBUS_LOG_DEBUG(...) ((void)0)
#endif

#if HF_ZJK_MODBUS_LOG_RX_BYTE_ENABLE
#define HF_ZJK_MODBUS_LOG_RX_BYTE(byte) \
    HF_ZJK_MODBUS_LOG_INFO(" %02X", (unsigned int)(byte))
#else
#define HF_ZJK_MODBUS_LOG_RX_BYTE(byte) ((void)(byte))
#endif

#if HF_ZJK_MODBUS_LOG_TX_HEX_ENABLE
#define HF_ZJK_MODBUS_LOG_TX_HEX_BEGIN(len) \
    HF_ZJK_MODBUS_LOG_INFO("[ZJK] TX [%u bytes]:", (unsigned int)(len))
#define HF_ZJK_MODBUS_LOG_TX_HEX_BYTE(byte) \
    HF_ZJK_MODBUS_LOG_INFO(" %02X", (unsigned int)(byte))
#define HF_ZJK_MODBUS_LOG_TX_HEX_END() \
    HF_ZJK_MODBUS_LOG_INFO("\r\n")
#else
#define HF_ZJK_MODBUS_LOG_TX_HEX_BEGIN(len) ((void)(len))
#define HF_ZJK_MODBUS_LOG_TX_HEX_BYTE(byte) ((void)(byte))
#define HF_ZJK_MODBUS_LOG_TX_HEX_END()      ((void)0)
#endif

/* ---------------- 集控自定义 485 帧格式 ----------------
 * 下行（集成系统 -> 网关）:
 *   +----------+--------+--------+----------+------------------+--------+
 *   | 网关地址 | 功能码 | 控制值 | 设备数量 |  设备地址(N x 2) |  校验  |
 *   +----------+--------+--------+----------+------------------+--------+
 * 上行（网关 -> 集成系统）:
 *   +----------+--------+--------+----------+--------------------------+--------+
 *   | 网关地址 | 功能码 | 控制值 | 设备数量 | 设备地址+状态值(N x byte)|  校验  |
 *   +----------+--------+--------+----------+--------------------------+--------+
 *
 * 与 MODBUS 的区别：
 *   1) 校验不是 CRC16，而是"从网关地址到最后一个数据字节"的算术累加和，取低 8 位；
 *   2) 帧长不固定，由"功能码 + 控制值 + 设备数量"共同决定；
 *   3) 控制指令的回显帧中设备地址域固定 2 字节
 *      （单台时为实际地址；多台/全部时网关用 0xFF 0xFF 代替，不再逐个复制）。
 */
#define HF_ZJK_MODBUS_ADDR_LEN        1U
#define HF_ZJK_MODBUS_FUNC_LEN        1U
#define HF_ZJK_MODBUS_CHECKSUM_LEN    1U

/* 设备状态数据块长度：
 *   0x50 常规状态块 10 字节（外/内/开关/设定温度/模式/风速/室温/故障/风向/其他）
 *   0x53 小数状态块 12 字节（设定温度与室温各占 2 字节，其后字段整体后移 2）
 *   在线状态块 3 字节（外/内/在线） */
#define HF_ZJK_MODBUS_DEV_BLOCK_LEN        10U
#define HF_ZJK_MODBUS_DEV_BLOCK_LEN_DEC    12U
#define HF_ZJK_MODBUS_ONLINE_BLOCK_LEN     3U

/* 控制指令回显帧中的设备地址域长度（固定 2 字节） */
#define HF_ZJK_MODBUS_CTRL_ACK_ADDR_LEN    2U

/* 0x90 性能查询回复的数据域长度（不含校验和） */
#define HF_ZJK_MODBUS_PERFORMANCE_DATA_LEN 7U

/* 发送 PDU 长度上限（网关地址+功能码+控制值+设备数量+设备地址）。
 * 与接收缓冲分开定义：接收要考虑"查询全部"的大批量回包，
 * 发送侧只需覆盖状态查询与多台设备控制，避免在 1KB 栈上开大数组。 */
#ifndef HF_ZJK_MODBUS_MAX_TX_PDU_LEN
#define HF_ZJK_MODBUS_MAX_TX_PDU_LEN       64U
#endif

/* 单帧最多容纳的设备数量。
 * 状态查询在"查询全部"时会把整条总线上的内机一次性回传，
 * 这里按 32 台内机预留；超出的帧只保持同步不落数据（frame->overflow = 1）。 */
#ifndef HF_ZJK_MODBUS_MAX_DEVICE_COUNT
#define HF_ZJK_MODBUS_MAX_DEVICE_COUNT     32U
#endif

#define HF_ZJK_MODBUS_MAX_DATA_LEN    (HF_ZJK_MODBUS_MAX_DEVICE_COUNT * \
                                       HF_ZJK_MODBUS_DEV_BLOCK_LEN_DEC)
#define HF_ZJK_MODBUS_MAX_FRAME_LEN   (HF_ZJK_MODBUS_MAX_DATA_LEN + \
                                       HF_ZJK_MODBUS_ADDR_LEN + HF_ZJK_MODBUS_FUNC_LEN + \
                                       HF_ZJK_MODBUS_CTRL_ACK_ADDR_LEN + \
                                       HF_ZJK_MODBUS_CHECKSUM_LEN)

/* 0x04 故障码回复不含设备数量，靠"某字节等于当前累加和"判定校验字节；
 * 为避免数据域前段出现巧合匹配，最少要收满一个完整状态块才允许终结。 */
#define HF_ZJK_MODBUS_SUM_TERMINATED_MIN_LEN HF_ZJK_MODBUS_DEV_BLOCK_LEN

/* 网关 485 地址（可在小程序修改，出厂默认 0x01） */
#define HF_ZJK_MODBUS_DEFAULT_GATEWAY_ADDR 0x01U

/* 全网设备地址域通配值：设备数量与外机/内机地址均填 0xFF 表示"全部设备" */
#define HF_ZJK_MODBUS_ADDR_GROUP_ALL       0xFFU

/* 内机地址域分隔符：设备地址 = (室外机地址 << 8) | 室内机地址 */
#define HF_ZJK_MODBUS_DEV_ADDR_OUTER_SHIFT 8U

/* 功能码（当前仅实现空调相关部分） */
typedef enum
{
    HF_ZJK_MODBUS_FUNC_AC_POWER       = 0x31,  /* 空调：开关 */
    HF_ZJK_MODBUS_FUNC_AC_TEMP        = 0x32,  /* 空调：设定温度 */
    HF_ZJK_MODBUS_FUNC_AC_MODE        = 0x33,  /* 空调：模式 */
    HF_ZJK_MODBUS_FUNC_AC_FAN_SPEED   = 0x34,  /* 空调：风速 */
    HF_ZJK_MODBUS_FUNC_AC_WIND        = 0x35,  /* 空调：风向 */
    HF_ZJK_MODBUS_FUNC_AC_TEMP_FRAC   = 0x36,  /* 空调：设定温度(带小数) */
    HF_ZJK_MODBUS_FUNC_AC_LOCK_POWER  = 0x41,  /* 空调：锁定开关 */
    HF_ZJK_MODBUS_FUNC_AC_LOCK_TEMP_MIN = 0x42,/* 空调：锁定温度下限 */
    HF_ZJK_MODBUS_FUNC_AC_LOCK_TEMP_MAX = 0x43,/* 空调：锁定温度上限 */
    HF_ZJK_MODBUS_FUNC_AC_LOCK_MODE   = 0x44,  /* 空调：锁定模式 */
    HF_ZJK_MODBUS_FUNC_AC_QUERY_STATUS = 0x50, /* 空调：查询状态 */
    HF_ZJK_MODBUS_FUNC_AC_QUERY_STATUS_DEC = 0x53, /* 空调：查询状态(温度含小数) */
    HF_ZJK_MODBUS_FUNC_AC_ALL          = 0x60, /* 空调：同时控制开关/温度/模式/风速 */
    HF_ZJK_MODBUS_FUNC_AC_ALL_FRAC     = 0x61, /* 空调：同时控制(温度含小数) */
    HF_ZJK_MODBUS_FUNC_AC_QUERY_PERFORMANCE = 0x90, /* 空调：查询品牌与性能 */
} hf_zjk_modbus_func_t;

/* 状态查询/性能查询回复中的"控制值"，同时表示查询类型 */
typedef enum
{
    HF_ZJK_MODBUS_QUERY_SINGLE = 0x01,  /* 指定地址的 1 台设备 */
    HF_ZJK_MODBUS_QUERY_ONLINE = 0x02,  /* 全部设备的在线状态 */
    HF_ZJK_MODBUS_QUERY_FAULT  = 0x04,  /* 全部设备的原厂故障码(ASCII) */
    HF_ZJK_MODBUS_QUERY_MULTI  = 0x0F,  /* 指定地址的多台设备 */
    HF_ZJK_MODBUS_QUERY_ALL    = 0xFF,  /* 全部设备，也用于性能查询 */
} hf_zjk_modbus_query_type_t;

/* 解析后的集控帧（接收方向，即网关回复帧） */
typedef struct
{
    uint8_t  gateway_addr;                     /* 网关 485 地址 */
    uint8_t  function;                         /* 功能码 */
    uint8_t  control;                          /* 控制值（多字节控制值时仅保留首字节） */
    uint8_t  device_count;                     /* 设备数量 */
    uint8_t  overflow;                         /* 1: 数据域超出缓冲上限，data[] 未完整保存 */
    uint16_t data_len;                         /* 数据域实际字节数 */
    uint8_t  data[HF_ZJK_MODBUS_MAX_DATA_LEN]; /* 数据域 */
    uint8_t  checksum;                         /* 收到的校验和 */
} hf_zjk_modbus_frame_t;

/* 环形缓冲区配置 */
#define HF_ZJK_MODBUS_RING_BUFFER_SIZE 256U

/* 环形缓冲区结构体 */
typedef struct
{
    uint8_t  buffer[HF_ZJK_MODBUS_RING_BUFFER_SIZE];
    uint16_t head;   /* 写入位置 */
    uint16_t tail;   /* 读取位置 */
    uint16_t count;  /* 缓冲区中的数据数量 */
} hf_zjk_modbus_ring_buffer_t;

/* 接收状态机状态 */
typedef enum
{
    HF_ZJK_MODBUS_STATE_IDLE = 0,        /* 等待网关地址（帧起始） */
    HF_ZJK_MODBUS_STATE_FUNC,            /* 功能码 */
    HF_ZJK_MODBUS_STATE_CONTROL,         /* 控制值（1 / 2 / 4 / 5 字节） */
    HF_ZJK_MODBUS_STATE_COUNT,           /* 设备数量 */
    HF_ZJK_MODBUS_STATE_PAYLOAD,         /* 数据域（长度已知） */
    HF_ZJK_MODBUS_STATE_PAYLOAD_TO_SUM,  /* 数据域（0x04 故障码，长度未知，靠校验和终结） */
    HF_ZJK_MODBUS_STATE_CHECKSUM,        /* 校验和 */
} hf_zjk_modbus_frame_state_t;

/* 帧解析器结构体 */
typedef struct
{
    hf_zjk_modbus_frame_state_t state;             /* 当前状态 */
    hf_zjk_modbus_frame_t       frame;             /* 当前解析中的帧 */
    uint16_t                    data_index;        /* 数据域已收字节数 */
    uint16_t                    payload_remaining; /* 数据域剩余字节数 */
    uint8_t                     control_remaining; /* 控制值剩余字节数 */
    uint8_t                     control_index;     /* 已收控制值字节数 */
    uint8_t                     calc_sum;          /* 逐字节累积的校验和 */
    uint8_t                     frame_ready;       /* 帧就绪标志 */
} hf_zjk_modbus_parser_t;

/* ---------------- 环形缓冲区接口 ---------------- */
void     hf_zjk_modbus_ring_buffer_init(hf_zjk_modbus_ring_buffer_t *rb);
uint8_t  hf_zjk_modbus_ring_buffer_push(hf_zjk_modbus_ring_buffer_t *rb, uint8_t data);
uint8_t  hf_zjk_modbus_ring_buffer_pop(hf_zjk_modbus_ring_buffer_t *rb, uint8_t *data);
uint16_t hf_zjk_modbus_ring_buffer_get_count(hf_zjk_modbus_ring_buffer_t *rb);
uint8_t  hf_zjk_modbus_ring_buffer_is_empty(hf_zjk_modbus_ring_buffer_t *rb);
uint8_t  hf_zjk_modbus_ring_buffer_is_full(hf_zjk_modbus_ring_buffer_t *rb);

/* ---------------- 校验和接口 ---------------- */
/* 逐字节累加（用于接收状态机流式计算，溢出只取低 8 位） */
uint8_t hf_zjk_modbus_checksum_update(uint8_t sum, uint8_t byte);
/* 一次性计算一段数据的校验和 */
uint8_t hf_zjk_modbus_checksum(const uint8_t *data, uint16_t len);

/* 判断功能码是否为空调控制指令（下行指令的回显帧） */
uint8_t hf_zjk_modbus_is_control_function(uint8_t function);

/* ---------------- 帧解析器接口 ---------------- */
void    hf_zjk_modbus_parser_init(hf_zjk_modbus_parser_t *parser);
void    hf_zjk_modbus_parser_process_byte(hf_zjk_modbus_parser_t *parser,
                                          uint8_t byte,
                                          hf_zjk_modbus_frame_t *out_frame);

/* ---------------- UART 接口 ---------------- */
void    hf_zjk_modbus_uart_init(void);
void    hf_zjk_modbus_uart_process(void);
uint8_t hf_zjk_modbus_uart_send_pdu(const uint8_t *pdu, uint16_t pdu_len);
void    hf_zjk_modbus_uart_rxcallback(uint8_t data);

/* ---------------- 模块使能接口 ----------------
 * 关闭(0)时本模块完全不工作：接收队列不再接收任何字节（已缓存的也清空），
 * 帧解析与所有发送（周期查询 / 控制帧 / 手动查询）一并停止；
 * 重新开启(1)后按上电流程重新计时发帧。 */
void    hf_zjk_modbus_set_enable(uint8_t enable);
/* 读取当前使能状态：1=开启 0=关闭 */
uint8_t hf_zjk_modbus_get_enable(void);

/* 读取当前内机地址 */
uint16_t hf_zjk_modbus_get_current_ac_addr(void);

/* 直接设置当前内机地址（供不需走空调上报回调的调用方使用；0xFFFF 表示未设置） */
void hf_zjk_modbus_set_current_ac_addr(uint16_t address);

/* ---------------- 周期查询调度（MCU -> 网关） ----------------
 * 本机作为集成系统主动轮询：
 *   - 上电 5 秒后开始周期"查询全部"(0x50 + 0xFF)以获取外机地址；
 *   - 收到查询全部回复后，切换到按 1 秒周期"查询单台内机状态"(0x50 + 0x01)，
 *     查询的设备地址 = (外机地址 << 8) | 内机地址（内机地址由平台下发），
 *     内机地址为 0xFFFF(未设置)时跳过本次查询。
 * 控制帧(0x31~0x36)与查询帧共用半双工 485 总线，统一由 hf_zjk_modbus_uart_process()
 * 内的发送调度串行发出：帧间隔 >100ms、控制帧优先；控制帧连发导致查询倒计时被反复
 * 重置时，查询最迟推迟 3 秒仍会发出一次。
 * 平台层 Query*HfCentralControl485 手动查询直接发帧、不经过该仲裁，仅供调试使用。
 */

/* 业务层解析到"查询全部"回复(已拿到外机地址)后调用，切到内机状态轮询阶段
 * @param outer_address 回复里解析出的外机地址 */
void hf_zjk_modbus_query_notify_outer_address_received(uint8_t outer_address);

/* 帧处理回调（业务层实现，见 hf_zjk_modbus_handle.h） */
void hf_zjk_modbus_handle_frame(const hf_zjk_modbus_frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif /* __HF_ZJK_MODBUS_CIRCULAR_BUFFER_H */
