#include "sd_spi.h"
#include "main.h"
#include <string.h>

extern SPI_HandleTypeDef hspi2;

/* SD_CS = PA8 (SD_CSA8_Pin from main.h) */
#define CS_LOW()   HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET)
#define CS_HIGH()  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET)

#define HAL_TIMEOUT  500u

static SD_CardType card_type = SD_TYPE_UNKNOWN;

/* Debugger: CMD0 response (should be 0x01; 0xFF = no response at all) */
uint8_t  sd_debug_cmd0_r  = 0xFF;
HAL_StatusTypeDef sd_debug_spi_status = HAL_OK; /* HAL_OK=0, HAL_ERROR=1, HAL_BUSY=2, HAL_TIMEOUT=3 */

/* 512-byte all-0xFF buffer for block reads via TransmitReceive */
static uint8_t ff_buf[512];

/* ------------------------------------------------------------------ */

static void spi_set_prescaler(uint32_t psc)
{
    hspi2.Init.BaudRatePrescaler = psc;
    HAL_SPI_Init(&hspi2);
}

static uint8_t spi_byte(uint8_t tx)
{
    uint8_t rx = 0xFF;
    HAL_StatusTypeDef s = HAL_SPI_TransmitReceive(&hspi2, &tx, &rx, 1, HAL_TIMEOUT);
    if (s != HAL_OK) sd_debug_spi_status = s;  /* capture first error */
    return rx;
}

static void spi_read_buf(uint8_t *buf, uint16_t len)
{
    /* A single bulk transfer instead of calling HAL_SPI_TransmitReceive byte
       by byte (512 separate HAL calls + their overhead) - ff_buf is already
       filled with 0xFF and used as the dummy TX source. */
    HAL_SPI_TransmitReceive(&hspi2, ff_buf, buf, len, HAL_TIMEOUT);
}

/* Send SD command, return R1 byte */
static uint8_t sd_cmd(uint8_t cmd, uint32_t arg)
{
    uint8_t crc = 0x01;
    if (cmd == 0) crc = 0x95;
    if (cmd == 8) crc = 0x87;

    spi_byte(0x40 | cmd);
    spi_byte((arg >> 24) & 0xFF);
    spi_byte((arg >> 16) & 0xFF);
    spi_byte((arg >>  8) & 0xFF);
    spi_byte( arg        & 0xFF);
    spi_byte(crc);

    /* Wait up to 8 bytes for valid R1 (MSB = 0) */
    uint8_t r = 0xFF;
    for (int i = 0; i < 8; i++) {
        r = spi_byte(0xFF);
        if (!(r & 0x80)) break;
    }
    return r;
}

/* ------------------------------------------------------------------ */

