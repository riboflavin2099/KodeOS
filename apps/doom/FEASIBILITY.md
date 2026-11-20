# A Doom engine on the Kode Dot

Research date: 2026-11-13, controls and the select screen updated 2026-11-19. SDK commit in this tree: `1841cc8c13d4b2e3e4d7288b40cdac5de207a133`. Chocolate Doom pin: `a2dfd51fcc00d272dc8a17a97ccd6683333ff4ea`.

## Verdict

A Doom-engine app is feasible as its own firmware, built the way `kode-app-template` is built and installed into the apps region. It should not be linked into kodeOS, and it should not be loaded from the microSD card into PSRAM. The card holds WADs, saves and config. The person running the app supplies every WAD. This tree contains none.

The engine to start from is **Chocolate Doom** at the commit above. Its Doom, Heretic and Hexen sources say "either version 2 of the License, or (at your option) any later version", so the app can convey them under GPL-3.0-or-later beside the SDK. Doom and Doom II are one tree. Heretic and Hexen are two more trees in the same repository. One firmware is the goal: the three trees collide on global names, and the link prefixes Heretic and Hexen. Strife is out of scope.

The select screen shows a logo button only for a game the scan found. The button is a text plate. The well is reserved for `M_DOOM`, `TITLEPIC`, `M_HTIC` or `TITLE` decoded later from that person's WAD. No id or Raven artwork is stored here. One detected game still shows the screen, as one large button, because launch is a tap.

Button 1, fire, is the **top** button, held as a level. Button 2, use, is the **bottom** button on the press edge. Holding bottom is kodeOS home (`KODE_BUTTON_HOLD_MS` is 600, then `KODE_SYS_HOME_GRACE_MS` is 200). Fire cannot sit on that button. A single tap on the glass pauses into a weapon list. A double tap, a second tap that starts within 300 ms, pauses into settings. The D-pad moves and turns.

Legacy of Rust (`id1.wad`) is detected and listed. Chocolate's vanilla limits do not play it: it is an MBF21/ID24 PWAD for Doom II. SIGIL and SIGIL II are vanilla-style PWADs on Ultimate Doom and this base can aim at them.

Full speed is likely and not measured on a Kode Dot. Expect the renderer to land near Doom's 35 Hz tic on ordinary maps, about 30–35 displayed frames a second, after a software palette expand and a small scale.

## What an app is

An app is a separate ESP-IDF firmware. kodeOS and the app do not run at the same time. kodeOS keeps installed apps in the `apps` flash region. To start one it writes a boot request and resets. The bootloader override in `bootloader_components/main/bootloader_start.c` calls `bootloader_utility_load_boot_image()`. The image runs in place through the flash cache.

`sdk/components/kbsp/partitions.csv` in this checkout:

| Region | Offset | Size | Role |
|---|---|---|---|
| `factory`, `ota_0`, `ota_1` | `0x20000` onward | 4 MB each | kodeOS slots |
| `apps` | `0xC40000` | `0xBC0000` (12,320,768 bytes) | installed images |
| `storage` | `0x1800000` | 2 MB | wear-levelled FAT |
| `app_pending` | `0x1A00000` | `0x600000` (6 MB) | mailbox |

The usable mailbox image is `0x5FF000` bytes. `sdk/components/kbsp/project_include.cmake` retargets `idf.py flash` of an app-role image to that mailbox. `idf.py erase-flash` is not retargeted. The catalogue cap is 8 MiB (kode-apps rule B6). A prboom-go text image on a P4 was reported around 529 KB (`giltal/RetroESP32-P4` `PSRAM_APP.md`). Three Chocolate trees with SDL and the Windows pieces left out should land in the low megabytes. If a real link exceeds the mailbox, `tools/kcli.py install --dst app` still fits the apps region. If it exceeds 8 MiB, split into a Doom-family image and a Heretic/Hexen image behind a Kconfig.

