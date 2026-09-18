#ifndef __HF_ZJK_MODBUS_HANDLE_H
#define __HF_ZJK_MODBUS_HANDLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hf_zjk_modbus_circular_buffer.h"
#include "hooch_protocol.h"

/*
 * 集控器 自定义 485 协议 —— 业务层（空调回复帧解析与平台同步）
 *
 * 本机是集成系统(轮询方)，协议规定网关只响应不主动发送，因此这里处理的都是网关回复帧：
 *   0x50 / 0x53 查询状态回复 —— 解析为空调状态（单台/多台/全部/在线状态/故障码）
 *   0x90        查询性能回复 —— 解析品牌、支持的模式/风速/温度范围
 *   0x31~0x61   控制指令回显   —— 仅记录网关已接收，实际结果需靠 0x50 查询确认
 * 控制指令与状态查询的发送侧（0x31~0x61 / 0x50）暂未实现。
 *
 * 当前只覆盖空调设备；新风(0x51 / 0x71~0x74)、地暖(0x52 / 0x81~0x84)未实现。
 */

/* 空调状态数据块字段偏移（0x50 / 0x53 回复，每台设备 10 或 12 字节） */
typedef enum
{
    HF_ZJK_MODBUS_DEV_OFFSET_OUTER     = 0,  /* 室外机(制冷系统)地址 */
    HF_ZJK_MODBUS_DEV_OFFSET_INNER     = 1,  /* 室内机地址 */
    HF_ZJK_MODBUS_DEV_OFFSET_POWER     = 2,  /* 开关: 0=关 1=开 */
    HF_ZJK_MODBUS_DEV_OFFSET_SET_TEMP  = 3,  /* 设定温度整数部分 */
    HF_ZJK_MODBUS_DEV_OFFSET_MODE      = 4,  /* 模式设定 */
    HF_ZJK_MODBUS_DEV_OFFSET_FAN_SPEED = 5,  /* 风速设定 */
    HF_ZJK_MODBUS_DEV_OFFSET_ROOM_TEMP = 6,  /* 房间温度 */
    HF_ZJK_MODBUS_DEV_OFFSET_FAULT     = 7,  /* 故障代码(1 字节，非原厂 ASCII 故障码) */
    HF_ZJK_MODBUS_DEV_OFFSET_WIND      = 8,  /* 风向设定 */
    HF_ZJK_MODBUS_DEV_OFFSET_OTHER     = 9,  /* 其他信息: bit0 1=主机 0=从机 */
} hf_zjk_modbus_device_offset_t;

/* 0x53 回复的数据块偏移：设定温度与房间温度各占 2 字节，其后字段整体后移 2 */
typedef enum
{
    HF_ZJK_MODBUS_DEV_DEC_OFFSET_SET_TEMP_INT   = 3,  /* 设定温度整数部分 */
    HF_ZJK_MODBUS_DEV_DEC_OFFSET_SET_TEMP_FRAC  = 4,  /* 设定温度小数部分: 0x00 / 0x05 */
    HF_ZJK_MODBUS_DEV_DEC_OFFSET_MODE           = 5,
    HF_ZJK_MODBUS_DEV_DEC_OFFSET_FAN_SPEED      = 6,
    HF_ZJK_MODBUS_DEV_DEC_OFFSET_ROOM_TEMP_INT  = 7,  /* 房间温度整数部分 */
    HF_ZJK_MODBUS_DEV_DEC_OFFSET_ROOM_TEMP_FRAC = 8,  /* 房间温度小数部分: 0x00 / 0x05 */
    HF_ZJK_MODBUS_DEV_DEC_OFFSET_FAULT          = 9,
    HF_ZJK_MODBUS_DEV_DEC_OFFSET_WIND           = 10,
    HF_ZJK_MODBUS_DEV_DEC_OFFSET_OTHER          = 11,
} hf_zjk_modbus_device_decimal_offset_t;

/* 温度小数部分取值（0x53） */
typedef enum
{
    HF_ZJK_MODBUS_TEMP_FRAC_NONE = 0x00,  /* .0 */
    HF_ZJK_MODBUS_TEMP_FRAC_HALF = 0x05,  /* .5 */
} hf_zjk_modbus_temp_fraction_t;

