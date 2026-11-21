/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_engine.c
 * @brief Run Chocolate Doom, Heretic or Hexen on its own task.
 *
 * The frame task only copies the latest RGB565 view and posts the pad.
 * Zone memory and the view buffers live in PSRAM. Heretic and Hexen are
 * linked with a symbol prefix; this file calls those prefixed entry points.
 */
#include "doom_engine.h"

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "doom_port.h"
#include "kode.h"
#include "kode_log.h"
#include "kqp_config.h"
#include "m_argv.h"
#include "m_controls.h"

#define TAG "doom.engine"
#define ENGINE_STACK 65536U
#define DOOM_ARG_SLOTS 16
#define ARG_LEN 192

extern void D_DoomMain(void);
extern void heretic_D_DoomMain(void);
extern void hexen_D_DoomMain(void);
extern void M_SetExeDir(void);

static doom_launch_t s_launch;
static bool s_running = false;
static bool s_paused = false;
static bool s_strafe = false;
static bool s_run = true;
static volatile int s_alive = 0;
static TaskHandle_t s_task = NULL;
static jmp_buf s_jmp;
static char s_store[DOOM_ARG_SLOTS][ARG_LEN];
static char *s_args[DOOM_ARG_SLOTS];
static int s_argc = 0;

static void add_arg(const char *text)
{
    if ( (text == NULL) || (s_argc >= DOOM_ARG_SLOTS) )
    {
        return;
    }
    snprintf(s_store[s_argc], ARG_LEN, "%s", text);
    s_args[s_argc] = s_store[s_argc];
    s_argc++;
}

static void to_posix(const char *file, char *out, size_t cap)
{
    const char *dirs[2] = { "sd/doom", "sd/doom/pwads" };
    unsigned i = 0U;

    if ( (file == NULL) || (file[0] == '\0') )
    {
        snprintf(out, cap, "%s", "");
        return;
    }
    if ( (file[0] == '/') || (strncmp(file, "sd/", 3) == 0) )
    {
        if ( strncmp(file, "sd/", 3) == 0 )
        {
            snprintf(out, cap, "/sdcard/%s", file + 3);
        }
        else
        {
            snprintf(out, cap, "%s", file);
        }
        return;
    }
    for ( i = 0U; i < 2U; i++ )
    {
        char kode[160];

        snprintf(kode, sizeof(kode), "%s/%s", dirs[i], file);
        if ( kode_file_exists(kode) )
        {
            snprintf(out, cap, "/sdcard/%s", kode + 3);
            return;
        }
    }
    snprintf(out, cap, "/sdcard/doom/%s", file);
}

static int episode_warp(const doom_launch_t *launch)
{
    if ( (launch->engine == DOOM_ENGINE_DOOM2) || (launch->engine == DOOM_ENGINE_HEXEN)
         || (launch->base == DOOM_BASE_TNT) || (launch->base == DOOM_BASE_PLUTONIA) )
    {
        return 0;
    }
    return 1;
}

static void build_args(const doom_launch_t *launch)
{
    char iwad[ARG_LEN];
    char addon[ARG_LEN];
    const char *ep = "1";
    const char *map = "1";

    s_argc = 0;
    to_posix(launch->iwad, iwad, sizeof(iwad));
    to_posix(launch->addon, addon, sizeof(addon));
    if ( strstr(launch->title, "SIGIL II") != NULL )
    {
        ep = "6";
    }
    else if ( strstr(launch->title, "SIGIL") != NULL )
    {
        ep = "5";
    }
    add_arg("/sdcard/doom/doom");
    add_arg("-iwad");
    add_arg(iwad);
    if ( launch->addon[0] != '\0' )
    {
        add_arg("-file");
        add_arg(addon);
    }
    add_arg("-warp");
    if ( episode_warp(launch) )
    {
        add_arg(ep);
    }
    add_arg(map);
    add_arg("-skill");
    add_arg("3");
    add_arg("-config");
    add_arg("/sdcard/doom/choco.cfg");
    myargc = s_argc;
    myargv = s_args;
}

