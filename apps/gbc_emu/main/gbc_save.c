/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_save.c
 * @brief Save RAM and the MBC3 clock on the microSD card.
 */
#include "gbc_save.h"

#include <stdio.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "kode.h"
#include "kode_log.h"

/** @brief This file's log tag. */
#define TAG "gbc.save"

/** @brief The folder the saves live in. gbc_save_load() creates it. */
#define GBC_SAVE_DIR  "sd/gbc/saves"

/**
 * @brief Copy the ROM name without its last extension.
 * @param name The file name.
 * @param out  The stem. Always NUL terminated when cap is above 0.
 * @param cap  Bytes of out.
 */
static void stem_of(const char *name, char *out, uint32_t cap)
{
    size_t n = strlen(name);
    size_t end = n;
    size_t i = n;

    if ( 0U == cap )
    {
        return;
    }
    while ( 0U < i )
    {
        i--;
        if ( '.' == name[i] )
        {
            if ( 0U < i )
            {
                end = i;
            }
            break;
        }
    }
    if ( end >= cap )
    {
        end = (size_t)cap - 1U;
    }
    memcpy(out, name, end);
    out[end] = '\0';
}

/**
 * @brief Build sd/gbc/saves/<stem><ext>.
 * @param rom_name The ROM's file name.
 * @param ext      ".sav" or ".rtc", including the dot.
 * @param out      The path.
 * @param cap      Bytes of out.
 * @return true when the path fits.
 */
static bool save_path(const char *rom_name, const char *ext, char *out, uint32_t cap)
{
    char stem[KODE_FILE_NAME_MAX];
    int wrote = 0;
    bool ok = false;

    stem_of(rom_name, stem, sizeof(stem));
    wrote = snprintf(out, cap, "%s/%s%s", GBC_SAVE_DIR, stem, ext);
    if ( (0 < wrote) && ((uint32_t)wrote < cap) )
    {
        ok = true;
    }
    return ok;
}

/**
 * @brief Create a folder when it is not already one.
 * @param path "sd/..." folder.
 * @return true when the folder is there afterwards.
 */
static bool ensure_folder(const char *path)
{
    kode_file_status_t const st = kode_file_status(path);
    bool ok = false;

    if ( st.exists && st.is_folder )
    {
        ok = true;
    }
    else if ( !st.exists )
    {
        ok = kode_file_create_folder(path);
    }
    return ok;
}

/**
 * @brief Read an 8-byte little-endian Unix time.
 * @param path     The RTC file.
 * @param unix_sec Receives the value, or -1 when the file is absent or short.
 */
static void read_clock(const char *path, int64_t *unix_sec)
{
    uint8_t raw[8];
    uint32_t got = 0U;
    uint64_t acc = 0U;
    uint32_t i = 0U;

    *unix_sec = -1;
    if ( kode_file_exists(path)
        && kode_file_read_at(path, 0U, raw, sizeof(raw), &got)
        && (sizeof(raw) == got) )
    {
        for ( i = 8U; 0U < i; i-- )
        {
            acc = (acc << 8) | (uint64_t)raw[i - 1U];
        }
        *unix_sec = (int64_t)acc;
    }
}

/**
 * @brief Write an 8-byte little-endian Unix time.
 * @param path     The RTC file.
 * @param unix_sec The value. Negative is refused by the caller.
 * @return true when the file holds the value.
 */
static bool write_clock(const char *path, int64_t unix_sec)
{
    uint8_t raw[8];
    uint64_t acc = (uint64_t)unix_sec;
    uint32_t i = 0U;

    for ( i = 0U; 8U > i; i++ )
    {
        raw[i] = (uint8_t)(acc & 0xFFU);
        acc >>= 8;
    }
    return kode_file_write_bytes(path, raw, sizeof(raw));
}

