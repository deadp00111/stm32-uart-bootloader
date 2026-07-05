#ifndef FLASH_H
#define FLASH_H
#include <stdint.h>

int  flash_sector_of_addr(uint32_t addr);
void flash_erase_sector(uint8_t sector);
void flash_write_word(uint32_t addr, uint32_t data);
void flash_write_buf(uint32_t addr, const uint8_t *buf, uint32_t len);

#endif
