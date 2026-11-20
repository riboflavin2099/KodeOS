/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_wad.c
 * @brief IWAD and campaign names.
 *
 * File names follow Chocolate Doom's table at a2dfd51, plus the 2024
 * Doom + Doom II re-release files (sigil.wad, sigil2.wad, id1.wad,
 * nerve.wad, masterlevels.wad, iddm1.wad) and the resource packs that
 * sit beside them and are not games. Sizes are the published v1.9 and
 * v1.1 lengths, and the three id1.wad lengths on DoomWiki.
 */
#include "doom_wad.h"

#include <string.h>

/** @brief One known file name. */
typedef struct {
    const char *file;
    doom_engine_t engine;
    doom_role_t role;
    doom_base_t base;
    const char *title;
    uint8_t refine; /**< 1: doom.wad lumps. 2: heretic.wad lumps. */
} wad_name_t;

static const wad_name_t k_names[] = {
    { "doom1.wad",      DOOM_ENGINE_DOOM,    DOOM_ROLE_IWAD,   DOOM_BASE_DOOM_SW,  "DOOM SHARE",      0 },
    { "doom.wad",       DOOM_ENGINE_DOOM,    DOOM_ROLE_IWAD,   DOOM_BASE_DOOMU,    "ULTIMATE DOOM",   1 },
    { "doom2.wad",      DOOM_ENGINE_DOOM2,   DOOM_ROLE_IWAD,   DOOM_BASE_DOOM2,    "DOOM II",         0 },
    { "doom2f.wad",     DOOM_ENGINE_DOOM2,   DOOM_ROLE_IWAD,   DOOM_BASE_DOOM2,    "DOOM II",         0 },
    { "tnt.wad",        DOOM_ENGINE_DOOM2,   DOOM_ROLE_IWAD,   DOOM_BASE_TNT,      "TNT",             0 },
    { "plutonia.wad",   DOOM_ENGINE_DOOM2,   DOOM_ROLE_IWAD,   DOOM_BASE_PLUTONIA, "PLUTONIA",        0 },
    { "freedoom1.wad",  DOOM_ENGINE_DOOM,    DOOM_ROLE_IWAD,   DOOM_BASE_DOOMU,    "FREEDOOM 1",      0 },
    { "freedoom2.wad",  DOOM_ENGINE_DOOM2,   DOOM_ROLE_IWAD,   DOOM_BASE_DOOM2,    "FREEDOOM 2",      0 },
    { "freedm.wad",     DOOM_ENGINE_DOOM2,   DOOM_ROLE_IWAD,   DOOM_BASE_DOOM2,    "FREEDM",          0 },
    { "heretic1.wad",   DOOM_ENGINE_HERETIC, DOOM_ROLE_IWAD,   DOOM_BASE_HERETIC,  "HERETIC SW",      0 },
    { "heretic.wad",    DOOM_ENGINE_HERETIC, DOOM_ROLE_IWAD,   DOOM_BASE_HERETIC,  "HERETIC",         2 },
    { "hexen.wad",      DOOM_ENGINE_HEXEN,   DOOM_ROLE_IWAD,   DOOM_BASE_HEXEN,    "HEXEN",           0 },
    { "chex.wad",       DOOM_ENGINE_NONE,    DOOM_ROLE_HIDDEN, DOOM_BASE_NONE,     "CHEX QUEST",      0 },
    { "hacx.wad",       DOOM_ENGINE_NONE,    DOOM_ROLE_HIDDEN, DOOM_BASE_NONE,     "HACX",            0 },
    { "strife1.wad",    DOOM_ENGINE_NONE,    DOOM_ROLE_HIDDEN, DOOM_BASE_NONE,     "STRIFE",          0 },
    { "sigil.wad",      DOOM_ENGINE_DOOM,    DOOM_ROLE_ADDON,  DOOM_BASE_DOOMU,    "SIGIL",           0 },
    { "sigil_compat.wad", DOOM_ENGINE_DOOM,  DOOM_ROLE_ADDON,  DOOM_BASE_DOOMU,    "SIGIL",           0 },
    { "sigil2.wad",     DOOM_ENGINE_DOOM,    DOOM_ROLE_ADDON,  DOOM_BASE_DOOMU,    "SIGIL II",        0 },
    { "sigil_ii.wad",   DOOM_ENGINE_DOOM,    DOOM_ROLE_ADDON,  DOOM_BASE_DOOMU,    "SIGIL II",        0 },
    { "sigilii.wad",    DOOM_ENGINE_DOOM,    DOOM_ROLE_ADDON,  DOOM_BASE_DOOMU,    "SIGIL II",        0 },
    { "id1.wad",        DOOM_ENGINE_DOOM2,   DOOM_ROLE_ADDON,  DOOM_BASE_DOOM2,    "LEGACY OF RUST",  0 },
    { "nerve.wad",      DOOM_ENGINE_DOOM2,   DOOM_ROLE_ADDON,  DOOM_BASE_DOOM2,    "NO REST",         0 },
    { "masterlevels.wad", DOOM_ENGINE_DOOM2, DOOM_ROLE_ADDON,  DOOM_BASE_DOOM2,    "MASTER LEVELS",   0 },
    { "iddm1.wad",      DOOM_ENGINE_DOOM2,   DOOM_ROLE_ADDON,  DOOM_BASE_DOOM2,    "ID DEATHMATCH",   0 },
    { "id1-mus.wad",    DOOM_ENGINE_NONE,    DOOM_ROLE_HIDDEN, DOOM_BASE_NONE,     "ID1 MUSIC",       0 },
    { "id1-res.wad",    DOOM_ENGINE_NONE,    DOOM_ROLE_HIDDEN, DOOM_BASE_NONE,     "ID1 RESOURCES",   0 },
    { "id1-tex.wad",    DOOM_ENGINE_NONE,    DOOM_ROLE_HIDDEN, DOOM_BASE_NONE,     "ID1 TEXTURES",    0 },
    { "id1-weap.wad",   DOOM_ENGINE_NONE,    DOOM_ROLE_HIDDEN, DOOM_BASE_NONE,     "ID1 WEAPONS",     0 },
    { "id24res.wad",    DOOM_ENGINE_NONE,    DOOM_ROLE_HIDDEN, DOOM_BASE_NONE,     "ID24 RESOURCES",  0 },
    { "extras.wad",     DOOM_ENGINE_NONE,    DOOM_ROLE_HIDDEN, DOOM_BASE_NONE,     "EXTRAS",          0 },
};

