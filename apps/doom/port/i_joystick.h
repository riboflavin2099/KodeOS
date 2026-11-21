#ifndef __I_JOYSTICK__
#define __I_JOYSTICK__

#include "doomtype.h"

#define JOY_GET_DPAD(x)   (((x) >> 0) & 0xf)
#define JOY_GET_LSTICK(x) (((x) >> 4) & 0xf)
#define JOY_GET_RSTICK(x) (((x) >> 8) & 0xf)

enum {
    JOY_DIR_NONE = 0x0,
    JOY_DIR_UP = 0x1,
    JOY_DIR_DOWN = 0x2,
    JOY_DIR_LEFT = 0x4,
    JOY_DIR_RIGHT = 0x8
};

extern int use_analog;
extern int joystick_turn_sensitivity;
extern int joystick_move_sensitivity;
extern int joystick_look_sensitivity;

void I_InitJoystick(void);
void I_ShutdownJoystick(void);
void I_UpdateJoystick(void);
void I_BindJoystickVariables(void);

#endif
