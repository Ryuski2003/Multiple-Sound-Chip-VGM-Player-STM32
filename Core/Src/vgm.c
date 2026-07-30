#include <stdint.h>
#include <string.h>
#include "vgm.h"
#include "SN76489.h"
#include "YM2151.h"
#include "msm6295.h"
#include "fat32.h"
#include "SI5351.h"
#include "vgz.h"

static inline uint32_t read_u32_le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}

typedef struct {
    uint32_t vgm_tag;
    uint32_t eof_offset;
    uint32_t version;
    uint32_t sn_clock;
    uint32_t ym2151_clock;
    uint32_t loop_offset;
    uint32_t loop_samples;
    uint32_t total_samples;
    uint32_t data_offset;
    uint32_t msm6295_clock;
    uint32_t gd3_offset;
} VGM_Header;

#define VGM_SAMPLE_RATE_HZ 44100UL

VGM_Header header;
uint8_t loopmode = 1;

/* --- Data source: flash (default) or the RAM buffer loaded from the SD card --- */

extern const uint8_t vgm_data_start[];
extern const uint8_t vgm_data_end[];

/* 512 KB RAM buffer - for loading from the SD card (the inflated VGM data lives here) */
static uint8_t vgm_ram_buf[512 * 1024];

/* Small buffer that temporarily holds the compressed (.vgz) file. Real
   chiptune VGMs are usually well under this before inflating; if it's
   exceeded the file is rejected (below, inside VGM_LoadFromSD). */
static uint8_t vgz_stage_buf[64 * 1024];

static const uint8_t *vgm      = vgm_data_start;
static uint32_t       vgm_size = 0;  /* set before VGM_ParseHeader is called */

int VGM_LoadFromSD(const FAT32_File *f)
{
    uint8_t magic[2];
    if (FAT32_ReadBytes(f, 0, magic, 2) != 2)
        return 0;

    if (VGZ_IsGzip(magic, 2)) {
        /* .vgz: gzip-compressed - read into the small staging buffer first,
           then inflate directly into vgm_ram_buf. */
        if (f->size > sizeof(vgz_stage_buf))
            return 0;

        int got = FAT32_ReadBytes(f, 0, vgz_stage_buf, f->size);
        if (got != (int)f->size)
            return 0;

        uint32_t inflated = VGZ_Inflate(vgz_stage_buf, f->size, vgm_ram_buf, sizeof(vgm_ram_buf));
        if (inflated == 0)
            return 0;

        vgm_size = inflated;
    } else {
        if (f->size > sizeof(vgm_ram_buf))
            return 0;

        int got = FAT32_ReadBytes(f, 0, vgm_ram_buf, f->size);
        if (got != (int)f->size)
            return 0;

        vgm_size = f->size;
    }

    /* "Vgm " magic check (on the inflated data) */
    if (vgm_ram_buf[0] != 'V' || vgm_ram_buf[1] != 'g' ||
        vgm_ram_buf[2] != 'm' || vgm_ram_buf[3] != ' ')
        return 0;

    vgm = vgm_ram_buf;
    return 1;
}

/* ------------------------------------------------------------------ */

