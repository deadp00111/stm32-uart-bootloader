# STM32F411 UART Bootloader

Bare-metal UART bootloader for STM32F411CEU6 (WeAct Black Pill).
Upload firmware over USB-to-TTL without ST-Link after first flash.

## Toolchain

- arm-none-eabi-gcc    : ARM Cortex-M4 compiler
- arm-none-eabi-binutils : Linker, objcopy
- stlink               : Flash via ST-Link
- python-pyserial      : PC uploader script
- make                 : Build automation

Arch install: sudo pacman -S arm-none-eabi-gcc arm-none-eabi-binutils stlink python-pyserial make

## How It Works

### Two Programs in Flash

Bootloader lives at 0x08000000 (16 KB)
Application lives at 0x08004000 (480 KB)

### Power-On Flow

    Power ON
       |
       v
    Bootloader starts
    (address 0x08000000)
       |
       +-- LED blinks 3 times
       +-- Send "BOOT
" on UART
       +-- Wait 3 seconds
       |
       +-- No command? ----> Jump to Application
       |                      (address 0x08004000)
       |
       +-- Got command? ---> Handle command
                             (ERASE / WRITE / GO)

### The Jump to Application

    Read stack pointer from 0x08004000
    Read reset handler from 0x08004004
       |
       v
    Is stack pointer in SRAM?
    (0x20000000 to 0x20020000)
       |
       +-- No ---> Stay in bootloader
       |
       +-- Yes ---> Disable interrupts
                     Set SCB_VTOR = 0x08004000
                     Set stack pointer
                     Jump to reset handler
                     (never returns)

### Why SCB_VTOR Matters

Without SCB_VTOR set:
    CPU looks for interrupts at 0x08000000
    This is bootloader code
    Result: CRASH on any interrupt

With SCB_VTOR = 0x08004000:
    CPU looks for interrupts at 0x08004000
    This is application code
    Result: Interrupts work correctly

### UART Protocol

Every command starts with:
    [CMD] [~CMD]
    command + bitwise complement

Example:
    CMD = 0x31
    ~CMD = 0xCE
    Check: 0x31 XOR 0xCE = 0xFF  (valid)

If bit flips in UART:
    CMD = 0x30 (flipped)
    ~CMD = 0xCE
    Check: 0x30 XOR 0xCE = 0xFE  (not 0xFF, invalid)
    Result: NACK sent

### Commands

ERASE (0x43):
    PC sends:  0x43 0xBC
    STM32:    ACK
    PC sends:  0xFF 0xFF (global erase)
    STM32:    ACK after erasing

WRITE (0x31):
    PC sends:  0x31 0xCE
    STM32:    ACK
    PC sends:  address (4 bytes) + checksum
    STM32:    ACK
    PC sends:  N=255 (256 bytes) + data + checksum
    STM32:    ACK after writing
    (repeat for next chunk)

GO (0x21):
    PC sends:  0x21 0xDE
    STM32:    ACK
    PC sends:  jump address (4 bytes) + checksum
    STM32:    ACK then jumps
    (never returns to bootloader)

### Full Upload Sequence

    PC:  ERASE command
    STM32: ACK, erase app flash
    PC:  WRITE command + address 0x08004000
    STM32: ACK
    PC:  256 bytes of firmware
    STM32: ACK
    PC:  256 bytes of firmware
    STM32: ACK
    ... (repeat until all sent)
    PC:  GO command + address 0x08004000
    STM32: ACK, jump to app
    App:  LED starts blinking

### Flash Memory Rules

Flash can only change: 1 -> 0
Flash cannot change: 0 -> 1

To write new data:
    Step 1: Erase sector (sets all bits to 1, value 0xFF)
    Step 2: Write data (changes some 1s to 0s)

Without erase:
    Old value: 0b11110000
    New data:  0b00001111
    Result:    0b00000000 (wrong!)

With erase:
    After erase: 0b11111111
    New data:    0b00001111
    Result:      0b00001111 (correct)

### Unlock Flash

Flash is locked by default (prevents accidental writes)

To unlock:
    Write 0x45670123 to FLASH_KEYR
    Write 0xCDEF89AB to FLASH_KEYR
    Now flash can be erased and written

To lock:
    Set FLASH_CR_LOCK bit
    Flash is protected again

### Memory Map

Flash (512 KB total):
    0x08000000 - 0x08003FFF : Bootloader (16 KB)
    0x08004000 - 0x0807FFFF : Application (480 KB)
    0x08080000              : End of flash

SRAM (128 KB total):
    0x20000000 - 0x2001FFFF : Variables, heap
    0x20020000              : Stack top (grows down)

### Build Output

    make all

    bootloader.elf:
        text: 2064 bytes
        data: 0 bytes
        bss:  0 bytes
        total: 2 KB

    app.elf:
        text: 140 bytes
        data: 0 bytes
        bss:  0 bytes
        total: 140 bytes

### Project Files

bootloader/
    Inc/
        stm32f411_regs.h    : Register addresses (GPIO, UART, Flash, RCC)
        uart.h              : UART function declarations
        flash.h             : Flash function declarations
        protocol.h          : Command codes (WRITE, ERASE, GO)
    Src/
        main.c                : Entry point, LED blink, start protocol
        uart.c                : UART init, send, receive
        flash.c               : Unlock, erase, write flash
        protocol.c            : Command handlers, jump to app
        startup_stm32f411xe.s : Assembly startup
    bootloader.ld             : Linker script (16 KB at 0x08000000)

application/
    main.c                    : App entry, set VTOR, blink LED
    startup_stm32f411xe.s     : Assembly startup
    app.ld                    : Linker script (480 KB at 0x08004000)

pc_tool/
    uploader.py               : Send firmware over UART

Makefile                      : Build rules

### Key Code: UART Init

    Enable GPIOA clock
    Enable USART1 clock
    Set PA9 and PA10 to Alternate Function mode
    Select AF7 (USART1) for both pins
    Set baud rate: 16 MHz / 115200 = 139
    Enable transmitter, receiver, UART

### Key Code: Flash Erase Sector

    Wait for flash not busy
    Clear old sector number
    Set new sector number
    Select sector erase mode
    Start erase
    Wait for completion
    Clear erase mode

### Key Code: Application Setup

    SCB_VTOR = 0x08000000 + 0x4000
    (set vector table to app location)

    Without this:
        Interrupts crash into bootloader
    With this:
        Interrupts work in application
