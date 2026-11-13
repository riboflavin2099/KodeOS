/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_audio.c
 * @brief Placeholder for the 48 kHz mono stream. Silence until the APU is linked.
 */
#include "gbc_audio.h"

#include "kode_log.h"

/** @brief This file's log tag. */
#define TAG "gbc.audio"

/** @brief So the refusal is logged once, not once a frame. */
static bool s_said = false;

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
}
