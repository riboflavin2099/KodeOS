/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file app_ao.h
 * @brief The app's Active Object: it reacts to events and never blocks.
 */
#ifndef APP_AO_H_
#define APP_AO_H_

#include "kode_sys.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- The Active Object ---- */

/** @brief The one instance, opaque outside app_ao.c. */
extern QActive * const AO_App;

/* ---- Calls ---- */

/**
 * @brief Construct the Active Object. Call after kode_init() and before AppAo_start().
 */
void AppAo_ctor(void);

/**
 * @brief Give the Active Object its task, queue and stack, and start it.
 */
void AppAo_start(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_AO_H_ */
