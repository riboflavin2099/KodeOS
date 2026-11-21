#if defined(DOOM_HOST)
#define _POSIX_C_SOURCE 200809L
#endif

#include "config.h"

#include "doomtype.h"
#include "i_timer.h"

#include "doom_port.h"

#include <stdint.h>

#if defined(DOOM_HOST)
#include <time.h>
#else
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

static uint64_t s_base;
static int s_have_base;

static uint64_t now_us(void)
{
#if defined(DOOM_HOST)
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ull + (uint64_t)ts.tv_nsec / 1000ull;
#else
    return (uint64_t)esp_timer_get_time();
#endif
}

static uint64_t elapsed_us(void)
{
    uint64_t now = now_us();

    if ( !s_have_base )
    {
        s_base = now;
        s_have_base = 1;
    }
    return now - s_base;
}

int I_GetTime(void)
{
    return (int)(elapsed_us() * 35ull / 1000000ull);
}

int I_GetTimeMS(void)
{
    return (int)(elapsed_us() / 1000ull);
}

void I_Sleep(int ms)
{
    if ( ms <= 0 )
    {
        return;
    }
    doom_port_note_sleep_ms(ms);
#if defined(DOOM_HOST)
    {
        struct timespec ts;

        ts.tv_sec = ms / 1000;
        ts.tv_nsec = (long)(ms % 1000) * 1000000L;
        nanosleep(&ts, NULL);
    }
#else
    vTaskDelay(pdMS_TO_TICKS(ms));
#endif
}

void I_WaitVBL(int count)
{
    I_Sleep((count * 1000) / 35);
}

void I_InitTimer(void)
{
}
