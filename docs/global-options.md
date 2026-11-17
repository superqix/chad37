# Chad37 global options

As built on 2026-11-17, read from `app/app.c` and `app/chord_scale.c`.

Global settings apply to the whole keyboard: changing preset doesn't change them. All of them are saved on the
keyboard, about 2 seconds after the last change with nothing held, or right away with **SAVE**.

Steps are per knob detent. Ranges stop at their ends; nothing wraps around.

## 1. Global menu: strum & voice

Press **GLOBE** once.

| Knob | Setting | Options / range | Default | Notes |
| :--- | :--- | :--- | :--- | :--- |
| K1 | Strum | OFF, 5–120 ms in 5 ms steps | OFF | Delay between chord tones, for chord pads and Easy Chord. Doesn't block playing. |
| K2 | Strum dir | UP, DOWN, ALT | UP | ALT alternates up and down on each strum. |
| K3 | Voicing | ROOT, 1ST INV, 2ND INV, SPREAD, 2X | ROOT | Same setting as K3 on the Scale page. 2X adds the same chord an octave below. |
| K4 | — | | | Free: the arp gate moved to the ARP page (K6). |
| K6 | Chord pad ch | KEYS, 1–16 | KEYS | MIDI channel for chord pads in CHORD, EASY CHORD and SCALE (and their pressure, and the arp while only pads are held). KEYS follows the preset's key channel. Keys always stay on the key channel. With both set, the chord screen header reads `CH 1/2` (keys/pads). |

**Surge XT Channel Split:** set Surge's scene mode to Channel Split with the split channel at 2. With keys on channel 1
and Chord pad ch at 2, the keys play Scene A and the chord pads play Scene B.

## 2. Hardware setup

Press **GLOBE** a second time; a third press returns to the main view.

| Knob | Setting | Options / range | Default | Notes |
| :--- | :--- | :--- | :--- | :--- |
| K1 | Pedal | AUTO, OPEN, CLOSED, EXPR | AUTO | AUTO learns the pedal's resting position at start-up. OPEN / CLOSED force a sustain pedal's polarity. EXPR sends the jack as Expression (CC 11) on the key channel. |
| K2 | K5 mode | CC+VOL, CC ONLY | CC+VOL | CC+VOL: K5 sends its CC and also sets the USB audio volume. CC ONLY: K5 only sends its CC. |
| K3 | Curve | FACTORY, SOFT, HARD, FIXED | FACTORY | Key velocity curve. SOFT boosts light playing, HARD needs a harder strike, FIXED is always 100. |
| K4 | Theme | GREEN, AMBER, CYAN, RED, MONO | AMBER | Screen accent colour. |
| K6 | Pad min | OFF (1), 2–100 | OFF | Pad velocity floor: pad velocities are scaled into PAD MIN–127, keeping the dynamics. |

The page also shows four read-only values: **JACK** (raw pedal reading), **REST** (learned pedal rest), **ACTV**
(sounding notes) and **PEND** (note-offs waiting to be sent).

## 3. Other keyboard-wide settings

These are also global and saved, but live on the mode pages.

### Scale page

Press **SCALE**. In Easy Chord, SCALE opens it without leaving Easy Chord.

| Knob | Setting | Options / range | Default | Notes |
| :--- | :--- | :--- | :--- | :--- |
| K1 | Key | C, C#, D … B | C | SCALE mode only; Easy Chord ignores it. |
| K2 | Scale | MAJOR, NAT MIN, HARM MIN, DORIAN, PHRYGIAN, LYDIAN, MIXOLYDN, PENT MAJ, PENT MIN | MAJOR | SCALE mode's white keys and diatonic pads. |
| K3 | Voicing | Same setting as GLOBAL K3 | ROOT | |
| K4 | Pad flavor | TRIADS, 7THS, 9THS | 7THS | SCALE mode's top pad row. |

### ARP page

Press **ARP**.

