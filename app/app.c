/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Michael Wilson */
/* Chad37 controller and chord/scale/arpeggiator layer (M3 + M4):
 *   - 37-key velocity keybed, 16 velocity + pressure RGB pads, 2x8 endless encoders (KNOB BANK),
 *     2x4 faders (FADER BANK), pitch & mod wheels, sustain/expression pedal jack, and panel buttons.
 *   - 4-state Mode Matrix on CHORD (B_ENV) and SCALE (B_SEL):
 *       1. CHORD off, SCALE off: chromatic keys + stock/drum pads.
 *       2. CHORD on,  SCALE off: chromatic keys with manual chord detection/latch + 16 custom chord pads.
 *       3. CHORD on,  SCALE on:  Easy Chord on keys (transposed by Scale Root, shaped by Voicing) + 16 custom chord pads.
 *       4. CHORD off, SCALE on:  white keys play notes in key (black keys muted) + 16 diatonic scale chord pads.
 *   - SEQ REC (B_REC) arms pad assignment in CHORD mode (blinking red pads) and disarms automatically on store.
 *   - Arpeggiator (B_ARP) with Up, Down, Up/Down, Random, and Full-Chord Progression modes, internal 1 kHz clock
 *     and automatic external MIDI Clock (0xF8) sync, plus non-blocking Auto-Strum (0..120 ms, Up/Down/Alt).
 *   - 8 presets + global device header persisted in OBJ_PRESETS, auto-saved after 2 s idle with nothing held. */
#include "easy_chord.c"
#include "note_refs.c"
#include "chord_scale.c"

#define MIDI_TO (TO_USB | TO_BLE | TO_TRS)
#define KEY_NOTE0 48                     /* key 0 is C3 */
#define CC_MOD 1u
#define CC_EXPR 11u
#define CC_SUSTAIN 64u
#define CC_PLAY 56u                      /* stock V016: SEQ PLAY sends CC 56 (127 down, 0 up) on key_ch */
#define CC_REC 57u                       /* stock V016: SEQ REC sends CC 57 (127 down, 0 up) on key_ch when CHORD off */
#define REL_LEFT 69u                     /* stock V016 relative encoder: 69 (0x45) per step CCW, 1 per step CW */
#define REL_RIGHT 1u
#define KNOB_MASTER 4                    /* K5 is the platform's MASTER virtual pot */
#define NKNOBS 8                         /* physical encoders K1..K8 */
#define NBANK_KNOBS 16                   /* 2 banks: K1..K8 (0..7) and K9..K16 (8..15) */
#define NFADERS 4                        /* physical faders F1..F4 */
#define NBANK_FADERS 8                   /* 2 banks: F1..F4 (0..3) and F5..F8 (4..7) */
#define NPRESETS 8
#define BOOT_QUIET_MS 1000u              /* MASTER's smoothing is still rising from 0: no CC for it yet */
#define SAVE_IDLE_MS 2000u               /* auto-save delay after last preset/setting edit */
#define SAVE_QUIET_MS 1500u              /* minimum quiet time after last note/control activity before flash write */
#ifdef PLAT_HOST
#define JACK_STEADY 8                    /* fewer frames needed in scripted host tests */
#else
#define JACK_STEADY 128                  /* ~0.2 s of steady rail reading before auto-learning pedal rest */
#endif

/* encoder role of K1..K8 (K5 is MASTER); plat.h: K6..K8 are SELECT, ALGORITHM, PRESETS */
static const int8_t KNOB_ROLE[NKNOBS] = {EN_K1, EN_K2, EN_K3, EN_K4, -1, EN_SELECT, EN_ALGO, EN_PRESET};
static const char *const BTN_NAME[NB] = {"FX", "SCALE", "CHORD", "REPEAT", "PARA", "GLOBE", "PATCH", "BT", "ARP", "SEQ",
                                         "PLAY", "REC", "OCT-", "OCT+"};

/* Stock V016 pad note numbers (Pads 1..16 = hardware indices 0..15):
 * Pads 1-4: 16..19, Pads 5-8: 111..114 (top row), Pads 9-16: 115..122 (bottom row). */
static const uint8_t STOCK_PAD_NOTES[SMK_NPADS] = {
    16, 17, 18, 19, 111, 112, 113, 114,
    115, 116, 117, 118, 119, 120, 121, 122
};
/* GM Drum pad notes (36 = C1 kick .. 51 = D#2 ride) */
static const uint8_t DRUM_PAD_NOTES[SMK_NPADS] = {
    36, 37, 38, 39, 40, 41, 42, 43,
    44, 45, 46, 47, 48, 49, 50, 51
};

/* Default Knob CC, Channel (0-indexed), and Relative flag across Bank 1 (0..7) and Bank 2 (8..15). Every knob sends
 * an absolute 0..127: K1-K8 CC 48-55 on ch 6, K9-K16 CC 102-109 (undefined) on ch 1. Stock V016 makes K7-K16 relative
 * (1 per step right, 69 per step left), which a synth takes as a two-position switch, on CCs that include Data Entry
 * LSB (38) and Bank Select LSB (32) */
static const uint8_t DEF_KNOB_CC[NBANK_KNOBS] = {
    48, 49, 50, 51, 52, 53, 54, 55,
    102, 103, 104, 105, 106, 107, 108, 109
};
static const uint8_t DEF_KNOB_CH[NBANK_KNOBS] = {
    5, 5, 5, 5, 5, 5, 5, 5,
    0, 0, 0, 0, 0, 0, 0, 0
};
static const uint8_t DEF_KNOB_REL[NBANK_KNOBS] = {0};
/* the knob defaults of a "CPD1" / "CPD2" store (stock V016's), moved once by store_load: K7-K16 relative on ch 1 */
static const uint8_t OLD_KNOB_CC[NBANK_KNOBS] = {
    48, 49, 50, 51, 52, 53, 38, 37,
    30, 31, 32, 33, 34, 35, 36, 37
};
#define OLD_KNOB_REL0 6u                 /* the first old relative knob, K7 */
/* CC 20..27, which the MIDI standard leaves undefined. Stock V016 sends 64..71, the first four the sustain, portamento,
 * sostenuto and soft pedals: a synth takes fader 1 as sustain and will not MIDI-learn it */
static const uint8_t DEF_FADER_CC[NBANK_FADERS] = {20, 21, 22, 23, 24, 25, 26, 27};
#define OLD_FADER_CC0 64u                /* the defaults of a "CPD1" store, moved once by store_load */

enum { PAD_MAP_STOCK, PAD_MAP_DRUMS, PAD_MAP_V016, NPAD_MAPS };
enum { PEDAL_AUTO, PEDAL_OPEN, PEDAL_CLOSED, PEDAL_EXPR, NPEDAL_MODES };
enum { K5_CC_VOL, K5_CC_ONLY, NK5_MODES };
enum { CURVE_FACTORY, CURVE_SOFT, CURVE_HARD, CURVE_FIXED, NCURVES };
enum { PG_MAIN, PG_EDIT, PG_GLOBAL, PG_SCALE, PG_ARP, PG_HW, NPAGES };
enum { CHORD_HELP_NONE, CHORD_HELP_ASSIGN, CHORD_HELP_EASY };

/* --------------------------------------------------------------- store --- */
#define STORE_MAGIC 0x33445043u          /* "CPD3": every knob absolute, K7-K8 on CC 54-55, K9-K16 on CC 102-109 */
#define STORE_MAGIC_2 0x32445043u        /* "CPD2": the same layout, K7-K16 relative on stock's CCs */
#define STORE_MAGIC_1 0x31445043u        /* "CPD1": as CPD2, faders on 64..71 */

typedef struct {
    char name[12];
    uint8_t key_ch;                      /* 0..15 (default 0 = Ch 1) */
    uint8_t pad_ch;                      /* 0..15 (default 1 = Ch 2 on Stock, 9 = Ch 10 on Drums) */
    uint8_t pad_map;                     /* PAD_MAP_* */
    uint8_t rsv0;
    uint8_t knob_cc[NBANK_KNOBS];
    uint8_t knob_ch[NBANK_KNOBS];
    uint8_t knob_rel[NBANK_KNOBS];
    uint8_t knob_min[NBANK_KNOBS];
    uint8_t knob_max[NBANK_KNOBS];
    uint8_t knob_val[NBANK_KNOBS];
    uint8_t fader_cc[NBANK_FADERS];
    uint8_t fader_ch[NBANK_FADERS];
    uint8_t fader_min[NBANK_FADERS];
    uint8_t fader_max[NBANK_FADERS];
    uint8_t pad_note[SMK_NPADS];
    cs_pad_chord_t chords[SMK_NPADS];    /* 16 user-assigned custom chords per preset (unvoiced) */
} preset_t;

typedef struct {
    uint32_t magic;
    uint8_t active_preset;               /* 0..7 */
    int8_t octave;                       /* -2..+2 */
    uint8_t knob_bank;                   /* 0..1 */
    uint8_t fader_bank;                  /* 0..1 */
    uint8_t pedal_mode;                  /* PEDAL_* */
    uint8_t k5_mode;                     /* K5_* */
    uint8_t vel_curve;                   /* CURVE_* */
    uint8_t theme;                       /* 0..4 (PALETTES) */
    /* Global device-wide musical settings (persist across preset changes) */
    uint8_t scale_root;                  /* 0..11 (C..B) */
    uint8_t scale_type;                  /* SCALE_* (0..8) */
    uint8_t voicing;                     /* VOICING_* (0..4) */
    uint8_t pad_flavor;                  /* FLAVOR_* (0..2, default FLAVOR_7THS) */
    uint8_t strum_ms;                    /* 0..120 ms per chord tone */
    uint8_t strum_dir;                   /* STRUM_* (0..2) */
    uint8_t arp_bpm;                     /* 40..240 BPM */
    uint8_t arp_div;                     /* ARP_DIV_* (0..3) */
    uint8_t arp_mode;                    /* ARP_* (0..4) */
    uint8_t arp_gate;                    /* 10..95 % */
    uint8_t arp_oct;                     /* 1..3 octaves */
    uint8_t pad_vel_min;                 /* 1..100: pad velocities scaled into pad_vel_min..127 (0, older stores: 1) */
    preset_t presets[NPRESETS];
} presets_store_t;

_Static_assert(sizeof(presets_store_t) <= PLAT_STORE_MAX, "OBJ_PRESETS exceeds 3840-byte store object limit");

static presets_store_t st;

