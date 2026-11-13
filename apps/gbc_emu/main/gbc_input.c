/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_input.c
 * @brief Map the Dot's buttons and two touch zones onto a Game Boy pad.
 */
#include "gbc_input.h"

#include "kode.h"

/**
 * @brief Whether the point is inside a glass rectangle, edges inclusive on the origin side.
 * @param x  Finger x.
 * @param y  Finger y.
 * @param rx Left edge.
 * @param ry Top edge.
 * @param rw Width.
 * @param rh Height.
 * @return true when the point is inside.
 */
static bool in_rect(int16_t x, int16_t y, int32_t rx, int32_t ry, int32_t rw, int32_t rh)
{
    int32_t const px = (int32_t)x;
    int32_t const py = (int32_t)y;
    bool hit = false;

    if ( (rx <= px) && (px < (rx + rw)) && (ry <= py) && (py < (ry + rh)) )
    {
        hit = true;
    }
    return hit;
}

void gbc_input_read(gbc_pad_t *out)
{
    if ( NULL != out )
    {
        kode_buttons_t const keys = kode_button_read_all();
        int16_t x = 0;
        int16_t y = 0;
        bool const finger = kode_touch_read(&x, &y);

        out->up = keys.up;
        out->down = keys.down;
        out->left = keys.left;
        out->right = keys.right;
        out->a = keys.top;
        out->b = keys.bottom;
        out->start = false;
        out->select = false;
        if ( finger )
        {
            out->start = in_rect(x, y, GBC_TOUCH_START_X, GBC_TOUCH_START_Y,
                                 GBC_TOUCH_BTN_W, GBC_TOUCH_BTN_H);
            out->select = in_rect(x, y, GBC_TOUCH_SELECT_X, GBC_TOUCH_START_Y,
                                  GBC_TOUCH_BTN_W, GBC_TOUCH_BTN_H);
        }
    }
}
