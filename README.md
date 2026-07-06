STM32F411 UART Bootloader
=========================

Bare-metal UART bootloader for STM32F411CEU6 (WeAct Black Pill).
Upload firmware over USB-to-TTL without ST-Link after first flash.

TOOLCHAIN
---------
arm-none-eabi-gcc       ARM Cortex-M4 compiler
arm-none-eabi-binutils  Linker, objcopy
stlink                  Flash via ST-Link
python-pyserial         PC uploader script
make                    Build automation
Arch install: sudo pacman -S arm-none-eabi-gcc arm-none-eabi-binutils stlink python-pyserial make

HOW IT WORKS
------------
Two programs in flash:
  Bootloader at 0x08000000 (16 KB)
  Application at 0x08004000 (480 KB)

Power-on flow:
    Power ON
       |
       v
    Bootloader starts
       |
       +-- LED blinks 3x
       +-- Send "BOOT" on UART
       +-- Wait 3 seconds
       |
       +-- No command? ----> Jump to app at 0x08004000
       |
       +-- Got command? ---> Erase, write, jump

THE JUMP
--------
Read stack pointer from 0x08004000
Read reset handler from 0x08004004
       |
       v
Check stack is in SRAM (0x20000000 to 0x20020000)
       |
       +-- Yes ---> Disable interrupts
       |             Set SCB_VTOR = 0x08004000
       |             Set stack pointer
       |             Jump to app (never returns)
       |
       +-- No ---> Stay in bootloader

Why SCB_VTOR:
  Without it: CPU looks at 0x08000000 for interrupts = CRASH
  With it: CPU looks at 0x08004000 for interrupts = WORKS

UART PROTOCOL
-------------
Every command: [CMD] [~CMD] (command + complement)
Example: CMD = 0x31, ~CMD = 0xCE, Check: 0x31 XOR 0xCE = 0xFF = valid
Commands:
  ERASE (0x43): Clear app flash sectors
  WRITE (0x31): Write up to 256 bytes to address
  GO (0x21): Jump to address, never return
Upload sequence:
  PC: ERASE command -> STM32: ACK, erase flash
  PC: WRITE + address 0x08004000 -> STM32: ACK
  PC: 256 bytes + checksum -> STM32: ACK, write to flash
  (repeat until all sent)
  PC: GO + address 0x08004000 -> STM32: ACK, jump to app

FLASH RULES
-----------
Flash only changes: 1 -> 0
Flash cannot: 0 -> 1
To write: Step 1: Erase sector (all bits become 1, value 0xFF)
          Step 2: Write data (change some 1s to 0s)
Unlock flash: Write 0x45670123 to FLASH_KEYR
              Write 0xCDEF89AB to FLASH_KEYR
Lock flash: Set FLASH_CR_LOCK bit

MEMORY MAP
----------
Flash 512 KB:
  0x08000000 - 0x08003FFF : Bootloader (16 KB)
  0x08004000 - 0x0807FFFF : Application (480 KB)
SRAM 128 KB:
  0x20000000 - 0x2001FFFF : Variables
  0x20020000 : Stack top

BUILD
-----
make clean
make all
Output: bootloader.bin (2 KB at 0x08000000)
        app.bin (140 bytes at 0x08004000)

FLASH BOOTLOADER (ONE TIME)
---------------------------
Connect ST-Link, then: make flash-boot
Or manually: st-flash write bootloader.bin 0x08000000
Unplug ST-Link. Done forever.

UPLOAD APP VIA UART
-------------------
1. Connect USB-TTL: TX->PA10, RX->PA9, GND->GND
2. Power-cycle Black Pill (unplug USB-C, plug back)
3. Watch LED blink 3 times (bootloader mode)
4. Within 3 seconds, run: make upload
   Or: python pc_tool/uploader.py app.bin /dev/ttyUSB0