/** @brief A published length. Used only when the name and the lumps do not decide. */
typedef struct {
    uint32_t bytes;
    doom_engine_t engine;
    doom_role_t role;
    doom_base_t base;
    const char *title;
} wad_size_t;

static const wad_size_t k_sizes[] = {
    { 4196024u,  DOOM_ENGINE_DOOM,  DOOM_ROLE_IWAD,  DOOM_BASE_DOOM_SW,  "DOOM SHARE" },
    { 12408292u, DOOM_ENGINE_DOOM,  DOOM_ROLE_IWAD,  DOOM_BASE_DOOMU,    "ULTIMATE DOOM" },
    { 14604584u, DOOM_ENGINE_DOOM2, DOOM_ROLE_IWAD,  DOOM_BASE_DOOM2,    "DOOM II" },
    { 17420824u, DOOM_ENGINE_DOOM2, DOOM_ROLE_IWAD,  DOOM_BASE_PLUTONIA, "PLUTONIA" },
    { 18195736u, DOOM_ENGINE_DOOM2, DOOM_ROLE_IWAD,  DOOM_BASE_TNT,      "TNT" },
    { 20083672u, DOOM_ENGINE_HEXEN, DOOM_ROLE_IWAD,  DOOM_BASE_HEXEN,    "HEXEN" },
    /* id1.wad, the three lengths DoomWiki lists for Legacy of Rust. */
    { 34793603u, DOOM_ENGINE_DOOM2, DOOM_ROLE_ADDON, DOOM_BASE_DOOM2,    "LEGACY OF RUST" },
    { 34816611u, DOOM_ENGINE_DOOM2, DOOM_ROLE_ADDON, DOOM_BASE_DOOM2,    "LEGACY OF RUST" },
    { 34816890u, DOOM_ENGINE_DOOM2, DOOM_ROLE_ADDON, DOOM_BASE_DOOM2,    "LEGACY OF RUST" },
};

