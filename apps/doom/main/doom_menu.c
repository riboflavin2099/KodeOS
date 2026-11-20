/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_menu.c
 * @brief The three screens: games, weapons, settings.
 */
#include "doom_menu.h"

#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#include "app_ao.h"
#include "doom_engine.h"
#include "doom_file.h"
#include "doom_font.h"
#include "doom_game_select.h"
#include "doom_input.h"
#include "doom_video.h"

typedef enum {
    SCREEN_SELECT = 0,
    SCREEN_PLAY,
    SCREEN_WEAPONS,
    SCREEN_SETTINGS
} screen_t;

typedef enum {
    ROW_WEAPON = 0,
    ROW_STRAFE,
    ROW_INV_PREV,
    ROW_INV_USE,
    ROW_INV_NEXT,
    ROW_JUMP,
    ROW_CLOSE,
    ROW_SFX,
    ROW_MUSIC,
    ROW_RUN,
    ROW_EXIT,
    ROW_RESUME
} row_kind_t;

typedef struct {
    row_kind_t kind;
    int slot;
    const char *label;
} row_t;

#define ROW_MAX  16U

static screen_t s_screen = SCREEN_SELECT;
static row_t s_rows[ROW_MAX];
static uint8_t s_nrows = 0U;
static uint8_t s_row = 0U;
static int s_sfx = 2;
static bool s_music = false;
static bool s_run = true;
static char s_pick[64];
static atomic_uint s_pick_seq = 0U;
static bool s_prev_finger = false;
static int s_down_row = -1;

static void add_row(row_kind_t kind, int slot, const char *label)
{
    if ( ROW_MAX > s_nrows )
    {
        s_rows[s_nrows].kind = kind;
        s_rows[s_nrows].slot = slot;
        s_rows[s_nrows].label = label;
        s_nrows++;
    }
}

static void build_weapons(doom_engine_t engine)
{
    s_nrows = 0U;
    s_row = 0U;
    if ( DOOM_ENGINE_HERETIC == engine )
    {
        add_row(ROW_WEAPON, 1, "GAUNTLETS");
        add_row(ROW_WEAPON, 2, "STAFF");
        add_row(ROW_WEAPON, 3, "CROSSBOW");
        add_row(ROW_WEAPON, 4, "CLAW");
        add_row(ROW_WEAPON, 5, "HELLSTAFF");
        add_row(ROW_WEAPON, 6, "PHOENIX");
        add_row(ROW_WEAPON, 7, "FIREMACE");
        add_row(ROW_INV_PREV, 0, "INV PREV");
        add_row(ROW_INV_USE, 0, "INV USE");
        add_row(ROW_INV_NEXT, 0, "INV NEXT");
    }
    else if ( DOOM_ENGINE_HEXEN == engine )
    {
        add_row(ROW_WEAPON, 1, "PIECE 1");
        add_row(ROW_WEAPON, 2, "PIECE 2");
        add_row(ROW_WEAPON, 3, "PIECE 3");
        add_row(ROW_WEAPON, 4, "PIECE 4");
        add_row(ROW_INV_PREV, 0, "INV PREV");
        add_row(ROW_INV_USE, 0, "INV USE");
        add_row(ROW_INV_NEXT, 0, "INV NEXT");
        add_row(ROW_JUMP, 0, "JUMP");
    }
    else
    {
        add_row(ROW_WEAPON, 1, "FIST");
        add_row(ROW_WEAPON, 2, "PISTOL");
        add_row(ROW_WEAPON, 3, "SHOTGUN");
        add_row(ROW_WEAPON, 4, "CHAINGUN");
        add_row(ROW_WEAPON, 5, "ROCKET");
        add_row(ROW_WEAPON, 6, "PLASMA");
        add_row(ROW_WEAPON, 7, "BFG");
    }
    add_row(ROW_STRAFE, 0, doom_engine_strafe_on() ? "STRAFE ON" : "STRAFE OFF");
    add_row(ROW_CLOSE, 0, "CLOSE");
}

static void build_settings(void)
{
    s_nrows = 0U;
    s_row = 0U;
    add_row(ROW_SFX, 0, "SOUND");
    add_row(ROW_MUSIC, 0, "MUSIC");
    add_row(ROW_RUN, 0, "ALWAYS RUN");
    add_row(ROW_EXIT, 0, "EXIT TO MENU");
    add_row(ROW_RESUME, 0, "RESUME");
}

static bool stays_open(row_kind_t kind)
{
    return (ROW_INV_PREV == kind) || (ROW_INV_NEXT == kind)
           || (ROW_SFX == kind) || (ROW_MUSIC == kind) || (ROW_RUN == kind);
}