/* 开关 */
typedef enum
{
    HF_ZJK_MODBUS_POWER_OFF = 0x00,  /* 关机 */
    HF_ZJK_MODBUS_POWER_ON  = 0x01,  /* 开机 */
} hf_zjk_modbus_power_t;

/* 模式：协议 0x33 的控制值与 0x50 状态值一致，均为按位/单值定义 */
typedef enum
{
    HF_ZJK_MODBUS_MODE_AUTO       = 0x00,  /* 自动 */
    HF_ZJK_MODBUS_MODE_COOL       = 0x01,  /* 制冷 */
    HF_ZJK_MODBUS_MODE_DRY        = 0x02,  /* 除湿 */
    HF_ZJK_MODBUS_MODE_FAN        = 0x04,  /* 送风 */
    HF_ZJK_MODBUS_MODE_HEAT       = 0x08,  /* 制热 */
    HF_ZJK_MODBUS_MODE_FLOOR_HEAT = 0x09,  /* 地暖 */
    HF_ZJK_MODBUS_MODE_HEAT_BOOST = 0x0A,  /* 强热(地暖+制热) */
    HF_ZJK_MODBUS_MODE_REFRESH    = 0x0C,  /* 清爽 */
    HF_ZJK_MODBUS_MODE_AUTO_DRY   = 0x0D,  /* 自动除湿 */
    HF_ZJK_MODBUS_MODE_SLEEP      = 0x0E,  /* 贴心睡眠 */
    HF_ZJK_MODBUS_MODE_UNKNOWN    = 0xFF,  /* 非法/未识别 */
} hf_zjk_modbus_mode_t;

/* 风速 */
typedef enum
{
    HF_ZJK_MODBUS_FAN_AUTO        = 0x00,  /* 自动 */
    HF_ZJK_MODBUS_FAN_HIGH        = 0x01,  /* 高速 */
    HF_ZJK_MODBUS_FAN_MEDIUM      = 0x02,  /* 中速 */
    HF_ZJK_MODBUS_FAN_MEDIUM_HIGH = 0x03,  /* 中高速 */
    HF_ZJK_MODBUS_FAN_LOW         = 0x04,  /* 低速 */
    HF_ZJK_MODBUS_FAN_MEDIUM_LOW  = 0x05,  /* 中低速 */
    HF_ZJK_MODBUS_FAN_ULTRA_LOW   = 0x06,  /* 极低速 */
    HF_ZJK_MODBUS_FAN_ULTRA_HIGH  = 0x07,  /* 超高速 */
    HF_ZJK_MODBUS_FAN_UNKNOWN     = 0xFF,  /* 非法/未识别 */
} hf_zjk_modbus_fan_speed_t;

/* 其他信息寄存器位定义 */
#define HF_ZJK_MODBUS_OTHER_HOST_BIT 0x01U  /* bit0: 1=主机 0=从机 */

/* 单台空调状态（由 0x50 / 0x53 回复解析得到） */
typedef struct
{
    uint8_t                  outer_address;      /* 室外机(制冷系统)地址 */
    uint8_t                  inner_address;      /* 室内机地址 */
    hf_zjk_modbus_power_t    power;              /* 开关 */
    hf_zjk_modbus_mode_t     mode;               /* 模式 */
    uint8_t                  set_temperature;    /* 设定温度整数部分 */
    uint8_t                  set_temperature_frac; /* 设定温度小数部分(0x00/0x05)，仅 0x53 有效 */
    hf_zjk_modbus_fan_speed_t fan_speed;         /* 风速 */
    uint8_t                  room_temperature;   /* 房间温度整数部分 */
    uint8_t                  room_temperature_frac; /* 房间温度小数部分(0x00/0x05)，仅 0x53 有效 */
    uint8_t                  fault_code;         /* 故障代码，0=无故障 */
    uint8_t                  wind_direction;     /* 风向设定 */
    uint8_t                  other_info;         /* 其他信息(bit0: 主/从机) */
} hf_zjk_modbus_ac_status_t;

/* ---------------- 接口 ---------------- */

/* 处理一帧解析完成的集控回复帧 */
void hf_zjk_modbus_handle_frame(const hf_zjk_modbus_frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif /* __HF_ZJK_MODBUS_HANDLE_H */