/**
 * @brief ASCII upper case.
 */
static char upper_ascii(char c)
{
    char out = c;

    if ( ('a' <= c) && ('z' >= c) )
    {
        out = (char)(c - ('a' - 'A'));
    }
    return out;
}

/**
 * @brief The last path segment. Never NULL.
 */
static const char *base_name(const char *path)
{
    const char *base = "";
    const char *s = NULL;

    if ( NULL != path )
    {
        base = path;
        for ( s = path; '\0' != *s; s++ )
        {
            if ( ('/' == *s) || ('\\' == *s) )
            {
                base = s + 1;
            }
        }
    }
    return base;
}

/**
 * @brief Full base-name match, ASCII case ignored.
 */
static bool name_is(const char *filename, const char *want)
{
    const char *a = base_name(filename);
    const char *b = want;
    bool match = true;

    while ( match && (('\0' != *a) || ('\0' != *b)) )
    {
        if ( upper_ascii(*a) != upper_ascii(*b) )
        {
            match = false;
        }
        else
        {
            if ( '\0' != *a )
            {
                a++;
            }
            if ( '\0' != *b )
            {
                b++;
            }
        }
    }
    return match;
}

static uint32_t read_u32(const uint8_t *p)
{
    return (uint32_t)p[0]
           | ((uint32_t)p[1] << 8)
           | ((uint32_t)p[2] << 16)
           | ((uint32_t)p[3] << 24);
}

static bool lump_is(const char raw[8], const char *want)
{
    size_t const n = strlen(want);
    size_t i = 0U;
    bool match = true;

    if ( 8U < n )
    {
        match = false;
    }
    for ( i = 0U; match && (n > i); i++ )
    {
        if ( upper_ascii(raw[i]) != upper_ascii(want[i]) )
        {
            match = false;
        }
    }
    for ( ; match && (8U > i); i++ )
    {
        if ( ('\0' != raw[i]) && (' ' != raw[i]) )
        {
            match = false;
        }
    }
    return match;
}

static void fill_id(doom_id_t *id, doom_engine_t engine, doom_role_t role,
                    doom_base_t base, const char *title)
{
    id->engine = engine;
    id->role = role;
    id->base = base;
    id->title = title;
    id->show = (DOOM_ROLE_HIDDEN != role);
}