/* ------------------------------------------------------------------- UI --- */
static struct {
    note_refs_t nr;                      /* reference-counted note tracking + per-port Note-Off retry */
    cs_strum_t strum;                    /* non-blocking auto-strum queue */
    cs_arp_t arp;                        /* arpeggiator state */
    uint8_t chord_on;                    /* CHORD mode (B_ENV) */
    uint8_t scale_on;                    /* SCALE mode (B_SEL) */
    uint8_t arp_on;                      /* ARP mode (B_ARP) */
    uint8_t latch_on;                    /* LATCH (NOTE REPEAT, B_LFO): chord pads sound until tapped again */
    uint32_t latched_pad;                /* the pad LATCH holds (bit p), kept in pads_on after it is let go */
    uint8_t program;                     /* the last Program Change the arrows sent, 0..127 (this session's) */
    uint8_t rec_arm;                     /* SEQ REC armed for pad chord assignment */
    uint8_t chord_help;                  /* entry hint, dismissed by the first key/pad strike */
    uint8_t chord_help_seen;             /* show the assignment introduction once per session */
    uint8_t chord_view_first, chord_view_last; /* keep the displayed octave range steady after release */
    cs_chord_t latched_chord;            /* last played/detected chord, unvoiced (SEQ REC stores it on a pad) */
    cs_chord_t key_ec_prev;              /* previous Easy Chord held on keys */
    uint64_t keys_on;                    /* keybed keys physically held */
    uint8_t key_note[SMK_NKEYBED];       /* MIDI note each held key started on (0xFF if muted black key) */
    uint8_t key_ch_on[SMK_NKEYBED];      /* MIDI channel each held key started on */
    uint8_t last_key_vel;                /* last key strike velocity (1..127) */
    uint32_t pads_on;                    /* pads currently held */
    uint8_t pad_note_on[SMK_NPADS];      /* MIDI note each held pad started on (normal mode) */
    uint8_t pad_ch_on[SMK_NPADS];        /* MIDI channel each held pad started on */
    uint8_t pad_has_note[SMK_NPADS];     /* 1 = single note in note_refs, 2 = chord in strum, 0 = none/arp */
    cs_chord_t pad_active_chord[SMK_NPADS]; /* chord currently held on pad p (for ARP / release) */
    uint8_t last_pad_vel;                /* last pad strike velocity (1..127) */
    uint8_t pad_press_base[SMK_NPADS];   /* initial strike pressure reading per pad */
    uint8_t pad_at_active;               /* 1 once held pad pressure moved past strike reading */
    uint8_t pad_at_sent;                 /* last channel pressure value sent (0..127) */
    uint32_t pad_rgb[SMK_NPADS];         /* cached RGB per pad to avoid redundant updates */
    uint8_t blink_phase;                 /* 200 ms blink phase for SEQ REC armed pads */
    int8_t fader_phys[NFADERS];          /* physical fader reading (-1 until settled) */
    uint8_t fader_val[NBANK_FADERS];     /* last value per fader across both banks */
    uint8_t master_seen;
    int32_t last_master;                 /* last 7-bit reading from plat_master() >> 5 */
    uint8_t ble_conn;
    int32_t pitch, mod;
    int pedal_raw, pedal_val;            /* raw ADC reading, and sent CC value (0..127) */
    int jack_rest, jack_init, jack_cand, jack_n;
    uint32_t btn, sb;
    uint8_t home_combo;                  /* 1 if a pad preset shortcut was used while holding PATCH */
    uint8_t page;                        /* PG_* */
    uint8_t dirty;                       /* 1 if st has unsaved edits */
    uint32_t dirty_at;                   /* plat_ms() of last persistent edit */
    uint32_t active_at;                  /* plat_ms() of last note/control activity */
    uint32_t last_out;                   /* last 4-byte USB-MIDI packet sent */
    uint32_t out_count;
    char last_ctrl[12];                  /* name of last control moved */
    int32_t last_val;                    /* value of last control moved */
    uint8_t last_ch;                     /* 1..16 channel of last message */
    char chord_name[16];                 /* last played note / chord name */
    char msg[2][24];                     /* transient toast banner */
    uint32_t msg_until;
    uint8_t redraw;
} ui;

static preset_t *cur_preset(void) { return &st.presets[st.active_preset % NPRESETS]; }

/* --------------------------------------------------------- text helpers --- */
static void copy_str(char *dst, uint32_t max, const char *src)
{
    uint32_t i = 0;
    if (src)
        for (; i + 1u < max && src[i]; i++)
            dst[i] = src[i];
    dst[i] = 0;
}

static void int_text(int32_t v, char *b)
{
    char t[12];
    int n = 0;
    uint32_t u = v < 0 ? 0u - (uint32_t)v : (uint32_t)v;
    do
        t[n++] = (char)('0' + u % 10u);
    while ((u /= 10u) && n < 11);
    if (v < 0)
        *b++ = '-';
    while (n)
        *b++ = t[--n];
    *b = 0;
}

static void mark_dirty(void)
{
    ui.dirty = 1;
    ui.dirty_at = plat_ms();
    ui.redraw = 1;
}

/* ----------------------------------------------------- preset defaults --- */
static void preset_init_default(preset_t *p, uint32_t idx)
{
    uint32_t i;
    memset(p, 0, sizeof *p);
    copy_str(p->name, sizeof p->name, "PRESET ");
    p->name[7] = (char)('1' + (idx & 7u));
    p->name[8] = 0;
    p->key_ch = 0;                       /* Channel 1 */
    if (idx == 1u) {                     /* Preset 2: GM Drums on Channel 10 */
        p->pad_ch = 9;
        p->pad_map = PAD_MAP_DRUMS;
    } else {                             /* Preset 1 & 3..8: Stock V016 map on Channel 2 */
        p->pad_ch = 1;
        p->pad_map = PAD_MAP_STOCK;
    }
    for (i = 0; i < NBANK_KNOBS; i++) {
        p->knob_cc[i] = DEF_KNOB_CC[i];
        p->knob_ch[i] = DEF_KNOB_CH[i];
        p->knob_rel[i] = DEF_KNOB_REL[i];
        p->knob_min[i] = 0;
        p->knob_max[i] = 127;
        p->knob_val[i] = 0;
    }
    for (i = 0; i < NBANK_FADERS; i++) {
        p->fader_cc[i] = DEF_FADER_CC[i];
        p->fader_ch[i] = 0;
        p->fader_min[i] = 0;
        p->fader_max[i] = 127;
    }
    for (i = 0; i < SMK_NPADS; i++)
        p->pad_note[i] = p->pad_map == PAD_MAP_DRUMS ? DRUM_PAD_NOTES[i] : STOCK_PAD_NOTES[i];
}

static void store_defaults(void)
{
    uint32_t i;
    memset(&st, 0, sizeof st);
    st.magic = STORE_MAGIC;
    st.active_preset = 0;
    st.octave = 0;
    st.knob_bank = 0;
    st.fader_bank = 0;
    st.pedal_mode = PEDAL_AUTO;
    st.k5_mode = K5_CC_VOL;
    st.vel_curve = CURVE_FACTORY;
    st.theme = 1;                        /* AMBER */
    st.scale_root = 0;                   /* C */
    st.scale_type = SCALE_MAJOR;
    st.voicing = VOICING_ROOT;
    st.pad_flavor = FLAVOR_7THS;
    st.strum_ms = 0;
    st.strum_dir = STRUM_UP;
    st.arp_bpm = 120;
    st.arp_div = ARP_DIV_1_8;
    st.arp_mode = ARP_UP;
    st.arp_gate = 75;
    st.arp_oct = 1;
    st.pad_vel_min = 1;
    for (i = 0; i < NPRESETS; i++)
        preset_init_default(&st.presets[i], i);
}

static int store_valid(const presets_store_t *s)
{
    uint32_t i, k;
    if (s->magic != STORE_MAGIC || s->active_preset >= NPRESETS || s->octave < -2 || s->octave > 2 ||
        s->knob_bank > 1 || s->fader_bank > 1 || s->pedal_mode >= NPEDAL_MODES || s->k5_mode >= NK5_MODES ||
        s->vel_curve >= NCURVES || s->theme >= NPALETTES || s->scale_root >= 12 || s->scale_type >= NSCALES ||
        s->voicing >= NVOICINGS || s->pad_flavor >= NFLAVORS || s->strum_ms > 120 || s->strum_dir >= NSTRUM_DIRS ||
        s->arp_bpm < 40 || s->arp_bpm > 240 || s->arp_div >= NARP_DIVS || s->arp_mode >= NARP_MODES ||
        s->arp_gate < 10 || s->arp_gate > 95 || s->arp_oct < 1 || s->arp_oct > 3 || s->pad_vel_min > 100)
        return 0;
    for (i = 0; i < NPRESETS; i++) {
        const preset_t *p = &s->presets[i];
        if (p->key_ch > 15 || p->pad_ch > 15 || p->pad_map >= NPAD_MAPS)
            return 0;
        for (k = 0; k < NBANK_KNOBS; k++)
            if (p->knob_cc[k] > 127 || p->knob_ch[k] > 15 || p->knob_rel[k] > 1 ||
                p->knob_min[k] > p->knob_max[k] || p->knob_max[k] > 127 || p->knob_val[k] > 127)
                return 0;
        for (k = 0; k < NBANK_FADERS; k++)
            if (p->fader_cc[k] > 127 || p->fader_ch[k] > 15 ||
                p->fader_min[k] > p->fader_max[k] || p->fader_max[k] > 127)
                return 0;
        for (k = 0; k < SMK_NPADS; k++)
            if (p->pad_note[k] > 127 || p->chords[k].count > CS_CHORD_NOTES)
                return 0;
    }
    return 1;
}

/* An older store (CPD1, CPD2) -> the current one, once: faders and knobs still on their old defaults move to the new
 * ones; anything else is kept. 1 if s was older and is now valid (save it), else 0 (s untouched unless older) */
static int store_migrate(presets_store_t *s)
{
    uint32_t old = s->magic, i, k;
    if (old != STORE_MAGIC_1 && old != STORE_MAGIC_2)
        return 0;
    s->magic = STORE_MAGIC;
    if (!store_valid(s))
        return 0;
    for (i = 0; i < NPRESETS; i++) {
        preset_t *p = &s->presets[i];
        if (old == STORE_MAGIC_1)
            for (k = 0; k < NBANK_FADERS; k++)
                if (p->fader_cc[k] == OLD_FADER_CC0 + k)
                    p->fader_cc[k] = DEF_FADER_CC[k];
        for (k = OLD_KNOB_REL0; k < NBANK_KNOBS; k++)
            if (p->knob_rel[k] && p->knob_cc[k] == OLD_KNOB_CC[k] && p->knob_ch[k] == 0) {
                p->knob_cc[k] = DEF_KNOB_CC[k];
                p->knob_ch[k] = DEF_KNOB_CH[k];
                p->knob_rel[k] = 0;
            }
    }
    return 1;
}

static void store_load(void)
{
    int ok = plat_store_load(OBJ_PRESETS, &st, sizeof st) == (int)sizeof st;
    if (ok && store_migrate(&st))
        mark_dirty();
    if (!ok || !store_valid(&st))
        store_defaults();
    palette_set(st.theme);
}

/* ------------------------------------------------------------ MIDI send --- */
static uint32_t midi_send_to(uint32_t st_byte, uint32_t d1, uint32_t d2, uint32_t to)
{
    uint32_t cin = st_byte >> 4;
    uint32_t pkt = cin | (st_byte & 0xFFu) << 8 | (d1 & 0x7Fu) << 16 | (d2 & 0x7Fu) << 24;
    uint32_t ok = plat_midi_send(pkt, to);
    ui.last_out = pkt;
    ui.last_ch = (uint8_t)((st_byte & 0x0Fu) + 1u);
    ui.out_count++;
    ui.active_at = plat_ms();
    ui.redraw = 1;
    return ok;
}

static void send(uint32_t st_byte, uint32_t d1, uint32_t d2)
{
    (void)midi_send_to(st_byte, d1, d2, MIDI_TO);
}

static uint32_t apply_curve(int raw_vel)
{
    int32_t v = clamp(raw_vel, 1, 127);
    if (st.vel_curve == CURVE_SOFT)
        v = 127 - ((127 - v) * (127 - v)) / 127;
    else if (st.vel_curve == CURVE_HARD)
        v = (v * v) / 127;
    else if (st.vel_curve == CURVE_FIXED)
        v = 100;
    return (uint32_t)clamp(v, 1, 127);
}

static uint32_t pad_floor(uint32_t vel)          /* 1..127 -> PAD MIN..127, the dynamics kept */
{
    uint32_t lo = st.pad_vel_min > 1u ? st.pad_vel_min : 1u;
    return lo + ((vel - 1u) * (127u - lo) + 63u) / 126u;
}

