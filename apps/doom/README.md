# Doom

A Doom, Doom II, Heretic and Hexen app for the Kode Dot. It builds as its own firmware, scans the microSD card for WADs, and runs Chocolate Doom. Heretic and Hexen are in the same image. The research note is [FEASIBILITY.md](FEASIBILITY.md). Licences are in [NOTICE.md](NOTICE.md).

The long-term home of this app is its own repository, made from [kode-app-template](https://github.com/kodediy/kode-app-template), with `project(doom)`. The catalogue ([kode-apps](https://github.com/kodediy/kode-apps)) only accepts that shape. It is kept in this fork so the note and the code can be reviewed together. It is not part of the kodeOS link. The branch is cut from `develop` and does not depend on the Game Boy Color scaffold.

## What an app is

An app is a separate firmware. kodeOS stores it in the `apps` region of flash and the bootloader maps and runs it in place. It is not loaded into PSRAM from the microSD card. The card holds WADs, saves and config. [FEASIBILITY.md](FEASIBILITY.md) has the partition table.

## Layout

| Path | What it is |
|---|---|
| `main/app_main.c`, `main/app_ao.c` | `kode_init()`, then one Active Object and a worker |
| `main/doom_frame.c` | The frame task on core 1 |
| `main/doom_wad.c`, `main/doom_layout.c` | WAD identification and button geometry. No SDK calls |
| `main/doom_file.c` | Scan `sd/doom` and `sd/doom/pwads` |
| `main/doom_game_select.c` | Logo buttons for the games that were found |
| `main/doom_menu.c` | Weapon overlay (single tap) and settings (double tap) |
| `main/doom_input.c` | Top is fire, bottom is use, 300 ms tap window |
| `main/doom_video.c` | Nearest scale into the panel. PPA path is stubbed |
| `main/doom_engine.c` | Starts Chocolate Doom, Heretic or Hexen on core 0 |
| `port/` | Video, input, timer, silent sound, and the 6 MB PSRAM zone |
| `main/doom_audio.c`, `main/doom_save.c` | Speaker refused; config line on the card |
| `host/test_doom.c` | Host checks, including a directory of real WADs |
| `host/engine_run.c` | Headless engine run and frame times |
| `third_party/chocolate-doom/` | Chocolate Doom at the pinned commit, GPL-2.0-or-later |

## Controls

| | |
|---|---|
| Top, held | Fire. Button 1. A long hold stays fire. It is not the home gesture |
| Bottom, tap | Use / open. Button 2. On the game list, this rescans the card |
| Bottom, hold | Home to kodeOS, after 600 ms plus a 200 ms grace. Not remapped |
| Pad | Move and turn. Left and right strafe after the strafe row is chosen |
| Tap the glass | Pause, weapon list. A weapon is applied as soon as it is chosen |
| Tap twice within 300 ms | Pause, settings. "EXIT TO MENU" returns to the game list |

Heretic inventory and Hexen jump are rows in the weapon list. Strafe is a row in every game, because the pad cannot turn and strafe together.

## WADs

Nothing in this repository is a WAD. Copy the files you own onto the card yourself. Shareware `doom1.wad` and Freedoom (BSD-3-Clause) are fine for a test. Do not commit a commercial WAD, SIGIL, or Legacy of Rust, and do not make the build download one.

```text
sd/doom/doom.wad
sd/doom/doom2.wad
sd/doom/heretic.wad
sd/doom/hexen.wad
sd/doom/tnt.wad
sd/doom/plutonia.wad
sd/doom/sigil.wad          needs doom.wad (Ultimate) or freedoom1.wad
sd/doom/sigil2.wad         same base as SIGIL
sd/doom/id1.wad            Legacy of Rust. Needs doom2.wad. MBF21/ID24, not played yet
sd/doom/nerve.wad          No Rest for the Living. Needs doom2.wad
sd/doom/masterlevels.wad
sd/doom/pwads/             more add-ons, same rules
sd/doom/saves/             created on boot
sd/doom/config.cfg         last=<file>, written when a game is launched
```

`id1-res.wad`, `id1-tex.wad`, `id1-weap.wad`, `id1-mus.wad`, `id24res.wad` and `extras.wad` are recognised and not shown as buttons. A button appears only for a game that was found. One game is still this screen, with one large button: launch is a tap, so the app does not start a file by itself.

SIGIL is a PWAD on top of Ultimate Doom. Legacy of Rust is `id1.wad` on top of Doom II. The scaffold will pair them when the base file is in the same folders. Without the base it stays on the list and says which file it needs.

### Host check, including a directory of your own files

From `apps/doom`, no ESP-IDF:

```bash
gcc -std=c11 -Wall -Wextra -Werror -I main -o /tmp/doom_test \
    host/test_doom.c main/doom_wad.c main/doom_layout.c
/tmp/doom_test
```

It prints `OK doom`. The synthetic directory it builds is deleted afterwards.

The engine runner links the same three trees and plays a WAD until 200 tics:

```bash
make -C host engine
DOOM_WAD_DIR=/path/to/your/wads DOOM_TICS=200 host/build/doom_engine
```

`DOOM_WAD_DIR` defaults to `/tmp/doom-wads`. It prefers `doom2.wad` or `freedoom2.wad`. A run against Doom II reported `frames 236 gametic 200 wall_avg 28.44 ms wall_max 30.00 ms busy_avg 2.81 ms busy_max 8.00 ms`. Wall time includes the 35 Hz wait. Busy time is the work with that wait removed. Do not commit the WAD.

To identify a folder of WADs you already have (nothing is copied into the repo):

```bash
/tmp/doom_test /path/to/your/wads
# or
DOOM_WAD_DIR=/path/to/your/wads /tmp/doom_test
```

Each `*.wad` is printed with the name the scanner chose. A known campaign file that comes back as the wrong game fails the test. `doom.wad` may be "ULTIMATE DOOM" or "DOOM" when the lumps say it is the registered three-episode file. Other files are printed and do not fail the test.

## Requirements

- ESP-IDF **v6.1**, target **esp32p4**. A build on 2026-11-20 produced `build/doom.bin` of `0x1d74d0` bytes (1,930,448). The apps region is `0xBC0000` bytes and the size check left `0x9e8b30` bytes free (84%). The same image fits the mailbox (`0x5FF000`).
- The `sdk/` submodule at the KodeOS root (`git submodule update --init sdk`).

## Build

From this directory, with IDF v6.1 exported:

```bash
git submodule update --init sdk          # from the KodeOS root, once
cd apps/doom
. "$HOME/esp/esp-idf/export.sh"
idf.py set-target esp32p4
idf.py build
```

`set-target` is only needed on a fresh tree. The image is `build/doom.bin`.

If `./install.sh esp32p4` stops while creating the Python environment, install `python3-venv` and run it again. That failure is an environment problem, not a fault in this project.

## Install and run

From this directory, with kodeOS already on the Dot:

```bash
idf.py -p PORT flash
```

`sdk/components/kbsp/project_include.cmake` retargets that command when `CONFIG_KODEOS_BOOT_ROLE_APP` is set. It writes the image to the mailbox at `0x1A00000` and nothing else. kodeOS adopts it at the next boot.

**Do not `idf.py erase-flash`.** That erases the whole chip, including kodeOS.

The same image over the console, from the KodeOS root:

```bash
python3 tools/kcli.py -p PORT install apps/doom/build/doom.bin --dst app
```

Hold the bottom button to return to kodeOS. The app stays installed. Opening the serial port starts a mailbox image again; hold bottom and left together for about a second to force kodeOS.

## Moving it to the catalogue

1. Create `kode-app-doom` from `kode-app-template`.
2. Copy `main/`, `appdata/`, `host/`, `third_party/`, `NOTICE.md`, `FEASIBILITY.md` and this README.
3. Delete the sibling-SDK block in `CMakeLists.txt`. Rename `main/idf_component.catalog.yml` to `main/idf_component.yml`. Point `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME` at `managed_components/kodediy__kbsp/partitions.csv`.
4. Change the `version:` line to a published kode-sdk tag. `v0.1.1` was the newest tag on 2026-11-13 and is not the commit this fork's submodule is on.
5. Run `tools/check_app.py` from a kode-apps checkout.

## Roadmap

1. Install the image on a Dot. Confirm the idle list, a tap on one synthetic-free card message, top and bottom, and that holding bottom returns home.
2. On a card with one IWAD, confirm the screen still shows one large button and that a tap is what launches. With SIGIL and `doom.wad` together, confirm one tap fills both names. With SIGIL alone, confirm the button says it needs `doom.wad`.
3. Point `host/test_doom.c` at a private directory of `doom.wad`, `doom2.wad`, `heretic.wad`, `hexen.wad`, `tnt.wad`, `plutonia.wad`, `sigil.wad`, `sigil2.wad` and `id1.wad`. Do not copy those files into git.
4. Time a level on a Dot. The host busy time is a few milliseconds; the panel and the PSRAM zone are not in that number.
5. Move `states` and `mobjinfo` out of internal RAM if a catalogue submission has to stay under 417,000 bytes. The image already fits the apps region and the mailbox.
6. Decode `M_DOOM` / `TITLEPIC` / `M_HTIC` / `TITLE` from the WAD into the button well. Keep the text plate when the lump is absent.
7. Expand the 8-bit frame through `PLAYPAL` and time `doom_video_scale_nearest` against a real PPA SRM client. Keep nearest if it is both fast enough and sharper.
8. Open the speaker stream and mix sound effects to 48 kHz mono. Music, via Chocolate's OPL, comes after and stays switchable.
9. Copy an IWAD into PSRAM when it fits. Stream `id1.wad`. Write the six save slots from the worker.
10. Split the app out and open the kode-apps pull request.

## Known gaps

- Sound is silent. The speaker stream stays closed. Music is not mixed.
- Title art is the lump's name on a plate, not the decoded picture.
- The view is still the CPU nearest scale. The PPA path is unchanged.
- Legacy of Rust is listed and not playable on this engine pin.
- Heretic and Hexen are linked (`heretic_D_DoomMain`, `hexen_D_DoomMain`) and were not run here: the test folder had no `heretic.wad` or `hexen.wad`.
- `idf.py build` for esp32p4 with ESP-IDF v6.1 completes. The image is 1,930,448 bytes. Static internal RAM (`DIRAM`) is 495,498 of 655,360 bytes (159,862 left). That is over the 417,000 byte catalogue note because `states` and `mobjinfo` stay in internal `.data`. Engine BSS, 1,180,496 bytes, is in PSRAM. At launch the zone takes another 6 MB of PSRAM, and the 320x200 buffers take about 450 KB more.
- `id1.wad` (about 35 MB) is streamed from the card. Chocolate Doom does not play it correctly.
- `ram_budget.txt` is still the draft ceiling. The linked image is larger than that draft.
- No screenshot. A catalogue submission needs one from a Dot. This tree was not flashed to hardware.
