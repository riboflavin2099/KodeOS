#!/usr/bin/env python3
"""Write host/roms/scroll.gb, a DMG program that scrolls a stripe.

The bytes are original to this repository. See roms/LICENSE.
"""
import pathlib

ROM_SIZE = 32 * 1024


def main() -> None:
    rom = bytearray(ROM_SIZE)
    rom[0x100] = 0x00
    rom[0x101] = 0xC3
    rom[0x102] = 0x50
    rom[0x103] = 0x01
    title = b"SCROLL"
    rom[0x134:0x134 + len(title)] = title
    rom[0x147] = 0x00
    rom[0x148] = 0x00
    rom[0x149] = 0x00
    code = bytes((
        0xF3,
        0x3E, 0x00,
        0xE0, 0x40,
        0x21, 0x00, 0x80,
        0x06, 0x10,
        0x3E, 0xF0,
        0x77,
        0x23,
        0x05,
        0x20, 0xFB,
        0x3E, 0xE4,
        0xE0, 0x47,
        0x3E, 0x91,
        0xE0, 0x40,
        0xF0, 0x44,
        0xFE, 0x90,
        0x20, 0xFA,
        0xF0, 0x43,
        0x3C,
        0xE0, 0x43,
        0x18, 0xF3,
    ))
    rom[0x150:0x150 + len(code)] = code
    checksum = 0
    for i in range(0x134, 0x14D):
        checksum = (checksum - rom[i] - 1) & 0xFF
    rom[0x14D] = checksum
    out = pathlib.Path(__file__).resolve().parent / "roms" / "scroll.gb"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(rom)
    print(f"wrote {out} checksum 0x{checksum:02x} code {len(code)} bytes")


if __name__ == "__main__":
    main()
