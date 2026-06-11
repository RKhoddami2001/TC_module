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
#define Disp_CS_Pin GPIO_PIN_13
#define Disp_CS_GPIO_Port GPIOC
#define ADC_SPI_CS_Pin GPIO_PIN_4
#define ADC_SPI_CS_GPIO_Port GPIOA
#define LED_Pin GPIO_PIN_12
#define LED_GPIO_Port GPIOB
#define Relay_signal_Pin GPIO_PIN_13
#define Relay_signal_GPIO_Port GPIOB
#define Flash_WP_Pin GPIO_PIN_15
#define Flash_WP_GPIO_Port GPIOA
#define Flash_Hold_Pin GPIO_PIN_2
#define Flash_Hold_GPIO_Port GPIOD
#define Flash_CS_Pin GPIO_PIN_3
#define Flash_CS_GPIO_Port GPIOB
#define input_PB1_Pin GPIO_PIN_4
#define input_PB1_GPIO_Port GPIOB
#define input_PB2_Pin GPIO_PIN_5
#define input_PB2_GPIO_Port GPIOB
#define input_PB3_Pin GPIO_PIN_8
#define input_PB3_GPIO_Port GPIOB
#define input_PB4_Pin GPIO_PIN_9
#define input_PB4_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
