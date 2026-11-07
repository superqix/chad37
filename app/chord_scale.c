/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Michael Wilson */
#include "chord_scale.h"

const char *const CS_ROOT_NAMES[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
const char *const CS_SCALE_NAMES[NSCALES] = {
    "MAJOR", "NAT MIN", "HARM MIN", "DORIAN", "PHRYGIAN", "LYDIAN", "MIXOLYDN", "PENT MAJ", "PENT MIN"
};
const char *const CS_VOICING_NAMES[NVOICINGS] = {"ROOT", "1ST INV", "2ND INV", "SPREAD"};
const char *const CS_FLAVOR_NAMES[NFLAVORS] = {"TRIADS", "7THS", "9THS"};
const char *const CS_STRUM_DIR_NAMES[NSTRUM_DIRS] = {"UP", "DOWN", "ALT"};
const char *const CS_ARP_MODE_NAMES[NARP_MODES] = {"UP", "DOWN", "UP/DN", "RANDOM", "CHORD"};
const char *const CS_ARP_DIV_NAMES[NARP_DIVS] = {"1/4", "1/8", "1/16", "1/8T"};

/* 7 white-key intervals per octave for each scale; pentatonics continue into the next octave on keys 6-7 */
static const uint8_t SCALE_KEY_INT[NSCALES][7] = {
    [SCALE_MAJOR]      = {0, 2, 4, 5, 7, 9, 11},
    [SCALE_NAT_MINOR]  = {0, 2, 3, 5, 7, 8, 10},
    [SCALE_HARM_MINOR] = {0, 2, 3, 5, 7, 8, 11},
    [SCALE_DORIAN]     = {0, 2, 3, 5, 7, 9, 10},
    [SCALE_PHRYGIAN]   = {0, 1, 3, 5, 7, 8, 10},
    [SCALE_LYDIAN]     = {0, 2, 4, 6, 7, 9, 11},
    [SCALE_MIXOLYDIAN] = {0, 2, 4, 5, 7, 9, 10},
    [SCALE_PENT_MAJOR] = {0, 2, 4, 7, 9, 12, 14},
    [SCALE_PENT_MINOR] = {0, 3, 5, 7, 10, 12, 15},
};

/* 7-degree diatonic scale used to stack thirds for the 16 scale pads (pentatonics use parent Major/Minor) */
static const uint8_t SCALE_DIATONIC[NSCALES][7] = {
    [SCALE_MAJOR]      = {0, 2, 4, 5, 7, 9, 11},
    [SCALE_NAT_MINOR]  = {0, 2, 3, 5, 7, 8, 10},
    [SCALE_HARM_MINOR] = {0, 2, 3, 5, 7, 8, 11},
    [SCALE_DORIAN]     = {0, 2, 3, 5, 7, 9, 10},
    [SCALE_PHRYGIAN]   = {0, 1, 3, 5, 7, 8, 10},
    [SCALE_LYDIAN]     = {0, 2, 4, 6, 7, 9, 11},
    [SCALE_MIXOLYDIAN] = {0, 2, 4, 5, 7, 9, 10},
    [SCALE_PENT_MAJOR] = {0, 2, 4, 5, 7, 9, 11},
    [SCALE_PENT_MINOR] = {0, 2, 3, 5, 7, 8, 10},
};

/* White key degree (0..6) for semitone 0..11 within an octave, or -1 for black keys */
static const int8_t WHITE_DEG[12] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};

static void int_to_str(int32_t v, char *b)
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

void cs_note_name(uint32_t note, char *buf)
{
    const char *p = CS_ROOT_NAMES[note % 12u];
    int32_t oct = (int32_t)(note / 12u) - 1;
    while (*p)
        *buf++ = *p++;
    int_to_str(oct, buf);
}

static void sort_notes(uint8_t *a, uint8_t n)
{
    uint8_t i, j;
    for (i = 1; i < n; i++) {
        uint8_t v = a[i];
        j = i;
        while (j > 0 && a[j - 1] > v) {
            a[j] = a[j - 1];
            j--;
        }
        a[j] = v;
    }
}

