# STM32F411 UART bootloader

Bare-metal UART bootloader for STM32F411CEU6. Flash layout:

```
0x08000000 - 0x08003FFF   bootloader  (16 KB)
0x08004000 - 0x0807FFFF   application (496 KB)
```

## Layout

```
bootloader/     Inc/ (headers) Src/ (main, bootloader, uart, flash, crc, protocol, startup asm)
                bootloader.ld
application/    main.c, startup asm, app.ld  (demo blinky, proves the jump works)
pc_tool/        uploader.py, requirements.txt
Makefile        top-level, builds both targets
```

`bootloader.c` holds the state machine (boot-pin check, app validity check,
jump-to-app, receive+flash loop). `protocol.c` only frames/unframes packets
and checks CRC. `main.c` is just `uart_init()` + `bootloader_run()`.

## How it works (beginner friendly)

**What a bootloader is**: a tiny program at `0x08000000` that runs first on
power-up. It decides: jump straight to the real app, or wait for new
firmware over UART.

**Why two `.ld` files**: flash is split into two zones so bootloader and
app never overwrite each other.
```
0x08000000  bootloader (16KB)
0x08004000  application (rest)
```

**Startup file** (`startup_stm32f411xe.s`): runs before `main()`. Sets the
stack pointer, zeroes `.bss`, copies `.data` from flash to RAM, then calls
`main()`.

**The jump trick** (`jump_to_app()` in `bootloader.c`): every app's vector
table starts with `[stack pointer][reset address]`.
```c
uint32_t sp           = *(volatile uint32_t *)addr;       // app's stack start
uint32_t reset_vector  = *(volatile uint32_t *)(addr + 4); // app's code start
```
Set `VTOR` (tells the CPU "interrupts point here now"), set `MSP` (stack
pointer), then call `reset_vector` like a function pointer. That's the
whole trick.

**UART frame** — one packet of firmware data:
```
[0xAA][LEN_H][LEN_L][DATA][CRC32]
```
- `0xAA` = "here comes a packet"
- `LEN` = how many data bytes follow
- `CRC32` = checksum, so corrupted data gets rejected instead of bricking flash

**Flash rule beginners miss**: you can't just write to flash like RAM.
1. Unlock it (`FLASH->KEYR`)
2. Erase the whole sector first (flash can only flip bits 1→0, erase resets to 1)
3. Then write

**Files, plain English:**
```
uart.c       - send/receive 1 byte at a time
protocol.c   - groups bytes into a packet, checks CRC
flash.c      - erase + write to flash memory
bootloader.c - decides: jump to app, or wait for new firmware
main.c       - just calls bootloader_run()
```

## Build (Arch Linux)

```bash
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-binutils stlink
make            # builds bootloader/bootloader.bin and application/app.bin
```

## Flash bootloader (one-time, via ST-Link/SWD)

```bash
make flash
# or: st-flash write bootloader/bootloader.bin 0x08000000
```

## Upload app over UART (no debugger after this)

```bash
pip install -r pc_tool/requirements.txt --break-system-packages
make flash_app
# or: python3 pc_tool/uploader.py --port /dev/ttyUSB0 --file application/app.bin
```

Board must be in bootloader mode: hold PA0 low at reset, or boot before the
app at 0x08004000 is valid.

## Protocol

```
[0xAA][LEN_H][LEN_L][DATA...N][CRC32 4B big-endian]
```
ACK (0x06) / NACK (0x15) per chunk. Uploader retries up to 5x.

## Known limitations

- CRC is software (poly 0xEDB88320) to match Python `zlib.crc32`. STM32F4's
  hardware CRC peripheral uses a fixed non-reflected poly (0x04C11DB7) and
  won't match zlib without extra bit-reversal work.
- No resync if a byte is dropped mid-frame — add a timeout-based state
  machine for production use.
- App validity check is just "SP looks like it's in SRAM" — add a stored
  CRC of the whole app image for a stronger check before jumping.
- USART2 assumes PA2/PA3 — adjust if your board wires UART elsewhere.
