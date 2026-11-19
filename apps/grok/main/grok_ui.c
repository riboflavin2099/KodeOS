/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_ui.c
 * @brief A screen of short lines. A canvas font is the next step.
 */
#include "grok_ui.h"

#include <string.h>

#include "grok_prompts.h"
#include "kode.h"
#include "kode_log.h"

/** @brief This file's log tag. */
#define TAG "grok.ui"

/** @brief Lines of the transcript drawn under the prompt. */
#define GROK_UI_LINES  5U

/**
 * @brief Copy @p text into @p dst.
 * @param dst  The line.
 * @param cap  Its size.
 * @param text The sentence.
 */
static void copy_line(char *dst, size_t cap, const char *text)
{
    size_t n = 0U;

    if ( (NULL == dst) || (0U == cap) )
    {
        return;
    }
    if ( NULL == text )
    {
        dst[0] = '\0';
        return;
    }
    while ( ('\0' != text[n]) && ((cap - 1U) > n) )
    {
        dst[n] = text[n];
        n++;
    }
    dst[n] = '\0';
}

void grok_ui_status(char *dst, size_t cap, const grok_config_t *cfg,
                    grok_net_status_t net, bool have_file, bool listening,
                    bool asking)
{
    if ( (NULL == dst) || (0U == cap) )
    {
        return;
    }
    dst[0] = '\0';
    if ( listening )
    {
        copy_line(dst, cap, "Listening. Release the top button.");
        return;
    }
    if ( asking )
    {
        copy_line(dst, cap, "Asking...");
        return;
    }
    if ( GROK_NET_UP != net )
    {
        copy_line(dst, cap, "No network.");
        return;
    }
    if ( !have_file )
    {
        copy_line(dst, cap, "No sd/grok/config.txt.");
        return;
    }
    if ( (NULL != cfg) && !cfg->has_key
         && (NULL != strstr(cfg->base_url, "://api.x.ai")) )
    {
        copy_line(dst, cap, "No API key in the config file.");
        return;
    }
    if ( (NULL != cfg) && ('\0' != cfg->model[0]) )
    {
        copy_line(dst, cap, cfg->model);
        return;
    }
    copy_line(dst, cap, "Ready.");
}

void grok_ui_show(const char *status, const grok_transcript_t *tr, int scroll,
                  size_t prompt_index)
{
    const grok_prompt_t  *prompt = grok_prompt_at(prompt_index);
    const grok_tr_line_t *win[GROK_UI_LINES];
    char                  line[80];
    size_t                n;
    size_t                i;
    size_t                used = 0U;
    const char           *ask = "Ask: ";
    const char           *lab = prompt->label;

    if ( !kode_screen_new("Grok") )
    {
        KLOG_W(TAG, "screen: %s", kode_error());
        return;
    }
    (void)kode_screen_add_text((NULL != status) ? status : "");
    while ( ('\0' != *ask) && ((sizeof(line) - 1U) > used) )
    {
        line[used] = *ask;
        used++;
        ask++;
    }
    while ( ('\0' != *lab) && ((sizeof(line) - 1U) > used) )
    {
        line[used] = *lab;
        used++;
        lab++;
    }
    line[used] = '\0';
    (void)kode_screen_add_text(line);
    (void)kode_screen_add_text(grok_keyboard_hint());
    if ( NULL == tr )
    {
        return;
    }
    n = grok_transcript_window(tr, GROK_UI_LINES, scroll, win, NULL);
    for ( i = 0U; n > i; i++ )
    {
        kode_widget_t w;
        const char   *text = win[i]->text;

        if ( '\0' == text[0] )
        {
            text = " ";
        }
        w = kode_screen_add_text(text);
        if ( 0U == w )
        {
            continue;
        }
        if ( GROK_TR_USER == win[i]->role )
        {
            (void)kode_widget_set_color(w, KODE_LIGHT_GREY);
        }
        else if ( GROK_TR_ASSIST == win[i]->role )
        {
            (void)kode_widget_set_color(w, KODE_BLUE);
        }
        else
        {
            (void)kode_widget_set_color(w, KODE_YELLOW);
        }
    }
}
