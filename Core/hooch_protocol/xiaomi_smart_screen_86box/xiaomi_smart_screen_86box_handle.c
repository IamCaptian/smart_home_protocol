/*
 * Xiaomi 86 Box — 帧分发与应答层
 *
 * 负责 0x44 设备控制帧的公共解析(设备头/子命令/参数)、应答帧拼装,
 * 以及把各设备子命令交给独立设备模块处理:
 *   xiaomi_86box_device_light.c      灯类
 *   xiaomi_86box_device_switch.c     开关类
 *   xiaomi_86box_device_curtain.c    窗帘
 *   xiaomi_86box_device_air.c        空调
 *   xiaomi_86box_device_freshair.c   新风
 *   xiaomi_86box_device_floor.c      地暖
 *   xiaomi_86box_device_scene.c      情景开关
 *   xiaomi_86box_device_radar.c      人存在雷达
 *   xiaomi_86box_device_ble.c        BLE 基本资料/屏显参数
 */
#include "xiaomi_smart_screen_86box_handle.h"
#include "xiaomi_86box_device_common.h"
#include "xiaomi_86box_device_light.h"
#include "xiaomi_86box_device_switch.h"
#include "xiaomi_86box_device_curtain.h"
#include "xiaomi_86box_device_air.h"
#include "xiaomi_86box_device_freshair.h"
#include "xiaomi_86box_device_floor.h"
#include "xiaomi_86box_device_scene.h"
#include "xiaomi_86box_device_radar.h"
#include "xiaomi_86box_device_ble.h"
#include "hooch_protocol.h"
#include "hooch_protocol_common.h"
#include <string.h>

/* ============================ 应答/上报 ============================ */

uint8_t xiaomi_86box_reply_frame(const XIAOMI_86BOX_Frame_t *request,
                                 uint8_t subcmd,
                                 const uint8_t *params,
                                 uint16_t params_len)
{
    XIAOMI_86BOX_Frame_t reply;
    uint16_t index = 0U;
    uint16_t header_len;

    if (request == NULL)
    {
        return 1U;
    }

    /*
     * 实测模组要求 0x44 应答帧原样回显 4 字节设备头 + 子命令:
     * 缺少设备头时模组会继续重发上报帧, 因此这里保留回显。
     */
    header_len = (request->command == (uint8_t)XIAOMI_86BOX_CMD_DEVICE_CONTROL) ? 4U : 0U;
    if (request->data_len < header_len)
    {
        return 1U;
    }

    if ((header_len + 1U + params_len) > XIAOMI_86BOX_MAX_FRAME_DATA_LEN)
    {
        return 1U;
    }

    reply.header = XIAOMI_86BOX_FRAME_HEADER;
    reply.version = xiaomi_86box_get_version();
    reply.command = request->command;

    for (index = 0U; index < header_len; index++)
    {
        reply.data[index] = request->data[index];
    }
    reply.data[index++] = subcmd;

    if ((params != NULL) && (params_len > 0U))
    {
        memcpy(&reply.data[index], params, params_len);
        index = (uint16_t)(index + params_len);
    }

    reply.data_len = index;
    return xiaomi_86box_uart_send_frame(&reply);
}

void xiaomi_86box_reply_no_param_frame(uint8_t command)
{
    XIAOMI_86BOX_Frame_t reply;

    reply.header = XIAOMI_86BOX_FRAME_HEADER;
    reply.version = xiaomi_86box_get_version();
    reply.command = command;
    reply.data_len = 0U;

    (void)xiaomi_86box_uart_send_frame(&reply);
}

uint8_t xiaomi_86box_send_report(uint8_t device_type,
                                 uint8_t device_id,
                                 uint8_t seq_id,
                                 uint8_t subcmd,
                                 const uint8_t *params,
                                 uint16_t params_len)
{
    XIAOMI_86BOX_Frame_t frame;
    uint16_t index = 0U;

    if ((5U + params_len) > XIAOMI_86BOX_MAX_FRAME_DATA_LEN)
    {
        return 1U;
    }

    frame.header = XIAOMI_86BOX_FRAME_HEADER;
    frame.version = xiaomi_86box_get_version();
    frame.command = (uint8_t)XIAOMI_86BOX_CMD_DEVICE_CONTROL;

    frame.data[index++] = 1U;               /* 上报类型: 1=米家设备 */
    frame.data[index++] = device_type;
    frame.data[index++] = device_id;
    frame.data[index++] = seq_id;
    frame.data[index++] = subcmd;

    if ((params != NULL) && (params_len > 0U))
    {
        memcpy(&frame.data[index], params, params_len);
        index = (uint16_t)(index + params_len);
    }

    frame.data_len = index;
    return xiaomi_86box_uart_send_frame(&frame);
}

