# Licences of the GBC app

This file says which licence applies to each part of `apps/gbc_emu`. Read it before distributing the app.

| Part | Owner | Licence |
|---|---|---|
| the app sources under `main/`, `host/` and the text of this tree, other than the rows below | the author of this scaffold, offered under the same terms as kodeOS | GPL-3.0-or-later, with the additional permission below |
| `third_party/walnut-cgb/walnut_cgb.h` and `third_party/walnut-cgb/LICENSE` | Mr. Paul, and the Peanut-GB and SameBoy authors named in the header | MIT. The header's own comment names Mahyar Koshkouei (Peanut-GB) and Lior Halphon (SameBoy fragments) |
| `third_party/walnut-cgb/minigb_apu/` | Alex Baines and Mahyar Koshkouei | MIT |

The app links the kode SDK. The SDK is GPL-3.0-or-later, except the parts `sdk/NOTICE.md` names (QP/C is GPL-3.0-or-later or a commercial licence of Quantum Leaps; RFAL is STMicroelectronics SLA0051, which is not an open source licence; the fonts are SIL OFL 1.1). A firmware that links the SDK is conveyed under GPL-3.0-or-later. The MIT files stay under the MIT grant: keep their copyright notices with any copy of those files.

`LICENSE` in this directory is the GNU GPL version 3, the text kode-apps rule A5 compares an app against.

## Additional permission under GNU GPL version 3 section 7

If you modify this Program, or any covered work, by linking or combining it with the RF Abstraction Layer (RFAL) and the ST25R3916 driver of STMicroelectronics (or a modified version of those libraries), containing parts covered by the terms of the STMicroelectronics SLA0051 licence, the licensors of this Program grant you additional permission to convey the resulting work. Corresponding Source for a non-source form of such a combination shall include the source code for the parts of RFAL and of the ST25R3916 driver used as well as that of the covered work.

## Trademarks

This licence gives no right to use the names Kode, Kode Dot, kodeOS or Kody as trademarks. Game Boy and Game Boy Color are trademarks of Nintendo. This app does not ship Nintendo software. A person who runs it supplies their own ROM.

## What is not in this tree

No commercial ROM, no boot ROM and no capture of a commercial game is included. `third_party/walnut-cgb/UPSTREAM.md` records the commit the vendored files were copied from. The upstream repository also contains GIF captures of commercial games. Those files were not copied.
