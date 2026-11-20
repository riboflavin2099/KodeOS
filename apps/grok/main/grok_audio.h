/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_audio.h
 * @brief Push-to-talk capture and a playback call for later TTS PCM.
 *
 * The codec is the ES8311 at 48 kHz mono s16. The microphone is the ZTS6211E.
 * These calls block and belong on the worker. They do not upload or decode.
 */
#ifndef GROK_AUDIO_H_
#define GROK_AUDIO_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The codec rate. Same number as KDRV_ES8311_RATE_HZ. */
#define GROK_AUDIO_RATE_HZ  48000U

/** @brief How long a hold records, unless the finger lifts first. */
#define GROK_AUDIO_PTT_MS   8000U

/**
 * @brief Start a recording of at most @p max_ms.
 *
 * Returns once the microphone is running, not when the time is up.
 *
 * @param max_ms 1 to the SDK's recording cap.
 * @return false when the microphone did not start.
 */
bool grok_audio_listen(uint32_t max_ms);

/**
 * @brief Stop the recording and report how long it ran.
 * @param ms_out Out, may be NULL: milliseconds recorded.
 * @return false when no recording was running.
 */
bool grok_audio_listen_end(uint32_t *ms_out);

/**
 * @brief Play mono s16 frames at GROK_AUDIO_RATE_HZ and wait until they are heard.
 *
 * The TTS step will call this with PCM from POST /v1/tts. Nothing calls it yet.
 *
 * @param pcm    The frames. NULL is refused.
 * @param frames How many, above 0.
 * @return false when they were not played.
 */
bool grok_audio_play(const int16_t *pcm, uint32_t frames);

#ifdef __cplusplus
}
#endif

#endif /* GROK_AUDIO_H_ */
