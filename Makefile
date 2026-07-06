CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CFLAGS = -mcpu=cortex-m4 -mthumb -O0 -g -Wall -nostdlib -ffreestanding
LDFLAGS = -Wl,--gc-sections
INCLUDES = -I bootloader/Inc

.PHONY: all clean size flash-boot flash-app upload

all: bootloader.bin app.bin

# Bootloader
bootloader.elf: bootloader/Src/main.c bootloader/Src/uart.c \
                bootloader/Src/flash.c bootloader/Src/protocol.c \
                bootloader/Src/startup_stm32f411xe.s bootloader/bootloader.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -T bootloader/bootloader.ld -o $@ \
		bootloader/Src/startup_stm32f411xe.s \
		bootloader/Src/main.c \
		bootloader/Src/uart.c \
		bootloader/Src/flash.c \
		bootloader/Src/protocol.c

bootloader.bin: bootloader.elf
	$(OBJCOPY) -O binary $< $@
	$(SIZE) $<

# Application
app.elf: application/main.c application/startup_stm32f411xe.s application/app.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -T application/app.ld -o $@ \
		application/startup_stm32f411xe.s \
		application/main.c

app.bin: app.elf
	$(OBJCOPY) -O binary $< $@
	$(SIZE) $<

flash-boot: bootloader.bin
	st-flash write bootloader.bin 0x08000000

flash-app: app.bin
	st-flash write app.bin 0x08004000

upload: app.bin
	python3 pc_tool/uploader.py app.bin

size: bootloader.elf app.elf
	$(SIZE) bootloader.elf app.elf

clean:
	rm -f bootloader.elf bootloader.bin app.elf app.bin