/* Additional chord templates beyond EASY_CHORD_TYPES for manual chord recognition */
#define CS_TYPE_DIM    (EASY_CHORD_TYPES + 0)
#define CS_TYPE_AUG    (EASY_CHORD_TYPES + 1)
#define CS_TYPE_SUS2   (EASY_CHORD_TYPES + 2)
#define CS_TYPE_M7B5   (EASY_CHORD_TYPES + 3)
#define CS_TYPE_5      (EASY_CHORD_TYPES + 4)
#define CS_TYPE_CUSTOM 0xFEu
#define CS_TYPE_SINGLE 0xFFu

static const struct {
    uint8_t type;
    uint16_t pc_mask;
    uint16_t no5_mask;
    const char *suffix;
} CHORD_PATTERNS[] = {
    {EASY_CHORD_MAJOR, (1u << 0) | (1u << 4) | (1u << 7), 0, "Maj"},
    {EASY_CHORD_MINOR, (1u << 0) | (1u << 3) | (1u << 7), 0, "Min"},
    {EASY_CHORD_DOM7,  (1u << 0) | (1u << 4) | (1u << 7) | (1u << 10), (1u << 0) | (1u << 4) | (1u << 10), "7"},
    {EASY_CHORD_MAJ7,  (1u << 0) | (1u << 4) | (1u << 7) | (1u << 11), (1u << 0) | (1u << 4) | (1u << 11), "Maj7"},
    {EASY_CHORD_MIN7,  (1u << 0) | (1u << 3) | (1u << 7) | (1u << 10), (1u << 0) | (1u << 3) | (1u << 10), "Min7"},
    {EASY_CHORD_MAJ9,  (1u << 0) | (1u << 4) | (1u << 7) | (1u << 11) | (1u << 2), (1u << 0) | (1u << 4) | (1u << 11) | (1u << 2), "Maj9"},
    {EASY_CHORD_MIN9,  (1u << 0) | (1u << 3) | (1u << 7) | (1u << 10) | (1u << 2), (1u << 0) | (1u << 3) | (1u << 10) | (1u << 2), "Min9"},
    {EASY_CHORD_ADD9,  (1u << 0) | (1u << 4) | (1u << 7) | (1u << 2), 0, "Add9"},
    {EASY_CHORD_SUS4,  (1u << 0) | (1u << 5) | (1u << 7), 0, "Sus4"},
    {EASY_CHORD_7SUS4, (1u << 0) | (1u << 5) | (1u << 7) | (1u << 10), (1u << 0) | (1u << 5) | (1u << 10), "7sus4"},
    {EASY_CHORD_DIM7,  (1u << 0) | (1u << 3) | (1u << 6) | (1u << 9), 0, "Dim7"},
    {CS_TYPE_M7B5,     (1u << 0) | (1u << 3) | (1u << 6) | (1u << 10), 0, "m7b5"},
    {CS_TYPE_DIM,      (1u << 0) | (1u << 3) | (1u << 6), 0, "Dim"},
    {CS_TYPE_AUG,      (1u << 0) | (1u << 4) | (1u << 8), 0, "Aug"},
    {CS_TYPE_SUS2,     (1u << 0) | (1u << 2) | (1u << 7), 0, "Sus2"},
    {CS_TYPE_5,        (1u << 0) | (1u << 7), 0, "5"},
};
#define NPATTERNS (sizeof(CHORD_PATTERNS) / sizeof(CHORD_PATTERNS[0]))

static uint16_t rot12(uint16_t m, uint32_t r)
{
    r %= 12u;
    return (uint16_t)(((m << r) | (m >> (12u - r))) & 0xFFFu);
}

void cs_chord_format_name(const cs_chord_t *ch, char name[16])
{
    uint32_t i;
    if (!ch || ch->count == 0) {
        name[0] = 0;
        return;
    }
    if (ch->type == CS_TYPE_SINGLE || ch->count == 1) {
        cs_note_name(ch->notes[0], name);
        return;
    }
    if (ch->type < EASY_CHORD_TYPES) {
        easy_chord_name(ch->root, ch->type, name);
        return;
    }
    for (i = 0; i < NPATTERNS; i++) {
        if (CHORD_PATTERNS[i].type == ch->type) {
            const char *r = CS_ROOT_NAMES[ch->root % 12u];
            const char *s = CHORD_PATTERNS[i].suffix;
            char *d = name;
            while (*r)
                *d++ = *r++;
            *d++ = ' ';
            while (*s)
                *d++ = *s++;
            *d = 0;
            return;
        }
    }
    cs_note_name(ch->notes[0], name);
    {
        uint32_t len = 0;
        while (name[len])
            len++;
        if (len + 2u < 16u) {
            name[len] = '+';
            name[len + 1u] = 0;
        }
    }
}

