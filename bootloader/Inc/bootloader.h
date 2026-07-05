#ifndef BOOTLOADER_H
#define BOOTLOADER_H
#include <stdint.h>

#define APP_BASE_ADDR 0x08004000UL

int  boot_pin_pressed(void);
int  app_is_valid(uint32_t addr);
void jump_to_app(uint32_t addr);
void bootloader_run(void);

#endif
