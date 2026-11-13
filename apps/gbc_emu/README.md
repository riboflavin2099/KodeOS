# GBC

A Game Boy Color emulator app for the Kode Dot. This directory is a scaffold: it builds as its own firmware, loads a ROM from the microSD card, and draws frames. It does not run the guest CPU yet. The research note is [FEASIBILITY.md](FEASIBILITY.md). Licences are in [NOTICE.md](NOTICE.md).

The long-term home of this app is its own repository, made from [kode-app-template](https://github.com/kodediy/kode-app-template), with `project(gbc_emu)`. The catalogue ([kode-apps](https://github.com/kodediy/kode-apps)) only accepts that shape. It is kept in this fork so the note and the code can be reviewed together. It is not part of the kodeOS link.

## What an app is

An app is a separate firmware. kodeOS stores it in the `apps` region of flash and the bootloader maps and runs it in place. It is not loaded into PSRAM from the microSD card. The card holds ROMs and saves. [FEASIBILITY.md](FEASIBILITY.md) has the partition table and the correction of the "loaded into PSRAM" picture.

## Layout

| Path | What it is |
|---|---|
| `main/app_main.c`, `main/app_ao.c` | `kode_init()`, then one Active Object |
| `main/gbc_frame.c` | The frame task on core 1. It may block on the panel; the Active Object may not |
| `main/gbc_display.c` | 2× blit into the panel back buffer. Includes `kdrv_display.h` (catalogue rule C3 warns on that) |
| `main/gbc_input.c` | Pad to the D-pad, top to A, bottom to B, two touch zones for Start and Select |
| `main/gbc_audio.c` | Refuses the speaker stream and drops samples |
| `main/gbc_rom.c`, `main/gbc_save.c` | `sd/gbc/*.gb` and `*.gbc`, saves under `sd/gbc/saves/` |
| `main/gbc_header.c` | Cartridge header parser. No SDK calls |
| `main/gbc_core.c` | Placeholder. Paints a pattern |
| `third_party/walnut-cgb/` | Walnut-CGB and minigb_apu, MIT, not in the link yet |
| `host/test_header.c` | Host test for the parser |

## Requirements

- ESP-IDF **v6.1**, the version kodeOS is built with. Older than 6.0 does not support this chip properly.
- The `sdk/` submodule checked out at the KodeOS root (`git submodule update --init sdk`). This project points at that checkout and turns the component manager off, so it matches the headers `FEASIBILITY.md` was written from.
- A Kode Dot with kodeOS already installed, for anything past the compile. The app does not replace kodeOS.

## Build

From this directory, with IDF exported:

```bash
. "$HOME/esp/esp-idf/export.sh"
idf.py set-target esp32p4
idf.py build
```

The image is `build/gbc_emu.bin`. A build with ESP-IDF v6.1 against this tree's SDK produced an image of `0xafd50` bytes, about 703 KB, which fits the mailbox.

`set-target` is only needed on a fresh tree. `idf.py build` is enough after that.

The header test does not need IDF:

```bash
gcc -std=c11 -Wall -Wextra -Werror -I main -o /tmp/gbc_test_header host/test_header.c main/gbc_header.c
/tmp/gbc_test_header
```

It prints `OK header`.

## Install and run

From this directory, with kodeOS already on the Dot:

```bash
idf.py -p PORT flash
```

`sdk/components/kbsp/project_include.cmake` retargets that command when `CONFIG_KODEOS_BOOT_ROLE_APP` is set. It writes `build/gbc_emu.bin` to the mailbox (`app_pending`) and nothing else. On this tree the mailbox is `0x1A00000`. kodeOS adopts the image at the next boot, installs it in the apps region and runs it. The build checks the image against the apps region and refuses `idf.py flash` if the file is bigger than the mailbox (`0x5FF000`). A larger image can still be installed over the console.

**Do not `idf.py erase-flash`.** That is not the retargeted flash target. It erases the whole chip, including kodeOS and every installed app.

The same image over the console, from the KodeOS root:

```bash
python3 tools/kcli.py -p PORT install apps/gbc_emu/build/gbc_emu.bin --dst app
```

The boot from the mailbox is sticky: opening the serial port starts the app again. Hold the bottom button and left together for about a second to return to kodeOS. The app stays installed. Holding bottom alone, for the SDK's home gesture, also returns.

The upstream template README currently says `write-flash` at `0x1C00000`, and says never to `idf.py flash`. Neither matches this SDK commit. `0x1C00000` is 2 MB into the mailbox. `idf.py flash` here is the mailbox write described above. Use the offset in `build/flasher_args.json` if you call esptool yourself. A project that does not link this `kbsp` still flashes over kodeOS's factory slot. Do not point a plain ESP-IDF app at this board and flash it.

Put ROMs on the card yourself. Nothing in this repository is a ROM.

```text
sd/gbc/your-game.gbc
sd/gbc/saves/your-game.sav    created when a save exists
sd/gbc/saves/your-game.rtc    8 little-endian bytes, Unix seconds
```

The scaffold loads the first `.gb` or `.gbc` in the first 24 names of `sd/gbc`. With no card or no ROM it draws the idle picture (dark green, a moving white border). With a ROM and no core yet it draws the stand-in (darker fill, amber border). Top (A) lights the top-left guest pixel. The two zones under the picture are Start and Select. They light while a finger is in them. They do not reach a core yet.

`idf.py -p PORT monitor` shows `gbc.ao` and `gbc.frame`. The frame task logs every 120 presents.

## Controls, once the core is linked

| | |
|---|---|
| Pad | D-pad |
| Top | A |
| Bottom, tap | B |
| Bottom, hold | home to kodeOS. Not remapped |
| Touch, left zone | Start |
| Touch, right zone | Select |

## Moving it to the catalogue

1. Create `kode-app-gbc_emu` from `kode-app-template`.
2. Copy `main/`, `appdata/`, `third_party/`, `host/`, `NOTICE.md`, `FEASIBILITY.md` and this README.
3. Delete the sibling-SDK block in `CMakeLists.txt`. Rename `main/idf_component.catalog.yml` to `main/idf_component.yml` so the component manager fetches the SDK. Point `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME` at `managed_components/kodediy__kbsp/partitions.csv`.
4. Change the `version:` line to a published kode-sdk tag. Rule B1 wants the newest release or the one before it. `v0.1.1` was the newest tag on 2026-11-13 and is not the commit this fork's submodule is on.
5. Run `tools/check_app.py` from a kode-apps checkout. `ram_budget.txt` here is a draft of the SDK's list and has not been checked with `tools/check_ram.sh`.

## Roadmap

1. Build this project with ESP-IDF v6.1 and install it with `kcli.py`. Confirm the idle picture moves, the touch zones light, top lights the corner pixel, and holding bottom returns to kodeOS. Capture one 410×502 screenshot with `kcli.py screenshot` for rule D9.
2. Include `third_party/walnut-cgb/walnut_cgb.h` from `gbc_core.c` only. Define the ROM and RAM callbacks over the PSRAM buffers. Call `gb_run_frame_dualfetch()` once per frame. Start with `WALNUT_GB_32BIT_ALIGNED`. Log milliseconds per frame on core 1 with audio still off, on a double-speed CGB scene and on a DMG scene.
3. If that stays inside the 16.8 ms panel period, wire `lcd_draw_line` to the RGB565 buffer (CGB palette indices and DMG shade-plus-layer are different). If it does not, try the other DMA macro, then binjgb. Do not switch to gnuboy unless its licence is shown to be GPL-2.0-or-later or the copyright holders relicense. See [FEASIBILITY.md](FEASIBILITY.md).
4. Open the speaker stream from AO_Audio and write 48 kHz mono frames from the frame task with `kdrv_es8311_stream_write()`. Build minigb_apu with `MINIGB_APU_AUDIO_FORMAT_S16SYS`. Resample its 32,768 Hz stereo, or rebuild it at 48 kHz. Keep the write off the Active Object.
5. Page the ROM list with `kode_file_list_from()` (24 names per page). Save from the worker on a timer and on the way home, using `gbc_save_store()`. Call Walnut's `gb_set_rtc()` with the clock `gbc_core_set_clock()` already stored. If `kode_time_unix()` is −1, say so in the log and keep the file's value.
6. Run a permissive homebrew and, once their licences have been read, dmg-acid2 and cgb-acid2. Do not add those files to the repository until the grant is in the commit message. Do not add a commercial ROM at all.
7. Split the app out as above and open the kode-apps pull request. Raise `PROJECT_VER` for every version that pull request publishes.

## Known gaps

- Guest instructions are not executed. The vendored core is not in the compile.
- The speaker is silent on purpose.
- Only the first page of `sd/gbc` is scanned.
- `ram_budget.txt` is unchecked against a link.
- No screenshot. Rule D9 needs one from a Dot.
- Tilt is not mapped. The D-pad is the D-pad.
