/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_file.c
 * @brief Read WAD headers from the card and publish the games to show.
 */
#include "doom_file.h"

#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "doom_save.h"
#include "kode.h"
#include "kode_log.h"

#define TAG "doom.file"

#define DOOM_LUMP_CAP  8192U

static doom_catalog_t s_empty;
static doom_catalog_t *s_pair = NULL;
static atomic_uint s_slot = 0U;
static atomic_uint s_gen = 0U;

static bool suffix_wad(const char *name)
{
    size_t n = strlen(name);
    const char *suf = ".wad";
    size_t i = 0U;
    bool match = false;

    if ( 4U <= n )
    {
        match = true;
        for ( i = 0U; 4U > i; i++ )
        {
            char c = name[(n - 4U) + i];

            if ( ('A' <= c) && ('Z' >= c) )
            {
                c = (char)(c + ('a' - 'A'));
            }
            if ( c != suf[i] )
            {
                match = false;
            }
        }
    }
    return match;
}

static uint32_t probe_lumps(const char *path, uint32_t size, doom_wad_kind_t kind,
                            uint32_t nlumps, uint32_t dir_off)
{
    uint32_t bits = 0U;
    uint32_t cap = nlumps;
    uint32_t done = 0U;
    uint8_t chunk[512];

    if ( (DOOM_WAD_BAD == kind) || (0U == nlumps) || (dir_off >= size) )
    {
        return 0U;
    }
    if ( DOOM_LUMP_CAP < cap )
    {
        cap = DOOM_LUMP_CAP;
    }
    while ( done < cap )
    {
        uint32_t want = (cap - done) * 16U;
        uint32_t got = 0U;
        uint32_t entries = 0U;
        uint32_t e = 0U;

        if ( sizeof(chunk) < want )
        {
            want = sizeof(chunk);
        }
        want -= want % 16U;
        if ( (0U == want)
            || !kode_file_read_at(path, dir_off + (done * 16U), chunk, want, &got)
            || (16U > got) )
        {
            break;
        }
        entries = got / 16U;
        for ( e = 0U; entries > e; e++ )
        {
            bits = doom_wad_note_entry(bits, &chunk[e * 16U]);
        }
        done += entries;
    }
    return bits;
}

static bool already(const doom_catalog_t *cat, const char *name)
{
    uint8_t i = 0U;
    bool found = false;

    for ( i = 0U; cat->count > i; i++ )
    {
        if ( 0 == strcmp(cat->items[i].name, name) )
        {
            found = true;
        }
    }
    return found;
}

/**
 * @brief Consider one file. Hidden files set *hidden and are not stored.
 */
static void consider(doom_catalog_t *cat, const char *path, const char *name,
                     uint32_t size, bool *hidden)
{
    uint8_t hdr[12];
    uint32_t got = 0U;
    uint32_t nlumps = 0U;
    uint32_t dir_off = 0U;
    uint32_t bits = 0U;
    doom_wad_kind_t kind = DOOM_WAD_BAD;
    doom_id_t id;
    const char *art = NULL;
    doom_entry_t *slot = NULL;

    if ( already(cat, name) || (DOOM_CATALOG_MAX <= cat->count) )
    {
        if ( DOOM_CATALOG_MAX <= cat->count )
        {
            cat->truncated = true;
        }
        return;
    }
    if ( (12U > size) || !kode_file_read_at(path, 0U, hdr, sizeof(hdr), &got)
        || (sizeof(hdr) != got) )
    {
        return;
    }
    kind = doom_wad_kind(hdr, sizeof(hdr), &nlumps, &dir_off);
    bits = probe_lumps(path, size, kind, nlumps, dir_off);
    id = doom_wad_identify(name, kind, bits, size);
    KLOG_I(TAG, "%s -> %s show %u", name, id.title, id.show ? 1U : 0U);
    if ( !id.show )
    {
        *hidden = true;
        return;
    }
    slot = &cat->items[cat->count];
    memset(slot, 0, sizeof(*slot));
    snprintf(slot->name, sizeof(slot->name), "%s", name);
    snprintf(slot->title, sizeof(slot->title), "%s", id.title);
    art = doom_wad_art_lump(&id, bits);
    if ( NULL != art )
    {
        snprintf(slot->art, sizeof(slot->art), "%s", art);
    }
    slot->engine = id.engine;
    slot->role = id.role;
    slot->base = id.base;
    cat->count++;
}

