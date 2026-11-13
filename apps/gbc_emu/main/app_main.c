/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file app_main.c
 * @brief The app's entry point: bring the system up, then start the Active Object.
 */
#include "kode.h"

#include "app_ao.h"

/* The name on the launcher's pill, and one category from the catalogue list. */
KODE_APP_MANIFEST("GBC", "games");

/**
 * @brief ESP-IDF's entry point.
 */
void app_main(void)
{
    /* Nothing may post to an Active Object before its queue exists. */
    kode_init();

    AppAo_ctor();
    AppAo_start();
}
