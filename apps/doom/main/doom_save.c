/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_save.c
 * @brief One config line on the card.
 */
#include "doom_save.h"

#include <stdio.h>
#include <string.h>

#include "kode.h"

static void copy_stem(const char *name, char *out, uint32_t cap)
{
    uint32_t n = 0U;
    uint32_t end = 0U;
    uint32_t i = 0U;

    if ( 0U == cap )
    {
        return;
    }
    if ( NULL == name )
    {
        out[0] = '\0';
        return;
    }
    while ( '\0' != name[n] )
    {
        n++;
    }
    end = n;
    i = n;
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
        end = cap - 1U;
    }
    memcpy(out, name, end);
    out[end] = '\0';
}

kode_status_t doom_save_load(char *name, uint32_t cap)
{
    const char *text = NULL;
    const char *p = NULL;
    uint32_t n = 0U;

    if ( (NULL == name) || (2U > cap) )
    {
        return KODE_DENIED;
    }
    name[0] = '\0';
    if ( !kode_file_exists(DOOM_CFG_PATH) )
    {
        return KODE_OK;
    }
    text = kode_file_read(DOOM_CFG_PATH);
    if ( NULL == text )
    {
        return KODE_OK;
    }
    p = strstr(text, "last=");
    if ( NULL == p )
    {
        return KODE_OK;
    }
    p += 5;
    while ( (' ' == *p) || ('\t' == *p) )
    {
        p++;
    }
    while ( ('\0' != *p) && ('\n' != *p) && ('\r' != *p) && ((n + 1U) < cap) )
    {
        name[n] = *p;
        n++;
        p++;
    }
    name[n] = '\0';
    return KODE_OK;
}

kode_status_t doom_save_store(const char *name)
{
    char line[96];
    uint32_t i = 0U;
    int wrote = 0;

    if ( (NULL == name) || ('\0' == name[0]) )
    {
        return KODE_DENIED;
    }
    for ( i = 0U; '\0' != name[i]; i++ )
    {
        if ( ('/' == name[i]) || ('\\' == name[i]) || ('\n' == name[i]) )
        {
            return KODE_DENIED;
        }
    }
    wrote = snprintf(line, sizeof(line), "last=%s\n", name);
    if ( (0 >= wrote) || ((uint32_t)wrote >= sizeof(line)) )
    {
        return KODE_DENIED;
    }
    if ( !kode_file_write(DOOM_CFG_PATH, line) )
    {
        return KODE_ERROR;
    }
    return KODE_OK;
}

bool doom_save_slot_path(const char *wad_name, unsigned slot, char *out, uint32_t cap)
{
    char stem[64];
    int wrote = 0;
    bool ok = false;

    if ( (NULL == out) || (5U < slot) )
    {
        return false;
    }
    copy_stem(wad_name, stem, sizeof(stem));
    wrote = snprintf(out, cap, "%s/%s/slot%u.dsg", DOOM_SAVE_DIR, stem, slot);
    if ( (0 < wrote) && ((uint32_t)wrote < cap) )
    {
        ok = true;
    }
    return ok;
}
