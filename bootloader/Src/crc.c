#include "crc.h"

/* Software CRC32 (poly 0xEDB88320, reflected) - matches Python zlib.crc32.
 * STM32 hardware CRC peripheral uses poly 0x04C11DB7 non-reflected by
 * default and won't match zlib without reconfiguring reflect bits (F4 CRC
 * unit has no reflect control) - done in software here for a guaranteed
 * match with the PC-side uploader. */
static uint32_t crc_table[256];
static int table_built = 0;

static void build_table(void)
{
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++)
            c = (c & 1) ? (0xEDB88320UL ^ (c >> 1)) : (c >> 1);
        crc_table[i] = c;
    }
    table_built = 1;
}

uint32_t crc32_calc(const uint8_t *buf, uint32_t len)
{
    if (!table_built) build_table();
    uint32_t crc = 0xFFFFFFFFUL;
    for (uint32_t i = 0; i < len; i++)
        crc = crc_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFUL;
}
