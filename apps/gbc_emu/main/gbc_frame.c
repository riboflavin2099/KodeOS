/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_frame.c
 * @brief One frame per loop, on the core the Active Objects are not pinned to.
 */
#include "gbc_frame.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "esp_memory_utils.h"
#include "esp_timer.h"
#include "app_ao.h"
#include "gbc_audio.h"
#include "gbc_core.h"
#include "gbc_display.h"
#include "gbc_input.h"
#include "gbc_rom.h"
#include "kode_log.h"
#include "kqp_config.h"

/** @brief This file's log tag. */
#define TAG "gbc.frame"

/** @brief Stack of the frame task, in bytes. ESP-IDF's xTaskCreate counts bytes.
 *  Walnut's line renderer keeps a scanline on this stack, so it is larger
 *  than the Active Object's. The allocation is internal RAM, not a static. */
#define GBC_FRAME_STACK_BYTES  24576U

/** @brief How often the task says it is still presenting, in frames. */
#define GBC_LOG_EVERY  120U

/** @brief How often a battery write is requested, in guest frames. */
#define GBC_SAVE_EVERY  300U

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
    uint32_t dropped = 0U;

    (void)arg;
    (void)gbc_audio_open();
    if ( NULL == lcd )
    {
        KLOG_E(TAG, "no PSRAM for the %u byte LCD frame",
               (unsigned)(GBC_LCD_PIXELS * sizeof(uint16_t)));
    }
    else
    {
        unsigned lcd_psram = 0U;

        if ( esp_ptr_external_ram(lcd) )
        {
            lcd_psram = 1U;
        }
        KLOG_I(TAG, "lcd buffer %u bytes, psram %u",
               (unsigned)(GBC_LCD_PIXELS * sizeof(uint16_t)), lcd_psram);
    }
    while ( NULL != lcd )
    {
        gbc_pad_t pad;
        uint32_t rom_len = 0U;
        const uint8_t *rom = NULL;
        int64_t const t0 = esp_timer_get_time();
        gbc_core_stats_t stats;
        uint32_t loop_us = 0U;
        unsigned lcd_psram = 0U;
        unsigned gb_psram = 0U;
        unsigned rom_psram = 0U;
        unsigned ram_psram = 0U;

        gbc_input_read(&pad);
        rom = gbc_rom_data(&rom_len);
        gbc_core_draw(lcd, &pad, rom, rom_len);
        if ( !gbc_display_present(lcd, &pad) )
        {
            dropped++;
            vTaskDelay(pdMS_TO_TICKS(GBC_RETRY_MS));
        }
        loop_us = (uint32_t)(esp_timer_get_time() - t0);
        frames++;
        if ( (0U == (frames % GBC_SAVE_EVERY)) && gbc_core_save_pending() )
        {
            AppAo_request_save();
        }
        if ( 0U == (frames % GBC_LOG_EVERY) )
        {
            gbc_core_stats(&stats);
            if ( esp_ptr_external_ram(lcd) )
            {
                lcd_psram = 1U;
            }
            if ( stats.gb_external )
            {
                gb_psram = 1U;
            }
            if ( stats.rom_external )
            {
                rom_psram = 1U;
            }
            if ( stats.ram_external )
            {
                ram_psram = 1U;
            }
            KLOG_I(TAG,
                   "emu last %u us avg %u us worst %u us loop %u us drops %u frames %u audio %u psram lcd %u gb %u rom %u ram %u",
                   (unsigned)stats.last_us, (unsigned)stats.avg_us,
                   (unsigned)stats.worst_us, (unsigned)loop_us,
                   (unsigned)dropped, (unsigned)frames,
                   (unsigned)stats.audio_frames,
                   lcd_psram, gb_psram, rom_psram, ram_psram);
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