kode_status_t gbc_save_load(const char *rom_name, uint32_t ram_bytes,
                            uint8_t **ram, int64_t *unix_sec)
{
    kode_status_t rc = KODE_DENIED;
    char path[KODE_FILE_PATH_MAX];

    if ( (NULL == rom_name) || ('\0' == rom_name[0]) || (NULL == ram) || (NULL == unix_sec) )
    {
        KLOG_W(TAG, "save load refused: bad argument");
    }
    else if ( !ensure_folder("sd/gbc") || !ensure_folder(GBC_SAVE_DIR) )
    {
        KLOG_W(TAG, "save folder is not on the card");
        rc = KODE_ERROR;
    }
    else if ( !save_path(rom_name, ".rtc", path, sizeof(path)) )
    {
        KLOG_W(TAG, "RTC path does not fit");
    }
    else
    {
        uint8_t *block = NULL;
        bool ram_ok = true;

        read_clock(path, unix_sec);
        if ( 0U == ram_bytes )
        {
            *ram = NULL;
            rc = KODE_OK;
        }
        else
        {
            block = heap_caps_malloc(ram_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if ( NULL == block )
            {
                KLOG_E(TAG, "no PSRAM for %u bytes of save RAM", (unsigned)ram_bytes);
                rc = KODE_NO_RESOURCES;
                ram_ok = false;
            }
            else
            {
                memset(block, 0, ram_bytes);
                if ( !save_path(rom_name, ".sav", path, sizeof(path)) )
                {
                    KLOG_W(TAG, "save path does not fit");
                    heap_caps_free(block);
                    ram_ok = false;
                }
                else if ( kode_file_exists(path) )
                {
                    uint32_t off = 0U;

                    while ( ram_ok && (off < ram_bytes) )
                    {
                        uint32_t cap = 32768U;
                        uint32_t got = 0U;

                        if ( (ram_bytes - off) < cap )
                        {
                            cap = ram_bytes - off;
                        }
                        if ( !kode_file_read_at(path, off, &block[off], cap, &got) )
                        {
                            KLOG_W(TAG, "save read failed at %u", (unsigned)off);
                            heap_caps_free(block);
                            ram_ok = false;
                            rc = KODE_ERROR;
                        }
                        else if ( 0U == got )
                        {
                            /* Shorter than the header's RAM size: the rest stays 0. */
                            off = ram_bytes;
                        }
                        else
                        {
                            off += got;
                        }
                    }
                }
                if ( ram_ok )
                {
                    *ram = block;
                    rc = KODE_OK;
                }
            }
        }
    }
    return rc;
}

kode_status_t gbc_save_store(const char *rom_name, const uint8_t *ram,
                             uint32_t ram_bytes, int64_t unix_sec)
{
    kode_status_t rc = KODE_DENIED;
    char path[KODE_FILE_PATH_MAX];

    if ( (NULL == rom_name) || ('\0' == rom_name[0]) )
    {
        KLOG_W(TAG, "save store refused: no ROM name");
    }
    else if ( (0U < ram_bytes) && (NULL == ram) )
    {
        KLOG_W(TAG, "save store refused: RAM pointer is NULL");
    }
    else if ( (0U < ram_bytes) && !save_path(rom_name, ".sav", path, sizeof(path)) )
    {
        KLOG_W(TAG, "save path does not fit");
    }
    else if ( (0U < ram_bytes) && !kode_file_write_bytes(path, ram, ram_bytes) )
    {
        KLOG_W(TAG, "save write failed");
        rc = KODE_ERROR;
    }
    else if ( (0 <= unix_sec) && !save_path(rom_name, ".rtc", path, sizeof(path)) )
    {
        KLOG_W(TAG, "RTC path does not fit");
    }
    else if ( (0 <= unix_sec) && !write_clock(path, unix_sec) )
    {
        KLOG_W(TAG, "RTC write failed");
        rc = KODE_ERROR;
    }
    else
    {
        rc = KODE_OK;
    }
    return rc;
}
