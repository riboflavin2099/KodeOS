/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_wad.h
 * @brief Name a WAD from its file name, its size, and a few lump names.
 *
 * No SDK calls. The host tests and the firmware scanner share this.
 */
#ifndef DOOM_WAD_H_
#define DOOM_WAD_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- What would run it ---- */

typedef enum {
    DOOM_ENGINE_NONE = 0,
    DOOM_ENGINE_DOOM,       /**< Doom, Ultimate Doom, Freedoom Phase 1, SIGIL. */
    DOOM_ENGINE_DOOM2,      /**< Doom II, Final Doom, Freedoom Phase 2, id1, nerve. */
    DOOM_ENGINE_HERETIC,
    DOOM_ENGINE_HEXEN
} doom_engine_t;

/** @brief The 12-byte header. */
typedef enum {
    DOOM_WAD_BAD = 0,
    DOOM_WAD_IWAD,
    DOOM_WAD_PWAD
} doom_wad_kind_t;

/**
 * @brief Whether the select screen offers this file.
 *
 * Hidden files are still named, so a log line can say what was skipped.
 */
typedef enum {
    DOOM_ROLE_HIDDEN = 0, /**< Unknown, a loose PWAD, a resource pack, or out of scope. */
    DOOM_ROLE_IWAD,       /**< Starts on its own. */
    DOOM_ROLE_ADDON       /**< A campaign that needs a base IWAD. */
} doom_role_t;

/**
 * @brief Which base game a file is, or which base an add-on needs.
 *
 * SIGIL needs Ultimate Doom or Freedoom Phase 1 (DOOM_BASE_DOOMU).
 * Legacy of Rust, No Rest, the Master Levels and the id deathmatch pack
 * need Doom II or Freedoom Phase 2 (DOOM_BASE_DOOM2). TNT and Plutonia
 * are Doom II engine games and are not that base.
 */
typedef enum {
    DOOM_BASE_NONE = 0,
    DOOM_BASE_DOOM_SW,    /**< Shareware Doom. Not a SIGIL base. */
    DOOM_BASE_DOOM,       /**< Registered Doom, no fourth episode. */
    DOOM_BASE_DOOMU,      /**< Ultimate Doom or Freedoom Phase 1. */
    DOOM_BASE_DOOM2,      /**< Doom II or Freedoom Phase 2. */
    DOOM_BASE_TNT,
    DOOM_BASE_PLUTONIA,
    DOOM_BASE_HERETIC,
    DOOM_BASE_HEXEN
} doom_base_t;

/* Lump bits. A directory walk ORs these in. */
#define DOOM_LUMP_E1M1      (1u << 0)
#define DOOM_LUMP_E2M1      (1u << 1)
#define DOOM_LUMP_E3M1      (1u << 2)
#define DOOM_LUMP_E4M1      (1u << 3)
#define DOOM_LUMP_E5M1      (1u << 4)
#define DOOM_LUMP_E6M1      (1u << 5)
#define DOOM_LUMP_MAP01     (1u << 6)
#define DOOM_LUMP_D_E1M1    (1u << 7)
#define DOOM_LUMP_D_RUNNIN  (1u << 8)
#define DOOM_LUMP_MUS_E1M1  (1u << 9)
#define DOOM_LUMP_BEHAVIOR  (1u << 10)
#define DOOM_LUMP_CAMO1     (1u << 11)
#define DOOM_LUMP_REDTNT2   (1u << 12)
#define DOOM_LUMP_EXTENDED  (1u << 13)
#define DOOM_LUMP_TITLEPIC  (1u << 14)
#define DOOM_LUMP_M_DOOM    (1u << 15)
#define DOOM_LUMP_M_HTIC    (1u << 16)
#define DOOM_LUMP_TITLE     (1u << 17)

/**
 * @brief One identified file.
 *
 * title and art_lump point at literals, or art_lump is NULL when the
 * directory did not contain a title picture this probe knows.
 */
typedef struct {
    doom_engine_t engine;
    doom_role_t role;
    doom_base_t base;
    const char *title;
    bool show;              /**< A logo button. */
} doom_id_t;

/* ---- Calls ---- */

/**
 * @brief Read the 12-byte header.
 * @param hdr     First bytes. NULL is a bad header.
 * @param len     Bytes at hdr.
 * @param nlumps  Directory count. May be NULL.
 * @param dir_off Directory offset. May be NULL.
 */
doom_wad_kind_t doom_wad_kind(const uint8_t *hdr, size_t len,
                              uint32_t *nlumps, uint32_t *dir_off);

/**
 * @brief Fold one 8-byte lump name into the bit set.
 * @param bits Collected bits.
 * @param name Eight directory bytes. NULL leaves bits unchanged.
 */
uint32_t doom_wad_note_lump(uint32_t bits, const char name[8]);

/**
 * @brief Fold the name inside one 16-byte directory entry.
 * The name is the last eight bytes.
 */
uint32_t doom_wad_note_entry(uint32_t bits, const uint8_t entry[16]);

/**
 * @brief Name the game.
 *
 * A known file name wins, then lump bits, then a published file size.
 * bytes may be 0 when the size is not known. PWAD magic is an add-on
 * or a hidden file, never an IWAD, whatever the name is, except a known
 * campaign name (SIGIL, id1.wad, nerve.wad) which stays an add-on.
 *
 * @param filename File name or path. NULL is unnamed.
 * @param kind     From doom_wad_kind().
 * @param lumps    From doom_wad_note_lump().
 * @param bytes    File length, or 0.
 */
doom_id_t doom_wad_identify(const char *filename, doom_wad_kind_t kind,
                            uint32_t lumps, uint32_t bytes);

/**
 * @brief Which lump the select screen will decode later, or NULL.
 *
 * M_DOOM for Doom, M_HTIC for Heretic, else TITLEPIC, else TITLE.
 * The picture is in the user's WAD. This tree does not contain one.
 */
const char *doom_wad_art_lump(const doom_id_t *id, uint32_t lumps);

#ifdef __cplusplus
}
#endif

#endif /* DOOM_WAD_H_ */
