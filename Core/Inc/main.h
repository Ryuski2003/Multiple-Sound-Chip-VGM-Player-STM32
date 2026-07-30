/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32h5xx_hal.h"

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
#define YM_D0_Pin GPIO_PIN_0
#define YM_D0_GPIO_Port GPIOF
#define YM_D1_Pin GPIO_PIN_1
#define YM_D1_GPIO_Port GPIOF
#define YM_D2_Pin GPIO_PIN_2
#define YM_D2_GPIO_Port GPIOF
#define YM_D3_Pin GPIO_PIN_3
#define YM_D3_GPIO_Port GPIOF
#define YM_D4_Pin GPIO_PIN_4
#define YM_D4_GPIO_Port GPIOF
#define YM_D5_Pin GPIO_PIN_5
#define YM_D5_GPIO_Port GPIOF
#define YM_D6_Pin GPIO_PIN_6
#define YM_D6_GPIO_Port GPIOF
#define YM_D7_Pin GPIO_PIN_7
#define YM_D7_GPIO_Port GPIOF
#define SPEED_RST_Pin GPIO_PIN_0
#define SPEED_RST_GPIO_Port GPIOA
#define SPEED_RST_EXTI_IRQn EXTI0_IRQn
#define LOOP_SW_Pin GPIO_PIN_2
#define LOOP_SW_GPIO_Port GPIOA
#define SHUFFLE_SW_Pin GPIO_PIN_3
#define SHUFFLE_SW_GPIO_Port GPIOA
#define CE_Pin GPIO_PIN_5
#define CE_GPIO_Port GPIOC
#define WE_Pin GPIO_PIN_6
#define WE_GPIO_Port GPIOC
#define TFT_RST_Pin GPIO_PIN_7
#define TFT_RST_GPIO_Port GPIOC
#define SD_CS_Pin GPIO_PIN_8
#define SD_CS_GPIO_Port GPIOA
#define TFT_CS_Pin GPIO_PIN_9
#define TFT_CS_GPIO_Port GPIOA
#define D0_Pin GPIO_PIN_0
#define D0_GPIO_Port GPIOD
#define D1_Pin GPIO_PIN_1
#define D1_GPIO_Port GPIOD
#define D2_Pin GPIO_PIN_2
#define D2_GPIO_Port GPIOD
#define D3_Pin GPIO_PIN_3
#define D3_GPIO_Port GPIOD
#define D4_Pin GPIO_PIN_4
#define D4_GPIO_Port GPIOD
#define D5_Pin GPIO_PIN_5
#define D5_GPIO_Port GPIOD
#define D6_Pin GPIO_PIN_6
#define D6_GPIO_Port GPIOD
#define D7_Pin GPIO_PIN_7
#define D7_GPIO_Port GPIOD
#define YM_IC_Pin GPIO_PIN_9
#define YM_IC_GPIO_Port GPIOG
#define YM_A0_Pin GPIO_PIN_10
#define YM_A0_GPIO_Port GPIOG
#define YM_WR_Pin GPIO_PIN_11
#define YM_WR_GPIO_Port GPIOG
#define YM_RD_Pin GPIO_PIN_12
#define YM_RD_GPIO_Port GPIOG
#define YM_CS_Pin GPIO_PIN_13
#define YM_CS_GPIO_Port GPIOG
#define TFT_DC_Pin GPIO_PIN_6
#define TFT_DC_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
/* Loop ON/OFF switch: PA2, short to GND + internal pull-up (the old
   TFT_A0 pin, confirmed unused, no longer assigned in CubeMX). */
#define LOOP_SW_Pin GPIO_PIN_2
#define LOOP_SW_GPIO_Port GPIOA
/* Shuffle ON/OFF switch: PA3, likewise GND + internal pull-up (the old
   TFT_RESET pin, confirmed unused - the real TFT reset line is PC7/TFT_RST). */
#define SHUFFLE_SW_Pin GPIO_PIN_3
#define SHUFFLE_SW_GPIO_Port GPIOA
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
