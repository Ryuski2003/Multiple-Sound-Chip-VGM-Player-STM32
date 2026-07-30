#ifndef VGM_PLAYER_H
#define VGM_PLAYER_H

#include <stdint.h>
#include "fat32.h"

#define VGM_CMD_SN76489 0x50  // SN76489 write command
#define VGM_CMD_YM2151 0x54  // YM2151 write command
#define VGM_CMD_WAIT_N 0x61  // wait n samples (16-bit)
#define VGM_CMD_WAIT_60 0x62 // wait 1/60 second (735 samples)
#define VGM_CMD_WAIT_50 0x63 // wait 1/50 second (882 samples)
#define VGM_CMD_END    0x66  // end of data
#define VGM_CMD_WAIT_1 0x70  // wait 1 sample (0x70-0x7F range, n+1)
#define VGM_CMD_MSM6295 0xB8 // MSM6295 write command

void VGM_ParseHeader(const uint8_t *vgm_data);
void VGM_Init(void);
void VGM_Update(void);

/* Returns 1 if the corresponding chip's clock in the current song's header is
   nonzero (i.e. the chip is used). */
int  VGM_IsSN76489Active(void);
int  VGM_IsYM2151Active(void);

/* Elapsed/total duration of the current song (seconds). Total comes from the
   total_samples field in the header; reset by VGM_Init(), elapsed
   incremented by each VGM_Update() call (= 1 sample). */
uint32_t VGM_GetElapsedSeconds(void);
uint32_t VGM_GetTotalSeconds(void);

/* Whether the file has a loop point, and if so, the position in time (in
   seconds) it wraps back to when looping - for marking on the progress bar. */
int      VGM_HasLoopPoint(void);
uint32_t VGM_GetLoopStartSeconds(void);

/* Whether loop mode is on (the loopmode variable). */
int VGM_IsLoopModeOn(void);
void VGM_SetLoopMode(int on);

/* Returns 1 if the song finished naturally while loop was off (clears the flag on read). */
int VGM_TrackFinished(void);

/* Writes the song title from the GD3 tag (Track name - English, translated to
   ASCII) into out; leaves out[0]='\0' if the tag is absent/empty. out must be
   at least max_len bytes. */
void VGM_GetTrackTitle(char *out, int max_len);

/* Loads a VGM from the SD card. Returns 1 on success, 0 if the file is too large. */
int  VGM_LoadFromSD(const FAT32_File *f);

#endif
