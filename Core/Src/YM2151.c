#include "YM2151.h"

#define YM_DATA_PORT GPIOF
#define YM_CTRL_PORT GPIOG

static uint8_t pan_shadow[8] = {0}; // shadow of the 0x20-0x27 registers (preserves FL/CON)

/* Per-channel key-on shadow: 0 = all operators off, >0 = at least one is on.
   Kept for the activity indicator without doing real audio analysis. */
static uint8_t keyon_shadow[8] = {0};

/* Shadow of the 0x60-0x7F Total Level registers (32 operators). The 0x7F
   writes during YM2151_Mute_TotalLevel() are not reflected here (the
   write-hook skips the update while tl_muting=1) - this way
   Unmute_TotalLevel() can restore the real pre-mute song values instead of
   forcing a fixed 0x00 (full volume) or 0x7F (silent). */
static uint8_t tl_shadow[32] = {0};
static uint8_t tl_muting = 0;

/* Real master clock (Hz), varies per song via the SI5351. The BUSY wait
   duration in YM2151_Write is scaled accordingly. Default: NTSC colorburst,
   the most common value, a safe baseline before the real clock has been
   reported. */
static uint32_t ym_master_clock_hz = 3579545U;

void YM2151_SetMasterClockHz(uint32_t hz) {
    if (hz > 0) ym_master_clock_hz = hz;
}

