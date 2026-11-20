/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_game_select.c
 * @brief Text plates in the shape of logo buttons.
 *
 * The well in each button is where a later step decodes M_DOOM, TITLEPIC,
 * M_HTIC or TITLE from the user's WAD. This scaffold paints the lump name,
 * or the game name when that lump was not in the directory. No id or Raven
 * artwork is stored here.
 */
#include "doom_game_select.h"

#include <stdio.h>
#include <string.h>

#include "doom_font.h"
#include "doom_layout.h"
#include "doom_video.h"

static uint8_t s_hi = 0U;
static uint8_t s_scroll = 0U;
static uint32_t s_gen = 0U;
static bool s_prev_finger = false;
static int s_down = -1;
static int s_chosen = -1;
static const char *s_need = NULL;

static uint8_t collect(const doom_catalog_t *cat, uint8_t *idx, uint8_t cap)
{
    uint8_t n = 0U;
    uint8_t i = 0U;

    if ( (NULL == cat) || (NULL == idx) )
    {
        return 0U;
    }
    for ( i = 0U; (cat->count > i) && (cap > n); i++ )
    {
        idx[n] = i;
        n++;
    }
    return n;
}

static uint16_t plate_color(doom_engine_t engine, bool hot)
{
    uint16_t rgb = doom_rgb565(48U, 32U, 28U);

    if ( DOOM_ENGINE_DOOM == engine )
    {
        rgb = doom_rgb565(140U, 36U, 28U);
    }
    else if ( DOOM_ENGINE_DOOM2 == engine )
    {
        rgb = doom_rgb565(120U, 64U, 20U);
    }
    else if ( DOOM_ENGINE_HERETIC == engine )
    {
        rgb = doom_rgb565(28U, 96U, 44U);
    }
    else if ( DOOM_ENGINE_HEXEN == engine )
    {
        rgb = doom_rgb565(36U, 52U, 128U);
    }
    if ( hot )
    {
        rgb = (uint16_t)(rgb | 0x2104U);
    }
    return rgb;
}

static void reveal(uint8_t n, int16_t pw, int16_t ph)
{
    doom_layout_t lay;

    if ( 2U >= n )
    {
        s_scroll = 0U;
        return;
    }
    if ( s_hi < s_scroll )
    {
        s_scroll = (uint8_t)(s_hi & (uint8_t)~1U);
    }
    for ( ;; )
    {
        doom_layout_build(n, s_scroll, pw, ph, &lay);
        if ( s_hi < (uint8_t)(s_scroll + lay.visible) )
        {
            break;
        }
        if ( (uint8_t)(s_scroll + 2U) >= n )
        {
            break;
        }
        s_scroll = (uint8_t)(s_scroll + 2U);
    }
}

static int find_base(const doom_catalog_t *cat, doom_base_t need)
{
    int found = -1;
    uint8_t i = 0U;

    for ( i = 0U; cat->count > i; i++ )
    {
        if ( (DOOM_ROLE_IWAD == cat->items[i].role) && (need == cat->items[i].base) )
        {
            if ( 0 > found )
            {
                found = (int)i;
            }
            if ( (0 == strcmp(cat->items[i].title, "DOOM II"))
                || (0 == strcmp(cat->items[i].title, "ULTIMATE DOOM")) )
            {
                found = (int)i;
            }
        }
    }
    return found;
}

static void choose(const doom_catalog_t *cat, const uint8_t *idx, uint8_t n, int which)
{
    const doom_entry_t *ent = NULL;

    s_need = NULL;
    if ( (0 > which) || ((int)n <= which) )
    {
        return;
    }
    ent = &cat->items[idx[which]];
    if ( DOOM_ROLE_ADDON == ent->role )
    {
        if ( 0 > find_base(cat, ent->base) )
        {
            s_need = (DOOM_BASE_DOOMU == ent->base) ? "NEEDS DOOM.WAD" : "NEEDS DOOM2.WAD";
            s_chosen = -1;
            return;
        }
    }
    s_chosen = which;
}

static void move(const doom_controls_t *in, uint8_t n)
{
    uint8_t const cols = (2U < n) ? 2U : 1U;

    if ( in->up_edge && (s_hi >= cols) )
    {
        s_hi = (uint8_t)(s_hi - cols);
    }
    if ( in->down_edge && ((uint8_t)(s_hi + cols) < n) )
    {
        s_hi = (uint8_t)(s_hi + cols);
    }
    if ( in->left_edge && (0U != (s_hi % cols)) )
    {
        s_hi--;
    }
    if ( in->right_edge && (0U == (s_hi % cols)) && ((uint8_t)(s_hi + 1U) < n) )
    {
        s_hi++;
    }
}