/* --------------------------------------------------------------- platform --- */
uint32_t app_gain(uint32_t master)
{
    if (st.k5_mode == K5_CC_ONLY)
        return 2048u;
    return (master * master) >> 12;
}

void app_init(void)
{
    note_refs_init(&ui.nr, midi_send_to);
    cs_strum_init(&ui.strum);
    cs_arp_init(&ui.arp);
    ui.last_key_vel = 100;
    ui.last_pad_vel = 100;
    store_load();
}

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    uint32_t i, pkt;
    int port;
    (void)master_q12;
    while (plat_midi_in(&pkt, &port)) {
        uint8_t st_b = (uint8_t)((pkt >> 8) & 0xFFu);
        if (st_b == 0xF8u || st_b == 0xFAu)
            cs_arp_midi_clock(&ui.arp, st_b, st.arp_div, plat_ms());
    }
    for (i = 0; i < 2u * n; i++)
        lr[i] = 0;
}

static void all_notes_off(void)
{
    if (ui.pad_at_sent > 0u) {
        send(0xD0u | cur_preset()->pad_ch, 0u, 0u);
        ui.pad_at_sent = 0;
    }
    ui.pad_at_active = 0;
    cs_arp_stop(&ui.arp, &ui.nr);
    cs_strum_release_all(&ui.strum, &ui.nr);
    note_refs_all_off(&ui.nr);
    memset(&ui.key_ec_prev, 0, sizeof ui.key_ec_prev);
    memset(ui.pad_has_note, 0, sizeof ui.pad_has_note);
    memset(ui.pad_active_chord, 0, sizeof ui.pad_active_chord);
    ui.keys_on = 0;
    ui.pads_on = 0;
    ui.latched_pad = 0;
    ui.redraw = 1;
}

void app_panic(void) { all_notes_off(); }

void app_say(const char *a, const char *b)
{
    copy_str(ui.msg[0], sizeof ui.msg[0], a);
    copy_str(ui.msg[1], sizeof ui.msg[1], b);
    ui.msg_until = plat_ms() + 1300u;
    ui.redraw = 1;
    if (ann_on())
        ann_say(ui.msg[0], ui.msg[1], ANN_NOW);
}

static void say_control(const char *what, int32_t v)
{
    char t[12];
    int_text(v, t);
    copy_str(ui.last_ctrl, sizeof ui.last_ctrl, what);
    ui.last_val = v;
    app_say(what, t);
}

static int say_chord_help(void)
{
    if (!ann_on() || !ui.chord_on || (ui.page != PG_MAIN && ui.page != PG_SCALE))
        return 0;
    if (ui.chord_help == CHORD_HELP_ASSIGN)
        ann_say("SEQ REC, then tap a pad to assign the active chord", 0, ANN_NOW);
    else if (ui.chord_help == CHORD_HELP_EASY)
        ann_say("Hold a root for a major chord. Add nearby keys on its left to change the chord. Play any key or pad to start.", 0, ANN_NOW);
    else
        return 0;
    return 1;
}

static void save_now(int quiet_toast)
{
    all_notes_off();
    if (plat_store_save(OBJ_PRESETS, &st, sizeof st) == 0) {
        ui.dirty = 0;
        if (!quiet_toast)
            app_say("SAVED", cur_preset()->name);
        ui.redraw = 1;
    } else if (!quiet_toast) {
        app_say("SAVE FAILED", cur_preset()->name);
    }
}

static void preset_select(uint32_t idx)
{
    idx %= NPRESETS;
    if (idx != st.active_preset) {
        all_notes_off();
        ui.rec_arm = 0;
        st.active_preset = (uint8_t)idx;
        mark_dirty();
    }
    copy_str(ui.last_ctrl, sizeof ui.last_ctrl, "PRESET");
    ui.last_val = (int32_t)idx + 1;
    app_say(cur_preset()->name, cur_preset()->pad_map == PAD_MAP_DRUMS ? "DRUMS" : "STOCK");
}

/* ------------------------------------------------------------------ input --- */
static void build_raw_key_mask(uint64_t mask, uint8_t held128[16])
{
    uint32_t k;
    memset(held128, 0, 16);
    for (k = 0; mask && k < SMK_NKEYBED; k++) {
        if (mask >> k & 1u) {
            uint32_t n = (uint32_t)clamp(KEY_NOTE0 + (int32_t)k + 12 * st.octave, 0, 127);
            held128[n >> 3] |= (uint8_t)(1u << (n & 7u));
        }
    }
}

static int chord_equal(const cs_chord_t *a, const cs_chord_t *b)
{
    uint8_t i;
    if (a->count != b->count)
        return 0;
    for (i = 0; i < a->count; i++)
        if (a->notes[i] != b->notes[i])
            return 0;
    return 1;
}

static void keys_input(void)
{
    preset_t *pr = cur_preset();
    uint64_t hits = plat_keybed_hits(), now = plat_keybed(), off = ui.keys_on & ~now;
    uint32_t k;
    uint8_t ch = pr->key_ch & 15u;

    if (hits && ui.chord_help) {
        ui.chord_help = CHORD_HELP_NONE;
        ui.redraw = 1;
    }

    /* Easy Chord mode (CHORD on + SCALE on) */
    if (ui.chord_on && ui.scale_on) {
        uint8_t raw_held[16];
        cs_chord_t ec;
        char cname[16];
        if (hits) {
            for (k = 0; k < SMK_NKEYBED; k++)
                if (hits >> k & 1u)
                    ui.last_key_vel = (uint8_t)apply_curve(plat_keybed_vel((int)k));
        }
        if ((hits | off) == 0 && now == ui.keys_on)
            return;
        ui.keys_on = now;
        build_raw_key_mask(now, raw_held);
        if (cs_resolve_easy_chord(raw_held, st.scale_root, VOICING_ROOT, &ec, cname)) {
            ui.latched_chord = ec;                                   /* unvoiced; played with the voicing */
            ec.count = cs_apply_voicing(ec.notes, ec.count, st.voicing);
            copy_str(ui.chord_name, sizeof ui.chord_name, cname);
            copy_str(ui.last_ctrl, sizeof ui.last_ctrl, cname);
            ui.last_val = ui.last_key_vel;
            ui.redraw = 1;
            if (!ui.arp_on && !chord_equal(&ec, &ui.key_ec_prev)) {
                cs_strum_trigger(&ui.strum, &ui.nr, CS_STRUM_SRC_KEYS, ch,
                                 ec.notes, ec.count, ui.last_key_vel,
                                 st.strum_ms, st.strum_dir, plat_ms());
                ui.key_ec_prev = ec;
            }
        } else {
            if (!ui.arp_on && ui.key_ec_prev.count > 0)
                cs_strum_release_src(&ui.strum, &ui.nr, CS_STRUM_SRC_KEYS);
            memset(&ui.key_ec_prev, 0, sizeof ui.key_ec_prev);
            ui.redraw = 1;
        }
        return;
    }

    for (k = 0; (hits | off) && k < SMK_NKEYBED; k++) {
        uint64_t b = (uint64_t)1u << k;
        if ((off & b) || ((hits & b) && (ui.keys_on & b))) {
            if (!ui.arp_on && ui.key_note[k] != 0xFFu)
                note_refs_off(&ui.nr, ui.key_ch_on[k], ui.key_note[k]);
            ui.keys_on &= ~b;
            ui.redraw = 1;
        }
        if (hits & b) {
            int mapped;
            uint8_t vel = (uint8_t)apply_curve(plat_keybed_vel((int)k));
            ui.last_key_vel = vel;
            if (!ui.chord_on && ui.scale_on)
                mapped = cs_scale_key_note(k, st.octave, st.scale_root, st.scale_type);
            else
                mapped = clamp(KEY_NOTE0 + (int32_t)k + 12 * st.octave, 0, 127);

            if (mapped < 0) {            /* black key muted in Scale-only mode */
                ui.key_note[k] = 0xFFu;
                if (now & b)
                    ui.keys_on |= b;
                continue;
            }
            ui.key_note[k] = (uint8_t)mapped;
            ui.key_ch_on[k] = ch;
            if (!ui.arp_on)
                note_refs_on(&ui.nr, ch, (uint8_t)mapped, vel);
            if (!ui.chord_on) {
                cs_note_name((uint32_t)mapped, ui.chord_name);
                copy_str(ui.last_ctrl, sizeof ui.last_ctrl, ui.chord_name);
                ui.last_val = (int32_t)vel;
            }
            if (now & b)
                ui.keys_on |= b;
            else if (!ui.arp_on)
                note_refs_off(&ui.nr, ch, (uint8_t)mapped);
            ui.redraw = 1;
        }
    }

    /* Manual Fingered Chord detection & latch in CHORD-only mode (CHORD on, SCALE off) */
    if (ui.chord_on && !ui.scale_on && (hits | off) && ui.keys_on != 0) {
        uint8_t raw_held[16];
        cs_chord_t det;
        char cname[16];
        uint8_t cnt;
        build_raw_key_mask(ui.keys_on, raw_held);
        cnt = cs_detect_manual_chord(raw_held, &det, cname);
        /* Update latch on new key strikes or when more notes are held, preserving chord on release */
        if (hits || cnt >= ui.latched_chord.count) {
            ui.latched_chord = det;
            copy_str(ui.chord_name, sizeof ui.chord_name, cname);
            copy_str(ui.last_ctrl, sizeof ui.last_ctrl, cname);
            ui.last_val = ui.last_key_vel;
            ui.redraw = 1;
        }
    }
}

static uint8_t pad_note_of(const preset_t *pr, uint32_t p)
{
    if (pr->pad_map == PAD_MAP_DRUMS)
        return DRUM_PAD_NOTES[p];
    if (pr->pad_map == PAD_MAP_V016)
        return STOCK_PAD_NOTES[p];
    return pr->pad_note[p];
}

/* LATCH applies while the pads play chords: CHORD on, or SCALE alone (its diatonic pads) */
static int latch_active(void) { return ui.latch_on && (ui.chord_on || ui.scale_on); }

static void pad_release(uint32_t p)
{
    uint32_t b = 1u << p;
    if ((ui.pads_on & ~b) == 0u && ui.pad_at_sent > 0u) {
        send(0xD0u | ui.pad_ch_on[p], 0u, 0u);
        ui.pad_at_sent = 0;
        ui.pad_at_active = 0;
    }
    if (!ui.arp_on) {
        if (ui.pad_has_note[p] == 1u)
            note_refs_off(&ui.nr, ui.pad_ch_on[p], ui.pad_note_on[p]);
        else if (ui.pad_has_note[p] == 2u)
            cs_strum_release_src(&ui.strum, &ui.nr, (uint8_t)p);
    }
    ui.pad_has_note[p] = 0;
    ui.pad_active_chord[p].count = 0;
    ui.pads_on &= ~b;
    ui.latched_pad &= ~b;
    ui.redraw = 1;
}

