/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "user_printf.h"
#include "xiaomi_smart_screen_circular_bufferc.h"
#include "knx_protocol_uart.h"
#include "hooch_protocol.h"
#include "mcu_api.h"

/* CLI驱动外部函数声明 */
extern void Hooch_CLI_Init(void);
extern void Hooch_CLI_Process(void);
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
void HOOCH_PROTOCOL_KeyStatusReport_Process(const HOOCH_PROTOCOL_KeyStatusDispatchFrame_t *frame);
void HOOCH_PROTOCOL_Setting_Process(const HOOCH_PROTOCOL_SettingFrame_t *frame);
void HOOCH_PROTOCOL_KeyName_Process(const HOOCH_PROTOCOL_KeyNameFrame_t *frame);
void HOOCH_PROTOCOL_KeyMode_Process(const HOOCH_PROTOCOL_KeyModeFrame_t *frame);
void HOOCH_PROTOCOL_AirConditioner_Process(const HOOCH_PROTOCOL_AirConditionerFrame_t *frame);
void HOOCH_PROTOCOL_FloorHeating_Process(const HOOCH_PROTOCOL_FloorHeatingFrame_t *frame);
void HOOCH_PROTOCOL_FreshAir_Process(const HOOCH_PROTOCOL_FreshAirFrame_t *frame);
void HOOCH_PROTOCOL_DimmerLight_Process(const HOOCH_PROTOCOL_DimmerLightFrame_t *frame);
void HOOCH_PROTOCOL_Curtain_Process(const HOOCH_PROTOCOL_CurtainFrame_t *frame);
void HOOCH_PROTOCOL_CodeMatchDispatch_Process(const HOOCH_PROTOCOL_CodeMatchReportFrame_t *frame);
void HOOCH_PROTOCOL_SceneDispatch_Process(const HOOCH_PROTOCOL_SceneDispatchFrame_t *frame);
/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  
  // 清除USART2所有中断标志，防止误触发
  __HAL_UART_CLEAR_FLAG(&huart2, UART_FLAG_RXNE | UART_FLAG_ORE | UART_FLAG_FE | UART_FLAG_NE);
  
  // 使能RXNE接收中断
  __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE); 
  
  // 初始化日志模块
  Log_Init();
  
  // 初始化小米UART协议解析模块（包含串口接收初始化）
  xiaoni_smart_screen_uart_init();

  // 初始化 KNX 面板协议解析模块
  // 注意：KNX 与小米协议共享 USART2，在 USART2 中断中会把每个字节同时喂给两套解析器
 // knx_uart_init();
 // extern void tuya_uart_init(void);
 // tuya_uart_init();
  // 初始化 BLE 协议解析模块
  // BLE 也复用 USART2 字节流，在中断中并行喂入 BLE 状态机。
//  ble_uart_init();
         
  // 开机打印
  LOG_INFO("========================================\r\n");
  LOG_INFO("  XiaoMi F103 System Startup\r\n");
  LOG_INFO("  MCU: STM32F103\r\n");
  LOG_INFO("  SysClock: 72MHz\r\n");
  LOG_INFO("  UART1: 115200 (Log)\r\n");
  LOG_INFO("  UART2: 115200 (Communication)\r\n");
  LOG_INFO("  UART3: 115200\r\n");
  LOG_INFO("========================================\r\n");
  
  // 初始化CLI命令行
  Hooch_CLI_Init();
  HOOCH_PROTOCOL_KeyStatusDispatch_RegisterCallback(HOOCH_PROTOCOL_KeyStatusReport_Process);
  HOOCH_PROTOCOL_Setting_RegisterCallback(HOOCH_PROTOCOL_Setting_Process);
  HOOCH_PROTOCOL_KeyName_RegisterCallback(HOOCH_PROTOCOL_KeyName_Process);
  HOOCH_PROTOCOL_KeyMode_RegisterCallback(HOOCH_PROTOCOL_KeyMode_Process);
  HOOCH_PROTOCOL_AirConditioner_RegisterCallback(HOOCH_PROTOCOL_AirConditioner_Process);
  HOOCH_PROTOCOL_FloorHeating_RegisterCallback(HOOCH_PROTOCOL_FloorHeating_Process);
  HOOCH_PROTOCOL_FreshAir_RegisterCallback(HOOCH_PROTOCOL_FreshAir_Process);

  
  HOOCH_PROTOCOL_Curtain_RegisterCallback(HOOCH_PROTOCOL_Curtain_Process);
  HOOCH_PROTOCOL_CodeMatchDispatch_RegisterCallback(HOOCH_PROTOCOL_CodeMatchDispatch_Process);
  HOOCH_PROTOCOL_SceneDispatch_RegisterCallback(HOOCH_PROTOCOL_SceneDispatch_Process);
  HOOCH_PROTOCOL_DimmerLight_RegisterCallback(HOOCH_PROTOCOL_DimmerLight_Process);
  /* Infinite loop */ 
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    
    
    // 处理小米UART协议帧解析
    xiaoni_smart_screen_uart_process();

    // 处理 KNX 面板协议帧解析（与小米协议并行解析同一串口字节流）
   // knx_uart_process();

