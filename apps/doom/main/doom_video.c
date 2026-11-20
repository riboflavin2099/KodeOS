/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_video.c
 * @brief CPU nearest scale, and the PPA call the next step will make.
 *
 * kdrv_display.h is the LCD driver header. Catalogue rule C3 warns on that
 * include. The frame task is the renderer, the same arrangement Kody Kart
 * uses.
 */
#include "doom_video.h"

#include "kdrv_display.h"
#include "kode_log.h"

#define TAG "doom.video"

#define DOOM_FLUSH_WAIT_MS  30U

static bool s_flip_busy = false;
static bool s_said_down = false;
static bool s_said_ppa = false;

uint16_t *doom_video_lock(void)
{
    uint16_t *fb = NULL;
    bool ready = true;

    if ( s_flip_busy )
    {
        ready = kdrv_display_flush_wait(DOOM_FLUSH_WAIT_MS);
        if ( ready )
        {
            s_flip_busy = false;
        }
    }
    if ( ready )
    {
        fb = (uint16_t *)kdrv_display_back_buffer();
        if ( (NULL == fb) && !s_said_down )
        {
            KLOG_I(TAG, "panel back buffer not up yet");
            s_said_down = true;
        }
    }
    return fb;
}

void doom_video_flip(uint16_t *fb)
{
    kode_status_t st;

    if ( NULL == fb )
    {
        return;
    }
    st = kdrv_display_flush(0, 0, (int32_t)KBSP_LCD_H_RES, (int32_t)KBSP_LCD_V_RES,
                            fb, true);
    if ( KODE_OK == st )
    {
        s_flip_busy = true;
        s_said_down = false;
    }
}

void doom_video_fill(uint16_t *fb, uint16_t rgb)
{
    uint32_t n = 0U;
    uint32_t i = 0U;

    if ( NULL == fb )
    {
        return;
    }
    n = (uint32_t)KBSP_LCD_H_RES * (uint32_t)KBSP_LCD_V_RES;
    for ( i = 0U; n > i; i++ )
    {
        fb[i] = rgb;
    }
}

void doom_video_rect(uint16_t *fb, int16_t x, int16_t y, int16_t w, int16_t h,
                     uint16_t rgb)
{
    int16_t row = 0;
    int16_t col = 0;
    int16_t x1 = x;
    int16_t y1 = y;
    int16_t x2 = (int16_t)(x + w);
    int16_t y2 = (int16_t)(y + h);

    if ( (NULL == fb) || (0 >= w) || (0 >= h) )
    {
        return;
    }
    if ( 0 > x1 )
    {
        x1 = 0;
    }
    if ( 0 > y1 )
    {
        y1 = 0;
    }
    if ( x2 > (int16_t)KBSP_LCD_H_RES )
    {
        x2 = (int16_t)KBSP_LCD_H_RES;
    }
    if ( y2 > (int16_t)KBSP_LCD_V_RES )
    {
        y2 = (int16_t)KBSP_LCD_V_RES;
    }
    for ( row = y1; y2 > row; row++ )
    {
        uint16_t *dst = &fb[((uint32_t)row * (uint32_t)KBSP_LCD_H_RES) + (uint32_t)x1];

        for ( col = x1; x2 > col; col++ )
        {
            dst[col - x1] = rgb;
        }
    }
}

void doom_video_scale_nearest(uint16_t *panel, const uint16_t *src)
{
    int16_t dy = 0;

    if ( (NULL == panel) || (NULL == src) )
    {
        return;
    }
    for ( dy = 0; (int16_t)DOOM_VIEW_H > dy; dy++ )
    {
        int16_t dx = 0;
        uint32_t const sy = (uint32_t)((dy * 2) / 3);
        uint16_t *dst = &panel[(((uint32_t)DOOM_VIEW_Y + (uint32_t)dy)
                                * (uint32_t)KBSP_LCD_H_RES) + (uint32_t)DOOM_VIEW_X];
        const uint16_t *row = &src[sy * DOOM_FB_W];

        for ( dx = 0; (int16_t)DOOM_VIEW_W > dx; dx++ )
        {
            dst[dx] = row[(uint32_t)((dx * 4) / 5)];
        }
    }
}

bool doom_video_scale_ppa(uint16_t *panel, const uint16_t *src)
{
    (void)panel;
    (void)src;
    if ( !s_said_ppa )
    {
        /*
         * Next step, after a palette expand to RGB565:
         *   ppa_srm_oper_config_t cfg, client oper_type PPA_OPERATION_SRM
         *   in:  src, pic 320x200, block 320x200, RGB565
         *   out: panel, buffer_size KDRV_DISPLAY_FB_BYTES (not 410*502*2),
         *        pic KBSP_LCD_H_RES x KBSP_LCD_V_RES, RGB565
         *        block_offset_x = 8, block_offset_y = 101
         *   scale_x = 1.25f, scale_y = 1.5f, rotation 0, blocking
         * kdrv_ppa_prepare() cannot do this: it accepts scale 1 or 2 only,
         * and that client belongs to the screen mirror.
         */
        KLOG_I(TAG, "PPA scale path not armed; CPU nearest presents the frame");
        s_said_ppa = true;
    }
    return false;
}