static void pads_input(void)
{
    preset_t *pr = cur_preset();
    uint32_t hits = plat_pad_hits(), held = plat_pads_held(), p;
    uint8_t max_press = 0, press_moved = 0;
    int latch = latch_active();

    if (hits && ui.chord_help) {
        ui.chord_help = CHORD_HELP_NONE;
        ui.redraw = 1;
    }

    /* Shortcut: holding PATCH (B_HOME) + tapping Pad 1..8 selects Preset 1..8 without sounding a note */
    if ((plat_buttons() & (1u << B_HOME)) && (hits & 0xFFu)) {
        for (p = 0; p < NPRESETS; p++)
            if (hits >> p & 1u) {
                ui.home_combo = 1;
                preset_select(p);
                return;
            }
    }

    /* SEQ REC pad assignment in CHORD mode: tapping any pad stores the latched chord and disarms SEQ REC */
    if (ui.chord_on && ui.rec_arm && hits) {
        for (p = 0; p < SMK_NPADS; p++) {
            if (hits >> p & 1u) {
                if (ui.latched_chord.count > 0) {
                    char cname[16], pnum[12];
                    cs_pad_chord_store(&pr->chords[p], &ui.latched_chord);
                    cs_chord_format_name(&ui.latched_chord, cname);
                    copy_str(ui.chord_name, sizeof ui.chord_name, cname);
                    copy_str(pnum, sizeof pnum, "PAD ");
                    int_text((int32_t)p + 1, pnum + 4);
                    ui.rec_arm = 0;
                    mark_dirty();
                    app_say(pnum, cname);
                } else {
                    ui.rec_arm = 0;
                    app_say("NO CHORD", "PLAY KEYS");
                }
                return;
            }
        }
    }

    /* LATCH: a tap on the held pad stops it (no new chord); a tap on another pad lets it go for that one */
    if (ui.latched_pad && hits) {
        uint32_t lp = ui.latched_pad;
        for (p = 0; p < SMK_NPADS; p++)
            if (lp >> p & 1u)
                pad_release(p);
        if (hits & lp) {
            hits &= ~lp;
            app_say("LATCH", "STOPPED");
        }
    }

    for (p = 0; p < SMK_NPADS; p++) {
        uint32_t b = 1u << p;
        if ((ui.pads_on & b) && !(ui.latched_pad & b) && (!(held & b) || (hits & b)))
            pad_release(p);
        if (hits & b) {
            uint8_t vel = (uint8_t)pad_floor(apply_curve(plat_pad_vel((int)p)));
            ui.last_pad_vel = vel;
            ui.pad_press_base[p] = (uint8_t)clamp(plat_pad_press((int)p), 0, 127);

            if (ui.chord_on || ui.scale_on) {
                cs_chord_t c_play;
                char cname[16];
                uint8_t ch = pr->key_ch & 15u;
                if (ui.chord_on) {       /* Custom user-assigned chord pads (CHORD only, or CHORD + SCALE) */
                    cs_pad_chord_load(&c_play, &pr->chords[p]);
                    if (c_play.count == 0) {
                        say_control("EMPTY PAD", (int32_t)p + 1);
                        continue;
                    }
                    cs_chord_format_name(&c_play, cname);
                } else {                 /* Diatonic scale chord pads (SCALE-only mode) */
                    cs_scale_pad_chord(p, st.octave, st.scale_root, st.scale_type,
                                       VOICING_ROOT, st.pad_flavor, &c_play, cname);
                    ui.latched_chord = c_play;                       /* unvoiced, as SEQ REC stores it */
                }
                c_play.count = cs_apply_voicing(c_play.notes, c_play.count, st.voicing);
                copy_str(ui.chord_name, sizeof ui.chord_name, cname);
                copy_str(ui.last_ctrl, sizeof ui.last_ctrl, cname);
                ui.last_val = (int32_t)vel;
                app_say(cname, "PAD");
                ui.pad_ch_on[p] = ch;
                ui.pad_active_chord[p] = c_play;
                if ((held & b) || latch) {
                    ui.pads_on |= b;
                    ui.pad_has_note[p] = 2u;
                    if (latch)
                        ui.latched_pad = b;
                    if (!ui.arp_on)
                        cs_strum_trigger(&ui.strum, &ui.nr, (uint8_t)p, ch,
                                         c_play.notes, c_play.count, vel,
                                         st.strum_ms, st.strum_dir, plat_ms());
                } else if (!ui.arp_on) {
                    cs_strum_trigger(&ui.strum, &ui.nr, (uint8_t)p, ch,
                                     c_play.notes, c_play.count, vel, 0, st.strum_dir, plat_ms());
                    cs_strum_release_src(&ui.strum, &ui.nr, (uint8_t)p);
                }
                ui.redraw = 1;
            } else {                     /* Normal controller pad notes (CHORD off, SCALE off) */
                uint8_t note = pad_note_of(pr, p);
                uint8_t ch = pr->pad_ch & 15u;
                uint8_t v016_fn = pr->pad_map == PAD_MAP_V016 && p < 4u;
                uint8_t pvel = v016_fn ? 0u : vel;
                ui.pad_note_on[p] = note;
                ui.pad_ch_on[p] = ch;
                ui.pad_has_note[p] = (uint8_t)!v016_fn;
                ui.pad_active_chord[p].count = 1;
                ui.pad_active_chord[p].notes[0] = note;
                if (v016_fn)
                    send(0x90u | ch, note, 0u);
                else if (!ui.arp_on)
                    note_refs_on(&ui.nr, ch, note, pvel);
                say_control("PAD", (int32_t)p + 1);
                if (held & b) {
                    ui.pads_on |= b;
                } else if (!v016_fn && !ui.arp_on) {
                    note_refs_off(&ui.nr, ch, note);
                }
                ui.redraw = 1;
            }
        }
        if ((ui.pads_on & b) && ui.pad_has_note[p]) {
            uint8_t pr_now = (uint8_t)clamp(plat_pad_press((int)p), 0, 127);
            if (pr_now != ui.pad_press_base[p])
                press_moved = 1;
            if (pr_now > max_press)
                max_press = pr_now;
        }
    }

    /* Channel pressure (0xD0 | ch): sent once pressure changes from the initial strike reading */
    if (ui.pads_on && (press_moved || ui.pad_at_active)) {
        uint8_t at_ch = (ui.chord_on || ui.scale_on) ? (pr->key_ch & 15u) : (pr->pad_ch & 15u);
        ui.pad_at_active = 1;
        if (max_press != ui.pad_at_sent) {
            ui.pad_at_sent = max_press;
            send(0xD0u | at_ch, max_press, 0u);
        }
    }
}

static void arp_service(uint32_t now_ms)
{
    preset_t *pr = cur_preset();
    cs_chord_t chords[CS_ARP_MAX_CHORDS];
    uint8_t pad_idx[CS_ARP_MAX_CHORDS];
    uint8_t n_chords = 0, p;
    uint8_t ch = (ui.chord_on || ui.scale_on || ui.keys_on) ? (pr->key_ch & 15u) : (pr->pad_ch & 15u);
    uint8_t vel = ui.pads_on ? ui.last_pad_vel : ui.last_key_vel;

    if (!ui.arp_on)
        return;

    /* Collect held pad chords in order P0..P15 */
    for (p = 0; p < SMK_NPADS; p++) {
        if ((ui.pads_on >> p & 1u) && ui.pad_active_chord[p].count > 0 && n_chords < CS_ARP_MAX_CHORDS) {
            chords[n_chords] = ui.pad_active_chord[p];
            pad_idx[n_chords] = p;
            n_chords++;
        }
    }

    /* Collect held keybed notes/chord */
    if (ui.keys_on && n_chords < CS_ARP_MAX_CHORDS) {
        if (ui.chord_on && ui.scale_on && ui.latched_chord.count > 0) {
            chords[n_chords] = ui.latched_chord;
            chords[n_chords].count = cs_apply_voicing(chords[n_chords].notes, chords[n_chords].count, st.voicing);
            pad_idx[n_chords] = 0xFF;
            n_chords++;
        } else {
            cs_chord_t kc;
            uint32_t k;
            memset(&kc, 0, sizeof kc);
            for (k = 0; k < SMK_NKEYBED && kc.count < CS_MAX_NOTES; k++)
                if ((ui.keys_on >> k & 1u) && ui.key_note[k] != 0xFFu)
                    kc.notes[kc.count++] = ui.key_note[k];
            if (kc.count > 0) {
                chords[n_chords] = kc;
                pad_idx[n_chords] = 0xFF;
                n_chords++;
            }
        }
    }

    cs_arp_tick(&ui.arp, &ui.nr, ch, chords, pad_idx, n_chords, vel,
                st.arp_bpm, st.arp_div, st.arp_mode, st.arp_oct, st.arp_gate, now_ms);
}

