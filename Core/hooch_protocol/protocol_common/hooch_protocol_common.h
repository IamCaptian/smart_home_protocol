#ifndef HOOCH_PROTOCOL_COMMON_H
#define HOOCH_PROTOCOL_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* weak定义：各协议模块检测到数据后调用，通知CPU1当前活跃协议类型 */
extern void hooch_cpu1_set_detected_protocol(uint8_t protocol);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_PROTOCOL_COMMON_H */