static void identify_notes(cs_chord_t *out, char name[16])
{
    uint16_t pc_mask = 0;
    uint8_t i, step, bass_pc;
    if (out->count == 0) {
        out->root = 0;
        out->type = 0;
        name[0] = 0;
        return;
    }
    if (out->count == 1) {
        out->root = out->notes[0];
        out->type = CS_TYPE_SINGLE;
        cs_chord_format_name(out, name);
        return;
    }
    for (i = 0; i < out->count; i++)
        pc_mask |= (uint16_t)(1u << (out->notes[i] % 12u));
    bass_pc = out->notes[0] % 12u;

    /* Pass 1: exact template match; Pass 2: match with omitted 5th; check bass_pc first */
    for (step = 0; step < 2; step++) {
        uint8_t c;
        for (c = 0; c < 12; c++) {
            uint8_t r = (uint8_t)((bass_pc + c) % 12u);
            uint32_t p;
            if (!(pc_mask & (1u << r)))
                continue;
            for (p = 0; p < NPATTERNS; p++) {
                uint16_t pat = step == 0 ? CHORD_PATTERNS[p].pc_mask : CHORD_PATTERNS[p].no5_mask;
                if (pat && pc_mask == rot12(pat, r)) {
                    out->root = (uint8_t)((out->notes[0] / 12u) * 12u + r);
                    out->type = CHORD_PATTERNS[p].type;
                    cs_chord_format_name(out, name);
                    return;
                }
            }
        }
    }
    out->root = out->notes[0];
    out->type = CS_TYPE_CUSTOM;
    cs_chord_format_name(out, name);
}

uint8_t cs_detect_manual_chord(const uint8_t held[16], cs_chord_t *out, char name[16])
{
    uint8_t n = 0;
    int note;
    *out = (cs_chord_t){0};
    for (note = 0; note < 128; note++) {
        if (held[note >> 3] & (1u << (note & 7))) {
            if (n < CS_MAX_NOTES)
                out->notes[n] = (uint8_t)note;
            n++;
        }
    }
    out->count = n > CS_MAX_NOTES ? CS_MAX_NOTES : n;
    identify_notes(out, name);
    return n;
}

void cs_apply_voicing(uint8_t *tones, uint8_t count, uint8_t voicing)
{
    if (count < 2 || voicing == VOICING_ROOT)
        return;
    sort_notes(tones, count);
    if (voicing == VOICING_INV1) {
        if (tones[0] + 12u <= 127u)
            tones[0] = (uint8_t)(tones[0] + 12u);
    } else if (voicing == VOICING_INV2) {
        if (tones[0] + 12u <= 127u)
            tones[0] = (uint8_t)(tones[0] + 12u);
        if (tones[1] + 12u <= 127u)
            tones[1] = (uint8_t)(tones[1] + 12u);
    } else if (voicing == VOICING_SPREAD) {
        if (tones[1] + 12u <= 127u)
            tones[1] = (uint8_t)(tones[1] + 12u);
    }
    sort_notes(tones, count);
}

int cs_resolve_easy_chord(const uint8_t raw_held[16], uint8_t scale_root, uint8_t voicing,
                          cs_chord_t *out, char name[16])
{
    struct easy_chord ec;
    uint32_t root;
    uint8_t i;
    *out = (cs_chord_t){0};
    if (!easy_chord_resolve(raw_held, &ec, 1)) {
        name[0] = 0;
        return 0;
    }
    root = (uint32_t)ec.root + (scale_root % 12u);
    if (root > 127u)
        root = 127u;
    out->root = (uint8_t)root;
    out->type = ec.type;
    out->count = (uint8_t)easy_chord_tones(root, ec.type, out->notes);
    for (i = 0; i < out->count; i++)
        if (out->notes[i] > 127u)
            out->notes[i] = 127u;
    cs_apply_voicing(out->notes, out->count, voicing);
    easy_chord_name(out->root, out->type, name);
    return 1;
}

