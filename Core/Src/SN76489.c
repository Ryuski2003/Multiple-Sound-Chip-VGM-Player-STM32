#include "SN76489.h"

extern uint8_t ready;

/* Last attenuation value written per channel (0=loudest, 15=silent).
   Kept as a shadow for the activity indicator without doing real audio analysis. */
static uint8_t atten_shadow[4] = {15, 15, 15, 15};

void SN76489_Init(void){
	HAL_GPIO_WritePin(GPIOC, CE_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, WE_Pin, GPIO_PIN_RESET);
	for (int i = 0; i < 4; i++) atten_shadow[i] = 15;
}

void SN76489_Write(uint8_t byte){
	/* If it's a latch byte (bit7=1) of attenuation type (bit4=1), update the
	   channel's shadow value: bits 6-5 channel (0=T1,1=T2,2=T3,3=Noise), bits 3-0 value. */
	if (byte & 0x80) {
		if (byte & 0x10) {
			uint8_t channel = (byte >> 5) & 0x03;
			atten_shadow[channel] = byte & 0x0F;
		}
	}

	GPIOD->ODR = (GPIOD->ODR & 0xFF00) | (__RBIT(byte) >> 24);
	HAL_GPIO_WritePin(GPIOC, WE_Pin, GPIO_PIN_RESET);
	for(int i=0; i<50; i++)
		__NOP();
	HAL_GPIO_WritePin(GPIOC, WE_Pin, GPIO_PIN_SET);
}

/* channel: 0=Tone1,1=Tone2,2=Tone3,3=Noise. Returns: 0=loudest,15=silent. */
uint8_t SN76489_GetAttenuation(uint8_t channel) {
	return (channel < 4) ? atten_shadow[channel] : 15;
}

void SN76489_MuteAll(void){
	SN76489_Write(TONE1_MUTE);
	SN76489_Write(TONE2_MUTE);
	SN76489_Write(TONE3_MUTE);
	SN76489_Write(NOISE_MUTE);
}

void SN76489_UnmuteAll(void){
	SN76489_Write(TONE1_ATTEN);
	SN76489_Write(TONE2_ATTEN);
	SN76489_Write(TONE3_ATTEN);
	SN76489_Write(NOISE_ATTEN);
}
