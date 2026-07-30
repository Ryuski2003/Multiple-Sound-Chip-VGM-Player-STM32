#include "ui.h"
#include "ILI9341.h"
#include "vgm.h"
#include "fat32.h"
#include "shuffle.h"
#include "SN76489.h"
#include "YM2151.h"
#include "msm6295.h"
#include <stdio.h>
#include <string.h>

extern char  vgm_filename[13];
extern uint8_t paused;
extern float playback_speed;

#define UI_BG     ILI9341_BLACK
#define UI_FG     ILI9341_WHITE
#define UI_ACCENT ILI9341_YELLOW

#define UI_TITLE_Y    20  /* GD3 song title (or the file name if absent), scale 2 */
#define UI_FILENAME_Y 38  /* raw file name, always shown, scale 1 */
#define UI_TIME_Y     50
#define UI_BAR_X      10
#define UI_BAR_Y      80
#define UI_BAR_W      220
#define UI_BAR_H      10
#define UI_STATUS_Y   100
#define UI_LOOP_Y     115
#define UI_SHUFFLE_Y  130

#define UI_ACT_SN_Y   145 /* SN76489 channel activity (4 boxes) */
#define UI_ACT_YM_Y   163 /* YM2151 channel activity (8 boxes) */
#define UI_ACT_OK_Y   181 /* MSM6295 (OKI ADPCM) channel activity (4 boxes) */
#define ACT_BOX_X     40
#define ACT_BOX_W     14
#define ACT_BOX_H     14
#define ACT_BOX_GAP   3

#define UI_SPEED_Y    200 /* playback speed set via the encoder */

#define LIST_ROW_H    16
#define LIST_TOP_Y    24
#define LIST_VISIBLE  ((ILI9341_HEIGHT - LIST_TOP_Y) / LIST_ROW_H)

static char     last_filename[13];
static char     last_title[41];
static uint32_t last_elapsed;
static uint32_t last_total;
static uint8_t  last_paused;
static uint8_t  last_loopmode;
static uint8_t  last_shufflemode;
static uint8_t  last_sn_active[4];
static uint8_t  last_ym_active[8];
static uint8_t  last_ok_active[4];
static int      last_speed_x100 = -1;
static uint8_t  first_draw;

static UI_Mode mode = UI_MODE_NOW_PLAYING;
static int     cursor_index = 0;
static int     page_start   = 0;

/* Name cache for the file list screen: UI_EnterFileList() fills it in a
   SINGLE PASS, so the SD card is never scanned again while drawing/navigating
   pages. */
#define FILE_CACHE_MAX 256
static char file_name_cache[FILE_CACHE_MAX][13];
static int  file_cache_count = 0;

void UI_Init(void) {
    ILI9341_FillScreen(UI_BG);
    last_filename[0] = '\0';
    last_title[0]    = '\0';
    last_elapsed  = 0xFFFFFFFF;
    last_total    = 0xFFFFFFFF;
    last_paused   = 0xFF;
    last_loopmode    = 0xFF;
    last_shufflemode = 0xFF;
    for (int i = 0; i < 4; i++) last_sn_active[i] = 0xFF;
    for (int i = 0; i < 8; i++) last_ym_active[i] = 0xFF;
    for (int i = 0; i < 4; i++) last_ok_active[i] = 0xFF;
    last_speed_x100  = -1;
    first_draw       = 1;
}

#define LOOP_MARK_W 2

static void draw_progress_bar(uint32_t elapsed, uint32_t total) {
    uint16_t fill_w = 0;
    if (total > 0) {
        uint32_t e = elapsed > total ? total : elapsed; /* elapsed>total can happen during a loop */
        fill_w = (uint16_t)(((uint64_t)e * UI_BAR_W) / total);
    }
    ILI9341_FillRect(UI_BAR_X, UI_BAR_Y, fill_w, UI_BAR_H, UI_ACCENT);
    ILI9341_FillRect(UI_BAR_X + fill_w, UI_BAR_Y, UI_BAR_W - fill_w, UI_BAR_H, UI_BG);

    /* Mark on the bar the position in time where the loop wraps back to */
    if (total > 0 && VGM_HasLoopPoint()) {
        uint32_t loop_s = VGM_GetLoopStartSeconds();
        uint16_t loop_x = (uint16_t)(((uint64_t)loop_s * UI_BAR_W) / total);
        if (loop_x > UI_BAR_W - LOOP_MARK_W) loop_x = UI_BAR_W - LOOP_MARK_W;
        ILI9341_FillRect(UI_BAR_X + loop_x, UI_BAR_Y, LOOP_MARK_W, UI_BAR_H, ILI9341_CYAN);
    }
}

