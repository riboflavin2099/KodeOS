/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_core.c
 * @brief Walnut-CGB front end: ROM, cart RAM, joypad, RGB565 lines, APU.
 *
 * The vendored header is the whole core. It is included once, in this file.
 */
#include "gbc_core.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "gbc_display.h"
#include "gbc_mix.h"

#ifdef GBC_HOST

#include <stdio.h>

#define GBC_LOG(fmt, ...) fprintf(stderr, "gbc.core: " fmt "\n", ##__VA_ARGS__)

static bool gbc_ptr_external(const void *p)
{
    (void)p;
    return false;
}

static uint32_t gbc_now_us(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((ts.tv_sec * 1000000L) + (ts.tv_nsec / 1000L));
}

void gbc_audio_write(const int16_t *frames, size_t n);

#else

#include "esp_heap_caps.h"
#include "esp_memory_utils.h"
#include "esp_timer.h"
#include "gbc_audio.h"
#include "gbc_rom.h"
#include "kode_log.h"

#define TAG "gbc.core"

#define GBC_LOG(...) KLOG_W(TAG, __VA_ARGS__)

static bool gbc_ptr_external(const void *p)
{
    return esp_ptr_external_ram(p);
}

static uint32_t gbc_now_us(void)
{
    return (uint32_t)esp_timer_get_time();
}

