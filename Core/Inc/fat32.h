#pragma once
#include <stdint.h>

typedef struct {
    uint32_t first_cluster;
    uint32_t size;
} FAT32_File;

/* Returns 0 on success, negative error code on failure */
int FAT32_Init(void);
int FAT32_FindFile(const char *name, FAT32_File *f);  /* name: "FILENAME.EXT" */
int FAT32_ReadBytes(const FAT32_File *f, uint32_t offset, uint8_t *buf, uint32_t len);

/* Returns the Nth file in the root directory (0-based). name_out: at least
   13 bytes, NULL may be passed. Returns: 0 success, -1 read error, -2 index
   out of range. */
int FAT32_FindFileByIndex(uint32_t index, FAT32_File *f, char *name_out);

/* Total number of valid files in the root directory. */
int FAT32_CountFiles(void);

/* Fills names[] with all file names in the root directory in a SINGLE PASS
   (limited by max_count). Returns: number of files found. */
int FAT32_CacheAllNames(char names[][13], int max_count);
