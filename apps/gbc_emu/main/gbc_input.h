/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_input.h
 * @brief The six buttons and two touch zones, as a Game Boy pad.
 */
#ifndef GBC_INPUT_H_
#define GBC_INPUT_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- The on-screen Start and Select zones ---- */

/** @brief Left edge of the Start zone, in glass pixels. */
#define GBC_TOUCH_START_X   45

/** @brief Top edge of both zones. Sits under the 2x picture. */
#define GBC_TOUCH_START_Y   372

/** @brief Width of one zone. */
#define GBC_TOUCH_BTN_W     150

/** @brief Height of one zone. */
#define GBC_TOUCH_BTN_H     72

/** @brief Gap between Start and Select. */
#define GBC_TOUCH_GAP       20

/** @brief Left edge of the Select zone. */
#define GBC_TOUCH_SELECT_X  (GBC_TOUCH_START_X + GBC_TOUCH_BTN_W + GBC_TOUCH_GAP)

/* ---- The pad ---- */

/**
 * @brief One sample of the controls the core will read.
 *
 * The pad is the D-pad. Top is A. Bottom is B while it is down; holding
 * Bottom is kodeOS's home gesture and is not given another meaning here.
 * Start and Select are the touch zones, because the Dot has two buttons
 * besides the pad.
 */
typedef struct {
    bool up;      /**< Pad up. */
    bool down;    /**< Pad down. */
    bool left;    /**< Pad left. */
    bool right;   /**< Pad right. */
    bool a;       /**< Top button. */
    bool b;       /**< Bottom button, level only. */
    bool start;   /**< Finger in the Start zone. */
    bool select;  /**< Finger in the Select zone. */
} gbc_pad_t;

/* ---- Calls ---- */

/**
 * @brief Read the buttons and the touch zones once.
 *
 * Safe on the frame task. Not for an Active Object: the touch read can wait
 * the first time it asks the panel for points.
 *
 * @param out Filled. A NULL out is ignored.
 */
void gbc_input_read(gbc_pad_t *out);

#ifdef __cplusplus
}
#endif

#endif /* GBC_INPUT_H_ */
