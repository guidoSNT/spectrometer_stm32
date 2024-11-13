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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "encoder.h"
#include "math.h"
#include "pwm.h"
#include "fft.h"
#include "fonts.h"
#include "ssd1306.h"
#include "display_analizador.h"
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
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
DMA_HandleTypeDef hdma_tim2_ch1;

UART_HandleTypeDef huart1;

/* Definitions for display */
osThreadId_t displayHandle;
const osThreadAttr_t display_attributes = {
  .name = "display",
  .stack_size = 203 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for fft */
osThreadId_t fftHandle;
const osThreadAttr_t fft_attributes = {
  .name = "fft",
  .stack_size = 277 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for switchesData */
osMessageQueueId_t switchesDataHandle;
const osMessageQueueAttr_t switchesData_attributes = {
  .name = "switchesData"
};
/* Definitions for fftRead */
osMutexId_t fftReadHandle;
const osMutexAttr_t fftRead_attributes = {
  .name = "fftRead"
};
/* Definitions for adcSave */
osMutexId_t adcSaveHandle;
const osMutexAttr_t adcSave_attributes = {
  .name = "adcSave"
};
/* USER CODE BEGIN PV */
uint16_t adc_data[256];
uint16_t count = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
void entryDisplay(void *argument);
void entryDatos(void *argument);

/* USER CODE BEGIN PFP */
void
AE_callback_in (int);
void
AE_callback_out (int);
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
  MX_DMA_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  SSD1306_Init ();
  PWM_init (&htim2, TIM_CHANNEL_1);

  PWM_change_freq_dma (500, 1);
  switch_values_t temp =
    { AN_inputs_reader ().button_encoder.value,
	AN_inputs_reader ().button_sec.value };
  AN_graph_control (temp, 1);
  SSD1306_UpdateScreen ();
  AN_init (&huart1);
  HAL_GPIO_TogglePin (GPIOC, GPIO_PIN_13);
  AN_send_uart ();
  HAL_GPIO_TogglePin (GPIOC, GPIO_PIN_13);
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of fftRead */
  fftReadHandle = osMutexNew(&fftRead_attributes);

  /* creation of adcSave */
  adcSaveHandle = osMutexNew(&adcSave_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of switchesData */
  switchesDataHandle = osMessageQueueNew (5, sizeof(switch_values_t), &switchesData_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of display */
  displayHandle = osThreadNew(entryDisplay, NULL, &display_attributes);

  /* creation of fft */
  fftHandle = osThreadNew(entryDatos, NULL, &fft_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
    {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 2880-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 10;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 71;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 921600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB13 PB14 PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB4 PB5 PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void
AE_callback_in (int tag)
{
  switch (tag)
    {
    case TAG_IDLE:
      HAL_GPIO_WritePin (GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
      break;
    case TAG_MONITOR:
      HAL_GPIO_WritePin (GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
      break;
    case TAG_PROCESO:
      HAL_GPIO_WritePin (GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
      break;
    }
}
void
AE_callback_out (int tag)
{
  switch (tag)
    {
    case TAG_IDLE:
      HAL_GPIO_WritePin (GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
      break;
    case TAG_MONITOR:
      HAL_GPIO_WritePin (GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
      break;
    case TAG_PROCESO:
      HAL_GPIO_WritePin (GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
      break;
    }
}

void
HAL_GPIO_EXTI_Callback (uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_13 || GPIO_Pin == GPIO_PIN_14)
    {
      AN_encoder_setter ();
    }
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_entryDisplay */
/**
 * @brief Function implementing the display thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_entryDisplay */
void entryDisplay(void *argument)
{
  /* USER CODE BEGIN 5 */
  vTaskSetApplicationTaskTag (NULL, (void*) TAG_MONITOR);
  TickType_t lastWakeTime = xTaskGetTickCount ();
  inputs_t input_old = AN_inputs_reader ();
  display_state_t display_state = fft_pre_off;
  display_event_t display_event = off_stay;
  config_disp_t disp_state;
  osMutexAcquire (adcSaveHandle, portMAX_DELAY);
  //AN_send_uart ();
  /* Infinite loop */
  for (;;)
    {
      inputs_t input_temp = AN_inputs_reader ();
      switch_values_t switches_vec[5];
      switches_vec[0].encoder = input_temp.button_encoder.value;
      switches_vec[0].second = input_temp.button_sec.value;
      uint16_t messages = osMessageQueueGetCount (switchesDataHandle);
      uint8_t update_fft = AN_config_reader ().fft_pass;
      int16_t diff = input_temp.encoder.position != input_old.encoder.position;
      uint8_t update_screen = 0, update_pwm = 0, send_fft = 0;

      if (messages != 0)
	{
	  for (int i = 0; i < messages; i++)
	    {
	      osMessageQueueGet (switchesDataHandle, &switches_vec[i], NULL,
	      osWaitForever);
	    }
	}

      input_old = AN_inputs_reader ();

      switch (switches_vec[0].second)
	{
	case on:
	  display_event = on_stay;
	  if (diff != 0 || messages != 0 || update_fft == 1)
	    {
	      update_screen = 1;
	      display_event = on_update;
	    }
	  break;
	case off:
	  display_event = off_stay;
	  if (diff != 0 || messages != 0 || update_fft == 1)
	    {
	      update_screen = 1;
	      display_event = off_update;
	    }
	  break;
	}

      if (messages != 0)
	messages--;
      if (update_fft == 1)
	AN_fft_update_setter ();

      do
	{
	  switch (display_state)
	    {
	    case config:
	      switch (display_event)
		{
		case off_update:
		  AN_graph_control (switches_vec[messages], 0);
		  if (AN_config_reader ().update_pwm == 1)
		    update_pwm = 1;
		  if (AN_config_reader ().fft_pass == 1)
		    send_fft = 1;
		  break;
		case on_update:
		case on_stay:
		  AN_graph_control (switches_vec[messages], 0);
		  if (AN_config_reader ().update_pwm == 1)
		    update_pwm = 1;
		  if (AN_config_reader ().fft_pass == 1)
		    send_fft = 1;
		  display_state = config_pre_off;
		  break;
		}
	      break;
	    case config_pre_off:
	      switch (display_event)
		{
		case on_update:
		  break;
		case off_update:
		case off_stay:
		  display_state = fft;
		  AN_graph_fft (input_temp.encoder.position % 128);
		  if (AN_config_reader ().update_pwm == 1)
		    update_pwm = 1;
		  HAL_TIM_Base_Start_IT (&htim3);
		  osMutexRelease (adcSaveHandle);
		  update_screen = 1;
		  break;
		}
	      break;
	    case fft:
	      osMutexAcquire (fftReadHandle, portMAX_DELAY);
	      switch (display_event)
		{
		case off_stay:
		  break;
		case off_update:
		  AN_graph_fft (input_temp.encoder.position % 128);
		  if (AN_config_reader ().update_pwm == 1)
		    update_pwm = 1;
		  update_screen = 1;
		  break;
		case on_stay:
		case on_update:
		  display_state = fft_pre_off;
		  osMutexAcquire (adcSaveHandle, portMAX_DELAY);
		  HAL_TIM_Base_Stop_IT (&htim3);
		  break;
		}
	      break;
	    case fft_pre_off:
	      switch (display_event)
		{
		case on_update:
		  break;
		case off_stay:
		case off_update:
		  AN_graph_control (switches_vec[messages], 1);
		  update_pwm = 1;
		  display_state = config;
		  break;
		}
	      break;
	    }
	}
      while (0 > --messages);

      if (send_fft == 1)
	{

	}
      if (update_pwm == 1)
	{
	  disp_state = AN_config_reader ();
	  PWM_change_freq_dma (disp_state.pwm_state.freq,
			       disp_state.pwm_state.amp / (float) 33);
	}
      if (update_screen == 1)
	SSD1306_UpdateScreen ();

      osMutexRelease (fftReadHandle);
      osDelay (pdMS_TO_TICKS(50));
      //vTaskDelayUntil (&lastWakeTime, pdMS_TO_TICKS(80));
    }

  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_entryDatos */
/**
 * @brief Function implementing the display thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_entryDatos */
void entryDatos(void *argument)
{
  /* USER CODE BEGIN entryDatos */
  vTaskSetApplicationTaskTag (NULL, (void*) TAG_PROCESO);

  /* Infinite loop */
  for (;;)
    {
      osMutexAcquire (adcSaveHandle, portMAX_DELAY);
      if (count == 255)
	{
	  osMutexAcquire (fftReadHandle, portMAX_DELAY);
	  HAL_TIM_Base_Stop_IT (&htim3);
	  AN_fft_fast (adc_data, 256);
	  AN_fft_update_setter ();
	  HAL_TIM_Base_Start_IT (&htim3);
	  osMutexRelease (fftReadHandle);
	  count = 0;
	}
      osMutexRelease (adcSaveHandle);
      osDelay (64);
    }
  /* USER CODE END entryDatos */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  if (htim->Instance == TIM4)
    {
      if (AN_button_setter () == 1)
	{
	  inputs_t input_temp = AN_inputs_reader ();
	  switch_values_t switches_temp;
	  switches_temp.encoder = input_temp.button_encoder.value;
	  switches_temp.second = input_temp.button_sec.value;
	  osMessageQueuePut (switchesDataHandle, &switches_temp, NULL, 0);
	}
    }

  if (htim->Instance == TIM3)
    {
      HAL_ADC_Start_IT (&hadc1);
      if (count < 255)
	{
	  adc_data[count] = HAL_ADC_GetValue (&hadc1);
	  count++;
	}
      HAL_ADC_Stop_IT (&hadc1);
    }
  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq ();
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
