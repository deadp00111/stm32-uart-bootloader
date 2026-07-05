#include "stm32f411_regs.h"
#include "uart.h"

/* USART2: PA2=TX(AF7), PA3=RX(AF7). HSI 16MHz assumed (default reset clock). */
void uart_init(uint32_t baud)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    GPIOA->MODER &= ~((3U << (2*2)) | (3U << (3*2)));
    GPIOA->MODER |=  ((2U << (2*2)) | (2U << (3*2))); /* AF mode */
    GPIOA->AFR[0] &= ~((0xFU << (2*4)) | (0xFU << (3*4)));
    GPIOA->AFR[0] |=  ((7U  << (2*4)) | (7U  << (3*4))); /* AF7 = USART2 */

    uint32_t apb1_clk = 16000000UL; /* HSI, no PLL */
    USART2->BRR = (apb1_clk + (baud / 2U)) / baud;
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void uart_send_byte(uint8_t b)
{
    while (!(USART2->SR & USART_SR_TXE));
    USART2->DR = b;
}

uint8_t uart_recv_byte(void)
{
    while (!(USART2->SR & USART_SR_RXNE));
    return (uint8_t)USART2->DR;
}

void uart_send_buf(const uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) uart_send_byte(buf[i]);
}

void uart_recv_buf(uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) buf[i] = uart_recv_byte();
}
