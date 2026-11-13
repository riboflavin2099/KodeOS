/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_header.h
 * @brief The cartridge header at 0x100, parsed without the emulator core.
 */
#ifndef GBC_HEADER_H_
#define GBC_HEADER_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Sizes ---- */

/** @brief Bytes of ROM the header itself occupies, from address 0. */
#define GBC_HEADER_MIN_BYTES  0x150U

/** @brief Largest ROM this app will load: an MBC5 8 MiB cartridge. */
#define GBC_ROM_MAX_BYTES     (8U * 1024U * 1024U)

/** @brief Largest cartridge RAM: an MBC5 128 KiB save. */
#define GBC_RAM_MAX_BYTES     (128U * 1024U)

/** @brief Bytes of the title this parser keeps, including the terminator. */
#define GBC_TITLE_MAX         17U

/* ---- The parsed header ---- */

/**
 * @brief What the cartridge header says. Filled only on a successful parse.
 */
typedef struct {
    char     title[GBC_TITLE_MAX]; /**< Printable title, NUL terminated. */
    uint8_t  cgb_flag;             /**< Byte 0x143. 0x80 works on both, 0xC0 is CGB only. */
    uint8_t  cart_type;            /**< Byte 0x147. */
    uint8_t  rom_code;             /**< Byte 0x148. */
    uint8_t  ram_code;             /**< Byte 0x149. */
    uint32_t rom_bytes;            /**< Size the ROM-size code names. */
    uint32_t ram_bytes;            /**< Size the RAM-size code names. 0 when there is no RAM. */
    bool     cgb_only;             /**< True when cgb_flag is 0xC0. */
    bool     cgb_capable;          /**< True when cgb_flag is 0x80 or 0xC0. */
    bool     has_ram;              /**< True when ram_bytes is above 0. */
    bool     has_rtc;              /**< True for the MBC3 types that carry a clock. */
} gbc_header_t;

/* ---- Calls ---- */

/**
 * @brief Parse a cartridge image's header.
 *
 * @param rom  The first bytes of the image. Not modified.
 * @param len  How many bytes rom holds.
 * @param out  Filled on success. Untouched on failure.
 * @return true when the header is long enough and its size codes are ones
 *         this app knows; false otherwise.
 */
bool gbc_header_parse(const uint8_t *rom, uint32_t len, gbc_header_t *out);

#ifdef __cplusplus
}
#endif

#endif /* GBC_HEADER_H_ */
