#include "../bootloader/Inc/stm32f411_regs.h"

#define APP_OFFSET  0x4000UL  // 16KB space for bootloader 


static void delay(volatile uint32_t n) {
    while (n--);
}

int main(void)
{
    // Point vector table to app flash location (0x08004000) Bootloader is at 0x08000000
    SCB_VTOR = FLASH_BASE + APP_OFFSET;

    // Enable clock for GPIOC (PC13 = onboard LED) 
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    //Set PC13 as output 
    GPIOC_MODER &= ~(3U << 26);
    GPIOC_MODER |=  (1U << 26);

    //Blink LED forever 
    while (1) {
        GPIOC_ODR ^= (1U << 13);    
        delay(500000);               // ~250ms delay 
    }
}