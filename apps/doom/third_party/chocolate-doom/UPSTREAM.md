# Chocolate Doom pin

Vendored under `src/` at one commit. Strife, the setup UI, and the SDL
platform files are not included. `port/` in the app replaces video, input,
timer, sound and the zone.

| | |
|---|---|
| Upstream | https://github.com/chocolate-doom/chocolate-doom |
| Commit | `a2dfd51fcc00d272dc8a17a97ccd6683333ff4ea` |
| Licence | GPL-2.0-or-later |

Checked at that commit: `src/doom/d_main.c`, `src/heretic/d_main.c` and
`src/hexen/h2_main.c` each say "either version 2 of the License, or (at your
option) any later version". The firmware is GPL-3.0-or-later because it links
the SDK, and this grant allows that. Do not replace this pin with
`id-Software/DOOM` (the 2024 README says "GPL 2.0" and does not say "or
later") or with retro-go's prboom-go.

`src/i_input.h` and `src/i_joystick.h` are removed. They include SDL. The
copies in `apps/doom/port` are what the compiler sees. `src/i_swap.h` stays;
`port/SDL_endian.h` supplies the byte swaps on a little-endian CPU.

Heretic and Hexen are compiled into their own archives. `port/prefix_objs.py`
prefixes the globals those trees define, then restores libc and shared names
so one image can call `D_DoomMain`, `heretic_D_DoomMain` and
`hexen_D_DoomMain`. Legacy of Rust (`id1.wad`) needs MBF21 or ID24, which
this pin does not implement. No WAD is vendored.
