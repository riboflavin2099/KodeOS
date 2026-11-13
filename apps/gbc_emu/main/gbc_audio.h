/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_audio.h
 * @brief The speaker path the core will feed. Not opened in this scaffold.
 */
#ifndef GBC_AUDIO_H_
#define GBC_AUDIO_H_

#include <stddef.h>
#include <stdint.h>

#include "kode_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- The rate the codec actually runs at ---- */

/**
 * @brief Samples per second the ES8311 is wired for.
 *
 * minigb_apu's own default is 32768 Hz stereo. The codec does not play that.
 * The next step resamples, or builds the APU at this rate, and then writes
 * the frames with kdrv_es8311_stream_write(). Opening that stream is
 * AO_Audio's job; this task only writes frames once the stream is open.
 */
#define GBC_AUDIO_RATE_HZ  48000U

/* ---- Calls ---- */

/**
 * @brief Ask for the speaker stream.
 *
 * The scaffold does not open it. The call records that, once, and refuses.
 *
 * @return KODE_DENIED until the stream is wired to AO_Audio.
 */
kode_status_t gbc_audio_open(void);

/**
 * @brief Queue mono frames. Dropped until gbc_audio_open() succeeds.
 *
 * @param frames  Mono s16 frames. Ignored while the stream is closed.
 * @param n       How many frames.
 */
void gbc_audio_write(const int16_t *frames, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* GBC_AUDIO_H_ */