static void engine_task(void *arg)
{
    (void)arg;
    s_alive = 1;
    if ( setjmp(s_jmp) == 0 )
    {
        doom_port_arm_quit(s_jmp);
        M_SetExeDir();
        if ( s_launch.engine == DOOM_ENGINE_HERETIC )
        {
            heretic_D_DoomMain();
        }
        else if ( s_launch.engine == DOOM_ENGINE_HEXEN )
        {
            hexen_D_DoomMain();
        }
        else
        {
            D_DoomMain();
        }
    }
    s_running = false;
    s_alive = 0;
    s_task = NULL;
    vTaskDelete(NULL);
}

static void stop_task(void)
{
    int spins = 0;

    if ( !s_alive )
    {
        return;
    }
    doom_port_request_quit();
    while ( s_alive && (spins < 25) )
    {
        vTaskDelay(pdMS_TO_TICKS(20));
        spins++;
    }
}

void doom_engine_launch(const doom_launch_t *launch)
{
    BaseType_t ok = pdFAIL;

    if ( launch == NULL )
    {
        return;
    }
    stop_task();
    s_launch = *launch;
    s_paused = false;
    s_strafe = false;
    s_running = true;
    doom_port_pause(false);
    build_args(launch);
    if ( s_argc < 3 )
    {
        KLOG_E(TAG, "launch arguments were not built");
        s_running = false;
        return;
    }
    KLOG_I(TAG, "start %s iwad %s", launch->title, s_args[2]);
    ok = xTaskCreatePinnedToCore(engine_task, "doom_eng", ENGINE_STACK, NULL,
                                 (UBaseType_t)KODE_PRIO_USER_GUI, &s_task, 0);
    if ( ok != pdPASS )
    {
        KLOG_E(TAG, "engine task did not start");
        s_running = false;
        s_task = NULL;
    }
}

void doom_engine_stop(void)
{
    doom_port_request_quit();
    s_running = false;
    s_paused = false;
}

bool doom_engine_running(void)
{
    return s_running;
}

void doom_engine_pause(bool paused)
{
    s_paused = paused;
    doom_port_pause(paused);
}

bool doom_engine_paused(void)
{
    return s_paused;
}

void doom_engine_weapon(int slot)
{
    int key = 0;

    switch ( slot )
    {
    case 1: key = key_weapon1; break;
    case 2: key = key_weapon2; break;
    case 3: key = key_weapon3; break;
    case 4: key = key_weapon4; break;
    case 5: key = key_weapon5; break;
    case 6: key = key_weapon6; break;
    case 7: key = key_weapon7; break;
    case 8: key = key_weapon8; break;
    default: return;
    }
    doom_port_pulse(key);
}

void doom_engine_strafe(bool on)
{
    s_strafe = on;
}

bool doom_engine_strafe_on(void)
{
    return s_strafe;
}

void doom_engine_jump(void)
{
    doom_port_pulse(key_jump);
}

void doom_engine_inventory(int dir)
{
    if ( dir < 0 )
    {
        doom_port_pulse(key_invleft);
    }
    else if ( dir > 0 )
    {
        doom_port_pulse(key_invright);
    }
    else
    {
        doom_port_pulse(key_useartifact);
    }
}

void doom_engine_always_run(bool on)
{
    s_run = on;
}

const char *doom_engine_title(void)
{
    if ( !s_running )
    {
        return "";
    }
    return s_launch.title;
}

doom_engine_t doom_engine_id(void)
{
    if ( !s_running )
    {
        return DOOM_ENGINE_NONE;
    }
    return s_launch.engine;
}

void doom_engine_draw(uint16_t *view, const doom_controls_t *in)
{
    doom_port_pad_t pad;
    uint32_t i = 0U;

    if ( (view == NULL) || !s_running )
    {
        return;
    }
    memset(&pad, 0, sizeof(pad));
    if ( (in != NULL) && !s_paused )
    {
        pad.up = in->up;
        pad.down = in->down;
        pad.left = in->left;
        pad.right = in->right;
        pad.fire = in->fire;
        if ( in->use_edge )
        {
            doom_port_pulse(key_use);
        }
    }
    pad.strafe = s_strafe;
    pad.run = s_run;
    doom_port_set_pad(&pad);
    if ( doom_port_copy_frame(view) )
    {
        return;
    }
    for ( i = 0U; i < (320U * 200U); i++ )
    {
        view[i] = (i / 320U) < 100U ? 0x1082U : 0x0841U;
    }
}
