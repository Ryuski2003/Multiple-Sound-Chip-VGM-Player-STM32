#include "msm6295.h"
#include <string.h>

static const int16_t step_table[49] = {
     16,   17,   19,   21,   23,   25,   28,   31,   34,   37,
     41,   45,   50,   55,   60,   66,   73,   80,   88,   97,
    107,  118,  130,  143,  157,  173,  190,  209,  230,  253,
    279,  307,  337,  371,  408,  449,  494,  544,  598,  658,
    724,  796,  876,  963, 1060, 1166, 1282, 1411, 1552
};

static const int8_t index_table[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

static const uint16_t vol_table[16] = {
    32767, 22527, 16384, 11264,  8192,  6144,  4096,  3072,
     2048,  2048,  1024,  1024,     0,     0,     0,     0
};

typedef struct {
    uint32_t rom_start;
    uint32_t rom_stop;
    uint32_t rom_pos;
    int32_t  signal;
    int32_t  step_idx;
    uint8_t  low_nibble;
    uint8_t  playing;
    uint16_t volume;
} Channel;

#define MAX_ROM_CHUNKS 8
typedef struct {
    const uint8_t *data;
    uint32_t       base;
    uint32_t       size;
} RomChunk;

static struct {
    Channel  ch[4];
    RomChunk chunks[MAX_ROM_CHUNKS];
    int      num_chunks;
    uint32_t rom_total_size;
    int      pending;
    uint32_t phase_acc;
    uint32_t phase_inc;
    int16_t  last_out;
} msm;

static uint8_t rom_read(uint32_t addr)
{
    for (int i = 0; i < msm.num_chunks; i++) {
        uint32_t off = addr - msm.chunks[i].base;
        if (off < msm.chunks[i].size)
            return msm.chunks[i].data[off];
    }
    return 0;
}

static int16_t decode_nibble(Channel *c, uint8_t nib)
{
    int32_t step  = step_table[c->step_idx];
    int32_t delta = step >> 3;
    if (nib & 1) delta += (step >> 2);
    if (nib & 2) delta += (step >> 1);
    if (nib & 4) delta +=  step;
    if (nib & 8) c->signal -= delta;
    else         c->signal += delta;

    if (c->signal >  2047) c->signal =  2047;
    if (c->signal < -2048) c->signal = -2048;

    c->step_idx += index_table[nib & 7];
    if (c->step_idx <  0) c->step_idx =  0;
    if (c->step_idx > 48) c->step_idx = 48;

    return (int16_t)c->signal;
}

static void channel_start(int n, uint8_t sample_num, uint16_t vol)
{
    if (msm.num_chunks == 0 || sample_num > 127) return;

    Channel *c = &msm.ch[n];
    c->playing = 0;
    __asm volatile("" ::: "memory");

    uint32_t tbl = (uint32_t)sample_num * 8;

    c->rom_start = ((uint32_t)rom_read(tbl + 0) << 16) | ((uint32_t)rom_read(tbl + 1) << 8) | (uint32_t)rom_read(tbl + 2);
    c->rom_stop  = ((uint32_t)rom_read(tbl + 3) << 16) | ((uint32_t)rom_read(tbl + 4) << 8) | (uint32_t)rom_read(tbl + 5);

    if (c->rom_start >= msm.rom_total_size) return;
    if (c->rom_stop  >= msm.rom_total_size) c->rom_stop = msm.rom_total_size - 1;

    c->rom_pos    = c->rom_start;
    c->signal     = 0;
    c->step_idx   = 0;
    c->low_nibble = 0;
    c->volume     = vol;

    __asm volatile("" ::: "memory");
    c->playing = 1;
}

uint8_t MSM6295_GetChannelPlaying(uint8_t channel) {
    return (channel < 4) ? msm.ch[channel].playing : 0;
}

void MSM6295_Init(uint32_t clock_hz, uint8_t pin7, uint32_t output_rate_hz)
{
    memset(&msm, 0, sizeof(msm));
    msm.pending = -1;
    uint32_t chip_rate = clock_hz / (pin7 ? 132u : 165u);
    msm.phase_inc = (chip_rate << 16) / output_rate_hz;
}

void MSM6295_LoadROM(const uint8_t *data, uint32_t data_size,
                     uint32_t start_offset, uint32_t rom_total_size)
{
    if (msm.num_chunks >= MAX_ROM_CHUNKS) return;
    msm.chunks[msm.num_chunks].data = data;
    msm.chunks[msm.num_chunks].base = start_offset;
    msm.chunks[msm.num_chunks].size = data_size;
    msm.num_chunks++;
    msm.rom_total_size = rom_total_size;
}

void MSM6295_Write(uint8_t data)
{
    if (msm.pending < 0) {
        if (data & 0x80) {
            msm.pending = (int)data;
        } else {
            /* Durdur: bit6=CH0, bit5=CH1, bit4=CH2, bit3=CH3 */
            uint8_t mask = (data >> 3) & 0x0F;
            for (int i = 0; i < 4; i++) {
                if (mask & (1u << i))
                    msm.ch[i].playing = 0;
            }
        }
    } else {
        /* byte 1 bits 3-0 = sample number, byte 2 bits 7-4 = channel mask */
        uint8_t sample_num = (uint8_t)msm.pending & 0x7F;
        uint8_t ch_mask    = (data >> 4) & 0x0F;
        uint8_t vol_idx    =  data       & 0x0F;

        for (int i = 0; i < 4; i++) {
            if (ch_mask & (1u << i))
                channel_start(i, sample_num, vol_table[vol_idx]);
        }
        msm.pending = -1;
    }
}

int16_t MSM6295_Update(void)
{
    msm.phase_acc += msm.phase_inc;

    if (msm.phase_acc < 0x10000u)
        return msm.last_out;

    int32_t mixed = 0;

    /* chip_rate >= output_rate: phase_inc can be >= 65536, a while loop is required */
    while (msm.phase_acc >= 0x10000u) {
        msm.phase_acc -= 0x10000u;

        mixed = 0;
        for (int i = 0; i < 4; i++) {
            Channel *c = &msm.ch[i];
            if (!c->playing) continue;

            uint8_t byte = rom_read(c->rom_pos);
            uint8_t nib;

            if (!c->low_nibble) {
                nib = (byte >> 4) & 0x0F;
                c->low_nibble = 1;
            } else {
                nib = byte & 0x0F;
                c->low_nibble = 0;
                c->rom_pos++;
                if (c->rom_pos > c->rom_stop)
                    c->playing = 0;
            }

            int16_t s = decode_nibble(c, nib);
            mixed += ((int32_t)s * c->volume) >> 15;
        }
    }

    if (mixed >  2047) mixed =  2047;
    if (mixed < -2048) mixed = -2048;

    msm.last_out = (int16_t)mixed;
    return msm.last_out;
}
