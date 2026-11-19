# A Grok chat app on the Kode Dot

Research date: 2026-11-19. SDK commit in this tree: `1841cc8c13d4b2e3e4d7288b40cdac5de207a133`. Public API notes are from docs.x.ai fetched that day (models, pricing, rate limits, streaming, speech). Nothing in this note was checked by calling the live API: this environment has no key, and the scaffold's tests use recorded bodies.

## Verdict

A conversational app is feasible as a Kode Dot firmware. Text chat is the part that fits the board today. Voice is practical as push-to-talk through xAI's own speech HTTP APIs, not as a full-duplex call, and not as an on-device model. A useful language model does not run on the ESP32-P4.

The app should be its own ESP-IDF firmware, the shape of `kode-app-template`, installed into the apps region. It should not be linked into kodeOS. It links the SDK, so the firmware that is distributed is GPL-3.0-or-later. The catalogue category is `ai`.

The first client should speak **Chat Completions** (`POST /v1/chat/completions`, `stream: true`) to a configurable base URL. The default model is **`grok-4.3`**. The Responses API is what xAI tells new work to use, and the parser already accepts its text-delta events, but the wire format of Chat Completions is the smaller one to bring up on a microcontroller. Point the same client at a LAN relay or at Ollama by changing `base_url` and `model`. The key, when the Dot talks to xAI itself, lives in `sd/grok/config.txt` and nowhere in the image.

## What an app actually is

The picture "a firmware loaded from the microSD card into PSRAM" does not match this tree.

An app is a separate ESP-IDF firmware. kodeOS and the app do not run at the same time. kodeOS keeps installed apps in the `apps` flash region. To start one it writes a boot request and resets. The bootloader override in `bootloader_components/main/bootloader_start.c` checks the request and then calls `bootloader_utility_load_boot_image()`. The comment there is "the stock loader maps and jumps". The image is executed in place through the flash cache. It is not copied into PSRAM first.

`sdk/components/kbsp/partitions.csv` in this checkout:

| Region | Offset | Size | Role |
|---|---|---|---|
| `factory`, `ota_0`, `ota_1` | `0x20000` onward | 4 MB each | kodeOS slots. An app never writes `otadata` |
| `apps` | `0xC40000` | `0xBC0000` (12,320,768 bytes) | directory, boot request, installed images |
| `storage` | `0x1800000` | 2 MB | wear-levelled FAT. App icons live at `.kode/appsdata/<project>/` |
| `app_pending` | `0x1A00000` | `0x600000` (6 MB) | mailbox. Not a boot slot |

Images in `apps` start on a 64 KB boundary (`KAPPS_ALIGN` / `KAPPS_DATA_OFF` are `0x10000` in `kapps_fmt.h`). The mailbox's usable image is `0x5FF000` (`KAPPS_PENDING_INTENT_OFF`). `CONFIG_KODEOS_BOOT_ROLE_APP` makes the app refuse to confirm `otadata`, so a crash still comes home. Holding the bottom button and left together returns to kodeOS. Holding bottom alone is the SDK's home gesture. This app does not give that hold a meaning.

The microSD card holds data. For this app that is `sd/grok/config.txt` (the base URL, the model, the key). It is not where the running firmware is fetched from.

PSRAM is 32 MB (`CONFIG_SPIRAM`, `CONFIG_SPIRAM_MODE_HEX`, `CONFIG_SPIRAM_SPEED_250M` in `sdkconfig.board`). It is heap for the running firmware: the JSON body, the SSE parser, the reply, TLS buffers. The SDK's `tests/target/ram_budget.txt` calls the on-chip RAM 640 KB and caps a full static image at 368,520 bytes. A catalogue app may not pass 417,000 bytes without a written reason (kode-apps rule B8). Internal RAM is the tight resource. PSRAM is not.

`sdkconfig.board` sets `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_400`. The ESP32-P4 is a dual-core RISC-V. The app is built with ESP-IDF v6.1, which is the version kodeOS is built with.

This directory is not registered in the kodeOS `CMakeLists.txt`. kodeOS does not link apps in.

## How an app gets a network

