# STM32F411 UART Bootloader

Bare-metal UART bootloader for STM32F411CEU6 (WeAct Black Pill). Upload firmware over USB-to-TTL without ST-Link after first flash.

## Toolchain

- arm-none-eabi-gcc
- ARM Cortex-M4 compiler
- arm-none-eabi-binutils (Linker, objcopy)
- stlink (Flash via ST-Link)
- python-pyserial (PC uploader script)

```bash
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-binutils stlink python-pyserial
```

## How It Works

Two programs in flash: **Bootloader** at `0x08000000` (16 KB) and **Application** at `0x08004000` (480 KB).

**Power-on flow:**

```
Power ON
  -> Bootloader starts
  -> LED blinks 3x -- Send "BOOT" on UART -- Wait 3 seconds
  -> No command? -> Jump to app at 0x08004000
  -> Got command? -> Erase, write, jump
```

## The Jump

Reset stack pointer from `0x08004004` -> Reset handler from `0x08004004`

- Check stack is in SRAM (`0x20000000` to `0x20020000`)
  - Yes -> Disable interrupts -> Set `SCB_VTOR = 0x08004000` -> Set stack pointer -> Jump to app (never returns)
  - No -> Stay in bootloader

**Why `SCB_VTOR`:** without it, CPU looks at `0x08000000` for interrupts -> crash. With it, CPU looks at `0x08004000` for interrupts -> works.

## UART Protocol

Every command: `[CMD]` `[~CMD]` (command + complement)

Example: `CMD = 0x31`, `~CMD = 0xCE`, Check: `0x31 XOR 0xCE = 0xFF` -> valid

**Commands:**
- `ERASE (0x43)`: Clear app flash sectors
- `WRITE (0x31)`: Write up to 256 bytes to address
- `GO (0x21)`: Jump to address, never return

**Upload sequence:**
1. PC: ERASE, erase flash
2. PC: WRITE + address `0x08004000` -> STM32: ACK, write to flash (repeat until all sent)
3. PC: GO + address `0x08004000` -> STM32: ACK, jump to app

## Flash Rules

Flash only changes: `1 -> 0`

`0 -> 1`: write cannot. To write: erase first (all bits become `1`, `0xFF`)

**Steps:**
1. Erase sector (all bits become 1, `0xFF`)
2. Write data (change some 1s to 0s)
3. Unlock flash: Write `0x45670123` to `FLASH_KEYR`, write `0xCDEF89AB` to `FLASH_KEYR`
4. Lock flash: Set `FLASH_CR_LOCK` bit

## Memory Map

| Region | Address Range | Size |
|---|---|---|
| Bootloader | `0x08000000` - `0x08003FFF` | 16 KB |
| Application | `0x08004000` - `0x0807FFFF` | 480 KB |
| SRAM | `0x20000000` - `0x2001FFFF` | 128 KB |
| Variables | `0x20000000` - `0x2001FFFF` | -- |
| Stack top | `0x20020000` | -- |

## Build

```bash
make clean
make all
```

**Output:**
- `bootloader.bin` (2 KB) at `0x08000000`
- `app.bin` (140 bytes) at `0x08004000`

## Flash Bootloader (One Time)

Connect ST-Link, then:

```bash
make flash-boot
```

Or manually:

```bash
st-flash write bootloader.bin 0x08000000
```

Unplug ST-Link. Done forever.

## Upload App via UART

1. Connect USB-TTL: `TX -> PA10`, `RX -> PA9`, `GND -> GND`
2. Power-cycle Black Pill (unplug USB-C, plug back)
3. Watch LED blink 3 times (bootloader mode)
4. Within 3 seconds, run:

```bash
make upload
```

Or:

```bash
python3 tool/uploader.py app.bin /dev/ttyUSB0
```
