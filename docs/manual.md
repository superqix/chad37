# Using Chad37

A complete guide to the M-Vave SMK-37 running Chad37: what every control does, what MIDI it sends, and how the chord, scale, and arpeggiator modes work in detail. To install or update the firmware, see [install.md](install.md).

---

## Contents

1. [The Screen](#1-the-screen)
2. [Buttons & Shortcuts](#2-buttons--shortcuts)
3. [Pad LED Color Guide](#3-pad-led-color-guide)
4. [MIDI Implementation & Controls](#4-midi-implementation--controls)
5. [Presets](#5-presets)
6. [Chord Mode & Chord Banks](#6-chord-mode--chord-banks)
7. [Easy Chord](#7-easy-chord)
8. [Voicings & Auto-Strum](#8-voicings--auto-strum)
9. [Scale Mode](#9-scale-mode)
10. [Arpeggiator & 16-Step Rhythm Editor](#10-arpeggiator--16-step-rhythm-editor)
11. [Menus & Settings Reference](#11-menus--settings-reference)
12. [Saving, Calibration & Firmware Mode](#12-saving-calibration--firmware-mode)

---

## 1. The Screen

Chad37 drives the 240 × 240 screen in two layouts: the **standard 3-strip layout** (header, body, footer) used on the main view and settings pages, and the full-screen **chord view** used in CHORD and Easy Chord modes.

### Header
The top bar shows:
- **Preset name** on the left, followed by `*` when there are unsaved changes waiting to be written to flash.
- **Active mode badges:** `LAT` (when LATCH is holding a pad chord) or `CHD` (when CHORD or Easy Chord is on), `SCL` (when SCALE or Easy Chord is on), `ARP` (when the arpeggiator is on, or `K1` / `K9` showing the active knob bank when ARP is off), and `F1-4` / `F5-8` for the fader bank. When **SEQ REC** is armed in CHORD mode, the badges are replaced with `REC PAD`.
- **BLE** on the right, lit bright when a Bluetooth MIDI host is connected.
- **Toast banner:** turning a knob on a menu page or pressing a button briefly replaces the header with the control's name and new value for 1.3 seconds.

### Main View
Press **PATCH** at any time to return to the main view:
- **Top-left box:** the last control moved or note played, its value, the raw 3-byte MIDI message in hex, and the MIDI channel (`CH1`–`CH16`).
- **Top-right box:** the current playing mode (`NOTE`, `CHORD`, `EASY CHORD`, or `SCALE`) and the name of the last note or chord played.
- **Middle grid:** live values for the 8 knobs in the current bank (`K1`–`K8` or `K9`–`K16`), the 4 faders (`F1`–`F4` or `F5`–`F8`), Pitch Bend (`PB`), Modulation (`MOD`), Pedal (`PDL`), and USB Audio Volume (`VOL`).
- **Big dial / fader overlay:** turning a knob or moving a fader on the main view opens a large animated dial or fader over the middle of the screen showing the control's name, value, CC number, and MIDI channel (plus `RELATIVE` if a migrated preset kept a relative knob). It closes automatically 1.5 seconds after the last movement.

### Footer
The bottom strip is visible on the main view and all menu pages:
- **37-key mini strip:** lights each physical key as you hold it.
- **16-pad mini strip:** shows all 16 pads (`1`–`16` from left to right) and highlights held pads.
- **Status line:** current octave shift (`OCT -2` to `OCT +2`), `KEY CH`, `PAD CH`, and the active page tag (`MAIN`, `EDIT`, `GLOB`, `SCAL`, `ARP`, `HW`).

### Chord View
When **CHORD** or **Easy Chord** is active on the main or scale page, the screen switches to the full-screen chord view:
- **Header:** the mode (`CHORD` or `EASY CHORD`), the active chord bank (`BANK 1`–`16`), preset (`P1`–`P8`), MIDI channel (`CH 1`, or `CH 1/2` when chord pads have their own channel), and a Bluetooth connection dot on the far right.
- **Chord readout:** the detected or played chord name in large type, with the individual note names listed underneath (or the note count if more than a few notes are sounding).
- **Live piano keyboard:** a proportional keyboard with labelled `C` octaves that highlights every sounding MIDI note in real time—including generated chord tones, inversions, spread/doubled voicings, and strummed notes. While the arpeggiator runs, it shows the whole chord the arp is playing from, held steady, rather than each step. Its range expands automatically to fit wide voicings and holds steady after you let go.
- **16-pad bar & hints:** a row of 16 pad indicators at the bottom (teal for pads with a stored chord, orange while held, dark when empty) along with prompts for **SEQ REC** assignment, pad erasing, and bank deletion.

---

## 2. Buttons & Shortcuts

### Panel Buttons

| Button | What it does | Button LED |
| :--- | :--- | :--- |
| **CHORD** | Cycles **CHORD** → **Easy Chord** → **Off** (from SCALE mode, goes straight to CHORD). | Lit when CHORD or Easy Chord is on |
| **SCALE** | Turns **SCALE** mode on (and opens the Scale page) or off. In **Easy Chord**, opens and closes the Scale page to adjust Voicing (`K3`) without leaving Easy Chord. | Lit in SCALE mode |
| **ARP** | Turns the **Arpeggiator** on and opens the ARP page. If already on the ARP page, turns ARP off. | Lit when ARP is on |
| **NOTE REPEAT** | **LATCH / HOLD:** latches chord pads in CHORD and SCALE modes, and holds keybed chords when ARP is on. | Lit when LATCH or HOLD is active |
| **SEQ REC** | **In CHORD / Easy Chord:** cycles **ASSIGN** → **ERASE** → **Off**. **On ARP page:** toggles **STEP EDIT** on and off. **Otherwise:** sends momentary CC 57 (`127` pressed, `0` released) on the key channel. | Lit when armed, editing steps, or held |
| **SEQ PLAY** | Sends momentary CC 56 (`127` pressed, `0` released) on the key channel. | Lit while held |
| **SEQ** | **Tap tempo:** tap two or more times in time to set the arpeggiator's BPM (`40`–`240`) and align the next beat. | Off |
| **PATCH** | Returns to the **main view** from any menu page. Hold **PATCH** to view and switch presets on pads `1`–`8`. | Off |
| **PARA** | Opens and closes the **EDIT** page (preset, key channel, pad channel, pad map). | Lit on the EDIT page |
| **GLOBE** | Cycles **GLOBAL** page (strum, voicing, chord pad channel) → **HARDWARE SETUP** page → **main view**. | Lit on GLOBAL or HARDWARE SETUP |
| **BT** | Saves all unsaved changes immediately (`SAVED`). Chad37 also auto-saves after 2 seconds of idle time. | Lit while there are unsaved changes (`*`) |
| **FX** | **Panic:** immediately turns off all sounding notes, latched pads, held arp chords, and pending strums (`PANIC ALL OFF`). | Off |
| **OCTAVE − / +** | Shifts the keybed and scale pads down or up by 1 octave (`-2` to `+2` octaves). | `OCT-` lit below `0`; `OCT+` lit above `0` |
| **KNOB BANK** | Toggles the 8 encoders between **Bank 1** (`K1`–`K8`) and **Bank 2** (`K9`–`K16`). | Lit on Bank 2 (`K9`–`K16`) |
| **FADER BANK** | Toggles the 4 faders between **Bank 1** (`F1`–`F4`) and **Bank 2** (`F5`–`F8`). | Lit on Bank 2 (`F5`–`F8`) |
| **< / >** | **In CHORD / Easy Chord:** selects **Chord Bank `1`–`16`**. **In other modes:** sends MIDI **Program Change** down/up (`1`–`128`) on the key channel. | Off |

### Button Combinations

| Combination | Action |
| :--- | :--- |
| **Hold PATCH + Tap Pad 1–8** | Immediately switches to **Preset 1–8** without sounding a pad note. |
| **Press `<` + `>` together** *(in CHORD / Easy Chord)* | Asks `DELETE BANK n?` on screen. Press **`<` + `>` together again** within 5 seconds to clear all 16 pads in the current bank, or press either **`<`** or **`>`** alone to cancel. |
| **Hold `OCTAVE −` + `OCTAVE +` for 5 seconds** | Restarts into the chip's own bootloader (a USB disk named `WL82 UBOOT`), for a full flash backup. You don't need it to install or update; see [section 12](#12-saving-calibration--firmware-mode). Let go before 5 seconds to cancel. |
| **Hold `OCTAVE −` + `OCTAVE +` while switching power on** | Re-teaches the button and knob map, a recovery tool you shouldn't need; see [section 12](#12-saving-calibration--firmware-mode). |

---

## 3. Pad LED Color Guide

The 16 RGB pads change color to show the current mode, stored chords, preset status, or arpeggiator rhythm steps. Physically, **Pads 1–8** are the **top row** (left to right) and **Pads 9–16** are the **bottom row** (left to right).

| Context / Mode | Pad State | Pad LED Color |
| :--- | :--- | :--- |
| **Any playing mode** | Pad currently held, latched, or sounding in `CHORD` arp mode | **Bright Orange** |
| **Hold PATCH** *(Preset picker)* | Active preset (Pads 1–8) | **White** |
| | Used preset (Presets 1 & 2, plus any preset selected previously) | **Red** |
| | Unused preset (Pads 3–8 never selected yet) | **Green** |
| **NOTE mode** *(CHORD & SCALE off)* | Idle pad | **Off** |
| **CHORD / Easy Chord** | Pad has a saved chord in the current bank | **Teal / Blue** |
| | Empty pad in the current bank | **Off** |
| | **SEQ REC** pressed once (`ASSIGN` mode): all 16 pads | **Blinking Red** |
| | **SEQ REC** pressed twice (`ERASE` mode): pads with a chord | **Blinking Orange** *(empty pads stay off)* |
| **SCALE mode** | Tonic / Root pads (`Pad 1` and `Pad 8` on top; `Pad 9` and `Pad 16` on bottom) | **Bright Amber** |
| | Scale degrees `II`–`VII` (`Pads 2–7` on top; `Pads 10–15` on bottom) | **Dim Green** |
| **ARP Step Edit** *(`SEQ REC` on ARP page)* | Currently playing step (playhead while arp is running) | **White** |
| | Normal note step | **Teal / Blue** |
| | Accented note step (`+40` velocity) | **Bright Amber** |
| | Rest step (silent) | **Dim Red** |
| | Steps past the pattern's length (`K8 STEPS`) | **Off** |

---

## 4. MIDI Implementation & Controls

Every MIDI message is sent simultaneously over **USB MIDI**, **Bluetooth MIDI** (on the SMK-37 Elite), and the rear **3.5 mm TRS MIDI Out** jack.

- **Reference-counted note tracking:** if the same note on the same channel is triggered by multiple sources at once (for example, a held key and a chord pad), Chad37 sends a single `Note On` when the first source starts and holds off sending `Note Off` until the last source lets go.
- **No stuck notes:** if one output port's buffer is momentarily full when a `Note Off` is sent, Chad37 remembers the missed `Note Off` per port and retries automatically on the next frame.

### Default Control Map

*(Channels below are 1–16 as shown in your DAW. Each preset stores its own channels and CCs.)*

| Control | Default Channel | MIDI Message | Range / Behavior |
| :--- | :--- | :--- | :--- |
| **37 Keys** | Key Ch (`Ch 1`) | Note On / Off | `C3`–`C6` (MIDI `48`–`84`) at `OCT 0`; shifts to `C1`–`C8` (`24`–`108`) across `OCT -2` to `+2`. Velocity `1`–`127` shaped by `CURVE`. |
| **16 Pads (NOTE mode)** | Pad Ch (`Ch 2` on Preset 1, `Ch 10` on Preset 2) | Note On / Off | Velocity `1`–`127` (scaled into `PAD MIN`–`127`). Note numbers depend on the preset's **Pad Map** (see below). |
| **16 Pads (CHORD / SCALE)** | Chord Pad Ch (`KEYS` = Key Ch by default, or `Ch 1`–`16`) | Note On / Off | Plays the assigned custom chord (CHORD / Easy Chord) or diatonic scale chord (SCALE). |
| **Pad Aftertouch** | Pad Ch *(or Chord Pad Ch in chord/scale modes)* | Channel Pressure (`0xDn`) | `0`–`127`. Starts sending once a held pad's pressure moves past its initial strike reading; returns to `0` when all pads are released. |
| **Knobs Bank 1 (`K1`–`K8`)** | `Ch 6` | CC `48`–`55` | Absolute `0`–`127` (fixes stock firmware's relative `K7`–`K8`). |
| **Knobs Bank 2 (`K9`–`K16`)** | `Ch 1` | CC `102`–`109` | Absolute `0`–`127` on undefined standard CCs (fixes stock firmware's relative `K9`–`K16`). |
| **Faders Bank 1 (`F1`–`F4`)** | `Ch 1` | CC `20`–`23` | Absolute `0`–`127` on undefined standard CCs (fixes stock firmware's `CC 64`–`67` sustain/pedal collision). |
| **Faders Bank 2 (`F5`–`F8`)** | `Ch 1` | CC `24`–`27` | Absolute `0`–`127`. |
| **Pitch Wheel** | Key Ch (`Ch 1`) | Pitch Bend (`0xEn`) | `0`–`16383` (`8192` exact center, self-calibrating deadband). Displayed as `-127` to `+127`. |
| **Mod Wheel** | Key Ch (`Ch 1`) | CC `1` | Absolute `0`–`127`. |
| **Pedal Jack** | Key Ch (`Ch 1`) | CC `64` (Sustain) or CC `11` (Expression) | In `AUTO`, `OPEN`, or `CLOSED` mode: sends CC `64` (`0` or `127`). In `EXPR` mode: sends continuous CC `11` (`0`–`127`). Not yet tested with a real pedal. |
| **SEQ PLAY** | Key Ch (`Ch 1`) | CC `56` | `127` when pressed, `0` when released. |
| **SEQ REC** *(outside CHORD & ARP page)* | Key Ch (`Ch 1`) | CC `57` | `127` when pressed, `0` when released. |
| **`<` / `>`** *(outside CHORD modes)* | Key Ch (`Ch 1`) | Program Change (`0xCn`) | Steps `0`–`127` (displayed as `PROGRAM 1`–`128`), clamped at both ends. |

### Pad Note Maps (`PARA` → `K4 PADS`)

In **NOTE** mode (both CHORD and SCALE off), the 16 pads send individual MIDI notes according to the preset's **Pad Map**:

| Pad Map | Top Row (`Pads 1–8`) | Bottom Row (`Pads 9–16`) | Default Pad Channel | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **STOCK** | Pads 1–4: `16`–`19` (`E0`–`G0`)<br>Pads 5–8: `111`–`114` (`D#8`–`F#8`) | Pads 9–16: `115`–`122` (`G8`–`D9`) | `Ch 2` (Presets 1, 3–8) | Same note numbers as M-Vave's stock V016 firmware, but **Pads 1–4 send normal velocity and Note-Offs** so all 16 pads are playable. |
| **DRUMS** | Pads 1–8: `36`–`43` (`C2`–`G2`) | Pads 9–16: `44`–`51` (`G#2`–`D#3`) | `Ch 10` (Preset 2) | Standard General MIDI drum kit range (Kick on Pad 1, Snare on Pad 3, Closed/Open Hats on Pads 7/11, etc.). |
| **V016** | Same note numbers as `STOCK` | Same note numbers as `STOCK` | `Ch 2` | Exact stock V016 compatibility: Pads 1–4 send `Note On` with velocity `0` and no `Note Off`. |

### K5 & USB Audio Volume

The SMK-37 includes a built-in USB audio interface (sound card) so your computer, phone, or tablet can play audio out through the keyboard's rear 3.5 mm headphone/line jack.
- By default (`K5 MODE: CC+VOL` in Hardware Setup), turning **K5** sends its MIDI CC (`CC 52` in Bank 1, `CC 106` in Bank 2) **and** adjusts the keyboard's USB audio output volume (`VOL 0`–`100` on the main screen).
- If you don't want K5 to change the audio volume, press **GLOBE** twice and turn **K2** to `CC ONLY`. USB audio volume will stay fixed at 100% while K5 acts purely as a MIDI knob.
- On startup, K5 waits 1 second before sending MIDI so its internal smoothing filter can settle without sending an unwanted CC jump.

---

## 5. Presets

Chad37 stores **8 presets** (`PRESET 1`–`PRESET 8`).
- **Default setup:** **Preset 1** and **Presets 3–8** use the `STOCK` pad map on Pad Channel `2` (Key Channel `1`). **Preset 2** comes pre-configured for **DRUMS** on Pad Channel `10`.
- **Quick switching:** hold **PATCH** and tap **Pad 1–8**. While **PATCH** is held, Pads 1–8 light up to show your presets:
  - **White:** the currently active preset.
  - **Red:** presets you have used (Presets 1 and 2, plus any preset you have selected before).
  - **Green:** unused presets (Pads 3–8 that have never been selected).
- **What belongs to a preset:** Key MIDI channel, Pad MIDI channel, Pad Map (`STOCK`, `DRUMS`, `V016`), and each knob's and fader's CC, channel, min/max range, and last knob values.
- **What is shared across all presets:** the **16 Chord Banks** (256 chord pads total), plus global musical and hardware settings (Scale Root/Type, Voicing, Pad Flavor, Strum, Arpeggiator settings and rhythm pattern, Octave, Knob/Fader bank selection, Chord Pad Channel, Pedal mode, Velocity curve, Pad minimum velocity, Knob speed, and UI Theme).

---

## 6. Chord Mode & Chord Banks

Press **CHORD** once to enter **CHORD** mode. In this mode, the 37 keys play chromatically while detecting the chord you hold, and the 16 pads play chords from the active **Chord Bank**.

### Manual Chord Recognition

Play any combination of keys (up to 6 notes). Chad37 analyzes the held pitch classes—checking the bass note first, and matching both full voicings and voicings with an omitted 5th—and displays the chord name in large type on the Chord View:

| Recognized Quality | Display Suffix | Interval Structure (Semitones from Root) | Omitted 5th Recognized? |
| :--- | :--- | :--- | :--- |
| **Major** | `Maj` | `0, 4, 7` | — |
| **Minor** | `Min` | `0, 3, 7` | — |
| **Dominant 7th** | `7` | `0, 4, 7, 10` | Yes (`0, 4, 10`) |
| **Major 7th** | `Maj7` | `0, 4, 7, 11` | Yes (`0, 4, 11`) |
| **Minor 7th** | `Min7` | `0, 3, 7, 10` | Yes (`0, 3, 10`) |
| **Major 9th** | `Maj9` | `0, 4, 7, 11, 14` | Yes (`0, 4, 11, 14`) |
| **Minor 9th** | `Min9` | `0, 3, 7, 10, 14` | Yes (`0, 3, 10, 14`) |
| **Added 9th** | `Add9` | `0, 4, 7, 14` | — |
| **Suspended 4th** | `Sus4` | `0, 5, 7` | — |
| **7th Suspended 4th** | `7sus4` | `0, 5, 7, 10` | Yes (`0, 5, 10`) |
| **Diminished 7th** | `Dim7` | `0, 3, 6, 9` | — |
| **Half-Diminished** | `m7b5` | `0, 3, 6, 10` | — |
| **Diminished Triad** | `Dim` | `0, 3, 6` | — |
| **Augmented Triad** | `Aug` | `0, 4, 8` | — |
| **Suspended 2nd** | `Sus2` | `0, 2, 7` | — |
| **Power Chord (Fifth)** | `5` | `0, 7` | — |
| **Single Note** | *(Note name, e.g. `C4`)* | `0` | — |
| **Custom Cluster** | *(Bass note + `+`, e.g. `C3+`)* | Any other 2–6 note voicing | — |

As you release keys after playing a chord, Chad37 **latches the fullest chord you held** in memory so you can let go of the keyboard before assigning it to a pad.

### Assigning & Erasing Chord Pads (`SEQ REC`)

In **CHORD** and **Easy Chord** modes, **SEQ REC** works as a 3-step cycle (`ASSIGN` → `ERASE` → `Off`):

1. **Assign a chord (`SEQ REC` once):**
   - Play a chord on the keys (or trigger a diatonic chord in SCALE mode first), then press **SEQ REC**.
   - All 16 pads blink **red**, and the screen previews the chord notes that will be stored.
   - Tap any pad (`1`–`16`). The chord is stored unvoiced on that pad (up to 6 tones), **SEQ REC** disarms automatically, and the pad lights **teal**.
2. **Erase pads (`SEQ REC` twice):**
   - Press **SEQ REC** a second time. Pads that have a chord blink **orange**; empty pads turn off.
   - Tap any blinking pad to clear its chord (`PAD n ERASED`). Erase mode stays active so you can clear several pads in a row.
3. **Exit (`SEQ REC` a third time):**
   - Press **SEQ REC** once more to return to normal playing.

> **Tip:** Chords are stored on pads in their raw, unvoiced form. When you play a pad later, Chad37 applies the current **Voicing** (`K3`) and **Auto-Strum** (`GLOBE` → `K1`/`K2`) live, so you can try different inversions or strums on your saved chord banks at any time.

### 16 Shared Chord Banks (`<` and `>`)

Chad37 provides **16 Chord Banks** (`BANK 1`–`BANK 16`), giving you `16 × 16 = 256` custom chord slots shared across all 8 presets:
- **Switch bank:** in CHORD or Easy Chord mode, press **`<`** or **`>`** to step through `BANK 1` to `BANK 16`.
- **Delete a bank:** press **`<` and `>` together**. The screen asks `DELETE BANK n?`. Press **`<` and `>` together a second time** within 5 seconds to erase all 16 pads in that bank, or press **`<`** or **`>`** alone to keep it.

### Separate Chord Pad Channel (Surge XT Channel Split)

By default, chord pads in CHORD, Easy Chord, and SCALE modes play on the preset's **Key Channel**. If you want your chord pads to drive a second synth patch or layer while your right hand plays lead lines on the keys:
1. Press **GLOBE** once to open the Global menu.
2. Turn **K6 (`CHORD PAD CH`)** from `KEYS` to any MIDI channel `1`–`16` (for example, `2`).
3. The chord view header now shows `CH 1/2` (Key Channel / Chord Pad Channel).
4. **In Surge XT:** set Scene Mode to **Channel Split** with Split Channel at `2`. The keyboard plays Scene A (`Ch 1`) while the chord pads—including their aftertouch and pad-only arpeggios—play Scene B (`Ch 2`).

---

## 7. Easy Chord

Press **CHORD** a second time (when already in CHORD mode) to enter **Easy Chord** mode (`EASY CHORD` in the header).
- On your first visit, an on-screen keyboard diagram shows how Easy Chord works; pressing any key or pad switches to the live Chord View.
- Easy Chord is **untransposed**: pressing `C` always plays a `C` chord regardless of the key chosen in SCALE mode.
- The 16 pads continue to play your custom **Chord Banks** (`BANK 1`–`16`), and you can use **SEQ REC** to assign any Easy Chord you play on the keys straight onto a pad.
- Press **SCALE** while in Easy Chord to open and close the Scale page so you can adjust **K3 (`VOICING`)** without leaving Easy Chord.

### How Root + Left-Hand Modifiers Work

In Easy Chord, the **highest key you hold is the Root note** of the chord. Any keys you hold **to the left of that root** (within 16 semitones below it) act as chord quality modifiers, based on the **two nearest white keys (`W1`, `W2`)** and **two nearest black keys (`B1`, `B2`)** below the root:

- **`W1`:** 1st white key to the left of the root *(e.g. `B3` when root is `C4`)*
- **`W2`:** 2nd white key to the left of the root *(e.g. `A3` when root is `C4`)*
- **`B1`:** 1st black key to the left of the root *(e.g. `A#3` when root is `C4`)*
- **`B2`:** 2nd black key to the left of the root *(e.g. `G#3` when root is `C4`)*

| Keys Held Below Root | Example (with `C4` as Root) | Resulting Chord | Intervals from Root |
| :--- | :--- | :--- | :--- |
| **Root only** *(no modifiers)* | `C4` | **Major** (`C Maj`) | `0, 4, 7` |
| **`B1` + Root** | `A#3` + `C4` | **Minor** (`C Min`) | `0, 3, 7` |
| **`W2` + Root** | `A3` + `C4` | **Dominant 7th** (`C 7`) | `0, 4, 7, 10` |
| **`W1` + Root** | `B3` + `C4` | **Major 7th** (`C Maj7`) | `0, 4, 7, 11` |
| **`B1` + `W1` + Root** | `A#3` + `B3` + `C4` | **Minor 7th** (`C Min7`) | `0, 3, 7, 10` |
| **`W2` + `W1` + Root** | `A3` + `B3` + `C4` | **Major 9th** (`C Maj9`) | `0, 4, 7, 11, 14` *(or `W2`+`B2`+`W1`)* |
| **`W2` + `B1` + `W1` + Root** | `A3` + `A#3` + `B3` + `C4` | **Minor 9th** (`C Min9`) | `0, 3, 7, 10, 14` |
| **`W2` + `B1` + Root** | `A3` + `A#3` + `C4` | **Added 9th** (`C Add9`) | `0, 4, 7, 14` |
| **`B2` + Root** | `G#3` + `C4` | **Suspended 4th** (`C Sus4`) | `0, 5, 7` *(or `B2`+`W1`)* |
| **`W2` + `B2` + Root** | `A3` + `G#3` + `C4` | **7th Suspended 4th** (`C 7sus4`) | `0, 5, 7, 10` |
| **`B2` + `B1` + Root** | `G#3` + `A#3` + `C4` | **Diminished 7th** (`C Dim7`) | `0, 3, 6, 9` |

*(Because `W1`, `W2`, `B1`, and `B2` are counted by key color rather than fixed semitones, the same physical finger shapes work on every root key across the keyboard.)*

---

## 8. Voicings & Auto-Strum

Both **Voicing** and **Auto-Strum** are global settings that shape **Chord Pads** (in CHORD, Easy Chord, and SCALE modes) as well as **Easy Chord** on the keys.

### Voicings (`SCALE` → `K3` or `GLOBE` → `K3`)

| Voicing | What it does | Example on `C Maj` (`C4–E4–G4`) |
| :--- | :--- | :--- |
| **`ROOT`** | Root position (notes in their natural ascending order). | `C4 – E4 – G4` |
| **`1ST INV`** | First inversion: shifts the lowest note up by 1 octave (`+12` semitones). | `E4 – G4 – C5` |
| **`2ND INV`** | Second inversion: shifts the two lowest notes up by 1 octave (`+12` semitones). | `G4 – C5 – E5` |
| **`SPREAD`** | Open voicing: shifts the second-lowest note (the 3rd) up by 1 octave (`+12` semitones). | `C4 – G4 – E5` |
| **`2X`** | Doubled octave: keeps the chord and adds every chord tone again 1 octave below (`-12` semitones, up to 12 notes total). | `C3 – E3 – G3 – C4 – E4 – G4` |

### Non-Blocking Auto-Strum (`GLOBE` → `K1` & `K2`)

Press **GLOBE** once to set up Auto-Strum:
- **`K1 STRUM` (`OFF`, `5`–`120 ms` in `5 ms` steps):** staggers the notes of a chord so they fire one after another instead of all at the same instant.
- **`K2 DIR` (`UP`, `DOWN`, `ALT`):** strums from lowest note to highest (`UP`), highest to lowest (`DOWN`), or alternates direction on every chord strike (`ALT`).
- **Smart legato morphing:** if you change from one chord to another while holding (for example in Easy Chord), notes shared between the old and new chords keep sounding smoothly without re-triggering, departed notes stop immediately, and only the newly added chord tones are strummed.

### Pad Latch (`NOTE REPEAT`)

When playing chord pads in **CHORD**, **Easy Chord**, or **SCALE** mode (with ARP off):
- Press **NOTE REPEAT** to turn **LATCH** on (`LAT` in the header).
- Tap any chord pad and let go: the chord keeps sounding on its own.
- **Tap a different pad** to switch the latched chord immediately to the new pad.
- **Tap the same latched pad again** (or press **NOTE REPEAT** to turn LATCH off) to stop the chord (`LATCH STOPPED`).

---

## 9. Scale Mode

Press **SCALE** to enter **SCALE** mode and open the Scale page:
- **`K1 KEY`:** selects the root key (`C`, `C#`, `D`, `D#`, `E`, `F`, `F#`, `G`, `G#`, `A`, `A#`, `B`).
- **`K2 SCALE`:** selects the scale type (9 scales; see table below).
- **`K3 VOICE`:** selects the chord voicing (`ROOT`, `1ST INV`, `2ND INV`, `SPREAD`, `2X`).
- **`K4 PADS`:** selects the **Pad Flavor** (`TRIADS`, `7THS`, `9THS`) for the 16 pads.

*(Press **PATCH** to view the main CC screen while keeping SCALE mode active, or press **SCALE** while on the Scale page to turn SCALE mode off.)*

### How the Keys Work in Scale Mode

- **White keys (`C`–`B`)** map to the 7 degrees of the chosen scale, starting with the scale's root note on every `C` key (`C3`, `C4`, `C5`, `C6`).
- **Black keys are muted** so you can play runs and melodies across the white keys without ever hitting an out-of-key note.
- On the 5-note **Pentatonic** scales (`PENT MAJ` and `PENT MIN`), the 6th and 7th white keys (`A` and `B`) continue smoothly into thefirst two notes of the next octave up.

| Scale (`K2`) | Semitone Offsets on White Keys (`C D E F G A B`) | Diatonic Pad Scale Used |
| :--- | :--- | :--- |
| **`MAJOR`** | `0, 2, 4, 5, 7, 9, 11` | Major (`I ii iii IV V vi vii°`) |
| **`NAT MIN`** | `0, 2, 3, 5, 7, 8, 10` | Natural Minor (`i ii° III iv v VI VII`) |
| **`HARM MIN`** | `0, 2, 3, 5, 7, 8, 11` | Harmonic Minor (`i ii° III+ iv V VI vii°`) |
| **`DORIAN`** | `0, 2, 3, 5, 7, 9, 10` | Dorian (`i ii III IV v vi° VII`) |
| **`PHRYGIAN`** | `0, 1, 3, 5, 7, 8, 10` | Phrygian (`i II III iv v° VI vii`) |
| **`LYDIAN`** | `0, 2, 4, 6, 7, 9, 11` | Lydian (`I II iii #iv° V vi vii`) |
| **`MIXOLYDN`** | `0, 2, 4, 5, 7, 9, 10` | Mixolydian (`I ii iii° IV v vi VII`) |
| **`PENT MAJ`** | `0, 2, 4, 7, 9, 12, 14` | Parent Major scale (`0, 2, 4, 5, 7, 9, 11`) |
| **`PENT MIN`** | `0, 3, 5, 7, 10, 12, 15` | Parent Natural Minor scale (`0, 2, 3, 5, 7, 8, 10`) |

### How the 16 Diatonic Scale Pads Are Laid Out

In SCALE mode, the 16 pads automatically fill with the 8 diatonic scale degrees (`I`, `ii`, `iii`, `IV`, `V`, `vi`, `vii`, and `I'` one octave up) stacked in thirds from the scale. The **tonic pads** (`I` and `I'` at both ends of each row) light **bright amber**, while degrees `II`–`VII` light **dim green**:

| Pad Row | Physical Pads (Left → Right) | Scale Degrees | `K4 = TRIADS` | `K4 = 7THS` *(Default)* | `K4 = 9THS` |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Top Row** | `Pad 1` … `Pad 8` | `I, II, III, IV, V, VI, VII, I'` | **Triads** (`+1` octave higher) | **7th chords** (4-note) | **9th chords** (5-note) |
| **Bottom Row** | `Pad 9` … `Pad 16` | `I, II, III, IV, V, VI, VII, I'` | **Triads** (3-note, base octave) | **Triads** (3-note, base octave) | **7th chords** (4-note, base octave) |

> **Copying a Scale Pad into a Custom Chord Bank:** every diatonic pad you tap in SCALE mode also latches its unvoiced chord in memory. You can tap a chord pad you like in SCALE mode, press **CHORD**, press **SEQ REC**, and tap any pad to save it into your custom Chord Bank!

---

## 10. Arpeggiator & 16-Step Rhythm Editor

Press **ARP** to turn on the arpeggiator and open the ARP page. The arpeggiator works with normal keys, **CHORD** pads, **Easy Chord**, and **SCALE** mode.

### Arpeggiator Controls (`ARP` Page)

| Control | Parameter | Range / Options | Default | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`K1`** | **BPM** | `40`–`240` BPM *(steps of 2)* | `120` | Internal clock tempo. Reads `EXT` when locked to incoming MIDI Clock. |
| **`K2`** | **RATE** | `1/4`, `1/8`, `1/16`, `1/8T` | `1/8` | Step note value (`24`, `12`, `6`, or `8` MIDI clocks per step). |
| **`K3`** | **MODE** | `UP`, `DOWN`, `UP/DN`, `RANDOM`, `CHORD` | `UP` | Note order (see modes below). |
| **`K4`** | **OCT** | `1`–`3` octaves | `1` | Expands the held note pool across 1, 2, or 3 octaves (`UP`, `DOWN`, `UP/DN`, `RANDOM`). |
| **`K6`** | **GATE%** | `10%`–`95%` *(steps of 5%)* | `75%` | How long each step's note stays held as a percentage of the step duration. |
| **`K7`** | **SWING** | `OFF` (`50%`), `51%`–`75%` | `OFF` | Delays every second step in each pair (`67%` is a true triplet shuffle). Works on both internal and external MIDI clock. |
| **`K8`** | **STEPS** | `1`–`16` steps | `16` | Length of the repeating rhythm pattern before it loops back to Step 1. |
| **`NOTE REPEAT`** | **HOLD** | `OFF`, `HOLD` | `OFF` | Keeps keybed notes arpeggiating after you lift your hands. |
| **`SEQ`** | **Tap Tempo** | 2+ taps (`40`–`240` BPM) | — | Averages up to the last 4 tap intervals and re-aligns the beat. |
| **`SEQ REC`** | **Step Edit** | `OFF`, `STEP EDIT` | `OFF` | Turns the 16 pads into the 16-step rhythm editor. |

### Arpeggiator Modes (`K3 MODE`)

- **`UP`:** plays the combined pool of held notes from lowest to highest across `1`–`3` octaves.
- **`DOWN`:** plays the combined pool from highest to lowest.
- **`UP/DN`:** bounces smoothly from lowest to highest and back down without repeating the top or bottom endpoint twice.
- **`RANDOM`:** picks a random note from the held pool on each step.
- **`CHORD` (Full-Chord Progression Mode):** instead of breaking chords into single notes, each step triggers an **entire held chord at once**:
  - Hold a single chord (or latch a pad) to pulse that chord rhythmically to the pattern's accents and rests.
  - Hold **multiple chord pads at once** (plus keys if you like) to step through them in pad order (`Pad 1` → `Pad 16` → `Keys`), creating an instant multi-chord sequence where the currently sounding pad lights up in time with the beat.

### Hold (`NOTE REPEAT` with ARP On)

When **ARP** is on, pressing **NOTE REPEAT** activates **HOLD**:
- Play a chord on the keys and let go: the arpeggiator keeps playing those notes indefinitely.
- **Adding notes:** while you still have at least one key held down, any new keys you press are **added** to the held arpeggio.
- **Replacing the chord:** once you have lifted all fingers off the keys, pressing a new key or chord **replaces** the held arpeggio with the new notes. (In Easy Chord mode, playing a new Easy Chord updates the held chord immediately.)
- Press **NOTE REPEAT** again to release the held notes.

### 16-Step Rhythm Editor (`SEQ REC` on the ARP Page)

On the **ARP** page, the bottom of the screen shows two rows of 8 step blocks matching the 16 physical pads (`Steps 1–8` on the top row, `Steps 9–16` on the bottom row).
1. Press **SEQ REC** while on the ARP page (`STEP EDIT TAP PADS`).
2. The 16 pads now represent **Steps 1–16** of the rhythm pattern (tapping pads edits steps without sounding pad notes):
   - Tap a pad to cycle its step through **Normal Note (Teal)** → **Accent (Bright Amber)** → **Rest (Dim Red)** → **Normal Note (Teal)**.
   - While the arpeggiator is running, a **White** playhead chases across the pads to show the current step.
   - Use **`K8 STEPS` (`1`–`16`)** to shorten or lengthen the loop (for example, `3`, `5`, `7`, or `12` steps for polymeters and odd time signatures). Pads past the pattern length turn off.
3. **How Accents and Rests work:**
   - **Accent:** boosts that step's MIDI velocity by **`+40`** (clamped at `127`), shown with a bright white cap on the screen's step bar.
   - **Rest:** leaves that step silent **without advancing the arpeggiator's note index**, so the next active step picks up the melody right where it left off.
4. Press **SEQ REC** again (or leave the ARP page) to exit Step Edit and play the pads normally.

### External MIDI Clock Sync & Tap Tempo

- **External MIDI Clock (`0xF8` / `0xFA`):** when a DAW, synth, or drum machine sends MIDI Timing Clock (`0xF8`, 24 PPQN) over USB, Bluetooth, or TRS MIDI In, the arpeggiator automatically locks to the external clock (`EXT CLK` on the ARP screen), including **Swing (`K7`)** and `MIDI Start` (`0xFA`, which resets the pattern to Step 1 on the downbeat). If external clock stops for more than `500 ms`, Chad37 seamlessly falls back to its internal `BPM` clock.
- **Tap Tempo (`SEQ` button):** tap **SEQ** in time (at least 2 taps). Chad37 averages up to the last 4 tap intervals (`40`–`240` BPM) and lands the running arpeggiator's next step right on your tap. Pausing for more than `1.6 seconds` starts a fresh tap count. *(Tap tempo is disabled while locked to external MIDI clock.)*

---

## 11. Menus & Settings Reference

All knob ranges stop cleanly at their minimum and maximum values without wrapping around.

### EDIT Page (`PARA` Button — Per-Preset Settings)

Press **PARA** once to open the Edit page; press **PARA** or **PATCH** to return to the main view.

| Knob | Label | Range / Options | Default | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`K1`** | **`PRST`** | `1`–`8` | `1` | Selects the active preset (`PRESET 1`–`PRESET 8`). |
| **`K2`** | **`KEYCH`** | `1`–`16` | `1` | MIDI channel for the 37 keys, pitch/mod wheels, sustain/expression pedal, `SEQ PLAY`/`REC`, and Program Change (`<`/`>`). |
| **`K3`** | **`PADCH`** | `1`–`16` | `2` (`10` on Preset 2) | MIDI channel for the 16 pads in normal **NOTE** mode (and their channel pressure). |
| **`K4`** | **`PADS`** | `STOCK`, `DRUMS`, `V016` | `STOCK` (`DRUMS` on Preset 2) | Selects the note mapping for the 16 pads in NOTE mode and resets the preset's pad notes to that map. |

### GLOBAL Page (`GLOBE` Button Once — Device-Wide Musical Settings)

Press **GLOBE** once to open the Global menu.

| Knob | Label | Range / Options | Default | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`K1`** | **`STRUM`** | `OFF`, `5`–`120 ms` *(steps of 5 ms)* | `OFF` | Delay between notes when strumming chord pads or Easy Chord. |
| **`K2`** | **`DIR`** | `UP`, `DOWN`, `ALT` | `UP` | Strum direction: ascending (`UP`), descending (`DOWN`), or alternating (`ALT`). |
| **`K3`** | **`VOICE`** | `ROOT`, `1ST INV`, `2ND INV`, `SPREAD`, `2X` | `ROOT` | Chord voicing (same setting as `K3` on the Scale page). |
| **`K6`** | **`CHORD PAD CH`** | `KEYS`, `1`–`16` | `KEYS` | MIDI channel for chord pads in CHORD, Easy Chord, and SCALE modes. `KEYS` follows the preset's Key Channel. |

### HARDWARE SETUP Page (`GLOBE` Button Twice — Device-Wide Hardware Settings)

Press **GLOBE** a second time from the Global menu; press **GLOBE** a third time (or **PATCH**) to return to the main view.

| Knob | Label | Range / Options | Default | Description |
| :--- | :--- | :--- | :--- | :--- |
| **`K1`** | **`PEDAL`** | `AUTO`, `OPEN`, `CLOSED`, `EXPR` | `AUTO` | **`AUTO`:** learns a sustain pedal's resting polarity at startup. **`OPEN` / `CLOSED`:** forces normally-open or normally-closed sustain polarity (`CC 64`). **`EXPR`:** continuous expression pedal (`CC 11`). |
| **`K2`** | **`K5`** | `CC+VOL`, `CC ONLY` | `CC+VOL` | **`CC+VOL`:** K5 controls both its MIDI CC and USB audio output volume. **`CC ONLY`:** K5 only sends MIDI CC (USB audio stays at 100%). |
| **`K3`** | **`CURVE`** | `FACTORY`, `SOFT`, `HARD`, `FIXED` | `FACTORY` | Key and pad velocity response curve. **`SOFT`** boosts lighter touches, **`HARD`** requires a firmer strike, and **`FIXED`** sends velocity `100` before pad floor scaling. |
| **`K4`** | **`THEME`** | `GREEN`, `AMBER`, `CYAN`, `RED`, `MONO` | `AMBER` | Changes the screen's UI color palette. |
| **`K6`** | **`PAD MIN`** | `OFF` (`1`), `2`–`100` | `OFF` | Minimum velocity floor for the 16 pads. Scales pad strikes smoothly into `PAD MIN`–`127` while preserving full `127` peaks. |
| **`K7`** | **`KNOB SPEED`** | `ACCEL`, `X1`, `X2`, `X4`, `X8` | `ACCEL` | How far a knob's value moves per click, for every knob sending its CC (`K1`–`K4`, `K6`–`K16`) in every preset. **`ACCEL`** follows how fast you turn: `1` per click when you turn slowly, for fine adjustments, rising smoothly to `12` per click when you spin it, so a quick spin sweeps `0`–`127` in about a dozen clicks. The first click after a pause or a change of direction always moves `1`. `X1` always moves `1` (a full sweep is 127 clicks), `X2` moves `2`, `X4` moves `4`, `X8` moves `8`. `K5` is a pot and isn't affected; nor are the knobs on these settings pages. Relative knobs send one message per step. |

The page shows its settings as two rows of cells, one per knob. **`K5`**'s cell shows the USB audio volume (`VOL`; `K5` is the volume knob, not a setting here), and **`K8`**'s is empty for now.

**Diagnostics readout along the bottom of the Hardware Setup page** (in place of the keys and pads):
- **`JACK`:** live raw 10-bit ADC reading (`0`–`1023`) from the rear 1/4" pedal jack.
- **`REST`:** the auto-learned resting value of the sustain pedal (`-1` until learned).
- **`ACTV`:** total number of currently sounding MIDI notes across all channels.
- **`PEND`:** `1` if any `Note Off` is waiting in the retry queue for a busy transport port, otherwise `0`.

---

## 12. Saving, Calibration & Firmware Mode

### Automatic & Manual Saving

- **Auto-Save:** whenever you change a preset, chord bank, or setting, the header shows `*` next to the preset name and the **BT** button LED lights up. Chad37 waits until **2 seconds after your last edit** AND **1.5 seconds of complete musical silence** (no keys, pads, buttons, or pedal held, and no notes or strums active) before writing to flash in the background. That way a flash write never interrupts your playing.
- **Manual Save (`BT` button):** press **BT** at any time to save immediately and show `SAVED` on screen.

### Calibration & Recovery Shortcuts

- **Calibration:** the keys, wheels and pads use M-Vave's factory calibration, which Chad37 reads and never changes. The pitch wheel also finds its own centre at power-on.
- **Button and knob map (`OCTAVE −` + `OCTAVE +` while switching the power on):** the screen asks you to press each button and turn each knob by its printed name, and saves which is which. The map is already right on a standard SMK-37, so this is only for recovery if buttons ever act as the wrong ones.
- **Updating:** open the [web installer](https://superqix.github.io/chad37/) in Chrome or Edge, or run the command-line installer (see [install.md](install.md)). No button combination is needed.
- **Bootloader (`OCTAVE −` + `OCTAVE +` held for 5 seconds):** the screen counts down, then the keyboard restarts into the chip's own bootloader, a USB disk named `WL82 UBOOT`, used only for a full flash backup ([flash-backup.md](flash-backup.md)). **If your computer offers to format that disk, cancel.** Switch the keyboard off and on to leave it; if it then waits in update mode, run the installer again to finish.
