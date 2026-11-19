/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_audio.c
 * @brief Silence until the mixer is linked.
 */
#include "doom_audio.h"

#include "kode_log.h"

#define TAG "doom.audio"

static bool s_said = false;

kode_status_t doom_audio_open(void)
{
    if ( !s_said )
    {
        KLOG_I(TAG, "speaker stream not opened; frames will be dropped");
        s_said = true;
    }
    return KODE_DENIED;
}

void doom_audio_write(const int16_t *frames, size_t n)
{
    (void)frames;
    (void)n;
}