static void page_knob_turn(int idx, int32_t e)
{
    preset_t *pr = cur_preset();
    if (ui.page == PG_EDIT) {
        if (idx == 0) {
            preset_select((uint32_t)clamp((int32_t)st.active_preset + e, 0, NPRESETS - 1));
        } else if (idx == 1) {
            int32_t ch = clamp((int32_t)pr->key_ch + e, 0, 15);
            if (ch != pr->key_ch) {
                all_notes_off();
                pr->key_ch = (uint8_t)ch;
                mark_dirty();
            }
            say_control("KEY CH", ch + 1);
        } else if (idx == 2) {
            int32_t ch = clamp((int32_t)pr->pad_ch + e, 0, 15);
            if (ch != pr->pad_ch) {
                all_notes_off();
                pr->pad_ch = (uint8_t)ch;
                mark_dirty();
            }
            say_control("PAD CH", ch + 1);
        } else if (idx == 3) {
            static const char *const MAP_N[NPAD_MAPS] = {"STOCK", "DRUMS", "V016"};
            int32_t m = clamp((int32_t)pr->pad_map + e, 0, NPAD_MAPS - 1);
            if (m != pr->pad_map) {
                uint32_t i;
                all_notes_off();
                pr->pad_map = (uint8_t)m;
                for (i = 0; i < SMK_NPADS; i++)
                    pr->pad_note[i] = m == PAD_MAP_DRUMS ? DRUM_PAD_NOTES[i] : STOCK_PAD_NOTES[i];
                mark_dirty();
            }
            app_say("PAD MAP", MAP_N[m]);
        }
    } else if (ui.page == PG_SCALE) {
        if (idx == 0) {
            int32_t r = clamp((int32_t)st.scale_root + e, 0, 11);
            if (r != st.scale_root) {
                all_notes_off();
                st.scale_root = (uint8_t)r;
                mark_dirty();
            }
            app_say("KEY ROOT", CS_ROOT_NAMES[st.scale_root]);
        } else if (idx == 1) {
            int32_t s = clamp((int32_t)st.scale_type + e, 0, NSCALES - 1);
            if (s != st.scale_type) {
                all_notes_off();
                st.scale_type = (uint8_t)s;
                mark_dirty();
            }
            app_say("SCALE", CS_SCALE_NAMES[st.scale_type]);
        } else if (idx == 2) {
            int32_t v = clamp((int32_t)st.voicing + e, 0, NVOICINGS - 1);
            if (v != st.voicing) {
                all_notes_off();
                st.voicing = (uint8_t)v;
                mark_dirty();
            }
            app_say("VOICING", CS_VOICING_NAMES[st.voicing]);
        } else if (idx == 3) {
            int32_t f = clamp((int32_t)st.pad_flavor + e, 0, NFLAVORS - 1);
            if (f != st.pad_flavor) {
                all_notes_off();
                st.pad_flavor = (uint8_t)f;
                mark_dirty();
            }
            app_say("PAD FLAVOR", CS_FLAVOR_NAMES[st.pad_flavor]);
        }
    } else if (ui.page == PG_ARP) {
        if (idx == 0) {
            int32_t b = clamp((int32_t)st.arp_bpm + e * 2, 40, 240);
            if (b != st.arp_bpm) {
                st.arp_bpm = (uint8_t)b;
                mark_dirty();
            }
            say_control("ARP BPM", st.arp_bpm);
        } else if (idx == 1) {
            int32_t d = clamp((int32_t)st.arp_div + e, 0, NARP_DIVS - 1);
            if (d != st.arp_div) {
                st.arp_div = (uint8_t)d;
                mark_dirty();
            }
            app_say("ARP RATE", CS_ARP_DIV_NAMES[st.arp_div]);
        } else if (idx == 2) {
            int32_t m = clamp((int32_t)st.arp_mode + e, 0, NARP_MODES - 1);
            if (m != st.arp_mode) {
                st.arp_mode = (uint8_t)m;
                mark_dirty();
            }
            app_say("ARP MODE", CS_ARP_MODE_NAMES[st.arp_mode]);
        } else if (idx == 3) {
            int32_t o = clamp((int32_t)st.arp_oct + e, 1, 3);
            if (o != st.arp_oct) {
                st.arp_oct = (uint8_t)o;
                mark_dirty();
            }
            say_control("ARP OCT", st.arp_oct);
        }
    } else if (ui.page == PG_GLOBAL) {
        if (idx == 0) {
            int32_t ms = clamp((int32_t)st.strum_ms + e * 5, 0, 120);
            if (ms != st.strum_ms) {
                st.strum_ms = (uint8_t)ms;
                mark_dirty();
            }
            say_control("STRUM MS", st.strum_ms);
        } else if (idx == 1) {
            int32_t d = clamp((int32_t)st.strum_dir + e, 0, NSTRUM_DIRS - 1);
            if (d != st.strum_dir) {
                st.strum_dir = (uint8_t)d;
                mark_dirty();
            }
            app_say("STRUM DIR", CS_STRUM_DIR_NAMES[st.strum_dir]);
        } else if (idx == 2) {
            int32_t v = clamp((int32_t)st.voicing + e, 0, NVOICINGS - 1);
            if (v != st.voicing) {
                all_notes_off();
                st.voicing = (uint8_t)v;
                mark_dirty();
            }
            app_say("VOICING", CS_VOICING_NAMES[st.voicing]);
        } else if (idx == 3) {
            int32_t g = clamp((int32_t)st.arp_gate + e * 5, 10, 95);
            if (g != st.arp_gate) {
                st.arp_gate = (uint8_t)g;
                mark_dirty();
            }
            say_control("ARP GATE", st.arp_gate);
        }
    } else if (ui.page == PG_HW) {
        if (idx == 0) {
            static const char *const PDL_N[NPEDAL_MODES] = {"AUTO", "OPEN", "CLOSED", "EXPR"};
            int32_t m = clamp((int32_t)st.pedal_mode + e, 0, NPEDAL_MODES - 1);
            if (m != st.pedal_mode) {
                st.pedal_mode = (uint8_t)m;
                ui.jack_init = 0;
                ui.jack_n = 0;
                mark_dirty();
            }
            app_say("PEDAL", PDL_N[m]);
        } else if (idx == 1) {
            static const char *const K5_N[NK5_MODES] = {"CC+VOL", "CC ONLY"};
            int32_t m = clamp((int32_t)st.k5_mode + e, 0, NK5_MODES - 1);
            if (m != st.k5_mode) {
                st.k5_mode = (uint8_t)m;
                mark_dirty();
            }
            app_say("K5 MODE", K5_N[m]);
        } else if (idx == 2) {
            static const char *const CRV_N[NCURVES] = {"FACTORY", "SOFT", "HARD", "FIXED"};
            int32_t c = clamp((int32_t)st.vel_curve + e, 0, NCURVES - 1);
            if (c != st.vel_curve) {
                st.vel_curve = (uint8_t)c;
                mark_dirty();
            }
            app_say("CURVE", CRV_N[c]);
        } else if (idx == 3) {
            int32_t t = clamp((int32_t)st.theme + e, 0, (int32_t)NPALETTES - 1);
            if (t != st.theme) {
                st.theme = (uint8_t)t;
                palette_set(st.theme);
                mark_dirty();
            }
            app_say("THEME", PALETTES[st.theme].name);
        } else if (idx == 5) {
            int32_t m = clamp((int32_t)(st.pad_vel_min > 1u ? st.pad_vel_min : 1u) + e, 1, 100);
            if (m != st.pad_vel_min) {
                st.pad_vel_min = (uint8_t)m;
                mark_dirty();
            }
            say_control("PAD MIN", m);
        }
    }
}

static void knobs_input(void)
{
    static const char *const NAME[NBANK_KNOBS] = {
        "K1", "K2", "K3", "K4", "K5", "K6", "K7", "K8",
        "K9", "K10", "K11", "K12", "K13", "K14", "K15", "K16"
    };
    preset_t *pr = cur_preset();
    uint32_t base = (uint32_t)st.knob_bank * NKNOBS;
    int i;
    for (i = 0; i < NKNOBS; i++) {
        uint32_t idx = base + (uint32_t)i;
        int32_t e = 0, v = pr->knob_val[idx];
        if (i == KNOB_MASTER) {
            int32_t m;
            if (plat_ms() < BOOT_QUIET_MS)
                continue;
            m = clamp((int32_t)(plat_master() >> 5), 0, 127);
            if (!ui.master_seen) {
                ui.master_seen = 1;
                ui.last_master = m;
                pr->knob_val[KNOB_MASTER] = (uint8_t)clamp(m, pr->knob_min[KNOB_MASTER], pr->knob_max[KNOB_MASTER]);
                ui.redraw = 1;
                continue;
            }
            e = m - ui.last_master;
            if (!e)
                continue;
            ui.last_master = m;
            if (!pr->knob_rel[idx]) {
                int32_t nv = clamp(m, pr->knob_min[idx], pr->knob_max[idx]);
                if (nv == pr->knob_val[idx])
                    continue;
                pr->knob_val[idx] = (uint8_t)nv;
                send(0xB0u | pr->knob_ch[idx], pr->knob_cc[idx], (uint32_t)nv);
                say_control(NAME[idx], nv);
                continue;
            }
        } else {
            e = plat_enc(KNOB_ROLE[i]);
            if (!e)
                continue;
            if (ui.page != PG_MAIN && (i < 4 || (ui.page == PG_HW && i == 5))) {   /* HW: K6 is PAD MIN */
                page_knob_turn(i, e);
                continue;
            }
        }
        if (pr->knob_rel[idx]) {
            uint32_t dir_val = e > 0 ? REL_RIGHT : REL_LEFT;
            int32_t steps = e > 0 ? e : -e;
            if (steps > 8)
                steps = 8;
            while (steps--)
                send(0xB0u | pr->knob_ch[idx], pr->knob_cc[idx], dir_val);
            pr->knob_val[idx] = (uint8_t)clamp(v + e, pr->knob_min[idx], pr->knob_max[idx]);
            say_control(NAME[idx], e > 0 ? 1 : -1);
        } else {
            int32_t nv = clamp(v + e, pr->knob_min[idx], pr->knob_max[idx]);
            if (nv == v)
                continue;
            pr->knob_val[idx] = (uint8_t)nv;
            send(0xB0u | pr->knob_ch[idx], pr->knob_cc[idx], (uint32_t)nv);
            say_control(NAME[idx], nv);
        }
    }
}

static void faders_input(void)
{
    static const char *const NAME[NBANK_FADERS] = {"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8"};
    preset_t *pr = cur_preset();
    uint32_t base = (uint32_t)st.fader_bank * NFADERS;
    int i;
    for (i = 0; i < NFADERS; i++) {
        int v = plat_fader(i);
        uint32_t idx = base + (uint32_t)i;
        if (v < 0 || v == ui.fader_phys[i])
            continue;
        if (ui.fader_phys[i] >= 0) {
            uint32_t lo = pr->fader_min[idx], hi = pr->fader_max[idx];
            uint32_t scaled = lo + ((uint32_t)v * (hi - lo) + 63u) / 127u;
            ui.fader_val[idx] = (uint8_t)scaled;
            send(0xB0u | pr->fader_ch[idx], pr->fader_cc[idx], scaled);
            say_control(NAME[idx], (int32_t)scaled);
        } else {
            ui.fader_val[i] = (uint8_t)v;
            ui.fader_val[i + NFADERS] = (uint8_t)v;
        }
        ui.fader_phys[i] = (int8_t)v;
        ui.redraw = 1;
    }
}

static void wheels_input(void)
{
    preset_t *pr = cur_preset();
    int32_t pitch, mod;
    if (!plat_wheels(&pitch, &mod))
        return;
    pitch = clamp(pitch, -127, 127);
    mod = clamp(mod, 0, 127);
    if (pitch != ui.pitch) {
        uint32_t bend = (uint32_t)(8192 + (pitch * (pitch < 0 ? 8192 : 8191)) / 127);
        ui.pitch = pitch;
        send(0xE0u | pr->key_ch, bend & 0x7Fu, bend >> 7);
        say_control("PITCH", pitch);
    }
    if (mod != ui.mod) {
        ui.mod = mod;
        send(0xB0u | pr->key_ch, CC_MOD, (uint32_t)mod);
        say_control("MOD", mod);
    }
}

static void pedal_input(void)
{
    preset_t *pr = cur_preset();
    int raw = plat_pedal_raw(), d, val;
    if (raw != ui.pedal_raw) {
        ui.pedal_raw = raw;
        ui.redraw = 1;
    }
    if (raw < 0)
        return;
    if (st.pedal_mode == PEDAL_EXPR) {
        val = clamp(raw >> 3, 0, 127);
        if (val > ui.pedal_val + 1 || val + 1 < ui.pedal_val ||
            (val != ui.pedal_val && (val == 0 || val == 127))) {
            ui.pedal_val = val;
            send(0xB0u | pr->key_ch, CC_EXPR, (uint32_t)val);
            say_control("EXPR", val);
        }
        return;
    }
    if (st.pedal_mode == PEDAL_OPEN) {
        d = 1023 - raw;
    } else if (st.pedal_mode == PEDAL_CLOSED) {
        d = raw;
    } else if (!ui.jack_init) {
        d = raw - ui.jack_cand;
        if (d < -24 || d > 24) {
            ui.jack_cand = raw;
            ui.jack_n = 0;
        } else if (++ui.jack_n >= JACK_STEADY && (ui.jack_cand < 200 || ui.jack_cand > 823)) {
            ui.jack_rest = ui.jack_cand;
            ui.jack_init = 1;
        }
        return;
    } else {
        d = raw - ui.jack_rest;
        if (d < 0)
            d = -d;
        if (d < 40)
            ui.jack_rest += (raw - ui.jack_rest) / 32;
    }
    val = d >= 340 ? 127 : 0;
    if (val != ui.pedal_val) {
        ui.pedal_val = val;
        send(0xB0u | pr->key_ch, CC_SUSTAIN, (uint32_t)val);
        say_control("SUSTAIN", val);
    }
}

static void page_set(int pg)
{
    static const char *const PG_NAME[NPAGES] = {"MAIN", "EDIT", "GLOBAL", "SCALE", "ARP", "SETUP"};
    pg = (pg % NPAGES + NPAGES) % NPAGES;
    if (pg != (int)ui.page) {
        ui.page = (uint8_t)pg;
        ui.redraw = 1;
    }
    app_say("PAGE", PG_NAME[ui.page]);
}

