/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_rom.h
 * @brief Load one cartridge image from the microSD card into PSRAM.
 */
#ifndef GBC_ROM_H_
#define GBC_ROM_H_

#include <stdint.h>

#include "kode_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- The card layout ---- */

/** @brief Folder the user copies .gb and .gbc files into. */
#define GBC_ROM_DIR  "sd/gbc"

/* ---- Calls ---- */

/**
 * @brief The worker job: mount nothing itself, list sd/gbc, load the first ROM.
 *
 * A missing card or an empty folder is KODE_OK with value 0. The frame task
 * keeps drawing the idle picture. Blocks on the file calls, so it belongs
 * on the worker and not in an Active Object step.
 *
 * @param arg   Unused. The worker passes whatever was submitted.
 * @param value Receives the ROM size in bytes, or 0 when nothing was loaded.
 * @return KODE_OK when the card was read, including when it held no ROM;
 *         KODE_NO_RESOURCES when the image does not fit in PSRAM;
 *         KODE_ERROR when a read failed after a file was chosen.
 */
kode_status_t gbc_rom_boot_job(void *arg, uint32_t *value);

/**
 * @brief The published image, if the boot job has finished loading one.
 *
 * The pointer stays valid for the life of the app. Call after an acquire
 * of the flag this function takes; the bytes are then stable.
 *
 * @param len Receives the size. May be NULL.
 * @return The image, or NULL when none is published.
 */
const uint8_t *gbc_rom_data(uint32_t *len);

#ifdef __cplusplus
}
#endif

#endif /* GBC_ROM_H_ */