/* Without doing real audio analysis, shows a simple active/inactive box per
   channel based on registers we've already written (SN76489 attenuation
   shadow, YM2151 key-on shadow). */
static void draw_activity(void) {
    if (first_draw) {
        ILI9341_DrawString(10, UI_ACT_SN_Y + 3, "SN", UI_FG, UI_BG, 1);
        ILI9341_DrawString(10, UI_ACT_YM_Y + 3, "YM", UI_FG, UI_BG, 1);
        ILI9341_DrawString(10, UI_ACT_OK_Y + 3, "OK", UI_FG, UI_BG, 1);
    }

    for (int ch = 0; ch < 4; ch++) {
        uint8_t active = SN76489_GetAttenuation((uint8_t)ch) < 15;
        if (active != last_sn_active[ch]) {
            last_sn_active[ch] = active;
            uint16_t x = ACT_BOX_X + ch * (ACT_BOX_W + ACT_BOX_GAP);
            ILI9341_FillRect(x, UI_ACT_SN_Y, ACT_BOX_W, ACT_BOX_H, active ? ILI9341_GREEN : UI_BG);
        }
    }

    for (int ch = 0; ch < 8; ch++) {
        uint8_t active = YM2151_GetKeyOn((uint8_t)ch) != 0;
        if (active != last_ym_active[ch]) {
            last_ym_active[ch] = active;
            uint16_t x = ACT_BOX_X + ch * (ACT_BOX_W + ACT_BOX_GAP);
            ILI9341_FillRect(x, UI_ACT_YM_Y, ACT_BOX_W, ACT_BOX_H, active ? ILI9341_CYAN : UI_BG);
        }
    }

    for (int ch = 0; ch < 4; ch++) {
        uint8_t active = MSM6295_GetChannelPlaying((uint8_t)ch) != 0;
        if (active != last_ok_active[ch]) {
            last_ok_active[ch] = active;
            uint16_t x = ACT_BOX_X + ch * (ACT_BOX_W + ACT_BOX_GAP);
            ILI9341_FillRect(x, UI_ACT_OK_Y, ACT_BOX_W, ACT_BOX_H, active ? ILI9341_YELLOW : UI_BG);
        }
    }
}

void UI_Update(void) {
    if (mode != UI_MODE_NOW_PLAYING) return;

    char title[41];
    VGM_GetTrackTitle(title, sizeof(title));
    if (title[0] == '\0') {
        strncpy(title, vgm_filename, sizeof(title) - 1);
        title[sizeof(title) - 1] = '\0';
    }
    if (first_draw || strcmp(title, last_title) != 0) {
        strcpy(last_title, title);
        ILI9341_FillRect(10, UI_TITLE_Y, ILI9341_WIDTH - 10, 16, UI_BG);
        ILI9341_DrawString(10, UI_TITLE_Y, title, UI_FG, UI_BG, 2);
    }

    if (first_draw || strcmp(vgm_filename, last_filename) != 0) {
        strcpy(last_filename, vgm_filename);
        ILI9341_FillRect(10, UI_FILENAME_Y, ILI9341_WIDTH - 10, 8, UI_BG);
        ILI9341_DrawString(10, UI_FILENAME_Y, vgm_filename, UI_FG, UI_BG, 1);
    }

    uint32_t elapsed = VGM_GetElapsedSeconds();
    uint32_t total    = VGM_GetTotalSeconds();
    if (elapsed != last_elapsed || total != last_total) {
        last_elapsed = elapsed;
        last_total   = total;
        char buf[24];
        snprintf(buf, sizeof(buf), "%02lu:%02lu / %02lu:%02lu",
                 (unsigned long)(elapsed / 60), (unsigned long)(elapsed % 60),
                 (unsigned long)(total   / 60), (unsigned long)(total   % 60));
        ILI9341_FillRect(10, UI_TIME_Y, 200, 16, UI_BG);
        ILI9341_DrawString(10, UI_TIME_Y, buf, UI_FG, UI_BG, 2);
        draw_progress_bar(elapsed, total);
    }

    if (paused != last_paused) {
        last_paused = paused;
        ILI9341_FillRect(10, UI_STATUS_Y, 100, 8, UI_BG);
        ILI9341_DrawString(10, UI_STATUS_Y, paused ? "PAUSED" : "PLAYING", UI_ACCENT, UI_BG, 1);
    }

    uint8_t loop_on = (uint8_t)VGM_IsLoopModeOn();
    if (loop_on != last_loopmode) {
        last_loopmode = loop_on;
        ILI9341_FillRect(10, UI_LOOP_Y, 100, 8, UI_BG);
        ILI9341_DrawString(10, UI_LOOP_Y, loop_on ? "LOOP: ON" : "LOOP: OFF", UI_FG, UI_BG, 1);
    }

    uint8_t shuffle_on = (uint8_t)Shuffle_IsEnabled();
    if (shuffle_on != last_shufflemode) {
        last_shufflemode = shuffle_on;
        ILI9341_FillRect(10, UI_SHUFFLE_Y, 100, 8, UI_BG);
        ILI9341_DrawString(10, UI_SHUFFLE_Y, shuffle_on ? "SHUFFLE: ON" : "SHUFFLE: OFF", UI_FG, UI_BG, 1);
    }

    int speed_x100 = (int)(playback_speed * 100.0f + 0.5f);
    if (speed_x100 != last_speed_x100) {
        last_speed_x100 = speed_x100;
        char sbuf[16];
        snprintf(sbuf, sizeof(sbuf), "SPEED: %d.%02dx", speed_x100 / 100, speed_x100 % 100);
        ILI9341_FillRect(10, UI_SPEED_Y, 120, 8, UI_BG);
        ILI9341_DrawString(10, UI_SPEED_Y, sbuf, UI_FG, UI_BG, 1);
    }

    draw_activity();
    first_draw = 0;
}

