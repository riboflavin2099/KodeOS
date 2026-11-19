/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_audio.c
 * @brief The SDK's microphone and speaker. Speech upload is not in this file.
 */
#include "grok_audio.h"

#include "kode.h"
#include "kode_log.h"

/** @brief This file's log tag. */
#define TAG "grok.audio"

bool grok_audio_listen(uint32_t max_ms)
{
    if ( (0U == max_ms) || (KODE_AUDIO_RECORD_MAX_MS < max_ms) )
    {
        return false;
    }
    if ( !kode_audio_record_start(max_ms) )
    {
        KLOG_W(TAG, "microphone: %s", kode_error());
        return false;
    }
    KLOG_I(TAG, "listening, up to %lu ms", (unsigned long)max_ms);
    return true;
}

bool grok_audio_listen_end(uint32_t *ms_out)
{
    kode_audio_record_status_t st;

    if ( !kode_audio_record_stop() )
    {
        KLOG_W(TAG, "stop: %s", kode_error());
        return false;
    }
    st = kode_audio_record_status();
    if ( NULL != ms_out )
    {
        *ms_out = st.ms;
    }
    KLOG_I(TAG, "heard %lu ms, %lu frames (not uploaded)",
           (unsigned long)st.ms, (unsigned long)st.frames);
    return true;
}

bool grok_audio_play(const int16_t *pcm, uint32_t frames)
{
    if ( (NULL == pcm) || (0U == frames) )
    {
        return false;
    }
    if ( !kode_audio_play_samples(pcm, frames) )
    {
        KLOG_W(TAG, "speaker: %s", kode_error());
        return false;
    }
    return true;
}
