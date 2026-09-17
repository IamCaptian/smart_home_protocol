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
    HOOCH_PROTOCOL_SOURCE_RS485,               /* 485 */
    HOOCH_PROTOCOL_SOURCE_CENTRAL_CONTROL,     /* 集控 */
    HOOCH_PROTOCOL_SOURCE_WIRED_CONTROLLER,    /* 线控 */
    HOOCH_PROTOCOL_SOURCE_MAX,                 /* 哨兵值，新增枚举请插入到上方 */
} HOOCH_PROTOCOL_Source_t;

/* weak定义：各协议模块检测到数据后调用，通知CPU1当前活跃协议类型 */
extern void hooch_cpu1_set_detected_protocol(uint8_t protocol);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_PROTOCOL_COMMON_H */ 
