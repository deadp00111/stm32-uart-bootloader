#ifndef STM32F411_REGS_H
#define STM32F411_REGS_H
#include <stdint.h>

#define PERIPH_BASE      0x40000000UL
#define AHB1PERIPH_BASE  (PERIPH_BASE + 0x00020000UL)
#define APB1PERIPH_BASE  (PERIPH_BASE)
#define APB2PERIPH_BASE  (PERIPH_BASE + 0x00010000UL)

typedef struct {
  volatile uint32_t CR, PLLCFGR, CFGR, CIR,
    AHB1RSTR, AHB2RSTR, RESERVED0[2],
    APB1RSTR, APB2RSTR, RESERVED1[2],
    AHB1ENR, AHB2ENR, RESERVED2[2],
    APB1ENR, APB2ENR, RESERVED3[2],
    AHB1LPENR, AHB2LPENR, RESERVED4[2],
    APB1LPENR, APB2LPENR, RESERVED5[2],
    BDCR, CSR, RESERVED6[2],
    SSCGR, PLLI2SCFGR;
} RCC_TypeDef;
#define RCC ((RCC_TypeDef *)(AHB1PERIPH_BASE + 0x3800UL))

typedef struct {
  volatile uint32_t MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, BSRR, LCKR, AFR[2];
} GPIO_TypeDef;
#define GPIOA ((GPIO_TypeDef *)(AHB1PERIPH_BASE + 0x0000UL))
#define GPIOC ((GPIO_TypeDef *)(AHB1PERIPH_BASE + 0x0800UL))

typedef struct {
  volatile uint32_t SR, DR, BRR, CR1, CR2, CR3, GTPR;
} USART_TypeDef;
#define USART2 ((USART_TypeDef *)(APB1PERIPH_BASE + 0x4400UL))

typedef struct {
  volatile uint32_t ACR, KEYR, OPTKEYR, SR, CR, OPTCR;
} FLASH_TypeDef;
#define FLASH ((FLASH_TypeDef *)(AHB1PERIPH_BASE + 0x3C00UL))

typedef struct {
  volatile uint32_t DR, IDR, CR;
} CRC_TypeDef;
#define CRC_HW ((CRC_TypeDef *)(AHB1PERIPH_BASE + 0x3000UL))

/* RCC enable bits */
#define RCC_AHB1ENR_GPIOAEN   (1U << 0)
#define RCC_AHB1ENR_CRCEN     (1U << 12)
#define RCC_APB1ENR_USART2EN  (1U << 17)

/* USART bits */
#define USART_SR_TXE   (1U << 7)
#define USART_SR_RXNE  (1U << 5)
#define USART_CR1_UE   (1U << 13)
#define USART_CR1_TE   (1U << 3)
#define USART_CR1_RE   (1U << 2)

/* FLASH bits */
#define FLASH_SR_BSY    (1U << 16)
#define FLASH_CR_PG     (1U << 0)
#define FLASH_CR_SER    (1U << 1)
#define FLASH_CR_STRT   (1U << 16)
#define FLASH_CR_LOCK   (1U << 31)
#define FLASH_CR_PSIZE_1 (1U << 9) /* x32 programming */
#define FLASH_KEY1  0x45670123UL
#define FLASH_KEY2  0xCDEF89ABUL

#endif
