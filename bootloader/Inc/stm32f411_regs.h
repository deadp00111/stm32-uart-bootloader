#ifndef STM32F411_REGS_H
#define STM32F411_REGS_H

#include <stdint.h>

//Memory bases 
#define FLASH_BASE      0x08000000UL
#define SRAM_BASE       0x20000000UL
#define PERIPH_BASE     0x40000000UL

//RCC 
#define RCC_BASE        (PERIPH_BASE + 0x23800UL)
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x44))

#define RCC_AHB1ENR_GPIOAEN     (1U << 0)
#define RCC_AHB1ENR_GPIOCEN     (1U << 2)
#define RCC_APB2ENR_USART1EN    (1U << 4)

// GPIOA - PA9 (TX), PA10 (RX) 
#define GPIOA_BASE      (PERIPH_BASE + 0x20000UL)
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_AFRH      (*(volatile uint32_t *)(GPIOA_BASE + 0x24))
#define GPIOA_OSPEEDR   (*(volatile uint32_t *)(GPIOA_BASE + 0x08))

//GPIOC - PC13 (LED) 
#define GPIOC_BASE      (PERIPH_BASE + 0x20800UL)
#define GPIOC_MODER     (*(volatile uint32_t *)(GPIOC_BASE + 0x00))
#define GPIOC_ODR       (*(volatile uint32_t *)(GPIOC_BASE + 0x14))

// USART1 
#define USART1_BASE     (PERIPH_BASE + 0x11000UL)
#define USART1_SR       (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART1_DR       (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART1_BRR      (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART1_CR1      (*(volatile uint32_t *)(USART1_BASE + 0x0C))

#define USART_SR_TXE    (1U << 7)
#define USART_SR_RXNE   (1U << 5)
#define USART_SR_TC     (1U << 6)
#define USART_CR1_TE    (1U << 3)
#define USART_CR1_RE    (1U << 2)
#define USART_CR1_UE    (1U << 13)

//Flash interface 
#define FLASH_IF_BASE   (PERIPH_BASE + 0x23C00UL)
#define FLASH_ACR       (*(volatile uint32_t *)(FLASH_IF_BASE + 0x00))
#define FLASH_KEYR      (*(volatile uint32_t *)(FLASH_IF_BASE + 0x04))
#define FLASH_SR        (*(volatile uint32_t *)(FLASH_IF_BASE + 0x0C))
#define FLASH_CR        (*(volatile uint32_t *)(FLASH_IF_BASE + 0x10))

#define FLASH_SR_BSY    (1U << 16)
#define FLASH_CR_PG     (1U << 0)
#define FLASH_CR_SER    (1U << 1)
#define FLASH_CR_SNB    (3U)
#define FLASH_CR_STRT   (1U << 16)
#define FLASH_CR_LOCK   (1U << 31)

#define FLASH_KEY1      0x45670123UL
#define FLASH_KEY2      0xCDEF89ABUL

//SCB 
#define SCB_VTOR        (*(volatile uint32_t *)(0xE000ED08UL))

//App starts after 16KB bootloader 
#define APP_START       (FLASH_BASE + 0x4000UL)

#endif