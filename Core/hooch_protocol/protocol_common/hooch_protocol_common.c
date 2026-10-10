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

// 百分比转化为一个字节最大255
uint8_t hooch_PercentageToChars(uint8_t per) {
    float data = (float)per*255.0/100.0 + 0.5;
    return (uint8_t)data;
}


// 字节转化为百分比
uint8_t hooch_CharsToPercentage(uint8_t per) {
    float data = (float)per*100.0/255.0 + 0.5;
    return (uint8_t)data;
}



void hooch_floatToChars(float value, uint8_t *chars) {
    // 将 float 转换为 char 数组
    for (int i = 0; i < 4; i++) {
        chars[i] = *((uint8_t *)&value + i);
    }
}

// 小端将 char 数组转换为 float
float hooch_charsToFloat(const uint8_t *chars) {
    // 将 char 数组转换为 float
    float value;
    for (int i = 0; i < 4; i++) {
        *((uint8_t *)&value + i) = chars[i];
    }
    return value;
}

