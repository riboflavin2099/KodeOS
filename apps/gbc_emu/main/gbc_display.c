/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_display.c
 * @brief 2x nearest-neighbour blit into the panel back buffer.
 *
 * kdrv_display.h is the LCD driver header. Rule C3 of the catalogue warns
 * on that include. The frame task is the renderer for this app, the same
 * arrangement Kody Kart uses, because a widget screen cannot present a
 * frame every panel refresh.
 */
#include "gbc_display.h"

#include "kdrv_display.h"
#include "kode_log.h"

/** @brief This file's log tag. */
#define TAG "gbc.display"

/** @brief How long to wait for the previous flip before skipping a frame. */
#define GBC_FLUSH_WAIT_MS  30U

/** @brief Left edge of the 2x picture. (410 - 320) / 2. */
#define GBC_VIEW_X  (((uint32_t)KBSP_LCD_H_RES - (GBC_LCD_W * GBC_LCD_SCALE)) / 2U)

/** @brief Top edge of the 2x picture. Leaves the touch zones clear underneath. */
#define GBC_VIEW_Y  48U

/** @brief True after a flush that the next present must wait out. */
static bool s_flip_busy = false;

/** @brief True once a missing back buffer has been logged. */
static bool s_said_down = false;

/**
 * @brief Pack an 8-bit channel colour into RGB565.
 * @param r Red, 0..255.
 * @param g Green, 0..255.
 * @param b Blue, 0..255.
 * @return The pixel.
 */
static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((uint16_t)(r >> 3) << 11)
                      | ((uint16_t)(g >> 2) << 5)
                      | (uint16_t)(b >> 3));
}

/**
 * @brief Fill one axis-aligned rectangle of the panel buffer.
 * @param fb  The back buffer, stride KBSP_LCD_H_RES.
 * @param x   Left.
 * @param y   Top.
 * @param w   Width.
 * @param h   Height.
 * @param rgb The colour.
 */
static void fill_rect(uint16_t *fb, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                      uint16_t rgb)
{
    uint32_t row = 0U;

    for ( row = 0U; h > row; row++ )
    {
        uint32_t col = 0U;
        uint16_t *dst = &fb[(((y + row) * (uint32_t)KBSP_LCD_H_RES) + x)];

        for ( col = 0U; w > col; col++ )
        {
            dst[col] = rgb;
        }
    }
}

/**
 * @brief Copy the Game Boy frame at 2x into the panel buffer.
 * @param fb   The back buffer.
 * @param lcd  160x144 RGB565.
 */
static void blit_2x(uint16_t *fb, const uint16_t *lcd)
{
    uint32_t y = 0U;

    for ( y = 0U; GBC_LCD_H > y; y++ )
    {
        uint32_t x = 0U;
        uint32_t const dy = GBC_VIEW_Y + (y * GBC_LCD_SCALE);

        for ( x = 0U; GBC_LCD_W > x; x++ )
        {
            uint16_t const px = lcd[(y * GBC_LCD_W) + x];
            uint32_t const dx = GBC_VIEW_X + (x * GBC_LCD_SCALE);
            uint32_t const stride = (uint32_t)KBSP_LCD_H_RES;

            fb[(dy * stride) + dx] = px;
            fb[(dy * stride) + dx + 1U] = px;
            fb[((dy + 1U) * stride) + dx] = px;
            fb[((dy + 1U) * stride) + dx + 1U] = px;
        }
    }
}

bool gbc_display_present(const uint16_t *lcd, const gbc_pad_t *pad)
{
    bool queued = false;
    void *back = NULL;
    bool ready = true;

    if ( NULL == lcd )
    {
        ready = false;
    }
    if ( ready && s_flip_busy )
    {
        ready = kdrv_display_flush_wait(GBC_FLUSH_WAIT_MS);
        if ( ready )
        {
            s_flip_busy = false;
        }
    }
    if ( ready )
    {
        back = kdrv_display_back_buffer();
        if ( NULL == back )
        {
            ready = false;
            if ( !s_said_down )
            {
                KLOG_I(TAG, "panel back buffer not up yet");
                s_said_down = true;
            }
        }
    }
    if ( ready )
    {
        uint16_t *fb = (uint16_t *)back;
        uint32_t i = 0U;
        uint32_t const n = (uint32_t)KBSP_LCD_H_RES * (uint32_t)KBSP_LCD_V_RES;
        uint16_t const start_rgb = rgb565(40U, 70U, 48U);
        uint16_t const select_rgb = rgb565(40U, 70U, 48U);
        uint16_t const down_rgb = rgb565(155U, 188U, 15U);
        uint16_t start_col = start_rgb;
        uint16_t select_col = select_rgb;
        kode_status_t st;

        for ( i = 0U; n > i; i++ )
        {
            fb[i] = 0U;
        }
        blit_2x(fb, lcd);
        if ( (NULL != pad) && pad->start )
        {
            start_col = down_rgb;
        }
        if ( (NULL != pad) && pad->select )
        {
            select_col = down_rgb;
        }
        fill_rect(fb, (uint32_t)GBC_TOUCH_START_X, (uint32_t)GBC_TOUCH_START_Y,
                  (uint32_t)GBC_TOUCH_BTN_W, (uint32_t)GBC_TOUCH_BTN_H, start_col);
        fill_rect(fb, (uint32_t)GBC_TOUCH_SELECT_X, (uint32_t)GBC_TOUCH_START_Y,
                  (uint32_t)GBC_TOUCH_BTN_W, (uint32_t)GBC_TOUCH_BTN_H, select_col);
        st = kdrv_display_flush(0, 0, (int32_t)KBSP_LCD_H_RES, (int32_t)KBSP_LCD_V_RES,
                                fb, true);
        if ( KODE_OK == st )
        {
            s_flip_busy = true;
            s_said_down = false;
            queued = true;
        }
    }
    return queued;
}
