#ifndef __HF_ZXK_MODBUS_HANDLE_H
#define __HF_ZXK_MODBUS_HANDLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hf_zxk_modbus_circular_buffer.h"
#include "hooch_protocol.h"

/*
 * 线控器 MODBUS-RTU 协议 —— 业务层（下发帧解析与平台同步）
 *
 * 本机是 MODBUS 主机(轮询方)，因此这里处理的"下发帧"即空调网关的回复帧：
 *   0x03 读保持寄存器回复 —— 解析为内机状态 / 性能参数
 *   0x06、0x10 写寄存器回显 —— 仅记录写入被接收，实际结果需靠 0x03 查询确认
 *   异常帧（功能码 | 0x80）  —— 打印异常码
 * 发送侧（0x03 周期查询、0x06/0x10 控制）暂未实现。
 */

/* ---------------- 保持寄存器地址 ---------------- */
typedef enum
{
    HF_ZXK_MODBUS_REG_UNIT_STATUS       = 0x0000,  /* 单台内机状态，起始寄存器 */
    HF_ZXK_MODBUS_REG_PERFORMANCE       = 0x07D0,  /* 性能查询，起始寄存器，共 3 个 */
    HF_ZXK_MODBUS_REG_CONTROL_POWER     = 0x0FA0,  /* 开关 */
    HF_ZXK_MODBUS_REG_CONTROL_MODE      = 0x0FA1,  /* 模式 */
    HF_ZXK_MODBUS_REG_CONTROL_TEMP      = 0x0FA2,  /* 设定温度 */
    HF_ZXK_MODBUS_REG_CONTROL_FAN_SPEED = 0x0FA3,  /* 风速 */
    HF_ZXK_MODBUS_REG_CONTROL_WIND      = 0x0FA4,  /* 风向 */
} hf_zxk_modbus_register_t;

/* 单台内机状态寄存器偏移（相对 HF_ZXK_MODBUS_REG_UNIT_STATUS） */
typedef enum
{
    HF_ZXK_MODBUS_UNIT_REG_POWER        = 0,  /* 开关: 0=关 1=开 */
    HF_ZXK_MODBUS_UNIT_REG_MODE         = 1,  /* 模式: 位定义，见 hf_zxk_modbus_mode_t */
    HF_ZXK_MODBUS_UNIT_REG_SET_TEMP     = 2,  /* 设定温度: 十进制 16~32 */
    HF_ZXK_MODBUS_UNIT_REG_FAN_SPEED    = 3,  /* 风速: 0=自动 1=低风 2=中风 3=高风 */
    HF_ZXK_MODBUS_UNIT_REG_WIND         = 4,  /* 风向: 0=无风向 */
    HF_ZXK_MODBUS_UNIT_REG_FAULT        = 5,  /* 故障码: 0=无故障 */
    HF_ZXK_MODBUS_UNIT_REG_ROOM_TEMP    = 6,  /* 室温: 十进制 (查询 8 个寄存器及以上有效) */
    HF_ZXK_MODBUS_UNIT_REG_PERMISSION   = 7,  /* 权限位: bit4=有切换权 (8 个及以上) */
    HF_ZXK_MODBUS_UNIT_REG_MODE_ALLOWED = 8,  /* 允许的模式位图 (查询 9 个寄存器时有效) */
    HF_ZXK_MODBUS_UNIT_REG_MAX          = 9,  /* 哨兵值 */
} hf_zxk_modbus_unit_register_t;

/* 性能查询寄存器偏移（相对 HF_ZXK_MODBUS_REG_PERFORMANCE） */
typedef enum
{
    HF_ZXK_MODBUS_PERFORMANCE_REG_UNKNOWN_0 = 0,  /* 协议文档未说明 */
    HF_ZXK_MODBUS_PERFORMANCE_REG_UNKNOWN_1 = 1,  /* 协议文档未说明 */
    HF_ZXK_MODBUS_PERFORMANCE_REG_WIND      = 2,  /* 0=无风向 */
    HF_ZXK_MODBUS_PERFORMANCE_REG_MAX       = 3,  /* 哨兵值 */
} hf_zxk_modbus_performance_register_t;

