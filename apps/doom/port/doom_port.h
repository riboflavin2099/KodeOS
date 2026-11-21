#ifndef DOOM_PORT_H_
#define DOOM_PORT_H_

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>

/* I_Quit and I_Error longjmp to this buffer. It is copied. */
void doom_port_arm_quit(jmp_buf buf);

/* Stop after this many gametics. Zero runs until quit. */
void doom_port_set_max_tics(int n);
int doom_port_max_tics(void);

void doom_port_request_quit(void);
int doom_port_quit_requested(void);
void doom_port_pause(bool paused);
int doom_port_paused(void);

typedef struct {
    bool up;
    bool down;
    bool left;
    bool right;
    bool fire;
    bool strafe;
    bool run;
} doom_port_pad_t;

void doom_port_set_pad(const doom_port_pad_t *pad);
void doom_port_read_pad(doom_port_pad_t *pad);

/* One keydown followed, on the next tic, by a keyup. Safe from the frame task. */
void doom_port_pulse(int key);
int doom_port_pulse_pending(void);
int doom_port_next_pulse(int *key, int *down);

void doom_port_note_sleep_ms(int ms);
int doom_port_take_slept_ms(void);

void doom_port_publish(const uint16_t *rgb, double wall_ms, double busy_ms);
int doom_port_copy_frame(uint16_t *dst);
int doom_port_frames(void);
void doom_port_frame_times(double *wall_avg, double *wall_max,
                           double *busy_avg, double *busy_max);

void doom_port_leave(int code);

#endif
