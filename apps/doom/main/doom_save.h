/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_save.h
 * @brief The last game chosen, and the path of a save slot.
 *
 * Config is sd/doom/config.cfg, one line, last=<file>. Save slots, once the
 * engine can write them, are sd/doom/saves/<stem>/slotN.dsg for N in 0..5.
 */
#ifndef DOOM_SAVE_H_
#define DOOM_SAVE_H_

#include <stdbool.h>
#include <stdint.h>

#include "kode_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DOOM_SAVE_DIR  "sd/doom/saves"
#define DOOM_CFG_PATH  "sd/doom/config.cfg"

/**
 * @brief Read the last file name, or an empty string when there is none.
 * Blocks. Call it from the worker.
 */
kode_status_t doom_save_load(char *name, uint32_t cap);

/** @brief Write last=<name>. The folder sd/doom must already exist. */
kode_status_t doom_save_store(const char *name);

/**
 * @brief Build sd/doom/saves/<stem>/slotN.dsg.
 * @return false when the path does not fit or the slot is past 5.
 */
bool doom_save_slot_path(const char *wad_name, unsigned slot, char *out, uint32_t cap);

#ifdef __cplusplus
}
#endif

#endif /* DOOM_SAVE_H_ */
