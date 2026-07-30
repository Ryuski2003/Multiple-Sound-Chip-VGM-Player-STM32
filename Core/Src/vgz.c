#include "vgz.h"
#include "puff.h"

int VGZ_IsGzip(const uint8_t *data, uint32_t len) {
    return (len >= 2) && (data[0] == 0x1F) && (data[1] == 0x8B);
}

/* Gzip container: magic(2) + CM(1) + FLG(1) + MTIME(4) + XFL(1) + OS(1)
 * = a fixed 10-byte header, followed by variable fields depending on the
 * FLG bits (FEXTRA/FNAME/FCOMMENT/FHCRC), then the raw deflate stream, and
 * finally an 8-byte trailer (CRC32 + ISIZE) - RFC 1952. */
uint32_t VGZ_Inflate(const uint8_t *src, uint32_t src_len, uint8_t *dst, uint32_t dst_cap) {
    if (!VGZ_IsGzip(src, src_len) || src_len < 18) return 0;
    if (src[2] != 8) return 0; /* CM: only deflate (8) is supported */

    uint8_t  flg = src[3];
    uint32_t pos = 10;

    if (flg & 0x04) { /* FEXTRA */
        if (pos + 2 > src_len) return 0;
        uint16_t xlen = (uint16_t)src[pos] | ((uint16_t)src[pos + 1] << 8);
        pos += 2 + xlen;
    }
    if (flg & 0x08) { /* FNAME: null-terminated */
        while (pos < src_len && src[pos] != 0) pos++;
        if (pos >= src_len) return 0;
        pos++;
    }
    if (flg & 0x10) { /* FCOMMENT: null-terminated */
        while (pos < src_len && src[pos] != 0) pos++;
        if (pos >= src_len) return 0;
        pos++;
    }
    if (flg & 0x02) { /* FHCRC */
        pos += 2;
    }
    if (pos + 8 > src_len) return 0; /* must leave room for the deflate stream + 8-byte trailer */

    unsigned long destlen = dst_cap;
    unsigned long srclen  = src_len - pos - 8; /* excluding the CRC32+ISIZE trailer */

    int ret = puff(dst, &destlen, &src[pos], &srclen);
    if (ret != 0) return 0; /* puff(): 0 = success, negative/positive = error */

    return (uint32_t)destlen;
}
