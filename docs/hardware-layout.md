# Hardware & Screen Layout Reference (SMK-37 & FM-1)

This document provides an ASCII layout and API sizing reference for agentic coders working on **Chad37** (`BOARD_SMK37=1`) and `fm1-base`.

---

## 1. M-VAVE SMK-37 Physical Panel Layout (ASCII)

The SMK-37 (Pro / Elite / MKE-P37) shares the JieLi AC791N chip with the FM-1 and compiles with `-DBOARD_SMK37=1` (`base/bsp/boards/smk37/fm1_input.h`).

```text
+======================================================================================================+
| REAR: [USB-C]  [Power Sw]  [Sustain Pedal 1/4"]  [MIDI Out 3.5mm TRS]  [Line Out / Headphones 3.5mm] |
+======================================================================================================+
|                                                                                                      |
|  WHEELS             16 RGB VELOCITY PADS (2x8)             SCREEN (IPS)      4 FADERS    8 ENCODERS  |
|  (Self-calibrating  (WS2812 RGB, SARADC Mux A & B)         240 x 240 px      (0..127)    (Endless,   |
|   ADC Mux C)        Top row: Pads 0..7 (Stock: Pad 1..8)   ST7789 RGB565     ADC Mux C    detented)  |
|                     Bot row: Pads 8..15 (Stock: Pad 9..16)                                           |
|                                                            +------------+                            |
|  +---+  +---+      +----+ +----+ +----+ +----+             | 0,0        |    |  |  |  |  (K1)(K2)(K3)(K4)
|  | P |  | M |      | P0 | | P1 | | P2 | | P3 | (Top L:1-4) |   HEADER   |    |  |  |  |   EN_K1..EN_K4
|  | I |  | O |      +----+ +----+ +----+ +----+             |  240 x 32  |    |  |  |  |              |
|  | T |  | D |      +----+ +----+ +----+ +----+             |------------|    |  |  |  |  (K5)(K6)(K7)(K8)
|  | C |  |   |      | P4 | | P5 | | P6 | | P7 | (Top R:5-8) |    BODY    |    |  |  |  |  K5=MASTER   |
|  | H |  | W |      +----+ +----+ +----+ +----+             |  240 x 144 |    |  |  |  |  K6=SELECT   |
|  |   |  | H |      +----+ +----+ +----+ +----+             |------------|    |  |  |  |  K7=ALGO     |
|  | 0 |  | L |      | P8 | | P9 | |P10 | |P11 | (Bot L:9-12)|   FOOTER   |    |  |  |  |  K8=PRESET   |
|  |ctr|  |   |      +----+ +----+ +----+ +----+             |  240 x 60  |    F1 F2 F3 F4             |
|  +---+  +---+      +----+ +----+ +----+ +----+             +------------+                            |
| -127..  0..127     |P12 | |P13 | |P14 | |P15 | (Bot R:13-16)          239,239                        |
| +127               +----+ +----+ +----+ +----+                                                       |
|                                                                                                      |
|  18 BACKLIT BUTTONS (Single-color LEDs; SEQ PLAY has dual Green/Red LED)                             |
|  +-----------+ +------------+ +-----+ +-----+ +---------+ +---------+                                |
|  | KNOB BANK | | FADER BANK | |  <  | |  >  | | OCTAVE- | | OCTAVE+ |  (Hold OCT- & OCT+ at boot:    |
|  | (SB_KNOB) | | (SB_FADER) | |(LEFT| |RIGHT| |(B_OCTDN)| |(B_OCTUP)|   Hardware Calibration;        |
|  +-----------+ +------------+ +-----+ +-----+ +---------+ +---------+   Hold 5s in app: UBOOT mode)  |
|  +-----+ +-------------+ +-------+ +-------+ +-------+ +----+ +-------+ +------+                     |
|  | ARP | | NOTE REPEAT | | SCALE | | CHORD | | GLOBE | | BT | | PATCH | | PARA |                     |
|  |B_ARP| |   (B_LFO)   | |(B_SEL)| |(B_ENV)| |(B_GLO)| |SAVE| |(HOME) | |(EDIT)|                     |
|  +-----+ +-------------+ +-------+ +-------+ +-------+ +----+ +-------+ +------+                     |
|                                                        +----+ +-----+ +----------+ +---------+       |
|                                                        | FX | | SEQ | | SEQ PLAY | | SEQ REC |       |
|                                                        |B_FX| |B_SEQ| | (B_PLAY) | | (B_REC) |       |
|                                                        +----+ +-----+ +----------+ +---------+       |
|                                                                                                      |
|  37-KEY VELOCITY KEYBED (Keys 0..36 = C3..C6, MIDI 48..84; 2-contact switch matrix per key, no LEDs) |
|  | |#| |#| | |#| |#| |#| | |#| |#| | |#| |#| |#| | |#| |#| | |#| |#| |#| | |                         |
|  |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|                         |
|   C3          F3 (Key 5 = FM-1 Key 0)                     G5 (Key 31)     C6                         |
|   Key 0       |<--------- plat_keys() 27-bit window --------->|           Key 36                     |
+======================================================================================================+
```