int cs_scale_key_note(uint32_t key_idx, int8_t octave, uint8_t scale_root, uint8_t scale_type)
{
    int8_t deg;
    int32_t note;
    if (key_idx >= 37u)
        return -1;
    deg = WHITE_DEG[key_idx % 12u];
    if (deg < 0)
        return -1;                       /* black keys are muted in Scale-only mode */
    note = 48 + 12 * (int32_t)octave + (int32_t)(scale_root % 12u) +
           (int32_t)(key_idx / 12u) * 12 + (int32_t)SCALE_KEY_INT[scale_type % NSCALES][deg];
    return note < 0 ? 0 : note > 127 ? 127 : note;
}

static uint8_t diatonic_pitch(int32_t base_c, uint8_t scale_root, uint8_t scale_type, uint32_t deg)
{
    const uint8_t *sc = SCALE_DIATONIC[scale_type % NSCALES];
    int32_t n = base_c + (int32_t)(scale_root % 12u) + (int32_t)(deg / 7u) * 12 + (int32_t)sc[deg % 7u];
    return (uint8_t)(n < 0 ? 0 : n > 127 ? 127 : n);
}

void cs_scale_pad_chord(uint32_t pad, int8_t octave, uint8_t scale_root, uint8_t scale_type,
                        uint8_t voicing, uint8_t flavor, cs_chord_t *out, char name[16])
{
    uint32_t top_row = pad < 8u;
    uint32_t deg = pad & 7u;             /* 0..6 = degrees I..VII, 7 = I' (+1 octave) */
    int32_t base_c = 48 + 12 * (int32_t)octave;
    uint8_t n_tones, i;

    *out = (cs_chord_t){0};
    if (!top_row) {
        /* Bottom row (P8..P15): diatonic triads (or 7ths when flavor == FLAVOR_9THS) */
        n_tones = flavor == FLAVOR_9THS ? 4u : 3u;
    } else {
        /* Top row (P0..P7): shaped by K4 Pad Chord Flavor (Triads, 7ths, 9ths) */
        n_tones = flavor == FLAVOR_TRIADS ? 3u : flavor == FLAVOR_7THS ? 4u : 5u;
        if (flavor == FLAVOR_TRIADS)
            base_c += 12;                /* top row triads +1 octave above bottom row */
    }
    for (i = 0; i < n_tones; i++)
        out->notes[i] = diatonic_pitch(base_c, scale_root, scale_type, deg + 2u * i);
    out->count = n_tones;
    identify_notes(out, name);
    cs_apply_voicing(out->notes, out->count, voicing);
}

/* ----------------------------------------------------------- Auto-Strum --- */
void cs_strum_init(cs_strum_t *st)
{
    uint8_t *p = (uint8_t *)st;
    uint32_t i;
    for (i = 0; i < sizeof *st; i++)
        p[i] = 0;
}

static void cancel_src_queue(cs_strum_t *st, uint8_t src)
{
    uint32_t i;
    for (i = 0; i < CS_STRUM_QMAX; i++)
        if (st->q[i].active && st->q[i].src == src)
            st->q[i].active = 0;
}

static int note_in_list(uint8_t note, const uint8_t *list, uint8_t count)
{
    uint8_t i;
    for (i = 0; i < count; i++)
        if (list[i] == note)
            return 1;
    return 0;
}

void cs_strum_release_src(cs_strum_t *st, note_refs_t *nr, uint8_t src)
{
    uint8_t i;
    if (src >= CS_STRUM_NSRC)
        return;
    cancel_src_queue(st, src);
    for (i = 0; i < st->fired_count[src]; i++)
        note_refs_off(nr, st->fired_ch[src], st->fired_notes[src][i]);
    st->fired_count[src] = 0;
}

void cs_strum_release_all(cs_strum_t *st, note_refs_t *nr)
{
    uint8_t s;
    uint32_t i;
    for (s = 0; s < CS_STRUM_NSRC; s++)
        cs_strum_release_src(st, nr, s);
    for (i = 0; i < CS_STRUM_QMAX; i++)
        st->q[i].active = 0;
}

