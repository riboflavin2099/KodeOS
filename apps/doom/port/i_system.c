#include "config.h"

#include "doomtype.h"
#include "i_system.h"

#include "doom_port.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(DOOM_HOST)
#include "esp_heap_caps.h"
#endif

#define AT_EXIT_MAX 32
#define ZONE_BYTES  (6 * 1024 * 1024)

static atexit_func_t s_exit[AT_EXIT_MAX];
static boolean s_run[AT_EXIT_MAX];
static int s_nexit;
static byte *s_zone;

void I_AtExit(atexit_func_t func, boolean run_if_error)
{
    if ( s_nexit >= AT_EXIT_MAX )
    {
        return;
    }
    s_exit[s_nexit] = func;
    s_run[s_nexit] = run_if_error;
    s_nexit++;
}

static void run_exit(boolean error)
{
    int i;

    for ( i = s_nexit - 1; i >= 0; --i )
    {
        if ( !error || s_run[i] )
        {
            s_exit[i]();
        }
    }
    s_nexit = 0;
}

void I_PrintBanner(const char *msg)
{
    printf("%s\n", msg);
}

void I_PrintDivider(void)
{
    printf("----------------------------------------\n");
}

void I_PrintStartupBanner(const char *gamedescription)
{
    printf("%s\n", gamedescription);
    printf(" Chocolate Doom, GPL-2.0-or-later, on KodeOS (GPL-3.0-or-later)\n");
}

void I_Quit(void)
{
    run_exit(false);
    doom_port_leave(0);
    abort();
}

void I_Error(const char *error, ...)
{
    va_list args;

    fprintf(stderr, "I_Error: ");
    va_start(args, error);
    vfprintf(stderr, error, args);
    va_end(args);
    fprintf(stderr, "\n");
    run_exit(true);
    doom_port_leave(2);
    abort();
}

boolean I_GetMemoryValue(unsigned int offset, void *value, int size)
{
    (void)offset;
    (void)value;
    (void)size;
    return false;
}

byte *I_ZoneBase(int *size)
{
    *size = ZONE_BYTES;
    if ( s_zone == NULL )
    {
#if defined(DOOM_HOST)
        s_zone = malloc((size_t)*size);
#else
        s_zone = heap_caps_malloc((size_t)*size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#endif
    }
    if ( s_zone == NULL )
    {
        I_Error("I_ZoneBase: no memory for the %d byte zone", *size);
    }
    memset(s_zone, 0, (size_t)*size);
    return s_zone;
}

void I_Tactile(int on, int off, int total)
{
    (void)on;
    (void)off;
    (void)total;
}

ticcmd_t *I_BaseTiccmd(void)
{
    static ticcmd_t empty;

    return &empty;
}

void *I_Realloc(void *ptr, size_t size)
{
    void *next = realloc(ptr, size);

    if ( (next == NULL) && (size != 0) )
    {
        I_Error("I_Realloc: failed on %u bytes", (unsigned)size);
    }
    return next;
}

boolean I_ConsoleStdout(void)
{
    return true;
}

void I_Init(void)
{
}

void I_BindVariables(void)
{
}

byte *I_AllocLow(int length)
{
    byte *p = malloc((size_t)length);

    if ( p == NULL )
    {
        I_Error("I_AllocLow: failed on %d bytes", length);
    }
    return p;
}

void I_StartupKeyboard(void)
{
}

void I_StartupJoystick(void)
{
}

void I_ShutdownKeyboard(void)
{
}
