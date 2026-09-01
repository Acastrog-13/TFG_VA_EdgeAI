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
#include "stm32h7xx_hal.h"

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define D6_Pin GPIO_PIN_4
#define D6_GPIO_Port GPIOE
#define D8_Pin GPIO_PIN_5
#define D8_GPIO_Port GPIOE
#define D9_Pin GPIO_PIN_6
#define D9_GPIO_Port GPIOE
#define p_on_Pin GPIO_PIN_13
#define p_on_GPIO_Port GPIOC
#define p_on_EXTI_IRQn EXTI15_10_IRQn
#define XCLK_Pin GPIO_PIN_0
#define XCLK_GPIO_Port GPIOA
#define HREF_Pin GPIO_PIN_4
#define HREF_GPIO_Port GPIOA
#define PCLK_Pin GPIO_PIN_6
#define PCLK_GPIO_Port GPIOA
#define led_rojo_Pin GPIO_PIN_12
#define led_rojo_GPIO_Port GPIOE
#define led_amarillo_Pin GPIO_PIN_15
#define led_amarillo_GPIO_Port GPIOE
#define led_verde_Pin GPIO_PIN_10
#define led_verde_GPIO_Port GPIOB
#define led_azul_Pin GPIO_PIN_11
#define led_azul_GPIO_Port GPIOB
#define RST_Pin GPIO_PIN_2
#define RST_GPIO_Port GPIOG
#define PWDN_Pin GPIO_PIN_3
#define PWDN_GPIO_Port GPIOG
#define D2_Pin GPIO_PIN_6
#define D2_GPIO_Port GPIOC
#define D3_Pin GPIO_PIN_7
#define D3_GPIO_Port GPIOC
#define D4_Pin GPIO_PIN_8
#define D4_GPIO_Port GPIOC
#define D5_Pin GPIO_PIN_9
#define D5_GPIO_Port GPIOC
#define D7_Pin GPIO_PIN_3
#define D7_GPIO_Port GPIOD
#define VSYNC_Pin GPIO_PIN_7
#define VSYNC_GPIO_Port GPIOB
#define SIOC_Pin GPIO_PIN_8
#define SIOC_GPIO_Port GPIOB
#define SIOD_Pin GPIO_PIN_9
#define SIOD_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
