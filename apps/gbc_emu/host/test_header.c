/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file test_header.c
 * @brief Host checks for the cartridge header parser. No SDK, no ROMs.
 */
#include <stdio.h>
#include <string.h>

#include "gbc_header.h"

static int s_failed = 0;

/**
 * @brief Count one failure.
 * @param cond True when the check passed.
 * @param what What was checked.
 */
static void expect(int cond, const char *what)
{
    if ( !cond )
    {
        fprintf(stderr, "FAIL %s\n", what);
        s_failed++;
    }
}

/**
 * @brief A header-sized buffer with the fields this test sets.
 * @param rom      At least GBC_HEADER_MIN_BYTES.
 * @param title    ASCII title, may be empty.
 * @param cgb      Byte 0x143.
 * @param cart     Byte 0x147.
 * @param rom_code Byte 0x148.
 * @param ram_code Byte 0x149.
 */
static void paint(uint8_t *rom, const char *title, uint8_t cgb, uint8_t cart,
                  uint8_t rom_code, uint8_t ram_code)
{
    size_t n = strlen(title);
    size_t i = 0U;

    memset(rom, 0, GBC_HEADER_MIN_BYTES);
    for ( i = 0U; (n > i) && (15U > i); i++ )
    {
        rom[0x134U + i] = (uint8_t)title[i];
    }
    rom[0x143U] = cgb;
    rom[0x147U] = cart;
    rom[0x148U] = rom_code;
    rom[0x149U] = ram_code;
}

int main(void)
{
    uint8_t rom[GBC_HEADER_MIN_BYTES];
    gbc_header_t hdr;

    expect(!gbc_header_parse(NULL, GBC_HEADER_MIN_BYTES, &hdr), "NULL image");
    expect(!gbc_header_parse(rom, GBC_HEADER_MIN_BYTES - 1U, &hdr), "short image");

    paint(rom, "TEST", 0x00U, 0x00U, 0U, 0U);
    expect(gbc_header_parse(rom, sizeof(rom), &hdr), "plain DMG parses");
    expect(0 == strcmp(hdr.title, "TEST"), "DMG title");
    expect(32768U == hdr.rom_bytes, "32 KiB ROM");
    expect(0U == hdr.ram_bytes, "no RAM");
    expect(!hdr.cgb_capable && !hdr.has_rtc, "not CGB, no RTC");

    paint(rom, "CLOCK", 0xC0U, 0x10U, 1U, 2U);
    expect(gbc_header_parse(rom, sizeof(rom), &hdr), "MBC3+RTC parses");
    expect(0 == strcmp(hdr.title, "CLOCK"), "CGB title stops before the flag");
    expect(hdr.cgb_only && hdr.cgb_capable, "CGB only");
    expect(65536U == hdr.rom_bytes, "64 KiB ROM");
    expect(8192U == hdr.ram_bytes, "8 KiB RAM");
    expect(hdr.has_rtc && hdr.has_ram, "RTC and RAM");

    paint(rom, "BIG", 0x80U, 0x1BU, 8U, 4U);
    expect(gbc_header_parse(rom, sizeof(rom), &hdr), "8 MiB parses");
    expect((8U * 1024U * 1024U) == hdr.rom_bytes, "8 MiB size");
    expect((128U * 1024U) == hdr.ram_bytes, "128 KiB RAM");
    expect(hdr.cgb_capable && !hdr.cgb_only, "CGB and DMG");

    paint(rom, "NO", 0x00U, 0x00U, 9U, 0U);
    expect(!gbc_header_parse(rom, sizeof(rom), &hdr), "ROM code 9 is refused");

    paint(rom, "NO", 0x00U, 0x00U, 0U, 6U);
    expect(!gbc_header_parse(rom, sizeof(rom), &hdr), "RAM code 6 is refused");

    if ( 0 == s_failed )
    {
        printf("OK header\n");
        return 0;
    }
    return 1;
}
