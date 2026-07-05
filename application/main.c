#include "../bootloader/Inc/stm32f411_regs.h"

/* Must match bootloader's app linker offset (bootloader.ld reserves 16K) */
#define VECT_TAB_OFFSET 0x4000UL

static void delay(volatile uint32_t n) { while (n--); }

int main(void)
{
    /* relocate vector table to match our flash origin (0x08004000) */
    *(volatile uint32_t *)0xE000ED08UL = 0x08000000UL + VECT_TAB_OFFSET;

    /* WeAct BlackPill: onboard LED is PC13, active-low (not PA5) */
    RCC->AHB1ENR |= (1U << 2); /* GPIOCEN */
    GPIOC->MODER &= ~(3U << (13*2));
    GPIOC->MODER |=  (1U << (13*2)); /* PC13 output */

    while (1) {
        GPIOC->ODR ^= (1U << 13);
        delay(500000);
    }
}
