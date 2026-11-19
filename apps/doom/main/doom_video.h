/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_video.h
 * @brief Present a 320x200 frame on the 410x502 panel.
 *
 * The picture is 400x300, which is 4:3, at (8, 101). That is scale 1.25 by
 * 1.5, and both factors are exact sixteenths, which is the PPA's step.
 * The scaffold scales on the CPU. doom_video_scale_ppa() is the hardware
 * path and it refuses until a second SRM client is registered.
 */
#ifndef DOOM_VIDEO_H_
#define DOOM_VIDEO_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DOOM_FB_W     320U
#define DOOM_FB_H     200U
#define DOOM_VIEW_X   8
#define DOOM_VIEW_Y   101
#define DOOM_VIEW_W   400
#define DOOM_VIEW_H   300

static inline uint16_t doom_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((uint16_t)(r >> 3) << 11)
                      | ((uint16_t)(g >> 2) << 5)
                      | (uint16_t)(b >> 3));
}

/**
 * @brief The panel back buffer after the previous flip has finished.
 * @return NULL while the panel is not up. The stride is the panel width.
 */
uint16_t *doom_video_lock(void);

/** @brief Flip a buffer from doom_video_lock(). */
void doom_video_flip(uint16_t *fb);

void doom_video_fill(uint16_t *fb, uint16_t rgb);

void doom_video_rect(uint16_t *fb, int16_t x, int16_t y, int16_t w, int16_t h,
                     uint16_t rgb);

/** @brief Nearest 320x200 to the 400x300 window. */
void doom_video_scale_nearest(uint16_t *panel, const uint16_t *src);

/**
 * @brief PPA scale into the back buffer.
 * @return false. The mirror's kdrv_ppa_prepare() only halves. A game frame
 *         needs its own SRM client. See FEASIBILITY.md.
 */
bool doom_video_scale_ppa(uint16_t *panel, const uint16_t *src);

#ifdef __cplusplus
}
#endif

#endif /* DOOM_VIDEO_H_ */