/* ============================ 获取所有状态 ============================ */

static void xiaomi_86box_reply_all_state(const XIAOMI_86BOX_Frame_t *request,
                                         const xiaomi_86box_device_header_t *header)
{
    uint8_t params[8];
    uint16_t len = 0U;
    uint8_t handled = 0U;

    if (xiaomi_86box_is_light_type(header->device_type) != 0U)
    {
        handled = xiaomi_86box_device_light_report_all(header, params, &len);
    }
    else if (xiaomi_86box_is_switch_type(header->device_type) != 0U)
    {
        handled = xiaomi_86box_device_switch_report_all(header, params, &len);
    }
    else if (header->device_type == XIAOMI_86BOX_DEV_CURTAIN)
    {
        handled = xiaomi_86box_device_curtain_report_all(header, params, &len);
    }
    else if (header->device_type == XIAOMI_86BOX_DEV_AIR_CONDITIONER)
    {
        handled = xiaomi_86box_device_air_report_all(header, params, &len);
    }
    else if (header->device_type == XIAOMI_86BOX_DEV_FRESH_AIR)
    {
        handled = xiaomi_86box_device_freshair_report_all(header, params, &len);
    }
    else if (header->device_type == XIAOMI_86BOX_DEV_FLOOR_HEATING)
    {
        handled = xiaomi_86box_device_floor_report_all(header, params, &len);
    }

    if (handled != 0U)
    {
        (void)xiaomi_86box_reply_frame(request, (uint8_t)XIAOMI_86BOX_SUBCMD_REPORT_ALL,
                                       params, len);
    }
}

/* ============================ 名称信息 ============================ */

static void xiaomi_86box_handle_name(const XIAOMI_86BOX_Frame_t *request,
                                     const xiaomi_86box_device_header_t *header,
                                     const uint8_t *params,
                                     uint16_t params_len)
{
    HOOCH_PROTOCOL_KeyNameFrame_t key_name;
    const HOOCH_PROTOCOL_KeyNameFrame_t *cached;

    memset(&key_name, 0, sizeof(key_name));

    /* 请求名称: 数据为单字节 0xFF, 直接回填缓存名称 */
    if ((params_len <= 1U) && ((params_len == 0U) || (params[0] == 0xFFU)))
    {
        uint8_t reply[1U + HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH];
        uint16_t name_len;

        cached = HOOCH_PROTOCOL_KeyName_GetFrame();
        reply[0] = (cached != NULL) ? (uint8_t)cached->icon_index : 0U;

        name_len = (cached != NULL) ? (uint16_t)strlen(cached->name) : 0U;
        if (name_len >= HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH)
        {
            name_len = (uint16_t)(HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH - 1U);
        }

        if (name_len > 0U)
        {
            memcpy(&reply[1], cached->name, name_len);
        }

        (void)xiaomi_86box_reply_frame(request, (uint8_t)XIAOMI_86BOX_SUBCMD_REPORT_NAME,
                                       reply, (uint16_t)(1U + name_len));
        return;
    }

    /* 全面屏回填名称: 图标ID + 名称 */
    key_name.key = (HOOCH_PROTOCOL_KeyNameKey_t)header->device_id;
    key_name.icon_index = (HOOCH_PROTOCOL_KeyNameIconIndex_t)params[0];
    key_name.page = xiaomi_86box_name_page_from_device_type(header->device_type);
    key_name.delivery = (header->device_id == 0U)
        ? HOOCH_PROTOCOL_KEY_NAME_DELIVERY_OFFSCREEN_TEXT
        : HOOCH_PROTOCOL_KEY_NAME_DELIVERY_KEY;

    if (params_len > 1U)
    {
        uint16_t copy_len = (uint16_t)(params_len - 1U);
        if (copy_len > (HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH - 1U))
        {
            copy_len = (uint16_t)(HOOCH_PROTOCOL_KEY_NAME_MAX_LENGTH - 1U);
        }
        memcpy(key_name.name, &params[1], copy_len);
        key_name.name[copy_len] = '\0';
    }

    if (strcmp(key_name.name, "undefined") != 0)
    {
        (void)HOOCH_PROTOCOL_KeyName_SetFrame(&key_name);
    }
}

/* ============================ 0x44 设备控制 ============================ */

