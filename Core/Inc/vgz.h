#ifndef _VGZ_H_
#define _VGZ_H_

#include <stdint.h>

/* Inflates a VGM file compressed in gzip (.vgz) format.
 * src/src_len : the whole compressed file (gzip header + deflate + trailer)
 * dst/dst_cap : output buffer and its capacity (decompressed data is written here)
 * Returns the number of decompressed bytes on success, 0 on error. */
uint32_t VGZ_Inflate(const uint8_t *src, uint32_t src_len, uint8_t *dst, uint32_t dst_cap);

/* Checks whether the first 2 bytes are the gzip magic (0x1F 0x8B). */
int VGZ_IsGzip(const uint8_t *data, uint32_t len);

#endif
