# Licensing

**Chordpad37 is MPL-2.0** (full text in `LICENSE`), file by file: every source file carries an
`SPDX-License-Identifier: MPL-2.0` header.

| What | Licence | Where |
| --- | --- | --- |
| Chordpad37's app and tests | MPL-2.0 | `app/`, `tests/` |
| fm1-base (the platform) and felucca-bsp (chip, board, update loader, packaging) | MPL-2.0 | `base/` and `base/bsp/`; their own `LICENSING.md` lists what they are built from |
| fm1-base's fonts (Barlow Semi Condensed) | SIL OFL 1.1 | `base/assets/fonts/` |
| JieLi AC79 SDK files a package needs, and its Bluetooth libraries | Apache-2.0, per fm1-base's notes; confirm before any public release | read from your SDK checkout at build time; never in this repository |

Forked from [Erik37](https://github.com/genkigenki/erik37) by M. Geisler. Erik37's piano samples and fonts are not in
this firmware, though they remain in the git history under their own licences (CC BY 3.0 and SIL OFL 1.1; see Erik37's
`LICENSING.md`).
