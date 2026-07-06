#include "stm32f411_regs.h"
#include "uart.h"
#include "protocol.h"

static void led_init(void) {
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    GPIOC_MODER &= ~(3U << 26);
    GPIOC_MODER |=  (1U << 26);
}

static void led_toggle(void) {
    GPIOC_ODR ^= (1U << 13);
}

static void delay(volatile uint32_t n) {
    while (n--);
}

int main(void) {
    led_init();
    uart_init();

    // Blink 3 times to show bootloader mode 
    for (int i = 0; i < 3; i++) {
        led_toggle();
        delay(500000);
        led_toggle();
        delay(500000);
    }

    bootloader_run();

    while (1) {
        led_toggle();
        delay(100000);
    }
}