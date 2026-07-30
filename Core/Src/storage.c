#include "storage.h"
#include "main.h"

/* The very last sector of flash: Bank2, Sector 127 (8KB), address 0x081FE000.
   vgm_data_start is very early in flash (~0x0800B8F0), the program never
   comes anywhere near here - this region is entirely safe/free for our data. */
#define STORAGE_FLASH_ADDR 0x081FE000UL
#define STORAGE_BANK       FLASH_BANK_2
#define STORAGE_SECTOR     127U
#define STORAGE_MAGIC      0x53564D31UL /* 'SVM1' */

/* Exactly 16 bytes = 1 quadword (the H5 flash programming unit). */
typedef struct {
    uint32_t magic;
    int32_t  song_index;
    uint32_t reserved1;
    uint32_t reserved2;
} StorageRecord;

#define FACTORY_DEFAULT_INDEX 8

static int cached_index = FACTORY_DEFAULT_INDEX;

void Storage_Init(void) {
    const StorageRecord *rec = (const StorageRecord *)STORAGE_FLASH_ADDR;
    if (rec->magic == STORAGE_MAGIC)
        cached_index = rec->song_index;
}

int Storage_GetLastSongIndex(void) {
    return cached_index;
}

void Storage_SaveLastSongIndex(int index) {
    if (index == cached_index) return; /* unchanged, no unnecessary flash write */

    StorageRecord rec = {0};
    rec.magic      = STORAGE_MAGIC;
    rec.song_index = index;

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase = {0};
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Banks     = STORAGE_BANK;
    erase.Sector    = STORAGE_SECTOR;
    erase.NbSectors = 1;

    uint32_t sector_error = 0;
    if (HAL_FLASHEx_Erase(&erase, &sector_error) == HAL_OK) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, STORAGE_FLASH_ADDR, (uint32_t)&rec);
    }

    HAL_FLASH_Lock();

    cached_index = index;
}
