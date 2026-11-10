#ifndef HOOCH_UNSUPPORTED_ATTRIBUTE_H
#define HOOCH_UNSUPPORTED_ATTRIBUTE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "hooch_setting.h"

/* 设备不支持属性编号
 * 成员编号 = bit 位 + 1（NONE 占用 0），上报时按 1U << (编号 - 1U) 折算成位图，
 * 具体如何编码成帧由各通道回调自行决定。
 *   bit0 -> 不支持空调
 *   bit1 -> 不支持新风
 *   bit2 -> 不支持地暖
 *   bit3 -> 不支持温度
 *   bit4 -> 不支持湿度
 *   bit5 -> 不支持人感灵敏度
 *   bit6 -> 不支持震动
 *   bit7 -> 不支持换壁纸
 *   bit8 -> 不支持 5-8 路继电器
 *   bit9 -> 不支持氛围灯
 */
typedef enum
{
    HOOCH_PROTOCOL_UNSUPPORTED_ATTRIBUTE_NONE = 0U,                  /* 无效属性 / 列表结束标志 */
    HOOCH_PROTOCOL_UNSUPPORTED_ATTRIBUTE_AIR_CONDITIONER = 1U,       /* bit0 空调 */
    HOOCH_PROTOCOL_UNSUPPORTED_ATTRIBUTE_FRESH_AIR,                  /* bit1 新风 */
    HOOCH_PROTOCOL_UNSUPPORTED_ATTRIBUTE_FLOOR_HEATING,              /* bit2 地暖 */
    HOOCH_PROTOCOL_UNSUPPORTED_ATTRIBUTE_TEMPERATURE,                /* bit3 温度 */
    HOOCH_PROTOCOL_UNSUPPORTED_ATTRIBUTE_HUMIDITY,                   /* bit4 湿度 */
    HOOCH_PROTOCOL_UNSUPPORTED_ATTRIBUTE_HUMAN_PRESENCE_SENSITIVITY, /* bit5 人感灵敏度 */
    HOOCH_PROTOCOL_UNSUPPORTED_ATTRIBUTE_VIBRATION,                  /* bit6 震动 */
    HOOCH_PROTOCOL_UNSUPPORTED_ATTRIBUTE_WALLPAPER,                  /* bit7 换壁纸 */
    HOOCH_PROTOCOL_UNSUPPORTED_ATTRIBUTE_RELAY_CH5_TO_CH8,           /* bit8 5-8 路继电器 */
    HOOCH_PROTOCOL_UNSUPPORTED_ATTRIBUTE_AMBIENCE_LIGHT,             /* bit9 氛围灯 */
    HOOCH_PROTOCOL_UNSUPPORTED_ATTRIBUTE_COUNT                       /* 属性编号上限（非有效属性） */
} HOOCH_PROTOCOL_UnsupportedAttribute_t;

/* 不支持属性上报回调函数类型
 * attributes : 本次上报的属性列表（按传入顺序）
 * count      : 列表元素个数
 * 注: attributes 指向静态缓存，仅在回调执行期间有效，回调返回后不要再持有。
 */
typedef void (*HOOCH_PROTOCOL_UnsupportedAttributeReportCallback_t)(
    const HOOCH_PROTOCOL_UnsupportedAttribute_t *attributes,
    uint8_t count);

/* 单次上报属性列表的最大长度（覆盖全部 10 个有效属性） */
#define HOOCH_PROTOCOL_UNSUPPORTED_REPORT_LIST_MAX        10U

/* 设置不支持属性列表并上报，参数个数可变，必须以 HOOCH_PROTOCOL_UNSUPPORTED_ATTRIBUTE_NONE 结尾 */
HOOCH_PROTOCOL_SettingResult_t HOOCH_PROTOCOL_UnsupportedAttribute_SendAttributes(
    HOOCH_PROTOCOL_UnsupportedAttribute_t first,
    ...);

/* 注册不支持属性上报回调（只保留一个，后注册覆盖先注册） */
void HOOCH_PROTOCOL_UnsupportedAttribute_RegisterReportCallback(
    HOOCH_PROTOCOL_UnsupportedAttributeReportCallback_t callback);

/* 注销不支持属性上报回调 */
void HOOCH_PROTOCOL_UnsupportedAttribute_UnregisterReportCallback(void);

/* 重发当前不支持属性列表（不改变列表内容） */
HOOCH_PROTOCOL_SettingResult_t HOOCH_PROTOCOL_UnsupportedAttribute_Report(void);

#ifdef __cplusplus
}
#endif

#endif /* HOOCH_UNSUPPORTED_ATTRIBUTE_H */