static void identify_lumps(uint32_t lumps, doom_wad_kind_t kind, doom_id_t *id)
{
    bool const e1 = (0U != (lumps & DOOM_LUMP_E1M1));
    bool const e2 = (0U != (lumps & DOOM_LUMP_E2M1));
    bool const e3 = (0U != (lumps & DOOM_LUMP_E3M1));
    bool const e4 = (0U != (lumps & DOOM_LUMP_E4M1));
    bool const e5 = (0U != (lumps & DOOM_LUMP_E5M1));
    bool const e6 = (0U != (lumps & DOOM_LUMP_E6M1));
    bool const map01 = (0U != (lumps & DOOM_LUMP_MAP01));
    bool const d_e1 = (0U != (lumps & DOOM_LUMP_D_E1M1));
    bool const runnin = (0U != (lumps & DOOM_LUMP_D_RUNNIN));
    bool const mus = (0U != (lumps & DOOM_LUMP_MUS_E1M1));
    bool const behavior = (0U != (lumps & DOOM_LUMP_BEHAVIOR));
    bool const camo = (0U != (lumps & DOOM_LUMP_CAMO1));
    bool const tnt = (0U != (lumps & DOOM_LUMP_REDTNT2));
    bool const extended = (0U != (lumps & DOOM_LUMP_EXTENDED));

    if ( DOOM_WAD_PWAD == kind )
    {
        if ( e6 )
        {
            fill_id(id, DOOM_ENGINE_DOOM, DOOM_ROLE_ADDON, DOOM_BASE_DOOMU, "SIGIL II");
        }
        else if ( e5 )
        {
            fill_id(id, DOOM_ENGINE_DOOM, DOOM_ROLE_ADDON, DOOM_BASE_DOOMU, "SIGIL");
        }
    }
    else if ( behavior && map01 )
    {
        fill_id(id, DOOM_ENGINE_HEXEN, DOOM_ROLE_IWAD, DOOM_BASE_HEXEN, "HEXEN");
    }
    else if ( mus || (e1 && !d_e1 && !map01 && !runnin) )
    {
        if ( extended )
        {
            fill_id(id, DOOM_ENGINE_HERETIC, DOOM_ROLE_IWAD, DOOM_BASE_HERETIC,
                    "HERETIC EXT");
        }
        else if ( e1 && !e2 )
        {
            fill_id(id, DOOM_ENGINE_HERETIC, DOOM_ROLE_IWAD, DOOM_BASE_HERETIC,
                    "HERETIC SW");
        }
        else
        {
            fill_id(id, DOOM_ENGINE_HERETIC, DOOM_ROLE_IWAD, DOOM_BASE_HERETIC,
                    "HERETIC");
        }
    }
    else if ( map01 && camo )
    {
        fill_id(id, DOOM_ENGINE_DOOM2, DOOM_ROLE_IWAD, DOOM_BASE_PLUTONIA, "PLUTONIA");
    }
    else if ( map01 && tnt )
    {
        fill_id(id, DOOM_ENGINE_DOOM2, DOOM_ROLE_IWAD, DOOM_BASE_TNT, "TNT");
    }
    else if ( map01 && (runnin || !e1) )
    {
        fill_id(id, DOOM_ENGINE_DOOM2, DOOM_ROLE_IWAD, DOOM_BASE_DOOM2, "DOOM II");
    }
    else if ( e4 )
    {
        fill_id(id, DOOM_ENGINE_DOOM, DOOM_ROLE_IWAD, DOOM_BASE_DOOMU, "ULTIMATE DOOM");
    }
    else if ( e2 || e3 )
    {
        fill_id(id, DOOM_ENGINE_DOOM, DOOM_ROLE_IWAD, DOOM_BASE_DOOM, "DOOM");
    }
    else if ( e1 && d_e1 )
    {
        fill_id(id, DOOM_ENGINE_DOOM, DOOM_ROLE_IWAD, DOOM_BASE_DOOM_SW, "DOOM SHARE");
    }
}

static void identify_size(uint32_t bytes, doom_wad_kind_t kind, doom_id_t *id)
{
    size_t i = 0U;

    if ( 0U == bytes )
    {
        return;
    }
    for ( i = 0U; (sizeof(k_sizes) / sizeof(k_sizes[0])) > i; i++ )
    {
        if ( k_sizes[i].bytes != bytes )
        {
            continue;
        }
        if ( (DOOM_ROLE_IWAD == k_sizes[i].role) && (DOOM_WAD_IWAD != kind) )
        {
            continue;
        }
        fill_id(id, k_sizes[i].engine, k_sizes[i].role, k_sizes[i].base,
                k_sizes[i].title);
        break;
    }
}

