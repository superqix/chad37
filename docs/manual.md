# Using Chad37

A guide to the SMK-37 running Chad37: what every control does, what it sends, and how the chord, scale and arp
modes work. For every setting with its range and default, see [global-options.md](global-options.md). To install,
see [install.md](install.md).

## The screen

The **header** shows the preset's name (with `*` while there are unsaved changes), the modes that are on (CHD, SCL,
ARP, LAT), the knob and fader banks, and BLE when Bluetooth is connected. A turned knob or a pressed button shows its
name and value there for a moment.

The **main view** (press **PATCH** to come back to it) shows the last control and the MIDI it sent, the last chord
or note played, every knob and fader value in the current banks, the wheels, the pedal and the volume. Turn a knob
or move a fader and it fills the middle of the screen as a big dial or fader, with its CC and channel, for a second
and a half.

The **footer** lights a strip of the 37 keys and the 16 pads as you play, and shows the octave, the key and pad
channels and the page you're on.

In CHORD and Easy Chord, the main view becomes the **chord view**: the chord's name in large type, its notes, and a
keyboard showing what's sounding.

## Buttons

| Button | What it does |
| :--- | :--- |
| **CHORD** | CHORD mode, then Easy Chord, then off. |
| **SCALE** | SCALE mode on and off. In Easy Chord, opens its voicing page instead. |
| **ARP** | Arpeggiator on, and its page. On the ARP page, ARP turns it off. |
| **NOTE REPEAT** | LATCH: holds a pad chord in CHORD and SCALE, and the keys' chord when ARP is on. |
| **SEQ REC** | CHORD modes: assign a chord to a pad, then erase pads, then off. ARP page: step editing on and off. Otherwise sends CC 57. |
| **SEQ PLAY** | Sends CC 56: 127 pressed, 0 let go. |
| **SEQ** | Tap tempo: tap it in time and the arp follows. |
| **PATCH** | Back to the main view. Hold it and tap pad 1–8 to switch preset. |
| **PARA** | The preset page: preset, key channel, pad channel and pad map. |
| **GLOBE** | The global page (strum, voicing, chord pad channel); again for hardware setup; again to go back. |
| **BT** | Saves now. Chad37 also saves on its own a couple of seconds after a change. |
| **FX** | Panic: every note off. |
| **OCTAVE − / +** | Shifts the keys by an octave, two either way. |
| **KNOB BANK** | K1–K8 or K9–K16. |
| **FADER BANK** | F1–F4 or F5–F8. |
| **< / >** | Program Change down or up on the key channel. In CHORD modes, the chord bank instead. |

## What it sends

Every message goes out over USB, Bluetooth and the TRS MIDI jack at once. Channels below are the defaults; each
preset sets its own.

| Control | Message |
| :--- | :--- |
| Keys | Notes on the key channel (1), with velocity. GLOBE twice, K3 picks the velocity curve. |
| Pads | Notes on the pad channel (2), with velocity, and channel pressure as you press harder. |
| K1–K8 | CC 48–55 on channel 6, absolute 0–127. |
| K9–K16 | CC 102–109 on channel 1, absolute 0–127. |
| F1–F8 | CC 20–27 on channel 1. |
| Pitch wheel | Pitch bend on the key channel. |
| Mod wheel | CC 1 on the key channel. |
| Pedal | Sustain (CC 64), or expression (CC 11) if set to EXPR in hardware setup. Not yet tested with a real pedal. |
| SEQ PLAY, SEQ REC | CC 56, CC 57 on the key channel: 127 pressed, 0 let go. |
| < and > | Program Change on the key channel. |

**K5** is also the volume of the keyboard's USB audio: the SMK-37 is a sound card too, so a computer can play through
its headphone and line outputs. Set K5 to CC ONLY in hardware setup to keep it off the volume.

## Presets

There are 8 presets, each with its own channels, pad map and knob and fader settings. Hold **PATCH** and the pads
show them: the current one white, used ones red, unused ones green. Tap pad 1–8 to switch. Preset 1 plays the pads
on channel 2; preset 2 is set up for drums, the pads on channel 10 with General MIDI drum notes.

On the **PARA** page, K4 picks the pad map: STOCK (the stock firmware's pad notes), DRUMS (notes 36–51), or V016
(as STOCK, but pads 1–4 send velocity 0, as the stock firmware does).

## CHORD mode

Press **CHORD**. Play a chord on the keys and the screen names it. To keep it on a pad:

1. Press **SEQ REC**: the pads blink red.
2. Tap a pad. It now plays that chord, and it lights up.

Press **SEQ REC** twice for ERASE: pads with a chord blink orange, and each one you tap is cleared. Press it once
more to finish.

The 16 pads' chords make a **bank**, and there are 16 banks, shared by all presets. **<** and **>** pick the bank.
Press **<** and **>** together to delete a bank's chords: the screen asks first, and pressing both again confirms.

Chord pads play on the key channel, or on their own channel if you set one (GLOBE, K6). With Surge XT's Channel Split,
that puts the keys and the chord pads on different scenes.

## Easy Chord

Press **CHORD** a second time. Each key plays a full chord: hold a key for its major chord, and add a nearby key on
its left to change the chord's type. The screen shows a diagram the first time. Easy Chord plays the chord you press,
whatever key SCALE mode is set to. **SCALE** opens its voicing page (K3). The pads still play your chord banks.

## SCALE mode

Press **SCALE**. K1 sets the key and K2 the scale: major, natural and harmonic minor, Dorian, Phrygian, Lydian,
Mixolydian, and the major and minor pentatonics.

- **The keys:** the white keys play only the scale's notes, starting from the key on C. The black keys are muted.
- **The pads:** the scale's chords. The bottom row has the triads on degrees I to VII and the octave; the top row
  has 7ths or 9ths (K4). The tonic pads light brighter.
- **K3** sets the voicing: root position, first or second inversion, spread, or doubled an octave down.

## Arpeggiator

Press **ARP**, then hold keys or chord pads. On the ARP page:

| Knob | Setting |
| :--- | :--- |
| K1 | Tempo, 40–240 BPM, or tap it on **SEQ**. Follows MIDI clock when one arrives. |
| K2 | Rate: 1/4, 1/8, 1/16, 1/8 triplets. |
| K3 | Mode: Up, Down, Up/Down, Random, or Chord (each held chord whole, in turn). |
| K4 | Octaves, 1–3. |
| K6 | Gate: how long each note lasts. |
| K7 | Swing, from off (straight) to a heavy shuffle. |
| K8 | The rhythm's length, 1–16 steps. |

**Hold:** press **NOTE REPEAT** (LATCH) and the chord keeps going after you let go. Keys pressed while others are
still down join the chord; a new chord replaces it. Press LATCH again to stop.

**Tap tempo:** tap **SEQ** in time, two taps or more. The tempo follows the last few taps, and a running arp's next
step lands on your tap. Pause for a couple of seconds to start a new count.

**Rhythm:** press **SEQ REC** on the ARP page, and the pads become the steps: 1–8 on the top row, 9–16 below. Tap a
pad to make its step an accent (louder), again for a rest, and again for a plain note. Teal is a note, amber an
accent, dim red a rest, and white is the step playing. Press **SEQ REC** again, and the pads play as usual.

## Strum

GLOBE, K1 strums chords from pads and Easy Chord: their notes start a few milliseconds apart, up to 120 ms. K2 sets
the direction: up, down, or alternating.

## Saving

Changes save on their own about two seconds after you stop, while nothing is held. **BT** saves at once. The header
shows `*` until it's saved, and BT lights up.
