# A Game Boy Color emulator on the Kode Dot

Research date: 2026-11-13. SDK commit in this tree: `1841cc8c13d4b2e3e4d7288b40cdac5de207a133`. Walnut-CGB files under `third_party/walnut-cgb/` are from `a42c186917516cc9c80515ceae1121600a071628`.

## Verdict

A Game Boy Color emulator is feasible as a Kode Dot app. It should be a separate firmware, built the way `kode-app-template` and Kody Kart are built, and installed into the apps region. It should not be linked into kodeOS.

The core to start from is **Walnut-CGB**, MIT, a single-header C library with Game Boy Color, MBC1, MBC2, MBC3 (including the clock), MBC5 and MBC30. Sound is a second MIT library, **minigb_apu**, which is vendored beside it. Both can sit inside an app that the catalogue requires to be GPL-3.0-or-later, provided their copyright notices stay with the files. The combined firmware is conveyed under GPL-3 because it links the SDK.

gnuboy is the core with a published ESP32-P4 result (full speed, no frameskip, in RetroESP32-P4's own report). It is the wrong code to copy. Retro-Go ships it under the GPL version 2 text, and Gambatte's sources say "GPL version 2" with no "or later". GPL-2-only does not combine with the SDK's GPL-3. Treat those trees as measurements, not as source.

Full speed is likely and not yet measured on a Kode Dot. The P4 in this board's `sdkconfig.board` is set to 400 MHz, with 32 MB of PSRAM at 250 MHz in HEX mode. The reports below were taken at 360 MHz on other boards. The scaffold in this directory draws and presents frames. It does not run guest instructions.

No commercial ROM is included, and none should be added. The person running the app supplies their own.

## What an app actually is

The picture "a whole firmware, loaded into PSRAM from the microSD card" does not match this tree.

An app is a separate ESP-IDF firmware. kodeOS and the app do not run at the same time. kodeOS keeps installed apps in the `apps` flash region. To start one it writes a boot request and resets. The bootloader override in `bootloader_components/main/bootloader_start.c` checks the request and then calls `bootloader_utility_load_boot_image()`. The comment there is "the stock loader maps and jumps". The image is executed in place through the flash cache. It is not copied into PSRAM first. The boot-model page on docs.kode.diy says the same thing: the bootloader starts an app from the region in place, without copying it.

`sdk/components/kbsp/partitions.csv` in this checkout:

| Region | Offset | Size | Role |
|---|---|---|---|
| `factory`, `ota_0`, `ota_1` | `0x20000` onward | 4 MB each | kodeOS slots. An app never writes `otadata` |
| `apps` | `0xC40000` | `0xBC0000` (12,320,768 bytes) | directory, boot request, installed images |
| `storage` | `0x1800000` | 2 MB | wear-levelled FAT. App icons live at `.kode/appsdata/<project>/` |
| `app_pending` | `0x1A00000` | `0x600000` (6 MB) | mailbox. Not a boot slot |

Images in `apps` start on a 64 KB boundary (`KAPPS_ALIGN` / `KAPPS_DATA_OFF` are `0x10000` in `kapps_fmt.h`). The directory occupies the first 64 KB, so the images share about 11.7 MB, up to 64 entries. A boot request is one-shot (`NEW`, then the bootloader writes `TRIED`) unless it came from the mailbox, in which case it is sticky (`ALWAYS`, then `ARMED`) and survives the resets a host produces, including opening the serial port. Holding the bottom button and left together is a power-on reset and returns to kodeOS. `CONFIG_KODEOS_BOOT_ROLE_APP` is what makes the app refuse to confirm `otadata`, so a crash still comes home.

The microSD card is a place files are read from, and one of the sources `install` accepts (`sd:`, `fs:`, `http:`, `https:`, `pending`, the cable). It is not where the running firmware is fetched into RAM. For this emulator the card is the right place for ROMs and battery saves. The firmware image itself is installed into `apps`.

PSRAM (32 MB, `CONFIG_SPIRAM` and `CONFIG_SPIRAM_SPEED_250M` in `sdkconfig.board`) is memory the running firmware allocates. The ROM image, the cartridge RAM and the 160×144 frame belong there. The SDK's `tests/target/ram_budget.txt` calls the on-chip RAM 640 KB and caps the static image of a full SDK build at 368,520 bytes. That is the budget that matters. A catalogue app that goes past it has to say why, and it may not pass 417,000 bytes (kode-apps rule B8).

The current `kode-app-template` README tells the reader to `write-flash` at `0x1C00000`, and says never to `idf.py flash`. Neither matches this SDK. The mailbox here is `0x1A00000`. `0x1C00000` is 2 MB into that partition. `sdk/components/kbsp/project_include.cmake` retargets `idf.py flash` and `app-flash`, in the app role only, to one write of the image at the mailbox. `build/flash_args` from the scaffold build is that single line. `idf.py erase-flash` is not retargeted and still erases the chip. `tools/kcli.py install --dst app` does not hard-code the address and is the path for an image the mailbox cannot hold (`0x5FF000`; the apps region is larger). A project that does not use this `kbsp` still flashes over the factory slot.

Where the project should live: its own repository, created from `kode-app-template`, named so the `project()` string is `gbc_emu`, then a submodule under `kode-apps` `apps/gbc_emu`. Rule A1 rejects an app that is not that shape. This directory is the scaffold inside the fork so the research and the code can be reviewed together. It is not registered in the kodeOS `CMakeLists.txt`, because kodeOS does not link apps in.

Kody Kart is the right reference for a real-time app. Its README says it paints without LVGL, doubles a half-resolution frame into `kdrv_display_back_buffer()` and flips, and gets about 45–50 fps against about 17 through an `lv_canvas`. kodeOS itself refreshes LVGL every 33 ms (`CONFIG_LV_DEF_REFR_PERIOD` in the kodeOS `sdkconfig.defaults`). An emulator should follow Kody Kart, not the widget screen.

## What the Dot gives the emulator

Numbers below are from this SDK checkout, not from a datasheet paraphrase, except where a file is named.

### Display

The panel is 410×502 RGB565 (`KBSP_LCD_H_RES`, `KBSP_LCD_V_RES` in `kbsp_board.h`). `kdrv_display.h` times one refresh at 16,829 µs, which is about 59.42 Hz. A Game Boy frame is 4,194,304 / 70,224 ≈ 59.73 Hz, in single speed and in CGB double speed (double speed runs the CPU twice per frame; the frame rate stays). Locking one guest frame to one panel flip runs the game about half a percent slow and avoids tearing. That is the right pacing. A free-running 59.73 Hz loop against a 59.42 Hz panel will tear.

A 2× nearest-neighbour scale is 320×288. Centred, the side margin is 45 px. This scaffold puts the picture at y = 48 so the band underneath can hold Start and Select. The ESP32-P4 has a pixel-processing accelerator, and `kdrv_display.h` documents using it to scale into the back buffer. Nearest-neighbour 2× does not need it. The back buffer is asked for after `kdrv_display_flush_wait()`, then `kdrv_display_flush()` of the whole panel flips with no extra copy when the pointer is one of the panel's own buffers.

`kode_display_set_brightness()` is the supported brightness call. `kdrv_display_init()` is AO_Display's. The app must not call it again.

Catalogue rule C3 warns on including the LCD driver header. It does not reject the build. Kody Kart includes it anyway. This scaffold does too, and the README says so.

### Input

`kode.h` names six buttons: `up`, `down`, `left`, `right`, `top`, `bottom`. That is a D-pad plus A and B, and it is not eight Game Boy buttons.

| Game Boy | Kode Dot |
|---|---|
| D-pad | the pad |
| A | top |
| B | bottom, level only |
| Start | touch zone under the picture |
| Select | the zone beside it |

Rule D8: holding bottom returns to kodeOS, and the app must not give that hold a meaning of its own. `kode_button_read_all()` samples the level. A long press of bottom is B for a moment and then the SDK resets. That is acceptable.

Touch is 410×502, origin top-left (`kode_touch_read`). The first call asks the panel for points, so it belongs on the frame task, not in an Active Object step.

`kode_imu_read()` exists (accelerometer and gyro). It is not wired. A tilt mapping fights the D-pad on a device that already has one. Leave it off until someone asks for it.

### Audio

The codec is an ES8311 at 48 kHz (`KDRV_ES8311_RATE_HZ`). `kode_audio_play_samples()` plays 48 kHz mono s16 and blocks until the buffer has been heard. That is the wrong call for a continuous emulator.

`kdrv_es8311.h` has a stream: a 200 ms ring in PSRAM (`KDRV_ES8311_STREAM_FRAMES`). Opening and closing it are AO_Audio's. `kdrv_es8311_stream_write()` is the writer's call, from its own task, and it accepts mono or stereo (a stereo pair is played as its mean). Rule C3 also warns on including the codec header. The scaffold does not include it yet. `gbc_audio.c` refuses the open and drops frames, and the header states the rate the next step has to hit.

minigb_apu's default is 32,768 Hz stereo (`AUDIO_SAMPLE_RATE` in `minigb_apu.h`). Either rebuild it at 48 kHz or resample. A half-percent frame-rate error against the panel is a small pitch error if audio is generated from emulated time and consumed at 48 kHz. Measure it. Do not use Blargg's Gb_Snd_Emu for the first port: that library is GPL and pulls the same licence problem as gnuboy.

### Memory

| Use | Size | Where |
|---|---|---|
| ROM | up to 8 MiB (MBC5) | PSRAM. `kode_file_read_bytes()` stops at 4 MiB (`KODE_FILE_READ_MAX`). `kode_file_read_at()` does not, and AO_Storage moves 32 KiB at a time (`KODE_SYS_FILE_READ_MAX`) |
| Cartridge RAM | up to 128 KiB | PSRAM, and a `.sav` on the card |
| MBC3 clock | 8 bytes | a `.rtc` file, Unix seconds. The live clock is `kode_time_unix()`, which returns −1 while unset |
| Guest LCD | 160×144×2 = 46,080 bytes | PSRAM |
| Panel framebuffers | two times about 412 KiB, cache-line rounded (`KDRV_DISPLAY_FB_BYTES`) | PSRAM, owned by the display driver |
| Codec ring | 200 ms at 48 kHz mono, about 19 KiB | PSRAM, owned by the codec driver |
| Emulator state | well under 256 KiB for this class of core | PSRAM for anything large. Hot state can stay internal if it is small |

32 MB of PSRAM holds an 8 MB ROM with room to spare. The path limit is 91 bytes including the terminator (`KODE_FILE_PATH_MAX`). A listing page is 24 names (`KODE_FILE_LIST_MAX`). The scaffold loads the first `.gb` or `.gbc` in the first page of `sd/gbc` and logs when the folder has more.

Internal RAM is the tight resource, not PSRAM. The frame task's 16 KB stack is created with `xTaskCreatePinnedToCore` (ESP-IDF counts that depth in bytes) and comes out of the internal heap. Rule C9 wants `test component=mem` to still show 64 KB free after the heaviest path. That has not been run.

The worker that reads the card has an 8 KB stack in PSRAM (`kode_worker_start`). A job on that stack must not call flash, a partition API or NVS. SD file calls are allowed there. The Active Object does not block.

### CPU

`sdkconfig.board` sets `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_400`. Two RISC-V cores. The frame task is pinned to core 1 at `KODE_PRIO_USER_GUI` (3). That priority is the renderer's. It is not taken by an Active Object in an app that does not start kgui. The task is not itself an Active Object, because `kdrv_display_flush_wait()` blocks.

A rough budget: about 4.2 million guest cycles per second, or about 8.4 million in CGB double speed, against 400 million host cycles on one core. That is on the order of 50 to 100 host cycles per guest cycle. Interpreters of the Peanut-GB / gnuboy kind fit in that. Cycle-accurate cores (SameBoy, Gambatte) are a different project.

Published points, none of them a Kode Dot:

- RetroESP32-P4 (`giltal/RetroESP32-P4`, architecture note dated 25 May 2026, ESP-IDF 5.5.2, ESP32-P4 at 360 MHz, 32 MB PSRAM) runs Game Boy and Game Boy Color on gnuboy as its own OTA image and puts them in the set that holds 60 fps with no frameskip. SNES, Genesis and Neo Geo are the ones that drop frames.
- A retro-go ESP32-P4 branch (`ducalex/retro-go` issue 211, July 2025, IDF 5.5-rc1, 360 MHz) reports Genesis at full speed and treats SNES and GBA as the hard cases. Game Boy is not the thing that person was struggling with.
- Peanut-GB's README says the DMG core is full speed on an RP2040. A 2019 ESP8266 attempt was display-bound; the same author reported a later ESP32 port as full speed. That is a comment on a blog, not a Kode Dot measurement.
- Walnut-CGB's README shows Game Boy Color titles on an ESP32-S3 Cardputer and says the captures were of emulation at the native frame rate. The Arduino example on that page is also described as RAM-limited and without paging. There is no cycle count and no P4 number. S3 evidence is not a budget.

So: the class of core fits a 360 MHz P4 in someone else's gnuboy port. Walnut is a faster rewrite of the Peanut-GB family and should fit this 400 MHz part. The first thing to measure, once `gb_run_frame_dualfetch()` is in the frame task, is milliseconds per frame on core 1 with audio off, then with minigb_apu on, on a CGB double-speed scene. If that misses 16.8 ms, the fallback is binjgb (also MIT) or, only after a licence review that finds an "or later" grant, gnuboy.

## Cores

| Core | Licence, as checked | Language | GBC | How it embeds | ESP32-class evidence |
|---|---|---|---|---|---|
| Walnut-CGB | MIT (file header plus `LICENSE`). Contains MIT Peanut-GB and marked SameBoy fragments | C, one header, about 280 KB | Yes. MBC1/2/3/30/5, RTC, serial hook | One `#include`. The front end supplies ROM and RAM callbacks and `lcd_draw_line`. Audio is not inside the header | S3 Cardputer captures. No P4 number. Line-based LCD, so some mid-scan effects are wrong (the README names Prehistorik Man, same caveat as Peanut-GB) |
| minigb_apu | MIT, Alex Baines and Mahyar Koshkouei | C | APU only | `audio_read` / `audio_write` hooks Walnut asks for when `ENABLE_SOUND` is 1. Default rate 32,768 Hz stereo | Used with Peanut-GB. Timing is not sample-accurate. The README says it runs on its own thread in the SDL example |
| Peanut-GB | MIT, in `peanut_gb.h` | C, one header | No. The README says only DMG, with CGB work started and not supported | Same shape Walnut grew out of | Full speed claimed on RP2040. ESP32 ports exist. Not a GBC core |
| binjgb | MIT, Ben Smith, 2016 | C, several files | Yes | A library, not a header. No GUI in the core. Used as WebAssembly | No ESP32 port found. The fallback if Walnut's compatibility is too thin |
| SameBoy | Expat (MIT), Lior Halphon, `LICENSE` on `master`. The iOS directory adds a marketplace clause that does not apply if that directory is left out | C | Yes, high accuracy | Large. A libretro or SDL port, not a single file | No embedded full-speed report worth trusting. Wrong first port |
| mGBA | MPL-2.0 | C | Yes, beside a GBA core | Large tree | MPL-2.0 can be combined with GPL-3 (the MPL files stay MPL). Too much code for a first GBC app |
| Gearboy | GPL-3.0 (GitHub licence on `drhelius/Gearboy`) | C++ | Yes | A program more than a library | GPL-3 is compatible with the SDK. C++ runtime, size and speed on the P4 are unknown. Not the first port |
| gnuboy, as in retro-go `retro-core/components/gnuboy` | The component's `COPYING` is the GPL version 2 text. The retro-go repository is classified GPL-2.0. Sampled sources (`gnuboy.c`, `cpu.c`) have no "or later" grant in the header | C | Yes, and it has its own APU | A small set of .c files, tied to retro-go's host struct | The ESP32 and ESP32-P4 full-speed result. Do not vendor it into this app unless the grant is shown to be GPL-2.0-or-later or the copyright holders relicense |
| Gambatte (`libretro/gambatte-libretro` `libgambatte/src/cpu.cpp`) | The file header says "GNU General Public License version 2" and does not say "or later" | C++ | Yes, accurate | libretro core | GPL-2.0-only. Do not link it with the GPL-3 SDK |
| Retro-Go | `COPYING` is GPL version 2. GitHub classifies the repository GPL-2.0 | C | Via gnuboy | A launcher and a hardware abstraction, not a library to drop into an Active Object | The ESP32 handheld tree, plus the P4 branch in issue 211. Useful as a performance note. Not embeddable here, and the licence does not combine with GPL-3 if it is version 2 only |
| RetroESP32-P4 | The README says "MIT License". The tree also contains other cores' `COPYING` / `LICENSE` files (gnuboy's GPL-2 text among them) | C | gnuboy, its own OTA slot | A multi-firmware launcher. Native apps there are loaded into PSRAM. That is not the Kode Dot's model | The best public P4 fps numbers. Do not copy the Game Boy slot out of it. The README's MIT line does not relicense gnuboy |

