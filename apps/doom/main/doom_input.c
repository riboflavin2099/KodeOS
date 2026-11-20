/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_input.c
 * @brief Levels, edges, and the 300 ms tap window.
 */
#include "doom_input.h"

#include "kode.h"

typedef enum {
    GST_IDLE = 0,
    GST_DOWN,
    GST_PENDING
} gst_t;

static gst_t s_gst = GST_IDLE;
static bool s_armed = false;
static bool s_second = false;
static bool s_moved = false;
static int16_t s_x0 = 0;
static int16_t s_y0 = 0;
static uint32_t s_up_ms = 0U;
static bool s_prev_up = false;
static bool s_prev_down = false;
static bool s_prev_left = false;
static bool s_prev_right = false;
static bool s_prev_fire = false;
static bool s_prev_use = false;

static int16_t iabs16(int16_t v)
{
    return (0 > v) ? (int16_t)(-v) : v;
}

void doom_input_gestures(bool enabled)
{
    if ( !enabled )
    {
        s_gst = GST_IDLE;
        s_second = false;
    }
    s_armed = enabled;
}

void doom_input_read(doom_controls_t *out)
{
    kode_buttons_t keys;
    int16_t x = 0;
    int16_t y = 0;
    bool finger = false;
    uint32_t now = 0U;

    if ( NULL == out )
    {
        return;
    }
    keys = kode_button_read_all();
    finger = kode_touch_read(&x, &y);
    now = kode_uptime_ms();
    out->up = keys.up;
    out->down = keys.down;
    out->left = keys.left;
    out->right = keys.right;
    out->up_edge = keys.up && !s_prev_up;
    out->down_edge = keys.down && !s_prev_down;
    out->left_edge = keys.left && !s_prev_left;
    out->right_edge = keys.right && !s_prev_right;
    out->fire = keys.top;
    out->fire_edge = keys.top && !s_prev_fire;
    out->use_down = keys.bottom;
    out->use_edge = keys.bottom && !s_prev_use;
    out->finger = finger;
    out->x = x;
    out->y = y;
    out->gesture = DOOM_GESTURE_NONE;
    s_prev_up = keys.up;
    s_prev_down = keys.down;
    s_prev_left = keys.left;
    s_prev_right = keys.right;
    s_prev_fire = keys.top;
    s_prev_use = keys.bottom;

    if ( !s_armed )
    {
        s_gst = GST_IDLE;
        s_second = false;
    }
    else if ( finger )
    {
        if ( GST_PENDING == s_gst )
        {
            if ( DOOM_TAP_DOUBLE_MS >= (now - s_up_ms) )
            {
                s_second = true;
            }
            else
            {
                out->gesture = DOOM_GESTURE_SINGLE;
                s_second = false;
            }
            s_gst = GST_DOWN;
            s_moved = false;
            s_x0 = x;
            s_y0 = y;
        }
        else if ( GST_DOWN != s_gst )
        {
            s_gst = GST_DOWN;
            s_second = false;
            s_moved = false;
            s_x0 = x;
            s_y0 = y;
        }
        else if ( (DOOM_TAP_SLOP_PX < iabs16((int16_t)(x - s_x0)))
                 || (DOOM_TAP_SLOP_PX < iabs16((int16_t)(y - s_y0))) )
        {
            s_moved = true;
        }
    }
    else if ( GST_DOWN == s_gst )
    {
        if ( s_moved )
        {
            s_gst = GST_IDLE;
            s_second = false;
        }
        else if ( s_second )
        {
            out->gesture = DOOM_GESTURE_DOUBLE;
            s_gst = GST_IDLE;
            s_second = false;
        }
        else
        {
            s_gst = GST_PENDING;
            s_up_ms = now;
        }
    }
    else if ( (GST_PENDING == s_gst) && (DOOM_TAP_DOUBLE_MS <= (now - s_up_ms)) )
    {
        out->gesture = DOOM_GESTURE_SINGLE;
        s_gst = GST_IDLE;
    }
}
