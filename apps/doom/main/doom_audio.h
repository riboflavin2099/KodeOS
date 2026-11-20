/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_audio.h
 * @brief The 48 kHz mono stream. Not opened in this scaffold.
 */
#ifndef DOOM_AUDIO_H_
#define DOOM_AUDIO_H_

#include <stddef.h>
#include <stdint.h>

#include "kode_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The ES8311 rate. Doom's 11025 Hz effects are mixed up to this. */
#define DOOM_AUDIO_RATE_HZ  48000U

/**
 * @brief Ask for the speaker stream.
 * @return KODE_DENIED until AO_Audio opens kdrv_es8311_stream_reset().
 */
kode_status_t doom_audio_open(void);

/** @brief Queue mono s16 frames. Dropped while the stream is closed. */
void doom_audio_write(const int16_t *frames, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* DOOM_AUDIO_H_ */
