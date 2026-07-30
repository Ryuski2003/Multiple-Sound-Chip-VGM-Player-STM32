#pragma once
#include <stdint.h>

typedef enum {
    SD_OK = 0,
    SD_ERR,
    SD_ERR_TIMEOUT,
    SD_ERR_NOCARD,
} SD_Status;

typedef enum {
    SD_TYPE_UNKNOWN = 0,
    SD_TYPE_MMC,
    SD_TYPE_V1,
    SD_TYPE_V2,
    SD_TYPE_SDHC,
} SD_CardType;

SD_Status   SD_Init(void);
SD_Status   SD_ReadBlock(uint32_t block, uint8_t *buf);
SD_CardType SD_GetType(void);
