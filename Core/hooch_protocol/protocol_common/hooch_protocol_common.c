#include "hooch_protocol_common.h"

__attribute__((weak)) void hooch_cpu1_set_detected_protocol(uint8_t protocol)
{
    (void)protocol;
}

/* 集控模块关闭时的弱定义：真正的实现在 hf_zjk_modbus_circular_buffer.c，
 * 这里提供空实现，避免关闭集控模块后调用处链接报错 */
__attribute__((weak)) void hf_zjk_modbus_set_current_ac_addr(uint16_t address)
{
    (void)address;
}


