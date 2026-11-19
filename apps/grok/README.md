# Grok

A chat app for the Kode Dot. This directory is a scaffold: it builds as its own firmware, reads a config file from the microSD card, and can POST one streamed chat turn. It does not upload speech yet. The research note is [FEASIBILITY.md](FEASIBILITY.md). Licences are in [NOTICE.md](NOTICE.md).

The long-term home of this app is its own repository, made from [kode-app-template](https://github.com/kodediy/kode-app-template), with `project(grok)`. The catalogue ([kode-apps](https://github.com/kodediy/kode-apps)) only accepts that shape. It is kept in this fork so the note and the code can be reviewed together. It is not part of the kodeOS link.

## What an app is

An app is a separate firmware. kodeOS stores it in the `apps` region of flash and the bootloader maps and runs it in place. It is not loaded into PSRAM from the microSD card. The card holds `sd/grok/config.txt`. [FEASIBILITY.md](FEASIBILITY.md) has the partition table and the correction of the "loaded into PSRAM" picture.

## Layout

| Path | What it is |
|---|---|
| `main/app_main.c`, `main/app_ao.c` | `kode_init()`, then one Active Object and one worker |
| `main/grok_config.c` | The card file. No SDK calls |
| `main/grok_request.c` | The Chat Completions JSON body |
| `main/grok_sse.c` | The streaming parser |
| `main/grok_http.c` | `esp_http_client` POST, bearer, certificate bundle |
| `main/grok_client.c` | One turn: body, POST, collect the answer |
| `main/grok_net.c` | `kode_wifi_is_connected` / `kode_wifi_connect` |
| `main/grok_ui.c` | Status, canned prompt, transcript lines |
| `main/grok_audio.c` | Push-to-talk record, and playback for later TTS |
| `main/grok_prompts.c` | Three prompts. The keyboard is a sentence |
| `host/test_grok.c` | Host test. No key, no network |
| `host/samples/` | Recorded SSE bodies |
| `relay/` | Optional LAN process that holds the key |
| `config.example.txt` | What to copy onto the card. The key line is empty |

## Requirements

- ESP-IDF **v6.1**, the version kodeOS is built with. Older than 6.0 does not support this chip properly.
- The `sdk/` submodule checked out at the KodeOS root (`git submodule update --init sdk`). This project points at that checkout. It matches the headers [FEASIBILITY.md](FEASIBILITY.md) was written from.
- A Kode Dot with kodeOS already installed, for anything past the compile. The app does not replace kodeOS.

## Build

From this directory, with IDF exported:

```bash
. "$HOME/esp/esp-idf/export.sh"
idf.py set-target esp32p4
idf.py build
```

The image is `build/grok.bin`.

A build with ESP-IDF v6.1 against this tree's SDK produced `build/grok.bin` of `0x22cfd0` bytes, about 2.17 MB. The apps region still had about 81% free. The image fits the mailbox. That build's `idf.py flash` line writes it at `0x1a00000`. It was not flashed: there was no board.

`set-target` is only needed on a fresh tree. `idf.py build` is enough after that.

The host test does not need IDF. Run it from this directory:

```bash
gcc -std=c11 -Wall -Wextra -Werror -I main -o /tmp/grok_test \
    host/test_grok.c main/grok_config.c main/grok_request.c \
    main/grok_sse.c main/grok_transcript.c main/grok_prompts.c
/tmp/grok_test
```

It prints `OK grok`. It reads `host/samples/`.

## Install and run

From this directory, with kodeOS already on the Dot:

```bash
idf.py -p PORT flash
```

`sdk/components/kbsp/project_include.cmake` retargets that command when `CONFIG_KODEOS_BOOT_ROLE_APP` is set. It writes `build/grok.bin` to the mailbox (`app_pending`) and nothing else. On this tree the mailbox is `0x1A00000`. kodeOS adopts the image at the next boot, installs it in the apps region and runs it. The build checks the image against the apps region and refuses `idf.py flash` if the file is bigger than the mailbox (`0x5FF000`).

**Do not `idf.py erase-flash`.** That is not the retargeted flash target. It erases the whole chip, including kodeOS and every installed app.

The same image over the console, from the KodeOS root:

```bash
python3 tools/kcli.py -p PORT install apps/grok/build/grok.bin --dst app
```

The boot from the mailbox is sticky: opening the serial port starts the app again. Hold the bottom button and left together for about a second to return to kodeOS. The app stays installed. Holding bottom alone, for the SDK's home gesture, also returns.

Put the config on the card yourself. Nothing in this repository is an API key.

```text
sd/grok/config.txt
```

Copy [config.example.txt](config.example.txt) and set `api_key` on the card. Join Wi-Fi once from the console with `wifi_join`. The app turns autojoin on, so that network comes back. `ssid` and `password` in the file are optional and are not logged.

With no card file the glass says so. With no address it says `No network.` Left and right change the canned prompt. Bottom sends it. Top, held, records for up to eight seconds and then says speech upload is not wired. The answer, when the POST runs, is cut into lines of 119 characters, eight lines at most.

`idf.py -p PORT monitor` shows `grok.ao` and `grok.http`. The log names the URL and the HTTP status. It does not name the key.

## Controls

| | |
|---|---|
| Pad up / down | Scroll the transcript |
| Pad left / right | The previous or next canned prompt |
| Bottom, tap | Send that prompt |
| Bottom, hold | home to kodeOS. Not remapped |
| Top, hold | Push-to-talk. Release ends the recording |

## Moving it to the catalogue

1. Create `kode-app-grok` from `kode-app-template`.
2. Copy `main/`, `appdata/`, `host/`, `relay/`, `NOTICE.md`, `FEASIBILITY.md` and this README.
3. Delete the sibling-SDK block in `CMakeLists.txt`. Rename `main/idf_component.catalog.yml` to `main/idf_component.yml` so the component manager fetches the SDK. Point `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME` at `managed_components/kodediy__kbsp/partitions.csv`.
4. Change the `version:` line to a published kode-sdk tag. Rule B1 wants the newest release or the one before it. `v0.1.1` was the newest tag on 2026-11-13 and is not the commit this fork's submodule is on.
5. Run `tools/check_app.py` from a kode-apps checkout. `ram_budget.txt` here is a draft of the SDK's list and has not been checked with `tools/check_ram.sh`.

## Roadmap

The ordered list is in [FEASIBILITY.md](FEASIBILITY.md). Short form: confirm the glass on a Dot, send one real turn, redraw as tokens arrive, then wire `/v1/stt` and `/v1/tts` at 48 kHz mono. Use the relay when the key should not be on the card.

## Known gaps

- Speech is recorded and not uploaded. Nothing is played back yet.
- The glass updates when the turn ends, not on each token.
- Eight lines of 119 characters. Older lines drop off.
- No on-screen keyboard.
- `ram_budget.txt` is unchecked against a link. The image did link: `0x22cfd0` bytes.
- No screenshot. A catalogue review wants one from a Dot.
- Not flashed. There was no board.
