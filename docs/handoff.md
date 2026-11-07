# Handoff: Chordpad37

For whoever picks this up next: another Claude session, another coding agent, or a person. Read this first, then
[plan.md](plan.md). Updated 2026-11-07.

## What this is

Chordpad37 is a MIDI controller firmware for the M-Vave SMK-37: Chordcade's Easy Chord on the keys and chords on the 16
pads, with no internal sound. It's a fork of [Erik37](https://github.com/genkigenki/erik37) on fm1-base and
felucca-bsp (both MPL-2.0, vendored in `base/` and `base/bsp/`, not submodules, so we change them directly).

- **Repo:** https://github.com/superqix/chordpad37 (`origin`). Branch `main`.
- **Owner:** Michael. He does every hands-on test on the keyboard; the code side is ours.

## Where it stands

**Milestones M0 to M4 are done** ([plan.md](plan.md) has each one; M1's table has the hardware results).

- **On the keyboard:** an SMK-37 **Elite** runs commit `29ccb99`. Michael plays it into Surge XT over USB and an
  FM-1 over TRS.
- **Not pushed:** `main` is 9 commits ahead of `origin`. Push only when Michael asks.
- **Local only:** `docs/review-handoff.md` (an older review by another agent) stays uncommitted at Michael's request.

### Verified on the Elite

| Area | Result |
|---|---|
| Install and return to stock | `fm1_install.py` from stock and over itself; M-Vave's open workbench restores V016 with calibration intact |
| Flash backup | Two identical 1 MiB reads ([flash-backup.md](flash-backup.md)) |
| Bluetooth MIDI | Pairs with a Mac as `Chordpad37`, reconnects, long sessions beside USB audio |
| USB MIDI and audio | Port `Chordpad37 SMK-37`; USB audio plays to the line out |
| TRS MIDI out | Drives an FM-1, Program Change included |
| Keys | Velocity 1–127, no stuck notes over hundreds of notes |
| Pads | Velocity and pressure; after the peak fix (`3943490`), 0 of 109 hits under velocity 20 |
| Faders | CC 20–27 (fader 1 MIDI-learns in Surge XT); saved presets migrated from 64–71 |
| CHORD mode, chord pads, LATCH | Work in Surge XT; latched chords end cleanly |
| `<` `>` arrows | Program Change; the FM-1 follows, Surge XT ignores Program Change |

### Not yet tested on the keyboard

- **Knobs and pitch bend** on the M2–M4 firmware. They worked in the M1 bring-up app; nobody has turned them since.
- **SCALE mode, Easy Chord on the keys, ARP, auto-strum, PAD MIN, presets 2–8, KNOB / FADER BANK 2.** Host-tested only.
- **Pedal:** no pedal on hand.
- **Releasing notes on entering UBOOT** (`release_notes()` in `base/device/main_fm1.c`) with notes held.
- **SMK-37 Pro and MKE-P37 builds:** built, never installed.

## Open items

Small, known, none blocking:

1. **Mod wheel jitter:** it sends ±1 at rest (34/35, 51/52). Add a dead band in `wheels_input()` (`app/app.c`).
2. **K5 "CC ONLY" plays USB audio at half level:** `app_gain()` returns 2048 of a 0–4096 scale; full level is 4096.
3. **An expression pedal resting above 0 blocks auto-save:** the save condition in `input()` checks `!ui.pedal_val`.
4. **Arp and strum timing:** [plan.md](plan.md) says they run on the 1 kHz `app_tick`; they run from the main-loop
   input pass (`input()`), so their timing follows the loop.
5. **Program Change for PC synths:** if one ignores a bare Program Change, the arrows could send Bank Select (CC 0 / 32)
   first. Michael hasn't asked for it.
6. **The arrows' program number starts at 0 each power-on:** a synth can't report its patch. Persisting it per preset
   (`preset_t.rsv0` is free) is an option.

Michael's next step is more features; ask him which.

## Working on it

### Build (WSL Ubuntu-24.04, from `/mnt/c/GitHub/chordpad37`)

```bash
./build.sh
```

All three models, packages in `build/chordpad37/`. One model, faster:

```bash
export JIELI_TOOLCHAIN=$HOME/.jieli/toolchain AC79_SDK=$HOME/fw-AC79_AIoT_SDK
BOARD=smk37 MODEL=elite python3 base/tools/build.py app
```

Without the two variables, packaging fails (`uboot.boot: No such file`). The package is
`app/build/smk37-elite/chordpad-smk37-elite.fwsc`, identity `SMK-37 Elite_570100`. `build.py`'s size, RAM and
register checks must pass (RAM was 78,848 of 98,304 bytes at `29ccb99`).

The repo has `core.autocrlf=false` locally, so the shell scripts keep LF endings for WSL. Run WSL commands as
`wsl -d Ubuntu-24.04 bash -lc 'cd /mnt/c/GitHub/chordpad37 && ...'` (Git Bash mangles `wsl --cd /mnt/c/...`).

### Tests (WSL)

```bash
sh app/tests/run_tests.sh
```

```bash
sh tests/run_host_tests.sh
```

```bash
sh base/tests/run_tests.sh
```

The app tests run every `app/tests/*.txt` scenario in the host simulator (`expect`, `expect_out USB|BLE|TRS ...`,
`tap`, `tappad`, `fader`, `spin`, `turn`). `expect midi_out N` counts every message sent so far, so adding output earlier
in `bringup.txt` shifts the later counts. The installer tests also run on Windows: `python base/tests/install_test.py`.

### Install (Windows PowerShell, Python 3.13 with `mido` and `python-rtmidi`)

```powershell
python base/tools/fm1_install.py --info
python base/tools/fm1_install.py app/build/smk37-elite/chordpad-smk37-elite.fwsc --yes
```

- It installs from stock V016 (check step, then write) or over Chordpad37. Ask Michael before every flash.
- **After a flash, Surge XT must be restarted** (or the keyboard re-ticked in its MIDI settings): the keyboard
  reconnects USB and Surge doesn't reopen the port.
- `fm1_install.py` can't install the stock package (a 36-block container).

### Watching what the keyboard sends

The quickest check is a timed `mido` logger on Windows that opens every input whose name contains `SMK` and writes one
line per message. **Windows MIDI inputs are single-client:** while a logger holds the port, Surge XT gets nothing. Ask
Michael before recording, and say how long it runs.

### Back to stock

[open.m-vave.com](https://open.m-vave.com), the SMK-37 Elite page: tick **Force flash**, then **Restore factory V16**,
and confirm the target `SMK-37 Elite_016`. The purple Force flash button on its own installs M-Vave's "Elite Open V01"
instead. The stock package is also at `C:\GitHub\chordpad-SMK37\firmware\SMK-37 Elite_016.fwsc`.

### Hardware facts

- The Elite's flash is the FM-1's Puya part, JEDEC `0x856014` (the board accepts it and `0xEB6014`), chip key `0x980F`.
- K5 is the MASTER virtual pot: the platform reads it as the volume, and the app also sends it as a CC.
- The knobs are endless encoders.
- Pad LEDs are on SPI1 at PC8–PC10; TRS out is UART1 TX on PC7.
- Windows may show the audio device under the name of the first `1209:0001` device it saw on that port (here
  "GemmaOne FM-1") until it's uninstalled in Device Manager.

## Where things are

| What | Where |
|---|---|
| Controller, modes, LATCH, arrows, presets, screen | `app/app.c` (one unity file; `app/app.h` for store objects) |
| Note ownership and per-transport note-off retry | `app/note_refs.c` |
| Chords, scales, voicings, strum, arp | `app/chord_scale.c`, `app/easy_chord.c` |
| Preset store | `presets_store_t` in `app/app.c`, magic `CPD2`, one object of at most 3,840 bytes; `store_load()` migrates `CPD1` |
| TRS out driver | `base/bsp/boards/smk37/fm1_trs_tx.h` |
| Pad scan, velocity peak | `smk__pad()` in `base/bsp/boards/smk37/fm1_input.h` |
| MIDI send per transport | `plat_midi_send()` in `base/device/plat_fm1.c` |
| Bluetooth name, USB name | `ble_name` and `usb_product` in `app/fm1.toml` |
| Controls, LEDs, pins | [hardware-layout.md](hardware-layout.md) |
| What stock V016 sends | [stock-midi-map.md](stock-midi-map.md) |

## Rules

- **Flash:** never write `0x0–0x3FFF`, `0xC0000–0xC1FFF`, `0xF4000–0xFDFFF` or `0xFF000–0xFFFFF` (calibration,
  pairing, key data). `base/bsp/boards/smk37/fm1_flash_map.h` enforces it; don't weaken it. Store objects go to
  `0x9D000–0xAEFFF`.
- **Store:** at most 9 objects of 3,840 bytes each on the SMK-37.
- **Registers:** only inside `base/bsp/`; `build.py` rejects them anywhere else.
- **Licensing:** MPL-2.0 with a header on every file. Don't copy GPL code (Snoop37, `smk_uart_tx.h`). The port kit's
  text is CC BY-NC: write our own wording and credit it.
- **Backups:** flash dumps hold unit-unique data. They live in `C:\Users\no2ga\smk37-backups\`, never in a repository.
  `jl-uboot-tool` gets only `read`; never `erase`, `write`, `erasechip` or `burnchipkey`.
- **Git:** commit and push only when Michael asks.
- **Stop and review with Michael** if Bluetooth or the return to stock ever fails.
