#include "doom_port.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(DOOM_HOST)
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
#define PORT_LOCK()   portENTER_CRITICAL(&s_mux)
#define PORT_UNLOCK() portEXIT_CRITICAL(&s_mux)
#else
#define PORT_LOCK()
#define PORT_UNLOCK()
#endif

#define FRAME_PIX  (320 * 200)
#define PULSE_MAX  16

static jmp_buf s_quit;
static int s_have_quit;
static int s_max_tics;
static atomic_int s_quit_req;
static atomic_int s_paused;
static doom_port_pad_t s_pad;

static int s_pulse[PULSE_MAX];
static int s_pulse_n;
static int s_pulse_r;
static int s_phase;
static int s_cur_key;

static atomic_int s_slept_ms;
static uint16_t *s_frame;
static atomic_uint s_seq;
static int s_frames;
static double s_wall_sum;
static double s_wall_max;
static double s_busy_sum;
static double s_busy_max;
static int s_timed;

static void *psram_or_malloc(size_t bytes)
{
#if defined(DOOM_HOST)
    return malloc(bytes);
#else
    return heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#endif
}

void doom_port_arm_quit(jmp_buf buf)
{
    memcpy(s_quit, buf, sizeof(jmp_buf));
    s_have_quit = 1;
    atomic_store(&s_quit_req, 0);
}

void doom_port_set_max_tics(int n)
{
    s_max_tics = n;
}

int doom_port_max_tics(void)
{
    return s_max_tics;
}

void doom_port_request_quit(void)
{
    atomic_store(&s_quit_req, 1);
    atomic_store(&s_paused, 0);
}

int doom_port_quit_requested(void)
{
    return atomic_load(&s_quit_req);
}

void doom_port_pause(bool paused)
{
    atomic_store(&s_paused, paused ? 1 : 0);
}

int doom_port_paused(void)
{
    return atomic_load(&s_paused);
}

void doom_port_set_pad(const doom_port_pad_t *pad)
{
    if ( pad == NULL )
    {
        return;
    }
    PORT_LOCK();
    s_pad = *pad;
    PORT_UNLOCK();
}

void doom_port_read_pad(doom_port_pad_t *pad)
{
    PORT_LOCK();
    *pad = s_pad;
    PORT_UNLOCK();
}

void doom_port_pulse(int key)
{
    PORT_LOCK();
    if ( s_pulse_n < PULSE_MAX )
    {
        int const at = (s_pulse_r + s_pulse_n) % PULSE_MAX;

        s_pulse[at] = key;
        s_pulse_n++;
    }
    PORT_UNLOCK();
}

int doom_port_pulse_pending(void)
{
    int pending = 0;

    PORT_LOCK();
    pending = (s_pulse_n > 0) || (s_phase != 0);
    PORT_UNLOCK();
    return pending;
}

int doom_port_next_pulse(int *key, int *down)
{
    int have = 0;

    PORT_LOCK();
    if ( s_phase == 2 )
    {
        *key = s_cur_key;
        *down = 0;
        s_phase = 0;
        have = 1;
    }
    else if ( (s_phase == 0) && (s_pulse_n > 0) )
    {
        s_cur_key = s_pulse[s_pulse_r];
        s_pulse_r = (s_pulse_r + 1) % PULSE_MAX;
        s_pulse_n--;
        s_phase = 2;
        *key = s_cur_key;
        *down = 1;
        have = 1;
    }
    PORT_UNLOCK();
    return have;
}

void doom_port_note_sleep_ms(int ms)
{
    if ( ms > 0 )
    {
        atomic_fetch_add(&s_slept_ms, ms);
    }
}

int doom_port_take_slept_ms(void)
{
    return atomic_exchange(&s_slept_ms, 0);
}

void doom_port_publish(const uint16_t *rgb, double wall_ms, double busy_ms)
{
    if ( s_frame == NULL )
    {
        s_frame = psram_or_malloc(FRAME_PIX * sizeof(uint16_t));
    }
    if ( (s_frame == NULL) || (rgb == NULL) )
    {
        return;
    }
    atomic_fetch_add(&s_seq, 1);
    memcpy(s_frame, rgb, FRAME_PIX * sizeof(uint16_t));
    atomic_fetch_add(&s_seq, 1);
    if ( s_timed )
    {
        s_frames++;
        s_wall_sum += wall_ms;
        s_busy_sum += busy_ms;
        if ( wall_ms > s_wall_max )
        {
            s_wall_max = wall_ms;
        }
        if ( busy_ms > s_busy_max )
        {
            s_busy_max = busy_ms;
        }
    }
    s_timed = 1;
}

int doom_port_copy_frame(uint16_t *dst)
{
    unsigned a = 0;
    unsigned b = 0;

    if ( (dst == NULL) || (s_frame == NULL) || !s_timed )
    {
        return 0;
    }
    do
    {
        a = atomic_load(&s_seq);
        if ( (a & 1u) != 0u )
        {
            continue;
        }
        memcpy(dst, s_frame, FRAME_PIX * sizeof(uint16_t));
        b = atomic_load(&s_seq);
    } while ( a != b );
    return 1;
}

int doom_port_frames(void)
{
    return s_frames;
}

void doom_port_frame_times(double *wall_avg, double *wall_max,
                           double *busy_avg, double *busy_max)
{
    if ( wall_avg != NULL )
    {
        *wall_avg = s_frames > 0 ? s_wall_sum / (double)s_frames : 0.0;
    }
    if ( wall_max != NULL )
    {
        *wall_max = s_wall_max;
    }
    if ( busy_avg != NULL )
    {
        *busy_avg = s_frames > 0 ? s_busy_sum / (double)s_frames : 0.0;
    }
    if ( busy_max != NULL )
    {
        *busy_max = s_busy_max;
    }
}

void doom_port_leave(int code)
{
    if ( s_have_quit )
    {
        longjmp(s_quit, code == 0 ? 1 : code);
    }
    fprintf(stderr, "doom: quit %d with no jump buffer\n", code);
    abort();
}
