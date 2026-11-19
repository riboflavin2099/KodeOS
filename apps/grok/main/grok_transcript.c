/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_transcript.c
 * @brief A fixed ring of short lines. No heap.
 */
#include "grok_transcript.h"

#include <string.h>

/**
 * @brief Copy @p text into a line, cutting it to fit.
 * @param line The line.
 * @param role Its role.
 * @param text The text. NULL is empty.
 */
static void fill_line(grok_tr_line_t *line, grok_tr_role_t role, const char *text)
{
    size_t n = 0U;

    line->role = role;
    if ( NULL == text )
    {
        line->text[0] = '\0';
        return;
    }
    while ( ('\0' != text[n]) && ((GROK_TR_CHARS - 1U) > n) )
    {
        line->text[n] = text[n];
        n++;
    }
    line->text[n] = '\0';
}

void grok_transcript_init(grok_transcript_t *t)
{
    if ( NULL != t )
    {
        (void)memset(t, 0, sizeof(*t));
    }
}

bool grok_transcript_add(grok_transcript_t *t, grok_tr_role_t role, const char *text)
{
    size_t i;

    if ( NULL == t )
    {
        return false;
    }
    if ( GROK_TR_LINES == t->count )
    {
        for ( i = 1U; GROK_TR_LINES > i; i++ )
        {
            t->lines[i - 1U] = t->lines[i];
        }
        t->count = GROK_TR_LINES - 1U;
    }
    fill_line(&t->lines[t->count], role, text);
    t->count++;
    return true;
}

bool grok_transcript_add_text(grok_transcript_t *t, grok_tr_role_t role,
                              const char *text)
{
    if ( NULL == t )
    {
        return false;
    }
    if ( (NULL == text) || ('\0' == text[0]) )
    {
        return grok_transcript_add(t, role, text);
    }
    while ( '\0' != text[0] )
    {
        char   line[GROK_TR_CHARS];
        size_t n = 0U;

        while ( ('\0' != text[n]) && ((GROK_TR_CHARS - 1U) > n) )
        {
            line[n] = text[n];
            n++;
        }
        line[n] = '\0';
        if ( !grok_transcript_add(t, role, line) )
        {
            return false;
        }
        text = &text[n];
    }
    return true;
}

bool grok_transcript_append(grok_transcript_t *t, grok_tr_role_t role, const char *text)
{
    grok_tr_line_t *tail;
    size_t          n;
    size_t          i = 0U;

    if ( NULL == t )
    {
        return false;
    }
    if ( (0U == t->count) || (role != t->lines[t->count - 1U].role) )
    {
        return grok_transcript_add(t, role, text);
    }
    tail = &t->lines[t->count - 1U];
    n = strlen(tail->text);
    if ( NULL == text )
    {
        return true;
    }
    while ( ('\0' != text[i]) && ((GROK_TR_CHARS - 1U) > n) )
    {
        tail->text[n] = text[i];
        n++;
        i++;
    }
    tail->text[n] = '\0';
    return true;
}

size_t grok_transcript_window(const grok_transcript_t *t, size_t max, int scroll,
                              const grok_tr_line_t **out, int *clamped)
{
    size_t shown;
    size_t start;
    size_t i;
    int    top;

    if ( (NULL == t) || (NULL == out) || (0U == max) )
    {
        if ( NULL != clamped )
        {
            *clamped = 0;
        }
        return 0U;
    }
    if ( 0 > scroll )
    {
        scroll = 0;
    }
    shown = t->count;
    if ( shown > max )
    {
        shown = max;
    }
    /* scroll moves the window up. It cannot pass the oldest line. */
    top = (int)t->count - (int)shown;
    if ( 0 > top )
    {
        top = 0;
    }
    if ( scroll > top )
    {
        scroll = top;
    }
    if ( NULL != clamped )
    {
        *clamped = scroll;
    }
    start = (size_t)(top - scroll);
    for ( i = 0U; shown > i; i++ )
    {
        out[i] = &t->lines[start + i];
    }
    return shown;
}
