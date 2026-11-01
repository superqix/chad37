# Chordpad37

A MIDI controller firmware for the M-Vave SMK-37, with Chordcade's Easy Chord on the keys and chords on the 16 pads.
It has no internal sound. Built on [fm1-base](https://github.com/charlesvestal/fm1-base) and
[felucca-bsp](https://github.com/hugelton/felucca-bsp), forked from [Erik37](https://github.com/genkigenki/erik37).

**Status: bring-up only, not yet run on a keyboard.** The app shows every control on screen and sends it over USB and
Bluetooth, to check the hardware. CC numbers are placeholders until `docs/stock-midi-map.md` is filled in. Easy Chord
is in `app/` and tested on a computer, but isn't connected to the keys or pads yet. There is no TRS MIDI out yet.

## Before installing

- Capture what stock V016 sends in `docs/stock-midi-map.md`. You can't do this later without going back to stock.
- Take a full flash backup. See `02-update-and-recovery.md` in
  [fm1-to-smk37](https://github.com/genkigenki/fm1-to-smk37).
- The first install clears stock settings. M-Vave's updater returns to stock, which then rebuilds them.

## Building

Tests, on a Mac or Linux with a C compiler and Python 3 with Pillow:

```bash
sh app/tests/run_tests.sh
```

Firmware, on Linux or WSL with JieLi's toolchain (`base/tools/get_toolchain.sh`) and the AC79 SDK:

```bash
./build.sh
```

Each model has its own package in `build/chordpad37/`. Install with:

```bash
python3 base/tools/fm1_install.py build/chordpad37/chordpad-smk37.fwsc
```

## Licence

MPL-2.0. See [LICENSING.md](LICENSING.md).
