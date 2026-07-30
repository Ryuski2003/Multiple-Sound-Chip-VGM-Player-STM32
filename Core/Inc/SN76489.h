#ifndef _SN76489_H_
#define _SN76489_H_
#endif

#include "main.h"

#define CLOCK_FREQ 4000000U

//------Channel Bits------
#define TONE1 0b000
#define TONE2 0b010
#define TONE3 0b100
#define NOISE 0b110

//------Mute Channels------
#define TONE1_MUTE 0x9F
#define TONE2_MUTE 0xBF
#define TONE3_MUTE 0xDF
#define NOISE_MUTE 0xFF

//------Channel Attenuation Control------
#define TONE1_ATTEN 0x90
#define TONE2_ATTEN 0xB0
#define TONE3_ATTEN 0xD0
#define NOISE_ATTEN 0xF0

//------Attenuation Values------
#define ATTEN_0DB 0b0000 //Unmutes channel.
#define ATTEN_2DB 0b0001
#define ATTEN_4DB 0b0010
#define ATTEN_6DB 0b0011

#define ATTEN_8DB 0b0100
#define ATTEN_10DB 0b0101
#define ATTEN_12DB 0b0110
#define ATTEN_14DB 0b0111

#define ATTEN_16DB 0b1000
#define ATTEN_18DB 0b1001
#define ATTEN_20DB 0b1010
#define ATTEN_22DB 0b1011

#define ATTEN_24DB 0b1100
#define ATTEN_26DB 0b1101
#define ATTEN_28DB 0b1110
#define ATTEN_30DB 0b1111 //Mutes channel.

void SN76489_Init(void);
void SN76489_Write(uint8_t byte);
void SN76489_MuteAll(void);
void SN76489_UnmuteAll(void);

/* channel: 0=Tone1,1=Tone2,2=Tone3,3=Noise. Returns: 0=loudest,15=silent. */
uint8_t SN76489_GetAttenuation(uint8_t channel);
