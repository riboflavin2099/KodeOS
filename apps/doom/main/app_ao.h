/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file app_ao.h
 * @brief The app's Active Object. It reacts and does not block.
 */
#ifndef APP_AO_H_
#define APP_AO_H_

#include "kode_sys.h"

#ifdef __cplusplus
extern "C" {
#endif

extern QActive * const AO_App;

void AppAo_ctor(void);
void AppAo_start(void);

/** @brief Ask the worker to write the last chosen file. Safe from the frame task. */
void AppAo_post_pick(void);

/** @brief Ask the worker to scan the card again. */
void AppAo_post_rescan(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_AO_H_ */
