/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_game_select.h
 * @brief Logo buttons for the games the scan actually found.
 *
 * One game is still this screen, with one large button. A tap launches.
 * The D-pad moves a highlight and button 1 launches it. Add-ons launch
 * only when their base IWAD is on the card too.
 */
#ifndef DOOM_GAME_SELECT_H_
#define DOOM_GAME_SELECT_H_

#include <stdint.h>

#include "doom_engine.h"
#include "doom_file.h"
#include "doom_input.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DOOM_SELECT_NONE = 0,
    DOOM_SELECT_LAUNCH,
    DOOM_SELECT_RESCAN
} doom_select_action_t;

doom_select_action_t doom_game_select_step(const doom_controls_t *in,
                                           const doom_catalog_t *cat, uint32_t gen,
                                           int16_t panel_w, int16_t panel_h);

void doom_game_select_draw(uint16_t *fb, int16_t panel_w, int16_t panel_h,
                           const doom_catalog_t *cat, uint32_t gen);

/**
 * @brief Fill a launch from the button that was just chosen.
 * @return false when an add-on has no base yet. The note says which file.
 */
bool doom_game_select_fill(const doom_catalog_t *cat, doom_launch_t *out);

#ifdef __cplusplus
}
#endif

#endif /* DOOM_GAME_SELECT_H_ */
