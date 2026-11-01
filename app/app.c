/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Michael Wilson */
/* The bring-up app (M1): every control on the screen and out over USB and Bluetooth, to check the hardware before the
 * controller layer is written. The CC numbers are placeholders until docs/stock-midi-map.md is filled in, and the pads
 * send a fixed velocity until the platform has plat_pad_vel. Part of the unity build (device/unity.c,
 * host/plat_host.c), after the platform: gfx.c and announce.c are in scope. */

#define MIDI_TO (TO_USB | TO_BLE)
#define KEY_CH 0u                        /* channel 1 */
#define PAD_CH 9u                        /* channel 10 */
#define PAD_VEL 100u
#define PAD_NOTE0 36
#define KEY_NOTE0 48                     /* key 0 is C3 */
#define CC_MOD 1u
#define CC_KNOB0 70u                     /* K1..K8: 70..77 */
#define CC_FADER0 20u                    /* faders 1..4: 20..23 */
#define KNOB_MASTER 4                    /* K5 is the platform's MASTER: a position, not detents */
#define NKNOBS 8
#define BOOT_QUIET_MS 1000u              /* MASTER's smoothing is still rising from 0: no CC for it yet */

/* the encoder role of K1..K8 (K5 has none); plat.h: K6..K8 are SELECT, ALGORITHM, PRESETS */
static const int8_t KNOB_ROLE[NKNOBS] = {EN_K1, EN_K2, EN_K3, EN_K4, -1, EN_SELECT, EN_ALGO, EN_PRESET};
/* SCALE, CHORD, NOTE REPEAT, PARA, GLOBE, PATCH, BT, SEQ PLAY and SEQ REC are the panel's names for the FM-1's */
static const char *const BTN_NAME[NB] = {"FX", "SCALE", "CHORD", "REPEAT", "PARA", "GLOBE", "PATCH", "BT", "ARP", "SEQ",
                                         "PLAY", "REC", "OCT-", "OCT+"};
static const char *const SB_NAME[NSB] = {"KBANK", "FBANK", "LEFT", "RIGHT"};

static struct {
    uint64_t keys_on;                    /* keybed keys sounding */
    uint8_t key_note[SMK_NKEYBED];       /* the note each sounding key started: an octave change leaves it alone */
    uint32_t pads_on;
    int8_t octave;                       /* -2 .. +2 */
    uint8_t knob[NKNOBS];                /* 0..127 */
    uint8_t master_seen;
    int8_t fader[4];                     /* -1 until the readings settle */
    int32_t pitch, mod;
    int pedal;
    uint32_t btn, sb;
    uint32_t last_out;                   /* the last packet sent, shown */
    uint32_t out_count;
    char msg[2][24];                     /* the control moved last */
    uint32_t msg_until;
    uint8_t redraw;
} ui;

static void midi_send(uint32_t st, uint32_t d1, uint32_t d2)
{
    uint32_t pkt = st >> 4 | st << 8 | d1 << 16 | d2 << 24;
    plat_midi_send(pkt, MIDI_TO);
    ui.last_out = pkt;
    ui.out_count++;
    ui.redraw = 1;
}

/* --------------------------------------------------------------- platform --- */
void app_init(void) {}

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    uint32_t i, pkt;
    int port;
    (void)master_q12;
    while (plat_midi_in(&pkt, &port))    /* nothing listens yet */
        ;
    for (i = 0; i < 2u * n; i++)
        lr[i] = 0;
}

static void all_notes_off(void)
{
    uint32_t k;
    for (k = 0; k < SMK_NKEYBED; k++)
        if (ui.keys_on >> k & 1u)
            midi_send(0x80u | KEY_CH, ui.key_note[k], 0u);
    for (k = 0; k < SMK_NPADS; k++)
        if (ui.pads_on >> k & 1u)
            midi_send(0x80u | PAD_CH, (uint32_t)PAD_NOTE0 + k, 0u);
    ui.keys_on = 0;
    ui.pads_on = 0;
}

void app_panic(void) { all_notes_off(); }

void app_say(const char *a, const char *b)
{
    uint32_t i;
    for (i = 0; i < 23u && a && a[i]; i++)
        ui.msg[0][i] = a[i];
    ui.msg[0][i] = 0;
    for (i = 0; i < 23u && b && b[i]; i++)
        ui.msg[1][i] = b[i];
    ui.msg[1][i] = 0;
    ui.msg_until = plat_ms() + 1300u;
    ui.redraw = 1;
    if (ann_on())
        ann_say(ui.msg[0], ui.msg[1], ANN_NOW);
}

