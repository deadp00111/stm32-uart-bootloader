#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(void);
void uart_send(uint8_t c);
uint8_t uart_recv(void);
int uart_recv_timeout(uint32_t ms);

#endif