## Licences, the SDK, and the catalogue

kodeOS and the parts of the SDK the `NOTICE.md` files do not list otherwise are GPL-3.0-or-later. QP/C is GPL-3.0-or-later (or a commercial Quantum Leaps licence; this tree uses the GPL). RFAL is SLA0051 and is not open source. The SDK's section 7 permission lets a GPL program be combined with RFAL. An app's `NOTICE.md` has to repeat that permission if the app links the NFC stack. This scaffold's `NOTICE.md` does, matching `kode-app-template`, even though the emulator does not call NFC.

kode-apps rule A5 rejects an app whose `LICENSE` is not the GPL version 3 text. Rule B1 rejects an SDK pin that is not the newest published kode-sdk release or the one before it. On this date the tags on `kodediy/kode-sdk` were `v0.1.0` and `v0.1.1` (`63af5e7`). This fork's submodule is `1841cc8`, which is not that tag. The scaffold builds against the submodule so the code matches the headers this note was written from. A catalogue pull request has to move the pin to a release tag and delete the `EXTRA_COMPONENT_DIRS` block. Rule B3 rejects a submodule or `EXTRA_COMPONENT_DIRS` for the SDK.

MIT into GPL-3 is the combination that works. The MIT files keep their notices (GPL-3 section 7 allows additional terms of that kind). The firmware the Dot runs is under GPL-3, because the SDK is. Someone who distributes the `.bin` has to offer the corresponding source of the app and of the SDK it linked.

