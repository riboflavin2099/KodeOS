#include "config.h"

#include "doomkeys.h"
#include "doomtype.h"
#include "d_event.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_controls.h"

#include "doom_port.h"

#include <stdlib.h>
#include <string.h>

#if !defined(DOOM_HOST)
#include "esp_heap_caps.h"
#endif

extern int gametic;

#define VIEW_W 320
#define VIEW_H 200
#define VIEW_N (VIEW_W * VIEW_H)

char *video_driver = "";
boolean screenvisible = true;
int vanilla_keyboard_mapping = 1;
boolean screensaver_mode = false;
int usegamma = 0;
pixel_t *I_VideoBuffer = NULL;
int screen_width = VIEW_W;
int screen_height = VIEW_H;
int fullscreen = 1;
int aspect_ratio_correct = 0;
int integer_scaling = 0;
int smooth_pixel_scaling = 0;
int vga_porch_flash = 0;
int force_software_renderer = 1;
int png_screenshots = 0;
char *window_position = "";
unsigned int joywait = 0;
int usemouse = 0;

static byte s_pal[256 * 3];
static uint16_t s_lut[256];
static uint16_t *s_rgb;
static int s_last_ms;
static int s_have_ms;

static bool s_prev_up;
static bool s_prev_down;
static bool s_prev_left;
static bool s_prev_right;
static bool s_prev_sleft;
static bool s_prev_sright;
static bool s_prev_fire;
static bool s_prev_strafe;
static bool s_prev_run;
static bool s_was_paused;

static void *psram_or_malloc(size_t bytes)
{
#if defined(DOOM_HOST)
    return malloc(bytes);
#else
    return heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#endif
}