> **Pad numbering note:** In firmware (`base/include/plat.h`), hardware pad indices `0..7` are the **top row** (left to right) and `8..15` are the **bottom row** (left to right).

---

## 2. M-VAVE FM-1 Physical Panel Layout (ASCII & Web Emulator Coordinates)

Because `base/include/plat.h` was designed around the FM-1, its enum names (`B_*`, `EN_*`) reflect the FM-1's silkscreen labels. The browser emulator (`base/web/emu/index.html`) draws the FM-1 in a `1000 x 640` SVG coordinate box:

```text
+===================================================================================================+
| M-VAVE FM-1 (1000 x 640 Emulator Viewport)                                                        |
|                                                                                                   |
|    LEFT KNOBS (r=24)              SCREEN BEZEL (x=300,y=44,236x236)     TOP-RIGHT KNOBS (r=22)    |
|    (MASTER)       (SELECT)        +-------------------------------+     (KNOB1) (KNOB2) (KNOB3) (KNOB4)
|    (105,100)      (215,100)       |  LCD Canvas (240 x 240 px)    |      x=610   x=714   x=818   x=922
|     Pot 0..4096    EN_SELECT      |  left:31.8% top:9.69%         |      y=100   y=100   y=100   y=100
|                                   |  width:20%  height:31.25%     |      EN_K1   EN_K2   EN_K3   EN_K4
|    (PRESETS)     (ALGORITHM)      |                               |                               |
|    (105,205)      (215,205)       |                               |     12 MODE BUTTONS (54x48)   |
|    EN_PRESET       EN_ALGO        +-------------------------------+     Tray: x=568,y=160,400x140 |
|                                                                         Row 1 (y=174, x=582+i*63):|
|    OCTAVE TRAY (x=58,y=258,210x58)                                      [FX] [SEL] [ENV] [LFO] [EDIT] [GLO]
|    +----------+  +----------+                                           Row 2 (y=236, x=582+i*63):|
|    |   OCT-   |  |   OCT+   |                                           [HOME][SAVE][ARP] [SEQ] [PLAY] [REC]
|    | (72,268) |  |(170,268) |                                           (PLAY has dual Green/Red LED)
|    |  82x38   |  |  82x38   |                                                                     |
|    +----------+  +----------+                                                                     |
|                                                                                                   |
|  27 CAPACITIVE/CARBON-PILL KEYS WITH LEDS (Tray: x=36, y=340, w=928, h=270; F3..G5, Keys 0..26)   |
|  +---------------------------------------------------------------------------------------------+  |
|  | Upper Row (11 Black Keys, h=33%):  [1] [3] [5]   [8] [10]   [13] [15] [17]   [20] [22]   [25]|  |
|  | Lower Row (16 White Keys, h=52%): [0] [2] [4] [6] [7] [9] [11][12][14][16][18][19][21][23][24][26]
|  |                                    F3  G3  A3  B3  C4  D4  E4  F4  G4  A4  B4  C5  D5  E5  F5  G5|
|  +---------------------------------------------------------------------------------------------+  |
+===================================================================================================+
```

---

## 3. Control Mapping & API Reference (SMK-37 ↔ FM-1)

### A. Knobs / Encoders (`8` on SMK-37, `7 + 1 pot` on FM-1)

Defined in `base/include/plat.h` and mapped in `base/device/panel.c`:

