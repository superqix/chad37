# Handoff: Chordpad37 on the PC

For a Claude session on the Windows PC, picking up from a session on the Mac. Read this, then `docs/plan.md` (the
approved plan) and `README.md`.

## What this is

Chordpad37 is a MIDI controller firmware for the M-Vave SMK-37: Chordcade's Easy Chord on the keys and chords on the
16 pads, with no internal sound. It's a fork of [Erik37](https://github.com/genkigenki/erik37), which runs on
fm1-base and felucca-bsp (both MPL-2.0, vendored in `base/` and `base/bsp/`, not submodules).

It replaces the `smk37_open` plan. Don't continue that one.

- **Repo:** https://github.com/superqix/chordpad37 (`origin`), with Erik37 as `upstream`.
- **Branch:** `main`. Erik37 1.2 (`c06c1f7`), then our commits.

## Where it stands

**Update, 2026-11-02: M0 and M1 are done on the PC.** The stock map is filled in (`docs/stock-midi-map.md`), the flash
is backed up (`docs/flash-backup.md`; the dumps stay off the repo), and M1 passed on an SMK-37 Elite: see the results
table under M1 in `docs/plan.md`. The keyboard is back on stock V016 while features are built. The app now advertises
over Bluetooth as `Chordpad37` and over USB as `Chordpad37 SMK-37`. The sections below describe the state when this
handoff was written; "Never done" and the steps under "What to do next" are history.

**Done on the Mac, tested in the host simulator only:**
- The piano engine, samples, fonts, site and Erik37's tools are removed.
- `app/app.c` is a bring-up app. It shows every control on screen and sends:
  - keys on channel 1 from note 48, and pads on channel 10 from note 36 at a fixed velocity of 100;
  - knobs as CC 70–77 and faders as CC 20–23, both placeholders until the stock capture is done;
  - the mod wheel as CC 1, and pitch bend.
  - The pedal and buttons are only shown on screen. Audio is silent and incoming MIDI is dropped.
  - Everything goes to USB and Bluetooth. K5 and the faders take their first reading as a baseline, so nothing is
    sent at boot.
- `app/easy_chord.c/.h` with 940 test vectors in `tests/`. Not connected to the keys or pads yet (that's M4).
- `docs/stock-midi-map.md`, the sheet for capturing what stock V016 sends.
- One change in `base/`: `_POSIX_C_SOURCE` raised to `200112L` in `base/host/plat_host.c`, so the simulator builds on
  macOS.
- `app/fm1.toml`: name `CHORDPAD`, `smk_id_prefix "5701"` (not checked against other firmware), `ble = true`.
- `app/words.tsv` has a placeholder URL (`superqix.github.io/chordpad37/words/`), because the build requires one.

**Never done:** a firmware build with JieLi's toolchain, or anything on the keyboard.

## Tests

```bash
sh app/tests/run_tests.sh
```

Easy Chord vectors under ASan and UBSan, then the bring-up script in the simulator (`app/tests/bringup.txt`). Both
pass on the Mac.

```bash
sh base/tests/run_tests.sh
```

Passes, except the update-path test, which is skipped without a template package.

## What to do next on the PC

Milestones M0 and M1 in `docs/plan.md`. In order:

1. **Ask whether the earlier PC session already has** a filled-in stock capture or a verified flash backup. Reuse them
   if so.
2. **Fill in `docs/stock-midi-map.md` while the keyboard is still on stock V016.** It's the spec for M3, and can't be
   done later without returning to stock.
3. **Confirm the model:** Pro, Elite or MKE. Each needs its own package.
4. **Set up the build in WSL2,** not native Windows:
   - `base/tools/get_toolchain.sh` (installs to `~/.jieli/toolchain`);
   - the AC79 SDK `AC79NN_SDK_V1.2.1_2023-12-13` at `~/fw-AC79_AIoT_SDK`, or set `AC79_SDK`;
   - Python 3 with Pillow (and libraqm) and NumPy.
5. **Build unmodified Erik37 first** (`git checkout c06c1f7`) to prove the toolchain, then `main` with `./build.sh`.
   `build.py`'s size, RAM and register checks must pass.
6. **Take a full flash backup before the first install,** using Method A in `02-update-and-recovery.md` of
   [fm1-to-smk37](https://github.com/genkigenki/fm1-to-smk37): run only the update's check step, send the UBOOT key,
   read the flash twice with `jl-uboot-tool`, check the SHA-256 values match and `trimdat` is at `0xC1224`.
   (Done: `fm1_install.py --check-only --uboot` and `tools/smk_backup.py`.)
7. **Install and test** as listed in M1: Bluetooth MIDI, USB audio, K1–K8 order and direction, K5, faders, wheels,
   pedal, latency, and returning to stock with M-Vave's updater.

**Stop and review with Michael if Bluetooth or the return to stock fails.**

## Rules that matter

- **Flash:** never write `0x0–0x3FFF`, `0xC0000–0xC1FFF`, `0xF4000–0xFDFFF` or `0xFF000–0xFFFFF` (calibration,
  pairing, key data). `base/bsp/boards/smk37/fm1_flash_map.h` enforces this; don't weaken it. The first install clears
  stock settings, and stock rebuilds them on return.
- **Store:** at most 9 objects of 3,840 bytes each on the SMK-37.
- **Registers:** only inside `base/bsp/`. `build.py` rejects them anywhere else, so the TRS out driver (M2) goes in
  `base/bsp/boards/smk37/`.
- **Licensing:** MPL-2.0, with a header on every file. Don't copy GPL code (Snoop37, `smk_uart_tx.h`). The port kit's
  text is CC BY-NC, so write our own wording and credit it.
- **Git:** commit and push only when Michael asks.

## Older work, for reference only

`chordpad-SMK37` has a local-only `easy-chord` branch (commit `d5d6753`) on the Mac. It was never pushed. Everything
useful from it is already copied into this repo.
