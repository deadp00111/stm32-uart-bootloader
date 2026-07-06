# STM32F411 UART Bootloader

A simple bare-metal UART bootloader for the **STM32F411CEU6** (WeAct Black Pill). Upload new firmware over USB-to-TTL without touching ST-Link again.

---

## Table of Contents

1. [What is a Bootloader?](#what-is-a-bootloader)
2. [How It Works](#how-it-works)
3. [Hardware Needed](#hardware-needed)
4. [Software Setup](#software-setup)
5. [Project Structure](#project-structure)
6. [Memory Layout](#memory-layout)
7. [Step-by-Step Usage](#step-by-step-usage)
8. [Understanding the Code](#understanding-the-code)
9. [Troubleshooting](#troubleshooting)
10. [Customizing Your Application](#customizing-your-application)

---

## What is a Bootloader?

A **bootloader** is a small program that runs first when your microcontroller powers on. Its job is to:

1. Check if you want to upload new firmware (via UART/USB)
2. If yes → receive and write new firmware to flash memory
3. If no → start your existing application normally

Think of it like your phone's recovery mode:
- **Normal boot** → Android starts
- **Recovery mode** → you can install updates

Our bootloader does the same for your STM32.

---

## How It Works

### Big Picture

```
┌─────────────────────────────────────────────────────────────┐
│                     POWER ON                                │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│  BOOTLOADER (0x0800_0000)                                   │
│  • Runs first automatically                                 │
│  • Blinks LED 3 times → "I'm alive!"                        │
│  • Sends "BOOT\r\n" on UART → "PC, talk to me!"           │
│  • Waits 3 seconds for command                              │
└─────────────────────────┬───────────────────────────────────┘
                          │
            ┌─────────────┴─────────────┐
            │                           │
            ▼                           ▼
┌─────────────────────┐   ┌─────────────────────────────────┐
│  NO COMMAND         │   │  COMMAND RECEIVED               │
│  (timeout 3 sec)    │   │  (PC sends erase/write/go)      │
│                     │   │                                 │
│  Read app vector    │   │  1. Erase old app flash         │
│  table at 0x08004000│   │  2. Write new firmware bytes    │
│                     │   │  3. Jump to new app             │
│  Stack pointer OK?  │   │                                 │
│  → Jump to app      │   │                                 │
│                     │   │                                 │
│  Stack bad?         │   │                                 │
│  → Fast blink error │   │                                 │
└─────────────────────┘   └─────────────────────────────────┘
```

### Communication Protocol

The PC and STM32 talk using a simple frame format:

```
PC sends:    [CMD] [~CMD]        → Command + its bitwise complement
STM32 replies: [ACK] or [NACK]   → 0x79 = OK, 0x1F = Error

Then more data follows depending on command.
```

| Command | Code | What It Does |
|---------|------|-------------|
| `WRITE` | `0x31` | Write up to 256 bytes to flash address |
| `ERASE` | `0x43` | Erase application flash sectors |
| `GO` | `0x21` | Jump to application address and run it |

---

## Hardware Needed

| Item | Purpose | Approximate Cost |
|------|---------|-----------------|
| **STM32F411CEU6** (WeAct Black Pill) | Target microcontroller | ₹200-300 |
| **USB-to-TTL adapter** (CH340/CP2102/FT232) | PC ↔ STM32 UART communication | ₹50-100 |
| **ST-Link V2** (clone OK) | Flash bootloader **one time only** | ₹150-250 |
| **USB-C cable** | Power the Black Pill | (any phone cable) |

### Where to Buy (India)

- **Amazon/Flipkart**: Search "WeAct Black Pill STM32F411", "CH340 USB to TTL", "ST-Link V2"
- **Local electronics market**: SP Road (Bangalore), Lamington Road (Mumbai), Ritchie Street (Chennai)
- **Robu.in**, **ThingBits**, **ElectronicsComp**: Online Indian vendors

---

### Wiring Diagram

```
USB-to-TTL Adapter          Black Pill (STM32F411)
┌─────────────┐            ┌─────────────┐
│          TX │───────────►│ PA10 (RX)   │  ← Cross-connect!
│          RX │◄───────────│ PA9  (TX)   │  ← TX→RX, RX→TX
│         GND │───────────│ GND         │  ← Common ground
│         3V3 │  (NC)    │ 3V3         │  ← Don't connect!
│         5V  │  (NC)    │ 5V          │  ← Don't connect!
└─────────────┘            └─────────────┘
        │                        │
        │                        │
        └────────────────────────┘
              USB-C to Laptop
              (for power only)
```

> ⚠️ **IMPORTANT**: Only connect **3 wires**: TX, RX, GND. Do NOT connect 3.3V or 5V — the Black Pill powers itself through USB-C.

### Pin Map (Black Pill)

| Pin | Function | Color (typical) |
|-----|----------|----------------|
| PA9 | USART1 TX | Green wire |
| PA10 | USART1 RX | White wire |
| PC13 | Onboard LED (active-low) | Blue LED on board |
| GND | Ground | Black wire |

---

## Software Setup

### On Arch Linux

```bash
# 1. Install ARM toolchain (compiler for STM32)
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-binutils

# 2. Install ST-Link tools (for flashing bootloader once)
sudo pacman -S stlink

# 3. Install Python serial library (for PC uploader tool)
sudo pacman -S python-pyserial

# 4. Verify installation
arm-none-eabi-gcc --version   # Should show 13.x or later
st-info --version             # Should show stlink version
python -c "import serial; print('pyserial OK')"
```

### On Ubuntu/Debian

```bash
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi
sudo apt install stlink-tools python3-serial
```

### On Windows

1. Download **STM32CubeCLT** or **gcc-arm-none-eabi** from ARM website
2. Install **ST-Link drivers** from ST website
3. Install Python from python.org, then: `pip install pyserial`
4. Use **Git Bash** or **WSL** for make commands

---

## Project Structure

```
STM32-UART-BOOTLOADER/
│
├── bootloader/                    ← BOOTLOADER CODE (runs first)
│   ├── Inc/                       ← Header files
│   │   ├── stm32f411_regs.h      ← Register addresses (GPIO, UART, Flash, etc.)
│   │   ├── uart.h                 ← UART function declarations
│   │   ├── flash.h                ← Flash function declarations
│   │   └── protocol.h             ← Command codes (WRITE, ERASE, GO)
│   │
│   ├── Src/                       ← Source files
│   │   ├── main.c                 ← Bootloader entry point (LED blink, then protocol)
│   │   ├── uart.c                 ← UART send/receive functions
│   │   ├── flash.c                ← Flash unlock/erase/write functions
│   │   ├── protocol.c             ← Command handler (WRITE, ERASE, GO)
│   │   └── startup_stm32f411xe.s  ← Assembly startup (sets stack, calls main)
│   │
│   └── bootloader.ld              ← Linker script: place code at 0x08000000, size 16KB
│
├── application/                   ← YOUR APPLICATION CODE
│   ├── main.c                     ← App entry point (LED blink example)
│   ├── startup_stm32f411xe.s      ← App startup
│   └── app.ld                     ← Linker script: place code at 0x08004000, size 480KB
│
├── pc_tool/
│   └── uploader.py               ← Python script: sends firmware to STM32 over UART
│
├── Makefile                       ← Build everything with one command
└── README.md                      ← This file
```

---

## Memory Layout

The STM32F411CEU6 has **512 KB Flash** and **128 KB SRAM**.

```
FLASH MEMORY (512 KB total)
┌─────────────────────────────────────────────────────────┐
│  0x0800_0000 ──┐                                        │
│                │  BOOTLOADER (16 KB)                    │
│                │  • First code to run                    │
│                │  • Handles UART updates                 │
│                │  • Cannot be changed without ST-Link    │
│  0x0800_4000 ──┘                                        │
│                                                         │
│  0x0800_4000 ──┐                                        │
│                │  APPLICATION (480 KB)                   │
│                │  • Your actual program                  │
│                │  • Can be updated any time via UART     │
│                │  • Vector table at start                │
│  0x0808_0000 ──┘                                        │
└─────────────────────────────────────────────────────────┘

SRAM (128 KB)
┌─────────────────────────────────────────────────────────┐
│  0x2000_0000 ──┐                                        │
│                │  Variables, heap, stack                │
│                │  • Stack starts at 0x2002_0000 (top)    │
│                │  • Grows downward                       │
│  0x2002_0000 ──┘                                        │
└─────────────────────────────────────────────────────────┘
```

### Why 16 KB for Bootloader?

| Sector | Size | Used For |
|--------|------|----------|
| 0 | 16 KB | Bootloader code |
| 1 | 16 KB | (reserved) |
| 2 | 16 KB | (reserved) |
| 3 | 16 KB | (reserved) |
| 4 | 64 KB | Application start |
| 5 | 128 KB | Application |
| 6 | 128 KB | Application |
| 7 | 128 KB | Application |

Total app space: 64 + 128 + 128 + 128 = **448 KB** (plus some of sector 3 if needed)

Our bootloader is only ~2 KB, but 16 KB gives room to grow.

---

## Step-by-Step Usage

### Step 1: Clone the Repository

```bash
git clone https://github.com/YOUR_USERNAME/stm32-uart-bootloader.git
cd stm32-uart-bootloader
```

### Step 2: Build Everything

```bash
make clean
make all
```

Output:
```
   text    data     bss     dec     hex filename
   2064       0       0    2064     810 bootloader.elf
    140       0       0     140      8c app.elf
```

Files created:
- `bootloader.bin` → 2 KB, goes to `0x08000000`
- `app.bin` → ~140 bytes, goes to `0x08004000`

### Step 3: Flash Bootloader (ONE TIME ONLY)

Connect **ST-Link** to Black Pill:

| ST-Link | Black Pill |
|---------|-----------|
| SWDIO | SWDIO |
| SWCLK | SWCLK |
| GND | GND |
| 3.3V | 3.3V |

Run:
```bash
make flash-boot
```

Or manually:
```bash
st-flash write bootloader.bin 0x08000000
```

Expected output:
```
st-flash 1.7.0
2024-... INFO common.c: Loading device parameters....
2024-... INFO common.c: Device connected is: STM32F411xC/xE, 128 KiB SRAM, 512 KiB flash
2024-... INFO common.c: Flash written and verified! jolly good!
```

> ✅ **Done!** Unplug ST-Link. You won't need it again for this board.

### Step 4: Connect USB-to-TTL Adapter

```
USB-TTL      Black Pill
  TX    →   PA10 (RX)
  RX    →   PA9  (TX)
  GND   →   GND
```

Plug USB-TTL into your laptop. Check port name:
```bash
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
# Typical output: /dev/ttyUSB0 or /dev/ttyACM0
```

### Step 5: Power-Cycle Black Pill

Unplug USB-C, wait 2 seconds, plug back in.

**Watch the LED** — it should blink **3 times slowly**. This means bootloader is active.

### Step 6: Upload Application via UART

**Within 3 seconds** of power-on (while LED is blinking or just after), run:

```bash
make upload
```

Or with specific port:
```bash
python pc_tool/uploader.py app.bin /dev/ttyUSB0
```

### Step 7: What You Should See

**PC Terminal:**
```
[*] Opening app.bin
[*] Firmware size: 140 bytes
[*] Waiting for bootloader...
    Bootloader: BOOT
[*] Erasing application flash...
[+] Erase successful
[*] Writing... 100.0%
[+] Write complete!
[*] Jumping to 0x08004000...
[+] Done! Jumped to application.
```

**Black Pill LED:**
- 3 blinks → bootloader mode
- Then: **steady blinking** → your app is running!

---

## Understanding the Code

### Register Header (`stm32f411_regs.h`)

This file maps human-readable names to hardware memory addresses.

```c
#define GPIOC_BASE      (PERIPH_BASE + 0x20800UL)
#define GPIOC_MODER     (*(volatile uint32_t *)(GPIOC_BASE + 0x00))
#define GPIOC_ODR       (*(volatile uint32_t *)(GPIOC_BASE + 0x14))
```

| What it means | Translation |
|-------------|-------------|
| `GPIOC_BASE` | GPIOC registers start at address `0x4002_0800` |
| `GPIOC_MODER` | Mode register is at offset `0x00` from base |
| `*(volatile uint32_t *)` | Treat this address as a 32-bit register |
| `volatile` | Don't optimize away — hardware can change this |

### UART Setup (`uart.c`)

```c
void uart_init(void) {
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;   // 1. Turn on GPIOA clock
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;  // 2. Turn on USART1 clock

    GPIOA_MODER |= (2U << 18);            // 3. PA9 = Alternate Function
    GPIOA_AFRH  |= (7U << 4);             // 4. AF7 = USART1

    USART1_BRR = 139;                     // 5. 115200 baud @ 16MHz
    USART1_CR1 = USART_CR1_TE |           // 6. Enable transmitter
                 USART_CR1_RE |           //    Enable receiver
                 USART_CR1_UE;            //    Enable UART
}
```

**Why clocks first?** Every STM32 peripheral needs its clock enabled before use. Otherwise it's dead.

**Baud rate math:**
- CPU runs at 16 MHz (internal oscillator after reset)
- `BRR = 16,000,000 / 115,200 = 138.88 ≈ 139`

### Flash Operations (`flash.c`)

Flash is **read-only by default**. To write:

```c
flash_unlock();              // 1. Magic keys to unlock
flash_erase_sector(4);     // 2. Must erase before write (sets all bits to 1)
flash_write(addr, data, n); // 3. Write 32-bit words
flash_lock();                // 4. Lock for safety
```

**Why erase first?** Flash can only change `1 → 0`, not `0 → 1`. Erasing sets all bits to `1`.

**Why 32-bit words?** STM32F411 flash interface writes 4 bytes at a time for speed.

### Protocol Handler (`protocol.c`)

The main loop:

```c
void bootloader_run(void) {
    uart_send('B'); uart_send('O'); uart_send('O'); uart_send('T');

    int cmd = uart_recv_timeout(3000);  // Wait 3 seconds

    if (cmd < 0) {
        jump_to_app();                   // No command → run app
    }

    while (1) {
        // Handle WRITE, ERASE, GO commands forever
    }
}
```

### Jump to Application (`handle_go()`)

```c
// Read app's vector table
uint32_t sp = *(uint32_t *)addr;        // Stack pointer (offset 0)
uint32_t pc = *(uint32_t *)(addr + 4);  // Reset handler (offset 4)

// Sanity check: stack must point to SRAM
if ((sp & 0x2FFE0000) != 0x20000000) return;

// Disable interrupts, set vector table, set stack, jump
__asm("cpsid i");                       // Disable interrupts
SCB_VTOR = addr;                         // Point vector table to app
__asm("msr msp, %0" :: "r"(sp));        // Set main stack pointer
void (*app)(void) = (void (*)(void))pc; // Function pointer
app();                                   // Jump! Never returns.
```

**Why `SCB_VTOR`?** Without this, interrupts would use the bootloader's vector table and crash.

---

## Troubleshooting

| Problem | Cause | Solution |
|---------|-------|----------|
| `No such file /dev/ttyUSB0` | Wrong port | Check `ls /dev/tty*` or `dmesg \| grep tty` |
| `Permission denied` | Not in group | `sudo usermod -aG uucp $USER` then **logout and login** |
| `ModuleNotFoundError: serial` | pyserial missing | `sudo pacman -S python-pyserial` |
| LED doesn't blink after upload | App not running | Check `SCB_VTOR = FLASH_BASE + 0x4000` in app |
| `st-flash` not found | ST-Link tools missing | Install `stlink` package |
| Upload fails at 0% | Bootloader not running | Reset board, try within 3 seconds |
| LED blinks 3 times then fast blink | No valid app | App not flashed or corrupted. Re-upload. |
| Upload says "Erase failed" | Checksum error | Check wiring, try again |
| `git push` fails | SSH key issue | See repository setup instructions |

### Check USB-TTL Port on Arch

```bash
# List all serial devices
ls /dev/tty*

# Check kernel messages for newly connected device
dmesg | tail -20

# Common names:
# /dev/ttyUSB0  → CH340/CP2102/FT232
# /dev/ttyACM0  → CDC ACM devices
```

### Verify Bootloader is Running

```bash
# Connect USB-TTL, open serial monitor
python -c "import serial; s=serial.Serial('/dev/ttyUSB0', 115200, timeout=5); 
import time; time.sleep(1); print(s.read_all())"
```

Should show: `b'BOOT\r\n'`

---

## Customizing Your Application

### Edit `application/main.c`

```c
#include "../bootloader/Inc/stm32f411_regs.h"

#define APP_OFFSET  0x4000UL  // MUST match bootloader.ld reservation

static void delay(volatile uint32_t n) {
    while (n--);
}

int main(void)
{
    // CRITICAL: Point vector table to app location
    // Bootloader is at 0x08000000, app is at 0x08004000
    SCB_VTOR = FLASH_BASE + APP_OFFSET;

    // Enable clock for GPIOC (PC13 = onboard LED)
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    // Set PC13 as output
    GPIOC_MODER &= ~(3U << 26);  // Clear old mode
    GPIOC_MODER |=  (1U << 26);  // Set to 01 = output

    // Blink forever
    while (1) {
        GPIOC_ODR ^= (1U << 13);  // Toggle PC13
        delay(500000);             // ~250ms delay
    }
}
```

### Key Rules for Application Code

| Rule | Why |
|------|-----|
| `SCB_VTOR = 0x08004000` | Without this, interrupts crash into bootloader |
| Linker starts at `0x08004000` | Don't overwrite bootloader |
| Don't use first 16 KB of flash | That's bootloader's space |

### Rebuild and Upload

```bash
# Build new app
make app.bin

# Upload via UART (remember to power-cycle first!)
python pc_tool/uploader.py app.bin /dev/ttyUSB0
```

### Adding More Peripherals

Example: Add UART print in your app

```c
// In application/main.c, after SCB_VTOR setup:

// Re-init UART (bootloader config is lost after jump)
RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
RCC_APB2ENR |= RCC_APB2ENR_USART1EN;
GPIOA_MODER |= (2U << 18) | (2U << 20);
GPIOA_AFRH  |= (7U << 4) | (7U << 8);
USART1_BRR = 139;
USART1_CR1 = USART_CR1_TE | USART_CR1_UE;

// Send message
const char *msg = "Hello from app!\r\n";
for (int i = 0; msg[i]; i++) {
    while (!(USART1_SR & USART_SR_TXE));
    USART1_DR = msg[i];
}
```

---

## How the PC Uploader Works (`uploader.py`)

```python
class Bootloader:
    def upload(self, firmware_path):
        # 1. Read app.bin file
        with open(firmware_path, 'rb') as f:
            firmware = f.read()

        # 2. Wait for "BOOT\r\n" from STM32
        time.sleep(0.5)
        boot_msg = self.ser.read_all()

        # 3. Send ERASE command (clear old app)
        self.send_cmd(CMD_ERASE)
        self.ser.write(bytes([0xFF, 0xFF]))  # Global erase

        # 4. Send WRITE command in 256-byte chunks
        for i in range(0, len(firmware), 256):
            chunk = firmware[i:i+256].ljust(256, b'\xFF')
            self.send_cmd(CMD_WRITE)
            # ... send address, data, checksum

        # 5. Send GO command (jump to app)
        self.send_cmd(CMD_GO)
        self.ser.write(address_bytes)
```

### Why 256-byte chunks?

STM32 flash writes efficiently in 256-byte (or larger) blocks. Smaller chunks work but are slower.

### Why `0xFF` padding?

Erased flash is all `0xFF`. Padding with `0xFF` means "don't change these bytes" — harmless if the chunk is short.

---

## Files Explained

| File | Purpose | Lines |
|------|---------|-------|
| `stm32f411_regs.h` | Register address definitions | ~80 |
| `uart.h` / `uart.c` | UART initialization, send, receive | ~50 |
| `flash.h` / `flash.c` | Flash unlock, erase, write | ~60 |
| `protocol.h` / `protocol.c` | Command handling, jump to app | ~150 |
| `main.c` (bootloader) | Entry point, LED blink, start protocol | ~30 |
| `main.c` (application) | Your code, LED blink example | ~20 |
| `startup_stm32f411xe.s` | Assembly: set stack, call main | ~10 |
| `bootloader.ld` | Linker: 16KB at 0x08000000 | ~15 |
| `app.ld` | Linker: 480KB at 0x08004000 | ~15 |
| `uploader.py` | PC tool: send firmware over UART | ~120 |
| `Makefile` | Build rules for all targets | ~40 |

**Total project:** ~600 lines of code. No HAL, no CubeMX, no complexity.

---

## Learning Path

1. **Start here**: Read this README, build and flash bootloader
2. **Understand registers**: Read `stm32f411_regs.h` — see how addresses map to hardware
3. **Trace UART init**: Follow `uart_init()` — clocks, pins, baud rate, enable
4. **Watch protocol**: Add `uart_send('X')` in `protocol.c` to trace execution
5. **Modify app**: Change blink rate, add UART output, read a button
6. **Extend bootloader**: Add CRC check, add password protection, add version query

---

## License

MIT License — free for personal, educational, and commercial use.

```
Copyright (c) 2024 [Your Name]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction...
```

---

## Credits & Resources

- **STM32F411 Reference Manual (RM0383)**: [st.com](https://www.st.com)
- **WeAct Black Pill schematic**: WeAct Studio
- **ARM Cortex-M4 User Guide**: [developer.arm.com](https://developer.arm.com)
- **Bare-metal STM32 tutorials**: [vivonomicon.com](https://vivonomicon.com)

**Written for**: Final year B.Tech projects, hobbyists, and anyone who wants to understand how microcontrollers actually work.

---

## Contact

For issues, questions, or improvements:
- GitHub Issues: [github.com/YOUR_USERNAME/stm32-uart-bootloader/issues](https://github.com/YOUR_USERNAME/stm32-uart-bootloader/issues)
- Email: your.email@example.com

**Happy hacking!** 🔧
