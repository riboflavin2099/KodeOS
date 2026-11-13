/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_rom.c
 * @brief Read a ROM from sd/gbc into PSRAM and publish it to the frame task.
 *
 * kode_file_read_bytes() stops at 4 MiB. An MBC5 image is 8 MiB, so this
 * file uses kode_file_read_at() in 32 KiB windows, which is the size
 * AO_Storage moves in one request.
 */
#include "gbc_rom.h"

#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "gbc_core.h"
#include "gbc_header.h"
#include "gbc_save.h"
#include "kode.h"
#include "kode_log.h"

/** @brief This file's log tag. */
#define TAG "gbc.rom"

/** @brief One storage window. KODE_SYS_FILE_READ_MAX. */
#define GBC_READ_WINDOW  32768U

/** @brief 0 until the image is fully in PSRAM, then 1. */
static atomic_uint s_published = 0U;

/** @brief The image. Read by the frame task only after s_published is 1. */
static uint8_t *s_rom = NULL;

/** @brief Bytes at s_rom. */
static uint32_t s_rom_len = 0U;

/** @brief Cartridge RAM from the save file. Held for the core; nothing reads it yet. */
static uint8_t *s_ram = NULL;

/**
 * @brief ASCII lower case, for the extension check. Other bytes pass through.
 * @param c The character.
 * @return The lower case letter, or c.
 */
static char lower_ascii(char c)
{
    char out = c;

    if ( ('A' <= c) && ('Z' >= c) )
    {
        out = (char)(c + ('a' - 'A'));
    }
    return out;
}

/**
 * @brief Whether name ends with suf, ignoring ASCII case.
 * @param name The file name.
 * @param suf  The suffix, including the dot.
 * @return true on a match.
 */
static bool suffix_is(const char *name, const char *suf)
{
    size_t const n = strlen(name);
    size_t const m = strlen(suf);
    bool match = false;

    if ( m <= n )
    {
        size_t i = 0U;

        match = true;
        for ( i = 0U; m > i; i++ )
        {
            if ( lower_ascii(name[(n - m) + i]) != lower_ascii(suf[i]) )
            {
                match = false;
            }
        }
    }
    return match;
}

/**
 * @brief Join sd/gbc and a file name.
 * @param name The file name from a listing.
 * @param out  The path.
 * @param cap  Bytes of out.
 * @return true when it fits.
 */
static bool rom_path(const char *name, char *out, uint32_t cap)
{
    int const wrote = snprintf(out, cap, "%s/%s", GBC_ROM_DIR, name);
    bool ok = false;

    if ( (0 < wrote) && ((uint32_t)wrote < cap) )
    {
        ok = true;
    }
    return ok;
}

/**
 * @brief The first .gb or .gbc name in one listing page.
 * @param list The page.
 * @return The name, or NULL. Points into the listing.
 */
static const char *first_rom(const kode_file_list_t *list)
{
    const char *found = NULL;
    uint32_t i = 0U;

    for ( i = 0U; list->count > i; i++ )
    {
        kode_file_entry_t const *ent = &list->entries[i];

        if ( !ent->is_folder
            && (suffix_is(ent->name, ".gbc") || suffix_is(ent->name, ".gb")) )
        {
            found = ent->name;
            break;
        }
    }
    return found;
}

/**
 * @brief Read the whole image described by the header.
 * @param path The file.
 * @param hdr  A header already parsed from this file.
 * @param len  Receives the number of bytes read, the header's ROM size.
 * @return The PSRAM block, or NULL. On NULL, kode_error() may say why the read failed.
 */
