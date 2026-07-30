/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32h5xx_it.c
  * @brief   Interrupt Service Routines.
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32h5xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//#include "StereoMadness.h"
#include "SMB_Overworld.h"
#include "vgm.h"
#include "YM2151.h"
#include "msm6295.h"
#include <string.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

uint32_t drumDACIncrement = 0;
uint8_t triangleOffset = 0;
uint8_t attenValue;
uint32_t last_tick;
static uint32_t last_tick_pf12 = 0;
static uint32_t last_tick_pd10 = 0;
static uint32_t last_tick_pg7  = 0;
static uint32_t last_tick_pa0  = 0;

volatile uint8_t list_button = 0;
volatile uint8_t speed_reset_button = 0;

extern volatile int8_t song_change;
/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

volatile uint8_t timerRunning = 1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
float currentNoteMelody[2] = {0.0, 0.0};
float currentNoteMelody2[2] = {0.0, 0.0};
float currentNoteBass[2] = {0.0, 0.0};
float currentDrum[3];
uint8_t pauseToggle = 0;
float bpm = BPM, bpmIncrements[3] = {0.1, 1, 10};
uint8_t bpmIncrementIndex = 1;
float beatDuration;

extern volatile uint16_t wait_samples;

uint16_t noteData;
/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim4;
/* USER CODE BEGIN EV */
extern float noteDurationMelody, noteDurationMelody2, noteDurationBass, drumDuration;
extern uint8_t solo1, solo2, solo3, solo4, solo5;
uint16_t noteIndexMelody = 0, noteIndexMelody2 = 0, noteIndexBass = 0;
uint16_t encoder_count;

extern uint8_t ready;
uint8_t tone1Sent, tone2Sent, tone3Sent, noiseSent = 0;
uint8_t tone1Muted, tone2Muted, tone3Muted, noiseMuted = 0;

extern float volume;
extern uint16_t decay_counter;

extern float volume;
extern uint16_t decay_counter;
extern uint8_t nes_noise(void);

uint32_t noteSize = sizeof(melodyNotes)/sizeof(melodyNotes[0]);
uint32_t noteSize2 = sizeof(melodyNotes2)/sizeof(melodyNotes2[0]);
uint32_t noteSize3 = sizeof(bassNotes)/sizeof(bassNotes[0]);

/*static void ToneMelody(uint16_t frequency)
{

}
static void ToneMelody2(uint16_t frequency)
{

}
static void ToneBass(uint16_t frequency)
{

}*/

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32H5xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32h5xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI Line0 interrupt.
  */
void EXTI0_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI0_IRQn 0 */
  if(HAL_GetTick() - last_tick_pa0 > 200){
	  speed_reset_button = 1;
	  last_tick_pa0 = HAL_GetTick();
  }
  /* USER CODE END EXTI0_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(SPEED_RST_Pin);
  /* USER CODE BEGIN EXTI0_IRQn 1 */

  /* USER CODE END EXTI0_IRQn 1 */
}

/**
  * @brief This function handles EXTI Line7 interrupt.
  */
void EXTI7_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI7_IRQn 0 */
  if(HAL_GetTick() - last_tick_pg7 > 50){
	  list_button = 1;
	  last_tick_pg7 = HAL_GetTick();
  }
  /* USER CODE END EXTI7_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
  /* USER CODE BEGIN EXTI7_IRQn 1 */

  /* USER CODE END EXTI7_IRQn 1 */
}

/**
  * @brief This function handles EXTI Line10 interrupt.
  */
void EXTI10_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI10_IRQn 0 */
  if(HAL_GetTick() - last_tick_pd10 > 200){
	  song_change = 1;
	  last_tick_pd10 = HAL_GetTick();
  }
  /* USER CODE END EXTI10_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
  /* USER CODE BEGIN EXTI10_IRQn 1 */

  /* USER CODE END EXTI10_IRQn 1 */
}

/**
  * @brief This function handles EXTI Line12 interrupt.
  */
void EXTI12_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI12_IRQn 0 */
  if(HAL_GetTick() - last_tick_pf12 > 200){
	  song_change = -1;
	  last_tick_pf12 = HAL_GetTick();
  }
  /* USER CODE END EXTI12_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
  /* USER CODE BEGIN EXTI12_IRQn 1 */

  /* USER CODE END EXTI12_IRQn 1 */
}

/**
  * @brief This function handles EXTI Line14 interrupt.
  */
void EXTI14_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI14_IRQn 0 */
	if(HAL_GetTick() - last_tick > 50){
			timerRunning = !timerRunning;
			last_tick = HAL_GetTick();
		}
  /* USER CODE END EXTI14_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
  /* USER CODE BEGIN EXTI14_IRQn 1 */

  /* USER CODE END EXTI14_IRQn 1 */
}

/**
  * @brief This function handles TIM2 global interrupt.
  */
void TIM2_IRQHandler(void)
{
  /* USER CODE BEGIN TIM2_IRQn 0 */
  VGM_Update();
  /* USER CODE END TIM2_IRQn 0 */
  HAL_TIM_IRQHandler(&htim2);
  /* USER CODE BEGIN TIM2_IRQn 1 */

  /* USER CODE END TIM2_IRQn 1 */
}

/**
  * @brief This function handles TIM4 global interrupt.
  */
void TIM4_IRQHandler(void)
{
  /* USER CODE BEGIN TIM4_IRQn 0 */
  int16_t sample = MSM6295_Update();
  DAC1->DHR12R1 = (uint16_t)(((int32_t)sample/2 + 2048) & 0xFFF);
  /* USER CODE END TIM4_IRQn 0 */
  HAL_TIM_IRQHandler(&htim4);
  /* USER CODE BEGIN TIM4_IRQn 1 */

  /* USER CODE END TIM4_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