static void *gbc_alloc(size_t bytes)
{
    return heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

static void gbc_free(void *p)
{
    heap_caps_free(p);
}

#endif

#ifdef GBC_HOST
static void *gbc_alloc(size_t bytes)
{
    return malloc(bytes);
}

static void gbc_free(void *p)
{
    free(p);
}
#endif

/* minigb_apu before Walnut. MINIGB_APU_AUDIO_FORMAT_S16SYS comes from the build. */
#include "minigb_apu.h"

uint8_t audio_read(const uint16_t addr);
void audio_write(const uint16_t addr, const uint8_t val);

/** @brief State the ROM callbacks and the line drawer share. */
typedef struct {
    const uint8_t *rom;   /**< Cartridge image. */
    uint32_t       rom_len;
    uint8_t       *ram;   /**< Cart RAM. May be NULL. */
    uint32_t       ram_len;
    uint16_t      *lcd;   /**< The frame being drawn. */
    struct gb_s   *gb;
} gbc_priv_t;

static struct gb_s          *s_gb = NULL;
static struct minigb_apu_ctx *s_apu = NULL;
static gbc_priv_t            s_priv;
static int16_t              *s_stereo = NULL;
static int16_t              *s_mono = NULL;
static int64_t               s_unix_sec = -1;
static bool                  s_ready = false;
static bool                  s_failed = false;
static bool                  s_dirty = false;
static uint32_t              s_frames = 0U;
static uint32_t              s_last_us = 0U;
static uint32_t              s_worst_us = 0U;
static uint64_t              s_sum_us = 0U;
static uint32_t              s_audio_frames = 0U;
static uint32_t              s_phase = 0U;
static const uint8_t        *s_rom = NULL;

/** @brief DMG shades, light to dark, as RGB565. */
static const uint16_t s_shade[4] = {
    0xFFFFU, 0xAD55U, 0x52AAU, 0x0000U
};

/** @brief Idle fill. */
#define GBC_COL_IDLE   0x0320U

/** @brief Idle border. */
#define GBC_COL_WHITE  0xFFFFU

/** @brief Shown when gb_init refuses the image. */
#define GBC_COL_FAIL   0x5000U

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#define ENABLE_SOUND 1
#define ENABLE_LCD 1
#include "walnut_cgb.h"
#pragma GCC diagnostic pop

/**
 * @brief Read one ROM byte. Banking is Walnut's job; addr is into the image.
 */
static uint8_t rom_read(struct gb_s *gb, const uint_fast32_t addr)
{
    gbc_priv_t *const priv = (gbc_priv_t *)gb->direct.priv;
    uint8_t byte = 0xFFU;

    if ( (NULL != priv) && (addr < priv->rom_len) )
    {
        byte = priv->rom[addr];
    }
    return byte;
}

/**
 * @brief Read two little-endian ROM bytes.
 */
static uint16_t rom_read16(struct gb_s *gb, const uint_fast32_t addr)
{
    uint16_t const lo = (uint16_t)rom_read(gb, addr);
    uint16_t const hi = (uint16_t)rom_read(gb, addr + 1U);

    return (uint16_t)(lo | (uint16_t)(hi << 8));
}

/**
 * @brief Read four little-endian ROM bytes.
 */
static uint32_t rom_read32(struct gb_s *gb, const uint_fast32_t addr)
{
    uint32_t value = 0U;
    uint32_t i = 0U;

    for ( i = 0U; 4U > i; i++ )
    {
        value |= (uint32_t)rom_read(gb, addr + i) << (8U * i);
    }
    return value;
}

/**
 * @brief Read cart RAM. Addresses past the buffer read as 0xFF.
 */
static uint8_t ram_read(struct gb_s *gb, const uint_fast32_t addr)
{
    gbc_priv_t *const priv = (gbc_priv_t *)gb->direct.priv;
    uint8_t byte = 0xFFU;

    if ( (NULL != priv) && (NULL != priv->ram) && (addr < priv->ram_len) )
    {
        byte = priv->ram[addr];
    }
    return byte;
}

/**
 * @brief Write cart RAM and remember that a save is due.
 */
static void ram_write(struct gb_s *gb, const uint_fast32_t addr, const uint8_t val)
{
    gbc_priv_t *const priv = (gbc_priv_t *)gb->direct.priv;

    if ( (NULL != priv) && (NULL != priv->ram) && (addr < priv->ram_len) )
    {
        priv->ram[addr] = val;
        s_dirty = true;
    }
}

/**
 * @brief Walnut calls this on a bad opcode. It must not return.
 */
static void on_error(struct gb_s *gb, const enum gb_error_e err, const uint16_t addr)
{
    (void)gb;
    GBC_LOG("guest error %u at 0x%04x", (unsigned)err, (unsigned)addr);
    abort();
}

/**
 * @brief APU register read. Walnut calls this by name when sound is on.
 */
uint8_t audio_read(const uint16_t addr)
{
    uint8_t byte = 0xFFU;

    if ( NULL != s_apu )
    {
        byte = minigb_apu_audio_read(s_apu, addr);
    }
    return byte;
}

/**
 * @brief APU register write.
 */
void audio_write(const uint16_t addr, const uint8_t val)
{
    if ( NULL != s_apu )
    {
        minigb_apu_audio_write(s_apu, addr, val);
    }
}

/**
 * @brief Expand one scanline into the RGB565 frame.
 */
static void on_line(struct gb_s *gb, const uint8_t *pixels, const uint_fast8_t line)
{
    gbc_priv_t *const priv = (gbc_priv_t *)gb->direct.priv;
    uint16_t *dst = NULL;
    uint32_t x = 0U;

    if ( (NULL == priv) || (NULL == priv->lcd) || (GBC_LCD_H <= line) )
    {
        return;
    }
    dst = &priv->lcd[(uint32_t)line * GBC_LCD_W];
    if ( 0U != gb->cgb.cgbMode )
    {
        for ( x = 0U; GBC_LCD_W > x; x++ )
        {
            uint8_t idx = pixels[x];

            if ( 0x40U <= idx )
            {
                idx = 0U;
            }
            dst[x] = gb->cgb.fixPalette[idx];
        }
    }
    else
    {
        for ( x = 0U; GBC_LCD_W > x; x++ )
        {
            dst[x] = s_shade[pixels[x] & 0x03U];
        }
    }
}

/**
 * @brief Joypad byte. A set bit is released, matching Walnut's OR into P1.
 */
static uint8_t joy_of(const gbc_pad_t *pad)
{
    uint8_t joy = 0xFFU;

    if ( NULL != pad )
    {
        if ( pad->a )
        {
            joy = (uint8_t)(joy & (uint8_t)~JOYPAD_A);
        }
        if ( pad->b )
        {
            joy = (uint8_t)(joy & (uint8_t)~JOYPAD_B);
        }
        if ( pad->select )
        {
            joy = (uint8_t)(joy & (uint8_t)~JOYPAD_SELECT);
        }
        if ( pad->start )
        {
            joy = (uint8_t)(joy & (uint8_t)~JOYPAD_START);
        }
        if ( pad->right )
        {
            joy = (uint8_t)(joy & (uint8_t)~JOYPAD_RIGHT);
        }
        if ( pad->left )
        {
            joy = (uint8_t)(joy & (uint8_t)~JOYPAD_LEFT);
        }
        if ( pad->up )
        {
            joy = (uint8_t)(joy & (uint8_t)~JOYPAD_UP);
        }
        if ( pad->down )
        {
            joy = (uint8_t)(joy & (uint8_t)~JOYPAD_DOWN);
        }
    }
    return joy;
}

/**
 * @brief Fill the guest frame and draw a moving border.
 */
static void fill_idle(uint16_t *lcd, uint16_t fill, uint16_t edge)
{
    uint32_t i = 0U;
    uint32_t x = 0U;
    uint32_t y = 0U;
    uint32_t const top = s_phase % GBC_LCD_H;

    for ( i = 0U; (GBC_LCD_W * GBC_LCD_H) > i; i++ )
    {
        lcd[i] = fill;
    }
    for ( x = 0U; GBC_LCD_W > x; x++ )
    {
        lcd[(top * GBC_LCD_W) + x] = edge;
        lcd[x] = edge;
        lcd[((GBC_LCD_H - 1U) * GBC_LCD_W) + x] = edge;
    }
    for ( y = 0U; GBC_LCD_H > y; y++ )
    {
        lcd[y * GBC_LCD_W] = edge;
        lcd[(y * GBC_LCD_W) + (GBC_LCD_W - 1U)] = edge;
    }
}

/**
 * @brief Release a previous context.
 */
static void close_core(void)
{
    gbc_free(s_gb);
    gbc_free(s_apu);
    gbc_free(s_stereo);
    gbc_free(s_mono);
    s_gb = NULL;
    s_apu = NULL;
    s_stereo = NULL;
    s_mono = NULL;
    s_ready = false;
    s_rom = NULL;
    memset(&s_priv, 0, sizeof(s_priv));
}

void gbc_core_set_clock(int64_t unix_sec)
{
    s_unix_sec = unix_sec;
}

bool gbc_core_open(const uint8_t *rom, uint32_t rom_len,
                   uint8_t *ram, uint32_t ram_len)
{
    enum gb_init_error_e err = GB_INIT_NO_ERROR;
    bool ok = false;

    close_core();
    s_failed = false;
    s_dirty = false;
    s_frames = 0U;
    s_last_us = 0U;
    s_worst_us = 0U;
    s_sum_us = 0U;
    s_audio_frames = 0U;
    gbc_mix_reset();
    if ( (NULL == rom) || (0U == rom_len) )
    {
        return false;
    }
    s_gb = (struct gb_s *)gbc_alloc(sizeof(struct gb_s));
    s_apu = (struct minigb_apu_ctx *)gbc_alloc(sizeof(struct minigb_apu_ctx));
    s_stereo = (int16_t *)gbc_alloc((size_t)AUDIO_SAMPLES_TOTAL * sizeof(int16_t));
    s_mono = (int16_t *)gbc_alloc(2048U * sizeof(int16_t));
    if ( (NULL == s_gb) || (NULL == s_apu) || (NULL == s_stereo) || (NULL == s_mono) )
    {
        GBC_LOG("no memory for the core (%u byte context)", (unsigned)sizeof(struct gb_s));
        close_core();
        s_failed = true;
        return false;
    }
    memset(s_gb, 0, sizeof(*s_gb));
    minigb_apu_audio_init(s_apu);
    s_priv.rom = rom;
    s_priv.rom_len = rom_len;
    s_priv.ram = ram;
    s_priv.ram_len = ram_len;
    s_priv.lcd = NULL;
    s_priv.gb = s_gb;
    err = gb_init(s_gb, &rom_read, &rom_read16, &rom_read32,
                  &ram_read, &ram_write, &on_error, &s_priv);
    if ( GB_INIT_NO_ERROR != err )
    {
        GBC_LOG("gb_init refused the image (%u)", (unsigned)err);
        close_core();
        s_failed = true;
        return false;
    }
    gb_init_lcd(s_gb, &on_line);
    if ( 0 <= s_unix_sec )
    {
        time_t const sec = (time_t)s_unix_sec;
        struct tm broken;

        if ( NULL != gmtime_r(&sec, &broken) )
        {
            gb_set_rtc(s_gb, &broken);
        }
    }
    s_rom = rom;
    s_ready = true;
    ok = true;
    {
        unsigned external = 0U;

        if ( gbc_ptr_external(s_gb) )
        {
            external = 1U;
        }
        GBC_LOG("core open, context %u bytes, external %u",
                (unsigned)sizeof(struct gb_s), external);
    }
    return ok;
}

/**
 * @brief One guest frame and its audio.
 */
static void run_one(const gbc_pad_t *pad, uint16_t *lcd)
{
    uint32_t const t0 = gbc_now_us();
    uint32_t dt = 0U;
    uint32_t mixed = 0U;

    s_priv.lcd = lcd;
    s_gb->direct.joypad = joy_of(pad);
    gb_run_frame_dualfetch(s_gb);
    minigb_apu_audio_callback(s_apu, s_stereo);
    mixed = gbc_mix_push(s_stereo, (uint32_t)AUDIO_SAMPLES, s_mono, 2048U);
    if ( 0U < mixed )
    {
        gbc_audio_write(s_mono, (size_t)mixed);
        s_audio_frames += mixed;
    }
    dt = gbc_now_us() - t0;
    s_last_us = dt;
    s_sum_us += (uint64_t)dt;
    if ( dt > s_worst_us )
    {
        s_worst_us = dt;
    }
    s_frames++;
}

void gbc_core_draw(uint16_t *lcd, const gbc_pad_t *pad,
                   const uint8_t *rom, uint32_t rom_len)
{
    if ( NULL == lcd )
    {
        return;
    }
    s_phase++;
    if ( (NULL == rom) || (0U == rom_len) )
    {
        fill_idle(lcd, GBC_COL_IDLE, GBC_COL_WHITE);
        return;
    }
    if ( s_failed )
    {
        fill_idle(lcd, GBC_COL_FAIL, GBC_COL_WHITE);
        return;
    }
    if ( !s_ready || (s_rom != rom) )
    {
        uint8_t *ram = NULL;
        uint32_t ram_len = 0U;

#ifndef GBC_HOST
        ram = gbc_rom_ram(&ram_len);
#endif
        if ( !gbc_core_open(rom, rom_len, ram, ram_len) )
        {
            fill_idle(lcd, GBC_COL_FAIL, GBC_COL_WHITE);
            return;
        }
    }
    run_one(pad, lcd);
}

void gbc_core_stats(gbc_core_stats_t *out)
{
    if ( NULL == out )
    {
        return;
    }
    out->frames = s_frames;
    out->last_us = s_last_us;
    out->worst_us = s_worst_us;
    out->avg_us = 0U;
    if ( 0U < s_frames )
    {
        out->avg_us = (uint32_t)(s_sum_us / (uint64_t)s_frames);
    }
    out->audio_frames = s_audio_frames;
    out->running = s_ready;
    out->gb_external = gbc_ptr_external(s_gb);
    out->rom_external = gbc_ptr_external(s_rom);
    out->lcd_external = gbc_ptr_external(s_priv.lcd);
    out->ram_external = gbc_ptr_external(s_priv.ram);
}

bool gbc_core_save_pending(void)
{
    bool pending = false;

    if ( s_ready && (s_dirty || (0 <= s_unix_sec)) )
    {
        pending = true;
    }
    return pending;
}

void gbc_core_save_clear(void)
{
    s_dirty = false;
}

uint8_t *gbc_core_cart_ram(uint32_t *len)
{
    uint8_t *ram = NULL;

    if ( s_ready )
    {
        ram = s_priv.ram;
        if ( NULL != len )
        {
            *len = s_priv.ram_len;
        }
    }
    else if ( NULL != len )
    {
        *len = 0U;
    }
    return ram;
}

int64_t gbc_core_clock(void)
{
    int64_t clock = -1;

    if ( 0 <= s_unix_sec )
    {
        clock = s_unix_sec + (int64_t)(s_frames / 60U);
    }
    return clock;
}
