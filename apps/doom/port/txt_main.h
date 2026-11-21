#ifndef DOOM_TXT_MAIN_H_
#define DOOM_TXT_MAIN_H_

/* Heretic's text startup. TXT_Init fails so that screen is skipped. */

typedef enum {
    TXT_COLOR_BLACK,
    TXT_COLOR_BLUE,
    TXT_COLOR_GREEN,
    TXT_COLOR_CYAN,
    TXT_COLOR_RED,
    TXT_COLOR_MAGENTA,
    TXT_COLOR_BROWN,
    TXT_COLOR_GREY,
    TXT_COLOR_DARK_GREY,
    TXT_COLOR_BRIGHT_BLUE,
    TXT_COLOR_BRIGHT_GREEN,
    TXT_COLOR_BRIGHT_CYAN,
    TXT_COLOR_BRIGHT_RED,
    TXT_COLOR_BRIGHT_MAGENTA,
    TXT_COLOR_YELLOW,
    TXT_COLOR_BRIGHT_WHITE
} txt_color_t;

int TXT_Init(void);
void TXT_Shutdown(void);
void TXT_UpdateScreen(void);
unsigned char *TXT_GetScreenData(void);
int TXT_GetChar(void);

#endif
