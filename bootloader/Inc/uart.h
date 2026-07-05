#ifndef UART_H
#define UART_H
#include <stdint.h>

void uart_init(uint32_t baud);
void uart_send_byte(uint8_t b);
uint8_t uart_recv_byte(void);
void uart_send_buf(const uint8_t *buf, uint32_t len);
void uart_recv_buf(uint8_t *buf, uint32_t len);

#endif