| SMK-37 Printed | FM-1 Printed | `plat.h` Role / API | SMK Matrix ID | Behavior & Sizing | Stock V016 MIDI (`docs/stock-midi-map.md`) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **K1** | KNOB 1 | `plat_enc(EN_K1)` (`3`) | Enc `0` (`0x97/0xa7`) | Endless detent (`+` = CW, `-` = CCW) | Bank 1: CC 48 (Ch 6, abs 0–127) / Bank 2 (K9): CC 30 (Ch 1, rel) |
| **K2** | KNOB 2 | `plat_enc(EN_K2)` (`4`) | Enc `1` (`0x37/0x47`) | Endless detent (`+` = CW, `-` = CCW) | Bank 1: CC 49 (Ch 6, abs 0–127) / Bank 2 (K10): CC 31* |
| **K3** | KNOB 3 | `plat_enc(EN_K3)` (`5`) | Enc `2` (`0x17/0x27`) | Endless detent (`+` = CW, `-` = CCW) | Bank 1: CC 50 (Ch 6, abs 0–127) / Bank 2 (K11): CC 32* |
| **K4** | KNOB 4 | `plat_enc(EN_K4)` (`6`) | Enc `3` (`0xb7/0xc7`) | Endless detent (`+` = CW, `-` = CCW) | Bank 1: CC 51 (Ch 6, abs 0–127) / Bank 2 (K12): CC 33* |
| **K5** | MASTER (Pot) | `plat_master()` | Enc `4` (`0x57/0x67`) | Virtual pot on SMK-37 (`0..4096`, `~96` per detent, starts at `2048`). Scales USB audio unless decoupled. | Bank 1: CC 52 (Ch 6, abs 0–127) / Bank 2 (K13): CC 34* |
| **K6** | SELECT | `plat_enc(EN_SELECT)` (`0`) | Enc `5` (`0x77/0x87`) | Endless detent (`+` = CW, `-` = CCW) | Bank 1: CC 53 (Ch 6, abs 0–127) / Bank 2 (K14): CC 35 (Ch 1, rel) |
| **K7** | ALGORITHM | `plat_enc(EN_ALGO)` (`1`) | Enc `6` (`0xd7/0xe7`) | Endless detent (`+` = CW, `-` = CCW) | Bank 1: CC 38 (Ch 1, rel `69`/`1`) / Bank 2 (K15): CC 36 (Ch 1, rel) |
| **K8** | PRESETS | `plat_enc(EN_PRESET)` (`2`) | Enc `7` (`0xf7/0x07`) | Endless detent (`+` = CW, `-` = CCW) | Bank 1: CC 37 (Ch 1, rel `69`/`1`) / Bank 2 (K16): CC 37* |

---

### B. Buttons (`18` on SMK-37 = `14` shared `B_*` + `4` SMK-only `SB_*`)

Read via `plat_buttons()` (returns bitmask `1u << B_*`) and `plat_smk_buttons()` (returns bitmask `1u << SB_*`). Lit via `plat_leds(buttons, keys)` and `plat_smk_leds(lit)` (`base/include/plat.h`, `base/device/panel.c`):

