/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_audio.c
 * @brief 48 kHz mono sink. The mix always runs; the speaker stays closed.
 *
 * Opening the codec stream is AO_Audio's call (kdrv_es8311_stream_reset).
 * Until that request exists, samples are counted and dropped. Define
 * GBC_SPEAKER to 1 only after that open is wired.
 */
#include "gbc_audio.h"

#include "kode_log.h"

/** @brief This file's log tag. */
#define TAG "gbc.audio"

/** @brief So the refusal is logged once, not once a frame. */
static bool s_said = false;

#ifndef GBC_SPEAKER
/** @brief Speaker playback. Off until AO_Audio opens the stream. */
#define GBC_SPEAKER 0
#endif

kode_status_t gbc_audio_open(void)
{
    if ( !s_said )
    {
        KLOG_I(TAG, "speaker stream not opened; frames will be dropped");
        s_said = true;
    }
    return KODE_DENIED;
}

void gbc_audio_write(const int16_t *frames, size_t n)
{
    (void)frames;
    (void)n;
    (void)GBC_SPEAKER;
}
