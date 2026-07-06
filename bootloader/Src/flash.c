#include "flash.h"
#include "stm32f411_regs.h"

// Wait until flash is not busy 
static void flash_wait(void) {
    while (FLASH_SR & FLASH_SR_BSY);
}

// Unlock flash for write/erase 
void flash_unlock(void) {
    if (FLASH_CR & FLASH_CR_LOCK) {
        FLASH_KEYR = FLASH_KEY1;
        FLASH_KEYR = FLASH_KEY2;
    }
}

// Lock flash after write/erase 
void flash_lock(void) {
    FLASH_CR |= FLASH_CR_LOCK;
}

// Erase one flash sector 
void flash_erase_sector(uint8_t sector) {
    flash_wait();

    FLASH_CR &= ~(0xFU << FLASH_CR_SNB);
    FLASH_CR |= (sector << FLASH_CR_SNB);
    FLASH_CR |= FLASH_CR_SER;
    FLASH_CR |= FLASH_CR_STRT;

    flash_wait();
    FLASH_CR &= ~FLASH_CR_SER;
}

// Write data to flash (must be erased first) 
void flash_write(uint32_t addr, uint8_t *data, uint32_t len) {
    uint32_t i;

    for (i = 0; i + 3 < len; i += 4) {
        uint32_t word = data[i] | (data[i+1] << 8) |
                       (data[i+2] << 16) | (data[i+3] << 24);

        flash_wait();
        FLASH_CR |= FLASH_CR_PG;
        *(volatile uint32_t *)(addr + i) = word;
        flash_wait();
        FLASH_CR &= ~FLASH_CR_PG;
    }

    //Handle remaining bytes 
    if (i < len) {
        uint32_t word = 0xFFFFFFFF;
        uint32_t j;
        for (j = 0; j < (len - i); j++) {
            word &= ~(0xFFU << (j * 8));
            word |= (data[i + j] << (j * 8));
        }
        flash_wait();
        FLASH_CR |= FLASH_CR_PG;
        *(volatile uint32_t *)(addr + i) = word;
        flash_wait();
        FLASH_CR &= ~FLASH_CR_PG;
    }
}