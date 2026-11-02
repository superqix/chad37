# Chordpad37: pure MIDI controller firmware for the SMK-37, on Erik37

## Context

Build a pure MIDI controller firmware for the SMK-37 with Chordcade's Easy Chord on the keys and chords on the 16 pads. It replaces the `smk37_open` plan: that base is a groovebox with closed engine binaries, and most of it would have to come out.

**Base: a new fork of [genkigenki/erik37](https://github.com/genkigenki/erik37), proposed name `superqix/chordpad37`.** Erik37 is a small piano app on an MPL-2.0 platform layer with an SMK-37 board port. The piano sits in `app/`, so replacing that folder removes it. `base/` and `base/bsp/` are vendored files in this repo, despite the docs calling them submodules, so we can change them directly.

**Accepted trade-offs:**
- **No internal sound:** the DX7 and piano go. The headphone jack is silent unless USB audio playback works (tested in M1).
- **MIDI Suite stops working:** settings move to the device, and a web editor can follow later.
- **Experimental base:** Erik37 is at v1.2 and tested on an SMK-37 Pro only.
- **Bluetooth is unproven on this keyboard:** M1 tests it before any feature work.
- **Licensing:** our files and the base stay MPL-2.0. JieLi's Bluetooth libraries are closed binaries linked from the AC79 SDK. Before any public release, confirm their terms and the attributions.

## What stays, changes and goes

| Area | Files | Plan |
|---|---|---|
| Platform, USB, storage, screen, installer | `base/` | Keep |
| SMK-37 board, flash guards | `base/bsp/boards/smk37/` | Keep; add TRS out and pad velocity |
| Piano engine, samples, fonts, site | `app/dsp`, `app/assets`, `design/`, `tools/`, `LISTEN.md`, `SOURCES.md` | Delete |
| Piano app | `app/app.c`, `ui.c`, `draw.c`, `boot.c` | Replace, starting from `base/template/` |
| Build script | `build.sh` | SMK-37 models only, no FM-1 build |
| Easy Chord | `app/easy_chord.c/.h`, tests, vectors | Copy from `chordpad-SMK37`, branch `easy-chord`; header becomes MPL-2.0 |
| Stock capture sheet | `docs/stock-midi-map.md` | Copy from the same branch |

`app/fm1.toml`: name `CHORDPAD`, USB product `Chordpad37 SMK-37`, `ble = true` with `ble_name` `Chordpad37`, and our own unused `smk_id_prefix`.

## Milestones

### M0: Setup and capture stock behaviour (needs you)
1. **Create the fork** on GitHub and clone it to `~/GitHub/chordpad37`.
2. **Set up the build on the PC under WSL2:** run `base/tools/get_toolchain.sh`, clone the AC79 SDK (`AC79NN_SDK_V1.2.1_2023-12-13`), and install Python with Pillow and NumPy. The Windows JieLi setup from the old plan doesn't apply. The Mac has no Docker or podman, so it runs host tests only for now.
3. **Fill in `docs/stock-midi-map.md` while still on stock V016.** Check whether the PC session already did this. This sheet is the spec for M3.
4. **Confirm the model:** Pro, Elite or MKE. Each needs its own package.

### M1: Early proof, with a stop point
1. Build unmodified Erik37 to prove the toolchain.
2. **Take the full flash backup before installing** (Method A in the port kit's `02-update-and-recovery.md`): run only the update's check step, send the UBOOT key, then read the flash twice with `jl-uboot-tool`. Check that the SHA-256 values match and `trimdat` is at `0xC1224`. `fm1_install.py` has no check-only option, so add one. Skip this if the PC session already has a verified dump.
3. Install a minimal app based on `base/template/`, with Bluetooth on. It shows every control on screen and sends keys over USB and Bluetooth.
4. **Test and record:**
   - **Bluetooth MIDI:** pairing, reconnecting after power-off, latency, and no crash with the radio running.
   - **USB audio:** does it appear on the computer, and does playback reach the headphone jack?
   - **K1–K8:** order and direction. The source marks the order "to confirm".
   - **K5:** it's a virtual pot that also drives MASTER. Can it send a CC without affecting audio level?
   - **The rest:** faders, wheels, pedal, and input-to-MIDI latency.
   - **Return to stock:** M-Vave's updater restores V016 with calibration intact.

**Stop and review together if Bluetooth or the return to stock fails.** If only Bluetooth fails, the fallback is a USB and TRS build with `--no-ble`. If the return to stock fails, nothing else proceeds until it's fixed.

The first install clears stock settings in `0x9D000–0xAEFFF`, though stock rebuilds them on return. Calibration, pairing and key data are never written, and `fm1_flash_map.h` enforces this at compile time and again in the flash driver.

### M2: Platform additions (`base/`)
- **TRS MIDI out:** a new driver in `base/bsp/boards/smk37/`, because the build rejects register access anywhere else.
  - Written fresh from the documented register setup. Don't copy GPL `smk_uart_tx.h`. The port kit's text is CC BY-NC, so write our own wording and credit it.
  - Non-blocking queue, drained from the 100 µs scan tick. The 1 kHz tick suggested in the kit would smear a 5-note chord over 15 ms.
  - Add `TO_TRS` to `plat_midi_send`, use `FELUCCA_LOADER` guards, and add it to the host simulator. Leave `FM1B_TRS` (the FM-1's MIDI in) off.
- **Pad velocity and pressure:** `plat_pad_vel(p)` and `plat_pad_press(p)`. Velocity currently reaches only the white-key mapping, not the app in `SMK_PADS_OWN` mode. Pressure is already in `fm1_in.pad_press[]`.

### M3: Controller layer (`app/`)
- **Input and output:** read `plat_keybed_hits`, `plat_keybed_vel`, pads, `plat_enc`, `plat_fader`, `plat_wheels`, `plat_pedal_raw` and buttons. Send everything to USB, Bluetooth and TRS through a single `send()`. Erik37's `app.c` is a reference for reading these inputs.
- **Knobs and faders:** two banks on the KNOB BANK and FADER BANK buttons, with CC numbers, ranges and absolute or relative behaviour from the M0 sheet. Faders get a dead band.
- **Buttons, wheels, pedal, octave:** send what stock sends. OCT− and OCT+ held for 5 s stays reserved for update mode.
- **Presets:** 8 presets in one `OBJ_PRESETS` store object.
  - The SMK-37 store allows 9 objects at 3,840 bytes each, so 8 presets need one object (about 260 bytes each, plus header).
  - A flash write briefly stops everything, so save only after 2 seconds with nothing held.
- **Screen:** new small UI showing preset, channels, last control and value, chord name, and settings pages.
- **Audio and MIDI in:** `app_render` outputs silence, and incoming MIDI is read and dropped.

### M4: Easy Chord and chord pads

| | Chord mode off | Chord mode on |
|---|---|---|
| Keys | Normal notes | Easy Chord: root plus modifier keys |
| Pad, no keys held | Drums on channel 10 | Plays the stored chord; empty pads are silent |
| Pad, keys held | Drums | Stores the held chord, without sounding it |

- **Mode button:** the printed CHORD button toggles chord mode.
- **Chord logic:** `easy_chord_resolve` runs on a 128-bit held set. Unchanged from `d5d6753` except the license header and names adjusted for the unity build, since `app.c` is compiled as one unit with the platform.
- **Note tracking:** new `app/note_refs.c`, a pure reference count per channel and note. Morphing changes only the tones that differ, and overlapping chords and pads can't cause stuck notes. Mode change, preset change, USB disconnect and panic release everything.
- **Storing:** with several roots, the highest is stored. Each preset holds 16 chords as root and type, and overwriting replaces a pad.
- **Output:** chord pads play on the key channel, using `plat_pad_rgb` to show stored and playing chords.

### Later, separate decisions
Web preset editor over `APP_SYSEX`, arp, scale, note repeat, pads 17–32, Chordcade strum and voicings, USB-to-TRS thru, and a public release.

## Verification

**Host tests (Mac or PC, no hardware):**
- `tests/run_host_tests.sh`: all 940 Easy Chord vectors under ASan and UBSan, plus new `note_refs` tests.
- `BOARD=smk37 base/tools/build.py app --host`, then scripted scenarios in `app/tests/` using `tap`, `pad`, `fader`, `expect_out`. These cover knobs and banks, chord morphing, store and recall, persistence over a simulated reboot, and no stuck notes after a mode change.
- `base/tests/run_tests.sh` still passes, including flash and installer tests.

**Each firmware build (WSL):** `build.py`'s size, RAM and register checks pass.

**On the keyboard:**
- **Against M0:** repeat the capture on all transports and explain each difference.
- **TRS:** to real hardware, with a logic or MIDI monitor confirming timing and a 5-note chord under 5 ms.
- **DAW:** a session over USB and Bluetooth.
- **Chords:** every type sends the right notes. Stored chords survive a power cycle and preset change.
- **Stress:** mode change, preset change, and USB or Bluetooth disconnect while notes are held.
- **Recovery:** return to V016 and reinstall, with calibration and pairing intact.

## Risks

- **Bluetooth on the SMK-37:** code runs on the FM-1 but I found no SMK-37 test, so M1 tests it first. The radio's RAM use with the rest of the app is unmeasured.
- **Bricking:** the base has boot-failure counting and the update-mode key combination. An unbrick dongle hasn't been tried on the SMK-37, so the verified backup comes before the first install.
- **K5 and USB audio:** both depend on M1. If K5 can't be separated from MASTER, it either stays a volume knob or USB audio goes.
- **Controls differ from the source's notes:** knob order and other details may vary by unit, so M1 checks them on yours.
- **Upstream changes:** we'll be changing `base/` directly, so upstream updates become deliberate merges.
- **Access:** I can do code and host tests on the Mac. Firmware builds need Docker on the Mac or access to WSL, and flashing and hands-on tests need you.
