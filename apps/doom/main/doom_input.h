/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_input.h
 * @brief D-pad, the two side buttons, and touch taps.
 *
 * Button 1 is the top button and it is fire. Fire is a level: holding it
 * keeps firing. The top button's hold event is not home.
 *
 * Button 2 is the bottom button and it is use, on the press edge. Holding
 * the bottom button is kodeOS home (KODE_BUTTON_HOLD_MS is 600, then
 * KODE_SYS_HOME_GRACE_MS is 200). Use does not need a hold, so fire is not
 * placed on that button.
 *
 * A tap on the glass is resolved only while doom_input_gestures() is on,
 * which is while a game is running and no overlay is up. The first finger-up
 * waits DOOM_TAP_DOUBLE_MS. A second tap that starts inside that window is a
 * double tap. Otherwise the wait ends as a single tap. The game keeps
 * running during the wait.
 */
#ifndef DOOM_INPUT_H_
#define DOOM_INPUT_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief How long a single tap waits to see if it was a double, in ms. */
#define DOOM_TAP_DOUBLE_MS  300U

/** @brief Movement past this many pixels is not a tap. */
#define DOOM_TAP_SLOP_PX    24

typedef enum {
    DOOM_GESTURE_NONE = 0,
    DOOM_GESTURE_SINGLE, /**< Weapon overlay. Arrives DOOM_TAP_DOUBLE_MS after finger-up. */
    DOOM_GESTURE_DOUBLE  /**< Settings overlay. The second tap started inside the window. */
} doom_gesture_t;

/**
 * @brief One sample.
 *
 * Edges are true for the sample where the control went down. fire stays
 * true for the whole hold. gesture is NONE on samples that did not resolve a tap.
 */
typedef struct {
    bool up;
    bool down;
    bool left;
    bool right;
    bool up_edge;
    bool down_edge;
    bool left_edge;
    bool right_edge;
    bool fire;          /**< Top button, level. Button 1. */
    bool fire_edge;     /**< Top button went down. */
    bool use_edge;      /**< Bottom button went down. Button 2. */
    bool use_down;      /**< Bottom button level. A long hold leaves for kodeOS. */
    doom_gesture_t gesture;
    bool finger;
    int16_t x;
    int16_t y;
} doom_controls_t;

/**
 * @brief Arm or silence tap resolution.
 *
 * Silencing clears a tap that was still waiting, so an overlay does not
 * open a second overlay underneath itself.
 */
void doom_input_gestures(bool enabled);

/**
 * @brief Read the pad, the buttons and the glass once.
 *
 * The touch read can block the first time it asks the panel. Call this from
 * the frame task, not from an Active Object.
 */
void doom_input_read(doom_controls_t *out);

#ifdef __cplusplus
}
#endif

#endif /* DOOM_INPUT_H_ */
