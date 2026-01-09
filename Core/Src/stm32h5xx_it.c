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
#include "SN76489.h"
#include <string.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

uint32_t drumIndex = 0;
uint32_t drumDACIncrement = 0;
uint8_t triangleOffset = 0;
uint8_t attenValue;

/* USER CODE END TD */

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

uint16_t noteData;
/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
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

static void ToneMelody(uint16_t frequency)
{
	setNote(frequency, TONE1);
}
static void ToneMelody2(uint16_t frequency)
{
	setNote(frequency, TONE2);
}
static void ToneBass(uint16_t frequency)
{
	setNote(frequency, TONE3);
}

void pausePlay(void){


}

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
  * @brief This function handles TIM2 global interrupt.
  */
void TIM2_IRQHandler(void)
{
  /* USER CODE BEGIN TIM2_IRQn 0 */
	if(!tone1Sent){
		triangleOffset = 0;
		memcpy(currentNoteMelody, melodyNotes[noteIndexMelody], sizeof(currentNoteMelody));
		if (currentNoteMelody[0] != 0){

			if(tone1Muted){
				tone1Muted = 0;
				sendByte(TONE1_ATTEN | ATTEN_0DB);
			}
			ToneMelody((uint32_t)currentNoteMelody[0]);
			TIM3->ARR = 1000000/(60*currentNoteMelody[0]) - 1;
		}
		else{
			sendByte(TONE1_MUTE);
			tone1Muted = 1;
		}
		tone1Sent = 1;
	}
	noteDurationMelody += 0.025;
	if (noteDurationMelody >= beatDuration*currentNoteMelody[1]){
		noteDurationMelody = 0;
		noteIndexMelody++;
		tone1Sent = 0;
	}
	if(noteIndexMelody >= noteSize)
		noteIndexMelody = 0;

	if(!tone2Sent){
		memcpy(currentNoteMelody2, melodyNotes2[noteIndexMelody2], sizeof(currentNoteMelody2));
		if (currentNoteMelody2[0] != 0){
			if(tone2Muted){
				tone2Muted = 0;
				sendByte(TONE2_ATTEN | ATTEN_0DB);
			}

			ToneMelody2((uint16_t)currentNoteMelody2[0]);
		}
		else{
			sendByte(TONE2_MUTE);
			tone2Muted = 1;
		}
		tone2Sent = 1;
	}
	noteDurationMelody2 += 0.025;
	if (noteDurationMelody2 >= beatDuration*currentNoteMelody2[1]){
		noteDurationMelody2 = 0;
		noteIndexMelody2++;
		tone2Sent = 0;
	}
	if(noteIndexMelody2 >= noteSize2)
		noteIndexMelody2 = 0;

	if(!tone3Sent){
		memcpy(currentNoteBass, bassNotes[noteIndexBass], sizeof(currentNoteBass));
		if (currentNoteBass[0] != 0){
			if(tone3Muted){
				tone3Muted = 0;
				sendByte(TONE3_ATTEN | ATTEN_8DB);
			}

			ToneBass(2*(uint16_t)currentNoteBass[0]);
		}
		else{
			sendByte(TONE3_MUTE);
			tone3Muted = 1;
		}
		tone3Sent = 1;
	}
	noteDurationBass += 0.025;
	if (noteDurationBass >= beatDuration*currentNoteBass[1]){
		noteDurationBass = 0;
		noteIndexBass++;
		tone3Sent = 0;
	}
	if(noteIndexBass >= noteSize3)
		noteIndexBass = 0;

  /* USER CODE END TIM2_IRQn 0 */
  HAL_TIM_IRQHandler(&htim2);
  /* USER CODE BEGIN TIM2_IRQn 1 */

  /* USER CODE END TIM2_IRQn 1 */
}

/**
  * @brief This function handles TIM3 global interrupt.
  */
void TIM3_IRQHandler(void)
{
  /* USER CODE BEGIN TIM3_IRQn 0 */

  /* USER CODE END TIM3_IRQn 0 */
  HAL_TIM_IRQHandler(&htim3);
  /* USER CODE BEGIN TIM3_IRQn 1 */

  /* USER CODE END TIM3_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