PSRAM is 32 MB at 250 MHz (`CONFIG_SPIRAM` and `CONFIG_SPIRAM_SPEED_250M` in `sdkconfig.board`). The CPU is 400 MHz dual-core RISC-V (`CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_400`). Internal RAM is the tight resource: the SDK's static ceiling in `sdk/tests/target/ram_budget.txt` is 368,520 bytes, and rule B8 stops at 417,000. Zone memory cannot live there.

This directory is the in-tree scaffold so the note and the code can be reviewed together. It is not registered in the kodeOS `CMakeLists.txt`. The catalogue home is a repository from `kode-app-template`, then a submodule in `kode-apps`. Building here points `EXTRA_COMPONENT_DIRS` at the sibling `sdk/`, which rule B3 rejects. That is deliberate.

The GBC scaffold on `cursor/gbc-emu-scaffold-cae5` is an unmerged app with no shared library. This branch is cut from `develop` and does not depend on it. The Active Object, the worker and the frame task follow that shape because it matches the SDK, not because the trees are linked.

## Engines

| Engine | Licence, as checked | Games | ESP32-class evidence | Embed |
|---|---|---|---|---|
| Chocolate Doom `a2dfd51` | GPL-2.0-or-later. File headers in `src/doom/d_main.c`, `src/heretic/d_main.c`, `src/hexen/h2_main.c` say "version 2 … or any later version". Hexen header also names Raven. DoomWiki lists the project as GPLv2+ | Doom, Doom II, Heretic, Hexen, and Strife as four programs. Software renderer | No P4 port of this tree found. The renderer is the portable part | The base. Drop SDL. Prefix Heretic and Hexen symbols |
| Crispy Doom | The project's README says the lineage is "released under the GPL-2+" | Doom and Doom II, limit-removing. Not Heretic or Hexen | A later upgrade for PWADs that break vanilla limits | After Chocolate plays |
| PrBoom+ | DoomWiki: GPLv2+. Confirm file headers before vendoring. Development stopped at 2.6.66 | Doom, Boom, MBF. Not Heretic or Hexen | retro-go's prboom is this family. See the next row | Not the multi-game base |
| prboom-go inside retro-go | retro-go `COPYING` is the GPL version 2 text. The "How to Apply" appendix is a sample, not a grant. The GBC note already classed this tree GPL-2.0 | Doom / Doom II via prboom | ESP32-class ports exist. RetroESP32-P4 loads it from PSRAM, which is not this boot model. Text+rodata about 529 KB | Do not vendor |
| doomgeneric (`ozkl/doomgeneric`) | `LICENSE` is the GPL version 2 text. `doomgeneric.c` has no "or later" line | Doom only, via a thin `DG_*` layer | `rear16/ESP-DOOM` asks for about 6.5 MB of PSRAM and a 6 MB zone | Pattern only. Do not vendor the wrapper |
| `id-Software/DOOM` | README.TXT, read 2026-11-13: "Licensed under the GNU General Public License 2.0." No "or later". Older revisions also carried the Doom Source License, which is not a free licence | Doom only | The historical tree | Do not vendor |
| `espressif/esp32-doom` | PrBoom, and Espressif calls it an unsupported proof of concept | Doom, a cut-down shareware WAD in flash | ESP32 with 4 MB PSRAM | Historical. Wrong boot model |
| `alexkid77/ESP32P4DOOM`, `relocsrl/scintix-p4-playing-doom` | Their trees were not adopted. Read them as measurements | Doom | P4, PPA scale, WAD copied into PSRAM. Scintix: renderer about 25 ms, CPU scale of 320x200 to 1024x600 about 24.5 ms, PPA scale about 9 ms, steady about 30 fps | Technique, not source |
| GZDoom | GPL, current trees are GPL-3-or-later in practice | All four, one binary | A hardware renderer and a C++ tree measured in tens of megabytes | Proof that one process can host all four. Wrong size for this chip |

