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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
volatile uint8_t modo = 0;         // 0: Espera, 1: Juego, 3: Ganador
volatile uint8_t contadorJ1 = 0;   // 0 a 4
volatile uint8_t contadorJ2 = 0;   // 0 a 4
volatile uint8_t ganador = 0;      // 1 para Jugador 1, 2 para Jugador 2
volatile uint8_t flagReset = 0;    // Bandera para procesar el reset en el main
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */
void ActualizarLeds(void);
void ResetearJuego(void);
void MostrarDisplay(uint8_t numero);
void IniciarCuentaRegresiva(void);
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
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
    {
      if (flagReset == 1)
      {
        flagReset = 0;      // Limpiar la bandera
        ResetearJuego();    // Ejecuta la cuenta regresiva con HAL_Delay libremente
        modo = 1;           // Inicia el juego tras finalizar el conteo
      }

      ActualizarLeds();
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LedR1_Pin|LedR2_Pin|LedB4_Pin|LedB3_Pin
                          |LedB2_Pin|LedB1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LedR3_Pin|LedR4_Pin|dispC_Pin|dispB_Pin
                          |dispA_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, dispG_Pin|dispF_Pin|dispE_Pin|dispD_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BotonJ1_Pin BotonJ2_Pin */
  GPIO_InitStruct.Pin = BotonJ1_Pin|BotonJ2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : USART_TX_Pin USART_RX_Pin */
  GPIO_InitStruct.Pin = USART_TX_Pin|USART_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LedR1_Pin LedR2_Pin LedB4_Pin LedB3_Pin
                           LedB2_Pin LedB1_Pin */
  GPIO_InitStruct.Pin = LedR1_Pin|LedR2_Pin|LedB4_Pin|LedB3_Pin
                          |LedB2_Pin|LedB1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LedR3_Pin LedR4_Pin dispC_Pin dispB_Pin
                           dispA_Pin */
  GPIO_InitStruct.Pin = LedR3_Pin|LedR4_Pin|dispC_Pin|dispB_Pin
                          |dispA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : dispG_Pin dispF_Pin dispE_Pin dispD_Pin */
  GPIO_InitStruct.Pin = dispG_Pin|dispF_Pin|dispE_Pin|dispD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : BotonReset_Pin */
  GPIO_InitStruct.Pin = BotonReset_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BotonReset_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  // Interrupción del Botón Reset / Inicio (PB4)
  if (GPIO_Pin == BotonReset_Pin)
  {
    if (modo == 0 || modo == 3)
    {
      flagReset = 1; // Levanta la bandera para atender la secuencia en el main()
    }
  }

  // Interrupción del Botón Jugador 1 (PC0)
  else if (GPIO_Pin == BotonJ1_Pin)
  {
    if (modo == 1)
    {
      if (contadorJ1 < 4)
      {
        contadorJ1++;
        if (contadorJ1 == 4)
        {
          ganador = 1; // Gana Jugador 1
          modo = 3;    // Bloquea botones
        }
      }
    }
  }

  // Interrupción del Botón Jugador 2 (PC1)
  else if (GPIO_Pin == BotonJ2_Pin)
  {
    if (modo == 1)
    {
      if (contadorJ2 < 4)
      {
        contadorJ2++;
        if (contadorJ2 == 4)
        {
          ganador = 2; // Gana Jugador 2
          modo = 3;    // Bloquea botones
        }
      }
    }
  }
}

void ResetearJuego(void)
{
  modo = 0;          // Mantiene botones bloqueados durante el conteo
  contadorJ1 = 0;
  contadorJ2 = 0;
  ganador = 0;
  ActualizarLeds();  // Asegura LEDs apagados

  IniciarCuentaRegresiva(); // Conteo 5 a 0 con HAL_Delay() funcional
}

void IniciarCuentaRegresiva(void)
{
  for (int i = 5; i > 0; i--)
  {
    MostrarDisplay(i);
    HAL_Delay(1000); // Ya no se congela porque estamos fuera de la ISR
  }
  MostrarDisplay(99);  // Apaga el display al terminar
}