/* ------------------------------------------------------------------ input --- */
static void int_text(int32_t v, char *b)     /* signed, no padding */
{
    char t[12];
    int n = 0;
    uint32_t u = (uint32_t)(v < 0 ? -v : v);
    do
        t[n++] = (char)('0' + u % 10u);
    while ((u /= 10u) && n < 11);
    if (v < 0)
        *b++ = '-';
    while (n)
        *b++ = t[--n];
    *b = 0;
}

static void say_value(const char *what, int32_t v)
{
    char t[12];
    int_text(v, t);
    app_say(what, t);
}

static void keys_input(void)
{
    uint64_t hits = plat_keybed_hits(), now = plat_keybed(), off = ui.keys_on & ~now;
    uint32_t k;
    for (k = 0; (hits | off) && k < SMK_NKEYBED; k++) {   /* (nothing new: no loop) */
        uint64_t b = (uint64_t)1u << k;
        if ((off & b) || ((hits & b) && (ui.keys_on & b))) {   /* released (or struck again before we saw it go) */
            midi_send(0x80u | KEY_CH, ui.key_note[k], 0u);
            ui.keys_on &= ~b;
        }
        if (hits & b) {
            int32_t note = KEY_NOTE0 + (int32_t)k + 12 * ui.octave;
            ui.key_note[k] = (uint8_t)note;
            midi_send(0x90u | KEY_CH, (uint32_t)note, (uint32_t)plat_keybed_vel((int)k));
            if (now & b)
                ui.keys_on |= b;
            else                         /* a tap already over */
                midi_send(0x80u | KEY_CH, (uint32_t)note, 0u);
        }
    }
}

static void pads_input(void)
{
    uint32_t hits = plat_pad_hits(), held = plat_pads_held(), p;
    for (p = 0; p < SMK_NPADS; p++) {
        uint32_t b = 1u << p, note = (uint32_t)PAD_NOTE0 + p;
        if ((ui.pads_on & b) && (!(held & b) || (hits & b))) {
            midi_send(0x80u | PAD_CH, note, 0u);
            ui.pads_on &= ~b;
            plat_pad_rgb((int)p, 0, 0, 0);
        }
        if (hits & b) {
            midi_send(0x90u | PAD_CH, note, PAD_VEL);
            say_value("PAD", (int32_t)p + 1);
            if (held & b) {
                ui.pads_on |= b;
                plat_pad_rgb((int)p, 242, 120, 72);
            } else {
                midi_send(0x80u | PAD_CH, note, 0u);
            }
        }
    }
}

static void knobs_input(void)
{
    static const char *const NAME[NKNOBS] = {"K1", "K2", "K3", "K4", "K5", "K6", "K7", "K8"};
    int i;
    for (i = 0; i < NKNOBS; i++) {
        int32_t v = ui.knob[i];
        if (i == KNOB_MASTER) {
            if (plat_ms() < BOOT_QUIET_MS)
                continue;
            v = clamp((int32_t)(plat_master() >> 5), 0, 127);
            if (!ui.master_seen) {       /* (the first reading is where it stands, not a move) */
                ui.master_seen = 1;
                ui.knob[i] = (uint8_t)v;
                ui.redraw = 1;
                continue;
            }
        } else {
            int32_t e = plat_enc(KNOB_ROLE[i]);
            if (!e)
                continue;
            v = clamp(v + e, 0, 127);
        }
        if (v == ui.knob[i])
            continue;
        ui.knob[i] = (uint8_t)v;
        midi_send(0xB0u | KEY_CH, CC_KNOB0 + (uint32_t)i, (uint32_t)v);
        say_value(NAME[i], v);
    }
}

static void faders_input(void)
{
    static const char *const NAME[4] = {"F1", "F2", "F3", "F4"};
    int i;
    for (i = 0; i < 4; i++) {
        int v = plat_fader(i);
        if (v < 0 || v == ui.fader[i])
            continue;
        if (ui.fader[i] >= 0) {          /* (the first reading is where it stands, not a move) */
            midi_send(0xB0u | KEY_CH, CC_FADER0 + (uint32_t)i, (uint32_t)v);
            say_value(NAME[i], v);
        }
        ui.fader[i] = (int8_t)v;
    }
}

static void wheels_input(void)
{
    int32_t pitch, mod;
    if (!plat_wheels(&pitch, &mod))
        return;
    if (pitch != ui.pitch) {
        uint32_t bend = (uint32_t)(8192 + (pitch * 8191) / 127);
        ui.pitch = pitch;
        midi_send(0xE0u | KEY_CH, bend & 0x7Fu, bend >> 7);
        say_value("PITCH", pitch);
    }
    if (mod != ui.mod) {
        ui.mod = mod;
        midi_send(0xB0u | KEY_CH, CC_MOD, (uint32_t)mod);
        say_value("MOD", mod);
    }
}

