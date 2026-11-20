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

The image is `build/gbc_emu.bin`. A build with ESP-IDF v6.1 against this tree's SDK, with Walnut linked, produced an image of `0xbb920` bytes, about 750 KB, which fits the mailbox.

`set-target` is only needed on a fresh tree. `idf.py build` is enough after that.

The host tests do not need IDF. From `apps/gbc_emu/host`:

```bash
python3 gen_scroll_rom.py   # rewrites roms/scroll.gb; already committed
make run                    # Walnut on roms/scroll.gb, writes frame_30.ppm and frame_90.ppm
make run ROM=/path/to/your.gbc
make header                 # cartridge header parser
```

`make run` prints average and worst microseconds per guest frame on this machine. Those numbers are not an ESP32-P4 prediction. `roms/scroll.gb` is a DMG program written for this tree (see `roms/LICENSE`). It paints a stripe and scrolls it. Pass any other ROM path you are allowed to use. Do not commit a commercial ROM.

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

The app loads the first `.gb` or `.gbc` in the first 24 names of `sd/gbc` and runs it with Walnut-CGB. With no card or no ROM it draws the idle picture (dark green, a moving white border). Top is A, bottom is B, the pad is the D-pad, and the two zones under the picture are Start and Select. A header Walnut refuses draws the failure picture (dark red, white border) instead of the guest.

Every 120 presented frames the frame task logs, at info, one line from core 1:

```text
gbc.frame: emu last N us avg N us worst N us loop N us drops N frames N audio N psram lcd N gb N rom N ram N
```

`emu` is `gb_run_frame_dualfetch` plus the resampler, in microseconds. `loop` is that plus the panel flip. `drops` counts presents the panel did not accept. `psram` is 1 when that pointer is in external RAM. The speaker mix runs; playback stays off until `GBC_SPEAKER` is set, because opening the codec stream belongs to AO_Audio. Cart RAM is written back about every 300 guest frames.

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

Walnut is linked. `gb_run_frame_dualfetch` runs when a ROM is open, `lcd_draw_line` writes RGB565, the joypad is the existing pad, and minigb_apu is resampled to 48 kHz mono. What is left:

1. Flash with `idf.py -p PORT flash` and read the `gbc.frame` line. The budget is about 16.8 ms. If a double-speed CGB scene misses it, try the other Walnut DMA macro, then binjgb. Do not switch to gnuboy unless its licence is GPL-2.0-or-later or the copyright holders relicense.
2. Open the speaker stream from AO_Audio and pass the mono frames to `kdrv_es8311_stream_write` from the frame task. Set `GBC_SPEAKER` to 1 only after that open exists.
3. Page the ROM list with `kode_file_list_from()`. The save job already runs about every 300 frames; also save on the way home.
4. Run dmg-acid2 and cgb-acid2 once their licences have been read. Do not commit a commercial ROM.
5. Split the app out as above and open the kode-apps pull request. Raise `PROJECT_VER` for every version that pull request publishes. Capture a 410×502 screenshot for rule D9.

## Known gaps

- The speaker mix runs and is dropped. `GBC_SPEAKER` stays 0 until AO_Audio opens the 48 kHz stream.
- Only the first page of `sd/gbc` is scanned.
- `ram_budget.txt` is unchecked against a link. The `gb_s` context (about 50 KB) and the LCD buffer are allocated, not static.
- No screenshot from a Dot. Rule D9 needs one.
- Tilt is not mapped. The D-pad is the D-pad.
- Host frame times do not predict the P4. The on-device log is the measurement that does.
