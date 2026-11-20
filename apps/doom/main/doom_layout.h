/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_layout.h
 * @brief Place logo buttons for the games that were actually found.
 *
 * No SDK calls. A count of zero draws nothing. One game is one large
 * button. Two stack. Three or more are a two-column grid, four visible,
 * scrolled by a whole row.
 */
#ifndef DOOM_LAYOUT_H_
#define DOOM_LAYOUT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief One button, in panel pixels. */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} doom_rect_t;

/** @brief How many buttons one screen of the grid holds. */
#define DOOM_LAYOUT_VIS  4U

typedef struct {
    doom_rect_t rect[DOOM_LAYOUT_VIS]; /**< Visible buttons, in list order. */
    uint8_t visible;                   /**< How many rects are used. */
    uint8_t cols;                      /**< 1 or 2. */
    uint8_t origin;                    /**< Index in the full list of rect[0]. */
} doom_layout_t;

/**
 * @brief Lay out count buttons.
 *
 * scroll is the first index the caller wants visible. The grid snaps it
 * to a row. A count of zero leaves visible at 0.
 *
 * @param count   Games to show.
 * @param scroll  Preferred first visible index.
 * @param panel_w Panel width.
 * @param panel_h Panel height.
 * @param out     Filled. NULL is ignored.
 */
void doom_layout_build(uint8_t count, uint8_t scroll,
                       int16_t panel_w, int16_t panel_h, doom_layout_t *out);

/**
 * @brief Which full-list index contains the point, or -1.
 */
int doom_layout_hit(const doom_layout_t *lay, int16_t x, int16_t y);

#ifdef __cplusplus
}
#endif

#endif /* DOOM_LAYOUT_H_ */
