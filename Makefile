CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
CPU     = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft

BOOT_DIR = bootloader
APP_DIR  = application

BOOT_SRC_C = $(BOOT_DIR)/Src/main.c $(BOOT_DIR)/Src/bootloader.c \
             $(BOOT_DIR)/Src/uart.c $(BOOT_DIR)/Src/flash.c \
             $(BOOT_DIR)/Src/crc.c  $(BOOT_DIR)/Src/protocol.c
BOOT_SRC_S = $(BOOT_DIR)/Src/startup_stm32f411xe.s
BOOT_OBJ   = $(BOOT_SRC_C:.c=.o) $(BOOT_SRC_S:.s=.o)
BOOT_CFLAGS  = $(CPU) -Wall -O2 -std=c11 -I$(BOOT_DIR)/Inc -ffunction-sections -fdata-sections
BOOT_LDFLAGS = $(CPU) -T $(BOOT_DIR)/bootloader.ld -Wl,--gc-sections -nostdlib

APP_SRC_C = $(APP_DIR)/main.c
APP_SRC_S = $(APP_DIR)/startup_stm32f411xe.s
APP_OBJ   = $(APP_SRC_C:.c=.o) $(APP_SRC_S:.s=.o)
APP_CFLAGS  = $(CPU) -Wall -O2 -std=c11
APP_LDFLAGS = $(CPU) -T $(APP_DIR)/app.ld -Wl,--gc-sections -nostdlib

.PHONY: all bootloader application flash flash_app clean

all: bootloader application

bootloader: $(BOOT_DIR)/bootloader.elf $(BOOT_DIR)/bootloader.bin
	$(SIZE) $(BOOT_DIR)/bootloader.elf

application: $(APP_DIR)/app.elf $(APP_DIR)/app.bin
	$(SIZE) $(APP_DIR)/app.elf

$(BOOT_DIR)/Src/%.o: $(BOOT_DIR)/Src/%.c
	$(CC) $(BOOT_CFLAGS) -c $< -o $@

$(BOOT_DIR)/Src/%.o: $(BOOT_DIR)/Src/%.s
	$(CC) $(CPU) -c $< -o $@

$(BOOT_DIR)/bootloader.elf: $(BOOT_OBJ)
	$(CC) $(BOOT_LDFLAGS) $(BOOT_OBJ) -o $@

$(BOOT_DIR)/bootloader.bin: $(BOOT_DIR)/bootloader.elf
	$(OBJCOPY) -O binary $< $@

$(APP_DIR)/%.o: $(APP_DIR)/%.c
	$(CC) $(APP_CFLAGS) -c $< -o $@

$(APP_DIR)/%.o: $(APP_DIR)/%.s
	$(CC) $(CPU) -c $< -o $@

$(APP_DIR)/app.elf: $(APP_OBJ)
	$(CC) $(APP_LDFLAGS) $(APP_OBJ) -o $@

$(APP_DIR)/app.bin: $(APP_DIR)/app.elf
	$(OBJCOPY) -O binary $< $@

flash: $(BOOT_DIR)/bootloader.bin
	st-flash write $(BOOT_DIR)/bootloader.bin 0x08000000

flash_app: $(APP_DIR)/app.bin
	python3 pc_tool/uploader.py --port /dev/ttyUSB0 --file $(APP_DIR)/app.bin

clean:
	rm -f $(BOOT_DIR)/Src/*.o $(BOOT_DIR)/*.elf $(BOOT_DIR)/*.bin
	rm -f $(APP_DIR)/*.o $(APP_DIR)/*.elf $(APP_DIR)/*.bin