doom_select_action_t doom_game_select_step(const doom_controls_t *in,
                                           const doom_catalog_t *cat, uint32_t gen,
                                           int16_t panel_w, int16_t panel_h)
{
    uint8_t idx[DOOM_CATALOG_MAX];
    uint8_t n = 0U;
    doom_layout_t lay;
    doom_select_action_t act = DOOM_SELECT_NONE;
    int hit = -1;

    if ( (NULL == in) || (NULL == cat) )
    {
        return DOOM_SELECT_NONE;
    }
    if ( gen != s_gen )
    {
        s_gen = gen;
        s_hi = 0U;
        s_scroll = 0U;
        s_need = NULL;
        s_chosen = -1;
        if ( (0xFFU != cat->preferred) && (cat->preferred < cat->count) )
        {
            uint8_t i = 0U;

            n = collect(cat, idx, DOOM_CATALOG_MAX);
            for ( i = 0U; n > i; i++ )
            {
                if ( idx[i] == cat->preferred )
                {
                    s_hi = i;
                }
            }
        }
    }
    n = collect(cat, idx, DOOM_CATALOG_MAX);
    if ( (0U < n) && (s_hi >= n) )
    {
        s_hi = (uint8_t)(n - 1U);
    }
    move(in, n);
    reveal(n, panel_w, panel_h);
    doom_layout_build(n, s_scroll, panel_w, panel_h, &lay);
    if ( in->finger )
    {
        hit = doom_layout_hit(&lay, in->x, in->y);
    }
    if ( in->finger && !s_prev_finger )
    {
        s_down = hit;
    }
    if ( !in->finger && s_prev_finger && (0 <= s_down) && (s_down == hit) )
    {
        choose(cat, idx, n, hit);
        if ( 0 <= s_chosen )
        {
            act = DOOM_SELECT_LAUNCH;
        }
    }
    s_prev_finger = in->finger;
    if ( in->fire_edge && (0U < n) )
    {
        choose(cat, idx, n, (int)s_hi);
        if ( 0 <= s_chosen )
        {
            act = DOOM_SELECT_LAUNCH;
        }
    }
    if ( in->use_edge )
    {
        act = DOOM_SELECT_RESCAN;
    }
    return act;
}

bool doom_game_select_fill(const doom_catalog_t *cat, doom_launch_t *out)
{
    uint8_t idx[DOOM_CATALOG_MAX];
    uint8_t n = 0U;
    const doom_entry_t *ent = NULL;
    int base = -1;
    bool ok = false;

    if ( (NULL == cat) || (NULL == out) || (0 > s_chosen) )
    {
        return false;
    }
    n = collect(cat, idx, DOOM_CATALOG_MAX);
    if ( (uint8_t)s_chosen >= n )
    {
        return false;
    }
    memset(out, 0, sizeof(*out));
    ent = &cat->items[idx[s_chosen]];
    if ( DOOM_ROLE_IWAD == ent->role )
    {
        snprintf(out->iwad, sizeof(out->iwad), "%s", ent->name);
        snprintf(out->title, sizeof(out->title), "%s", ent->title);
        out->engine = ent->engine;
        out->base = ent->base;
        ok = true;
    }
    else if ( DOOM_ROLE_ADDON == ent->role )
    {
        base = find_base(cat, ent->base);
        if ( 0 <= base )
        {
            snprintf(out->iwad, sizeof(out->iwad), "%s", cat->items[base].name);
            snprintf(out->addon, sizeof(out->addon), "%s", ent->name);
            snprintf(out->title, sizeof(out->title), "%s", ent->title);
            out->engine = ent->engine;
            out->base = ent->base;
            ok = true;
        }
    }
    return ok;
}

