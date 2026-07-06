#!/usr/bin/env python3
"""
STM32F411 UART Bootloader - PC Uploader
"""

import serial
import struct
import sys
import time

ACK = 0x79
NACK = 0x1F

CMD_WRITE = 0x31
CMD_ERASE = 0x43
CMD_GO = 0x21


class Bootloader:
    def __init__(self, port='/dev/ttyUSB0', baud=115200):
        self.ser = serial.Serial(port, baud, timeout=3)

    def send_cmd(self, cmd):
        """Send command and its complement, wait for ACK"""
        self.ser.write(bytes([cmd, cmd ^ 0xFF]))
        resp = self.ser.read(1)
        return resp[0] == ACK if resp else False

    def erase_app(self):
        """Erase all application sectors (4,5,6,7)"""
        print("[*] Erasing application flash...")
        if not self.send_cmd(CMD_ERASE):
            print("[!] Erase command failed")
            return False

        # Global erase: N=0xFF, checksum=0xFF
        self.ser.write(bytes([0xFF, 0xFF]))
        resp = self.ser.read(1)
        if resp and resp[0] == ACK:
            print("[+] Erase successful")
            return True
        print("[!] Erase failed")
        return False

    def write_memory(self, addr, data):
        """Write data to flash memory"""
        if not self.send_cmd(CMD_WRITE):
            return False

        # Send address (big-endian) + checksum
        addr_bytes = struct.pack('>I', addr)
        addr_cs = addr_bytes[0] ^ addr_bytes[1] ^ addr_bytes[2] ^ addr_bytes[3]
        self.ser.write(addr_bytes + bytes([addr_cs]))
        if self.ser.read(1)[0] != ACK:
            return False

        # Send data in chunks of 256 bytes
        total = len(data)
        written = 0

        for i in range(0, total, 256):
            chunk = data[i:i+256]
            chunk = chunk.ljust(256, b'\xFF')

            N = len(chunk) - 1
            cs = N
            for b in chunk:
                cs ^= b

            self.ser.write(bytes([N]) + chunk + bytes([cs]))
            resp = self.ser.read(1)
            if not resp or resp[0] != ACK:
                print(f"\n[!] Write failed at offset {i}")
                return False

            written += len(chunk)
            pct = (written / total) * 100
            print(f"\r[*] Writing... {pct:.1f}%", end='', flush=True)

        print()
        return True

    def go(self, addr):
        """Jump to application"""
        print(f"[*] Jumping to 0x{addr:08X}...")
        if not self.send_cmd(CMD_GO):
            return False

        addr_bytes = struct.pack('>I', addr)
        addr_cs = addr_bytes[0] ^ addr_bytes[1] ^ addr_bytes[2] ^ addr_bytes[3]
        self.ser.write(addr_bytes + bytes([addr_cs]))
        resp = self.ser.read(1)
        return resp and resp[0] == ACK

    def upload(self, firmware_path):
        """Full upload sequence"""
        print(f"[*] Opening {firmware_path}")
        with open(firmware_path, 'rb') as f:
            firmware = f.read()

        print(f"[*] Firmware size: {len(firmware)} bytes")

        # Wait for bootloader ready signal "BOOT\r\n"
        print("[*] Waiting for bootloader...")
        time.sleep(0.5)
        boot_msg = self.ser.read_all()
        if boot_msg:
            print(f"    Bootloader: {boot_msg.decode('ascii', errors='ignore')}")

        # Small delay then start sending commands
        time.sleep(0.1)

        if not self.erase_app():
            return False

        if not self.write_memory(0x08004000, firmware):
            return False

        print("[+] Write complete!")

        self.go(0x08004000)
        print("[+] Done! Jumped to application.")


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 uploader.py <firmware.bin> [port]")
        print("Example: python3 uploader.py app.bin /dev/ttyUSB0")
        sys.exit(1)

    firmware = sys.argv[1]
    port = sys.argv[2] if len(sys.argv) > 2 else '/dev/ttyUSB0'

    bl = Bootloader(port)
    bl.upload(firmware)


if __name__ == '__main__':
    main()
    