static void buttons_input(void)
{
    uint32_t btn = plat_buttons(), sb = plat_smk_buttons(), k;
    for (k = 0; k < NB; k++) {
        if (!(btn >> k & 1u) || (ui.btn >> k & 1u))
            continue;
        if (k == B_OCTDN || k == B_OCTUP) {
            ui.octave = (int8_t)clamp(ui.octave + (k == B_OCTUP ? 1 : -1), -2, 2);
            say_value("OCTAVE", ui.octave);
        } else {
            app_say("BUTTON", BTN_NAME[k]);
        }
    }
    for (k = 0; k < NSB; k++)
        if ((sb >> k & 1u) && !(ui.sb >> k & 1u))
            app_say("BUTTON", SB_NAME[k]);
    ui.btn = btn;
    ui.sb = sb;
}

static void input(void)
{
    int pedal = plat_pedal_raw();
    keys_input();
    pads_input();
    knobs_input();
    faders_input();
    wheels_input();
    buttons_input();
    if (pedal != ui.pedal) {             /* shown raw: polarity and modes come from the stock capture */
        ui.pedal = pedal;
        ui.redraw = 1;
    }
    plat_glow(1);
    plat_leds(ui.btn, 0);
}

void app_input_only(void) { input(); }

/* --------------------------------------------------------------------- UI --- */
static void value_cell(int32_t x, int32_t y, const char *label, int32_t v)
{
    char t[12];
    int_text(v, t);
    cv_text(x, y, &FONT_XS, label, C_GRAY);
    cv_text(x + 24, y, &FONT_XS, t, C_HI);
}

static void draw(void)
{
    static const char *const NAME[NKNOBS] = {"K1", "K2", "K3", "K4", "K5", "K6", "K7", "K8"};
    static const char HEX[] = "0123456789ABCDEF";
    char h[12];
    uint32_t b;
    int i;
    cv_begin(240, 32, C_BLACK);
    if (plat_ms() < ui.msg_until) {
        int32_t x = cv_text(10, 6, &FONT_B, ui.msg[0], C_WHITE);
        cv_text(x + 8, 6, &FONT_B, ui.msg[1], C_HI);
    } else {
        int32_t x = cv_text(10, 6, &FONT_B, APP_NAME, C_WHITE);
        if (FM1B_BLE)                    /* grey while it waits, lit while a computer is connected */
            cv_text(x + 8, 10, &FONT_XS, "BLE", plat_ble_connected() ? C_HI : C_DIM);
    }
    cv_blit(0, 0);
    cv_begin(240, 144, C_BLACK);
    for (i = 0; i < NKNOBS; i++)
        value_cell(4 + (i % 4) * 58, (i / 4) * 16, NAME[i], ui.knob[i]);
    for (i = 0; i < 4; i++) {
        static const char *const FN[4] = {"F1", "F2", "F3", "F4"};
        value_cell(4 + i * 58, 40, FN[i], ui.fader[i]);
    }
    value_cell(4, 64, "PB", ui.pitch);
    value_cell(62, 64, "MOD", ui.mod);
    value_cell(120, 64, "OCT", ui.octave);
    value_cell(4, 80, "PDL", ui.pedal);
    value_cell(62, 80, "VOL", (int32_t)(plat_master() * 100u / 4096u));
    for (b = 0; b < 3u; b++) {           /* the last message sent */
        uint32_t v = (ui.last_out >> (8u + 8u * b)) & 0xFFu;
        h[3 * b] = HEX[v >> 4];
        h[3 * b + 1] = HEX[v & 15u];
        h[3 * b + 2] = b < 2u ? ' ' : 0;
    }
    cv_text(4, 104, &FONT_XS, "OUT", C_GRAY);
    if (ui.out_count)
        cv_text(32, 104, &FONT_XS, h, C_WHITE);
    cv_blit(0, 36);
}

void app_ui_init(void)
{
    int i;
    plat_smk_pads(SMK_PADS_OWN);
    plat_pad_lights(1);
    for (i = 0; i < 4; i++)
        ui.fader[i] = -1;
    ui.pedal = -1;
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.redraw = 1;
}

void app_frame(void)
{
    uint32_t now = plat_ms();
    input();
    if (ann_on() && ann_where())         /* a screen reader asks for the screen */
        ann_say(APP_NAME, 0, ANN_NOW);
    ann_tick(now);
    if (ui.msg_until && now >= ui.msg_until) {
        ui.msg_until = 0;
        ui.redraw = 1;
    }
    if (ui.redraw) {
        ui.redraw = 0;
        draw();
    }
}

#ifdef PLAT_HOST
int app_expect(const char *what, int *got)
{
    if (!strcmp(what, "octave"))
        *got = ui.octave;
    else if (!strncmp(what, "knob", 4))      /* knobN: K(N+1)'s value */
        *got = ui.knob[atoi(what + 4) % NKNOBS];
    else
        return 0;
    return 1;
}
#endif
