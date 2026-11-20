/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_engine.h
 * @brief The seat Chocolate Doom will occupy. Nothing here runs a tic.
 */
#ifndef DOOM_ENGINE_H_
#define DOOM_ENGINE_H_

#include <stdbool.h>
#include <stdint.h>

#include "doom_input.h"
#include "doom_wad.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief What the select screen asked to start. addon is empty for an IWAD. */
typedef struct {
    char iwad[64];
    char addon[64];
    char title[24];
    doom_engine_t engine;
    doom_base_t base;
} doom_launch_t;

void doom_engine_launch(const doom_launch_t *launch);
void doom_engine_stop(void);
bool doom_engine_running(void);
void doom_engine_pause(bool paused);
bool doom_engine_paused(void);

/** @brief Remember a weapon slot. The engine is not linked, so this is stored. */
void doom_engine_weapon(int slot);
void doom_engine_strafe(bool on);
bool doom_engine_strafe_on(void);
void doom_engine_jump(void);
void doom_engine_inventory(int dir);

const char *doom_engine_title(void);
doom_engine_t doom_engine_id(void);

/**
 * @brief Paint a 320x200 stand-in. Fire, use and the pad change the picture.
 * While paused, the picture does not advance.
 */
void doom_engine_draw(uint16_t *view, const doom_controls_t *in);

#ifdef __cplusplus
}
#endif

#endif /* DOOM_ENGINE_H_ */