Wi-Fi is not on the P4. It is on the ESP32-C5, over esp-hosted SDIO. The slave firmware is the image kodeOS ships from `kodediy/kode_slave_c5` (built by `tools/build_c5.sh` into the storage image). The owner on the P4 is AO_Radio. `CONFIG_KODEOS_RADIO` selects `ESP_HOSTED_ENABLED` (`sdk/components/kaos/Kconfig`). The board file leaves esp-hosted off, because a build that leaves it on claims the P4's one SDMMC controller before `app_main`, whether or not the app wanted a radio. An app that wants Wi-Fi turns `CONFIG_KODEOS_RADIO`, `CONFIG_KODEOS_WIFI` and `CONFIG_KODEOS_NET` on and does **not** force `ESP_HOSTED_ENABLED` off. This scaffold also sets `CONFIG_ESP_HOSTED_ENABLED=y` so the later defaults file wins over the board file, and `CONFIG_KODEOS_WIFI_AUTOJOIN=y`.

AO_Radio is what makes the card and the radio share that controller. Before the C5 link comes up it calls `kdrv_sd_set_hosted(true)` and asks the storage owner to unmount (`radio_ao.c`, `radio_begin_connect`). The card comes back on the hosted path. A read of `sd/grok/config.txt` during that unmount fails. The boot job reads once, waits up to about eight seconds for an address, and reads again if the first read found nothing. A join from the file, when `ssid` is set, happens after that, through `kode_wifi_connect()`.

What an app calls, from a worker, never from an Active Object step:

| Call | What it does |
|---|---|
| `kode_wifi_is_connected()` | An address right now. Does not wait |
| `kode_wifi_connect(ssid, password)` | Join and wait, up to `KODE_WIFI_CONNECT_MS` (30 s). A network that joined is remembered |
| `kode_wifi_scan()` | Both bands. Long. Not used here |
| `CONFIG_KODEOS_WIFI_AUTOJOIN` | AO_Radio joins the strongest remembered network when the link is up |

The console verb the network and MQTT examples name is `wifi_join`. That is the provisioning path for a board that has never joined: USB console, once, and autojoin after that. The password is the SDK's to remember. This app does not have to store it. The card's `ssid` and `password` lines are optional, for a board whose console is not at hand. They are not logged.

`kode_http_post()` is the wrong call for xAI. It picks `Content-Type` from the body, it cannot set `Authorization`, it returns the whole body, and `KODE_HTTP_BODY_MAX` is 64 KiB. `kdrv_net_http_post()` can POST, and `kdrv_net`'s exchange does know a bearer, but the public POST used by AO_Net is capped (`KDRV_NET_POST_MAX` is 1024 bytes, `KDRV_NET_BODY_MAX` is 1528, `KDRV_NET_URL_MAX` is 80). The streaming reader (`kdrv_net_stream_open`) is GET only. A chat with a system prompt and a few turns does not fit in 1024 bytes, and a token stream is not a GET.

So the worker calls `esp_http_client` itself, the same client `kdrv_net.c` uses. `crt_bundle_attach` is `esp_crt_bundle_attach` (the Mozilla bundle). Redirects are disabled so a bearer is not forwarded. A read of zero bytes is not treated as the end of the body: `kdrv_net.c` records that this reported a finished download after zero bytes, and the scaffold uses `esp_http_client_is_complete_data_received()` the same way. The URL and the status are logged. The key and the `Authorization` value are not.

mbedTLS keeps its large buffers in PSRAM (`CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC` in `sdkconfig.board`). The comment there says a session still holds about 30 KB of the internal pool, and a renewal is two handshakes. That is the heap to watch with `test component=mem`, not the 32 MB of PSRAM. The board file also sets `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_CROSS_SIGNED_VERIFY` because some public hosts chain through a root the bundle dropped. `api.x.ai` has to be confirmed on a Dot; a handshake failure is a log line, not a guess from this note.

The network example measured about 395 KB of image for the stack on an app that never opened a socket (1,791,536 bytes with `KODEOS_NET`, 1,387,280 without). This app pays that once. The mailbox is about 6 MB. The image will not approach it.

## The xAI API