SD_Status SD_Init(void)
{
    memset(ff_buf, 0xFF, sizeof(ff_buf));

    /* Slow clock for init: PLL2P 6.32 MHz / 32 = ~197 kHz (<400 kHz required) */
    spi_set_prescaler(SPI_BAUDRATEPRESCALER_32);

    CS_HIGH();
    HAL_Delay(10);

    /* 160 dummy clocks with CS deasserted */
    for (int i = 0; i < 20; i++)
        spi_byte(0xFF);

    /* CMD0: reset to SPI mode — retry up to 5 times */
    uint8_t r = 0xFF;
    for (int attempt = 0; attempt < 5 && r != 0x01; attempt++) {
        CS_HIGH();
        for (int i = 0; i < 10; i++) spi_byte(0xFF);
        CS_LOW();
        r = sd_cmd(0, 0);
        CS_HIGH();
        spi_byte(0xFF);
    }
    sd_debug_cmd0_r = r;   /* inspect in debugger: 0x01=OK, 0xFF=no response */
    if (r != 0x01)
        return SD_ERR_NOCARD;

    /* CMD8: detect SDv2 */
    CS_LOW();
    r = sd_cmd(8, 0x000001AA);
    if (r == 0x01) {
        /* SDv2: read 4-byte R7 tail */
        uint8_t r7[4];
        spi_read_buf(r7, 4);
        CS_HIGH();
        spi_byte(0xFF);

        if (r7[3] != 0xAA)
            return SD_ERR; /* voltage range rejected */

        /* ACMD41 with HCS=1 until idle clears */
        uint32_t deadline = HAL_GetTick() + 2000;
        do {
            CS_LOW(); sd_cmd(55, 0);       CS_HIGH(); spi_byte(0xFF);
            CS_LOW(); r = sd_cmd(41, 0x40000000); CS_HIGH(); spi_byte(0xFF);
        } while (r == 0x01 && (int32_t)(HAL_GetTick() - deadline) < 0);

        if (r != 0x00)
            return SD_ERR;

        /* CMD58: read OCR — CCS bit determines SDHC */
        CS_LOW();
        r = sd_cmd(58, 0);
        if (r == 0x00) {
            uint8_t ocr[4];
            spi_read_buf(ocr, 4);
            card_type = (ocr[0] & 0x40) ? SD_TYPE_SDHC : SD_TYPE_V2;
        }
        CS_HIGH();
        spi_byte(0xFF);

    } else {
        /* SDv1 or MMC */
        CS_HIGH();
        spi_byte(0xFF);

        uint32_t deadline = HAL_GetTick() + 2000;
        do {
            CS_LOW(); sd_cmd(55, 0);  CS_HIGH(); spi_byte(0xFF);
            CS_LOW(); r = sd_cmd(41, 0); CS_HIGH(); spi_byte(0xFF);
        } while (r == 0x01 && (int32_t)(HAL_GetTick() - deadline) < 0);

        if (r != 0x00) {
            /* Try CMD1 for MMC */
            deadline = HAL_GetTick() + 2000;
            do {
                CS_LOW(); r = sd_cmd(1, 0); CS_HIGH(); spi_byte(0xFF);
            } while (r == 0x01 && (int32_t)(HAL_GetTick() - deadline) < 0);

            if (r != 0x00)
                return SD_ERR;
            card_type = SD_TYPE_MMC;
        } else {
            card_type = SD_TYPE_V1;
        }

        /* CMD16: force 512-byte block size for SDSC/MMC */
        CS_LOW();
        r = sd_cmd(16, 512);
        CS_HIGH();
        spi_byte(0xFF);
        if (r != 0x00)
            return SD_ERR;
    }

    /* CMD59: disable CRC checking in SPI mode */
    CS_LOW();
    sd_cmd(59, 0);
    CS_HIGH();
    spi_byte(0xFF);

    /* Fast clock: 6.32 MHz / 2 = ~3.16 MHz. Most SD cards handle well above
       this (20+ MHz) in SPI mode; drop to PRESCALER_4/8 if issues arise. */
    spi_set_prescaler(SPI_BAUDRATEPRESCALER_2);
    return SD_OK;
}

SD_Status SD_ReadBlock(uint32_t block, uint8_t *buf)
{
    /* SDHC addresses by block; SDSC/MMC by byte */
    uint32_t addr = (card_type == SD_TYPE_SDHC) ? block : block * 512u;

    for (int attempt = 0; attempt < 3; attempt++) {
        CS_LOW();
        uint8_t r = sd_cmd(17, addr);
        if (r != 0x00) {
            CS_HIGH();
            spi_byte(0xFF);
            continue;
        }

        /* Wait for data token 0xFE */
        uint32_t deadline = HAL_GetTick() + 500;
        do {
            r = spi_byte(0xFF);
        } while (r == 0xFF && (int32_t)(HAL_GetTick() - deadline) < 0);

        if (r != 0xFE) {
            CS_HIGH();
            spi_byte(0xFF);
            continue;
        }

        /* Read 512 bytes + 2 CRC bytes (discard) */
        spi_read_buf(buf, 512);
        spi_byte(0xFF);
        spi_byte(0xFF);

        CS_HIGH();
        spi_byte(0xFF);
        return SD_OK;
    }
    return SD_ERR;
}

SD_CardType SD_GetType(void)
{
    return card_type;
}