void cs_strum_trigger(cs_strum_t *st, note_refs_t *nr, uint8_t src, uint8_t ch,
                      const uint8_t *notes, uint8_t count, uint8_t vel,
                      uint8_t strum_ms, uint8_t strum_dir, uint32_t now_ms)
{
    uint8_t ord[CS_MAX_NOTES];
    uint8_t new_fired[CS_MAX_NOTES];
    uint8_t n_fired = 0, i, down;

    if (src >= CS_STRUM_NSRC)
        return;
    if (count > CS_MAX_NOTES)
        count = CS_MAX_NOTES;
    cancel_src_queue(st, src);

    if (count == 0) {
        cs_strum_release_src(st, nr, src);
        return;
    }

    /* If channel changed, release all previously fired notes on the old channel */
    if (st->fired_count[src] && st->fired_ch[src] != ch)
        cs_strum_release_src(st, nr, src);
    st->fired_ch[src] = ch;

    /* Immediate morph when strum_ms == 0 */
    if (strum_ms == 0) {
        note_refs_morph(nr, ch, st->fired_notes[src], st->fired_count[src], notes, count, vel);
        for (i = 0; i < count; i++)
            st->fired_notes[src][i] = notes[i];
        st->fired_count[src] = count;
        return;
    }

    /* Release previously fired notes that are not in the new chord */
    for (i = 0; i < st->fired_count[src]; i++) {
        uint8_t old_n = st->fired_notes[src][i];
        if (note_in_list(old_n, notes, count))
            new_fired[n_fired++] = old_n;
        else
            note_refs_off(nr, ch, old_n);
    }
    for (i = 0; i < n_fired; i++)
        st->fired_notes[src][i] = new_fired[i];
    st->fired_count[src] = n_fired;

    /* Order tones according to strum_dir (UP, DOWN, ALT) */
    for (i = 0; i < count; i++)
        ord[i] = notes[i];
    sort_notes(ord, count);
    down = strum_dir == STRUM_DOWN || (strum_dir == STRUM_ALT && (st->alt_flip & 1u));
    if (strum_dir == STRUM_ALT)
        st->alt_flip ^= 1u;
    if (down) {
        for (i = 0; i < count / 2u; i++) {
            uint8_t tmp = ord[i];
            ord[i] = ord[count - 1u - i];
            ord[count - 1u - i] = tmp;
        }
    }

    /* Fire or schedule each tone that is not already sounding for this source */
    {
        uint8_t step = 0;
        for (i = 0; i < count; i++) {
            uint8_t n = ord[i];
            if (note_in_list(n, st->fired_notes[src], st->fired_count[src]))
                continue;
            if (step == 0) {
                note_refs_on(nr, ch, n, vel);
                if (st->fired_count[src] < CS_MAX_NOTES)
                    st->fired_notes[src][st->fired_count[src]++] = n;
            } else {
                uint32_t q_idx;
                for (q_idx = 0; q_idx < CS_STRUM_QMAX; q_idx++) {
                    if (!st->q[q_idx].active) {
                        st->q[q_idx].active = 1;
                        st->q[q_idx].src = src;
                        st->q[q_idx].ch = ch;
                        st->q[q_idx].note = n;
                        st->q[q_idx].vel = vel;
                        st->q[q_idx].fire_ms = now_ms + (uint32_t)step * (uint32_t)strum_ms;
                        break;
                    }
                }
            }
            step++;
        }
    }
}

void cs_strum_tick(cs_strum_t *st, note_refs_t *nr, uint32_t now_ms)
{
    uint32_t i;
    for (i = 0; i < CS_STRUM_QMAX; i++) {
        cs_strum_ev_t *e = &st->q[i];
        if (!e->active || (int32_t)(now_ms - e->fire_ms) < 0)
            continue;
        e->active = 0;
        if (e->src < CS_STRUM_NSRC && !note_in_list(e->note, st->fired_notes[e->src], st->fired_count[e->src])) {
            note_refs_on(nr, e->ch, e->note, e->vel);
            if (st->fired_count[e->src] < CS_MAX_NOTES)
                st->fired_notes[e->src][st->fired_count[e->src]++] = e->note;
        }
    }
}

int cs_strum_pending(const cs_strum_t *st)
{
    uint32_t i;
    for (i = 0; i < CS_STRUM_QMAX; i++)
        if (st->q[i].active)
            return 1;
    return 0;
}

