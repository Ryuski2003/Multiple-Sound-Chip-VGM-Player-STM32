#ifndef MSM6295_H
#define MSM6295_H

#include <stdint.h>

void    MSM6295_Init(uint32_t clock_hz, uint8_t pin7, uint32_t output_rate_hz);
void    MSM6295_LoadROM(const uint8_t *data, uint32_t data_size,
                        uint32_t start_offset, uint32_t rom_total_size);
void    MSM6295_Write(uint8_t data);
int16_t MSM6295_Update(void);

/* channel: 0-3. Returns: whether the channel is currently playing an ADPCM sample (1/0). */
uint8_t MSM6295_GetChannelPlaying(uint8_t channel);

#endif
