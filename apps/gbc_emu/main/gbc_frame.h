/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_frame.h
 * @brief The task that samples input, draws one frame and presents it.
 */
#ifndef GBC_FRAME_H_
#define GBC_FRAME_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Calls ---- */

/**
 * @brief Start the frame task on core 1.
 *
 * The task is not an Active Object because presenting a frame waits on the
 * panel DMA. Call it after kode_init(), from the app Active Object's initial
 * transition.
 *
 * @return true when the task was created.
 */
bool gbc_frame_start(void);

#ifdef __cplusplus
}
#endif

#endif /* GBC_FRAME_H_ */
