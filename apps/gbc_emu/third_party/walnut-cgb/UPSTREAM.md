# Walnut-CGB pin

Vendored from <https://github.com/Mr-PauI/Walnut-CGB> at commit `a42c186917516cc9c80515ceae1121600a071628` (branch `master`, 2026-11-13).

| Path here | Upstream path |
|---|---|
| `LICENSE` | `LICENSE` |
| `walnut_cgb.h` | `walnut_cgb.h` |
| `minigb_apu/LICENSE` | `examples/sdl2/minigb_apu/LICENSE` |
| `minigb_apu/minigb_apu.h` | `examples/sdl2/minigb_apu/minigb_apu.h` |
| `minigb_apu/minigb_apu.c` | `examples/sdl2/minigb_apu/minigb_apu.c` |

`walnut_cgb.h` is MIT and also carries the MIT grant for Peanut-GB (Mahyar Koshkouei) and for the SameBoy fragments marked in that file (Lior Halphon). `minigb_apu` is MIT (Alex Baines and Mahyar Koshkouei).

Nothing in this directory is in the firmware link yet. `main/gbc_core.c` is the placeholder that will include `walnut_cgb.h`. Do not add the header to `CMakeLists.txt` until the callbacks and the `ENABLE_LCD` / `MINIGB_APU_AUDIO_FORMAT_S16SYS` defines are in place: the header is a single-file library that compiles the core into whichever `.c` includes it.

The upstream tree also contains GIF captures of commercial games. Those files are not vendored here and must not be copied into `appdata/screenshots/`.
