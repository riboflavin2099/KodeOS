#include "txt_io.h"
#include "txt_main.h"

#include <stddef.h>

int TXT_Init(void)
{
    return 0;
}

void TXT_Shutdown(void)
{
}

void TXT_UpdateScreen(void)
{
}

unsigned char *TXT_GetScreenData(void)
{
    return NULL;
}

int TXT_GetChar(void)
{
    return 0;
}

void TXT_PutChar(int c)
{
    (void)c;
}

void TXT_Puts(const char *s)
{
    (void)s;
}

void TXT_GotoXY(int x, int y)
{
    (void)x;
    (void)y;
}

void TXT_FGColor(txt_color_t color)
{
    (void)color;
}

void TXT_BGColor(int color, int blinking)
{
    (void)color;
    (void)blinking;
}
