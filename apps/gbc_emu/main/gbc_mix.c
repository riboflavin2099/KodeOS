/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_mix.c
 * @brief 32768 Hz stereo to 48000 Hz mono. No SDK calls.
 */
#include "gbc_mix.h"

#include <stddef.h>

/** @brief Accumulator in units of the input rate. */
static uint32_t s_acc = 0U;

void gbc_mix_reset(void)
{
    s_acc = 0U;
}

uint32_t gbc_mix_push(const int16_t *stereo, uint32_t n_in,
                      int16_t *mono, uint32_t cap)
{
    uint32_t produced = 0U;
    uint32_t i = 0U;

    if ( (NULL == stereo) || (NULL == mono) )
    {
        return 0U;
    }
    while ( (i < n_in) && (produced < cap) )
    {
        int32_t const left = (int32_t)stereo[i * 2U];
        int32_t const right = (int32_t)stereo[(i * 2U) + 1U];
        int32_t const mid = (left + right) / 2;
        int16_t const sample = (int16_t)mid;

        s_acc += GBC_MIX_OUT_HZ;
        while ( (s_acc >= GBC_MIX_IN_HZ) && (produced < cap) )
        {
            mono[produced] = sample;
            produced++;
            s_acc -= GBC_MIX_IN_HZ;
        }
        i++;
    }
    return produced;
}