/* ---------------------------------------------------------- Arpeggiator --- */
void cs_arp_init(cs_arp_t *arp)
{
    uint8_t *p = (uint8_t *)arp;
    uint32_t i;
    for (i = 0; i < sizeof *arp; i++)
        p[i] = 0;
    arp->active_pad = 0xFF;
    arp->dir = 1;
    arp->rng = 0x12345678u;
}

static void arp_gate_close(cs_arp_t *arp, note_refs_t *nr)
{
    uint8_t i;
    for (i = 0; i < arp->sounding_count; i++)
        note_refs_off(nr, arp->sounding_ch, arp->sounding_notes[i]);
    arp->sounding_count = 0;
    arp->active_pad = 0xFF;
    arp->gate_open = 0;
}

void cs_arp_stop(cs_arp_t *arp, note_refs_t *nr)
{
    arp_gate_close(arp, nr);
    arp->was_holding = 0;
    arp->step_idx = 0;
    arp->dir = 1;
    arp->ext_clk_ticks = 0;
    arp->ext_step_due = 0;
}

static uint8_t div_clocks(uint8_t arp_div)
{
    static const uint8_t CLK[NARP_DIVS] = {24, 12, 6, 8};
    return CLK[arp_div % NARP_DIVS];
}

static uint32_t step_period_ms(uint8_t bpm, uint8_t arp_div)
{
    uint32_t b = bpm < 40u ? 40u : bpm > 240u ? 240u : bpm;
    switch (arp_div % NARP_DIVS) {
    case ARP_DIV_1_4:  return 60000u / b;
    case ARP_DIV_1_8:  return 30000u / b;
    case ARP_DIV_1_16: return 15000u / b;
    case ARP_DIV_1_8T: return 20000u / b;
    default:           return 30000u / b;
    }
}

void cs_arp_midi_clock(cs_arp_t *arp, uint8_t status_byte, uint8_t arp_div, uint32_t now_ms)
{
    if (status_byte == 0xFAu) {          /* MIDI Start */
        arp->step_idx = 0;
        arp->dir = 1;
        arp->ext_clk_ticks = 0;
        arp->ext_step_due = 1;
        arp->ext_clk_last_ms = now_ms;
    } else if (status_byte == 0xF8u) {   /* MIDI Timing Clock (24 PPQN) */
        uint8_t c_per_step = div_clocks(arp_div);
        arp->ext_clk_last_ms = now_ms;
        if (arp->ext_clk_ticks == 0u)
            arp->ext_step_due = 1;
        arp->ext_clk_ticks = (uint8_t)((arp->ext_clk_ticks + 1u) % c_per_step);
    }
}

static uint8_t build_tone_pool(const cs_chord_t *chords, uint8_t n_chords, uint8_t arp_oct,
                               uint8_t pool[CS_ARP_MAX_POOL])
{
    uint8_t base[CS_ARP_MAX_POOL];
    uint8_t n_base = 0, n_pool = 0, c, i, oct;
    uint8_t octs = arp_oct < 1u ? 1u : arp_oct > 3u ? 3u : arp_oct;

    for (c = 0; c < n_chords; c++) {
        for (i = 0; i < chords[c].count; i++) {
            uint8_t n = chords[c].notes[i];
            if (!note_in_list(n, base, n_base) && n_base < CS_ARP_MAX_POOL)
                base[n_base++] = n;
        }
    }
    sort_notes(base, n_base);
    for (oct = 0; oct < octs; oct++) {
        for (i = 0; i < n_base; i++) {
            uint32_t n = (uint32_t)base[i] + 12u * oct;
            if (n <= 127u && !note_in_list((uint8_t)n, pool, n_pool) && n_pool < CS_ARP_MAX_POOL)
                pool[n_pool++] = (uint8_t)n;
        }
    }
    return n_pool;
}

