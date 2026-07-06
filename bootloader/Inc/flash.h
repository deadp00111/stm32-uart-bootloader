#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>

void flash_unlock(void);
void flash_lock(void);
void flash_erase_sector(uint8_t sector);
void flash_write(uint32_t addr, uint8_t *data, uint32_t len);

#endif