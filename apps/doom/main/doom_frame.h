/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_frame.h
 * @brief The task that presents one panel frame. It may block. The AO may not.
 */
#ifndef DOOM_FRAME_H_
#define DOOM_FRAME_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool doom_frame_start(void);

#ifdef __cplusplus
}
#endif

#endif /* DOOM_FRAME_H_ */
