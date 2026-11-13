/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_core.c
 * @brief Stand-in for Walnut-CGB. Draws a pattern, runs no guest code.
 */
#include "gbc_core.h"

#include "gbc_display.h"
#include "kode.h"

/** @brief Unix time handed to the core, or -1 when none has been. */
static int64_t s_unix_sec = -1;

/** @brief Idle picture, no cartridge: dark green. */
#define GBC_COL_IDLE  0x0320U

/** @brief Stand-in picture once a ROM is in PSRAM, clock not set. */
#define GBC_COL_STUB  0x294AU

/** @brief Stand-in picture once a ROM is in PSRAM and a clock was stored. */
#define GBC_COL_CLOCK 0x2D4AU

/** @brief White border on the idle picture. */
#define GBC_COL_WHITE 0xFFFFU

/** @brief Amber border on the stand-in, so the two states are distinct. */
#define GBC_COL_AMBER 0xFFE0U

/**
 * @brief Fill the Game Boy frame with one colour.
 * @param lcd The frame.
 * @param rgb The colour.
 */
static void fill_lcd(uint16_t *lcd, uint16_t rgb)
{
    uint32_t i = 0U;

    for ( i = 0U; GBC_LCD_PIXELS > i; i++ )
    {
        lcd[i] = rgb;
    }
}

/**
 * @brief Draw a one-pixel frame inside the Game Boy image, offset by phase.
 * @param lcd   The frame.
 * @param phase Scrolls the border so a stalled loop is obvious.
 * @param rgb   The border colour.
 */
static void border(uint16_t *lcd, uint32_t phase, uint16_t rgb)
{
    uint32_t x = 0U;
    uint32_t y = 0U;
    uint32_t const top = phase % GBC_LCD_H;

    for ( x = 0U; GBC_LCD_W > x; x++ )
    {
        lcd[(top * GBC_LCD_W) + x] = rgb;
        lcd[x] = rgb;
        lcd[((GBC_LCD_H - 1U) * GBC_LCD_W) + x] = rgb;
    }
    for ( y = 0U; GBC_LCD_H > y; y++ )
    {
        lcd[(y * GBC_LCD_W)] = rgb;
        lcd[(y * GBC_LCD_W) + (GBC_LCD_W - 1U)] = rgb;
    }
}

void gbc_core_set_clock(int64_t unix_sec)
{
    s_unix_sec = unix_sec;
}

void gbc_core_draw(uint16_t *lcd, const gbc_pad_t *pad, const uint8_t *rom, uint32_t rom_len)
{
    if ( NULL != lcd )
    {
        uint32_t const phase = kode_uptime_ms() / 32U;
        bool a_down = false;

        if ( (NULL != pad) && pad->a )
        {
            a_down = true;
        }
        if ( (NULL == rom) || (0U == rom_len) )
        {
            fill_lcd(lcd, GBC_COL_IDLE);
            border(lcd, phase, GBC_COL_WHITE);
        }
        else
        {
            /* A loaded image, and no core to run it. The border moves so
             * the frame task is visibly alive. A set clock changes the fill,
             * and A lights the top-left pixel. */
            uint16_t fill = GBC_COL_STUB;

            if ( 0 <= s_unix_sec )
            {
                fill = GBC_COL_CLOCK;
            }
            fill_lcd(lcd, fill);
            border(lcd, phase, GBC_COL_AMBER);
            if ( a_down )
            {
                lcd[0] = GBC_COL_WHITE;
            }
        }
    }
}