| Knob | Setting | Options / range | Default | Notes |
| :--- | :--- | :--- | :--- | :--- |
| K1 | BPM | 40–240 in steps of 2 | 120 | Internal clock; follows external MIDI clock automatically when one arrives. |
| K2 | Rate | 1/4, 1/8, 1/16, 1/8T | 1/8 | |
| K3 | Mode | UP, DOWN, UP/DN, RANDOM, CHORD | UP | CHORD plays each held chord whole, one chord per step, as a progression. |
| K4 | Octaves | 1–3 | 1 | |
| K6 | Gate | 10–95 % in 5 % steps | 75 % | Note length, as a share of each step. |
| K7 | Swing | OFF (50 %), 51–75 % | OFF | The first step of each pair takes this share of the pair: 67 % is a triplet shuffle. Works on external clock too. |
| K8 | Steps | 1–16 | 16 | The rhythm pattern's length. |

**Rhythm.** On the ARP page, **SEQ REC** turns STEP EDIT on: the pads become the pattern's steps, 1–8 on the top row
and 9–16 below, and each tap moves a step on from note to accent to rest. Pads light teal for a note, amber for an
accent, dim red for a rest, off past the pattern's end, and white for the step playing. Nothing sounds from the pads
while editing; SEQ REC again (or leaving the page) and they play. An accented step adds 40 to the velocity (127 at
most); a rest skips a step without losing the notes' order. The default pattern is 16 notes: the arp as it was.

**Hold.** With ARP on, **LATCH** (NOTE REPEAT) holds the keys' chord: let go and it keeps arpeggiating. Keys pressed
while others are down join the chord; a new chord after letting go replaces it. LATCH again stops it. In CHORD and
SCALE, LATCH still holds a pad chord too.

### Buttons

| Control | Options / range | Default |
| :--- | :--- | :--- |
| OCTAVE − / + | −2 to +2 | 0 |
| KNOB BANK | Bank 1 (K1–K8) / Bank 2 (K9–K16) | Bank 1 |
| FADER BANK | Bank 1 (F1–F4) / Bank 2 (F5–F8) | Bank 1 |
| Active preset | 1–8 (EDIT page K1, or hold PATCH + pad 1–8) | 1 |
| < / > (CHORD, Easy Chord) | Chord bank 1–16, stops at the ends | 1 |
| < / > (other modes) | Program Change 1–128 on the key channel | — |
| SEQ REC (CHORD, Easy Chord) | ASSIGN (tap a pad: store the chord, then off) → ERASE (tap pads: clear each, stays on) → off | off |
| SEQ REC (ARP page) | STEP EDIT on / off: the pads edit the arp's rhythm | off |
| LATCH (NOTE REPEAT) | CHORD / SCALE: holds a pad chord. ARP on: also HOLD for the keys' chord | off |
| < + > together (CHORD, Easy Chord) | Asks "DELETE BANK n?"; < + > again within 5 s clears its 16 pads; one arrow keeps it | — |

While **PATCH** is held, pads 1–8 show the presets: the current one white, a used one red (presets 1 and 2, and any
you've selected), an unused one green.

### Chord banks

The 16 pad chords come from the active **chord bank**. There are 16 banks, shared by all presets, saved in their own
storage slot. The chord screen's header shows the bank, e.g. `BANK 2 P1 CH 1/2`. On the first start with banks,
presets 1–8's pad chords became banks 1–8, so nothing was lost.

## 4. Per-preset settings, for comparison

Press **EDIT**. These change with the preset.

| Knob | Setting | Options / range | Default | Notes |
| :--- | :--- | :--- | :--- | :--- |
| K1 | Preset | 1–8 | 1 | |
| K2 | Key ch | 1–16 | 1 | |
| K3 | Pad ch | 1–16 | 2 (STOCK map) / 10 (DRUMS map) | |
| K4 | Pad map | STOCK, DRUMS, V016 | STOCK | Changing it resets the pad notes to that map. |

Each preset also holds its knob and fader CCs, channels and ranges. (Its old 16 pad chords are still saved but no
longer played: the chord banks replaced them.) Defaults: K1–K8 on
CC 48–55 ch 6, K9–K16 on CC 102–109 ch 1, faders on CC 20–27.

## 5. Things that stand out

- **Voicing has two homes:** GLOBAL K3 and Scale page K3 change the same setting.
- **GLOBAL K4** is free since the arp gate moved to the ARP page.
- **K6** carries a fifth setting on two pages: Chord pad ch on GLOBAL, Pad min on HARDWARE SETUP.
- **Key, scale and pad flavor** only affect SCALE mode now, but the Scale page still shows them in Easy Chord.
