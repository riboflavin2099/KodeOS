# Licences of the Grok app

This file says which licence applies to each part of `apps/grok`. Read it before distributing the app.

| Part | Owner | Licence |
|---|---|---|
| the app sources under `main/`, `host/`, `relay/` and the text of this tree | the author of this scaffold, offered under the same terms as kodeOS | GPL-3.0-or-later, with the additional permission below |

The app links the kode SDK. The SDK is GPL-3.0-or-later, except the parts `sdk/NOTICE.md` names (QP/C is GPL-3.0-or-later or a commercial licence of Quantum Leaps; RFAL is STMicroelectronics SLA0051, which is not an open source licence; the fonts are SIL OFL 1.1). A firmware that links the SDK is conveyed under GPL-3.0-or-later.

`LICENSE` in this directory is the GNU GPL version 3, the text kode-apps rule A5 compares an app against.

No model weights, no API key and no recording of a conversation are in this tree. cJSON ships with ESP-IDF (MIT) and is not linked: the request builder and the SSE parser in `main/` are this app's own code so the host test needs no library. ESP-SR and esp-tts are not vendored. Their licences are named in [FEASIBILITY.md](FEASIBILITY.md) and have to be read before either is added.

## Additional permission under GNU GPL version 3 section 7

If you modify this Program, or any covered work, by linking or combining it with the RF Abstraction Layer (RFAL) and the ST25R3916 driver of STMicroelectronics (or a modified version of those libraries), containing parts covered by the terms of the STMicroelectronics SLA0051 licence, the licensors of this Program grant you additional permission to convey the resulting work. Corresponding Source for a non-source form of such a combination shall include the source code for the parts of RFAL and of the ST25R3916 driver used as well as that of the covered work.

## Trademarks

This licence gives no right to use the names Kode, Kode Dot, kodeOS or Kody as trademarks. Grok and xAI are names of their owners. This app does not grant any right to use them.
