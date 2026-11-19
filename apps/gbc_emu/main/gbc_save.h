/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_save.h
 * @brief Battery RAM and the MBC3 clock, as files on the microSD card.
 */
#ifndef GBC_SAVE_H_
#define GBC_SAVE_H_

#include <stdint.h>

#include "kode_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Calls ---- */

/**
 * @brief Load the save that belongs to a ROM name, or a zeroed RAM image.
 *
 * Files are sd/gbc/saves/<name>.sav and sd/gbc/saves/<name>.rtc. The RTC
 * file is 8 little-endian bytes, Unix seconds UTC. A missing file is not
 * an error: RAM comes back zeroed and the clock comes back as -1.
 *
 * Blocks. Call it from the worker, never from an Active Object.
 *
 * @param rom_name  The file name, not a path, such as "game.gbc".
 * @param ram_bytes Cartridge RAM size from the header. 0 skips the RAM file.
 * @param ram       Receives a PSRAM block of ram_bytes, or NULL when ram_bytes is 0.
 *                  The block stays for the life of the app.
 * @param unix_sec  Receives the saved clock, or -1 when there is none.
 * @return KODE_OK, KODE_DENIED for a bad argument or a path that does not fit,
 *         KODE_NO_RESOURCES when PSRAM will not hold the RAM.
 */
kode_status_t gbc_save_load(const char *rom_name, uint32_t ram_bytes,
                            uint8_t **ram, int64_t *unix_sec);

/**
 * @brief Write the RAM image and the clock back.
 *
 * Blocks. The folders must already exist; gbc_save_load() creates them.
 *
 * @param rom_name  As for gbc_save_load().
 * @param ram       The RAM image. NULL only when ram_bytes is 0.
 * @param ram_bytes Bytes of ram.
 * @param unix_sec  The clock to store. -1 writes no RTC file.
 * @return KODE_OK, or KODE_ERROR when a write did not finish, or KODE_DENIED
 *         for a bad argument.
 */
kode_status_t gbc_save_store(const char *rom_name, const uint8_t *ram,
                             uint32_t ram_bytes, int64_t unix_sec);

#ifdef __cplusplus
}
#endif

#endif /* GBC_SAVE_H_ */
