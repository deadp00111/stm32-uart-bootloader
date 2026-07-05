#!/usr/bin/env python3
"""
UART firmware uploader for STM32F411 bootloader.
Frame format: [0xAA][LEN_H][LEN_L][DATA...N][CRC32 4B big-endian]
"""
import argparse
import struct
import sys
import zlib

import serial

FRAME_START = 0xAA
ACK = 0x06
NACK = 0x15
CHUNK_SIZE = 256
MAX_RETRIES = 5


def build_frame(chunk: bytes) -> bytes:
    crc = zlib.crc32(chunk) & 0xFFFFFFFF
    return bytes([FRAME_START]) + struct.pack(">H", len(chunk)) + chunk + struct.pack(">I", crc)


def send_firmware(port: str, filepath: str, baud: int = 115200):
    with open(filepath, "rb") as f:
        data = f.read()

    total_chunks = (len(data) + CHUNK_SIZE - 1) // CHUNK_SIZE
    print(f"Loaded {filepath}: {len(data)} bytes, {total_chunks} chunks")

    ser = serial.Serial(port, baud, timeout=2)

    for i in range(0, len(data), CHUNK_SIZE):
        chunk = data[i:i + CHUNK_SIZE]
        frame = build_frame(chunk)

        for attempt in range(1, MAX_RETRIES + 1):
            ser.write(frame)
            resp = ser.read(1)
            if resp == bytes([ACK]):
                break
            if resp == bytes([NACK]):
                print(f"  chunk {i // CHUNK_SIZE}: NACK, retry {attempt}/{MAX_RETRIES}")
                continue
            print(f"  chunk {i // CHUNK_SIZE}: no response (timeout), retry {attempt}/{MAX_RETRIES}")
        else:
            ser.close()
            sys.exit(f"FAILED at chunk {i // CHUNK_SIZE} after {MAX_RETRIES} retries")

        pct = (i + len(chunk)) * 100 // len(data)
        print(f"\r  progress: {pct}%", end="", flush=True)

    print("\nFirmware upload complete.")
    ser.close()


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="STM32 UART bootloader uploader")
    ap.add_argument("--port", required=True, help="e.g. /dev/ttyUSB0")
    ap.add_argument("--file", required=True, help="path to app.bin")
    ap.add_argument("--baud", type=int, default=115200)
    args = ap.parse_args()

    send_firmware(args.port, args.file, args.baud)
