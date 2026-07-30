/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "SN76489.h"
#include "YM2151.h"
#include "vgm.h"
#include "msm6295.h"
#include "sd_spi.h"
#include "fat32.h"
#include "ILI9341.h"
#include "SI5351.h"
#include "ui.h"
#include "shuffle.h"
#include "storage.h"
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

DAC_HandleTypeDef hdac1;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

/* USER CODE BEGIN PV */
SD_Status   sd_status;
int         fat_status;
FAT32_File  vgm_file;
char        vgm_filename[13]; /* e.g. "BALROG.VGM\0" */
int         vgm_valid;


uint8_t solo1;
uint8_t solo2;
uint8_t solo3;
uint8_t solo4;
uint8_t solo5;

extern const uint8_t vgm_data_start[];
extern const uint8_t vgm_data_end[];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ICACHE_Init(void);
static void MX_TIM2_Init(void);
static void MX_DAC1_Init(void);
static void MX_TIM4_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
float noteDurationMelody = 0.0;
float noteDurationMelody2 = 0.0;
float noteDurationBass = 0.0;
float drumDuration = 0.0;

uint8_t PitchData[5];

uint32_t PitchSquare1;
uint32_t PitchSquare2;
uint32_t PitchTriangle;
uint32_t PitchSawtooth;
uint32_t PitchDrum;

uint8_t paused = 1;
extern volatile uint16_t wait_samples;
extern volatile uint8_t timerRunning;
extern volatile uint8_t list_button;
extern volatile uint8_t speed_reset_button;

volatile int8_t song_change = 0;
int current_song_index = 8;

/* For continuous navigation (typematic) while a nav button is held in the file list */
#define NAV_REPEAT_DELAY_MS    400
#define NAV_REPEAT_INTERVAL_MS 120
static uint32_t nav_hold_start  = 0;
static uint32_t nav_last_repeat = 0;
static int8_t   nav_hold_dir    = 0;

/* Playback speed via rotary encoder (TIM3, X4 quadrature). TIM2's period is
   changed inversely proportional to speed: VGM_Update() is called more/less
   often, so the song content plays faster/slower in real time (without
   changing pitch). */
#define TIM2_BASE_PERIOD        1450U
#define PLAYBACK_SPEED_MIN      0.5f
#define PLAYBACK_SPEED_MAX      2.0f
#define PLAYBACK_SPEED_STEP     0.05f
#define ENCODER_COUNTS_PER_STEP 4
float          playback_speed = 1.0f;
static uint16_t last_encoder_count = 0;

