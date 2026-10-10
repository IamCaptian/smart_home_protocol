#ifndef HOOCH_PROTOCOL_COMMON_H
#define HOOCH_PROTOCOL_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 消息来源：标注一条控制/上报消息是从哪个入口进来的 */
typedef enum
{
    HOOCH_PROTOCOL_SOURCE_INVALID = 0U,        /* 未指定 */
    HOOCH_PROTOCOL_SOURCE_BLUETOOTH,           /* 蓝牙 */
    HOOCH_PROTOCOL_SOURCE_TUYA,                /* 涂鸦 */
    HOOCH_PROTOCOL_SOURCE_XIAOMI,              /* 小米 */
    HOOCH_PROTOCOL_SOURCE_KNX,                 /* KNX */
    HOOCH_PROTOCOL_SOURCE_RS485,                   /* 485 */
    HOOCH_PROTOCOL_SOURCE_HF_WIRED_CONTROLLER,     /* HF线控 */
    HOOCH_PROTOCOL_SOURCE_HF_CENTRAL_CONTROL,      /* HF集控 */
    HOOCH_PROTOCOL_SOURCE_HF_WIRED_CONTROLLER_485, /* HF线控485 */
    HOOCH_PROTOCOL_SOURCE_HF_CENTRAL_CONTROL_485,  /* HF集控485 */
    HOOCH_PROTOCOL_SOURCE_MAX,                     /* 哨兵值，新增枚举请插入到上方 */
} HOOCH_PROTOCOL_Source_t;

/* weak定义：各协议模块检测到数据后调用，通知CPU1当前活跃协议类型 */
extern void hooch_cpu1_set_detected_protocol(uint8_t protocol);

/* weak定义：直接设置集控内机地址(设备地址 = 外机地址 << 8 | 内机地址)。
 * 真正实现在 hf_zjk_modbus_circular_buffer.c，集控模块关闭时由弱定义兜底，
 * 调用方无需依赖集控模块头文件。 */
extern void hf_zjk_modbus_set_current_ac_addr(uint16_t address);

/* ---------------- 通用数据转换接口 ---------------- */

/* 百分比(0~100) 转 单字节(0~255) */
extern uint8_t hooch_PercentageToChars(uint8_t per);

/* 单字节(0~255) 转 百分比(0~100) */
extern uint8_t hooch_CharsToPercentage(uint8_t per);

/* float 转 4 字节小端数组 */
extern void hooch_floatToChars(float value, uint8_t *chars);

/* 4 字节小端数组 转 float */
extern float hooch_charsToFloat(const uint8_t *chars);


/* ---------------- 协议编译开关 ----------------
 * 置 0 时对应协议模块被整段条件编译掉，只保留 init / process / send / rxcallback
 * 四个空实现，因此调用方(main.c、stm32f1xx_it.c)无需屏蔽任何代码，
 * 也不会有收发行为，更不会占用 Flash 与 RAM。 */
#ifndef HOOCH_PROTOCOL_ENABLE_HF_WIRED_CONTROLLER_485
#define HOOCH_PROTOCOL_ENABLE_HF_WIRED_CONTROLLER_485 0  /* HF线控器 MODBUS-RTU 协议 */
#endif

#ifndef HOOCH_PROTOCOL_ENABLE_HF_CENTRAL_CONTROL_485
#define HOOCH_PROTOCOL_ENABLE_HF_CENTRAL_CONTROL_485 1   /* HF集控器 自定义485 协议 */
#endif

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_PROTOCOL_COMMON_H */ 
