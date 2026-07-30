#include "SI5351.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;

#define SI5351_ADDR       (0x60 << 1)   /* HAL expects an 8-bit address */
#define SI5351_XTAL_HZ    25000000UL

#define REG_OUTPUT_ENABLE 3
#define REG_CLK_CTRL(n)   (16 + (n))
#define REG_XTAL_LOAD     183
#define REG_PLL_RESET     177

/* MS0,MS1,MS2 base registers (0x2A,0x32,0x3A) */
static const uint8_t MS_BASE[3]  = {42, 50, 58};
/* MSNA,MSNB (PLLA/PLLB feedback) base registers (0x1A,0x22) */
static const uint8_t PLL_BASE[2] = {26, 34};

/* RAM copy of the Output Enable register: kept here instead of reading it back over I2C */
static uint8_t oe_shadow = 0xFF;

static void WriteReg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    HAL_I2C_Master_Transmit(&hi2c1, SI5351_ADDR, buf, 2, HAL_MAX_DELAY);
}

static void WriteBlock(uint8_t reg, const uint8_t *data, uint8_t len) {
    uint8_t buf[9];
    buf[0] = reg;
    memcpy(&buf[1], data, len);
    HAL_I2C_Master_Transmit(&hi2c1, SI5351_ADDR, buf, len + 1, HAL_MAX_DELAY);
}

void SI5351_Init(void) {
    WriteReg(REG_OUTPUT_ENABLE, 0xFF); /* all outputs disabled until configuration is done */
    WriteReg(REG_XTAL_LOAD, 0xC0);     /* 10pF crystal load: standard for most 25MHz breakouts */
}

void SI5351_OutputEnable(uint8_t clk_num, uint8_t enable) {
    if (clk_num > 7) return;
    if (enable) oe_shadow &= (uint8_t)~(1 << clk_num);
    else        oe_shadow |= (uint8_t)(1 << clk_num);
    WriteReg(REG_OUTPUT_ENABLE, oe_shadow);
}

/* PLL/Multisynth parameter formula from AN619: for ratio = a + b/c
 *   P1 = 128*a + floor(128*b/c) - 512
 *   P2 = 128*b - c*floor(128*b/c)
 *   P3 = c
 * Applies to both the PLL feedback (MSNA/MSNB) and the output divider (MS0/MS1). */
void SI5351_SetFrequency(uint8_t clk_num, uint32_t freq_hz) {
    if (clk_num > 1 || freq_hz == 0) return;

    /* --- Output divider (d): the smallest even integer that keeps the VCO
       in the 600-900MHz range. For audio chips (a few MHz) d is typically
       150-300. */
    uint32_t d = 600000000UL / freq_hz;
    if (d < 6) d = 6;
    if (d & 1) d++;
    uint64_t fvco = (uint64_t)freq_hz * d;
    while (fvco > 900000000ULL) { d -= 2; fvco = (uint64_t)freq_hz * d; }
    while (fvco < 600000000ULL) { d += 2; fvco = (uint64_t)freq_hz * d; }

    /* --- PLL feedback ratio: fvco = XTAL * (a + b/c) --- */
    uint32_t a   = (uint32_t)(fvco / SI5351_XTAL_HZ);
    uint64_t rem = fvco - (uint64_t)a * SI5351_XTAL_HZ;
    uint32_t c   = 1000000UL;
    uint32_t b   = (uint32_t)((rem * c) / SI5351_XTAL_HZ);

    uint32_t p1 = 128 * a + (128 * b) / c - 512;
    uint32_t p2 = 128 * b - c * ((128 * b) / c);
    uint32_t p3 = c;

    uint8_t pll_regs[8] = {
        (uint8_t)(p3 >> 8), (uint8_t)p3,
        (uint8_t)((p1 >> 16) & 0x03),
        (uint8_t)(p1 >> 8), (uint8_t)p1,
        (uint8_t)(((p3 >> 12) & 0xF0) | ((p2 >> 16) & 0x0F)),
        (uint8_t)(p2 >> 8), (uint8_t)p2
    };
    WriteBlock(PLL_BASE[clk_num], pll_regs, 8);

    /* --- Output multisynth: d is an integer (b=0, c=1) --- */
    uint32_t ms_p1 = 128 * d - 512;
    uint8_t ms_regs[8] = {
        0, 1,
        (uint8_t)((ms_p1 >> 16) & 0x03),
        (uint8_t)(ms_p1 >> 8), (uint8_t)ms_p1,
        0, 0, 0
    };
    WriteBlock(MS_BASE[clk_num], ms_regs, 8);

    /* CLKx control: power on, integer divider (MSx_INT), its own PLL as
       source (clk0->PLLA, clk1->PLLB), its own multisynth as source, 8mA drive */
    uint8_t src_bit = clk_num; /* 0=PLLA, 1=PLLB */
    uint8_t ctrl = 0x40 | (uint8_t)(src_bit << 5) | 0x0C | 0x03;
    WriteReg(REG_CLK_CTRL(clk_num), ctrl);

    WriteReg(REG_PLL_RESET, 0xA0); /* PLLA+PLLB reset: prevents phase jumps on frequency change */

    SI5351_OutputEnable(clk_num, 1);
}
