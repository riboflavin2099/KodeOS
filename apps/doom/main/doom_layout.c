/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_layout.c
 * @brief Logo-button geometry for the game select screen.
 */
#include "doom_layout.h"

#include <stddef.h>

void doom_layout_build(uint8_t count, uint8_t scroll,
                       int16_t panel_w, int16_t panel_h, doom_layout_t *out)
{
    uint8_t i = 0U;

    if ( NULL == out )
    {
        return;
    }
    out->visible = 0U;
    out->cols = 1U;
    out->origin = 0U;
    for ( i = 0U; DOOM_LAYOUT_VIS > i; i++ )
    {
        out->rect[i].x = 0;
        out->rect[i].y = 0;
        out->rect[i].w = 0;
        out->rect[i].h = 0;
    }
    if ( (0U == count) || (0 >= panel_w) || (0 >= panel_h) )
    {
        return;
    }
    if ( 1U == count )
    {
        out->visible = 1U;
        out->rect[0].w = (int16_t)(panel_w - 48);
        out->rect[0].h = (int16_t)(panel_h / 2);
        out->rect[0].x = (int16_t)((panel_w - out->rect[0].w) / 2);
        out->rect[0].y = (int16_t)((panel_h - out->rect[0].h) / 2);
    }
    else if ( 2U == count )
    {
        int16_t const gap = 16;
        int16_t const h = (int16_t)((panel_h - 120 - gap) / 2);

        out->visible = 2U;
        out->rect[0].w = (int16_t)(panel_w - 32);
        out->rect[0].h = h;
        out->rect[0].x = 16;
        out->rect[0].y = 64;
        out->rect[1] = out->rect[0];
        out->rect[1].y = (int16_t)(out->rect[0].y + h + gap);
    }
    else
    {
        int16_t const gap = 12;
        int16_t const margin = 16;
        int16_t cell_w = 0;
        int16_t cell_h = 150;
        uint8_t const cols = 2U;
        uint8_t rows_fit = 2U;
        uint8_t origin = 0U;
        uint8_t n = 0U;

        if ( scroll >= count )
        {
            scroll = (uint8_t)((count - 1U) & (uint8_t)~1U);
        }
        origin = (uint8_t)(scroll & (uint8_t)~1U);
        cell_w = (int16_t)((panel_w - (margin * 2) - gap) / (int16_t)cols);
        out->cols = cols;
        out->origin = origin;
        n = (uint8_t)(count - origin);
        if ( n > (rows_fit * cols) )
        {
            n = (uint8_t)(rows_fit * cols);
        }
        out->visible = n;
        for ( i = 0U; n > i; i++ )
        {
            uint8_t const col = (uint8_t)(i % cols);
            uint8_t const row = (uint8_t)(i / cols);

            out->rect[i].w = cell_w;
            out->rect[i].h = cell_h;
            out->rect[i].x = (int16_t)(margin + (col * (cell_w + gap)));
            out->rect[i].y = (int16_t)(72 + (row * (cell_h + gap)));
        }
    }
}

int doom_layout_hit(const doom_layout_t *lay, int16_t x, int16_t y)
{
    uint8_t i = 0U;
    int hit = -1;

    if ( NULL != lay )
    {
        for ( i = 0U; lay->visible > i; i++ )
        {
            doom_rect_t const *r = &lay->rect[i];

            if ( (r->x <= x) && (x < (int16_t)(r->x + r->w))
                && (r->y <= y) && (y < (int16_t)(r->y + r->h)) )
            {
                hit = (int)lay->origin + (int)i;
            }
        }
    }
    return hit;
}