static void draw_message(uint16_t *fb, int16_t pw, int16_t ph, const doom_catalog_t *cat,
                         uint32_t gen)
{
    uint16_t const ink = doom_rgb565(220U, 200U, 180U);
    const char *title = "NO WADS";
    const char *line2 = "COPY IWADS INTO";
    const char *line3 = "SD/DOOM";

    (void)ph;
    if ( 0U == gen )
    {
        title = "SCANNING";
    }
    else if ( (NULL != cat) && cat->no_card )
    {
        title = "NO CARD";
        line2 = "INSERT A MICROSD";
        line3 = "THEN SD/DOOM";
    }
    else if ( (NULL != cat) && cat->loose_only )
    {
        title = "NO IWAD";
        line2 = "ADD-ONS NEED A BASE";
        line3 = "DOOM.WAD OR DOOM2.WAD";
    }
    doom_font_text(fb, (uint32_t)pw, (uint32_t)pw, (uint32_t)ph, 24, 80, title, ink, 2U);
    if ( 0U != gen )
    {
        doom_font_text(fb, (uint32_t)pw, (uint32_t)pw, (uint32_t)ph, 24, 160,
                       line2, ink, 2U);
        doom_font_text(fb, (uint32_t)pw, (uint32_t)pw, (uint32_t)ph, 24, 190,
                       line3, ink, 2U);
        doom_font_text(fb, (uint32_t)pw, (uint32_t)pw, (uint32_t)ph, 24, 240,
                       "DOOM  DOOM2  HERETIC", ink, 1U);
        doom_font_text(fb, (uint32_t)pw, (uint32_t)pw, (uint32_t)ph, 24, 256,
                       "HEXEN  TNT  PLUTONIA", ink, 1U);
        doom_font_text(fb, (uint32_t)pw, (uint32_t)pw, (uint32_t)ph, 24, 272,
                       "SIGIL  SIGIL2  ID1", ink, 1U);
        doom_font_text(fb, (uint32_t)pw, (uint32_t)pw, (uint32_t)ph, 24, 300,
                       "FREEDOOM WORKS TOO", ink, 1U);
    }
}

void doom_game_select_draw(uint16_t *fb, int16_t panel_w, int16_t panel_h,
                           const doom_catalog_t *cat, uint32_t gen)
{
    uint8_t idx[DOOM_CATALOG_MAX];
    uint8_t n = 0U;
    doom_layout_t lay;
    uint8_t i = 0U;
    uint16_t const ink = doom_rgb565(240U, 220U, 200U);

    if ( NULL == fb )
    {
        return;
    }
    doom_font_text(fb, (uint32_t)panel_w, (uint32_t)panel_w, (uint32_t)panel_h,
                   16, 16, "CHOOSE", ink, 2U);
    if ( NULL != s_need )
    {
        doom_font_text(fb, (uint32_t)panel_w, (uint32_t)panel_w, (uint32_t)panel_h,
                       16, 40, s_need, doom_rgb565(240U, 64U, 48U), 1U);
    }
    n = collect(cat, idx, DOOM_CATALOG_MAX);
    if ( (0U == n) || (0U == gen) )
    {
        draw_message(fb, panel_w, panel_h, cat, gen);
        return;
    }
    doom_layout_build(n, s_scroll, panel_w, panel_h, &lay);
    for ( i = 0U; lay.visible > i; i++ )
    {
        uint8_t const list_i = (uint8_t)(lay.origin + i);
        const doom_entry_t *ent = &cat->items[idx[list_i]];
        doom_rect_t const *r = &lay.rect[i];
        bool const hot = (list_i == s_hi);
        int16_t well_h = (int16_t)((r->h * 3) / 5);
        const char *art = ('\0' != ent->art[0]) ? ent->art : ent->title;

        doom_video_rect(fb, r->x, r->y, r->w, r->h, plate_color(ent->engine, hot));
        if ( hot )
        {
            doom_video_rect(fb, r->x, r->y, r->w, 3, 0xFFFFU);
        }
        doom_video_rect(fb, (int16_t)(r->x + 8), (int16_t)(r->y + 8),
                        (int16_t)(r->w - 16), well_h, doom_rgb565(12U, 8U, 8U));
        doom_font_text(fb, (uint32_t)panel_w, (uint32_t)panel_w, (uint32_t)panel_h,
                       (int32_t)r->x + 16, (int32_t)r->y + 24, art, ink,
                       (1U == n) ? 2U : 1U);
        doom_font_text(fb, (uint32_t)panel_w, (uint32_t)panel_w, (uint32_t)panel_h,
                       (int32_t)r->x + 12, (int32_t)r->y + well_h + 16, ent->title,
                       ink, 1U);
        if ( DOOM_ROLE_ADDON == ent->role )
        {
            doom_font_text(fb, (uint32_t)panel_w, (uint32_t)panel_w, (uint32_t)panel_h,
                           (int32_t)r->x + 12, (int32_t)r->y + r->h - 16, "ADD-ON",
                           doom_rgb565(200U, 180U, 80U), 1U);
        }
    }
    doom_font_text(fb, (uint32_t)panel_w, (uint32_t)panel_w, (uint32_t)panel_h,
                   16, (int32_t)panel_h - 28, "TAP OR TOP TO START", ink, 1U);
}
