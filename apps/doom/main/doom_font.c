/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_font.c
 * @brief Draw the scaffold's 5x7 face.
 */
#include "doom_font.h"

#include <stddef.h>

#include "doom_font_glyphs.inc"

void doom_font_text(uint16_t *fb, uint32_t stride, uint32_t panel_w, uint32_t panel_h,
                    int32_t x, int32_t y, const char *text, uint16_t color,
                    uint8_t scale)
{
    int32_t pen = x;
    const char *p = text;

    if ( (NULL == fb) || (NULL == text) || (0U == scale) )
    {
        return;
    }
    if ( 3U < scale )
    {
        scale = 3U;
    }
    for ( p = text; '\0' != *p; p++ )
    {
        unsigned char ch = (unsigned char)*p;
        const uint8_t *glyph = NULL;
        uint8_t row = 0U;

        if ( ('a' <= ch) && ('z' >= ch) )
        {
            ch = (unsigned char)(ch - ('a' - 'A'));
        }
        if ( (32U > ch) || (126U < ch) )
        {
            ch = (unsigned char)'?';
        }
        glyph = k_glyph[ch - 32U];
        for ( row = 0U; 7U > row; row++ )
        {
            uint8_t col = 0U;

            for ( col = 0U; 5U > col; col++ )
            {
                uint8_t sy = 0U;

                if ( 0U == (glyph[row] & (uint8_t)(1U << (4U - col))) )
                {
                    continue;
                }
                for ( sy = 0U; scale > sy; sy++ )
                {
                    uint8_t sx = 0U;
                    int32_t py = y + (int32_t)((row * scale) + sy);

                    if ( (0 > py) || ((int32_t)panel_h <= py) )
                    {
                        continue;
                    }
                    for ( sx = 0U; scale > sx; sx++ )
                    {
                        int32_t px = pen + (int32_t)((col * scale) + sx);

                        if ( (0 <= px) && ((int32_t)panel_w > px) )
                        {
                            fb[((uint32_t)py * stride) + (uint32_t)px] = color;
                        }
                    }
                }
            }
        }
        pen += (int32_t)(6U * scale);
    }
}
