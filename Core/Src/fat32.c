#include "fat32.h"
#include "sd_spi.h"
#include <string.h>

static uint8_t  sect[512];

static uint32_t part_start;
static uint32_t fat_start;
static uint32_t data_start;
static uint32_t spc;           /* sectors per cluster */
static uint32_t root_cluster;

/* ------------------------------------------------------------------ */

static uint16_t ru16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t ru32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}

static uint32_t cluster_lba(uint32_t c) {
    return data_start + (c - 2) * spc;
}

static uint32_t next_cluster(uint32_t c) {
    uint32_t fat_off  = c * 4;
    uint32_t fat_sect = fat_start + fat_off / 512;
    uint32_t entry_off = fat_off % 512;
    if (SD_ReadBlock(fat_sect, sect) != SD_OK) return 0x0FFFFFFFu;
    return ru32(&sect[entry_off]) & 0x0FFFFFFFu;
}

/* ------------------------------------------------------------------ */

int FAT32_Init(void)
{
    /* MBR */
    if (SD_ReadBlock(0, sect) != SD_OK)              return -1;
    if (sect[510] != 0x55 || sect[511] != 0xAA)     return -2;

    /* First partition entry at 0x1BE, LBA start at +8 */
    part_start = ru32(&sect[0x1BE + 8]);
    if (part_start == 0)                             return -3;

    /* FAT32 boot sector */
    if (SD_ReadBlock(part_start, sect) != SD_OK)     return -4;
    if (sect[510] != 0x55 || sect[511] != 0xAA)     return -5;
    if (ru16(&sect[0x0B]) != 512)                    return -6;  /* must be 512 B/sect */

    spc          = sect[0x0D];
    uint32_t rsvd = ru16(&sect[0x0E]);
    uint32_t nfat = sect[0x10];
    uint32_t fatsz = ru32(&sect[0x24]);
    root_cluster   = ru32(&sect[0x2C]);

    fat_start  = part_start + rsvd;
    data_start = fat_start  + nfat * fatsz;

    return 0;
}

/* Convert "FILENAME.EXT" -> FAT32 8.3 uppercase space-padded (11 bytes) */
static void to_83(const char *name, char out[11])
{
    memset(out, ' ', 11);
    int i = 0, j = 0;
    while (name[i] && name[i] != '.' && j < 8) {
        char c = name[i++];
        out[j++] = (c >= 'a' && c <= 'z') ? c - 32 : c;
    }
    if (name[i] == '.') { i++; j = 8; }
    while (name[i] && j < 11) {
        char c = name[i++];
        out[j++] = (c >= 'a' && c <= 'z') ? c - 32 : c;
    }
}

int FAT32_FindFile(const char *name, FAT32_File *f)
{
    char key[11];
    to_83(name, key);

    uint32_t cluster = root_cluster;
    while (cluster < 0x0FFFFFF8u) {
        uint32_t lba = cluster_lba(cluster);
        for (uint32_t s = 0; s < spc; s++) {
            if (SD_ReadBlock(lba + s, sect) != SD_OK) return -1;
            for (int e = 0; e < 16; e++) {
                uint8_t *en = &sect[e * 32];
                if (en[0] == 0x00) return -2;   /* end of directory */
                if (en[0] == 0xE5) continue;    /* deleted */
                if (en[11] == 0x0F) continue;   /* LFN */
                if (en[11] & 0x18) continue;    /* volume label or directory */

                if (memcmp(en, key, 11) == 0) {
                    f->first_cluster = ((uint32_t)ru16(&en[20]) << 16) | ru16(&en[26]);
                    f->size = ru32(&en[28]);
                    return 0;
                }
            }
        }
        cluster = next_cluster(cluster);
    }
    return -3;  /* not found */
}

/* Converts an 8.3 entry to "FILENAME.EXT" format (max 12 chars + null) */
static void from_83(const uint8_t *en, char *out)
{
    int i = 0, j = 0;
    /* Name part (8 bytes) - skip trailing spaces */
    while (i < 8 && en[i] != ' ') out[j++] = en[i++];
    i = 8;
    /* Extension part (3 bytes) - add a dot if present */
    if (en[8] != ' ') {
        out[j++] = '.';
        while (i < 11 && en[i] != ' ') out[j++] = en[i++];
    }
    out[j] = '\0';
}

/* Returns the Nth file in the root directory (0-based).
   name_out: optional, at least 13-byte buffer; NULL may be passed.
   Returns: 0 success, -1 read error, -2 index out of range. */
