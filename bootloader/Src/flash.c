#include "stm32f411_regs.h"
#include "flash.h"

static void flash_unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
}

static void flash_lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

/* STM32F411 sector map (sectors 0-7, 512KB device) */
static const uint32_t sector_base[8] = {
    0x08000000, 0x08004000, 0x08008000, 0x0800C000,
    0x08010000, 0x08020000, 0x08040000, 0x08060000
};

int flash_sector_of_addr(uint32_t addr)
{
    for (int i = 7; i >= 0; i--)
        if (addr >= sector_base[i]) return i;
    return -1;
}

void flash_erase_sector(uint8_t sector)
{
    flash_unlock();
    while (FLASH->SR & FLASH_SR_BSY);

    FLASH->CR &= ~(0xFU << 3);
    FLASH->CR |= (sector << 3) | FLASH_CR_SER;
    FLASH->CR |= FLASH_CR_STRT;
    while (FLASH->SR & FLASH_SR_BSY);
    FLASH->CR &= ~FLASH_CR_SER;

    flash_lock();
}

void flash_write_word(uint32_t addr, uint32_t data)
{
    flash_unlock();
    while (FLASH->SR & FLASH_SR_BSY);

    FLASH->CR |= FLASH_CR_PG | FLASH_CR_PSIZE_1;
    *(volatile uint32_t *)addr = data;
    while (FLASH->SR & FLASH_SR_BSY);
    FLASH->CR &= ~FLASH_CR_PG;

    flash_lock();
}

void flash_write_buf(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    /* len must be multiple of 4; caller pads */
    for (uint32_t i = 0; i < len; i += 4) {
        uint32_t word;
        __builtin_memcpy(&word, &buf[i], 4);
        flash_write_word(addr + i, word);
    }
}