Base URL `https://api.x.ai/v1`. Authentication is `Authorization: Bearer <key>`. The docs and the OpenAPI index at `https://api.x.ai/docs/` describe an OpenAI-shaped HTTP API.

### Chat

Two text endpoints, both streamed as server-sent events when `"stream": true`:

| | Chat Completions | Responses |
|---|---|---|
| Method and path | `POST /v1/chat/completions` | `POST /v1/responses` |
| Docs, 2026-11-19 | Called a legacy endpoint. New features land on Responses first | The one the best-practices page tells new code to use |
| Body | `messages[]` of `{role, content}` | `input` |
| Text delta | `choices[0].delta.content` | event `response.output_text.delta`, JSON field `delta` |
| End | `data: [DONE]` | `response.completed` |
| Reasoning | `delta.reasoning_content`, and multi-turn reasoning is not carried back | Encrypted reasoning items can be sent back |

A recorded Chat Completions chunk, in the shape the streaming page shows (the page pretty-prints it; the wire form is one JSON object per `data:` line):

```text
data: {"choices":[{"index":0,"delta":{"content":"Ah","role":"assistant"}}]}

data: [DONE]
```

The scaffold's parser takes that, and it takes a Responses `delta` string when the event name or the JSON `type` is `response.output_text.delta`. `reasoning_content` is reported separately and is not shown as the answer. The key match is exact, so the letters `content` inside `reasoning_content` are not the answer. One SSE event over 2048 bytes is an error and is skipped. Token deltas are far smaller. A `response.completed` blob can be large; the text has already arrived as deltas.

Chat Completions is the body `grok_request_write()` builds, because it is a short JSON object, it has no tool-call schema, and Ollama's OpenAI-compatible server speaks it too. Moving the default to Responses later is a second builder behind the same base URL. The path is a config field (`/chat/completions`).

### Which model

Prices below are USD per 1 million tokens, from the pricing page fetched 2026-11-19. Long-context rates apply to the whole request once the prompt crosses the threshold (200k tokens on these models). A pocket chat will not cross it.

| Model | Context | Input | Cached input | Output |
|---|---|---|---|---|
| `grok-4.3` | 1M | $1.25 | $0.20 | $2.50 |
| `grok-4.20-0309-non-reasoning` and the other 4.20 rows | 1M | $1.25 | $0.20 | $2.50 |
| `grok-4.5`, `grok-4.6`, `grok-4.7` | 500k | $2.00 | $0.30 to $0.50 | $6.00 |
| `grok-build-0.1` | 256k | $1.00 | $0.20 | $2.00 |

`grok-4.7` is the flagship the streaming examples name. It is the wrong default for this glass: a reply the screen can show is a few hundred tokens, and the flagship's output price is more than twice `grok-4.3`. `grok-build-0.1` is cheaper and is a build model, not a general chat default.

On 15 May 2026 xAI retired `grok-4-fast-*`, `grok-4-1-fast-*`, `grok-4-0709`, `grok-3` and `grok-code-fast-1`. Those slugs redirect to `grok-4.3` (or `grok-build-0.1` for the code slug) and bill at the new price. Do not pin them.

A short turn, 500 input tokens and 200 output tokens on `grok-4.3`, is about a tenth of a cent before tax. A hundred such turns is about ten cents. Reasoning that runs long is the cost that moves: the streaming page says to raise the client timeout, because a reasoning model can sit silent, and reasoning tokens are billed as output. This client waits 120 seconds for the next byte and does not display `reasoning_content`. Set the model string in the config to try another. There is no `reasoning_effort` field in the Chat Completions body this scaffold sends. The Responses API is where that knob lives (`none`, `low`, `medium`, `high` in the May 2026 migration note).

### Rate limits

Tiers follow cumulative spend since 1 January 2026. Tier 0 is the default. `grok-4.3` at tier 0 is 37 requests per second and 10 million tokens per minute. `grok-4.7` at tier 0 is 150 requests per second and 50 million tokens per minute. One Dot will not hit either. The client still maps HTTP 429 to a line on the glass. Voice is limited by requests per second and concurrent sessions, not by tokens: speech-to-text is 10 requests per second at tier 0, text-to-speech is 50, and `grok-voice-3.0` is 10 concurrent sessions.

