#ifndef _YM2151_H_
#define _YM2151_H_
#endif

#include "main.h"

#define CLOCK_FREQ 4000000U

#define YM2151_REG_KEYON_OFF    0x08
#define YM2151_REG_PAN_FL_CON   0x20 // channels 0x20 - 0x27
#define YM2151_REG_TOTAL_LEVEL  0x60 // operators 0x60 - 0x7F

void YM2151_Init(void);
void YM2151_Mute_KeyOff(void);
void YM2151_Mute_TotalLevel(void);
void YM2151_Mute_OutputDisable(void);
void YM_System_All_Mute(void);
void YM2151_Unmute_KeyOn(void);
void YM2151_Unmute_TotalLevel(void);
void YM2151_Unmute_OutputEnable(void);
void YM_System_All_Unmute(void);
void YM2151_Delay(uint8_t delay);
void YM2151_SetPins(uint8_t byte);
void YM2151_Write(uint8_t addr, uint8_t data);

/* channel: 0-7. Returns: 0 = all operators off, >0 = at least one is on. */
uint8_t YM2151_GetKeyOn(uint8_t channel);

/* Reports the real master clock programmed via the SI5351 - the BUSY wait
   duration in YM2151_Write is scaled accordingly (not a fixed 4MHz assumption). */
void YM2151_SetMasterClockHz(uint32_t hz);
