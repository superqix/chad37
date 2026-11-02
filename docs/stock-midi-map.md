# Stock SMK-37 MIDI map

This is the spec for the controller firmware. The stock manual doesn't list CC numbers, so fill this in from a MIDI monitor while the keyboard is still on stock V016. Use factory settings, preset 1.

**Keyboard:** Elite  
**Firmware version on screen:** V016, factory settings, never updated  
**Monitor used:** a Python logger (mido) on Windows, all three USB MIDI input ports, 2026-11-02

## Controls

Move or press each control and record the message. For knobs, note whether values are absolute (0–127) or relative (around 64, or 1 and 127), and what happens at each end.

Channels are 1–16 as a DAW shows them.

| Control | Message (type, number, channel) | Value behaviour | Bank 2 message |
|---|---|---|---|
| Key, lowest and highest | Note on 48 (C3) to 84 (C6), ch 1; note off `80 nn 40` | Velocity range: 43–127 seen, curve steep (a soft middle C gave 98) | n/a |
| Pad 1 (top left) and pad 16 | Pads 1–4: note on 16–19, ch 2, velocity 0, nothing on release. Pads 5–16: notes 111–122, ch 2, note off `81 nn 40` | Velocity range: 35–122 seen on pads 5–16; pads 1–4 always 0 | Pads 17–32: no way found without a menu |
| Pad pressure | Channel pressure, ch 2 | 0–127, only on a hard press, back to 0 before the note off; none seen from pads 1–4 | |
| Knobs K1–K8 | K1–K6: CC 48–53, ch 6. K7: CC 38, ch 1. K8: CC 37, ch 1 | K1–K6 absolute 0–127. K7 and K8 relative: 69 (`0x45`) per step left, 1 per step right, many steps per turn | K9 (K1): CC 30, K14 (K6): CC 35, K15 (K7): CC 36, all ch 1, relative 69 / 1. K10–K13 and K16 not captured; CC 31–34 and 37 likely |
| Faders 1–4 | CC 64–67, ch 1 (fader 1 sends the sustain CC) | Absolute 0–127 | Faders 5–8: CC 68–71, ch 1 (5 and 8 captured) |
| Pitch wheel | Pitch bend, ch 1 | 0–16383, rests at 8256 (`40 40`), not 8192 | n/a |
| Mod wheel | CC 1, ch 1 | Absolute 0–127 | n/a |
| Pedal, sustain mode | Not tested (no pedal) | | n/a |
| Pedal, expression mode | Not tested (no pedal) | | n/a |
| Left / Right buttons | Nothing sent | | n/a |
| Seq Play / Seq Rec | CC 56 / CC 57, ch 1 | 127 on press, 0 on release | n/a |
| Octave − / + | Nothing sent; shifts the keys by 12 | Middle C sends 72 at +1, 48 at −1 | n/a |
| Other buttons | Knob Bank, Fader Bank, Arp, Note Repeat, Scale, Chord, Glide, BT, Patch, Param, FX, Sequence: nothing sent | | n/a |

**Pad notes in order, 1–16:** 16, 17, 18, 19 (velocity 0), then 111, 112, 113, 114 (top row), 115–122 (bottom row). Pads 1–4 light and change the screen when pressed, so stock likely uses them for a function rather than notes.

**Banks:** two each. KNOB BANK shows K1–K8 or K9–K16; FADER BANK shows faders 1–4 or 5–8. In bank 2 every knob is relative, K1–K6 included.

**Ports:** the keyboard has three USB MIDI ports. Every message goes to the first two as identical copies; the third sent nothing.

## Transports

Play a key, a pad, a knob and a fader over each one. Note anything that differs from USB.

| Transport | Port or device name | Works? | Differences |
|---|---|---|---|
| USB MIDI | SMK-37 Elite Midi (ports 0–2, as "SMK-37 Elite Midi", "MIDIIN2", "MIDIIN3" on Windows); USB ID `4353:B74B` | Yes | The reference for this sheet |
| Bluetooth MIDI | | Yes (tested by Michael before this capture) | Not compared message by message |
| TRS MIDI out | 3.5 mm TRS jack | Yes, played an M-Vave FM-1 through its TRS MIDI in | None noticed |
| USB audio, playback | Speakers (SMK-37 Elite Audio) | Yes, heard on the keyboard's headphones | A held tone was audible in the background throughout; cause unknown |
| USB audio, recording | Microphone (SMK-37 Elite Audio) | Not tested | |

## Stock features you use

Mark each Need, Nice or Never. This sets what gets rebuilt after the first milestones.

Michael, 2026-11-02: all Nice, after the MVP.

| Feature | Need / Nice / Never |
|---|---|
| Internal DX7 sound | Nice |
| MIDI Suite editing | Nice |
| Presets 1–8 | Nice |
| Stock chord mode | Nice |
| Arpeggiator | Nice |
| Scale mode | Nice |
| Note repeat | Nice |
| Sequencer | Nice |
| Pads 17–32 | Nice |
| Knob and fader second banks | Nice |
| Bluetooth audio | Nice |