void VGM_ParseHeader(const uint8_t *vgm_data) {
    if (vgm_data == NULL) vgm_data = vgm;  /* NULL = use the current source */
    header.vgm_tag      = read_u32_le(&vgm_data[0x00]);
    header.version      = read_u32_le(&vgm_data[0x08]);
    header.sn_clock     = read_u32_le(&vgm_data[0x0C]);
    header.ym2151_clock = read_u32_le(&vgm_data[0x30]);
    header.total_samples = read_u32_le(&vgm_data[0x18]);

    uint32_t loop_val = read_u32_le(&vgm_data[0x1C]);
    header.loop_offset = loop_val ? (0x1C + loop_val) : 0;
    header.loop_samples = read_u32_le(&vgm_data[0x20]);

    uint32_t gd3_val = read_u32_le(&vgm_data[0x14]);
    header.gd3_offset = gd3_val ? (0x14 + gd3_val) : 0;

    uint32_t data_val = read_u32_le(&vgm_data[0x34]);
    header.data_offset = (header.version < 0x150 || data_val == 0) ? 0x40 : (0x34 + data_val);

    uint8_t pin7 = 1;  /* CPS1: OKIM6295 pin7 HIGH = /132 (~7576 Hz) */
    if (header.version >= 0x151) {
        /* VGM 1.61+: 0x9C = OKIM6295 clock; bit31=1 overrides to /132, bit31=0 keeps default */
        uint32_t raw = read_u32_le(&vgm_data[0x9C]);
        if (raw >> 31)
            pin7 = 1;
        header.msm6295_clock = raw & 0x3FFFFFFF;
    }
    if (header.msm6295_clock == 0)
        header.msm6295_clock = 1000000;

    /* Also update vgm_size here (in flash mode) */
    if (vgm == vgm_data_start)
        vgm_size = (uint32_t)(vgm_data_end - vgm_data_start);

    MSM6295_Init(header.msm6295_clock, pin7, 8000);

    /* Clock 0 = this chip isn't used in the VGM file -> mute it immediately
       to cut off any sound left over from the previous song. VGM_Update also
       skips writing to this chip. */
    if (header.sn_clock == 0)
        SN76489_MuteAll();
    if (header.ym2151_clock == 0)
        YM_System_All_Mute();

    /* SI5351: program each chip's real master clock according to this
       song's header value, so pitch is correct for every VGM. If the clock
       is 0, disable the output entirely (the chip is already muted above). */
    if (header.sn_clock != 0)
        SI5351_SetFrequency(SI5351_CLK_SN76489, header.sn_clock);
    else
        SI5351_OutputEnable(SI5351_CLK_SN76489, 0);

    if (header.ym2151_clock != 0) {
        SI5351_SetFrequency(SI5351_CLK_YM2151, header.ym2151_clock);
        /* Scale the BUSY wait duration in YM2151_Write against this real
           clock - the fixed 4MHz assumption no longer holds. */
        YM2151_SetMasterClockHz(header.ym2151_clock);
    } else {
        SI5351_OutputEnable(SI5351_CLK_YM2151, 0);
    }
}

int VGM_IsSN76489Active(void) {
    return header.sn_clock != 0;
}

int VGM_IsYM2151Active(void) {
    return header.ym2151_clock != 0;
}

volatile uint16_t wait_samples = 0;
volatile uint32_t elapsed_samples = 0;
static volatile uint8_t track_finished = 0;
uint32_t pc;
uint8_t cmd, cmd1, cmd2, cmd3;

void VGM_Init(void)
{
    pc = header.data_offset;
    wait_samples = 0;
    elapsed_samples = 0;
}

/* Returns 1 when the song finished naturally while loop was off (and clears
   the flag); the main loop handles this the same way as a "next song" command. */
int VGM_TrackFinished(void) {
    if (track_finished) {
        track_finished = 0;
        return 1;
    }
    return 0;
}

int VGM_IsLoopModeOn(void) {
    return loopmode != 0;
}

void VGM_SetLoopMode(int on) {
    loopmode = on ? 1 : 0;
}

uint32_t VGM_GetElapsedSeconds(void) {
    return elapsed_samples / VGM_SAMPLE_RATE_HZ;
}

uint32_t VGM_GetTotalSeconds(void) {
    return header.total_samples / VGM_SAMPLE_RATE_HZ;
}

int VGM_HasLoopPoint(void) {
    return header.loop_offset != 0;
}

/* The position in time (on the total_samples timeline) that playback wraps
   back to when looping - for marking the loop point on the progress bar. */
uint32_t VGM_GetLoopStartSeconds(void) {
    if (header.loop_offset == 0 || header.loop_samples >= header.total_samples)
        return 0;
    return (header.total_samples - header.loop_samples) / VGM_SAMPLE_RATE_HZ;
}

/* First string in the GD3 block (Track name - English) - UTF-16LE,
   null-terminated. Replaces non-ASCII characters with '?'; our small 5x7
   font only supports printable ASCII anyway. Leaves out[0]='\0' if the tag
   is absent or empty. */
void VGM_GetTrackTitle(char *out, int max_len) {
    out[0] = '\0';
    if (max_len <= 0 || header.gd3_offset == 0) return;
    if (header.gd3_offset + 12 > vgm_size) return;
    if (vgm[header.gd3_offset]     != 'G' || vgm[header.gd3_offset + 1] != 'd' ||
        vgm[header.gd3_offset + 2] != '3' || vgm[header.gd3_offset + 3] != ' ')
        return;

    uint32_t pos = header.gd3_offset + 12; /* after "Gd3 " + version(4) + length(4) */
    int i = 0;
    while (i < max_len - 1 && pos + 1 < vgm_size) {
        uint16_t code = (uint16_t)vgm[pos] | ((uint16_t)vgm[pos + 1] << 8);
        pos += 2;
        if (code == 0) break; /* end of string */
        out[i++] = (code >= 0x20 && code < 0x7F) ? (char)code : '?';
    }
    out[i] = '\0';
}

