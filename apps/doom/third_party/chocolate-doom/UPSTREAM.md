# Chocolate Doom pin

Not vendored. The engine sources are not in this tree and are not in the link.
`main/doom_engine.c` is the placeholder.

| | |
|---|---|
| Upstream | https://github.com/chocolate-doom/chocolate-doom |
| Commit | `a2dfd51fcc00d272dc8a17a97ccd6683333ff4ea` (master on 2026-11-13) |
| Licence | GPL-2.0-or-later |

Checked at that commit: `src/doom/d_main.c`, `src/heretic/d_main.c` and
`src/hexen/h2_main.c` each say "either version 2 of the License, or (at your
option) any later version". `src/hexen/h2_main.c` also carries the Raven
Software copyright. `src/d_iwad.c` is the file-name table the scanner follows,
plus the 2024 re-release names added in `doom_wad.c`.

The combined firmware is GPL-3.0-or-later because it links the SDK. The
Chocolate grant allows that choice. Do not replace this pin with
`id-Software/DOOM` (the 2024 README says "GPL 2.0" and does not say "or
later") or with retro-go's prboom-go.

Import, when the engine is linked: `src/doom`, `src/heretic`, `src/hexen`,
and the shared files they need. Leave out SDL video, the Windows tree, the
setup UI, and Strife. Heretic and Hexen use the same global names as Doom, so
the link prefixes those two trees. Legacy of Rust (`id1.wad`) needs MBF21 or
ID24, which this pin does not implement.