void cs_arp_tick(cs_arp_t *arp, note_refs_t *nr, uint8_t ch,
                 const cs_chord_t *chords, const uint8_t *chord_pad_idx, uint8_t n_chords,
                 uint8_t vel, uint8_t bpm, uint8_t arp_div, uint8_t arp_mode,
                 uint8_t arp_oct, uint8_t arp_gate, uint32_t now_ms)
{
    uint8_t pool[CS_ARP_MAX_POOL];
    uint8_t n_pool = 0;
    uint32_t per_ms, gate_ms;
    int ext_sync, fire_now = 0;

    if (n_chords > 0)
        n_pool = build_tone_pool(chords, n_chords, arp_oct, pool);

    if (n_pool == 0) {
        if (arp->was_holding || arp->gate_open)
            cs_arp_stop(arp, nr);
        return;
    }

    ext_sync = arp->ext_clk_last_ms != 0u && (now_ms - arp->ext_clk_last_ms) < 500u;
    per_ms = step_period_ms(bpm, arp_div);
    gate_ms = (per_ms * (arp_gate < 10u ? 10u : arp_gate > 95u ? 95u : arp_gate)) / 100u;
    if (gate_ms < 10u)
        gate_ms = 10u;
    if (gate_ms + 5u > per_ms)
        gate_ms = per_ms > 5u ? per_ms - 5u : per_ms;

    if (!arp->was_holding) {
        arp->was_holding = 1;
        arp->step_idx = 0;
        arp->dir = 1;
        fire_now = 1;
    } else if (ext_sync) {
        if (arp->ext_step_due) {
            arp->ext_step_due = 0;
            fire_now = 1;
        }
    } else if ((int32_t)(now_ms - arp->next_step_ms) >= 0) {
        fire_now = 1;
    }

    if (!fire_now && arp->gate_open && (int32_t)(now_ms - arp->gate_off_ms) >= 0)
        arp_gate_close(arp, nr);

    if (!fire_now)
        return;

    /* Close previous step's notes before triggering the new step */
    arp_gate_close(arp, nr);
    arp->sounding_ch = ch;
    arp->step_start_ms = now_ms;
    arp->gate_off_ms = now_ms + gate_ms;
    arp->next_step_ms = now_ms + per_ms;
    arp->gate_open = 1;

    if ((arp_mode % NARP_MODES) == ARP_CHORD) {
        uint8_t c_idx = arp->step_idx % n_chords;
        const cs_chord_t *c = &chords[c_idx];
        uint8_t i;
        for (i = 0; i < c->count && i < CS_MAX_NOTES; i++) {
            note_refs_on(nr, ch, c->notes[i], vel);
            arp->sounding_notes[i] = c->notes[i];
        }
        arp->sounding_count = c->count > CS_MAX_NOTES ? CS_MAX_NOTES : c->count;
        arp->active_pad = chord_pad_idx ? chord_pad_idx[c_idx] : 0xFF;
        arp->step_idx = (uint8_t)((arp->step_idx + 1u) % n_chords);
        return;
    }

    {
        uint8_t note = pool[0];
        uint8_t m = arp_mode % NARP_MODES;
        if (m == ARP_UP) {
            uint8_t idx = arp->step_idx % n_pool;
            note = pool[idx];
            arp->step_idx = (uint8_t)((idx + 1u) % n_pool);
        } else if (m == ARP_DOWN) {
            uint8_t idx = arp->step_idx % n_pool;
            note = pool[n_pool - 1u - idx];
            arp->step_idx = (uint8_t)((idx + 1u) % n_pool);
        } else if (m == ARP_UP_DOWN) {
            if (n_pool == 1u) {
                note = pool[0];
                arp->step_idx = 0;
            } else {
                int32_t idx = (int32_t)arp->step_idx;
                if (idx >= (int32_t)n_pool)
                    idx = (int32_t)n_pool - 1;
                if (idx < 0)
                    idx = 0;
                note = pool[idx];
                if (arp->dir > 0 && idx + 1 >= (int32_t)n_pool)
                    arp->dir = -1;
                else if (arp->dir < 0 && idx <= 0)
                    arp->dir = 1;
                arp->step_idx = (uint8_t)(idx + arp->dir);
            }
        } else {                         /* ARP_RANDOM */
            uint32_t x = arp->rng ? arp->rng : 0x12345678u;
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            arp->rng = x;
            note = pool[x % n_pool];
        }
        note_refs_on(nr, ch, note, vel);
        arp->sounding_notes[0] = note;
        arp->sounding_count = 1;
    }
}