GPL-2-only into GPL-3 does not work. That is gnuboy, as it sits in retro-go, and Gambatte, as its header is written. RetroESP32-P4's README calling the whole project MIT does not fix the gnuboy files inside it.

Publishing on apps.kode.diy adds the catalogue rules on top of the licence: the app repository is `kode-app-gbc_emu`, screenshots are 1 to 4 PNGs of 410×502 taken with `kcli.py screenshot` (rule D9, not done), the image is at most 8 MiB (rule B6) and in practice also has to fit the mailbox, whose usable size is `0x5FF000` (about 6.0 MB). An interpreter of this size will not approach either cap. The 8 MB figure is not a limit on the ROM. The ROM is a file on the card.

Trademarks: Game Boy and Game Boy Color are Nintendo's. The pill name in the scaffold is "GBC", which is still their mark used descriptively. A catalogue reviewer may want a name that does not use it. This note does not give permission to use Nintendo's marks, or Kode's.

ROMs: do not commit one, do not download one as part of the build, do not point the app at a ROM site. A homebrew or a test ROM is fine once its licence file has been read. Usual display tests, after that check, are Matt Currie's dmg-acid2 and cgb-acid2, and a small program built with RGBDS from a repository that states a permissive licence. Blargg's instruction tests are widely used; use a copy whose grant you have read. The scaffold ships no ROM. `host/test_header.c` builds a fake header in memory.