### TLS, JSON, heap

The request is HTTPS with the certificate bundle. A LAN relay may be plain `http://`. The bundle attach is harmless on plain HTTP, which is what `kdrv_net.c` says too.

JSON in the scaffold is a builder and a walker written for the two event shapes, covered by `host/test_grok.c`. cJSON is the right library when the schema grows (tool calls, usage, a general error object). It is the ESP-IDF `json` component, header `cJSON.h`, MIT, already in the IDF tree. It is not linked yet, so the host test is plain gcc and does not need the IDF. A streaming parser still wants a bounded walker: cJSON parses one complete `data:` payload, which is what this walker does, and it does not want the whole SSE response in one buffer.

Budget for one turn, all of it PSRAM except the TLS internal residue:

| Use | Size |
|---|---|
| JSON body | 8 KiB (`GROK_REQ_MAX`) |
| SSE parser | about 4 KiB |
| Reply text | 2 KiB (`GROK_REPLY_MAX`), then cut into lines of 119 characters |
| TLS session | about 30 KB internal, the rest in PSRAM, from the board file's comment |
| Worker stack | 20 KiB, in PSRAM |

The transcript keeps 8 lines. A long answer pushes the question off the ring. That is a scaffold limit, not an API limit.

## Voice

### What the Dot has

`kdrv_es8311.h` names the path: an ES8311 codec, an NS4150B amplifier, a ZTS6211E microphone. The only rate is 48 kHz, 16-bit, mono (`KDRV_ES8311_RATE_HZ`, and `kode.h` says the same for `kode_audio_*`). The microphone is analog (`KDRV_ES_DMIC` must read 0). Gain is `kode_audio_set("mic_gain", ...)`, 0 to 42 dB in steps of 6.

Capture and playback that an app should call:

| Call | Behaviour |
|---|---|
| `kode_audio_record_start(ms)` | Returns once the microphone is running. At most `KODE_AUDIO_RECORD_MAX_MS` (30 s) |
| `kode_audio_record_stop()` | Ends it and keeps the frames |
| `kode_audio_record_status()` | Milliseconds and, once stopped, the frame count |
| `kode_audio_record_save(path)` | Writes a 48 kHz mono WAV |
| `kode_audio_play_samples(pcm, n)` | Plays 48 kHz mono s16 and blocks until they have been heard |
| `kode_audio_record_samples(...)` | A blocking capture into the caller's buffer. Wrong shape for "hold to talk" |

The top button is push-to-talk: press starts `kode_audio_record_start(8000)`, release calls `kode_audio_record_stop()`. Eight seconds of 48 kHz mono s16 is 768,000 bytes. The codec driver's own buffer is 30 seconds (`KDRV_ES8311_BUF_SECONDS`), in PSRAM. The scaffold records and then says the upload is not wired. It does not allocate a second copy of the audio.

The amplifier is switched by the C5. With `KODEOS_RADIO` on, the C5 is up for Wi-Fi, which is also what the amplifier needs. The Kconfig text says a sound in the first ~1.5 s of a boot where the C5 was in reset is silent. Do not play TTS in that window.

Holding bottom is home. The top button is free for talk. The pad scrolls and picks a canned prompt. A tap of bottom (the release, if the hold did not fire) sends that prompt.

### What xAI offers

xAI does have speech endpoints. They are the right cloud path. Checked 2026-11-19:

| API | Endpoint | Fit for this board |
|---|---|---|
| Speech to text | `POST /v1/stt`, or `wss://api.x.ai/v1/stt` | Batch REST is the one to wire first. PCM signed 16-bit little-endian, sample rates include 16000 and 48000. Default model `grok-voice-transcribe-2.0` |
| Text to speech | `POST /v1/tts`, or a WebSocket | Ask for `output_format.codec = "pcm"` and `sample_rate = 48000`. The duration formula on the docs page uses 2 bytes per PCM sample. Play the bytes with `kode_audio_play_samples()`. Default voice `eve`. `language` is required (`en` or `auto`) |
| Speech to speech | `wss://api.x.ai/v1/realtime?model=grok-voice-latest` | Full duplex. `grok-voice-latest` is `grok-voice-3.0`. PCM rates include 48000. Ephemeral tokens are the recommended client credential |

