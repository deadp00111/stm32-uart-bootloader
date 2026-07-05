#include "stm32f411_regs.h"
#include "bootloader.h"
#include "uart.h"
#include "flash.h"
#include "crc.h"
#include "protocol.h"

typedef void (*app_entry_t)(void);
static volatile uint32_t *SCB_VTOR(void); /* fwd decl, defined below */

int app_is_valid(uint32_t addr)
{
    uint32_t sp = *(volatile uint32_t *)addr;
    /* SP must point inside SRAM range 0x2000_0000 - 0x2001_FFFF (128K) */
    return (sp & 0x2FFE0000UL) == 0x20000000UL;
}

void jump_to_app(uint32_t addr)
{
    uint32_t sp           = *(volatile uint32_t *)addr;
    uint32_t reset_vector = *(volatile uint32_t *)(addr + 4);

    __asm volatile ("cpsid i" ::: "memory"); /* disable IRQs */

    /* deinit peripherals we touched so app starts from a clean reset state */
    USART2->CR1 = 0;
    RCC->APB1ENR = 0;
    RCC->AHB1ENR = 0;

    *SCB_VTOR() = addr;
    __asm volatile ("msr msp, %0" :: "r"(sp));

    app_entry_t app_reset = (app_entry_t)reset_vector;
    app_reset();
    while (1); /* never reached */
}

static volatile uint32_t *SCB_VTOR(void)
{
    /* fixed address; avoids pulling in full CMSIS core header */
    return (volatile uint32_t *)0xE000ED08UL;
}

int boot_pin_pressed(void)
{
    /* PA0 pulled low = force bootloader mode. Configure as input pull-up. */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER &= ~(3U << (0*2));      /* PA0 input */
    GPIOA->PUPDR &= ~(3U << (0*2));
    GPIOA->PUPDR |=  (1U << (0*2));      /* pull-up */
    for (volatile int i = 0; i < 1000; i++); /* settle */
    return (GPIOA->IDR & 1U) == 0;
}

static void recv_and_flash_firmware(void)
{
    static uint8_t chunk[MAX_CHUNK];
    uint32_t write_addr = APP_BASE_ADDR;
    int erased_sectors[8];
    for (int i = 0; i < 8; i++) erased_sectors[i] = 0; /* avoid memset under -nostdlib */

    while (1) {
        int len = protocol_recv_frame(chunk, MAX_CHUNK);
        if (len < 0) {
            uart_send_byte(NACK);
            continue;
        }

        int sector = flash_sector_of_addr(write_addr);
        if (sector >= 0 && !erased_sectors[sector]) {
            flash_erase_sector((uint8_t)sector);
            erased_sectors[sector] = 1;
        }

        /* pad to 4-byte alignment for word programming */
        uint32_t padded_len = ((uint32_t)len + 3) & ~3U;
        if (padded_len > (uint32_t)len) {
            for (uint32_t i = (uint32_t)len; i < padded_len; i++) chunk[i] = 0xFF;
        }

        flash_write_buf(write_addr, chunk, padded_len);

        /* read-back verify */
        if (crc32_calc((uint8_t *)write_addr, (uint32_t)len) != crc32_calc(chunk, (uint32_t)len)) {
            uart_send_byte(NACK);
            continue;
        }

        write_addr += padded_len;
        uart_send_byte(ACK);

        if (app_is_valid(APP_BASE_ADDR)) return; /* enough to boot, host may still send more */
    }
}

void bootloader_run(void)
{
    if (!boot_pin_pressed() && app_is_valid(APP_BASE_ADDR)) {
        jump_to_app(APP_BASE_ADDR);
    }

    while (1) {
        recv_and_flash_firmware();
        if (app_is_valid(APP_BASE_ADDR)) {
            jump_to_app(APP_BASE_ADDR);
        }
    }
}

/* -nostdlib means no libc; GCC's loop-idiom pass can still emit calls to
 * memset/memcpy from plain loops at -O2, so we provide minimal versions. */
void *memset(void *s, int c, unsigned int n)
{
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

void *memcpy(void *dst, const void *src, unsigned int n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dst;
}
