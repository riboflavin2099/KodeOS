#include "config.h"

#include "doomtype.h"
#include "i_input.h"

float mouse_acceleration = 2.0f;
int mouse_threshold = 10;
int novert = 1;

void I_BindInputVariables(void)
{
}

void I_ReadMouse(void)
{
}

void I_StartTextInput(int x1, int y1, int x2, int y2)
{
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
}

void I_StopTextInput(void)
{
}
