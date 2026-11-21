/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file engine_run.c
 * @brief Run Chocolate Doom headless for a few hundred tics and print frame times.
 *
 * The WAD comes from DOOM_WAD_DIR or /tmp/doom-wads. Nothing here is a
 * commercial WAD, and this file does not embed one.
 */
#include <dirent.h>
#include <execinfo.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "doom_port.h"
#include "m_argv.h"

extern void D_DoomMain(void);
extern int gametic;

static int ends_with(const char *name, const char *suffix)
{
    size_t n = strlen(name);
    size_t s = strlen(suffix);

    if ( n < s )
    {
        return 0;
    }
    return strcasecmp(name + (n - s), suffix) == 0;
}

static int is_wad(const char *name)
{
    return ends_with(name, ".wad");
}

static int prefer(const char *name)
{
    if ( !is_wad(name) )
    {
        return 0;
    }
    if ( strcasecmp(name, "DOOM2.WAD") == 0 || strcasecmp(name, "freedoom2.wad") == 0 )
    {
        return 100;
    }
    if ( strcasecmp(name, "DOOM.WAD") == 0 || strcasecmp(name, "freedoom1.wad") == 0
         || strcasecmp(name, "doom1.wad") == 0 )
    {
        return 80;
    }
    if ( strcasecmp(name, "TNT.WAD") == 0 || strcasecmp(name, "PLUTONIA.WAD") == 0 )
    {
        return 60;
    }
    if ( strcasecmp(name, "HERETIC.WAD") == 0 )
    {
        return 50;
    }
    if ( strcasecmp(name, "HEXEN.WAD") == 0 )
    {
        return 40;
    }
    return 1;
}

static int find_wad(const char *dir, char *out, size_t cap)
{
    DIR *dp = opendir(dir);
    struct dirent *ent = NULL;
    int best = 0;
    char chosen[256];

    chosen[0] = '\0';
    if ( dp == NULL )
    {
        return 0;
    }
    while ( (ent = readdir(dp)) != NULL )
    {
        int rank = prefer(ent->d_name);

        if ( rank > best )
        {
            best = rank;
            snprintf(chosen, sizeof chosen, "%s", ent->d_name);
        }
    }
    closedir(dp);
    if ( best == 0 )
    {
        return 0;
    }
    snprintf(out, cap, "%s/%s", dir, chosen);
    return 1;
}

static int map_warp(const char *path)
{
    const char *base = strrchr(path, '/');

    base = base == NULL ? path : base + 1;
    if ( strcasecmp(base, "DOOM2.WAD") == 0 || strcasecmp(base, "freedoom2.wad") == 0
         || strcasecmp(base, "TNT.WAD") == 0 || strcasecmp(base, "PLUTONIA.WAD") == 0
         || strcasecmp(base, "HEXEN.WAD") == 0 )
    {
        return 1;
    }
    return 2;
}

static void on_crash(int sig)
{
    void *frames[32];
    int n = backtrace(frames, 32);

    fprintf(stderr, "signal %d\n", sig);
    backtrace_symbols_fd(frames, n, STDERR_FILENO);
    _exit(128 + sig);
}

int main(int argc, char **argv)
{
    const char *dir = getenv("DOOM_WAD_DIR");
    char wad[512];
    char *args[12];
    char store[12][512];
    int n = 0;
    int code = 0;
    int tics = 200;
    jmp_buf quit;
    double wall_avg = 0.0;
    double wall_max = 0.0;
    double busy_avg = 0.0;
    double busy_max = 0.0;
    const char *tics_env = getenv("DOOM_TICS");

    (void)argc;
    (void)argv;
    signal(SIGSEGV, on_crash);
    signal(SIGABRT, on_crash);
    if ( (dir == NULL) || (dir[0] == '\0') )
    {
        dir = "/tmp/doom-wads";
    }
    if ( tics_env != NULL )
    {
        tics = atoi(tics_env);
    }
    if ( !find_wad(dir, wad, sizeof wad) )
    {
        fprintf(stderr, "no WAD in %s (set DOOM_WAD_DIR)\n", dir);
        return 1;
    }
    snprintf(store[n], sizeof store[n], "%s", "/tmp/doom");
    args[n] = store[n];
    n++;
    snprintf(store[n], sizeof store[n], "%s", "-iwad");
    args[n] = store[n];
    n++;
    snprintf(store[n], sizeof store[n], "%s", wad);
    args[n] = store[n];
    n++;
    snprintf(store[n], sizeof store[n], "%s", "-warp");
    args[n] = store[n];
    n++;
    snprintf(store[n], sizeof store[n], "%s", "1");
    args[n] = store[n];
    n++;
    if ( map_warp(wad) == 2 )
    {
        snprintf(store[n], sizeof store[n], "%s", "1");
        args[n] = store[n];
        n++;
    }
    snprintf(store[n], sizeof store[n], "%s", "-skill");
    args[n] = store[n];
    n++;
    snprintf(store[n], sizeof store[n], "%s", "3");
    args[n] = store[n];
    n++;
    myargc = n;
    myargv = args;
    doom_port_set_max_tics(tics);
    fprintf(stderr, "engine %s for %d tics\n", wad, tics);
    code = setjmp(quit);
    if ( code == 0 )
    {
        doom_port_arm_quit(&quit);
        M_SetExeDir();
        D_DoomMain();
    }
    doom_port_frame_times(&wall_avg, &wall_max, &busy_avg, &busy_max);
    printf(
        "frames %d gametic %d wall_avg %.2f ms wall_max %.2f ms "
        "busy_avg %.2f ms busy_max %.2f ms exit %d\n",
        doom_port_frames(),
        gametic,
        wall_avg,
        wall_max,
        busy_avg,
        busy_max,
        code);
    return (code == 1 || code == 0) ? 0 : 1;
}