| SMK-37 Printed | FM-1 Printed | Enum (`plat.h`) | Bit | SMK Btn ID | LED Behavior | Notes / Stock Behavior |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **FX** | FX | `B_FX` | `0` | `14` (`0x85`) | Single LED | Mode / function button |
| **SCALE** | SEL | `B_SEL` | `1` | `8` (`0x25`) | Single LED | Scale mode |
| **CHORD** | ENV | `B_ENV` | `2` | `9` (`0x45`) | Single LED | Cycles CHORD → EASY CHORD → off in Chad37 |
| **NOTE REPEAT** | LFO | `B_LFO` | `3` | `7` (`0x65`) | Single LED | Note repeat mode |
| **PARA** | EDIT | `B_EDIT` | `4` | `13` (`0x55`) | Single LED | Parameter edit |
| **GLOBE** | GLO | `B_GLO` | `5` | `10` (`0x15`) | Single LED | Global settings |
| **PATCH** | HOME | `B_HOME` | `6` | `12` (`0x95`) | Single LED | Preset / Home screen |
| **BT** | SAVE | `B_SAVE` | `7` | `11` (`0x86`) | Single LED | Bluetooth / Save |
| **ARP** | ARP | `B_ARP` | `8` | `6` (`0x75`) | Single LED | Arpeggiator |
| **SEQ** | SEQ | `B_SEQ` | `9` | `15` (`0x35`) | Single LED | Sequencer |
| **SEQ PLAY** | PLAY / STOP | `B_PLAY` | `10` | `16` (`0x16`) | Dual: Green (`16`) / Red (`18`) | Use `plat_play_red(1)` for Red, default Green (`LED_PLAY_GREEN`). Stock: CC 56 (Ch 1, 127/0) |
| **SEQ REC** | REC | `B_REC` | `11` | `17` (`0x46`) | Single Red LED (`0x1b`) | Stock: CC 57 (Ch 1, 127/0) |
| **OCTAVE-** | OCT- | `B_OCTDN` | `12` | `4` (`0x66`) | Single LED | Shifts keybed octave down (`-2..+2`) |
| **OCTAVE+** | OCT+ | `B_OCTUP` | `13` | `5` (`0x36`) | Single LED | Shifts keybed octave up (`-2..+2`) |
| **KNOB BANK** | *(none)* | `SB_KNOB_BANK` | `0` | `0` (`0x26`) | `plat_smk_leds(1<<0)` | Toggles K1–K8 vs K9–K16 |
| **FADER BANK** | *(none)* | `SB_FADER_BANK` | `1` | `1` (`0x76`) | `plat_smk_leds(1<<1)` | Toggles F1–F4 vs F5–F8 |
| **< (LEFT)** | *(none)* | `SB_LEFT` | `2` | `2` (`0x96`) | `plat_smk_leds(1<<2)` | Program Change −1 on the key channel (the synth's previous patch; clamped at 0) |
| **> (RIGHT)** | *(none)* | `SB_RIGHT` | `3` | `3` (`0x56`) | `plat_smk_leds(1<<3)` | Program Change +1 on the key channel (the synth's next patch; clamped at 127) |

> **SMK-37 Button LED Note:** Unlike the FM-1, the SMK-37 button LEDs are driven via 74HC595 shift registers in 2 multiplexed groups and **do not support PWM dim glow** (`dim_buttons` is ignored on SMK-37; LEDs are strictly on/off).

---

### C. Pads, Faders, Wheels, Pedal, and Keybed (SMK-37)

Defined in `base/include/plat.h` and `base/bsp/boards/smk37/fm1_input.h`:

| Control Group | Count / Range | C API (`plat.h`) | Details & Hardware Notes |
| :--- | :--- | :--- | :--- |
| **37-Key Keybed** | `37` keys (`0..36` = `C3..C6`, MIDI `48..84`) | `plat_keybed()` (`uint64_t` held mask)<br>`plat_keybed_hits()` (`uint64_t` press edges)<br>`plat_keybed_vel(k)` (`1..127` velocity) | 2-contact velocity switch per key, calibrated via 5-point curve in flash (`0xC1000` `trimdat`). Keys `5..31` (`F3..G5`) also map to the 27-bit `plat_keys()` mask. |
| **16 RGB Pads** | `16` pads (`0..7` top row, `8..15` bottom row) | `plat_smk_pads(SMK_PADS_OWN)`<br>`plat_pad_hits()` (`uint32_t` press edges)<br>`plat_pads_held()` (`uint32_t` held mask)<br>`plat_pad_lights(1)` + `plat_pad_rgb(p, r, g, b)` | Analog pressure-sensitive pads (`pad_raw`, `pad_peak`, `pad_press[0..127]`). `SMK_PADS_OWN` decouples pads from `plat_keys()`. 16 WS2812 RGB LEDs (`0..255` per channel). |
| **4 Faders** | `4` faders (`i = 0..3`), value `0..127` | `plat_fader(i)` | Returns `-1` for the first 200 frames (~320 ms) while the median-of-3 + IIR filter settles, then `0..127` with ±1 deadband. Stock CC: `64..67` (Bank 1), `68..71` (Bank 2). |
| **Pitch & Mod Wheels** | Pitch: `-127..+127` (`0` center)<br>Mod: `0..127` | `plat_wheels(&pitch, &modw)` | Returns `0` for first 300 frames (~480 ms) or when unchanged; `1` when new values arrive. Self-calibrating center & endpoints with deadband. |
| **Sustain Pedal** | `1` jack, raw `0..1023` | `plat_pedal_raw()` | Raw 10-bit ADC reading on Mux C addr 6 (`-1` if uninitialized). |

---

## 4. Screen Layout & Sizing (`240 x 240` ST7789 IPS LCD)

The display (`base/device/lcd.c`, `base/device/gfx.c`) is a **240 × 240 pixel** RGB565 screen (`(0,0)` top-left to `(239,239)` bottom-right). Because RAM is tight when Bluetooth or Wi-Fi is active, apps do **not** keep a full `240 x 240` framebuffer in RAM; instead they draw into a strip canvas `cv_px` (`APP_CANVAS_PX`, default `240 * 144` pixels = 69,120 bytes in `.pool`, or `240 * 48` for Wi-Fi builds) using `cv_begin(w, h, bg)` and DMA-blit strips to the screen with `cv_blit(x, y)`.

```text
+-------------------------------------------------------+ (0, 0)
| HEADER STRIP: cv_begin(240, 32, C_BLACK) -> cv_blit(0, 0)
|  x=10, y=6: App Name / Transient Toast (FONT_B, 16px) |
|  x=Right, y=6..10: Status badges (BLE, DEV, Note/Val) |
+-------------------------------------------------------+ (0, 32)
| 4px black gap (y = 32..35)                            |
+-------------------------------------------------------+ (0, 36)
| MAIN BODY STRIP: cv_begin(240, 144, C_BLACK)          |
|                  -> cv_blit(0, 36)                    |
|                                                       |
|  4-Column Grid Layout (used in app/app.c & template): |
|  +-----------+ +-----------+ +-----------+ +--------+ |
|  | Col 0     | | Col 1     | | Col 2     | | Col 3  | |
|  | x = 4     | | x = 62    | | x = 120   | | x = 178| |
|  | w = 56    | | w = 56    | | w = 56    | | w = 56 | |
|  +-----------+ +-----------+ +-----------+ +--------+ |
|  (4px left margin + 4 * 58px column pitch = 236px)    |
|                                                       |
+-------------------------------------------------------+ (0, 180)
| FOOTER STRIP: cv_begin(240, 60, C_BLACK)              |
|               -> cv_blit(0, 180)                      |
|  y=0..28: Mini keybed / pad indicators                |
|  y=36..48: Bottom status row (OCT, PRESET, MIDI, VOL) |
+-------------------------------------------------------+ (239, 239)
```

### Font Sizes (`felucca_font.h` — `barlow` fontset)

Generated by `base/tools/gen_font.py` into `felucca_font.h` (proportional fonts with tabular numerals `tnum`):

| Font Constant | Pixel Height (`f->h`) | Character Range | Typical UI Role |
| :--- | :--- | :--- | :--- |
| `&FONT_T` | **12 px** | ASCII `32..95` (Caps, digits, punctuation) | Tiny footers, compact badges |
| `&FONT_XS` | **15 px** | ASCII `32..126` (Full printable ASCII) | Grid cell labels, units, parameter names (`value_cell`) |
| `&FONT_S` | **16 px** | ASCII `32..126` (Medium weight) | Body text, lists, prompts |
| `&FONT_B` | **16 px** | ASCII `32..126` (Bold weight) | Top bar headers, toast messages (`app_say`), active row |
| `&FONT_M` | **24 px** | ASCII `32..95` (Caps & digits only) | Medium knob/chord readouts |
| `&FONT_L` | **32 px** | ASCII `32..95` (Caps & digits only) | Splash screen title, huge center readout (`APP_BIG_FONT`) |

*(Note: Passing lowercase characters to `FONT_T`, `FONT_M`, or `FONT_L` automatically folds them to uppercase in `glyph()`.)*

### Theme & Color Constants (`base/device/gfx.c`)

Colors are 16-bit `RGB(r, g, b)` (`RGB565`, byte-swapped automatically by `cv_*` drawing functions):
* `C_BLACK` (`0x0000`): Background
* `C_LINE` (`pal[0]`, dark grey): Rules, boxes, inactive cell backgrounds
* `C_DIM` (`pal[1]`, mid-dark grey): Empty steps, disabled badges, units
* `C_GRAY` (`pal[2]`, light grey): Static parameter labels
* `C_AMB` (`pal[3]`, theme secondary color): Secondary values/highlights
* `C_HI` (`pal[4]`, theme primary bright color): Active parameter values, lit cells, curves
* `C_WHITE` (`0xFFFF`): Primary title, focused/touched item, transient alert text
