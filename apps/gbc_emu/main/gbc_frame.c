/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_frame.c
 * @brief One frame per loop, on the core the Active Objects are not pinned to.
 */
#include "gbc_frame.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "gbc_audio.h"
#include "gbc_core.h"
#include "gbc_display.h"
#include "gbc_input.h"
#include "gbc_rom.h"
#include "kode_log.h"
#include "kqp_config.h"

/** @brief This file's log tag. */
#define TAG "gbc.frame"

/** @brief Stack of the frame task, in bytes. ESP-IDF's xTaskCreate counts bytes. */
#define GBC_FRAME_STACK_BYTES  16384U

/** @brief How often the task says it is still presenting, in frames. */
#define GBC_LOG_EVERY  120U

/** @brief Pause when the panel is not ready, so the task does not spin. */
#define GBC_RETRY_MS  20U

/**
 * @brief The frame loop.
 * @param arg Unused.
 */
static void frame_task(void *arg)
{
    uint16_t *lcd = heap_caps_malloc(GBC_LCD_PIXELS * sizeof(uint16_t),
                                     MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    uint32_t frames = 0U;

    (void)arg;
    (void)gbc_audio_open();
    if ( NULL == lcd )
    {
        KLOG_E(TAG, "no PSRAM for the %u byte LCD frame",
               (unsigned)(GBC_LCD_PIXELS * sizeof(uint16_t)));
    }
    while ( NULL != lcd )
    {
        gbc_pad_t pad;
        uint32_t rom_len = 0U;
        const uint8_t *rom = NULL;

        gbc_input_read(&pad);
        rom = gbc_rom_data(&rom_len);
        gbc_core_draw(lcd, &pad, rom, rom_len);
        if ( !gbc_display_present(lcd, &pad) )
        {
            vTaskDelay(pdMS_TO_TICKS(GBC_RETRY_MS));
        }
        frames++;
        if ( 0U == (frames % GBC_LOG_EVERY) )
        {
            KLOG_I(TAG, "presented %u frames, rom %u bytes",
                   (unsigned)frames, (unsigned)rom_len);
        }
    }
    while ( true )
    {
        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}

bool gbc_frame_start(void)
{
    BaseType_t const ok = xTaskCreatePinnedToCore(frame_task, "gbc_frame",
                                                  GBC_FRAME_STACK_BYTES, NULL,
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
