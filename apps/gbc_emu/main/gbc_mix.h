/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_mix.h
 * @brief Turn one frame of 32768 Hz stereo into 48000 Hz mono.
 */
#ifndef GBC_MIX_H_
#define GBC_MIX_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Rates ---- */

/** @brief minigb_apu's native rate, matching the Game Boy clock. */
#define GBC_MIX_IN_HZ   32768U

/** @brief The ES8311 rate. */
#define GBC_MIX_OUT_HZ  48000U

/* ---- Calls ---- */

/**
 * @brief Forget leftover phase. Call when a new cartridge starts.
 */
void gbc_mix_reset(void);

/**
 * @brief Resample one block.
 *
 * Zero-order hold. The phase carries across calls so frames join.
 *
 * @param stereo  Interleaved left, right, s16. n_in pairs.
 * @param n_in    How many stereo frames.
 * @param mono    Output samples.
 * @param cap     Room in mono.
 * @return How many mono samples were written. Less than the ratio asks for
 *         when cap runs out; the leftover input is dropped.
 */
uint32_t gbc_mix_push(const int16_t *stereo, uint32_t n_in,
                      int16_t *mono, uint32_t cap);

#ifdef __cplusplus
}
#endif

#endif /* GBC_MIX_H_ */