static uint8_t *read_image(const char *path, const gbc_header_t *hdr, uint32_t *len)
{
    uint8_t *buf = heap_caps_aligned_alloc(16U, hdr->rom_bytes,
                                           MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    uint32_t off = 0U;
    bool ok = true;

    if ( NULL == buf )
    {
        KLOG_E(TAG, "no PSRAM for %u byte ROM", (unsigned)hdr->rom_bytes);
        ok = false;
    }
    while ( ok && (off < hdr->rom_bytes) )
    {
        uint32_t cap = GBC_READ_WINDOW;
        uint32_t got = 0U;

        if ( (hdr->rom_bytes - off) < cap )
        {
            cap = hdr->rom_bytes - off;
        }
        if ( !kode_file_read_at(path, off, &buf[off], cap, &got) || (0U == got) )
        {
            KLOG_W(TAG, "ROM read failed at %u", (unsigned)off);
            ok = false;
        }
        else
        {
            off += got;
        }
    }
    if ( !ok && (NULL != buf) )
    {
        heap_caps_free(buf);
        buf = NULL;
    }
    if ( NULL != buf )
    {
        *len = hdr->rom_bytes;
    }
    return buf;
}

kode_status_t gbc_rom_boot_job(void *arg, uint32_t *value)
{
    const kode_file_list_t *list = NULL;
    const char *name = NULL;
    char path[KODE_FILE_PATH_MAX];
    uint8_t lead[GBC_HEADER_MIN_BYTES];
    uint32_t got = 0U;
    gbc_header_t hdr;
    kode_file_status_t st;
    kode_status_t rc = KODE_OK;

    (void)arg;
    if ( NULL == value )
    {
        rc = KODE_DENIED;
    }
    else
    {
        *value = 0U;
        list = kode_file_list(GBC_ROM_DIR);
        name = first_rom(list);
        if ( NULL == name )
        {
            if ( list->more )
            {
                KLOG_W(TAG, "no ROM in the first %u names of %s",
                       (unsigned)list->count, GBC_ROM_DIR);
            }
            else
            {
                KLOG_I(TAG, "no ROM in %s", GBC_ROM_DIR);
            }
        }
        else if ( !rom_path(name, path, sizeof(path)) )
        {
            KLOG_W(TAG, "ROM path does not fit: %s", name);
            rc = KODE_DENIED;
        }
        else
        {
            st = kode_file_status(path);
            if ( !st.exists || st.is_folder || (GBC_HEADER_MIN_BYTES > st.size) )
            {
                KLOG_W(TAG, "ROM is not a file long enough to hold a header");
                rc = KODE_ERROR;
            }
            else if ( !kode_file_read_at(path, 0U, lead, sizeof(lead), &got)
                     || (sizeof(lead) != got)
                     || !gbc_header_parse(lead, got, &hdr) )
            {
                KLOG_W(TAG, "ROM header was refused");
                rc = KODE_ERROR;
            }
            else if ( hdr.rom_bytes > st.size )
            {
                KLOG_W(TAG, "ROM file is %u bytes; the header says %u",
                       (unsigned)st.size, (unsigned)hdr.rom_bytes);
                rc = KODE_ERROR;
            }
            else
            {
                uint32_t rom_len = 0U;
                uint8_t *image = read_image(path, &hdr, &rom_len);
                uint8_t *ram = NULL;
                int64_t saved_clock = -1;
                int64_t live_clock = kode_time_unix();
                unsigned cgb = 0U;
                unsigned rtc = 0U;

                if ( NULL == image )
                {
                    rc = KODE_NO_RESOURCES;
                }
                else if ( KODE_OK != gbc_save_load(name, hdr.ram_bytes, &ram, &saved_clock) )
                {
                    heap_caps_free(image);
                    rc = KODE_ERROR;
                }
                else
                {
                    if ( (0 > saved_clock) && (0 <= live_clock) )
                    {
                        saved_clock = live_clock;
                    }
                    gbc_core_set_clock(saved_clock);
                    s_ram = ram;
                    s_rom = image;
                    s_rom_len = rom_len;
                    atomic_store_explicit(&s_published, 1U, memory_order_release);
                    *value = rom_len;
                    if ( hdr.cgb_capable )
                    {
                        cgb = 1U;
                    }
                    if ( hdr.has_rtc )
                    {
                        rtc = 1U;
                    }
                    KLOG_I(TAG, "loaded %s \"%s\" %u bytes, cgb %u, rtc %u",
                           name, hdr.title, (unsigned)rom_len, cgb, rtc);
                }
            }
        }
    }
    return rc;
}

const uint8_t *gbc_rom_data(uint32_t *len)
{
    const uint8_t *rom = NULL;

    if ( 0U != atomic_load_explicit(&s_published, memory_order_acquire) )
    {
        /* s_ram is the cartridge RAM kept for the core. Nothing reads it yet. */
        (void)s_ram;
        rom = s_rom;
        if ( NULL != len )
        {
            *len = s_rom_len;
        }
    }
    else if ( NULL != len )
    {
        *len = 0U;
    }
    return rom;
}
