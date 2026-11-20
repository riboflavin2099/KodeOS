/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_core.h
 * @brief Walnut-CGB: one guest frame into a 160x144 RGB565 buffer.
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
 * Applied when the core is opened. -1 leaves the emulated clock at reset.
 *
 * @param unix_sec Seconds since 1970-01-01 UTC, or -1 when the clock is unset.
 */
void gbc_core_set_clock(int64_t unix_sec);

/**
 * @brief Start Walnut on this image. Call again to switch cartridges.
 *
 * The ROM and RAM pointers must stay valid. RAM may be NULL when ram_len is 0.
 * The Game Boy context itself is allocated here (PSRAM on the device).
 *
 * @param rom      Cartridge image.
 * @param rom_len  Bytes at rom.
 * @param ram      Cartridge RAM, or NULL.
 * @param ram_len  Bytes at ram.
 * @return true when gb_init accepted the image.
 */
bool gbc_core_open(const uint8_t *rom, uint32_t rom_len,
                   uint8_t *ram, uint32_t ram_len);

/**
 * @brief Paint one 160x144 RGB565 frame into lcd.
 *
 * With no ROM, paints the idle border. With a ROM, runs one guest frame
 * (and the APU) when the core is open. On the device, a ROM that is not
 * open yet is opened from the save RAM gbc_rom_ram() published.
 *
 * @param lcd      GBC_LCD_PIXELS of RGB565. NULL is ignored.
 * @param pad      The controls. NULL is nothing held.
 * @param rom      The cartridge image, or NULL when none is loaded.
 * @param rom_len  Bytes at rom.
 */
void gbc_core_draw(uint16_t *lcd, const gbc_pad_t *pad,
                   const uint8_t *rom, uint32_t rom_len);

/**
 * @brief Timing of gb_run_frame plus the resampler, not the panel flip.
 */
typedef struct {
    uint32_t frames;       /**< Guest frames since open. */
    uint32_t last_us;      /**< The latest frame. */
    uint32_t avg_us;       /**< Mean since open. */
    uint32_t worst_us;     /**< Slowest since open. */
    uint32_t audio_frames; /**< Mono samples handed to the speaker sink. */
    bool     running;      /**< A cartridge is open. */
    bool     gb_external;  /**< The gb_s allocation is outside internal RAM. */
    bool     rom_external; /**< The ROM pointer is outside internal RAM. */
    bool     lcd_external; /**< The frame buffer is outside internal RAM. */
    bool     ram_external; /**< The cart RAM pointer is outside internal RAM. */
} gbc_core_stats_t;

/**
 * @brief Copy the counters.
 * @param out Filled. NULL is ignored.
 */
void gbc_core_stats(gbc_core_stats_t *out);

/**
 * @brief Whether a save should be written.
 *
 * True when cart RAM has been written, or a clock was supplied (so the
 * .rtc file can move).
 *
 * @return true when gbc_save_store() has something to do.
 */
bool gbc_core_save_pending(void);

/**
 * @brief Forget the RAM-dirty flag after a successful store.
 */
void gbc_core_save_clear(void);

/**
 * @brief Cartridge RAM the core is writing.
 * @param len Receives the size. May be NULL.
 * @return The bytes, or NULL when the cart has none or the core is closed.
 */
uint8_t *gbc_core_cart_ram(uint32_t *len);

/**
 * @brief Clock to store: the value from gbc_core_set_clock plus emulated seconds.
 * @return Unix seconds, or -1 when no clock was supplied.
 */
int64_t gbc_core_clock(void);

#ifdef __cplusplus
}
#endif

#endif /* GBC_CORE_H_ */
