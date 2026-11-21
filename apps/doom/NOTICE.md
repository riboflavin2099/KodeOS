# Licences of the Doom app

This file says which licence applies to each part of `apps/doom`. Read it before distributing the app.

| Part | Owner | Licence |
|---|---|---|
| the app sources under `main/`, `host/`, `port/` and the text of this tree | the author of this app, offered under the same terms as kodeOS | GPL-3.0-or-later, with the additional permission below |
| Chocolate Doom, imported at the commit in `third_party/chocolate-doom/UPSTREAM.md` | id Software, Raven Software, Simon Howard and the other authors named in those files | GPL-2.0-or-later. The firmware conveys it under GPL-3.0-or-later |

The app links the kode SDK. The SDK is GPL-3.0-or-later, except the parts `sdk/NOTICE.md` names (QP/C is GPL-3.0-or-later or a commercial licence of Quantum Leaps; RFAL is STMicroelectronics SLA0051, which is not an open source licence; the fonts are SIL OFL 1.1). A firmware that links the SDK is conveyed under GPL-3.0-or-later.

`LICENSE` in this directory is the GNU GPL version 3, the text kode-apps rule A5 compares an app against.

The 5x7 face in `main/doom_font_glyphs.inc` was drawn for this scaffold. It is not a game logo and it is not taken from an id or Raven asset.

## Additional permission under GNU GPL version 3 section 7

If you modify this Program, or any covered work, by linking or combining it with the RF Abstraction Layer (RFAL) and the ST25R3916 driver of STMicroelectronics (or a modified version of those libraries), containing parts covered by the terms of the STMicroelectronics SLA0051 licence, the licensors of this Program grant you additional permission to convey the resulting work. Corresponding Source for a non-source form of such a combination shall include the source code for the parts of RFAL and of the ST25R3916 driver used as well as that of the covered work.

## Trademarks

This licence gives no right to use the names Kode, Kode Dot, kodeOS or Kody as trademarks. Doom, Doom II, Heretic, Hexen, Final Doom and the game logos are trademarks of their owners. This app does not ship those marks as artwork, and it does not ship a WAD.

## What is not in this tree

No IWAD, no PWAD, no SIGIL file, no Legacy of Rust file, and no title-screen picture is included. The person running the app supplies those. The select screen paints a text plate and, later, a picture decoded from that person's own WAD.
