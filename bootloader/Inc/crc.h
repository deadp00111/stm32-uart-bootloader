#ifndef CRC_H
#define CRC_H
#include <stdint.h>

uint32_t crc32_calc(const uint8_t *buf, uint32_t len);

#endif