ESP-IDF is Apache-2.0. GPL-2.0-only does not combine with Apache-2.0 or with the SDK's GPL-3. GPL-2.0-or-later can be conveyed as GPL-3, which combines with both. That is the same rule the GBC note used for gnuboy and Gambatte.

Nuked OPL3 is published as LGPL-2.1. LGPL-2.1-only does not combine with GPL-3. Do not vendor it unless the headers grant "or later" or LGPL-3. Chocolate's own OPL is inside the GPL-2.0-or-later tree and is the one to try.

## Display and the PPA

The panel is 410×502 RGB565. `kdrv_display.h` times one refresh at 16,829 µs, about 59.42 Hz. Doom's tic rate is 35 Hz. Present the latest finished frame on the panel clock and repeat frames. Do not chase 60 unique game frames.

`kdrv_display_back_buffer()` is documented as the buffer "a PPA scaling a game's frame" writes, then `kdrv_display_flush()` of that pointer flips with no extra copy. The buffer size to name is `KDRV_DISPLAY_FB_BYTES`. 410×502×2 is 411,640 bytes, 56 past a 64-byte cache line, and the PPA refuses an output whose size is not a whole number of lines. `kdrv_ppa.c` records that this exact miss shipped once.

The PPA SRM engine (`esp_driver_ppa`, IDF v6.1) scales, rotates by 0/90/180/270, mirrors, and converts among RGB565, RGB888, ARGB8888, YUV and GRAY8. Blend and fill are separate engines. Scale factors are truncated to a step of 1/16. The scaler is bilinear. There is no paletted mode: the IDF 6.0 `ppa_types.h` comment still says "TODO: Support CLUT to support L4/L8". GRAY8 is luminance, not Doom's colormap. 2D-DMA, which the PPA driver uses for some YUV conversions, does not expand a palette either.

So the path is: the renderer writes a 320×200 8-bit frame, the CPU expands `PLAYPAL` to RGB565, then something scales to the glass.

| Axis | Factor | Pixels | On a 1/16 step |
|---|---|---|---|
| width | 410/320 = 1.28125 | would be 410 | 1.25 → 400, or 1.3125 → 420 which does not fit |
| height for 4:3 | 300/200 = 1.5 | 300 | 1.5 is 24/16, exact |

400×300 is 4:3, which is how 320×200 non-square pixels are meant to look. Centering would start at x = 5. Five pixels is 10 bytes, not a 16-byte row start, so the window starts at x = 8, y = 101. The CPU nearest scaler in `doom_video.c` writes that window. `doom_video_scale_ppa()` returns false and logs the config the next step will submit on its own SRM client.

`kdrv_ppa_prepare()` is the wrong client for this. It accepts scale 1 or 2 only, into its own buffer, and `display_ao.c` uses it for the screen mirror. The IDF driver allows another SRM client from the frame task. kgui's slide comment says its own PPA path is off; the glass composite there is software.

A CPU nearest scale of 320×200 to 400×300 is about a fifth of the pixels Scintix scaled to 1024×600 in 24.5 ms, so on the order of 5 ms. That fits a 28.6 ms tic if the renderer is near the 25 ms Scintix measured. Bilinear PPA is softer and frees the core. Measure both. The scaffold ships nearest so the picture does not depend on the PPA.

## Performance and memory