static void buttons_input(void)
{
    preset_t *pr = cur_preset();
    uint32_t btn = plat_buttons(), sb = plat_smk_buttons();
    uint32_t down = btn & ~ui.btn, up = ui.btn & ~btn;
    uint32_t sdown = sb & ~ui.sb;
    uint32_t k;

    /* OCT- and OCT+ held together is the 5 s update-mode chord in main_fm1.c; only shift on single presses */
    if ((down & ((1u << B_OCTDN) | (1u << B_OCTUP))) &&
        (btn & ((1u << B_OCTDN) | (1u << B_OCTUP))) != ((1u << B_OCTDN) | (1u << B_OCTUP))) {
        int32_t step = (down & (1u << B_OCTUP)) ? 1 : -1;
        int32_t noct = clamp((int32_t)st.octave + step, -2, 2);
        if (noct != st.octave) {
            st.octave = (int8_t)noct;
            mark_dirty();
        }
        say_control("OCTAVE", st.octave);
    }

    /* CHORD (B_ENV): toggles CHORD mode */
    if (down & (1u << B_ENV)) {
        all_notes_off();
        ui.chord_on ^= 1u;
        if (!ui.chord_on)
            ui.rec_arm = 0;
        if (ui.chord_on) {
            ui.page = PG_MAIN;
            ui.chord_help = ui.scale_on ? CHORD_HELP_EASY :
                ui.chord_help_seen ? CHORD_HELP_NONE : CHORD_HELP_ASSIGN;
            ui.chord_help_seen = 1;
        } else {
            ui.chord_help = CHORD_HELP_NONE;
        }
        app_say("CHORD", ui.chord_on ? (ui.scale_on ? "EASY CHORD" : "ON") : "OFF");
        say_chord_help();
    }

    /* SCALE (B_SEL): turns Scale on and opens PG_SCALE; if already on PG_SCALE, turns Scale off */
    if (down & (1u << B_SEL)) {
        if (!ui.scale_on) {
            all_notes_off();
            ui.scale_on = 1;
            ui.page = PG_SCALE;
            if (ui.chord_on) {
                ui.chord_help = CHORD_HELP_EASY;
            }
            app_say("SCALE ON", CS_SCALE_NAMES[st.scale_type]);
            say_chord_help();
        } else if (ui.page != PG_SCALE) {
            ui.page = PG_SCALE;
            app_say("SCALE PAGE", CS_SCALE_NAMES[st.scale_type]);
        } else {
            all_notes_off();
            ui.scale_on = 0;
            ui.page = PG_MAIN;
            ui.chord_help = CHORD_HELP_NONE;
            app_say("SCALE", "OFF");
        }
        ui.redraw = 1;
    }

    /* ARP (B_ARP): turns ARP on and opens PG_ARP; if already on PG_ARP, turns ARP off */
    if (down & (1u << B_ARP)) {
        if (!ui.arp_on) {
            all_notes_off();
            ui.arp_on = 1;
            ui.page = PG_ARP;
            app_say("ARP ON", CS_ARP_MODE_NAMES[st.arp_mode]);
        } else if (ui.page != PG_ARP) {
            ui.page = PG_ARP;
            app_say("ARP PAGE", CS_ARP_MODE_NAMES[st.arp_mode]);
        } else {
            all_notes_off();
            ui.arp_on = 0;
            ui.page = PG_MAIN;
            app_say("ARP", "OFF");
        }
        ui.redraw = 1;
    }

    /* NOTE REPEAT (B_LFO): LATCH for the chord pads, while they play chords; off lets a held chord go */
    if ((down & (1u << B_LFO)) && (ui.chord_on || ui.scale_on)) {
        ui.latch_on ^= 1u;
        if (!ui.latch_on && ui.latched_pad) {
            for (k = 0; k < SMK_NPADS; k++)
                if (ui.latched_pad >> k & 1u)
                    pad_release(k);
        }
        app_say("LATCH", ui.latch_on ? "ON" : "OFF");
        ui.redraw = 1;
    }

    /* SEQ PLAY (CC 56): 127 on press, 0 on release on key_ch */
    if (down & (1u << B_PLAY)) {
        send(0xB0u | pr->key_ch, CC_PLAY, 127u);
        say_control("PLAY", 127);
    }
    if (up & (1u << B_PLAY)) {
        send(0xB0u | pr->key_ch, CC_PLAY, 0u);
        say_control("PLAY", 0);
    }

    /* SEQ REC (B_REC): arms pad assignment in CHORD mode; sends CC 57 (127/0) when CHORD mode is off */
    if (ui.chord_on) {
        if (down & (1u << B_REC)) {
            ui.chord_help = CHORD_HELP_NONE;
            ui.rec_arm ^= 1u;
            app_say("PAD ASSIGN", ui.rec_arm ? "TAP A PAD" : "CANCELLED");
        }
    } else {
        if (down & (1u << B_REC)) {
            send(0xB0u | pr->key_ch, CC_REC, 127u);
            say_control("REC", 127);
        }
        if (up & (1u << B_REC)) {
            send(0xB0u | pr->key_ch, CC_REC, 0u);
            say_control("REC", 0);
        }
    }

    /* Page & function buttons */
    if (down & (1u << B_HOME))
        ui.home_combo = 0;
    if ((up & (1u << B_HOME)) && !ui.home_combo)
        page_set(PG_MAIN);
    if (down & (1u << B_EDIT))
        page_set(ui.page == PG_EDIT ? PG_MAIN : PG_EDIT);
    if (down & (1u << B_GLO))
        page_set(ui.page == PG_GLOBAL ? PG_HW : ui.page == PG_HW ? PG_MAIN : PG_GLOBAL);
    if (down & (1u << B_SAVE))
        save_now(0);
    if (down & (1u << B_FX)) {
        all_notes_off();
        app_say("PANIC", "ALL OFF");
    }
    for (k = 0; k < NB; k++) {
        if (!(down >> k & 1u))
            continue;
        if (k == B_LFO && (ui.chord_on || ui.scale_on))
            continue;
        if (k == B_OCTDN || k == B_OCTUP || k == B_ENV || k == B_SEL || k == B_ARP ||
            k == B_PLAY || k == B_REC || k == B_HOME || k == B_EDIT || k == B_GLO || k == B_SAVE || k == B_FX)
            continue;
        app_say("BUTTON", BTN_NAME[k]);
    }

    /* SMK-37 bank and page navigation buttons */
    if (sdown & (1u << SB_KNOB_BANK)) {
        st.knob_bank ^= 1u;
        mark_dirty();
        app_say("KNOB BANK", st.knob_bank ? "K9-K16" : "K1-K8");
    }
    if (sdown & (1u << SB_FADER_BANK)) {
        st.fader_bank ^= 1u;
        mark_dirty();
        app_say("FADER BANK", st.fader_bank ? "F5-F8" : "F1-F4");
    }
    /* < and >: the synth's previous / next patch (Program Change on key_ch); shown 1..128 */
    if (sdown & ((1u << SB_LEFT) | (1u << SB_RIGHT))) {
        int32_t prog = clamp((int32_t)ui.program + ((sdown & (1u << SB_RIGHT)) ? 1 : -1), 0, 127);
        ui.program = (uint8_t)prog;
        send(0xC0u | pr->key_ch, (uint32_t)prog, 0u);
        say_control("PROGRAM", prog + 1);
    }

    ui.btn = btn;
    ui.sb = sb;
}

static void set_pad_color(uint32_t p, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t rgb = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    if (ui.pad_rgb[p] != rgb) {
        ui.pad_rgb[p] = rgb;
        plat_pad_rgb((int)p, r, g, b);
    }
}

static void update_leds(uint32_t now_ms)
{
    const preset_t *pr = cur_preset();
    uint32_t b = 0, sb_lit = 0, p;
    uint8_t blink = (uint8_t)((now_ms / 200u) & 1u);

    if (blink != ui.blink_phase) {
        ui.blink_phase = blink;
        if (ui.rec_arm)
            ui.redraw = 1;
    }

    for (p = 0; p < SMK_NPADS; p++) {
        int active = (ui.pads_on >> p & 1u) || (ui.arp_on && ui.arp.gate_open && ui.arp.active_pad == p);
        if (active) {
            set_pad_color(p, 255, 160, 80);          /* Active / playing / arpeggiating pad: bright orange/white */
        } else if (ui.chord_on && ui.rec_arm) {
            if (blink)
                set_pad_color(p, 220, 24, 24);       /* SEQ REC armed: blinking red */
            else
                set_pad_color(p, 40, 0, 0);
        } else if (ui.chord_on) {
            if (pr->chords[p].count > 0)
                set_pad_color(p, 16, 96, 128);       /* Assigned custom chord pad: dim teal/blue */
            else
                set_pad_color(p, 0, 0, 0);           /* Unassigned custom pad: off */
        } else if (ui.scale_on) {
            if ((p & 7u) == 0u || (p & 7u) == 7u)
                set_pad_color(p, 180, 110, 16);      /* Diatonic scale tonic/root (I, I'): brighter amber */
            else
                set_pad_color(p, 32, 100, 40);       /* Diatonic scale degrees II..VII: dim green/amber */
        } else {
            set_pad_color(p, 0, 0, 0);
        }
    }

    if (st.octave < 0)
        b |= 1u << B_OCTDN;
    if (st.octave > 0)
        b |= 1u << B_OCTUP;
    if (ui.chord_on)
        b |= 1u << B_ENV;
    if (ui.scale_on)
        b |= 1u << B_SEL;
    if (ui.arp_on)
        b |= 1u << B_ARP;
    if (latch_active())
        b |= 1u << B_LFO;
    if (ui.page == PG_EDIT)
        b |= 1u << B_EDIT;
    if (ui.page == PG_GLOBAL || ui.page == PG_HW)
        b |= 1u << B_GLO;
    if (ui.dirty)
        b |= 1u << B_SAVE;
    if (ui.btn & (1u << B_PLAY))
        b |= 1u << B_PLAY;
    if (ui.rec_arm || (ui.btn & (1u << B_REC)))
        b |= 1u << B_REC;
    if (st.knob_bank)
        sb_lit |= 1u << SB_KNOB_BANK;
    if (st.fader_bank)
        sb_lit |= 1u << SB_FADER_BANK;
    plat_smk_leds(sb_lit);
    plat_glow(1);
    plat_leds(b, 0);
}

static void input(void)
{
    uint32_t now = plat_ms();
    note_refs_flush(&ui.nr);
    keys_input();
    pads_input();
    cs_strum_tick(&ui.strum, &ui.nr, now);
    arp_service(now);
    knobs_input();
    faders_input();
    wheels_input();
    pedal_input();
    buttons_input();
    update_leds(now);

    /* Auto-save: only after 2 s since last edit and 1.5 s of silence with nothing held or pending */
    if (ui.dirty && now - ui.dirty_at >= SAVE_IDLE_MS && now - ui.active_at >= SAVE_QUIET_MS &&
        plat_keybed() == 0 && plat_pads_held() == 0 && ui.btn == 0 && ui.sb == 0 && !ui.rec_arm &&
        !ui.pedal_val && !cs_strum_pending(&ui.strum) && note_refs_active(&ui.nr) == 0 && !note_refs_pending(&ui.nr)) {
        save_now(1);
    }
}

void app_input_only(void) { input(); }

/* ----------------------------------------------------------------- splash --- */
#include "chad_logo.h"                   /* generated at build time from app/logo/chad37.txt (app/tools/logo.py) */
#define LOGO_S 4u                        /* screen pixels per art pixel */
#define LOGO_X ((240u - CHAD_LOGO_W * LOGO_S) / 2u)
#define LOGO_Y 12u
_Static_assert(LOGO_Y + CHAD_LOGO_H * LOGO_S + 6u + 64u <= 240u, "the head, the name and the tagline fit the screen");

