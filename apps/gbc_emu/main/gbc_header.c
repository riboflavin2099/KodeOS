/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_header.c
 * @brief Cartridge header parser. No SDK calls, so the host test can link it.
 */
#include "gbc_header.h"

#include <stddef.h>

/** @brief Title field. On a CGB image the last byte is the CGB flag, not text. */
#define GBC_TITLE_OFF   0x134U

/** @brief CGB compatibility flag. */
#define GBC_CGB_OFF     0x143U

/** @brief Cartridge type. */
#define GBC_TYPE_OFF    0x147U

/** @brief ROM size code. */
#define GBC_ROM_OFF     0x148U

/** @brief RAM size code. */
#define GBC_RAM_OFF     0x149U

/** @brief CGB flag: works on a DMG and a CGB. */
#define GBC_CGB_BOTH    0x80U

/** @brief CGB flag: CGB only. */
#define GBC_CGB_ONLY    0xC0U

/** @brief MBC3 with a battery and a clock, no RAM. */
#define GBC_MBC3_RTC    0x0FU

/** @brief MBC3 with RAM, a battery and a clock. */
#define GBC_MBC3_RTC_RAM 0x10U

/**
 * @brief ROM size from the header code. Codes 0..8 are 32 KiB doubled each step.
 * @param code    Byte 0x148.
 * @param bytes   Receives the size.
 * @return true for a code this app accepts.
 */
static bool rom_bytes_of(uint8_t code, uint32_t *bytes)
{
    bool ok = false;

    if ( 8U >= code )
    {
        *bytes = 32768U << code;
        ok = true;
    }
    return ok;
}

/**
 * @brief Cartridge RAM size from the header code.
 * @param code    Byte 0x149.
 * @param bytes   Receives the size. 0 is a cart with no RAM, and it is valid.
 * @return true for a code this app accepts.
 */
static bool ram_bytes_of(uint8_t code, uint32_t *bytes)
{
    bool ok = true;

    switch ( code )
    {
        case 0U:
        {
            *bytes = 0U;
            break;
        }
        case 1U:
        {
            *bytes = 2U * 1024U;
            break;
        }
        case 2U:
        {
            *bytes = 8U * 1024U;
            break;
        }
        case 3U:
        {
            *bytes = 32U * 1024U;
            break;
        }
        case 4U:
        {
            *bytes = 128U * 1024U;
            break;
        }
        case 5U:
        {
            *bytes = 64U * 1024U;
            break;
        }
        default:
        {
            ok = false;
            break;
        }
    }
    return ok;
}

/**
 * @brief Keep one title byte when it is printable ASCII.
 * @param c The byte from the header.
 * @return The character, or 0 when it should end the title.
 */
static char title_char(uint8_t c)
{
    char out = '\0';

    if ( (' ' <= c) && (0x7EU >= c) )
    {
        out = (char)c;
    }
    return out;
}

bool gbc_header_parse(const uint8_t *rom, uint32_t len, gbc_header_t *out)
{
    bool ok = false;

    if ( (NULL != rom) && (NULL != out) && (GBC_HEADER_MIN_BYTES <= len) )
    {
        uint32_t rom_bytes = 0U;
        uint32_t ram_bytes = 0U;
        uint8_t const cgb = rom[GBC_CGB_OFF];
        uint8_t const cart = rom[GBC_TYPE_OFF];
        bool const cgb_capable = (GBC_CGB_BOTH == cgb) || (GBC_CGB_ONLY == cgb);
        uint32_t title_n = 16U;
        uint32_t i = 0U;

        if ( rom_bytes_of(rom[GBC_ROM_OFF], &rom_bytes)
            && ram_bytes_of(rom[GBC_RAM_OFF], &ram_bytes)
            && (GBC_ROM_MAX_BYTES >= rom_bytes)
            && (GBC_RAM_MAX_BYTES >= ram_bytes) )
        {
            gbc_header_t parsed;

            if ( cgb_capable )
            {
                title_n = 15U;
            }
            for ( i = 0U; GBC_TITLE_MAX > i; i++ )
            {
                parsed.title[i] = '\0';
            }
            for ( i = 0U; title_n > i; i++ )
            {
                char const ch = title_char(rom[GBC_TITLE_OFF + i]);

                if ( '\0' == ch )
                {
                    break;
                }
                parsed.title[i] = ch;
            }
            parsed.cgb_flag = cgb;
            parsed.cart_type = cart;
            parsed.rom_code = rom[GBC_ROM_OFF];
            parsed.ram_code = rom[GBC_RAM_OFF];
            parsed.rom_bytes = rom_bytes;
            parsed.ram_bytes = ram_bytes;
            parsed.cgb_only = (GBC_CGB_ONLY == cgb);
            parsed.cgb_capable = cgb_capable;
            parsed.has_ram = (0U < ram_bytes);
            parsed.has_rtc = (GBC_MBC3_RTC == cart) || (GBC_MBC3_RTC_RAM == cart);
            *out = parsed;
            ok = true;
        }
    }
    return ok;
}
