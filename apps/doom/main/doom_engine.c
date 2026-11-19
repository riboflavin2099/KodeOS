/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file doom_engine.c
 * @brief Stand-in view. Chocolate Doom is pinned and not linked.
 *
 * The header to bring in, when that changes, is the Chocolate tree at the
 * commit in third_party/chocolate-doom/UPSTREAM.md. Doom and Doom II share
 * one tree. Heretic and Hexen are two more, and their global names collide,
 * so the link prefixes them. Legacy of Rust (id1.wad) needs MBF21/ID24,
 * which this base does not have: launching it stores the name and the
 * stand-in says so.
 */
#include "doom_engine.h"

#include <string.h>

#include "kode.h"

#define DOOM_PIX  (320U * 200U)

static doom_launch_t s_launch;
static bool s_running = false;
static bool s_paused = false;
static int s_weapon = 2;
static bool s_strafe = false;
static int s_inv = 0;
static uint32_t s_jump = 0U;
static uint32_t s_phase = 0U;

void doom_engine_launch(const doom_launch_t *launch)
{
    if ( NULL == launch )
    {
        return;
    }
    s_launch = *launch;
    s_running = true;
    s_paused = false;
    s_weapon = 2;
    s_strafe = false;
    s_inv = 0;
    s_jump = 0U;
    s_phase = 0U;
}

void doom_engine_stop(void)
{
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
}

bool doom_engine_paused(void)
{
    return s_paused;
}

void doom_engine_weapon(int slot)
{
    if ( 0 < slot )
    {
        s_weapon = slot;
    }
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
    s_jump = 8U;
}

void doom_engine_inventory(int dir)
{
    if ( 0 == dir )
    {
        s_inv += 0;
    }
    else
    {
        s_inv += dir;
    }
}

doom_engine_t doom_engine_id(void)
{
    doom_engine_t id = DOOM_ENGINE_NONE;

    if ( s_running )
    {
        id = s_launch.engine;
    }
    return id;
}

const char *doom_engine_title(void)
{
    if ( !s_running )
    {
        return "";
    }
    return s_launch.title;
}

static uint16_t body_color(void)
{
    uint16_t rgb = 0x0841U;

    if ( DOOM_ENGINE_DOOM2 == s_launch.engine )
    {
        rgb = 0x4000U;
    }
    else if ( DOOM_ENGINE_HERETIC == s_launch.engine )
    {
        rgb = 0x02C0U;
    }
    else if ( DOOM_ENGINE_HEXEN == s_launch.engine )
    {
        rgb = 0x0014U;
    }
    else if ( DOOM_ENGINE_DOOM == s_launch.engine )
    {
        rgb = 0x6000U;
    }
    return rgb;
}

void doom_engine_draw(uint16_t *view, const doom_controls_t *in)
{
    uint32_t i = 0U;
    uint32_t horizon = 100U;
    uint16_t sky = 0x1082U;
    uint16_t ground = body_color();
    bool fire = false;
    bool left = false;
    bool right = false;

    if ( (NULL == view) || !s_running )
    {
        return;
    }
    if ( !s_paused )
    {
        s_phase++;
        if ( 0U < s_jump )
        {
            s_jump--;
        }
    }
    if ( NULL != in )
    {
        fire = in->fire;
        left = in->left;
        right = in->right;
        if ( in->up && !s_paused )
        {
            horizon = 90U;
        }
        if ( in->down && !s_paused )
        {
            horizon = 112U;
        }
    }
    if ( 0U < s_jump )
    {
        horizon = 80U;
    }
    for ( i = 0U; DOOM_PIX > i; i++ )
    {
        uint32_t const y = i / 320U;

        view[i] = (y < horizon) ? sky : ground;
    }
    if ( fire )
    {
        view[(100U * 320U) + 160U] = 0xFFE0U;
    }
    if ( 0 != s_weapon )
    {
        view[(8U * 320U) + (uint32_t)(s_weapon & 15)] = 0xFFFFU;
    }
    if ( 0 != s_inv )
    {
        view[(12U * 320U) + 8U] = 0x07E0U;
    }
    if ( left || right )
    {
        uint32_t const x = s_strafe ? 40U : (uint32_t)(40U + (s_phase % 240U));
        uint32_t y = 0U;

        for ( y = 40U; 160U > y; y++ )
        {
            view[(y * 320U) + x] = 0xFFFFU;
        }
    }
    if ( '\0' != s_launch.addon[0] )
    {
        view[0] = 0xF800U;
    }
}