static void xiaomi_86box_handle_device_control(XIAOMI_86BOX_Frame_t *frame)
{
    xiaomi_86box_device_header_t header;
    uint8_t subcmd;
    const uint8_t *params;
    uint16_t params_len;

    /* 应答帧可能只有 1 字节子命令(不带设备头), 收到即视为对方 ACK */
    if (frame->data_len == 1U)
    {
        XIAOMI_86BOX_LOG_PRINTF("ACK cmd=0x44 sub=0x%02X\r\n", (unsigned int)frame->data[0]);
        return;
    }

    if (frame->data_len < 5U)
    {
        XIAOMI_86BOX_LOG_WARN("[86BOX] 0x44 data too short: %u\r\n",
                              (unsigned int)frame->data_len);
        return;
    }

    header.report_type = frame->data[0];
    header.device_type = frame->data[1];
    header.device_id = frame->data[2];
    header.seq_id = frame->data[3];
    subcmd = frame->data[4];
    params = &frame->data[5];
    params_len = (uint16_t)(frame->data_len - 5U);

    XIAOMI_86BOX_LOG_INFO("[86BOX] CTL type=0x%02X id=%u sub=0x%02X len=%u\r\n",
                          (unsigned int)header.device_type,
                          (unsigned int)header.device_id,
                          (unsigned int)subcmd,
                          (unsigned int)params_len);

    if (subcmd == (uint8_t)XIAOMI_86BOX_SUBCMD_GET_ALL)
    {
        xiaomi_86box_reply_all_state(frame, &header);
        return;
    }

    if (subcmd == (uint8_t)XIAOMI_86BOX_SUBCMD_REQUEST_NAME)
    {
        xiaomi_86box_handle_name(frame, &header, params, params_len);
        return;
    }

    /*
     * 0x44 的应答帧只有 4 字节设备头 + 子命令(data_len=5) 且不带参数,
     * 收到即视为对方对本机控制/上报的 ACK, 只记录, 不再回复, 避免回环。
     */
    if (frame->data_len == 5U)
    {
        XIAOMI_86BOX_LOG_PRINTF("ACK cmd=0x44 sub=0x%02X\r\n", (unsigned int)subcmd);
        return;
    }

    if (xiaomi_86box_is_light_type(header.device_type) != 0U)
    {
        (void)xiaomi_86box_device_light_handle(&header, subcmd, params, params_len);
    }
    else if (xiaomi_86box_is_switch_type(header.device_type) != 0U)
    {
        (void)xiaomi_86box_device_switch_handle(&header, subcmd, params, params_len);
    }
    else if (header.device_type == XIAOMI_86BOX_DEV_CURTAIN)
    {
        (void)xiaomi_86box_device_curtain_handle(&header, subcmd, params, params_len);
    }
    else if (header.device_type == XIAOMI_86BOX_DEV_AIR_CONDITIONER)
    {
        (void)xiaomi_86box_device_air_handle(&header, subcmd, params, params_len);
    }
    else if (header.device_type == XIAOMI_86BOX_DEV_FRESH_AIR)
    {
        (void)xiaomi_86box_device_freshair_handle(&header, subcmd, params, params_len);
    }
    else if (header.device_type == XIAOMI_86BOX_DEV_FLOOR_HEATING)
    {
        (void)xiaomi_86box_device_floor_handle(&header, subcmd, params, params_len);
    }
    else if (header.device_type == XIAOMI_86BOX_DEV_SCENE_SWITCH)
    {
        (void)xiaomi_86box_device_scene_handle(&header, subcmd, params, params_len);
    }
    else if (header.device_type == XIAOMI_86BOX_DEV_86PIR)
    {
        (void)xiaomi_86box_device_radar_handle(&header, subcmd, params, params_len);
    }
    else if (header.device_type == XIAOMI_86BOX_DEV_BLE_BASIC)
    {
        (void)xiaomi_86box_device_ble_handle(&header, subcmd, params, params_len);
    }
    else
    {
        XIAOMI_86BOX_LOG_WARN("[86BOX] unsupported device type: 0x%02X\r\n",
                              (unsigned int)header.device_type);
    }

    /* 带参数的 0x44 帧(控制/上报)对方都需要应答, 统一回空帧 */
    (void)xiaomi_86box_reply_frame(frame, subcmd, NULL, 0U);
}

/* ============================ 主命令分发 ============================ */

