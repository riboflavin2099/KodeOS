/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file app_main.c
 * @brief The app's entry point.
 */
#include "kode.h"

#include "app_ao.h"

KODE_APP_MANIFEST("DOOM", "games");

void app_main(void)
{
    kode_init();
    AppAo_ctor();
    AppAo_start();
}