| Use | Where |
|---|---|
| Zone, about 6 MB (ESP-DOOM's figure for a doomgeneric port) | PSRAM. It cannot live in the 640 KB internal RAM |
| IWAD bytes | PSRAM when `wad bytes + 8 MB` still fits after the panel buffers. Otherwise a lump cache and the file stays on the card |
| 320×200 paletted frame, 64 KB, and the RGB565 expand, 128 KB | PSRAM until a profile says the column loop should move |
| Two panel buffers, about 0.79 MB (`KDRV_DISPLAY_FB_BYTES` × 2) | PSRAM, owned by the display driver |
| Codec ring, 200 ms | PSRAM, owned by the codec |

`kode_file_read_bytes()` stops at 4 MiB (`KODE_FILE_READ_MAX` is 4,194,304). Shareware `doom1.wad` v1.9 is 4,196,024 bytes. Every retail IWAD is larger. The scanner and the future loader use `kode_file_read_at()`.

Published lengths used when the name and the lumps do not decide: Doom shareware 4,196,024; Ultimate Doom 12,408,292; Doom II 14,604,584; Plutonia 17,420,824; TNT 18,195,736; Hexen 20,083,672. Legacy of Rust `id1.wad` is about 35 MB (DoomWiki lists 34,793,603, 34,816,611 and 34,816,890). Copying that file plus a 6 MB zone into 32 MB of PSRAM does not fit. Stream `id1.wad`, or refuse to copy it and keep a lump cache.

Scintix's lesson on WAD access: random seeks through a WAD on flash storage took about 40 s at startup; one sequential copy of a 4 MB shareware WAD into PSRAM took about 1.3 s. Prefer the copy for IWADs that fit. Do not memory-map the card.

The frame task is pinned to core 1 at `KODE_PRIO_USER_GUI`. The mixer, once it exists, belongs on the other core. A Reddit report of 42–44 fps on an ESP32-S3 is one post, not a budget. An ESP32-S2 prboom-go report of 5–15 fps is a smaller chip.

## Controls

| Role | Control | Why |
|---|---|---|
| Button 1, fire | Top button, level | Chaingun and plasma are holds. `AO_Buttons` publishes `BUTTON_HOLD_SIG` for the top button after 600 ms and that signal is not home. The app ignores it and keeps sampling the level, so a long burst stays fire. The top button's own double-press (`KODE_BUTTON_DOUBLE_MS`, 800 ms) is also ignored: double-tap is a touch gesture |
| Button 2, use / open | Bottom button, rising edge | Use is a press. Holding bottom publishes home at 600 ms and the board resets 200 ms later unless the app defers. The hold is not given a second meaning. The frames before the hold still see the press edge, so a door opens on the way home. That is acceptable. Putting fire on this button would exit the game every time a weapon was held past 600 ms |
| Move | D-pad up / down | Forward and back |
| Turn | D-pad left / right | Turn, unless strafe is latched |
| Strafe | A row in the weapon overlay | Latches left/right to strafe, then returns to the game. The D-pad cannot turn and strafe at once. It is offered in every game |
| Weapon | Single tap | Pause. D-pad moves. Top or bottom confirms the row. A tap on a row confirms it. A weapon, jump, inventory-use or strafe row applies and unpauses. Inventory prev/next stay in the list so a player can browse |
| Settings | Double tap | Pause. Sound, music, always-run, resume, and exit to the game list. Exit stops the engine and returns to the select screen |
| Heretic inventory | Rows in the weapon overlay | Prev, use, next. Fly is the wings artifact, used from this list. Vanilla Heretic has no jump |
| Hexen jump | A row in the weapon overlay | One jump impulse, then the game resumes. Look can wait; vanilla look is not on the D-pad |
| Home | Hold bottom | kodeOS. Not remapped |
| IMU | Off | `kode_imu_read()` exists. Yaw-as-turn can be a later settings row. It stays off until someone asks, so the D-pad remains the turn control |

Tap timing: `DOOM_TAP_DOUBLE_MS` is 300. A finger-up that moved less than 24 pixels (`DOOM_TAP_SLOP_PX`) starts the wait. A second tap whose finger-down lands inside those 300 ms is the double tap, even if the second finger-up is a little later. If 300 ms pass with no second finger-down, the gesture is a single tap and the weapon list opens. The game keeps running during the wait. 300 ms is about 18 panel frames at 59.42 Hz and about 10 Doom tics. It is shorter than the top button's 800 ms double-press because that timer belongs to a different control and 800 ms would make every weapon list feel late.

The select screen does not use this wait. A tap on a logo launches immediately. Button 1 launches the highlighted logo. Button 2 rescans the card.

`doom_input_gestures(false)` while a list is up, so a tap inside a list cannot open a second list. It also clears a tap that was still waiting.

## The select screen

The scan reads `sd/doom` and `sd/doom/pwads`, case-insensitive `*.wad` names, the 12-byte magic, up to 8192 directory entries, and the file size.

A button is shown only for a file `doom_wad_identify()` marks `show`. Hidden, and logged: Chex, Hacx, Strife, `extras.wad`, `id1-mus.wad`, `id1-res.wad`, `id1-tex.wad`, `id1-weap.wad`, `id24res.wad`, and a PWAD that is not a known campaign. A PWAD that merely borrowed the name `doom2.wad` is not treated as Doom II.

| File | What it is | Button |
|---|---|---|
| `doom1.wad` | Shareware Doom | Yes, alone |
| `doom.wad` | Ultimate Doom if `E4M1` is present or no map lump says otherwise. Registered Doom if `E2M1`/`E3M1` and not `E4M1` | Yes, alone |
| `doom2.wad`, `doom2f.wad` | Doom II | Yes, alone. The preferred base for add-ons |
| `tnt.wad`, `plutonia.wad` | Final Doom | Yes, alone. Not the base for Legacy of Rust |
| `freedoom1.wad`, `freedoom2.wad`, `freedm.wad` | Freedoom, BSD-3-Clause. Phase 1 counts as an Ultimate Doom base. Phase 2 counts as a Doom II base | Yes |
| `heretic1.wad`, `heretic.wad` | Heretic. `EXTENDED` retitles the registered file | Yes |
| `hexen.wad` | Hexen. `BEHAVIOR` plus `MAP01` names a renamed file | Yes |
| `sigil.wad`, `sigil_compat.wad` | SIGIL, a PWAD. `E5M1` names a renamed file | Yes, and it needs Ultimate Doom or Freedoom Phase 1 |
| `sigil2.wad`, `sigil_ii.wad`, `sigilii.wad` | SIGIL II. `E6M1` names a renamed file | Yes, same base as SIGIL |
| `id1.wad` | Legacy of Rust. Also the three published sizes if the name was changed | Yes, and it needs Doom II or Freedoom Phase 2. The engine stub stores the name and does not claim to play ID24 |
| `nerve.wad` | No Rest for the Living, from the 2024 re-release and from BFG | Yes, needs Doom II |
| `masterlevels.wad` | Master Levels | Yes, needs Doom II |
| `iddm1.wad` | id deathmatch pack from the 2024 re-release | Yes, needs Doom II |

Layout, in `doom_layout.c`: zero games draws no button. One game is one large button, still on this screen, because a tap is the launch and an automatic start would hide a bad detection. Two games stack. Three or more are two columns, four visible, scrolled by a row. The D-pad moves the highlight. A tap or the top button launches. An add-on with no base on the card stays on the screen and says `NEEDS DOOM.WAD` or `NEEDS DOOM2.WAD`.

No card: "NO CARD", and where the files go. No IWAD and only loose files: "NO IWAD". A card with no WAD: "NO WADS" and the names to copy into `sd/doom`.

The art well shows the lump name when the directory contained it (`M_DOOM` for Doom, `M_HTIC` for Heretic, otherwise `TITLEPIC` or `TITLE`). Decoding Doom's column posts and `PLAYPAL` into that rectangle is the step after the engine is linked. A drawn copy of the commercial logo would still be their mark, so the plate stays text.

## Audio

The codec is an ES8311 at 48 kHz (`KDRV_ES8311_RATE_HZ`). `kode_audio_play_samples()` blocks for the whole buffer and is the wrong call. `kdrv_es8311_stream_write()` is the writer's call after `AO_Audio` has opened the stream. The scaffold refuses the open and drops samples.

SFX first: up to eight voices of about 11,025 Hz 8-bit, mixed to 48 kHz mono s16. That is cheap beside a 25 ms renderer.

Music second, and switchable off: MUS to MIDI to the OPL emulator already in Chocolate, on the other core. A waveform synth is a fallback and will not sound like a Sound Blaster. Soundfonts are not feasible. Nuked OPL3 stays out until its licence is shown to combine, and it is too heavy anyway.

## Saves

`sd/doom/config.cfg` is one line, `last=<file>`. The worker writes it when a game is launched. Slots, once the engine can snapshot, are `sd/doom/saves/<stem>/slot0.dsg` through `slot5.dsg`. `doom_save_slot_path()` builds that path and nothing writes a slot yet.

## Risks

- Speed is unmeasured here. Slaughter maps and Hexen hubs are the scenes that can miss a tic. Measure before adding interpolation.
- Heretic and Hexen symbol collisions are real. A naive link of the three trees fails. The prefix step is part of importing the engine, not a reason to ship three apps on the first try.
- Legacy of Rust and, on some maps, No Rest for the Living need a limit-removing or ID24 engine. Listing them is honest. Claiming Chocolate plays them is not.
- `id1.wad` does not fit in PSRAM beside a 6 MB zone. Plan on streaming it.
- The PPA is bilinear and will soften pixels. Nearest is the default until a side-by-side on hardware says otherwise.
- The 300 ms tap wait is a feel choice. It can move after someone plays with it. It should stay well under the 600 ms home hold so a tap is not a hold.
- Catalogue rules B1 and B3 fail on purpose for this in-tree build. They pass after the app is split out and pinned to a release tag.
- Rule C3 will warn on `kdrv_display.h`. That warning is the cost of a full-rate present.
- `ram_budget.txt` is unchecked against a link.
- No commercial WAD, SIGIL file, or Legacy of Rust file is in this tree, and none should be added.

## What the scaffold does

It builds as `project(doom)`, role app, radio off. It scans the card, names the games above, and draws a button for each one it will show. It resolves fire, use, and the 300 ms tap window. Weapon and settings lists pause the stand-in. The stand-in is a flat horizon, not a renderer. Chocolate Doom is pinned and not compiled.

## Sources

- This tree: `sdk/components/kbsp/partitions.csv`, `sdk/components/kbsp/sdkconfig.board`, `sdk/components/kdrv/display/include/kdrv_display.h`, `sdk/components/kdrv/mirror/src/kdrv_ppa.c`, `sdk/components/kdrv/audio/include/kdrv_es8311.h`, `sdk/components/kqp/include/kqp_config.h` (`KODE_BUTTON_HOLD_MS` 600, `KODE_BUTTON_DOUBLE_MS` 800, `KODE_SYS_HOME_GRACE_MS` 200), `sdk/components/kaos/boot/src/boot_ao.c`, `sdk/components/kaos/buttons/src/buttons_ao.c`.
- Chocolate Doom `a2dfd51`: `COPYING.md`, `src/d_iwad.c`, `src/doom/d_main.c`, `src/heretic/d_main.c`, `src/hexen/h2_main.c`.
- `id-Software/DOOM` `README.TXT` (the "GPL 2.0" line).
- `ducalex/retro-go` `COPYING`. `ozkl/doomgeneric` `LICENSE` and `doomgeneric.c`.
- ESP-IDF v6.1 PPA guide and `ppa_types.h` (SRM modes, 1/16 scale, CLUT still a TODO).
- Scintix P4 Doom README and the Hackaday log of 2026-08-25 (30 fps, PPA scale).
- DoomWiki: Legacy of Rust, `id1.wad` sizes, Doom + Doom II (the 2024 file list: `doom.wad`, `doom2.wad`, `tnt.wad`, `plutonia.wad`, `sigil.wad`, `sigil2.wad`, `id1.wad`, `nerve.wad`, `masterlevels.wad`, `iddm1.wad`, and the resource packs).
- `rear16/ESP-DOOM` (about 6.5 MB PSRAM, 6 MB zone). RetroESP32-P4 `PSRAM_APP.md` (prboom-go size, and a PSRAM app loader this board does not use).
