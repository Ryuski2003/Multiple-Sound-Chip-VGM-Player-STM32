#ifndef _UI_H_
#define _UI_H_

#include <stdint.h>

typedef enum { UI_MODE_NOW_PLAYING = 0, UI_MODE_FILE_LIST } UI_Mode;

/* Draws the static layout once, resets tracking of which fields have changed. */
void UI_Init(void);

/* Called every iteration of the main loop; only redraws fields that changed.
   Only redraws anything while in UI_MODE_NOW_PLAYING mode. */
void UI_Update(void);

UI_Mode UI_GetMode(void);

/* When EXTI7 (mode switch) fires: Now Playing -> File List. */
void UI_EnterFileList(int current_index);

/* When EXTI7 (selection confirm) fires: File List -> Now Playing. */
void UI_ExitFileList(void);

/* EXTI10/12: moves the cursor up/down while in File List mode
   (dir>0 down, dir<0 up). No effect in other modes. */
void UI_MoveCursor(int8_t dir);

/* Moves the cursor a full PAGE (LIST_VISIBLE files) up/down at once while in
   File List mode, via the encoder - for fast navigation. No effect in other
   modes. */
void UI_MoveCursorByPage(int8_t dir);

/* Index of the file currently selected in the list (needed by main.c to confirm). */
int UI_GetSelectedIndex(void);

#endif
