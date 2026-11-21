#ifndef DOOM_SDL_FILESYSTEM_H_
#define DOOM_SDL_FILESYSTEM_H_

#include <stdlib.h>

char *SDL_GetPrefPath(const char *org, const char *app);
#define SDL_free free

#endif