//    // 处理 BLE 协议帧解析（与小米/KNX 并行解析同一串口字节流）
//    ble_uart_process();

    // 处理 Tuya Zigbee UART 协议帧解析
  //  uart_service_parse();
    
    // 处理CLI命令
    Hooch_CLI_Process();
    
    
    HAL_Delay(20);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

void HOOCH_PROTOCOL_Curtain_Process(const HOOCH_PROTOCOL_CurtainFrame_t *frame)
{
  if (frame == NULL)
  {
    LOG_INFO("[HOOCH] Curtain callback: frame is NULL\r\n");
    return;
  }

  LOG_INFO("[HOOCH] Curtain switch_status=%u, stop=%u, percent=%u, angle=%u, control_item=%u, valid=%u\r\n",
           (unsigned int)frame->switch_status.value,
           (unsigned int)frame->stop.value,
           (unsigned int)frame->percent.value,
           (unsigned int)frame->angle.value,
           (unsigned int)frame->control_item,
           (unsigned int)frame->valid);
}
void HOOCH_PROTOCOL_SceneDispatch_Process(const HOOCH_PROTOCOL_SceneDispatchFrame_t *frame)
{
  if (frame == NULL)
  {
    LOG_INFO("[HOOCH] SceneDispatch callback: frame is NULL\r\n");
    return;
  }

  LOG_INFO("[HOOCH] SceneDispatch scene=%u, sequence=%u, valid=%u item=%u, value=%u\r\n",
           (unsigned int)frame->scene,
           (unsigned int)frame->sequence,
           (unsigned int)frame->valid,
           (unsigned int)frame->control_item,
           (unsigned int)frame->value
          );
}
void HOOCH_PROTOCOL_AirConditioner_Process(const HOOCH_PROTOCOL_AirConditionerFrame_t *frame)
{
  if (frame == NULL)
  {
    LOG_INFO("[HOOCH] AirConditioner callback: frame is NULL\r\n");
    return;
  }

  LOG_INFO("[HOOCH] AirConditioner channel=%u, power=%u, mode=%u, fan_speed=%u, temperature=%u, control_item=%u, sequence=%u, valid=%u\r\n",
           (unsigned int)frame->channel,
           (unsigned int)frame->power,
           (unsigned int)frame->mode,
           (unsigned int)frame->fan_speed,
           (unsigned int)frame->temperature,
           (unsigned int)frame->control_item,
           (unsigned int)frame->sequence,
           (unsigned int)frame->valid);
}
void HOOCH_PROTOCOL_FloorHeating_Process(const HOOCH_PROTOCOL_FloorHeatingFrame_t *frame)
{
  if (frame == NULL)
  {
    LOG_INFO("[HOOCH] FloorHeating callback: frame is NULL\r\n");
    return;
  }

  LOG_INFO("[HOOCH] FloorHeating channel=%u, power=%u, mode=%u, target_temp=%u, current_temp=%u, control_item=%u, sequence=%u, valid=%u\r\n",
           (unsigned int)frame->channel,
           (unsigned int)frame->power,
           (unsigned int)frame->mode,
           (unsigned int)frame->target_temperature,
           (unsigned int)frame->current_temperature,
           (unsigned int)frame->control_item,
           (unsigned int)frame->sequence,
           (unsigned int)frame->valid);
}
void HOOCH_PROTOCOL_FreshAir_Process(const HOOCH_PROTOCOL_FreshAirFrame_t *frame)
{
  if (frame == NULL)
  {
    LOG_INFO("[HOOCH] FreshAir callback: frame is NULL\r\n");
    return;
  }

  LOG_INFO("[HOOCH] FreshAir channel=%u, power=%u, mode=%u, fan_speed=%u, control_item=%u, sequence=%u, valid=%u\r\n",
           (unsigned int)frame->channel,
           (unsigned int)frame->power,
           (unsigned int)frame->mode,
           (unsigned int)frame->fan_speed,
           (unsigned int)frame->control_item,
           (unsigned int)frame->sequence,
           (unsigned int)frame->valid);
}
void HOOCH_PROTOCOL_DimmerLight_Process(const HOOCH_PROTOCOL_DimmerLightFrame_t *frame)
{
  if (frame == NULL)
  {
    LOG_INFO("[HOOCH] DimmerLight callback: frame is NULL\r\n");
    return;
  }

  LOG_INFO("[HOOCH] DimmerLight brightness=%u, color_temperature=%u, switch_state=%u, control_item=%u, sequence=%u, valid=%u\r\n",
           (unsigned int)frame->brightness,
           (unsigned int)frame->color_temperature,
           (unsigned int)frame->switch_state,
           (unsigned int)frame->control_item,
           (unsigned int)frame->sequence,
           (unsigned int)frame->valid);
}

