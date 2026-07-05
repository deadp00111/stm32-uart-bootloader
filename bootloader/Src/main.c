#include "bootloader.h"
#include "uart.h"

int main(void)
{
    uart_init(115200);
    bootloader_run();
    while (1); /* unreachable - bootloader_run() either jumps or loops forever */
}