/* 开关 */
typedef enum
{
    HF_ZXK_MODBUS_POWER_OFF = 0,  /* 关闭 */
    HF_ZXK_MODBUS_POWER_ON  = 1,  /* 开启 */
} hf_zxk_modbus_power_t;

/* 模式：协议按位定义，控制寄存器 0x0FA1 的写入值与状态寄存器取值一致 */
typedef enum
{
    HF_ZXK_MODBUS_MODE_INVALID = 0x00,  /* 未识别 */
    HF_ZXK_MODBUS_MODE_HEAT    = 0x01,  /* 制热 */
    HF_ZXK_MODBUS_MODE_COOL    = 0x02,  /* 制冷 */
    HF_ZXK_MODBUS_MODE_FAN     = 0x04,  /* 送风 */
    HF_ZXK_MODBUS_MODE_DRY     = 0x08,  /* 除湿 */
} hf_zxk_modbus_mode_t;

/* 风速 */
typedef enum
{
    HF_ZXK_MODBUS_FAN_SPEED_AUTO    = 0x00,  /* 自动 */
    HF_ZXK_MODBUS_FAN_SPEED_LOW     = 0x01,  /* 低风 */
    HF_ZXK_MODBUS_FAN_SPEED_MEDIUM  = 0x02,  /* 中风 */
    HF_ZXK_MODBUS_FAN_SPEED_HIGH    = 0x03,  /* 高风 */
    HF_ZXK_MODBUS_FAN_SPEED_INVALID = 0xFF,  /* 非法值 */
} hf_zxk_modbus_fan_speed_t;

/* 风向 */
typedef enum
{
    HF_ZXK_MODBUS_WIND_NONE    = 0x00,  /* 无风向(无导风板) */
    HF_ZXK_MODBUS_WIND_SMALL   = 0x01,  /* 开启小角度 */
    HF_ZXK_MODBUS_WIND_MEDIUM  = 0x02,  /* 开启中角度 / 45 度 */
    HF_ZXK_MODBUS_WIND_LARGE   = 0x04,  /* 开启大角度 */
    HF_ZXK_MODBUS_WIND_SWING   = 0x07,  /* 摆动 */
    HF_ZXK_MODBUS_WIND_INVALID = 0xFF,  /* 非法值 */
} hf_zxk_modbus_wind_direction_t;

/* 权限寄存器(offset 7) 位定义 */
#define HF_ZXK_MODBUS_PERMISSION_SWITCH_BIT 0x0010U  /* bit4: 有切换权 */

/* 单台内机状态（由 0x03 回复解析得到） */
typedef struct
{
    uint8_t                        address;          /* 485 从机地址 */
    uint8_t                        reg_count;        /* 本次回复包含的寄存器数量(6/8/9) */
    hf_zxk_modbus_power_t          power;            /* 开关 */
    hf_zxk_modbus_mode_t           mode;             /* 模式 */
    uint16_t                       set_temperature;  /* 设定温度 */
    hf_zxk_modbus_fan_speed_t      fan_speed;        /* 风速 */
    hf_zxk_modbus_wind_direction_t wind_direction;   /* 风向 */
    uint16_t                       fault_code;       /* 故障码，0=无故障 */
    uint16_t                       room_temperature; /* 室温，仅 8 个寄存器及以上有效 */
    uint16_t                       permission;       /* 权限位，仅 8 个寄存器及以上有效 */
    uint16_t                       mode_allowed;     /* 允许的模式位图，仅 9 个寄存器时有效 */
} hf_zxk_modbus_unit_status_t;

/* ---------------- 接口 ---------------- */

/* 处理一帧解析完成的 MODBUS 回复帧 */
void hf_zxk_modbus_handle_frame(const hf_zxk_modbus_frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif /* __HF_ZXK_MODBUS_HANDLE_H */
