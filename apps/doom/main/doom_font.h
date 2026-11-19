/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_font.h
 * @brief A small text face for the select screen and the overlays.
 *
 * The bitmaps are original to this scaffold. They are not a game logo.
 */
#ifndef DOOM_FONT_H_
#define DOOM_FONT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Draw text. scale is 1, 2 or 3. Lower case is drawn as upper case.
 */
void doom_font_text(uint16_t *fb, uint32_t stride, uint32_t panel_w, uint32_t panel_h,
                    int32_t x, int32_t y, const char *text, uint16_t color,
                    uint8_t scale);

#ifdef __cplusplus
}
#endif

#endif /* DOOM_FONT_H_ */
