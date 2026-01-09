#include "SN76489.h"

extern TIM_HandleTypeDef htim3;

extern uint8_t ready;

uint16_t tenBitData;
uint8_t firstByte;
uint8_t secondByte;

void sendByte(uint8_t byte){
	HAL_GPIO_WritePin(GPIOC, WE_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, D0_Pin, (byte & 128)   ? GPIO_PIN_SET: GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, D1_Pin, (byte & 64)   	? GPIO_PIN_SET: GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, D2_Pin, (byte & 32)   	? GPIO_PIN_SET: GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, D3_Pin, (byte & 16)   	? GPIO_PIN_SET: GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, D4_Pin, (byte & 8)  	? GPIO_PIN_SET: GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, D5_Pin, (byte & 4)  	? GPIO_PIN_SET: GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, D6_Pin, (byte & 2)  	? GPIO_PIN_SET: GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, D7_Pin, (byte & 1) 	? GPIO_PIN_SET: GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, WE_Pin, GPIO_PIN_RESET);
	for(int i = 0; i < 512; i++);
	HAL_GPIO_WritePin(GPIOC, WE_Pin, GPIO_PIN_SET);
}
void setNote(uint16_t frequency, uint8_t toneChannel){
	firstByte = 0x80 | (toneChannel << 4);
	secondByte = 0x00;
	if(frequency != 0){

		tenBitData = (CLOCK_FREQ/(32*frequency)); //10-bit Note Data
		firstByte = (tenBitData & 0b1111) | (toneChannel << 4 | 0b10000000);
		secondByte = (((tenBitData >> 4) & 0x3F));
	}
	sendByte(firstByte);
	sendByte(secondByte);
}