void xiaomi_86box_handle_frame(XIAOMI_86BOX_Frame_t *frame)
{
    if (frame == NULL)
    {
        return;
    }

    switch (frame->command)
    {
    case XIAOMI_86BOX_CMD_DEVICE_CONTROL:
        xiaomi_86box_handle_device_control(frame);
        break;

    case XIAOMI_86BOX_CMD_PAIRING_CONTROL:
        /* 带数据为请求, 需回空帧; 空帧是本机请求的应答, 不再回复 */
        if (frame->data_len >= 1U)
        {
            XIAOMI_86BOX_LOG_PRINTF("[86BOX] Pairing control: %u\r\n",
                                    (unsigned int)frame->data[0]);
            xiaomi_86box_reply_no_param_frame(frame->command);
        }
        else
        {
            XIAOMI_86BOX_LOG_PRINTF("ACK cmd=0x%02X\r\n", (unsigned int)frame->command);
        }
        break;

    case XIAOMI_86BOX_CMD_PAIRING_STATUS:
        if (frame->data_len >= 1U)
        {
            HOOCH_PROTOCOL_SettingNetworkStatus_t status;

            switch (frame->data[0])
            {
            case 0U: status = HOOCH_PROTOCOL_SETTING_NETWORK_STATUS_OFFLINE; break;
            case 1U: status = HOOCH_PROTOCOL_SETTING_NETWORK_STATUS_JOINED;  break;
            case 2U: status = HOOCH_PROTOCOL_SETTING_NETWORK_STATUS_JOINING; break;
            case 3U: status = HOOCH_PROTOCOL_SETTING_NETWORK_STATUS_OFFLINE; break;
            default: status = HOOCH_PROTOCOL_SETTING_NETWORK_STATUS_ERROR;   break;
            }

            xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_NETWORK_STATUS,
                                          (uint32_t)status,
                                          HOOCH_PROTOCOL_SETTING_PAGE_INVALID);

            /* 带数据的 0x82 是模组上报, 需要应答空帧; 空帧本身是应答, 不再回复 */
            xiaomi_86box_reply_no_param_frame(frame->command);
        }
        else
        {
            XIAOMI_86BOX_LOG_PRINTF("ACK cmd=0x%02X\r\n", (unsigned int)frame->command);
        }
        break;

    case XIAOMI_86BOX_CMD_RESET_CONTROL:
        /* 带数据为请求, 需回空帧; 空帧是本机请求的应答, 不再回复 */
        if (frame->data_len >= 1U)
        {
            xiaomi_86box_dispatch_setting(HOOCH_PROTOCOL_SETTING_ITEM_FACTORY_RESET,
                                          1U, HOOCH_PROTOCOL_SETTING_PAGE_INVALID);
            xiaomi_86box_reply_no_param_frame(frame->command);
        }
        else
        {
            XIAOMI_86BOX_LOG_PRINTF("ACK cmd=0x%02X\r\n", (unsigned int)frame->command);
        }
        break;

    case XIAOMI_86BOX_CMD_PRODUCTION_TEST:
        /* 带数据为请求, 需回空帧; 空帧是本机请求的应答, 不再回复 */
        if (frame->data_len >= 1U)
        {
            XIAOMI_86BOX_LOG_PRINTF("[86BOX] Production test: %u\r\n",
                                    (unsigned int)frame->data[0]);
            xiaomi_86box_reply_no_param_frame(frame->command);
        }
        else
        {
            XIAOMI_86BOX_LOG_PRINTF("ACK cmd=0x%02X\r\n", (unsigned int)frame->command);
        }
        break;

    case XIAOMI_86BOX_CMD_PRODUCTION_STATUS:
    case XIAOMI_86BOX_CMD_RESET_STATUS:
        /* 带数据为模组上报, 需应答空帧; 空帧是全面屏对本机上报的应答, 直接忽略 */
        if (frame->data_len >= 1U)
        {
            XIAOMI_86BOX_LOG_INFO("[86BOX] status report cmd=0x%02X value=%u\r\n",
                                  (unsigned int)frame->command,
                                  (unsigned int)frame->data[0]);
            xiaomi_86box_reply_no_param_frame(frame->command);
        }
        else
        {
            XIAOMI_86BOX_LOG_PRINTF("ACK cmd=0x%02X\r\n", (unsigned int)frame->command);
        }
        break;

    default:
        XIAOMI_86BOX_LOG_PRINTF("[86BOX] unknown command: 0x%02X len=%u\r\n",
                                (unsigned int)frame->command,
                                (unsigned int)frame->data_len);
        break;
    }
}