UI_Mode UI_GetMode(void) {
    return mode;
}

int UI_GetSelectedIndex(void) {
    return cursor_index;
}

/* Draws a single row of the visible page (reading the file name from the cache). */
static void draw_list_row(int row_slot, int file_index, int is_selected) {
    uint16_t y      = LIST_TOP_Y + (uint16_t)row_slot * LIST_ROW_H;
    uint16_t fg     = is_selected ? ILI9341_BLACK : ILI9341_WHITE;
    uint16_t row_bg = is_selected ? UI_ACCENT : UI_BG;

    ILI9341_FillRect(0, y, ILI9341_WIDTH, LIST_ROW_H, row_bg);
    if (file_index >= 0 && file_index < file_cache_count) {
        ILI9341_DrawString(4, y + 2, file_name_cache[file_index], fg, row_bg, 2);
    }
}

static void draw_full_page(void) {
    ILI9341_FillRect(0, 0, ILI9341_WIDTH, LIST_TOP_Y, UI_BG);
    ILI9341_DrawString(4, 4, "SELECT FILE", ILI9341_CYAN, UI_BG, 1);
    for (int i = 0; i < LIST_VISIBLE; i++) {
        draw_list_row(i, page_start + i, (page_start + i) == cursor_index);
    }
}

void UI_EnterFileList(int current_index) {
    mode = UI_MODE_FILE_LIST;
    file_cache_count = FAT32_CacheAllNames(file_name_cache, FILE_CACHE_MAX);
    cursor_index = current_index < 0 ? 0 : current_index;
    page_start = (cursor_index / LIST_VISIBLE) * LIST_VISIBLE;
    draw_full_page();
}

void UI_ExitFileList(void) {
    mode = UI_MODE_NOW_PLAYING;
    UI_Init(); /* redraw the Now Playing screen from scratch */
}

void UI_MoveCursor(int8_t dir) {
    if (mode != UI_MODE_FILE_LIST || dir == 0) return;

    int new_index = cursor_index + (dir > 0 ? 1 : -1);

    if (new_index < 0) {
        new_index = file_cache_count - 1;
        if (new_index < 0) return; /* no files */
    } else if (new_index >= file_cache_count) {
        new_index = 0; /* end of the list: wrap to the start */
        if (new_index >= file_cache_count) return; /* no files */
    }

    int old_index = cursor_index;
    cursor_index = new_index;

    if (new_index < page_start || new_index >= page_start + LIST_VISIBLE) {
        page_start = (cursor_index / LIST_VISIBLE) * LIST_VISIBLE;
        draw_full_page();
    } else {
        draw_list_row(old_index - page_start, old_index, 0);
        draw_list_row(new_index - page_start, new_index, 1);
    }
}

void UI_MoveCursorByPage(int8_t dir) {
    if (mode != UI_MODE_FILE_LIST || dir == 0) return;

    int total = file_cache_count;
    if (total <= 0) return;

    int new_index = cursor_index + (dir > 0 ? LIST_VISIBLE : -LIST_VISIBLE);
    if (new_index < 0) {
        new_index = total - 1;      /* wrapped past the start: jump to the end */
    } else if (new_index >= total) {
        new_index = 0;              /* end of the list: wrap to the start */
    }

    cursor_index = new_index;
    page_start   = (cursor_index / LIST_VISIBLE) * LIST_VISIBLE;
    draw_full_page(); /* a page jump almost always changes the page anyway */
}