static uint16_t pack565(byte r, byte g, byte b)
{
    return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

static void rebuild_lut(void)
{
    int i;

    for ( i = 0; i < 256; i++ )
    {
        s_lut[i] = pack565(s_pal[i * 3], s_pal[i * 3 + 1], s_pal[i * 3 + 2]);
    }
}

static void post_key(int key, int down)
{
    event_t ev;

    ev.type = down ? ev_keydown : ev_keyup;
    ev.data1 = key;
    ev.data2 = 0;
    ev.data3 = 0;
    D_PostEvent(&ev);
}

static void edge(bool now, bool *prev, int key)
{
    if ( now != *prev )
    {
        post_key(key, now ? 1 : 0);
        *prev = now;
    }
}

static void release_held(void)
{
    if ( s_prev_up )
    {
        post_key(key_up, 0);
        s_prev_up = false;
    }
    if ( s_prev_down )
    {
        post_key(key_down, 0);
        s_prev_down = false;
    }
    if ( s_prev_left )
    {
        post_key(key_left, 0);
        s_prev_left = false;
    }
    if ( s_prev_right )
    {
        post_key(key_right, 0);
        s_prev_right = false;
    }
    if ( s_prev_sleft )
    {
        post_key(key_strafeleft, 0);
        s_prev_sleft = false;
    }
    if ( s_prev_sright )
    {
        post_key(key_straferight, 0);
        s_prev_sright = false;
    }
    if ( s_prev_fire )
    {
        post_key(key_fire, 0);
        s_prev_fire = false;
    }
    if ( s_prev_strafe )
    {
        post_key(key_strafe, 0);
        s_prev_strafe = false;
    }
    if ( s_prev_run )
    {
        post_key(key_speed, 0);
        s_prev_run = false;
    }
}

void I_InitGraphics(void)
{
    if ( I_VideoBuffer == NULL )
    {
        I_VideoBuffer = psram_or_malloc(VIEW_N);
    }
    if ( s_rgb == NULL )
    {
        s_rgb = psram_or_malloc(VIEW_N * sizeof(uint16_t));
    }
    if ( (I_VideoBuffer == NULL) || (s_rgb == NULL) )
    {
        I_Error("I_InitGraphics: no memory for the view");
    }
    memset(I_VideoBuffer, 0, VIEW_N);
    screenvisible = true;
}

void I_ShutdownGraphics(void)
{
}

void I_GraphicsCheckCommandLine(void)
{
}

void I_SetPalette(byte *palette)
{
    memcpy(s_pal, palette, sizeof s_pal);
    rebuild_lut();
}

int I_GetPaletteIndex(int r, int g, int b)
{
    int best = 0;
    int best_d = 1 << 30;
    int i;

    for ( i = 0; i < 256; i++ )
    {
        int dr = r - s_pal[i * 3];
        int dg = g - s_pal[i * 3 + 1];
        int db = b - s_pal[i * 3 + 2];
        int d = dr * dr + dg * dg + db * db;

        if ( d < best_d )
        {
            best_d = d;
            best = i;
        }
    }
    return best;
}

void I_UpdateNoBlit(void)
{
}

void I_FinishUpdate(void)
{
    int now = I_GetTimeMS();
    int slept = doom_port_take_slept_ms();
    int i;
    double wall = 0.0;
    double busy = 0.0;

    if ( (I_VideoBuffer == NULL) || (s_rgb == NULL) )
    {
        return;
    }
    for ( i = 0; i < VIEW_N; i++ )
    {
        s_rgb[i] = s_lut[I_VideoBuffer[i]];
    }
    if ( s_have_ms )
    {
        wall = (double)(now - s_last_ms);
        busy = wall - (double)slept;
        if ( busy < 0.0 )
        {
            busy = 0.0;
        }
    }
    s_last_ms = now;
    s_have_ms = 1;
    doom_port_publish(s_rgb, wall, busy);
    if ( (doom_port_max_tics() > 0) && (gametic >= doom_port_max_tics()) )
    {
        doom_port_request_quit();
    }
}

void I_ReadScreen(pixel_t *scr)
{
    if ( (scr != NULL) && (I_VideoBuffer != NULL) )
    {
        memcpy(scr, I_VideoBuffer, VIEW_N);
    }
}

void I_BeginRead(void)
{
}

void I_SetWindowTitle(const char *title)
{
    (void)title;
}

void I_CheckIsScreensaver(void)
{
}

void I_SetGrabMouseCallback(grabmouse_callback_t func)
{
    (void)func;
}

void I_DisplayFPSDots(boolean dots_on)
{
    (void)dots_on;
}

void I_BindVideoVariables(void)
{
}

void I_InitWindowTitle(void)
{
}

void I_RegisterWindowIcon(const unsigned int *icon, int width, int height)
{
    (void)icon;
    (void)width;
    (void)height;
}

void I_InitWindowIcon(void)
{
}

void I_StartFrame(void)
{
    while ( doom_port_paused() && !doom_port_quit_requested()
            && !doom_port_pulse_pending() )
    {
        I_Sleep(10);
    }
    if ( doom_port_quit_requested()
         || ((doom_port_max_tics() > 0) && (gametic >= doom_port_max_tics())) )
    {
        I_Quit();
    }
}

void I_StartTic(void)
{
    int key = 0;
    int down = 0;
    int paused = doom_port_paused();
    doom_port_pad_t pad;

    if ( paused && !s_was_paused )
    {
        release_held();
    }
    s_was_paused = paused;
    while ( doom_port_next_pulse(&key, &down) )
    {
        post_key(key, down);
    }
    if ( paused )
    {
        return;
    }
    doom_port_read_pad(&pad);
    edge(pad.up, &s_prev_up, key_up);
    edge(pad.down, &s_prev_down, key_down);
    edge(pad.fire, &s_prev_fire, key_fire);
    edge(pad.run, &s_prev_run, key_speed);
    edge(pad.strafe, &s_prev_strafe, key_strafe);
    if ( pad.strafe )
    {
        edge(false, &s_prev_left, key_left);
        edge(false, &s_prev_right, key_right);
        edge(pad.left, &s_prev_sleft, key_strafeleft);
        edge(pad.right, &s_prev_sright, key_straferight);
    }
    else
    {
        edge(false, &s_prev_sleft, key_strafeleft);
        edge(false, &s_prev_sright, key_straferight);
        edge(pad.left, &s_prev_left, key_left);
        edge(pad.right, &s_prev_right, key_right);
    }
}

void I_EnableLoadingDisk(int xoffs, int yoffs)
{
    (void)xoffs;
    (void)yoffs;
}

void I_GetWindowPosition(int *x, int *y, int w, int h)
{
    (void)w;
    (void)h;
    if ( x != NULL )
    {
        *x = 0;
    }
    if ( y != NULL )
    {
        *y = 0;
    }
}
