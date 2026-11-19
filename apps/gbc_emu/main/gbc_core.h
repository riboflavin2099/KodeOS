/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_core.h
 * @brief The emulator core's seat. Walnut-CGB is vendored and not linked yet.
 */
#ifndef GBC_CORE_H_
#define GBC_CORE_H_

#include <stdbool.h>
#include <stdint.h>

#include "gbc_input.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Calls ---- */

/**
 * @brief Remember the wall clock for an MBC3 cart.
 *
 * Stored only. Walnut's gb_set_rtc() is the call that will consume it.
 *
 * @param unix_sec Seconds since 1970-01-01 UTC, or -1 when the clock is unset.
 */
void gbc_core_set_clock(int64_t unix_sec);

/**
 * @brief Paint one 160x144 RGB565 frame into lcd.
 *
 * With no ROM, paints a moving border so the panel path is visible. With a
 * ROM, paints a stand-in and lights a corner while A is held. Neither path
 * runs Walnut. The header to include, when that changes, is
 * third_party/walnut-cgb/walnut_cgb.h, from one .c file, with ENABLE_LCD
 * left at its default and the ROM callbacks pointed at the PSRAM image.
 *
 * @param lcd      GBC_LCD_PIXELS of RGB565. NULL is ignored.
 * @param pad      The controls. NULL is treated as nothing held.
 * @param rom      The cartridge image, or NULL when none is loaded.
 * @param rom_len  Bytes at rom.
 */
void gbc_core_draw(uint16_t *lcd, const gbc_pad_t *pad, const uint8_t *rom, uint32_t rom_len);

#ifdef __cplusplus
}
#endif

#endif /* GBC_CORE_H_ */