## How the pieces meet

1. `kode_init()` brings the owners up, including AO_Display and AO_Audio. The app Active Object starts after that. It does not block.
2. The worker lists `sd/gbc`, parses the header (`gbc_header.c`, covered by `host/test_header.c`), reads the image into aligned PSRAM and loads `sd/gbc/saves/<stem>.sav` and `.rtc`.
3. The frame task on core 1 reads the pad, calls the core for one frame into a 160×144 RGB565 buffer, and `gbc_display_present()` scales 2× into the back buffer and flushes.
4. Walnut's `lcd_draw_line` writes rows of that buffer. For a CGB image the line is palette indices; for a DMG image it is shade plus layer. The callback expands both to RGB565. `WALNUT_GB_32BIT_ALIGNED` is the macro to start with on the P4. The header's own note says 16-bit DMA can need `-fno-strict-aliasing` and is aimed at the ESP32-S3's unaligned behaviour. Prefer the aligned 32-bit path and measure before turning the other one on.
5. minigb_apu runs from the hooks Walnut calls when sound is enabled. Its samples are resampled to 48 kHz mono and written with `kdrv_es8311_stream_write` after AO_Audio has opened the stream.
6. Saves: call `gbc_save_store()` from the worker on a timer and when the app is about to be reset, not from the frame loop. `gb_get_save_size()` is Walnut's size. The header parser's RAM-size byte is what the scaffold uses until then.
7. RTC: `gb_set_rtc()` from the Unix time stored at load. If `kode_time_unix()` is −1, keep the file's value and accept that the clock is wrong until the Dot has a time.