void VGM_Update(void) {
    if (vgm_size == 0) return;  /* VGM not loaded yet */

    elapsed_samples++;

    if (wait_samples > 0) {
        if (--wait_samples > 0)
            return;
    }

    while (wait_samples == 0) {
        if (pc >= vgm_size) {
            VGM_Init();
            return;
        }

        cmd  = vgm[pc];
        cmd1 = vgm[pc + 1];
        cmd2 = vgm[pc + 2];
        cmd3 = vgm[pc + 3];

        switch (cmd) {
            case VGM_CMD_SN76489:
                if (header.sn_clock != 0)
                    SN76489_Write(vgm[pc + 1]);
                pc += 2;
                break;

            case VGM_CMD_YM2151:
                if (header.ym2151_clock != 0)
                    YM2151_Write(vgm[pc + 1], vgm[pc + 2]);
                pc += 3;
                break;

            case VGM_CMD_WAIT_N:
                wait_samples = vgm[pc + 1] | (vgm[pc + 2] << 8);
                pc += 3;
                break;

            case VGM_CMD_WAIT_60:
                wait_samples = 735;
                pc += 1;
                break;

            case VGM_CMD_WAIT_50:
                wait_samples = 882;
                pc += 1;
                break;

            case 0xB8:
                MSM6295_Write(vgm[pc + 2]);
                pc += 3;
                break;

            case 0x70 ... 0x7F:
                wait_samples = (cmd & 0x0F) + 1;
                pc += 1;
                break;

            case 0x67:
                {
                    uint8_t  blk_type    = vgm[pc + 2];
                    /* Bit 31: ROM/RAM modifier flag - must be masked off for the data size */
                    uint32_t data_length = read_u32_le(&vgm[pc + 3]) & 0x7FFFFFFF;

                    if (blk_type == 0x8B) {
                        uint32_t rom_total   = read_u32_le(&vgm[pc + 7]);
                        uint32_t start_off   = read_u32_le(&vgm[pc + 11]);
                        uint32_t actual_size = (data_length > 8) ? (data_length - 8) : 0;
                        MSM6295_LoadROM(&vgm[pc + 15], actual_size, start_off, rom_total);
                    }

                    pc += (7 + data_length);
                }
                break;

            case VGM_CMD_END:
                if (header.loop_offset != 0 && loopmode) {
                    pc = header.loop_offset;
                    /* Don't let the duration display grow forever: when
                       looping, wrap the timer back to the position where the
                       loop first started (total_samples - loop_samples),
                       not 0. */
                    elapsed_samples = (header.loop_samples < header.total_samples)
                                        ? (header.total_samples - header.loop_samples)
                                        : 0;
                } else {
                    /* Loop off: instead of restarting the same song, tell
                       the main loop "this song finished" - it advances using
                       the same path as EXTI10 (next song). */
                    track_finished = 1;
                }
                return;

            /* Unknown / unhandled commands - advance PC according to the VGM spec's command lengths */
            default:
                if      (cmd >= 0x30 && cmd <= 0x3F) pc += 2; /* PSG variants: 1 operand */
                else if (cmd >= 0x40 && cmd <= 0x4E) pc += 3; /* various chips: 2 operands */
                else if (cmd == 0x4F)                pc += 2; /* Game Gear stereo: 1 operand */
                else if (cmd >= 0x51 && cmd <= 0x5F) pc += 3; /* FM chips: 2 operands */
                else if (cmd == 0x64)                pc += 4; /* override wait length: 3 operands */
                else if (cmd == 0x65)                pc += 1; /* reserved: 0 operands */
                else if (cmd >= 0x80 && cmd <= 0x8F) pc += 1; /* YM2612 DAC+wait: 0 operands */
                else if (cmd == 0xA0)                pc += 3; /* AY8910: 2 operands */
                else if (cmd >= 0xB0 && cmd <= 0xBF) pc += 3; /* various chips: 2 operands */
                else if (cmd >= 0xC0 && cmd <= 0xC8) pc += 4; /* various chips: 3 operands */
                else if (cmd >= 0xD0 && cmd <= 0xD6) pc += 4; /* various chips: 3 operands */
                else if (cmd >= 0xE0 && cmd <= 0xE3) pc += 5; /* various chips: 4 operands */
                else                                 pc += 1; /* truly unknown */
                break;
        }
    }
}
