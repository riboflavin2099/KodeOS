/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file test_run.c
 * @brief Run Walnut on a ROM file and report host frame times.
 *
 * Default ROM is roms/scroll.gb, written for this tree (see roms/LICENSE).
 * Pass another path to time a ROM you supply. No commercial ROM is shipped.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "gbc_core.h"
#include "gbc_display.h"

/** @brief Guest frames to time. */
#define RUN_FRAMES  180U

/** @brief Frames written as PPM, so a scroll is visible. */
#define DUMP_A  30U
#define DUMP_B  90U

/** @brief Sink for resampled audio. The device has its own in gbc_audio.c. */
static uint32_t s_audio = 0U;

void gbc_audio_write(const int16_t *frames, size_t n)
{
    (void)frames;
    s_audio += (uint32_t)n;
}

/**
 * @brief Read a whole file.
 * @param path The ROM.
 * @param len  Receives the size.
 * @return The bytes, or NULL.
 */
static uint8_t *read_file(const char *path, uint32_t *len)
{
    FILE *fh = fopen(path, "rb");
    long size = 0;
    uint8_t *buf = NULL;

    if ( NULL == fh )
    {
        fprintf(stderr, "cannot open %s\n", path);
        return NULL;
    }
    if ( 0 != fseek(fh, 0, SEEK_END) )
    {
        fclose(fh);
        return NULL;
    }
    size = ftell(fh);
    if ( (0 > size) || (8U * 1024U * 1024U < (unsigned long)size) )
    {
        fclose(fh);
        fprintf(stderr, "ROM size refused\n");
        return NULL;
    }
    rewind(fh);
    buf = (uint8_t *)malloc((size_t)size);
    if ( NULL == buf )
    {
        fclose(fh);
        return NULL;
    }
    if ( 1U != fread(buf, (size_t)size, 1U, fh) )
    {
        free(buf);
        fclose(fh);
        return NULL;
    }
    fclose(fh);
    *len = (uint32_t)size;
    return buf;
}

/**
 * @brief Write one RGB565 frame as a binary PPM.
 */
static void write_ppm(const char *path, const uint16_t *lcd)
{
    FILE *fh = fopen(path, "wb");
    uint32_t i = 0U;

    if ( NULL == fh )
    {
        fprintf(stderr, "cannot write %s\n", path);
        return;
    }
    fprintf(fh, "P6\n%u %u\n255\n", GBC_LCD_W, GBC_LCD_H);
    for ( i = 0U; (GBC_LCD_W * GBC_LCD_H) > i; i++ )
    {
        uint16_t const px = lcd[i];
        uint8_t const rgb[3] = {
            (uint8_t)(((px >> 11) & 0x1FU) * 255U / 31U),
            (uint8_t)(((px >> 5) & 0x3FU) * 255U / 63U),
            (uint8_t)((px & 0x1FU) * 255U / 31U)
        };

        fwrite(rgb, 1U, 3U, fh);
    }
    fclose(fh);
    printf("wrote %s\n", path);
}

/**
 * @brief How many distinct RGB565 values are in the frame.
 */
static uint32_t colours_of(const uint16_t *lcd)
{
    uint16_t seen[8];
    uint32_t n = 0U;
    uint32_t i = 0U;

    for ( i = 0U; (GBC_LCD_W * GBC_LCD_H) > i; i++ )
    {
        uint32_t k = 0U;
        bool found = false;

        for ( k = 0U; n > k; k++ )
        {
            if ( seen[k] == lcd[i] )
            {
                found = true;
            }
        }
        if ( !found && (8U > n) )
        {
            seen[n] = lcd[i];
            n++;
        }
    }
    return n;
}

/**
 * @brief First line of model name from /proc/cpuinfo.
 */
static void print_cpu(void)
{
    FILE *fh = fopen("/proc/cpuinfo", "r");
    char line[256];

    if ( NULL == fh )
    {
        printf("host cpu: unknown\n");
        return;
    }
    while ( NULL != fgets(line, (int)sizeof(line), fh) )
    {
        if ( 0 == strncmp(line, "model name", 10) )
        {
            printf("host cpu: %s", strchr(line, ':') ? strchr(line, ':') + 2 : line);
            break;
        }
    }
    fclose(fh);
}

int main(int argc, char **argv)
{
    const char *path = "roms/scroll.gb";
    uint8_t *rom = NULL;
    uint32_t rom_len = 0U;
    uint16_t *lcd = NULL;
    uint16_t *earlier = NULL;
    uint32_t i = 0U;
    int rc = 1;
    gbc_core_stats_t stats;

    if ( 1 < argc )
    {
        path = argv[1];
    }
    rom = read_file(path, &rom_len);
    lcd = (uint16_t *)calloc((size_t)GBC_LCD_W * GBC_LCD_H, sizeof(uint16_t));
    earlier = (uint16_t *)malloc((size_t)GBC_LCD_W * GBC_LCD_H * sizeof(uint16_t));
    if ( (NULL == rom) || (NULL == lcd) || (NULL == earlier) )
    {
        fprintf(stderr, "setup failed\n");
        free(rom);
        free(lcd);
        free(earlier);
        return 1;
    }
    gbc_core_set_clock(1700000000);
    if ( !gbc_core_open(rom, rom_len, NULL, 0U) )
    {
        fprintf(stderr, "core refused %s\n", path);
        free(rom);
        free(lcd);
        free(earlier);
        return 1;
    }
    for ( i = 0U; RUN_FRAMES > i; i++ )
    {
        gbc_core_draw(lcd, NULL, rom, rom_len);
        if ( DUMP_A == i )
        {
            memcpy(earlier, lcd, (size_t)GBC_LCD_W * GBC_LCD_H * sizeof(uint16_t));
            write_ppm("frame_30.ppm", lcd);
        }
        if ( DUMP_B == i )
        {
            write_ppm("frame_90.ppm", lcd);
        }
    }
    gbc_core_stats(&stats);
    print_cpu();
    printf("rom %s bytes %u\n", path, (unsigned)rom_len);
    printf("frames %u avg_us %u worst_us %u last_us %u audio_samples %u\n",
           (unsigned)stats.frames, (unsigned)stats.avg_us,
           (unsigned)stats.worst_us, (unsigned)stats.last_us,
           (unsigned)stats.audio_frames);
    printf("host sink samples %u\n", (unsigned)s_audio);
    printf("caveat: host microseconds do not predict ESP32-P4 frame time\n");
    if ( 0U == stats.frames )
    {
        fprintf(stderr, "no frames\n");
    }
    else if ( 2U > colours_of(lcd) )
    {
        fprintf(stderr, "frame stayed one colour\n");
    }
    else if ( 0 == memcmp(earlier, lcd, (size_t)GBC_LCD_W * GBC_LCD_H * sizeof(uint16_t)) )
    {
        fprintf(stderr, "frame did not change between %u and %u\n", DUMP_A, RUN_FRAMES - 1U);
    }
    else
    {
        rc = 0;
        printf("OK run\n");
    }
    free(rom);
    free(lcd);
    free(earlier);
    return rc;
}