## Risks

- Speed is unmeasured here. Double-speed CGB plus audio is the case that can miss a frame. The mitigation is to measure before adding a picker, tilt, or a shader.
- Walnut is young (copyright 2025–2026) and inherits Peanut-GB's line renderer. Some games will be wrong. binjgb is the MIT fallback. gnuboy is the fast fallback only with a licence that actually combines.
- Audio will be late and slightly off pitch until the resampler is tied to emulated time. minigb_apu is not cycle-accurate.
- The mailbox address in the upstream template README (`0x1C00000`) does not match this partitions table (`0x1A00000`). `idf.py flash` on this scaffold is safe because `kbsp` retargets it. `idf.py erase-flash` is not. A project without that retarget overwrites kodeOS.
- Catalogue rules B1 and B3 fail on purpose for this in-tree build. They pass only after the app is split out and pinned to a release tag.
- Rule C3 will warn on `kdrv_display.h`. That warning is the cost of 60 Hz. Do not also start LVGL.
- Internal RAM, not PSRAM, is the resource that can fail `test component=mem`. The 16 KB frame stack and Walnut's call depth are the things to watch.
- A ROM larger than the header claims, or a header whose size code this parser does not know, is refused. Unofficial size codes `0x52`, `0x53` and `0x54` are not accepted.
- Legal: distributing Nintendo's ROMs, boot ROMs or artwork is not part of this project. The vendored Walnut tree upstream contains GIF captures of commercial games. Those files were not copied. Do not add them as screenshots.