static void refine_named(const wad_name_t *row, uint32_t lumps, doom_id_t *id)
{
    bool const e1 = (0U != (lumps & DOOM_LUMP_E1M1));
    bool const e2 = (0U != (lumps & DOOM_LUMP_E2M1));
    bool const e3 = (0U != (lumps & DOOM_LUMP_E3M1));
    bool const e4 = (0U != (lumps & DOOM_LUMP_E4M1));

    if ( 1U == row->refine )
    {
        if ( e4 )
        {
            fill_id(id, DOOM_ENGINE_DOOM, DOOM_ROLE_IWAD, DOOM_BASE_DOOMU, "ULTIMATE DOOM");
        }
        else if ( e1 && !e2 && !e3 )
        {
            fill_id(id, DOOM_ENGINE_DOOM, DOOM_ROLE_IWAD, DOOM_BASE_DOOM_SW, "DOOM SHARE");
        }
        else if ( (e2 || e3) && !e4 )
        {
            fill_id(id, DOOM_ENGINE_DOOM, DOOM_ROLE_IWAD, DOOM_BASE_DOOM, "DOOM");
        }
    }
    else if ( 2U == row->refine )
    {
        if ( 0U != (lumps & DOOM_LUMP_EXTENDED) )
        {
            id->title = "HERETIC EXT";
        }
        else if ( e1 && !e2 )
        {
            id->title = "HERETIC SW";
        }
    }
}

doom_wad_kind_t doom_wad_kind(const uint8_t *hdr, size_t len,
                              uint32_t *nlumps, uint32_t *dir_off)
{
    doom_wad_kind_t kind = DOOM_WAD_BAD;
    uint32_t lumps = 0U;
    uint32_t off = 0U;

    if ( (NULL != hdr) && (12U <= len) )
    {
        lumps = read_u32(&hdr[4]);
        off = read_u32(&hdr[8]);
        if ( 0 == memcmp(hdr, "IWAD", 4) )
        {
            kind = DOOM_WAD_IWAD;
        }
        else if ( 0 == memcmp(hdr, "PWAD", 4) )
        {
            kind = DOOM_WAD_PWAD;
        }
    }
    if ( NULL != nlumps )
    {
        *nlumps = lumps;
    }
    if ( NULL != dir_off )
    {
        *dir_off = off;
    }
    return kind;
}

uint32_t doom_wad_note_lump(uint32_t bits, const char name[8])
{
    uint32_t out = bits;

    if ( NULL == name )
    {
        return out;
    }
    if ( lump_is(name, "E1M1") )
    {
        out |= DOOM_LUMP_E1M1;
    }
    else if ( lump_is(name, "E2M1") )
    {
        out |= DOOM_LUMP_E2M1;
    }
    else if ( lump_is(name, "E3M1") )
    {
        out |= DOOM_LUMP_E3M1;
    }
    else if ( lump_is(name, "E4M1") )
    {
        out |= DOOM_LUMP_E4M1;
    }
    else if ( lump_is(name, "E5M1") )
    {
        out |= DOOM_LUMP_E5M1;
    }
    else if ( lump_is(name, "E6M1") )
    {
        out |= DOOM_LUMP_E6M1;
    }
    else if ( lump_is(name, "MAP01") )
    {
        out |= DOOM_LUMP_MAP01;
    }
    else if ( lump_is(name, "D_E1M1") )
    {
        out |= DOOM_LUMP_D_E1M1;
    }
    else if ( lump_is(name, "D_RUNNIN") )
    {
        out |= DOOM_LUMP_D_RUNNIN;
    }
    else if ( lump_is(name, "MUS_E1M1") )
    {
        out |= DOOM_LUMP_MUS_E1M1;
    }
    else if ( lump_is(name, "BEHAVIOR") )
    {
        out |= DOOM_LUMP_BEHAVIOR;
    }
    else if ( lump_is(name, "CAMO1") )
    {
        out |= DOOM_LUMP_CAMO1;
    }
    else if ( lump_is(name, "REDTNT2") )
    {
        out |= DOOM_LUMP_REDTNT2;
    }
    else if ( lump_is(name, "EXTENDED") )
    {
        out |= DOOM_LUMP_EXTENDED;
    }
    else if ( lump_is(name, "TITLEPIC") )
    {
        out |= DOOM_LUMP_TITLEPIC;
    }
    else if ( lump_is(name, "M_DOOM") )
    {
        out |= DOOM_LUMP_M_DOOM;
    }
    else if ( lump_is(name, "M_HTIC") )
    {
        out |= DOOM_LUMP_M_HTIC;
    }
    else if ( lump_is(name, "TITLE") )
    {
        out |= DOOM_LUMP_TITLE;
    }
    return out;
}

