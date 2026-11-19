/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_file.h
 * @brief Scan sd/doom and sd/doom/pwads for games to show.
 */
#ifndef DOOM_FILE_H_
#define DOOM_FILE_H_

#include <stdbool.h>
#include <stdint.h>

#include "doom_wad.h"
#include "kode_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DOOM_DIR       "sd/doom"
#define DOOM_PWAD_DIR  "sd/doom/pwads"
#define DOOM_NAME_MAX  64U
#define DOOM_TITLE_MAX 24U
#define DOOM_ART_MAX   12U
#define DOOM_CATALOG_MAX 16U

/** @brief One game the select screen may show. Hidden files are not stored. */
typedef struct {
    char name[DOOM_NAME_MAX];
    char title[DOOM_TITLE_MAX];
    char art[DOOM_ART_MAX];     /**< Lump to decode later, or empty. */
    doom_engine_t engine;
    doom_role_t role;
    doom_base_t base;
} doom_entry_t;

typedef struct {
    doom_entry_t items[DOOM_CATALOG_MAX];
    uint8_t count;
    uint8_t preferred;   /**< Index of the saved name, or 0xFF. */
    bool no_card;
    bool truncated;
    bool loose_only;     /**< Files were seen, and none of them is a button. */
} doom_catalog_t;

/**
 * @brief The worker job: list the card and publish a catalog.
 * A missing card is KODE_OK with no_card set.
 */
kode_status_t doom_file_boot_job(void *arg, uint32_t *value);

/**
 * @brief The published catalog.
 * @param gen Receives 0 until the first scan finishes. May be NULL.
 * @return A stable snapshot. Never NULL.
 */
const doom_catalog_t *doom_file_catalog(uint32_t *gen);

#ifdef __cplusplus
}
#endif

#endif /* DOOM_FILE_H_ */