## What the scaffold does and does not do

It does: an ESP-IDF project named `gbc_emu`, role app, radio off, an Active Object plus a worker, a frame task, header parsing, SD load and save paths, a 2× present into the panel buffer, touch zones for Start and Select, and the Walnut and minigb_apu sources with their MIT notices.

It does not: execute a Game Boy instruction, open the speaker stream, or boot a ROM. `gbc_core_draw()` paints a pattern so the panel path can be seen before the core is included.

## Sources

- This tree: `README.md`, `NOTICE.md`, `bootloader_components/main/bootloader_start.c`, `sdk/components/kbsp/partitions.csv`, `sdk/components/kbsp/sdkconfig.board`, `sdk/components/kdrv/apps/include/kapps_fmt.h`, `sdk/components/kdrv/display/include/kdrv_display.h`, `sdk/components/kdrv/audio/include/kdrv_es8311.h`, `sdk/components/kode/include/kode.h`, `sdk/tests/target/ram_budget.txt`.
- docs.kode.diy, boot model and toolchain setup, fetched 2026-11-13.
- `kodediy/kode-app-template` `README.md`, `CMakeLists.txt`, `main/idf_component.yml` (pin `v0.1.1` in the file fetched that day).
- `kodediy/kode-apps` `RULES.md` and `categories.txt` on `develop`.
- `kodediy/kode-app-kody_kart` `README.md`.
- Walnut-CGB `README.md`, `LICENSE`, `walnut_cgb.h`, `examples/sdl2/minigb_apu/`.
- Peanut-GB `README.md` and the MIT block in `peanut_gb.h`.
- binjgb `LICENSE`. SameBoy `LICENSE`. `libretro/gambatte-libretro` `libgambatte/src/cpu.cpp` header. `ducalex/retro-go` `COPYING` and `retro-core/components/gnuboy/`. `ducalex/retro-go` issue 211. `giltal/RetroESP32-P4` `README.md` and `ARCHITECTURE.md`. `drhelius/Gearboy` and `mgba-emu/mgba` as classified by GitHub's licence API.