int FAT32_FindFileByIndex(uint32_t index, FAT32_File *f, char *name_out)
{
    uint32_t count   = 0;
    uint32_t cluster = root_cluster;

    while (cluster < 0x0FFFFFF8u) {
        uint32_t lba = cluster_lba(cluster);
        for (uint32_t s = 0; s < spc; s++) {
            if (SD_ReadBlock(lba + s, sect) != SD_OK) return -1;
            for (int e = 0; e < 16; e++) {
                uint8_t *en = &sect[e * 32];
                if (en[0] == 0x00) return -2;   /* end of directory */
                if (en[0] == 0xE5) continue;    /* deleted */
                if (en[11] == 0x0F) continue;   /* LFN entry */
                if (en[11] & 0x18) continue;    /* volume label or directory */

                if (count == index) {
                    f->first_cluster = ((uint32_t)ru16(&en[20]) << 16) | ru16(&en[26]);
                    f->size = ru32(&en[28]);
                    if (name_out) from_83(en, name_out);
                    return 0;
                }
                count++;
            }
        }
        cluster = next_cluster(cluster);
    }
    return -2;
}

/* Fills names[] with ALL file names in the root directory in a SINGLE PASS
   (instead of calling FAT32_FindFileByIndex once per index and rescanning
   from the start N times, this walks the directory once and collects them
   all - gives O(n) instead of O(n^2) when drawing the list screen).
   Returns: number of files found (limited by max_count). */
int FAT32_CacheAllNames(char names[][13], int max_count)
{
    int      count   = 0;
    uint32_t cluster = root_cluster;

    while (cluster < 0x0FFFFFF8u && count < max_count) {
        uint32_t lba = cluster_lba(cluster);
        for (uint32_t s = 0; s < spc && count < max_count; s++) {
            if (SD_ReadBlock(lba + s, sect) != SD_OK) return count;
            for (int e = 0; e < 16 && count < max_count; e++) {
                uint8_t *en = &sect[e * 32];
                if (en[0] == 0x00) return count;   /* end of directory */
                if (en[0] == 0xE5) continue;       /* deleted */
                if (en[11] == 0x0F) continue;      /* LFN entry */
                if (en[11] & 0x18) continue;       /* volume label or directory */

                from_83(en, names[count]);
                count++;
            }
        }
        cluster = next_cluster(cluster);
    }
    return count;
}

/* Returns the number of valid files in the root directory (0-based index
   upper bound = return value - 1). */
int FAT32_CountFiles(void)
{
    int      count   = 0;
    uint32_t cluster = root_cluster;

    while (cluster < 0x0FFFFFF8u) {
        uint32_t lba = cluster_lba(cluster);
        for (uint32_t s = 0; s < spc; s++) {
            if (SD_ReadBlock(lba + s, sect) != SD_OK) return count;
            for (int e = 0; e < 16; e++) {
                uint8_t *en = &sect[e * 32];
                if (en[0] == 0x00) return count;    /* end of directory */
                if (en[0] == 0xE5) continue;        /* deleted */
                if (en[11] == 0x0F) continue;        /* LFN entry */
                if (en[11] & 0x18) continue;        /* volume label or directory */
                count++;
            }
        }
        cluster = next_cluster(cluster);
    }
    return count;
}

int FAT32_ReadBytes(const FAT32_File *f, uint32_t offset, uint8_t *buf, uint32_t len)
{
    uint32_t cluster_size = spc * 512;

    /* Walk to starting cluster */
    uint32_t cluster = f->first_cluster;
    for (uint32_t i = 0; i < offset / cluster_size; i++) {
        cluster = next_cluster(cluster);
        if (cluster >= 0x0FFFFFF8u) return -1;
    }

    uint32_t done = 0;
    uint32_t pos  = offset;

    while (done < len && cluster < 0x0FFFFFF8u) {
        uint32_t c_off  = pos % cluster_size;
        uint32_t s_idx  = c_off / 512;
        uint32_t s_off  = c_off % 512;

        if (SD_ReadBlock(cluster_lba(cluster) + s_idx, sect) != SD_OK) return -2;

        uint32_t copy = 512 - s_off;
        if (copy > len - done) copy = len - done;
        memcpy(buf + done, sect + s_off, copy);

        done += copy;
        pos  += copy;

        if (pos % cluster_size == 0)
            cluster = next_cluster(cluster);
    }

    return (int)done;
}