/* the boot screen (APP_SPLASH): the head, then the name and the tagline, on the platform's paper */
void app_splash(void)
{
    const uint16_t paper = RGB(250, 244, 232);
    lcd_fill(0, 0, 240, 240, paper);
    for (uint32_t i = 0; i < CHAD_LOGO_NRUNS; i++) {
        const uint8_t *r = chad_logo_runs[i];                        /* y, x, n, colour */
        lcd_fill(LOGO_X + r[1] * LOGO_S, LOGO_Y + r[0] * LOGO_S, r[2] * LOGO_S, LOGO_S, chad_logo_pal[r[3]]);
    }
    cv_begin(240, 64, paper);
    int32_t w = text_w(&APP_BIG_FONT, APP_NAME);
    cv_text((240 - w) / 2, 0, &APP_BIG_FONT, APP_NAME, APP_SPLASH_RGB);
    w = text_w(&FONT_S, APP_TAGLINE);
    cv_text((240 - w) / 2, 42, &FONT_S, APP_TAGLINE, RGB(150, 136, 118));
    cv_blit(0, LOGO_Y + CHAD_LOGO_H * LOGO_S + 6u);
}

/* --------------------------------------------------------------------- UI --- */
static void value_cell(int32_t x, int32_t y, const char *label, int32_t v, int is_rel)
{
    char t[12];
    cv_text(x, y, &FONT_XS, label, C_GRAY);
    if (is_rel) {
        cv_text(x + 26, y, &FONT_XS, "REL", C_AMB);
    } else {
        int_text(v, t);
        cv_text(x + 26, y, &FONT_XS, t, C_HI);
    }
}

static void choice_cell(int32_t x, int32_t y, const char *label, const char *val)
{
    cv_rect(x, y, 56, 36, C_LINE);
    cv_text(x + 4, y + 3, &FONT_XS, label, C_GRAY);
    cv_text(x + 4, y + 19, &FONT_XS, val, C_HI);
}

static void draw_header(const preset_t *pr)
{
    cv_begin(240, 32, C_BLACK);
    if (plat_ms() < ui.msg_until) {
        int32_t x = cv_text(8, 6, &FONT_B, ui.msg[0], C_WHITE);
        cv_text(x + 8, 6, &FONT_B, ui.msg[1], C_HI);
    } else {
        int32_t x = cv_text(8, 6, &FONT_B, pr->name, C_WHITE);
        if (ui.dirty)
            x = cv_text(x + 2, 6, &FONT_B, "*", C_AMB);
        if (ui.rec_arm) {
            cv_text(96, 9, &FONT_XS, "REC PAD", C_WHITE);
        } else {
            cv_text(92, 9, &FONT_T, latch_active() ? "LAT" : ui.chord_on ? "CHD" : "", C_HI);
            cv_text(114, 9, &FONT_T, ui.scale_on ? "SCL" : "", C_HI);
            cv_text(136, 9, &FONT_T, ui.arp_on ? "ARP" : (st.knob_bank ? "K9" : "K1"), ui.arp_on || st.knob_bank ? C_HI : C_GRAY);
        }
        cv_text(172, 9, &FONT_XS, st.fader_bank ? "F5-8" : "F1-4", st.fader_bank ? C_HI : C_GRAY);
        if (FM1B_BLE)
            cv_text(212, 9, &FONT_XS, "BLE", ui.ble_conn ? C_HI : C_DIM);
    }
    cv_line(0, 31, 239, 31, C_LINE);
    cv_blit(0, 0);
}

static void draw_main_body(const preset_t *pr)
{
    static const char *const KN[NBANK_KNOBS] = {
        "K1", "K2", "K3", "K4", "K5", "K6", "K7", "K8",
        "K9", "K10", "K11", "K12", "K13", "K14", "K15", "K16"
    };
    static const char *const FN[NBANK_FADERS] = {"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8"};
    static const char HEX[] = "0123456789ABCDEF";
    char h[12], t[16];
    uint32_t b, kbase = (uint32_t)st.knob_bank * NKNOBS, fbase = (uint32_t)st.fader_bank * NFADERS;
    int i;

    cv_begin(240, 144, C_BLACK);

    /* Top readout band: last control & MIDI packet on the left, chord / note box on the right */
    cv_rect(4, 0, 140, 34, C_LINE);
    if (ui.out_count) {
        int32_t x = cv_text(8, 2, &FONT_XS, ui.last_ctrl[0] ? ui.last_ctrl : "MIDI", C_WHITE);
        int_text(ui.last_val, t);
        cv_text(x + 6, 2, &FONT_XS, t, C_HI);
        for (b = 0; b < 3u; b++) {
            uint32_t v = (ui.last_out >> (8u + 8u * b)) & 0xFFu;
            h[3 * b] = HEX[v >> 4];
            h[3 * b + 1] = HEX[v & 15u];
            h[3 * b + 2] = b < 2u ? ' ' : 0;
        }
        cv_text(8, 18, &FONT_XS, h, C_GRAY);
        copy_str(t, sizeof t, "CH");
        int_text(ui.last_ch, t + 2);
        cv_text(104, 18, &FONT_XS, t, C_AMB);
    } else {
        cv_text(8, 10, &FONT_XS, "READY", C_GRAY);
    }

    cv_rect(148, 0, 88, 34, C_LINE);
    cv_text(152, 2, &FONT_T, ui.chord_on && ui.scale_on ? "EASY CHORD" : ui.chord_on ? "CHORD" : ui.scale_on ? "SCALE" : "NOTE", C_GRAY);
    cv_text(152, 15, &FONT_B, ui.chord_name[0] ? ui.chord_name : "--", C_HI);

    /* Knobs (2 rows x 4 columns) */
    for (i = 0; i < NKNOBS; i++) {
        uint32_t idx = kbase + (uint32_t)i;
        value_cell(4 + (i % 4) * 58, 40 + (i / 4) * 18, KN[idx], pr->knob_val[idx], pr->knob_rel[idx]);
    }

    /* Faders (1 row x 4 columns) */
    for (i = 0; i < NFADERS; i++) {
        uint32_t idx = fbase + (uint32_t)i;
        int32_t fv = ui.fader_phys[i] < 0 ? -1 : (int32_t)ui.fader_val[idx];
        value_cell(4 + i * 58, 82, FN[idx], fv, 0);
    }

    /* Wheels, Pedal, Master Volume */
    value_cell(4, 106, "PB", ui.pitch, 0);
    value_cell(62, 106, "MOD", ui.mod, 0);
    value_cell(120, 106, "PDL", ui.pedal_raw < 0 ? -1 : ui.pedal_val, 0);
    value_cell(178, 106, "VOL", st.k5_mode == K5_CC_ONLY ? 100 : (int32_t)(plat_master() * 100u / 4096u), 0);

    cv_line(0, 128, 239, 128, C_LINE);
    cv_text(4, 131, &FONT_T, "CHORD/SCALE/ARP: MODES  PARA: CH  GLOBE: MENU", C_DIM);
    cv_blit(0, 36);
}

static void draw_scale_body(void)
{
    char notes_line[32];
    uint32_t i;

    cv_begin(240, 144, C_BLACK);
    cv_text(4, 2, &FONT_B, ui.chord_on ? "SCALE + EASY CHORD (K1-K4)" : "SCALE MODE (K1-K4)", C_WHITE);
    choice_cell(4, 24, "K1 KEY", CS_ROOT_NAMES[st.scale_root]);
    choice_cell(62, 24, "K2 SCALE", CS_SCALE_NAMES[st.scale_type]);
    choice_cell(120, 24, "K3 VOICE", CS_VOICING_NAMES[st.voicing]);
    choice_cell(178, 24, "K4 PADS", CS_FLAVOR_NAMES[st.pad_flavor]);

    cv_text(4, 68, &FONT_XS, ui.chord_on ? "KEYS: EASY CHORD   PADS: CUSTOM CHORDS"
                                         : "KEYS: WHITE IN KEY  PADS: DIATONIC I-I'", C_GRAY);
    copy_str(notes_line, sizeof notes_line, "LAST: ");
    copy_str(notes_line + 6, sizeof notes_line - 6, ui.chord_name[0] ? ui.chord_name : "--");
    cv_text(4, 88, &FONT_XS, notes_line, C_HI);
    if (ui.latched_chord.count > 0) {
        int32_t x = 4;
        for (i = 0; i < ui.latched_chord.count; i++) {
            char nn[8];
            cs_note_name(ui.latched_chord.notes[i], nn);
            x = cv_text(x, 106, &FONT_XS, nn, C_AMB) + 8;
        }
    }
    cv_text(4, 126, &FONT_T, "PATCH: MAIN CC VIEW   SCALE: TURN OFF", C_AMB);
    cv_blit(0, 36);
}

static void draw_arp_body(void)
{
    char bpm_s[8], oct_s[8], gt_s[8];
    int_text(st.arp_bpm, bpm_s);
    int_text(st.arp_oct, oct_s);
    int_text(st.arp_gate, gt_s);

    cv_begin(240, 144, C_BLACK);
    cv_text(4, 2, &FONT_B, "ARPEGGIATOR (K1-K4)", C_WHITE);
    choice_cell(4, 24, "K1 BPM", bpm_s);
    choice_cell(62, 24, "K2 RATE", CS_ARP_DIV_NAMES[st.arp_div]);
    choice_cell(120, 24, "K3 MODE", CS_ARP_MODE_NAMES[st.arp_mode]);
    choice_cell(178, 24, "K4 OCT", oct_s);

    value_cell(4, 72, "GATE", st.arp_gate, 0);
    cv_text(74, 72, &FONT_XS, (ui.arp.ext_clk_last_ms && plat_ms() - ui.arp.ext_clk_last_ms < 500u)
                              ? "CLK: EXT MIDI" : "CLK: INTERNAL", C_HI);
    cv_text(4, 96, &FONT_XS, "HOLD KEYS OR CHORD PADS TO ARPEGGIATE", C_GRAY);
    cv_text(4, 126, &FONT_T, "PATCH: MAIN CC VIEW   ARP: TURN OFF", C_AMB);
    cv_blit(0, 36);
}

static void draw_edit_body(const preset_t *pr)
{
    static const char *const MAP_N[NPAD_MAPS] = {"STOCK", "DRUMS", "V016"};
    char pstr[8], kch[8], pch[8];
    int_text((int32_t)st.active_preset + 1, pstr);
    int_text((int32_t)pr->key_ch + 1, kch);
    int_text((int32_t)pr->pad_ch + 1, pch);

    cv_begin(240, 144, C_BLACK);
    cv_text(4, 2, &FONT_B, "PRESET & CHANNELS (K1-K4)", C_WHITE);
    choice_cell(4, 24, "K1 PRST", pstr);
    choice_cell(62, 24, "K2 KEYCH", kch);
    choice_cell(120, 24, "K3 PADCH", pch);
    choice_cell(178, 24, "K4 PADS", MAP_N[pr->pad_map]);

    cv_text(4, 70, &FONT_XS, "BANK 1: K1-8 CC 48-55 CH 6", C_GRAY);
    cv_text(4, 88, &FONT_XS, "BANK 2: K9-16 CC 102-109 CH 1", C_GRAY);
    cv_text(4, 106, &FONT_XS, "FADERS: F1-4 CC 20-23, F5-8 CC 24-27", C_GRAY);
    cv_text(4, 126, &FONT_T, "PATCH+PAD 1-8: QUICK PRESET   BT: SAVE", C_AMB);
    cv_blit(0, 36);
}

static void draw_global_body(void)
{
    char st_s[12], gt_s[12];
    if (st.strum_ms == 0) {
        copy_str(st_s, sizeof st_s, "OFF");
    } else {
        int_text(st.strum_ms, st_s);
    }
    int_text(st.arp_gate, gt_s);

    cv_begin(240, 144, C_BLACK);
    cv_text(4, 2, &FONT_B, "GLOBAL MENU: STRUM & VOICE", C_WHITE);
    choice_cell(4, 24, "K1 STRUM", st_s);
    choice_cell(62, 24, "K2 DIR", CS_STRUM_DIR_NAMES[st.strum_dir]);
    choice_cell(120, 24, "K3 VOICE", CS_VOICING_NAMES[st.voicing]);
    choice_cell(178, 24, "K4 GATE%", gt_s);

    cv_text(4, 72, &FONT_XS, "AUTO-STRUM STAGGERS CHORD PAD & EASY", C_GRAY);
    cv_text(4, 90, &FONT_XS, "CHORD TONES NON-BLOCKINGLY (0-120 MS)", C_GRAY);
    cv_text(4, 126, &FONT_T, "GLOBE AGAIN: HARDWARE SETUP   PATCH: MAIN", C_AMB);
    cv_blit(0, 36);
}