void ActualizarLeds(void)
{
  // Si está en modo 0 (conteo o espera), los LEDs deben permanecer apagados
  if (modo == 0)
  {
    HAL_GPIO_WritePin(GPIOA, LedR1_Pin|LedR2_Pin|LedB1_Pin|LedB2_Pin|LedB3_Pin|LedB4_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, LedR3_Pin|LedR4_Pin, GPIO_PIN_RESET);
    return;
  }

  // --- Mapeo de LEDs para Jugador 1 (PA6, PA7, PC4, PC5) ---
  HAL_GPIO_WritePin(GPIOA, LedR1_Pin, (contadorJ1 >= 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, LedR2_Pin, (contadorJ1 >= 2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, LedR3_Pin, (contadorJ1 >= 3) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, LedR4_Pin, (contadorJ1 >= 4) ? GPIO_PIN_SET : GPIO_PIN_RESET);

  // --- Mapeo de LEDs para Jugador 2 (PA12, PA11, PA10, PA9) ---
  HAL_GPIO_WritePin(GPIOA, LedB1_Pin, (contadorJ2 >= 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, LedB2_Pin, (contadorJ2 >= 2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, LedB3_Pin, (contadorJ2 >= 3) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, LedB4_Pin, (contadorJ2 >= 4) ? GPIO_PIN_SET : GPIO_PIN_RESET);

  // --- Control del Display de 7 Segmentos en Modo Ganador ---
  if (modo == 3)
  {
    MostrarDisplay(ganador); // Muestra 1 o 2
  }
}

void MostrarDisplay(uint8_t numero)
{
  // Mapeo de Pines del Display (Ánodo Común):
  // dispA (PC8), dispB (PC7), dispC (PC6), dispD (PB15), dispE (PB14), dispF (PB13), dispG (PB12)

  switch (numero)
  {
    case 0: // Muestra "0" (A, B, C, D, E, F)
      HAL_GPIO_WritePin(GPIOC, dispA_Pin|dispB_Pin|dispC_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, dispD_Pin|dispE_Pin|dispF_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, dispG_Pin, GPIO_PIN_RESET);
      break;

    case 1: // Muestra "1" (B, C)
      HAL_GPIO_WritePin(GPIOC, dispA_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOC, dispB_Pin|dispC_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, dispD_Pin|dispE_Pin|dispF_Pin|dispG_Pin, GPIO_PIN_RESET);
      break;

    case 2: // Muestra "2" (A, B, D, E, G)
      HAL_GPIO_WritePin(GPIOC, dispA_Pin|dispB_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOC, dispC_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, dispD_Pin|dispE_Pin|dispG_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, dispF_Pin, GPIO_PIN_RESET);
      break;

    case 3: // Muestra "3" (A, B, C, D, G)
      HAL_GPIO_WritePin(GPIOC, dispA_Pin|dispB_Pin|dispC_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, dispD_Pin|dispG_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, dispE_Pin|dispF_Pin, GPIO_PIN_RESET);
      break;

    case 4: // Muestra "4" (B, C, F, G)
      HAL_GPIO_WritePin(GPIOC, dispA_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOC, dispB_Pin|dispC_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, dispD_Pin|dispE_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, dispF_Pin|dispG_Pin, GPIO_PIN_SET);
      break;

    case 5: // Muestra "5" (A, C, D, F, G)
      HAL_GPIO_WritePin(GPIOC, dispA_Pin|dispC_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOC, dispB_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, dispD_Pin|dispF_Pin|dispG_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, dispE_Pin, GPIO_PIN_RESET);
      break;

    default: // Apagado total
      HAL_GPIO_WritePin(GPIOC, dispA_Pin|dispB_Pin|dispC_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, dispD_Pin|dispE_Pin|dispF_Pin|dispG_Pin, GPIO_PIN_RESET);
      break;
  }
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
#ifdef USE_FULL_ASSERT
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