static void scan_folder(const char *folder, doom_catalog_t *cat, bool *saw, bool *hidden)
{
    uint32_t skip = 0U;
    bool more = true;

    while ( more && !cat->truncated )
    {
        const kode_file_list_t *list = kode_file_list_from(folder, skip);
        uint32_t i = 0U;

        more = list->more;
        if ( 0U == list->count )
        {
            break;
        }
        for ( i = 0U; list->count > i; i++ )
        {
            kode_file_entry_t const *ent = &list->entries[i];
            char name[DOOM_NAME_MAX];
            char path[KODE_FILE_PATH_MAX];
            int wrote = 0;

            if ( ent->is_folder || !suffix_wad(ent->name) )
            {
                continue;
            }
            *saw = true;
            snprintf(name, sizeof(name), "%s", ent->name);
            wrote = snprintf(path, sizeof(path), "%s/%s", folder, name);
            if ( (0 < wrote) && ((uint32_t)wrote < sizeof(path)) )
            {
                consider(cat, path, name, ent->size, hidden);
            }
        }
        skip += list->count;
    }
}

static void publish(const doom_catalog_t *built)
{
    uint32_t cur = atomic_load_explicit(&s_slot, memory_order_relaxed);
    uint32_t nxt = 1U - cur;
    uint32_t gen = atomic_load_explicit(&s_gen, memory_order_relaxed) + 1U;

    s_pair[nxt] = *built;
    atomic_store_explicit(&s_slot, nxt, memory_order_relaxed);
    atomic_store_explicit(&s_gen, gen, memory_order_release);
}

kode_status_t doom_file_boot_job(void *arg, uint32_t *value)
{
    doom_catalog_t built;
    uint64_t total = 0U;
    uint64_t free_b = 0U;
    char last[DOOM_NAME_MAX];
    bool saw = false;
    bool hidden = false;
    uint8_t i = 0U;

    (void)arg;
    if ( NULL != value )
    {
        *value = 0U;
    }
    memset(&built, 0, sizeof(built));
    built.preferred = 0xFFU;
    if ( NULL == s_pair )
    {
        s_pair = (doom_catalog_t *)heap_caps_calloc(2U, sizeof(doom_catalog_t),
                                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if ( NULL == s_pair )
        {
            return KODE_NO_RESOURCES;
        }
    }
    if ( !kode_file_space("sd", &total, &free_b) )
    {
        built.no_card = true;
        publish(&built);
        return KODE_OK;
    }
    (void)kode_file_create_folder(DOOM_DIR);
    (void)kode_file_create_folder(DOOM_PWAD_DIR);
    (void)kode_file_create_folder(DOOM_SAVE_DIR);
    (void)doom_save_load(last, sizeof(last));
    scan_folder(DOOM_DIR, &built, &saw, &hidden);
    scan_folder(DOOM_PWAD_DIR, &built, &saw, &hidden);
    built.loose_only = saw && hidden && (0U == built.count);
    for ( i = 0U; built.count > i; i++ )
    {
        if ( ('\0' != last[0]) && (0 == strcmp(built.items[i].name, last)) )
        {
            built.preferred = i;
        }
    }
    publish(&built);
    if ( NULL != value )
    {
        *value = built.count;
    }
    KLOG_I(TAG, "scan found %u games", (unsigned)built.count);
    return KODE_OK;
}

const doom_catalog_t *doom_file_catalog(uint32_t *gen)
{
    uint32_t g = atomic_load_explicit(&s_gen, memory_order_acquire);
    uint32_t slot = 0U;

    if ( NULL != gen )
    {
        *gen = g;
    }
    if ( (0U == g) || (NULL == s_pair) )
    {
        return &s_empty;
    }
    slot = atomic_load_explicit(&s_slot, memory_order_relaxed);
    return &s_pair[slot];
}
