# Chad37: pure MIDI controller firmware for the SMK-37, on Erik37

## Context

Build a pure MIDI controller firmware for the SMK-37 with Chordcade's Easy Chord on the keys and chords on the 16 pads. It replaces the `smk37_open` plan: that base is a groovebox with closed engine binaries, and most of it would have to come out.

**Base: a new fork of [genkigenki/erik37](https://github.com/genkigenki/erik37), proposed name `superqix/chordpad37`.** Erik37 is a small piano app on an MPL-2.0 platform layer with an SMK-37 board port. The piano sits in `app/`, so replacing that folder removes it. `base/` and `base/bsp/` are vendored files in this repo, despite the docs calling them submodules, so we can change them directly.

**Accepted trade-offs:**
- **No internal sound:** the DX7 and piano go. The headphone jack is silent unless USB audio playback works (tested in M1).
- **MIDI Suite stops working:** settings move to the device, and a web editor can follow later.
- **Experimental base:** Erik37 is at v1.2 and tested on an SMK-37 Pro only.
- **Bluetooth on this keyboard:** unproven when this plan was written; M1 proved it on an SMK-37 Elite.
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

`app/fm1.toml`: name `CHAD37`, USB product `Chad37 SMK-37`, `ble = true` with `ble_name` `Chad37`, and our own unused `smk_id_prefix`.

## Milestones

### M0: Setup and capture stock behaviour (needs you)
1. **Create the fork** on GitHub and clone it to `~/GitHub/chordpad37`.
2. **Set up the build on the PC under WSL2:** run `base/tools/get_toolchain.sh`, clone the AC79 SDK (`AC79NN_SDK_V1.2.1_2023-12-13`), and install Python with Pillow and NumPy. The Windows JieLi setup from the old plan doesn't apply. The Mac has no Docker or podman, so it runs host tests only for now.
3. **Fill in `docs/stock-midi-map.md` while still on stock V016.** Check whether the PC session already did this. This sheet is the spec for M3.
4. **Confirm the model:** Pro, Elite or MKE. Each needs its own package.

### M1: Early proof, with a stop point
1. Build unmodified Erik37 to prove the toolchain.
2. **Take the full flash backup before installing** (Method A in the port kit's `02-update-and-recovery.md`): run only the update's check step, send the UBOOT key, then read the flash twice with `jl-uboot-tool`. Check that the SHA-256 values match and `trimdat` is at `0xC1224`. `fm1_install.py --check-only --uboot` does the first two, `tools/smk_backup.py` the reads and checks ([flash-backup.md](flash-backup.md)).
3. Install a minimal app based on `base/template/`, with Bluetooth on. It shows every control on screen and sends keys over USB and Bluetooth.
4. **Test and record:**
   - **Bluetooth MIDI:** pairing, reconnecting after power-off, latency, and no crash with the radio running.
   - **USB audio:** does it appear on the computer, and does playback reach the headphone jack?
   - **K1–K8:** order and direction. The source marks the order "to confirm".
   - **K5:** it's a virtual pot that also drives MASTER. Can it send a CC without affecting audio level?
   - **The rest:** faders, wheels, pedal, and input-to-MIDI latency.
   - **Return to stock:** M-Vave's updater restores V016 with calibration intact.

**Results, 2026-11-02, on an SMK-37 Elite (V016, flash JEDEC `0x856014`, the FM-1's Puya part): M1 passed.**

| Check | Result |
|---|---|
| Toolchain | Proven by our own Elite build. The unmodified Erik37 build was skipped. |
| Flash backup | Two identical 1 MiB reads, `trimdat` at `0xC1224` ([flash-backup.md](flash-backup.md)). |
| Install | `fm1_install.py` from stock and over itself. |
| Bluetooth MIDI | Pairs with a Mac as `Chordpad37` (the name then), and reconnects after a power-cycle. Long sessions driving Surge XT, with MIDI Learn and USB audio running at the same time, without a crash. |
| USB audio | Appears and plays to the line out. Using USB and the line out into the same speakers gave a ground-loop hum. Windows keeps the first name it saw for `1209:0001` on a port (here a GemmaOne FM-1) until the device is uninstalled. |
| K1–K8 | Panel order, CC 70–77 in the bring-up app. Right raises all eight. They are endless: the app's value moves a few steps per detent. |
| K5 | It is MASTER: the platform reads it as the volume, and it scales USB audio playback. Whether it stays the volume or becomes a CC only (USB audio then at a fixed level) is an M3 decision. |
| Faders, wheels | Full range. Pitch bend reached 1 at M1 (subsequently fixed in `app/app.c` to reach 0 at −127). |
| Pedal | Not tested: no pedal. |
| Latency | None noticeable over USB or Bluetooth, by feel. |
| Return to stock | M-Vave's open workbench (open.m-vave.com, Elite page: Force flash ticked, then Restore factory V16, target `SMK-37 Elite_016`). Stock came back with velocity, pitch-bend rest and pads as captured in M0. |

**Stop and review together if Bluetooth or the return to stock fails.** If only Bluetooth fails, the fallback is a USB and TRS build with `--no-ble`. If the return to stock fails, nothing else proceeds until it's fixed.

The first install clears stock settings in `0x9D000–0xAEFFF`, though stock rebuilds them on return. Calibration, pairing and key data are never written, and `fm1_flash_map.h` enforces this at compile time and again in the flash driver.

### M2: Platform additions (`base/`) — completed
- **TRS MIDI out (`base/bsp/boards/smk37/fm1_trs_tx.h`):**
  - Clean-room MPL-2.0 UART1 TX driver on `PC7` at 31,250 baud (`PLL_48M / 31250 / 4 - 1 = 383`), routed via Output Channel 1 (`IOMAP_CON3` bits `[23:20] = 1`, bit 7 cleared; `PC7` `DIR = 0, PU = 1, PD = 1, OUT = 0, DIE = 1`) so it never touches `IOMAP_CON1` used by the WS2812 pad LED driver.
  - Non-blocking 256-byte ring (`SMK_TRS_TXQ`), kicked immediately when idle and drained from the 10 kHz (100 µs) `TIMER5` scan ISR so a 5-note chord serializes back-to-back in ~4.8 ms without smearing across 1 ms ticks.
  - Added `TO_TRS` (`1u << PORT_TRS`) to `plat_midi_send`, guarded device TRS out with `#ifndef FELUCCA_LOADER`, wired `smk_trs_tx_pending()` into `release_notes()`, added `TO_TRS` to the host simulator, and left `FM1B_TRS` (the FM-1's MIDI in) disabled on `BOARD_SMK37`.
- **Pad velocity and pressure:** exposed `plat_pad_vel(int pad)` (per-pad strike velocity from `fm1_in.pad_vel[]`, captured at peak scan) and `plat_pad_press(int pad)` (live aftertouch pressure `0..127` while held, `0` when up) on both the device (`plat_fm1.c`) and the host simulator (`plat_host.c`).

### M3: Controller layer (`app/`) — completed
- **Input, output, and reference-counted note tracking (`app/note_refs.c`, `app/app.c`):**
  - Implemented `app/note_refs.c` (with host unit tests in `tests/note_refs_test.c`) for reference-counted `(channel, note)` ownership, chord morphing, and independent per-transport (`USB`, `BLE`, `TRS`) Note-Off overflow recovery.
  - Reads `plat_keybed_hits`, `plat_keybed_vel`, pads (`plat_pad_hits`, `plat_pads_held`, `plat_pad_vel`, `plat_pad_press`), `plat_enc`, `plat_fader`, `plat_wheels`, `plat_pedal_raw` and buttons (`plat_buttons`, `plat_smk_buttons`), sending all MIDI messages to `TO_USB | TO_BLE | TO_TRS` through a single `send()`.
- **Knobs and faders (from `docs/stock-midi-map.md`):**
  - Two knob banks (`KNOB BANK`): Bank 1 (`K1–K6` = CC 48–53 on ch 6 abs `0–127`, `K7` = CC 38 ch 1 rel `69`/`1`, `K8` = CC 37 ch 1 rel `69`/`1`) and Bank 2 (`K9–K16` = CC 30–37 on ch 1 rel `69`/`1`). `K5` supports both `CC+VOL` (default) and `CC ONLY` (USB audio at fixed level via `APP_GAIN`).
  - Two fader banks (`FADER BANK`): Bank 1 (`F1–F4` = CC 20–23 ch 1) and Bank 2 (`F5–F8` = CC 24–27 ch 1), with baseline rest pickup at boot and no jump on bank switch. Stock sends CC 64–71, but 64–67 are the sustain, portamento, sostenuto and soft pedals, so a synth took fader 1 as sustain and wouldn't MIDI-learn it; 20–27 are undefined in the MIDI standard. A saved "CPD1" store moves faders still on 64–71 to the new defaults once.
- **Pads, buttons, wheels, pedal, octave:**
  - Pads send per-pad strike velocity and Channel Pressure (`0xD0 | pad_ch`) once pressure changes from the initial strike reading, returning to `0` before the last pad's Note-Off. Supports `STOCK` (ch 2, notes `16–19, 111–122` with full velocity), `DRUMS` (ch 10, notes `36–51`, default on Preset 2), and `V016` (Pads 1–4 velocity 0 with no release).
  - `<` and `>` (under the pad bank) send Program Change −1 / +1 on `key_ch` to step the synth's patch, shown as 1–128; the number starts at 0 each session (a synth can't report its patch). Pages are reached by their own buttons.
  - `SEQ PLAY` sends CC 56 (`127` down / `0` up) and `SEQ REC` sends CC 57 (`127` down / `0` up) on `key_ch`.
  - Pedal supports `AUTO` (auto-learned open/closed rest, ignoring floating middle readings), `OPEN`, `CLOSED` (CC 64), and `EXPR` (CC 11). `OCT−` and `OCT+` held 5 s remains reserved for update mode.
- **Presets (`OBJ_PRESETS`):**
  - 8 presets + global header stored in a single `OBJ_PRESETS` flash object (`<= 3,840` bytes), auto-saved only after 2 s since the last edit and 1.5 s of silence with nothing held or pending, plus manual save via `BT` and quick `PATCH + Pad 1–8` preset selection.
- **Screen:** 3-strip UI (`MAIN`, `EDIT`, `GLOBAL` pages) showing active preset, bank badges, BLE status, last control/value/channel/hex packet, played note/chord name, knob/fader grids, mini 37-key and 16-pad indicators, and settings pages.
- **Audio and MIDI in:** `app_render` outputs silence, and incoming MIDI is read and dropped.

### M4: Chord mode, Scale mode, Arpeggiator, and Global menu (completed)

#### 1. Mode Matrix (`CHORD` = `B_ENV`, `SCALE` = `B_SEL`)

| Mode State | 37-Key Keybed (`C3..C6`) | 16 RGB Pads (`P0..P15`) |
|---|---|---|
| **CHORD off, SCALE off** | Normal chromatic notes | Drums / stock pad notes on channel 10 |
| **CHORD on, SCALE off** | Normal chromatic notes + **Manual Fingered Chord detection** (recognized chord name & notes shown and latched on screen) | Plays the preset's **16 user-assigned custom chords** on the key channel (empty pads are silent) |
| **CHORD on, SCALE on** | **Easy Chord** (`easy_chord_resolve`: root key + 2 white / 2 black modifier keys below it), transposed by the `K1` Scale Root offset; played chord shown and latched on screen | Plays the preset's **16 user-assigned custom chords** on the key channel |
| **CHORD off, SCALE on** | **White keys play notes in key** (sequential scale degrees `1st..7th` per octave starting from the selected root on `C`); **black keys are muted** | Plays **auto-generated diatonic chords in the active key/scale** (Bottom row `P8..P15` = degrees `I..VII` + `I'` triads; Top row `P0..P7` = 7th/extended chords, shaped by `K3` Voicing and `K4` Pad Chord Flavor) |

#### 2. Pad Assignment Workflow (`SEQ REC` = `B_REC`)
- **Latch on play:** In `CHORD` mode (`CHORD` only or `CHORD + SCALE`), playing a chord on the keys displays its detected name and notes on screen and latches it as the last played chord (so keys can either remain held or be released before assigning).
- **Arm & assign:** Press **SEQ REC** (`B_REC`) to arm pad assignment (`SEQ REC` red LED lights and pads blink red). Tap any pad (`P0..P15`) to store the latched/held chord to that pad without sounding a drum note; **SEQ REC automatically turns off** after the assignment.
- **Stored chord format:** Each pad stores up to **6 raw MIDI notes** plus detected `(root, type)` metadata so both arbitrary manual fingered voicings and Easy Chords (highest root wins if multiple Easy Chords are held) are preserved accurately.

- **Latch (`NOTE REPEAT` = `B_LFO`):** while the pads play chords (`CHORD` on, or `SCALE` alone), `NOTE REPEAT` toggles LATCH (its light, and `LAT` in the header). A tapped chord pad keeps sounding after it is let go; another pad switches to its chord (an empty pad just stops it); the latched pad again stops it; LATCH off, a mode or preset change, or `FX` panic lets it go. With `ARP` on, the arpeggiator keeps running on the latched chord.

#### 3. Scale Mode & Knobs (`SCALE` = `B_SEL`, `PATCH` = `B_HOME`)
- **Page & knob focus:** Pressing **SCALE** turns Scale mode on (lights `SCALE` LED) and opens the **Scale Page** on screen, assigning `K1..K4` to edit Scale parameters:
  - **K1 (Key / Root):** `C, C#, D, D#, E, F, F#, G, G#, A, A#, B`
  - **K2 (Scale Type):** `Major, Natural Minor, Harmonic Minor, Dorian, Phrygian, Lydian, Mixolydian, Pentatonic Major, Pentatonic Minor`
  - **K3 (Voicing / Inversion):** `Root, 1st Inversion, 2nd Inversion, Spread`
  - **K4 (Pad Chord Flavors):** `Triads, 7ths, 9ths`
- **Returning to MIDI CC control:** Pressing **PATCH** (`B_HOME`) returns the screen to the Main/Controller view while keeping `SCALE` active in the background, restoring `K1..K4` to normal MIDI CC output. Pressing `SCALE` while already on the Scale page turns Scale mode off (pressing `SCALE` from another page while Scale is active focuses the Scale page first).

#### 4. Arpeggiator (`ARP` = `B_ARP`)
- **Behavior:** When `ARP` is on, holding one or more chord pads (and/or keys) arpeggiates through the active chord tones (or steps through whole chords across held pads).
- **Clocking:** Driven by the `1 kHz` `app_tick` internal clock, with automatic sync to incoming external MIDI Clock (`0xF8`) when present.
- **Controls:** Pressing **ARP** turns ARP on and opens the **ARP Page** (`K1 = BPM`, `K2 = Rate/Division`, `K3 = Pattern/Mode`, `K4 = Gate/Octave range`; pressing **PATCH** returns to the Main/CC page with `ARP` still active).
- **ARP Modes:** `Up`, `Down`, `Up/Down`, `Random`, and `Full-Chord Repeat / Progression` (steps across held pads as full chords).

#### 5. Global Menu (`GLOBE` = `B_GLO`)
- Pressing **GLOBE** opens the Global Settings menu, exposing:
  - **Strum Chords (Auto-Strum):** Staggered note-on delay (`0..120 ms` per chord tone, `Up / Down / Alt`) when triggering a chord pad or Easy Chord, queued non-blockingly via `app_tick`.
  - **Chord Voicing:** `Root, 1st Inversion, 2nd Inversion, Spread` (shared with `K3` on the Scale page).
  - **ARP Mode & Settings:** Pattern, Rate/Division, Gate, and Octave range.

#### 6. Note Tracking, Persistence, and Pad RGB Feedback
- **Note tracking:** New `app/note_refs.c`, a pure reference count per channel and note. Morphing changes only the tones that differ, and overlapping chords, strums, arpeggiator steps, and pads cannot cause stuck notes. Mode change, preset change, USB/BLE disconnect, UBOOT entry, and panic release everything.
- **Persistence (`OBJ_PRESETS`):**
  - **Per-preset:** Each of the 8 presets stores its 16 custom-assigned pad chords (up to 6 notes + root/type per pad) and controller mappings.
  - **Global device-wide (in the store header):** Scale (`Key`, `Scale Type`, `Pad Flavor`), `Voicing`, `Auto-Strum`, and `ARP` settings persist globally across preset changes.
- **Pad RGB LED states (`plat_pad_rgb`):**
  - Unassigned custom pad (`CHORD` mode): Off `(0, 0, 0)`
  - Assigned custom chord pad: Dim teal/blue
  - Diatonic scale chord pad (`SCALE`-only mode): Amber/green (with tonic/root degrees highlighted brighter)
  - `SEQ REC` armed: Blinking red
  - Active / playing / arpeggiating pad: Bright white/orange

### After M4 (2026-11-07, on the Elite)
- **Pad velocity:** the peak is read while the reading still rises (up to 6 samples, about 14 ms), since quick taps
  were read at the curve's floor; `PAD MIN` on the hardware page (K6) sets a velocity floor.
- **Faders** moved to CC 20–27 (see M3), **LATCH** on `NOTE REPEAT` (see M4 §2), and the `<` `>` arrows send Program
  Change (see M3).
- Open items and what's untested on the keyboard: [handoff.md](handoff.md).

### Later, separate decisions
Web preset editor over `APP_SYSEX`, note repeat (its button is LATCH now), pads 17–32, manual wheel strum, USB-to-TRS
thru, and a public release.

## Verification

**Host tests (Mac or PC, no hardware):**
- `tests/run_host_tests.sh`: all 940 Easy Chord vectors under ASan and UBSan, plus new `note_refs`, scale mapping, voicing/strum, and arpeggiator tests.
- `BOARD=smk37 base/tools/build.py app --host`, then scripted scenarios in `app/tests/` using `tap`, `pad`, `fader`, `expect_out`. These cover knobs and banks, `CHORD` vs `SCALE` mode matrix, `SEQ REC` pad assignment and disarm, diatonic scale pads and white-key scale mapping, ARP patterns, auto-strum, persistence over a simulated reboot, and no stuck notes after a mode change.
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

- **Bluetooth on the SMK-37:** M1 proved pairing, reconnect, and long sessions on an SMK-37 Elite. Keep watching RAM use as the controller and chord layers are added.
- **Bricking:** the base has boot-failure counting and the update-mode key combination. An unbrick dongle hasn't been tried on the SMK-37, so the verified backup comes before the first install.
- **K5 and USB audio:** both depend on M1. If K5 can't be separated from MASTER, it either stays a volume knob or USB audio goes.
- **Controls differ from the source's notes:** knob order and other details may vary by unit, so M1 checks them on yours.
- **Upstream changes:** we'll be changing `base/` directly, so upstream updates become deliberate merges.
- **Access:** I can do code and host tests on the Mac. Firmware builds need Docker on the Mac or access to WSL, and flashing and hands-on tests need you.