void HOOCH_PROTOCOL_KeyStatusReport_Process(const HOOCH_PROTOCOL_KeyStatusDispatchFrame_t *frame)
{
  if (frame == NULL)
  {
    LOG_INFO("[HOOCH] KeyStatus callback: frame is NULL\r\n");
    return;
  }

  LOG_INFO("[HOOCH] KeyStatus key=%u, state=%u, sequence=%u, valid=%u\r\n",
           (unsigned int)frame->key,
           (unsigned int)frame->state,
           (unsigned int)frame->sequence,
           (unsigned int)frame->valid);
}

void HOOCH_PROTOCOL_Setting_Process(const HOOCH_PROTOCOL_SettingFrame_t *frame)
{
  if (frame == NULL)
  {
    LOG_INFO("[HOOCH] Setting callback: frame is NULL\r\n");
    return;
  }
  
  LOG_INFO("[HOOCH] Setting item=%u, value=%u, page=%u, sequence=%u, valid=%u, screen_off_time_type=%u\r\n",
           (unsigned int)frame->item,
           (unsigned int)frame->value,
           (unsigned int)frame->page,
           (unsigned int)frame->sequence,
           (unsigned int)frame->valid,
          (unsigned int)frame->screen_off_time_type);

  if (frame->item == HOOCH_PROTOCOL_SETTING_ITEM_KEY_JOG_TIME)
  {
    /* JOG_TIME: value=按键(1-based), param1=开关(0关/1开), param2=时间(1~10s) */
    LOG_INFO("[HOOCH] JogTime key=%u switch=%s time=%us\r\n",
             (unsigned int)frame->value,
             (frame->param1 == 0U) ? "OFF" : "ON",
             (unsigned int)frame->param2);
  }

  if (frame->item == HOOCH_PROTOCOL_SETTING_ITEM_ADD_PAGE ||
      frame->item == HOOCH_PROTOCOL_SETTING_ITEM_DELETE_PAGE)
  {
    /* PAGE_MGR 拆分回调: item=ADD/HIDE, value=元素个数, page=具体页枚举 */
    const char *action = (frame->item == HOOCH_PROTOCOL_SETTING_ITEM_ADD_PAGE) ? "ADD" : "HIDE";
    LOG_INFO("[HOOCH] PageMgr page=%u action=%s val=%u\r\n",
             (unsigned int)frame->page,
             action,
             (unsigned int)frame->value);
  }

  if (frame->item == HOOCH_PROTOCOL_SETTING_ITEM_MULTICAST_GROUP_ID)
  {
    /* 组播ID: value=按键通道(1-based), param1/param2=组播ID(uint16 BE) */
    unsigned short gid = ((unsigned short)frame->param1 << 8) | frame->param2;
    LOG_INFO("[HOOCH] MulticastGroup key=%u gid=0x%04X (%u)\r\n",
             (unsigned int)frame->value,
             (unsigned int)gid,
             (unsigned int)gid);
  }

}



