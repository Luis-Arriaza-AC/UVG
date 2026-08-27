/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define BotonJ1_Pin GPIO_PIN_0
#define BotonJ1_GPIO_Port GPIOC
#define BotonJ1_EXTI_IRQn EXTI0_IRQn
#define BotonJ2_Pin GPIO_PIN_1
#define BotonJ2_GPIO_Port GPIOC
#define BotonJ2_EXTI_IRQn EXTI1_IRQn
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define LedR1_Pin GPIO_PIN_6
#define LedR1_GPIO_Port GPIOA
#define LedR2_Pin GPIO_PIN_7
#define LedR2_GPIO_Port GPIOA
#define LedR3_Pin GPIO_PIN_4
#define LedR3_GPIO_Port GPIOC
#define LedR4_Pin GPIO_PIN_5
#define LedR4_GPIO_Port GPIOC
#define dispG_Pin GPIO_PIN_12
#define dispG_GPIO_Port GPIOB
#define dispF_Pin GPIO_PIN_13
#define dispF_GPIO_Port GPIOB
#define dispE_Pin GPIO_PIN_14
#define dispE_GPIO_Port GPIOB
#define dispD_Pin GPIO_PIN_15
#define dispD_GPIO_Port GPIOB
#define dispC_Pin GPIO_PIN_6
#define dispC_GPIO_Port GPIOC
#define dispB_Pin GPIO_PIN_7
#define dispB_GPIO_Port GPIOC
#define dispA_Pin GPIO_PIN_8
#define dispA_GPIO_Port GPIOC
#define LedB4_Pin GPIO_PIN_9
#define LedB4_GPIO_Port GPIOA
#define LedB3_Pin GPIO_PIN_10
#define LedB3_GPIO_Port GPIOA
#define LedB2_Pin GPIO_PIN_11
#define LedB2_GPIO_Port GPIOA
#define LedB1_Pin GPIO_PIN_12
#define LedB1_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define BotonReset_Pin GPIO_PIN_4
#define BotonReset_GPIO_Port GPIOB
#define BotonReset_EXTI_IRQn EXTI4_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
