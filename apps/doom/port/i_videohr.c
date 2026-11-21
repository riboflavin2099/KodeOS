#include "config.h"

#include "doomtype.h"
#include "i_videohr.h"

boolean I_SetVideoModeHR(void)
{
    return false;
}

void I_UnsetVideoModeHR(void)
{
}

void I_SetWindowTitleHR(const char *title)
{
    (void)title;
}

void I_ClearScreenHR(void)
{
}

void I_SlamBlockHR(int x, int y, int w, int h, const byte *src)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)src;
}

void I_SlamHR(const byte *buffer)
{
    (void)buffer;
}

void I_InitPaletteHR(void)
{
}

void I_SetPaletteHR(const byte *palette)
{
    (void)palette;
}

void I_FadeToPaletteHR(const byte *palette)
{
    (void)palette;
}

void I_BlackPaletteHR(void)
{
}

boolean I_CheckAbortHR(void)
{
    return false;
}
