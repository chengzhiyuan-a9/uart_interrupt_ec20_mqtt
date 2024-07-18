/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "ec20_device.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SUB_TOPIC_1 "TestDev/%s/RcvMsg1"
#define SUB_TOPIC_2 "TestDev/%s/RcvMsg2"
#define SEND_TOPIC "TestDev/%s/SendMsg2"
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern uint8_t rx_buffer_;
EC20_CONFIG_INFO g_ec20_config_info;
ISP_INFO g_isp_info;
MQTT_REGISTER_INFO g_mqtt_register_info;
MQTT_SUB_TOPIC g_mqtt_sub_topic;
EC20_STATE_INFO g_ec20_state_info;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void util_str_cpy(char * dest, char * src, int length);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

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
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_buffer_, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  uint32_t uid[3] = {0};
  char id[64] = {0};
  uid[0] = HAL_GetUIDw0();
  uid[1] = HAL_GetUIDw1();
  uid[2] = HAL_GetUIDw2();
  snprintf(id, sizeof(id), "%lu%lu%lu", uid[0], uid[1], uid[2]);

  uart_printf("[%d]:UUID = %s\n", __LINE__, id);

  char dev_id[DEV_ID_LENGTH] = {0};
  snprintf(dev_id, sizeof(dev_id), "%s", id);

  char sub_topic_1_[MQTT_TOPIC_LENGTH] = {0};
  char sub_topic_2_[MQTT_TOPIC_LENGTH] = {0};
  char send_topic[MQTT_TOPIC_LENGTH] = {0};

  snprintf(sub_topic_1_, MQTT_TOPIC_LENGTH, SUB_TOPIC_1, dev_id);
  snprintf(sub_topic_2_, MQTT_TOPIC_LENGTH, SUB_TOPIC_2, dev_id);
  snprintf(send_topic, MQTT_TOPIC_LENGTH, SEND_TOPIC, dev_id);

  memset(&g_mqtt_sub_topic, 0, sizeof(g_mqtt_sub_topic));
  util_str_cpy(g_mqtt_sub_topic.subTopic[0], sub_topic_1_, MQTT_TOPIC_LENGTH);
  util_str_cpy(g_mqtt_sub_topic.subTopic[1], sub_topic_2_, MQTT_TOPIC_LENGTH);
  g_mqtt_sub_topic.topicNum = 2;

  memset(&g_ec20_config_info, 0, sizeof(g_ec20_config_info));
  g_ec20_config_info.MQTTTopic = &g_mqtt_sub_topic;

  memset(&g_isp_info, 0, sizeof(g_isp_info));
  util_str_cpy(g_isp_info.ispAPNName, ISP_APN_NAME, ISP_NAME_LENGTH);
  g_ec20_config_info.ispInfo = &g_isp_info;

  memset(&g_mqtt_register_info, 0, sizeof(g_mqtt_register_info));
  util_str_cpy(g_mqtt_register_info.mqttUrl, MQTT_REGISTER_INFO_URL, MQTT_URL_LENGTH);
  util_str_cpy(g_mqtt_register_info.username, MQTT_REGISTER_INFO_USR_NAME, MQTT_USERNAME_LENGTH);
  util_str_cpy(g_mqtt_register_info.password, MQTT_REGISTER_INFO_PASSWORD, MQTT_PASSWORD_LENGTH);
  util_str_cpy(g_mqtt_register_info.cliName, dev_id, MQTT_CLI_NAME_LENGTH);
  g_ec20_config_info.MQTTRegisterInfo = &g_mqtt_register_info;

  memset(&g_ec20_state_info, 0, sizeof(g_ec20_state_info));
  g_ec20_config_info.ec20StateInfo = &g_ec20_state_info;

  if (DEV_OK != ec20Init(&g_ec20_config_info)) {
    uart_printf("ec20 init failed\r\n");
  }
  uart_printf("ec20 init success\r\n");

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    ec20SendMsg(send_topic, "hello mqtt", 20);
    HAL_Delay(2000);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 6;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(huart);
  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_UART_TxCpltCallback could be implemented in the user file
*/
  if(huart->Instance == USART6)
  {
    UartHandle();
  }

}

int uart_printf(char *fmt, ...) {
  char sprint_buf[500];
  va_list args;
  int n;
  va_start(args, fmt);
  n = vsprintf(sprint_buf, fmt, args);
  va_end(args);
  HAL_UART_Transmit(&huart1, (uint8_t *) sprint_buf, n, n);
  return 0;
}

void util_str_cpy(char * dest, char * src, int length)
{
  while(*src != '\0' && length > 0)
  {
    *dest = *src;
    src++;
    dest++;
    length--;
  }

  *dest = '\0';
}
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