void HOOCH_PROTOCOL_KeyName_Process(const HOOCH_PROTOCOL_KeyNameFrame_t *frame)
{
  if (frame == NULL)
  {
    LOG_INFO("[HOOCH] KeyName callback: frame is NULL\r\n");
    return;
  }
  
  LOG_INFO("[HOOCH] KeyName key=%u, icon=%u, page=%u, delivery=%u, name=%s, sequence=%u, valid=%u\r\n",
           (unsigned int)frame->key,
           (unsigned int)frame->icon_index,
           (unsigned int)frame->page,
           (unsigned int)frame->delivery,
           frame->name,
           (unsigned int)frame->sequence,
           (unsigned int)frame->valid);
}

void HOOCH_PROTOCOL_KeyMode_Process(const HOOCH_PROTOCOL_KeyModeFrame_t *frame)
{
  if (frame == NULL)
  {
    LOG_INFO("[HOOCH] KeyMode callback: frame is NULL\r\n");
    return;
  }

  LOG_INFO("[HOOCH] KeyMode key=%u, power_on_status=%u, type=%u, sequence=%u, valid=%u\r\n",
           (unsigned int)frame->key,
           (unsigned int)frame->power_on_status,
           (unsigned int)frame->type,
           (unsigned int)frame->sequence,
           (unsigned int)frame->valid);
}

void HOOCH_PROTOCOL_CodeMatchDispatch_Process(const HOOCH_PROTOCOL_CodeMatchReportFrame_t *frame)
{
  if (frame == NULL)
  {
    LOG_INFO("[HOOCH] CodeMatch callback: frame is NULL\r\n");
    return;
  }

  /*
   * 根据 page 区分 channel 的解析规则：
   *   开关页面(0x00): channel 高4bit=按键索引(0~3), 低4bit=模式(0=调光, 1=窗帘)
   *   其他页面:     channel 为顺序子项索引
   */
  if (frame->page == 0x00U)
  {
    uint8_t key  = (frame->channel >> 4) + 1U;      /* 按键1~4 */
    uint8_t mode = frame->channel & 0x0FU;           /* 0=调光, 1=窗帘 */
    const char *mode_str = (mode == 0U) ? "DIMMER" : "CURTAIN";

    LOG_INFO("[HOOCH] CodeMatch page=SWITCH, key=%u, mode=%s, action=%s, seq=%u, valid=%u\r\n",
             (unsigned int)key,
             mode_str,
             (frame->action == HOOCH_PROTOCOL_CODE_MATCH_REPORT_ACTION_MATCH) ? "MATCH" : "CLEAR",
             (unsigned int)frame->sequence,
             (unsigned int)frame->valid);
  }
  else
  {
    const char *page_str;
    switch (frame->page)
    {
      case 0x02U: page_str = "LIGHT";    break;
      case 0x03U: page_str = "CURTAIN";  break;
      case 0x04U: page_str = "AC";       break;
      default:    page_str = "UNKNOWN";  break;
    }

    LOG_INFO("[HOOCH] CodeMatch page=%s, channel=%u, action=%s, seq=%u, valid=%u\r\n",
             page_str,
             (unsigned int)frame->channel,
             (frame->action == HOOCH_PROTOCOL_CODE_MATCH_REPORT_ACTION_MATCH) ? "MATCH" : "CLEAR",
             (unsigned int)frame->sequence,
             (unsigned int)frame->valid);
  }
}


#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}

#endif /* USE_FULL_ASSERT */