float round_to(float value, float step) {
    return round(value / step) * step;
}

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

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ICACHE_Init();
  MX_TIM2_Init();
  MX_DAC1_Init();
  MX_TIM4_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_I2C1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  /* NOTE: When I2C1 (or another chosen I2C peripheral) is enabled in CubeMX
     and the code is regenerated, an MX_I2C1_Init(); call will be added next
     to the MX_SPI2_Init() line above - make sure that line is called BEFORE
     this point, otherwise hi2c1 will not be initialized yet. */
  SI5351_Init();

  ILI9341_Init();
  UI_Init();

  /* Use the last song index stored in flash if present, otherwise fall
     back to the factory default (8). */
  Storage_Init();
  current_song_index = Storage_GetLastSongIndex();

  sd_status  = SD_Init();
  fat_status = FAT32_Init();
  if (fat_status == 0) {
      fat_status = FAT32_FindFileByIndex(current_song_index, &vgm_file, vgm_filename);
      if (fat_status == 0) {
          vgm_valid = VGM_LoadFromSD(&vgm_file);
      }
  }

  /* If loaded from SD, the SD data has already been copied into the RAM
     buffer inside vgm.c. VGM_ParseHeader(NULL) uses that buffer; passing a
     different pointer would use that instead.
     Program the clocks into the SI5351 FIRST (inside VGM_ParseHeader), THEN
     do the chip resets - otherwise if the first song doesn't use YM2151
     (SI5351 output still disabled/uninitialized), the Init() writes would
     happen without a clock. */
  VGM_ParseHeader(NULL);
  YM2151_Init();
  SN76489_Init();
  VGM_Init();
  /* YM2151_Init sets all PAN registers to 0x00 (L=0, R=0 = silent).
     Open the output of all channels before VGM playback starts; VGM will
     overwrite these with its own values.
     But if this song doesn't use YM2151 (clock=0), don't unmute so it
     stays silent. */
  if (VGM_IsYM2151Active())
      YM_System_All_Unmute();
  if (VGM_IsSN76489Active())
      SN76489_UnmuteAll();
  /* Enable the TIM2 VGM timing interrupt (TIM2 > GPDMA priority) */
  HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
  HAL_TIM_Base_Start_IT(&htim2);

  HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
  HAL_TIM_Base_Start_IT(&htim4);

  /* Speed control encoder (PB4/PB5) */
  HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
  last_encoder_count = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  UI_Update();

	  // Speed control encoder (TIM3, X4 quadrature - 4 counts = 1 detent)
	  {
		  uint16_t enc_now   = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);
		  int16_t  enc_delta = (int16_t)(enc_now - last_encoder_count);

		  if (enc_delta >= ENCODER_COUNTS_PER_STEP || enc_delta <= -ENCODER_COUNTS_PER_STEP) {
			  int steps = enc_delta / ENCODER_COUNTS_PER_STEP;
			  last_encoder_count = (uint16_t)(enc_now - (enc_delta % ENCODER_COUNTS_PER_STEP));

			  if (UI_GetMode() == UI_MODE_FILE_LIST) {
				  /* In list mode the encoder changes role: instead of speed,
				     it pages through the list (LIST_VISIBLE files at a time). */
				  int8_t page_dir = (steps > 0) ? 1 : -1;
				  int    n        = (steps > 0) ? steps : -steps;
				  for (int i = 0; i < n; i++)
					  UI_MoveCursorByPage(page_dir);
			  } else {
				  playback_speed += steps * PLAYBACK_SPEED_STEP;
				  if (playback_speed < PLAYBACK_SPEED_MIN) playback_speed = PLAYBACK_SPEED_MIN;
				  if (playback_speed > PLAYBACK_SPEED_MAX) playback_speed = PLAYBACK_SPEED_MAX;

				  uint32_t new_period = (uint32_t)(TIM2_BASE_PERIOD / playback_speed + 0.5f);
				  __HAL_TIM_SET_AUTORELOAD(&htim2, new_period);
			  }
		  }
	  }

	  // Speed reset button (PA0/SPEED_RST, EXTI0) - resets playback speed to 1.0x
	  if(speed_reset_button){
		  speed_reset_button = 0;
		  playback_speed = 1.0f;
		  __HAL_TIM_SET_AUTORELOAD(&htim2, TIM2_BASE_PERIOD);
	  }

	  // Loop ON/OFF switch (PA2, persistent state - polled every loop iteration)
	  VGM_SetLoopMode(HAL_GPIO_ReadPin(LOOP_SW_GPIO_Port, LOOP_SW_Pin) == GPIO_PIN_RESET);

	  // Shuffle ON/OFF switch (PA3, persistent state - polled every loop iteration).
	  // Shuffle_SetEnabled internally handles the OFF->ON transition and building the bag.
	  Shuffle_SetEnabled(HAL_GPIO_ReadPin(SHUFFLE_SW_GPIO_Port, SHUFFLE_SW_Pin) == GPIO_PIN_RESET,
	                      current_song_index);

	  // Pause / Resume (EXTI14)
	  if(timerRunning)
	  {
		if(paused){
			if (VGM_IsYM2151Active())
				YM_System_All_Unmute();
			if (VGM_IsSN76489Active())
				SN76489_UnmuteAll();
		  	HAL_TIM_Base_Start_IT(&htim2);
			paused = 0;
		}
	  }
	  else
	  {
		 if(!paused){
			HAL_TIM_Base_Stop_IT(&htim2);
			YM_System_All_Mute();
			SN76489_MuteAll();
		  	paused = 1;
		 }
	  }

	  // File list <-> Now Playing transition / selection confirm (EXTI7)
	  if(list_button){
		  list_button = 0;
		  if(UI_GetMode() == UI_MODE_NOW_PLAYING){
			  UI_EnterFileList(current_song_index);
		  } else {
			  int selected = UI_GetSelectedIndex();
			  UI_ExitFileList();
			  song_change = (int8_t)(selected - current_song_index);
		  }
	  }

	  // If loop is off and the song finished naturally, auto-advance to the next song
	  // (don't consume the flag while in list mode, so we don't move the cursor by accident)
	  if(UI_GetMode() != UI_MODE_FILE_LIST && VGM_TrackFinished()){
		  song_change = 1;
	  }

	  // Song change / list navigation (EXTI12 / EXTI10)
	  if(song_change != 0){
		  if(UI_GetMode() == UI_MODE_FILE_LIST){
			  UI_MoveCursor(song_change);
			  song_change = 0;
		  } else {
		  HAL_TIM_Base_Stop_IT(&htim2);
		  YM_System_All_Mute();
		  SN76489_MuteAll();

		  int step = song_change;
		  song_change = 0;

		  if(Shuffle_IsEnabled() && (step == 1 || step == -1)){
			  /* Advancing directly via button/auto-advance: continue from the shuffle bag */
			  current_song_index = (step > 0) ? Shuffle_Next() : Shuffle_Prev();
		  } else {
			  /* Manual selection from the file list (delta not 1) or shuffle off */
			  current_song_index += step;

			  if(current_song_index < 0){
				  int last = FAT32_CountFiles() - 1;
				  current_song_index = (last < 0) ? 0 : last; /* wrapped past the start: jump to the end */
			  } else {
				  FAT32_File probe;
				  char probe_name[13];
				  if(FAT32_FindFileByIndex((uint32_t)current_song_index, &probe, probe_name) != 0)
					  current_song_index = 0; /* end of the list: wrap to the start */
			  }

			  if(Shuffle_IsEnabled())
				  Shuffle_Reseed(current_song_index); /* manual selection: continue the bag from this song */
		  }

		  Storage_SaveLastSongIndex(current_song_index);

		  if(FAT32_FindFileByIndex(current_song_index, &vgm_file, vgm_filename) == 0){
			  vgm_valid = VGM_LoadFromSD(&vgm_file);
		  }
		  /* Program the new song's clock into the SI5351 FIRST (inside
		     VGM_ParseHeader), THEN do the chip resets - otherwise if a chip
		     was disabled in the previous song (SI5351 output disabled), these
		     Init() writes would happen without a clock/invalid. */
		  VGM_ParseHeader(NULL);
		  YM2151_Init();
		  SN76489_Init();
		  VGM_Init();

		  if(!paused){
			  if (VGM_IsYM2151Active())
				  YM_System_All_Unmute();
			  if (VGM_IsSN76489Active())
				  SN76489_UnmuteAll();
			  HAL_TIM_Base_Start_IT(&htim2);
		  }
		  }
	  }

	  // Continuous navigation while a nav button is held in the file list
	  if(UI_GetMode() == UI_MODE_FILE_LIST){
		  uint8_t next_held = (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_10) == GPIO_PIN_RESET);
		  uint8_t prev_held = (HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_12) == GPIO_PIN_RESET);
		  int8_t  held_dir  = next_held ? 1 : (prev_held ? -1 : 0);
		  uint32_t now = HAL_GetTick();

		  if(held_dir == 0){
			  nav_hold_dir = 0;
		  } else if(held_dir != nav_hold_dir){
			  nav_hold_dir    = held_dir;
			  nav_hold_start  = now;
			  nav_last_repeat = now;
		  } else if(now - nav_hold_start >= NAV_REPEAT_DELAY_MS &&
		            now - nav_last_repeat >= NAV_REPEAT_INTERVAL_MS){
			  UI_MoveCursor(held_dir);
			  nav_last_repeat = now;
		  }
	  } else {
		  nav_hold_dir = 0;
	  }
	  /*solo1 = !(HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_7) | HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_8)| HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_9)| HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_5)) | HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_6);
	  solo2 = !(HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_6) | HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_8)| HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_9)| HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_5)) | HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_7);
	  solo3 = !(HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_6) | HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_7)| HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_9)| HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_5)) | HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_8);
	  solo4 = !(HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_6) | HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_7)| HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_8)| HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_5)) | HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_9);
	  solo5 = !(HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_6) | HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_7)| HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_8)| HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_9)) | HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_5);*/
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the programming delay
  */
  __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_1);
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI1|RCC_PERIPHCLK_SPI2;
  PeriphClkInitStruct.PLL2.PLL2Source = RCC_PLL2_SOURCE_HSI;
  PeriphClkInitStruct.PLL2.PLL2M = 4;
  PeriphClkInitStruct.PLL2.PLL2N = 32;
  PeriphClkInitStruct.PLL2.PLL2P = 81;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2_VCIRANGE_3;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2_VCORANGE_WIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.PLL2.PLL2ClockOut = RCC_PLL2_DIVP;
  PeriphClkInitStruct.Spi1ClockSelection = RCC_SPI1CLKSOURCE_PLL2P;
  PeriphClkInitStruct.Spi2ClockSelection = RCC_SPI2CLKSOURCE_PLL2P;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief DAC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC1_Init(void)
{

  /* USER CODE BEGIN DAC1_Init 0 */

  /* USER CODE END DAC1_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC1_Init 1 */

  /* USER CODE END DAC1_Init 1 */

  /** DAC Initialization
  */
  hdac1.Instance = DAC1;
  if (HAL_DAC_Init(&hdac1) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT1 config
  */
  sConfig.DAC_HighFrequency = DAC_HIGH_FREQUENCY_INTERFACE_MODE_DISABLE;
  sConfig.DAC_DMADoubleDataMode = DISABLE;
  sConfig.DAC_SignedFormat = DISABLE;
  sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  sConfig.DAC_Trigger = DAC_TRIGGER_T4_TRGO;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_EXTERNAL;
  sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
  if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT2 config
  */
  if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC1_Init 2 */

  /* USER CODE END DAC1_Init 2 */

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
  hi2c1.Init.Timing = 0x10707DBC;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief ICACHE Initialization Function
  * @param None
  * @retval None
  */
static void MX_ICACHE_Init(void)
{

  /* USER CODE BEGIN ICACHE_Init 0 */

  /* USER CODE END ICACHE_Init 0 */

  /* USER CODE BEGIN ICACHE_Init 1 */

  /* USER CODE END ICACHE_Init 1 */

  /** Enable instruction cache in 1-way (direct mapped cache)
  */
  if (HAL_ICACHE_ConfigAssociativityMode(ICACHE_1WAY) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_ICACHE_Enable() != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ICACHE_Init 2 */

  /* USER CODE END ICACHE_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 0x7;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  hspi1.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
  hspi1.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 0x7;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  hspi2.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
  hspi2.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

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

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1450;
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
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 8;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim3, &sConfig) != HAL_OK)
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
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 63;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 124;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, YM_D0_Pin|YM_D1_Pin|YM_D2_Pin|YM_D3_Pin
                          |YM_D4_Pin|YM_D5_Pin|YM_D6_Pin|YM_D7_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, CE_Pin|WE_Pin|TFT_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SD_CS_Pin|TFT_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, D0_Pin|D1_Pin|D2_Pin|D3_Pin
                          |D4_Pin|D5_Pin|D6_Pin|D7_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, YM_IC_Pin|YM_A0_Pin|YM_WR_Pin|YM_RD_Pin
                          |YM_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : YM_D0_Pin YM_D1_Pin YM_D2_Pin YM_D3_Pin
                           YM_D4_Pin YM_D5_Pin YM_D6_Pin YM_D7_Pin */
  GPIO_InitStruct.Pin = YM_D0_Pin|YM_D1_Pin|YM_D2_Pin|YM_D3_Pin
                          |YM_D4_Pin|YM_D5_Pin|YM_D6_Pin|YM_D7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : SPEED_RST_Pin */
  GPIO_InitStruct.Pin = SPEED_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SPEED_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LOOP_SW_Pin SHUFFLE_SW_Pin */
  GPIO_InitStruct.Pin = LOOP_SW_Pin|SHUFFLE_SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : CE_Pin WE_Pin */
  GPIO_InitStruct.Pin = CE_Pin|WE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PF12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : PD10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PG7 PG14 */
  GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pin : TFT_RST_Pin */
  GPIO_InitStruct.Pin = TFT_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TFT_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SD_CS_Pin TFT_CS_Pin */
  GPIO_InitStruct.Pin = SD_CS_Pin|TFT_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : D0_Pin D1_Pin D2_Pin D3_Pin
                           D4_Pin D5_Pin D6_Pin D7_Pin */
  GPIO_InitStruct.Pin = D0_Pin|D1_Pin|D2_Pin|D3_Pin
                          |D4_Pin|D5_Pin|D6_Pin|D7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : YM_IC_Pin YM_A0_Pin YM_WR_Pin YM_RD_Pin
                           YM_CS_Pin */
  GPIO_InitStruct.Pin = YM_IC_Pin|YM_A0_Pin|YM_WR_Pin|YM_RD_Pin
                          |YM_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pin : TFT_DC_Pin */
  GPIO_InitStruct.Pin = TFT_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TFT_DC_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI7_IRQn);

  HAL_NVIC_SetPriority(EXTI10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI10_IRQn);

  HAL_NVIC_SetPriority(EXTI12_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI12_IRQn);

  HAL_NVIC_SetPriority(EXTI14_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI14_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* Loop ON/OFF switch: persistent state, not an interrupt - polled in the main loop. */
  GPIO_InitStruct.Pin  = LOOP_SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(LOOP_SW_GPIO_Port, &GPIO_InitStruct);

  /* Shuffle ON/OFF switch: likewise persistent state, polled. */
  GPIO_InitStruct.Pin  = SHUFFLE_SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SHUFFLE_SW_GPIO_Port, &GPIO_InitStruct);
  /* USER CODE END MX_GPIO_Init_2 */
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
