#include "SDL_filesystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

char *SDL_GetPrefPath(const char *org, const char *app)
{
    const char *dir = "/tmp/choco-cfg";
    char *copy = NULL;

    (void)org;
    (void)app;
#if !defined(DOOM_HOST)
    dir = "/sdcard/doom";
#endif
    copy = malloc(strlen(dir) + 2);
    if ( copy == NULL )
    {
        return NULL;
    }
    sprintf(copy, "%s/", dir);
#if defined(DOOM_HOST)
    mkdir(dir, 0755);
#else
    (void)dir;
#endif
    return copy;
}