void YM2151_Init(void) {
    /* Enable the DWT cycle counter (for precise delay in YM2151_Write) */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL       |= DWT_CTRL_CYCCNTENA_Msk;

    /* Hardware reset */
    HAL_GPIO_WritePin(YM_CTRL_PORT, YM_WR_Pin | YM_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(YM_CTRL_PORT, YM_IC_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(YM_CTRL_PORT, YM_IC_Pin, GPIO_PIN_SET);
    HAL_Delay(1);

    /* The generic reset loop below writes register 0x08 (key-on/off) only
       once, with data=0 - this ONLY key-offs channel 0. If channels 1-7
       still have notes triggered from the previous song, they can keep
       sounding while the TL registers below are reset to 0x00 (max
       volume!) below, producing a brief noise/click - explicitly key-off
       all channels first. */
    YM2151_Mute_KeyOff();

    /* Reset all registers */
    for (int i = 0; i < 0xFF; i++)
        YM2151_Write(i, 0x00);

    /* NOTE: the generic reset above also writes Total Level to 0x00, which
       on the YM2151 means 0dB attenuation = MAXIMUM VOLUME (not silent!).
       While the envelope slowly decays in the release phase after key-off,
       it can get stuck there since Release Rate is also reset to 0 (the
       slowest), and can still be audible once the output is opened
       (Unmute). Force TL to maximum attenuation explicitly for guaranteed
       silence regardless of envelope state. */
    YM2151_Mute_TotalLevel();
}

void YM2151_SetPins(uint8_t byte) {
    YM_DATA_PORT->ODR = (YM_DATA_PORT->ODR & 0xFF00) | byte;
}

/* YM2151 write cycle:
 *   Address: A0=0, data_bus=addr, WR↓, CS↓, CS↑, WR↑
 *   Data:    A0=1, data_bus=data, WR↓, CS↓, CS↑, WR↑        */
void YM2151_Write(uint8_t addr, uint8_t data) {
    if (addr >= 0x20 && addr <= 0x27)
        pan_shadow[addr - 0x20] = data;

    if (addr == YM2151_REG_KEYON_OFF)
        keyon_shadow[data & 0x07] = (data >> 3) & 0x0F;

    if (addr >= YM2151_REG_TOTAL_LEVEL && addr <= 0x7F && !tl_muting)
        tl_shadow[addr - YM2151_REG_TOTAL_LEVEL] = data;

    /* --- Address write --- */
    HAL_GPIO_WritePin(YM_CTRL_PORT, YM_A0_Pin, GPIO_PIN_RESET);
    YM2151_SetPins(addr);
    HAL_GPIO_WritePin(YM_CTRL_PORT, YM_WR_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(YM_CTRL_PORT, YM_CS_Pin, GPIO_PIN_RESET);
    __NOP(); __NOP(); __NOP(); __NOP();
    HAL_GPIO_WritePin(YM_CTRL_PORT, YM_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(YM_CTRL_PORT, YM_WR_Pin, GPIO_PIN_SET);

    /* --- Data write --- */
    HAL_GPIO_WritePin(YM_CTRL_PORT, YM_A0_Pin, GPIO_PIN_SET);
    YM2151_SetPins(data);
    HAL_GPIO_WritePin(YM_CTRL_PORT, YM_WR_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(YM_CTRL_PORT, YM_CS_Pin, GPIO_PIN_RESET);
    __NOP(); __NOP(); __NOP(); __NOP();
    HAL_GPIO_WritePin(YM_CTRL_PORT, YM_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(YM_CTRL_PORT, YM_WR_Pin, GPIO_PIN_SET);

    /* YM2151 BUSY wait: at most 128 master clocks. The master clock now
       varies per song via the SI5351 (not a fixed 4MHz) - compute the wait
       duration dynamically against the real clock, with a 25% margin. Uses
       the DWT cycle counter, independent of optimization level. */
    {
        uint32_t t0 = DWT->CYCCNT;
        uint32_t wait_cycles = (uint32_t)(((uint64_t)128 * SystemCoreClock * 5) /
                                           ((uint64_t)ym_master_clock_hz * 4));
        /* Safety cap: for real YM2151 clocks (a few hundred kHz to a few
           MHz) this value always stays around a few thousand cycles. If an
           unexpected (too small) clock value comes through, avoid an
           indefinite/excessively long wait. */
        if (wait_cycles > 50000U) wait_cycles = 50000U;
        /* DWT->CYCCNT doesn't appear to increment as expected in this
           environment - add a software counter, fully independent of the
           hardware, as a safety net: the loop terminates for certain
           whether or not the DWT is working. */
        uint32_t guard = 0;
        while (((DWT->CYCCNT - t0) < wait_cycles) && (guard < 200000UL))
            guard++;
    }
}

void YM2151_Delay(uint8_t delay) {
    for (volatile int i = 0; i < delay; i++);
}

/* channel: 0-7. Returns: 0 = all operators off, >0 = at least one is on. */
uint8_t YM2151_GetKeyOn(uint8_t channel) {
    return (channel < 8) ? keyon_shadow[channel] : 0;
}
void YM2151_Mute_KeyOff(void) {
    for (uint8_t channel = 0; channel < 8; channel++) {
        // Bits 0-2: Channel number
        // Bits 3-6: Operator key-on (set to 0 to mute)
        YM2151_Write(YM2151_REG_KEYON_OFF, channel & 0x07);
    }
}

/**
 * @brief METHOD 2: Brings all operators' Total Level down to zero (i.e. max
 * attenuation). Cuts the sound instantly and sharply.
 */
void YM2151_Mute_TotalLevel(void) {
    // Writes 127 (maximum attenuation) to all operator registers 0x60-0x7F.
    // While tl_muting=1 the write-hook does not update tl_shadow, so the
    // real pre-mute song values remain saved for Unmute_TotalLevel().
    tl_muting = 1;
    for (uint8_t reg = YM2151_REG_TOTAL_LEVEL; reg <= 0x7F; reg++) {
        YM2151_Write(reg, 0x7F); // 0x7F = 127 (the most silent attenuation level)
    }
    tl_muting = 0;
}

/**
 * @brief METHOD 3: Disconnects the channels' Left/Right analog output.
 */
void YM2151_Mute_OutputDisable(void) {
    for (uint8_t channel = 0; channel < 8; channel++) {
        // Clear the L/R bits (7-6), preserve FL/CON from the shadow
        YM2151_Write(YM2151_REG_PAN_FL_CON + channel, pan_shadow[channel] & 0x3F);
    }
}


/**
 * @brief Combined mute sequence.
 * The recommended order for clean silence when a song is stopped or ends.
 * Mute the source (Total Level) FIRST, THEN cut the output (PAN) - otherwise
 * if the output is cut abruptly while a note is at full volume, the sudden
 * discontinuity in the signal produces an audible "click".
 */
void YM_System_All_Mute(void) {
    YM2151_Mute_TotalLevel();
    YM2151_Mute_OutputDisable();
}

void YM2151_Unmute_KeyOn(void) {
    for (uint8_t channel = 0; channel < 8; channel++) {
        // Bits 3,4,5,6 -> trigger all operators (M1, C1, M2, C2) (0x78)
        // Bits 0-2 -> channel number
        YM2151_Write(YM2151_REG_KEYON_OFF, 0x78 | (channel & 0x07));
    }
}

/**
 * @brief REVERSE OF METHOD 2: Restores the Total Level (volume) values to
 * what the song itself had set before muting (from tl_shadow).
 * We do NOT force a fixed 0x00 (full volume) - that would suddenly blast an
 * operator to full volume even if it was genuinely silent before the mute.
 */
void YM2151_Unmute_TotalLevel(void) {
    for (uint8_t reg = YM2151_REG_TOTAL_LEVEL; reg <= 0x7F; reg++) {
        YM2151_Write(reg, tl_shadow[reg - YM2151_REG_TOTAL_LEVEL]);
    }
}

/**
 * @brief REVERSE OF METHOD 3: Reconnects the channels to the Left and Right
 * outputs (opens stereo). Bit 7 (L) and bit 6 (R) are set high (0xC0).
 * NOTE: this function zeroes the FL (Feedback) and CON (Connection/
 * Algorithm) values. For correct sound the instrument (patch) data needs to
 * be reloaded afterward.
 */
void YM2151_Unmute_OutputEnable(void) {
    for (uint8_t channel = 0; channel < 8; channel++) {
        // Set the L/R bits (7-6), restore FL/CON from the shadow
        YM2151_Write(YM2151_REG_PAN_FL_CON + channel, pan_shadow[channel] | 0xC0);
    }
}


/**
 * @brief Combined unmute sequence (order matters).
 * Restore Total Level to its pre-mute values FIRST (output is still closed,
 * inaudible), THEN open the output (PAN) - symmetric with the mute sequence,
 * no click.
 * NOTE: if this order is reversed (output opened first), sustained notes
 * would stay silent - since TL is still 0x7F (silent) - until a new TL write
 * arrives for that operator. This was the cause of the "melody disappears"
 * bug.
 */
void YM_System_All_Unmute(void) {
    YM2151_Unmute_TotalLevel();
    YM2151_Unmute_OutputEnable();
}