static void apply_row(const row_t *row)
{
    if ( ROW_WEAPON == row->kind )
    {
        doom_engine_weapon(row->slot);
    }
    else if ( ROW_STRAFE == row->kind )
    {
        doom_engine_strafe(!doom_engine_strafe_on());
    }
    else if ( ROW_INV_PREV == row->kind )
    {
        doom_engine_inventory(-1);
    }
    else if ( ROW_INV_NEXT == row->kind )
    {
        doom_engine_inventory(1);
    }
    else if ( ROW_INV_USE == row->kind )
    {
        doom_engine_inventory(0);
    }
    else if ( ROW_JUMP == row->kind )
    {
        doom_engine_jump();
    }
    else if ( ROW_SFX == row->kind )
    {
        s_sfx = (s_sfx + 1) % 3;
    }
    else if ( ROW_MUSIC == row->kind )
    {
        s_music = !s_music;
    }
    else if ( ROW_RUN == row->kind )
    {
        s_run = !s_run;
    }
    else if ( ROW_EXIT == row->kind )
    {
        doom_engine_stop();
        s_screen = SCREEN_SELECT;
        return;
    }
    if ( !stays_open(row->kind) && (ROW_EXIT != row->kind) )
    {
        doom_engine_pause(false);
        s_screen = SCREEN_PLAY;
    }
}

static void arm_overlay(screen_t which)
{
    s_screen = which;
    s_prev_finger = false;
    s_down_row = -1;
    doom_engine_pause(true);
    if ( SCREEN_SETTINGS == which )
    {
        build_settings();
    }
    else
    {
        build_weapons(doom_engine_id());
    }
}

static int row_at(int16_t y, int16_t panel_h)
{
    int i = 0;
    int hit = -1;

    (void)panel_h;
    for ( i = 0; (int)s_nrows > i; i++ )
    {
        int16_t const top = (int16_t)(64 + (i * 28));

        if ( (top <= y) && (y < (int16_t)(top + 26)) )
        {
            hit = i;
        }
    }
    return hit;
}

/**
 * @brief Apply one row. The index is unsigned so a miss (-1) cannot
 *        become a subscript GCC treats as inside the table.
 */
static void apply_at(unsigned index)
{
    /* ROW_MAX is the array length. A runtime count is not enough for
     * -Warray-bounds: a uint8_t count still allows an index of 254. */
    if ( ROW_MAX <= index )
    {
        return;
    }
    if ( (unsigned)s_nrows <= index )
    {
        return;
    }
    s_row = (uint8_t)index;
    apply_row(&s_rows[index]);
}

static void overlay_step(const doom_controls_t *in, int16_t panel_h)
{
    int hit = -1;

    if ( 0U == s_nrows )
    {
        return;
    }
    if ( in->up_edge && (0U < s_row) )
    {
        s_row--;
    }
    if ( in->down_edge && ((unsigned)s_row + 1U) < (unsigned)s_nrows )
    {
        s_row++;
    }
    if ( in->left_edge || in->right_edge )
    {
        unsigned const index = s_row;

        if ( (index < ROW_MAX) && (index < (unsigned)s_nrows)
            && ((ROW_SFX == s_rows[index].kind) || (ROW_MUSIC == s_rows[index].kind)
                || (ROW_RUN == s_rows[index].kind)) )
        {
            apply_at(index);
        }
    }
    if ( in->fire_edge || in->use_edge )
    {
        apply_at(s_row);
    }
    if ( in->finger )
    {
        hit = row_at(in->y, panel_h);
    }
    if ( in->finger && !s_prev_finger )
    {
        s_down_row = hit;
    }
    if ( !in->finger && s_prev_finger && (hit == s_down_row) )
    {
        apply_at((unsigned)hit);
    }
    s_prev_finger = in->finger;
}