uint32_t doom_wad_note_entry(uint32_t bits, const uint8_t entry[16])
{
    uint32_t out = bits;

    if ( NULL != entry )
    {
        out = doom_wad_note_lump(bits, (const char *)&entry[8]);
    }
    return out;
}

doom_id_t doom_wad_identify(const char *filename, doom_wad_kind_t kind,
                            uint32_t lumps, uint32_t bytes)
{
    doom_id_t id;
    size_t i = 0U;
    bool named = false;

    id.engine = DOOM_ENGINE_NONE;
    id.role = DOOM_ROLE_HIDDEN;
    id.base = DOOM_BASE_NONE;
    id.title = "UNKNOWN";
    id.show = false;
    if ( DOOM_WAD_BAD == kind )
    {
        return id;
    }
    for ( i = 0U; (sizeof(k_names) / sizeof(k_names[0])) > i; i++ )
    {
        if ( name_is(filename, k_names[i].file) )
        {
            fill_id(&id, k_names[i].engine, k_names[i].role, k_names[i].base,
                    k_names[i].title);
            refine_named(&k_names[i], lumps, &id);
            /* A PWAD that borrowed an IWAD's name is not that game. */
            if ( (DOOM_WAD_PWAD == kind) && (DOOM_ROLE_IWAD == id.role) )
            {
                fill_id(&id, DOOM_ENGINE_NONE, DOOM_ROLE_HIDDEN, DOOM_BASE_NONE,
                        "ADD-ON");
            }
            named = true;
            break;
        }
    }
    if ( !named )
    {
        identify_lumps(lumps, kind, &id);
    }
    if ( (DOOM_ROLE_HIDDEN == id.role) && (0 == strcmp(id.title, "UNKNOWN")) )
    {
        identify_size(bytes, kind, &id);
    }
    if ( (DOOM_ROLE_HIDDEN == id.role) && (DOOM_WAD_PWAD == kind)
        && (0 == strcmp(id.title, "UNKNOWN")) )
    {
        id.title = "ADD-ON";
    }
    return id;
}

const char *doom_wad_art_lump(const doom_id_t *id, uint32_t lumps)
{
    const char *lump = NULL;

    if ( NULL == id )
    {
        return NULL;
    }
    if ( (DOOM_ENGINE_HERETIC == id->engine) && (0U != (lumps & DOOM_LUMP_M_HTIC)) )
    {
        lump = "M_HTIC";
    }
    else if ( (DOOM_ENGINE_DOOM == id->engine) && (0U != (lumps & DOOM_LUMP_M_DOOM)) )
    {
        lump = "M_DOOM";
    }
    else if ( 0U != (lumps & DOOM_LUMP_TITLEPIC) )
    {
        lump = "TITLEPIC";
    }
    else if ( 0U != (lumps & DOOM_LUMP_TITLE) )
    {
        lump = "TITLE";
    }
    else if ( 0U != (lumps & DOOM_LUMP_M_DOOM) )
    {
        lump = "M_DOOM";
    }
    else if ( 0U != (lumps & DOOM_LUMP_M_HTIC) )
    {
        lump = "M_HTIC";
    }
    return lump;
}