Prices on the same page: speech-to-text $0.10 per hour batch, $0.20 per hour streaming; text-to-speech $15 per million characters; speech-to-speech $0.08 per minute plus $0.004 per text input. Eight seconds of transcription is a fraction of a cent. A short spoken reply is well under a cent at the character price. A minute of duplex is about eight cents.

Push-to-talk then REST is the plan:

1. Record 48 kHz mono s16, stop on release.
2. `POST /v1/stt` with that PCM (or a WAV from `kode_audio_record_save`). 48 kHz is accepted, so the first version does not resample. 16 kHz cuts the upload to a third and is the documented default.
3. Feed the transcript to the chat client already in this tree.
4. `POST /v1/tts` with PCM at 48000 and play it.

The first board capture has to confirm the TTS PCM is little-endian s16. The STT page says that explicitly for its PCM. The TTS page says raw PCM and 2 bytes per sample, and lists 48000 as a legal rate. If the first utterance is noise, the endianness is the first thing to flip. MP3 is the TTS default and the Dot's `kode_audio_play()` refuses it: the header says 48 kHz mono WAV or samples only.

Duplex on `wss://api.x.ai/v1/realtime` is a later project. ESP-IDF has `esp_websocket_client`. The session is JSON plus binary PCM, the server can do its own voice-activity detection, and the docs want an ephemeral token rather than the account key on the device. That token is minted by the relay. It is the right voice UX once text chat and push-to-talk both work. It is the wrong first milestone: a WebSocket, a continuous capture, and barge-in, on top of a TLS session that already costs internal RAM.

### On-device speech, if the LAN is down

These do not replace Grok. They are the offline leftovers.

**ESP-SR** (`espressif/esp-sr`) runs on the ESP32-P4. WakeNet is a wake word. MultiNet is a command list. VADNet is voice activity. None of them is free dictation, and none of them is a conversation. The repository `LICENSE` fetched 2026-11-19 is the Espressif MIT licence: use on Espressif chips, keep the notice. That is not the ordinary MIT grant. It is an extra restriction. It can run on a Dot. Do not drop it into this GPL-3 firmware until someone has decided that the restriction is acceptable to convey. Wake-word names that are trademarks (the README lists Alexa and others) need their own rights before a commercial use. The scaffold does not vendor ESP-SR.

**esp-tts**, in that same tree, is a small embedded voice. Headers in the tree have carried an Apache-2.0 notice, which combines with GPL-3 if the Apache text stays with the files. Confirm the file you actually copy. The voice will not sound like `eve`, and the languages are the ones that component was built for. It is a fallback for a status sentence, not for the model's answer.

No local LLM. A quantised model that is actually useful is hundreds of megabytes. This part has 32 MB of PSRAM and no accelerator aimed at transformers. A tiny model that fit would not be a Grok. The local option is a computer on the LAN.

## Offline, and keeping the key off the Dot

The Dot does not run the model. Two computers can:

1. **Ollama or llama.cpp** on a PC, OpenAI-compatible `POST /v1/chat/completions`. The Dot's `base_url` becomes `http://192.168.1.20:11434/v1` and `model` becomes whatever that server loaded. No key. The LAN has to be up. Latency is the PC's, plus Wi-Fi, plus the TLS-less HTTP round trip. This is the honest offline mode: offline from xAI, not offline from the room.
2. **The relay** in `relay/relay.py`. Same JSON. The process holds `XAI_API_KEY` and forwards to `https://api.x.ai/v1`, or `GROK_UPSTREAM` points it at Ollama on localhost. The card's `api_key` stays empty. Ephemeral tokens for the realtime voice socket would be minted here too.

The relay is a stdlib Python script. It was not run against a live key. It streams the upstream body back with chunked encoding. It does not log headers.

There is no mode in which the Dot answers with a language model and no network at all. The glass should say "No network." and keep the transcript. That is what the status line does.

## Where the key goes

Recommend **one file on the card, `sd/grok/config.txt`**, for a Dot the owner keeps. The example is `config.example.txt`. The key is never compiled in, never committed, and never logged. The parser stores it in RAM for the boot. A value that does not fit the 179-character field is cut and the log says a value was cut, not what it was.