static void draw_overlay(uint16_t *fb, int16_t pw, int16_t ph, const char *heading)
{
    uint8_t i = 0U;
    uint16_t const ink = doom_rgb565(230U, 220U, 200U);
    static const char *sfx_name[] = { "OFF", "LOW", "HIGH" };

    doom_font_text(fb, (uint32_t)pw, (uint32_t)pw, (uint32_t)ph, 16, 20, heading, ink, 2U);
    for ( i = 0U; s_nrows > i; i++ )
    {
        int32_t const y = 64 + ((int32_t)i * 28);
        char line[32];
        const char *text = s_rows[i].label;

        if ( y > (int32_t)ph - 20 )
        {
            break;
        }
        if ( i == s_row )
        {
            doom_video_rect(fb, 12, (int16_t)y - 2, (int16_t)(pw - 24), 24,
                            doom_rgb565(64U, 24U, 20U));
        }
        if ( ROW_SFX == s_rows[i].kind )
        {
            snprintf(line, sizeof(line), "SOUND %s", sfx_name[s_sfx]);
            text = line;
        }
        else if ( ROW_MUSIC == s_rows[i].kind )
        {
            text = s_music ? "MUSIC ON" : "MUSIC OFF";
        }
        else if ( ROW_RUN == s_rows[i].kind )
        {
            text = s_run ? "ALWAYS RUN ON" : "ALWAYS RUN OFF";
        }
        else if ( ROW_STRAFE == s_rows[i].kind )
        {
            text = doom_engine_strafe_on() ? "STRAFE ON" : "STRAFE OFF";
        }
        doom_font_text(fb, (uint32_t)pw, (uint32_t)pw, (uint32_t)ph, 20, y, text, ink, 1U);
    }
}

static void start_game(const doom_catalog_t *cat)
{
    doom_launch_t launch;

    if ( doom_game_select_fill(cat, &launch) )
    {
        doom_engine_launch(&launch);
        doom_menu_publish_pick(('\0' != launch.addon[0]) ? launch.addon : launch.iwad);
        AppAo_post_pick();
        s_screen = SCREEN_PLAY;
    }
}

void doom_menu_publish_pick(const char *name)
{
    uint32_t n = 0U;

    atomic_fetch_add_explicit(&s_pick_seq, 1U, memory_order_release);
    memset(s_pick, 0, sizeof(s_pick));
    if ( NULL != name )
    {
        while ( ('\0' != name[n]) && ((n + 1U) < sizeof(s_pick)) )
        {
            s_pick[n] = name[n];
            n++;
        }
    }
    atomic_fetch_add_explicit(&s_pick_seq, 1U, memory_order_release);
}

bool doom_menu_copy_pick(char *out, uint32_t cap)
{
    char tmp[64];
    uint32_t a = 0U;
    uint32_t b = 0U;
    uint32_t n = 0U;
    bool any = false;

    if ( (NULL == out) || (2U > cap) )
    {
        return false;
    }
    do
    {
        a = atomic_load_explicit(&s_pick_seq, memory_order_acquire);
        if ( 0U != (a & 1U) )
        {
            continue;
        }
        memcpy(tmp, s_pick, sizeof(tmp));
        b = atomic_load_explicit(&s_pick_seq, memory_order_acquire);
    } while ( a != b );
    out[0] = '\0';
    while ( ('\0' != tmp[n]) && ((n + 1U) < cap) )
    {
        out[n] = tmp[n];
        n++;
    }
    out[n] = '\0';
    any = ('\0' != out[0]);
    return any;
}

void doom_menu_frame(uint16_t *panel, uint16_t *view, int16_t panel_w, int16_t panel_h)
{
    doom_controls_t in;
    uint32_t gen = 0U;
    const doom_catalog_t *cat = NULL;
    doom_select_action_t act = DOOM_SELECT_NONE;

    if ( NULL == panel )
    {
        return;
    }
    doom_input_gestures(SCREEN_PLAY == s_screen);
    doom_input_read(&in);
    cat = doom_file_catalog(&gen);
    doom_video_fill(panel, doom_rgb565(8U, 6U, 6U));
    if ( SCREEN_SELECT == s_screen )
    {
        act = doom_game_select_step(&in, cat, gen, panel_w, panel_h);
        if ( DOOM_SELECT_LAUNCH == act )
        {
            start_game(cat);
        }
        else if ( DOOM_SELECT_RESCAN == act )
        {
            AppAo_post_rescan();
        }
        doom_game_select_draw(panel, panel_w, panel_h, cat, gen);
    }
    else if ( SCREEN_PLAY == s_screen )
    {
        if ( NULL != view )
        {
            doom_engine_draw(view, &in);
            doom_video_scale_nearest(panel, view);
        }
        doom_font_text(panel, (uint32_t)panel_w, (uint32_t)panel_w, (uint32_t)panel_h,
                       16, 16, doom_engine_title(), doom_rgb565(230U, 210U, 180U), 1U);
        if ( DOOM_GESTURE_SINGLE == in.gesture )
        {
            arm_overlay(SCREEN_WEAPONS);
        }
        else if ( DOOM_GESTURE_DOUBLE == in.gesture )
        {
            arm_overlay(SCREEN_SETTINGS);
        }
    }
    else
    {
        overlay_step(&in, panel_h);
        draw_overlay(panel, panel_w, panel_h,
                     (SCREEN_SETTINGS == s_screen) ? "SETTINGS" : "WEAPONS");
    }
    (void)s_run;
}
