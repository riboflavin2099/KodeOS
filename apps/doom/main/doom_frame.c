/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_frame.c
 * @brief One present per loop, on core 1.
 */
#include "doom_frame.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "doom_audio.h"
#include "doom_menu.h"
#include "doom_video.h"
#include "kdrv_display.h"
#include "kode_log.h"
#include "kqp_config.h"

#define TAG "doom.frame"

#define DOOM_FRAME_STACK_BYTES  16384U
#define DOOM_RETRY_MS           20U

static void frame_task(void *arg)
{
    uint16_t *view = heap_caps_malloc(DOOM_FB_W * DOOM_FB_H * sizeof(uint16_t),
                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    (void)arg;
    (void)doom_audio_open();
    (void)doom_video_scale_ppa(NULL, NULL);
    if ( NULL == view )
    {
        KLOG_E(TAG, "no PSRAM for the 320x200 frame");
    }
    while ( true )
    {
        uint16_t *panel = doom_video_lock();

        if ( NULL == panel )
        {
            vTaskDelay(pdMS_TO_TICKS(DOOM_RETRY_MS));
        }
        else
        {
            doom_menu_frame(panel, view, (int16_t)KBSP_LCD_H_RES, (int16_t)KBSP_LCD_V_RES);
            doom_video_flip(panel);
        }
    }
}

bool doom_frame_start(void)
{
    BaseType_t const ok = xTaskCreatePinnedToCore(frame_task, "doom_frame",
                                                  DOOM_FRAME_STACK_BYTES, NULL,
                                                  (UBaseType_t)KODE_PRIO_USER_GUI,
                                                  NULL, 1);
    bool started = false;

    if ( pdPASS == ok )
    {
        started = true;
        KLOG_I(TAG, "frame task on core 1");
    }
    else
    {
        KLOG_E(TAG, "frame task did not start");
    }
    return started;
}