static void draw_hw_body(void)
{
    static const char *const PDL_N[NPEDAL_MODES] = {"AUTO", "OPEN", "CLOSED", "EXPR"};
    static const char *const K5_N[NK5_MODES] = {"CC+VOL", "CC ONLY"};
    static const char *const CRV_N[NCURVES] = {"FACTORY", "SOFT", "HARD", "FIXED"};

    cv_begin(240, 144, C_BLACK);
    cv_text(4, 2, &FONT_B, "HARDWARE SETUP (K1-K4)", C_WHITE);
    choice_cell(4, 24, "K1 PEDAL", PDL_N[st.pedal_mode]);
    choice_cell(62, 24, "K2 K5", K5_N[st.k5_mode]);
    choice_cell(120, 24, "K3 CURVE", CRV_N[st.vel_curve]);
    choice_cell(178, 24, "K4 THEME", PALETTES[st.theme].name);

    value_cell(4, 72, "JACK", ui.pedal_raw, 0);
    value_cell(62, 72, "REST", ui.jack_init ? ui.jack_rest : -1, 0);
    value_cell(120, 72, "ACTV", (int32_t)note_refs_active(&ui.nr), 0);
    value_cell(178, 72, "PEND", note_refs_pending(&ui.nr), 0);

    {
        char t[32] = "K6 PAD MIN VEL: ";
        int_text(st.pad_vel_min > 1u ? st.pad_vel_min : 1, t + 16);
        if (st.pad_vel_min <= 1u)
            copy_str(t + 16, sizeof t - 16, "OFF");
        cv_text(4, 102, &FONT_XS, t, C_GRAY);
    }
    cv_text(4, 126, &FONT_T, "PATCH: MAIN VIEW   FX: ALL NOTES OFF", C_AMB);
    cv_blit(0, 36);
}

static void draw_footer(const preset_t *pr)
{
    static const char *const PG_TAG[NPAGES] = {"MAIN", "EDIT", "GLOB", "SCAL", "ARP", "HW"};
    char t[16];
    uint32_t k, p;
    cv_begin(240, 60, C_BLACK);
    cv_line(0, 0, 239, 0, C_LINE);

    /* 37-key mini indicator (x = 9..230, 6 px pitch) */
    for (k = 0; k < SMK_NKEYBED; k++) {
        uint16_t c = (ui.keys_on >> k & 1u) ? C_HI : C_LINE;
        cv_rect(9 + (int32_t)k * 6, 6, 5, 14, c);
    }

    /* 16-pad mini indicator (x = 9..232, 14 px pitch) */
    for (p = 0; p < SMK_NPADS; p++) {
        int assigned = ui.chord_on ? (pr->chords[p].count > 0) : 1;
        uint16_t c = (ui.pads_on >> p & 1u) ? C_HI : assigned ? C_DIM : C_LINE;
        cv_rect(9 + (int32_t)p * 14, 24, 12, 8, c);
    }

    /* Bottom status bar: OCT, KEY CH, PAD CH, active page */
    copy_str(t, sizeof t, "OCT ");
    int_text(st.octave, t + 4);
    cv_text(8, 38, &FONT_XS, t, st.octave ? C_HI : C_GRAY);

    copy_str(t, sizeof t, "KEY CH ");
    int_text((int32_t)pr->key_ch + 1, t + 7);
    cv_text(68, 38, &FONT_XS, t, C_GRAY);

    copy_str(t, sizeof t, "PAD CH ");
    int_text((int32_t)pr->pad_ch + 1, t + 7);
    cv_text(138, 38, &FONT_XS, t, C_GRAY);

    cv_text(204, 38, &FONT_XS, PG_TAG[ui.page % NPAGES], C_AMB);
    cv_blit(0, 180);
}

#include "chord_ui.c"                   /* chord performance view and illustrated Easy Chord introduction */

static void draw(void)
{
    const preset_t *pr = cur_preset();
    if (ui.chord_on && (ui.page == PG_MAIN || ui.page == PG_SCALE)) {
        draw_chord_view(pr);
        return;
    }
    draw_header(pr);
    if (ui.page == PG_EDIT)
        draw_edit_body(pr);
    else if (ui.page == PG_SCALE)
        draw_scale_body();
    else if (ui.page == PG_ARP)
        draw_arp_body();
    else if (ui.page == PG_GLOBAL)
        draw_global_body();
    else if (ui.page == PG_HW)
        draw_hw_body();
    else
        draw_main_body(pr);
    draw_footer(pr);
}

void app_ui_init(void)
{
    int i;
    plat_smk_pads(SMK_PADS_OWN);
    plat_pad_lights(1);
    for (i = 0; i < NFADERS; i++)
        ui.fader_phys[i] = -1;
    for (i = 0; i < SMK_NPADS; i++)
        ui.pad_rgb[i] = 0xFFFFFFFFu;
    ui.pedal_raw = -1;
    ui.ble_conn = (uint8_t)(plat_ble_connected() != 0);
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.redraw = 1;
}

void app_frame(void)
{
    uint32_t now = plat_ms();
    uint8_t ble = (uint8_t)(plat_ble_connected() != 0);
    if (ble != ui.ble_conn) {
        ui.ble_conn = ble;
        ui.redraw = 1;
    }
    input();
    if (ann_on() && ann_where()) {
        if (!say_chord_help())
            ann_say(APP_NAME, ui.chord_on && ui.chord_name[0] ? ui.chord_name : cur_preset()->name, ANN_NOW);
    }
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
void app_host_reboot(void)
{
    memset(&ui, 0, sizeof ui);
    memset(&st, 0, sizeof st);
}

/* store_migrate on a CPD1 store with stock's defaults (faders on 64+, K7-K16 relative) and one knob a user moved:
 * 1 when every default moved, the user's knob stayed and the result is a valid current store */
static int host_store_migrate_check(void)
{
    static presets_store_t s;
    uint32_t i, k;
    s = st;
    for (i = 0; i < NPRESETS; i++) {
        preset_t *p = &s.presets[i];
        for (k = 0; k < NBANK_FADERS; k++)
            p->fader_cc[k] = (uint8_t)(OLD_FADER_CC0 + k);
        for (k = OLD_KNOB_REL0; k < NBANK_KNOBS; k++) {
            p->knob_cc[k] = OLD_KNOB_CC[k];
            p->knob_ch[k] = 0;
            p->knob_rel[k] = 1;
        }
    }
    s.presets[2].knob_cc[6] = 99;                          /* preset 3's K7: the user's own CC, kept relative */
    s.magic = STORE_MAGIC_1;
    if (!store_migrate(&s) || s.magic != STORE_MAGIC || !store_valid(&s))
        return 0;
    for (i = 0; i < NPRESETS; i++) {
        const preset_t *p = &s.presets[i];
        for (k = 0; k < NBANK_FADERS; k++)
            if (p->fader_cc[k] != DEF_FADER_CC[k])
                return 0;
        for (k = 0; k < NBANK_KNOBS; k++) {
            int mine = i == 2 && k == 6;
            if (mine ? (p->knob_cc[k] != 99 || !p->knob_rel[k] || p->knob_ch[k] != 0)
                     : (p->knob_cc[k] != DEF_KNOB_CC[k] || p->knob_rel[k] || p->knob_ch[k] != DEF_KNOB_CH[k]))
                return 0;
        }
    }
    s.magic = STORE_MAGIC;                                  /* a current store is left alone */
    return !store_migrate(&s);
}

int app_expect(const char *what, int *got)
{
    const preset_t *pr = cur_preset();
    if (!strcmp(what, "octave"))
        *got = st.octave;
    else if (!strcmp(what, "pending_off"))
        *got = note_refs_pending(&ui.nr);
    else if (!strcmp(what, "active_notes"))
        *got = (int)note_refs_active(&ui.nr);
    else if (!strcmp(what, "knob_bank"))
        *got = st.knob_bank;
    else if (!strcmp(what, "fader_bank"))
        *got = st.fader_bank;
    else if (!strcmp(what, "preset"))
        *got = (int)st.active_preset + 1;
    else if (!strcmp(what, "key_ch"))
        *got = (int)pr->key_ch + 1;
    else if (!strcmp(what, "pad_ch"))
        *got = (int)pr->pad_ch + 1;
    else if (!strcmp(what, "pad_map"))
        *got = pr->pad_map;
    else if (!strcmp(what, "pedal_mode"))
        *got = st.pedal_mode;
    else if (!strcmp(what, "k5_mode"))
        *got = st.k5_mode;
    else if (!strcmp(what, "pad_vel_min"))
        *got = st.pad_vel_min;
    else if (!strcmp(what, "page"))
        *got = ui.page;
    else if (!strcmp(what, "store_migrate"))
        *got = host_store_migrate_check();
    else if (!strcmp(what, "dirty"))
        *got = ui.dirty;
    else if (!strcmp(what, "chord_on"))
        *got = ui.chord_on;
    else if (!strcmp(what, "scale_on"))
        *got = ui.scale_on;
    else if (!strcmp(what, "arp_on"))
        *got = ui.arp_on;
    else if (!strcmp(what, "program"))
        *got = ui.program;
    else if (!strcmp(what, "latch_on"))
        *got = latch_active();
    else if (!strcmp(what, "latched_pad"))
        *got = ui.latched_pad ? __builtin_ctz(ui.latched_pad) + 1 : 0;
    else if (!strcmp(what, "rec_arm"))
        *got = ui.rec_arm;
    else if (!strcmp(what, "chord_help"))
        *got = ui.chord_help;
    else if (!strcmp(what, "scale_root"))
        *got = st.scale_root;
    else if (!strcmp(what, "scale_type"))
        *got = st.scale_type;
    else if (!strcmp(what, "voicing"))
        *got = st.voicing;
    else if (!strcmp(what, "pad_flavor"))
        *got = st.pad_flavor;
    else if (!strcmp(what, "strum_ms"))
        *got = st.strum_ms;
    else if (!strcmp(what, "strum_dir"))
        *got = st.strum_dir;
    else if (!strcmp(what, "arp_bpm"))
        *got = st.arp_bpm;
    else if (!strcmp(what, "arp_div"))
        *got = st.arp_div;
    else if (!strcmp(what, "arp_mode"))
        *got = st.arp_mode;
    else if (!strcmp(what, "arp_oct"))
        *got = st.arp_oct;
    else if (!strcmp(what, "arp_gate"))
        *got = st.arp_gate;
    else if (!strcmp(what, "latched_count"))
        *got = ui.latched_chord.count;
    else if (!strncmp(what, "pad_chord", 9) && what[9] >= '0' && what[9] <= '9') {
        int p = atoi(what + 9);
        if (p < 0 || p >= SMK_NPADS)
            return 0;
        *got = pr->chords[p].count;
    } else if (!strncmp(what, "knob", 4)) {
        int k = atoi(what + 4);
        if (what[4] < '0' || what[4] > '9' || k < 0 || k >= NBANK_KNOBS)
            return 0;
        *got = pr->knob_val[k];
    } else
        return 0;
    return 1;
}
#endif
