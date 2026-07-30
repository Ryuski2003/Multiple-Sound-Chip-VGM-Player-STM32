#ifndef _SI5351_H_
#define _SI5351_H_

#include "main.h"

#define SI5351_CLK_SN76489 0
#define SI5351_CLK_YM2151  1

void SI5351_Init(void);
void SI5351_SetFrequency(uint8_t clk_num, uint32_t freq_hz);
void SI5351_OutputEnable(uint8_t clk_num, uint8_t enable);

#endif
