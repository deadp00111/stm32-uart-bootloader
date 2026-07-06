#include "uart.h"
#include "stm32f411_regs.h"

// Init UART1 on PA9 (TX) and PA10 (RX) at 115200 baud 
void uart_init(void) {
    /* Enable clocks */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;

    // PA9, PA10 as alternate function 
    GPIOA_MODER &= ~((3U << 18) | (3U << 20));
    GPIOA_MODER |=  (2U << 18) | (2U << 20);

    /* AF7 for USART1 */
    GPIOA_AFRH &= ~((0xFU << 4) | (0xFU << 8));
    GPIOA_AFRH |=  (7U << 4) | (7U << 8);

    GPIOA_OSPEEDR |= (3U << 18) | (3U << 20);

    //115200 baud @ 16MHz HSI 
    USART1_BRR = 139;

    //Enable TX, RX, UART 
    USART1_CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

//Send one byte 
void uart_send(uint8_t c) {
    while (!(USART1_SR & USART_SR_TXE));
    USART1_DR = c;
    while (!(USART1_SR & USART_SR_TC));
}

// Receive one byte(blocking) 
uint8_t uart_recv(void) {
    while (!(USART1_SR & USART_SR_RXNE));
    return (uint8_t)USART1_DR;
}

// Receive with timeout in ms, returns -1 if timeout 
int uart_recv_timeout(uint32_t ms) {
    volatile uint32_t count = 0;
    while (!(USART1_SR & USART_SR_RXNE)) {
        if (++count > (ms * 1600)) {
            return -1;
        }
    }
    return (uint8_t)USART1_DR;
}