Why this and not the others:

| Place | Why it is not the default |
|---|---|
| The source tree | A committed key is a published key. Do not. |
| NVS, via `kode_setting_set` | The value cap is `KODE_SETTING_VALUE_MAX` (192 bytes), which a key might fit. The board file does not turn on flash encryption or NVS encryption. A flash dump reads it. Settings also go through a path a PSRAM worker must not call (`kode_worker.h`: no NVS from a job). |
| The serial console, as the only store | Fine for Wi-Fi (`wifi_join` already remembers a network). Awkward for a key that is rotated by editing a file on a laptop. The console is still how Wi-Fi should be provisioned. |
| The relay | Better when the Dot is shared or the key is a team key, because the card is removable and the flash is not encrypted. It is an extra process that has to be running. Use it then, and leave `api_key` empty. Do not do both. |

The card is not a secret store. Anyone who takes the card takes the key. That is acceptable for a personal Dot and a personal key, and it is the reason the relay exists for every other case. Rotate a key by editing the file. Do not put the key in a screenshot, a log, or a crash dump.

`kode_http` and this client both refuse to put the key in the URL. The HTTP layer also refuses a key that contains a space or a control character, so it cannot break out of the header.

## Wi-Fi, failure, latency

Provisioning: join once with `wifi_join` on the console. `CONFIG_KODEOS_WIFI_AUTOJOIN` brings it back. The app waits about eight seconds at boot for that address before it reads the card a second time and, if `ssid` is set and there is still no address, calls `kode_wifi_connect()`.

What the glass says:

| State | Line |
|---|---|
| No address | `No network.` |
| No file | `No sd/grok/config.txt.` |
| xAI base, empty key | `No API key in the config file.` |
| Microphone open | `Listening. Release the top button.` |
| Request out | `Asking...` |
| Otherwise | The model name |
| HTTP 401 | `The API key was refused.` |
| HTTP 429 | `Rate limit. Wait and try again.` |
| No route, TLS failure, stall | The short reason from the client (`could not connect`, `the reply stalled`, …) |

Latency, as an expectation, not a measurement on a Dot:

| Step | What to expect |
|---|---|
| Join | Up to 30 s (`KODE_WIFI_CONNECT_MS`). Usually a few seconds on a network the C5 has joined before |
| First TLS handshake | On the order of a second, sometimes two, plus the ~30 KB internal alloc. Later turns on a new connection pay it again. This client does not keep the connection open |
| First token, `grok-4.3`, short prompt, no long reasoning | Often under a few seconds on a decent link. Not measured here |
| A reasoning model | Can be quiet for a long time. The read budget is 120 s. The streaming docs say to raise it |
| Push-to-talk round trip, once wired | Record, upload ~0.8 MB for 8 s at 48 kHz, STT, chat, TTS, download, play. Several seconds. Not a phone call |
| Duplex voice, later | The speech-to-speech page describes sub-second turns. That is the server. The Dot still has to move PCM both ways over TLS |

The C5's radio draw is called out by the MQTT example as about 27 mA for as long as the link is up. A chat app holds the link for the whole run, because autojoin and the C5 amplifier want it. That is a battery cost kodeOS-without-radio does not pay.

## What the scaffold does

It does:

- An ESP-IDF project named `grok`, role app, radio on, one Active Object and one worker.
- A config parser, a Chat Completions body builder (with escaping, and a fitter that drops the oldest turn when the body will not fit), and an SSE parser for Chat Completions and for Responses text deltas.
- A host test, plain gcc, no network and no key, fed with files in `host/samples/`.
- A worker path that will POST the body with `esp_http_client` when the image is built.
- A glass of short lines: status, the armed prompt, a keyboard sentence, and up to five transcript lines. Left and right change the prompt. Up and down scroll. Bottom sends. Top is push-to-talk.
- Microphone start/stop and a `grok_audio_play()` that plays 48 kHz mono s16. Nothing calls play yet.
- `relay/relay.py`, optional, not built in.

It does not:

- Upload audio or call `/v1/stt` or `/v1/tts`.
- Open the realtime WebSocket.
- Draw the reply token by token. The parser yields deltas; the glass updates when the turn ends.
- Offer a keyboard. `grok_keyboard_hint()` is the stub.
- Keep more than eight lines, or more than 119 characters on a line. `kode_screen_add_text` stops at `KODE_WIDGET_TEXT_MAX` (128, NUL included), which is why the line is short.
- Run on a board in this environment. See the README for the build.

## Risks

- The image has not been linked here if the ESP-IDF build did not finish. The README says which. Internal RAM, not PSRAM, is what can fail `test component=mem` once TLS and LVGL are both up. `kode_screen` pulls LVGL. A later canvas font could drop that if the static image or the heap is tight.
- `api.x.ai`'s certificate chain was not exercised. The bundle is the same one `kdrv_net` uses. A failure is a log line and "could not connect".
- Chat Completions is the legacy endpoint. It is the right first protocol because Ollama speaks it and the JSON is small. A multi-turn reasoning conversation will be worse on it than on Responses, because reasoning is not sent back. The parser will not have to be thrown away to move.
- The card and the flash are both readable. The key recommendation is for a personal Dot only.
- Voice PCM endianness for TTS is documented as raw 16-bit, and STT is explicit about little-endian. Confirm on the first utterance.
- ESP-SR's licence is not a clean MIT grant. Do not vendor it in a hurry.
- Catalogue rules B1 and B3 fail on purpose for this in-tree build, as they do for `apps/gbc_emu`. They pass only after the app is split out and pinned to a release tag.
- Holding bottom returns home. A press that becomes a hold does not also send. A short press does.

## Next steps

1. Build with ESP-IDF v6.1 and install with `idf.py flash` (the mailbox write `kbsp` retargets) or `tools/kcli.py install`. Confirm the idle glass, that bottom-hold returns home, and that a missing file and a missing network say so. Do not `idf.py erase-flash`.
2. Put a real key only on the card. Send one canned prompt. Read the log for the status code and the first tokens. If TLS fails, the error name is the next fact, not a code change guessed in advance.
3. Redraw on deltas: the parser callback already runs as bytes arrive. Post a signal every few characters instead of waiting for the turn. Raise the line width only with a font on a canvas; the widget cap is 127 bytes.
4. Push-to-talk: save the WAV, `POST /v1/stt`, then the chat path, then `POST /v1/tts` with PCM at 48000 and `grok_audio_play()`.
5. If the key must leave the Dot, run the relay and clear `api_key`. The same base URL talks to Ollama.
6. Split the app out to `kode-app-grok` from `kode-app-template` when it is worth a catalogue review. Pin a kode-sdk release tag. The in-tree `EXTRA_COMPONENT_DIRS` block is deleted then.

## Sources

- This tree: `sdk/components/kbsp/partitions.csv`, `sdk/components/kbsp/sdkconfig.board`, `sdk/components/kdrv/apps/include/kapps_fmt.h`, `sdk/components/kode/include/kode.h` (audio, Wi-Fi, HTTP), `sdk/components/kdrv/radio/include/kdrv_net.h`, `sdk/components/kdrv/radio/src/kdrv_net.c`, `sdk/components/kdrv/audio/include/kdrv_es8311.h`, `sdk/components/kaos/Kconfig` (`KODEOS_RADIO` selects esp-hosted), `sdk/components/kaos/radio/src/radio_ao.c` (`kdrv_sd_set_hosted`), `sdk/examples/network/sdkconfig.defaults`, `sdk/examples/mqtt/sdkconfig.defaults`, `bootloader_components/main/bootloader_start.c`.
- docs.kode.diy boot model, as cited by `apps/gbc_emu/FEASIBILITY.md` for the same SDK commit.
- docs.x.ai, fetched 2026-11-19: streaming, models, pricing, rate limits, the May 2026 retirement note, speech-to-text, text-to-speech, speech-to-speech. OpenAPI index `https://api.x.ai/docs/`.
- `espressif/esp-sr` `LICENSE` and README, fetched 2026-11-19. The Espressif MIT text limits use to Espressif chips.
- `kodediy/kode-apps` `categories.txt` on `develop`: `ai` is a legal category.
