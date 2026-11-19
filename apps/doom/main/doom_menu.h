/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_menu.h
 * @brief Select screen, weapon overlay, and settings overlay.
 *
 * Single tap while playing opens weapons and pauses. Double tap opens
 * settings and pauses. Choosing a weapon applies it and returns to the
 * game. Settings can return to the select screen.
 */
#ifndef DOOM_MENU_H_
#define DOOM_MENU_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Draw one frame and apply the controls.
 * @param panel   The glass. NULL draws nothing.
 * @param view    320x200 stand-in. NULL skips the play picture.
 * @param panel_w Panel width.
 * @param panel_h Panel height.
 */
void doom_menu_frame(uint16_t *panel, uint16_t *view, int16_t panel_w, int16_t panel_h);

/** @brief Publish the file name a save job should write. */
void doom_menu_publish_pick(const char *name);

/** @brief Copy the published name. False when none has been published. */
bool doom_menu_copy_pick(char *out, uint32_t cap);

#ifdef __cplusplus
}
#endif

#endif /* DOOM_MENU_H_ */
