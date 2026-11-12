/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Michael Wilson */
/* Included by app.c: code-drawn keyboards, using the existing strip canvas and fonts.
 * Lit keys are sounding MIDI notes, including voicings, strum and arp. When REC is armed they preview the
 * assignment instead. The Easy Chord diagram shows available modifier keys to the left of an example C root. */

#define CH_UI_ACCENT APP_SPLASH_RGB
#define CH_UI_MOD RGB(112, 205, 202)
#define CH_UI_PAPER RGB(224, 226, 228)
#define CH_UI_MUTED RGB(150, 155, 164)
#define CH_UI_LINE RGB(38, 42, 48)
#define CH_UI_RED RGB(248, 92, 84)

/* The no-radio firmware is freestanding: do not require the Bluetooth shim's strlen/strcmp. */
static unsigned chord_ui_length(const char *s)
{
    unsigned n = 0;
    while (s[n]) n++;
    return n;
}

static int chord_ui_equal(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static void chord_ui_center(int y, const felucca_font_t *font, const char *text, uint16_t color)
{
    cv_text((240 - text_w(font, text)) / 2, y, font, text, color);
}

static int chord_ui_held(const uint8_t notes[16], int note)
{
    return note >= 0 && note < 128 && (notes[note >> 3] >> (note & 7) & 1u);
}

static unsigned chord_ui_notes(const preset_t *pr, uint8_t notes[16])
{
    unsigned n, count = 0;
    memset(notes, 0, 16);
    for (n = 0; n < 128u; n++) {
        if (ui.nr.refs[pr->key_ch & 15u][n]) {
            notes[n >> 3] |= (uint8_t)(1u << (n & 7));
            count++;
        }
    }
    return count;
}

/* A proportional piano, with black keys over white ones. The range is inclusive, starts on a white key,
 * and may grow beyond two octaves for spread/2X voicings. Labelled Cs make octave placement unambiguous. */
static void chord_ui_keyboard(int x, int y, int w, int h, int first, int last,
                               const uint8_t notes[16], int tutorial)
{
    int note, whites = 0, index = 0;
    for (note = first; note <= last; note++)
        if (!ec_is_black(note))
            whites++;
    for (note = first; note <= last; note++) {
        int left, right, on;
        uint16_t color;
        if (ec_is_black(note))
            continue;
        left = x + index * w / whites;
        right = x + (++index) * w / whites;
        on = chord_ui_held(notes, note);
        color = on ? CH_UI_ACCENT : CH_UI_PAPER;
        if (tutorial && (note == 57 || note == 59))
            color = RGB(174, 211, 210);
        cv_rect(left, y, right - left - 1, h, color);
        if (on)
            cv_rect(left + 2, y + h - 7, right - left - 5, 3, C_BLACK);
        if (tutorial && (note == 57 || note == 59 || note == 60)) {
            const char *label = note == 57 ? "7" : note == 59 ? "MAJ7" : "C";
            cv_text((left + right - text_w(&FONT_T, label)) / 2, y + h - 20, &FONT_T, label, C_BLACK);
        } else if (!tutorial && note % 12 == 0) {
            char label[8];
            cs_note_name((uint32_t)note, label);
            if (text_w(&FONT_T, label) < right - left)
                cv_text((left + right - text_w(&FONT_T, label)) / 2, y + h - 21, &FONT_T, label, C_BLACK);
        }
    }
    index = 0;
    for (note = first; note <= last; note++) {
        int center, bw, left, bh = h * 3 / 5;
        uint16_t color;
        if (!ec_is_black(note)) {
            index++;
            continue;
        }
        center = x + index * w / whites;
        bw = w * 2 / (whites * 3);
        left = center - bw / 2;
        color = chord_ui_held(notes, note) ? CH_UI_ACCENT : RGB(18, 21, 27);
        if (tutorial && (note == 56 || note == 58))
            color = RGB(53, 96, 100);
        cv_rect(left - 1, y, bw + 2, bh + 1, C_BLACK);
        cv_rect(left, y, bw, bh, color);
        if (tutorial && (note == 56 || note == 58)) {
            const char *label = note == 56 ? "SUS4" : "MIN";
            cv_text(center - text_w(&FONT_T, label) / 2, y + 14, &FONT_T, label, C_WHITE);
        } else if (chord_ui_held(notes, note)) {
            cv_rect(left + 2, y + bh - 7, bw - 4, 3, C_BLACK);
        }
    }
}

static void chord_ui_header(const preset_t *pr, const char *mode)
{
    char status[20];
    cv_text(12, 10, &FONT_B, mode, CH_UI_ACCENT);
    copy_str(status, sizeof status, "P");
    int_text((int32_t)st.active_preset + 1, status + 1);
    copy_str(status + chord_ui_length(status), sizeof status - chord_ui_length(status), "   CH ");
    int_text((int32_t)pr->key_ch + 1, status + chord_ui_length(status));
    cv_text(218 - text_w(&FONT_XS, status), 12, &FONT_XS, status, CH_UI_MUTED);
    cv_rect(226, 16, 4, 4, ui.ble_conn ? CH_UI_MOD : CH_UI_LINE);
    cv_line(12, 31, 227, 31, CH_UI_LINE);
}

static void chord_ui_easy_help(const preset_t *pr)
{
    uint8_t example[16] = {0};
    example[60 >> 3] = 1u << (60 & 7);
    chord_ui_header(pr, "EASY CHORD");
    chord_ui_center(42, &FONT_M, "C = C MAJOR", C_WHITE);

    cv_text(39, 77, &FONT_T, "MODIFIERS", CH_UI_MOD);
    cv_line(58, 91, 121, 91, CH_UI_MOD);
    cv_line(58, 91, 58, 96, CH_UI_MOD);
    cv_line(121, 91, 121, 96, CH_UI_MOD);
    cv_text(140, 77, &FONT_T, "ROOT", CH_UI_ACCENT);
    cv_line(151, 91, 151, 96, CH_UI_ACCENT);

    /* F3 G3 A3 B3 C4 D4 E4: W2=A3, W1=B3, B2=G#3, B1=A#3, with C4 as the root. */
    chord_ui_keyboard(12, 99, 216, 69, 53, 64, example, 1);
    chord_ui_center(179, &FONT_XS, "Hold root + add a key on its left", CH_UI_MUTED);
    chord_ui_center(204, &FONT_T, "SEQ REC > PAD: ASSIGN YOUR CHORD", CH_UI_MUTED);
    cv_line(12, 221, 227, 221, CH_UI_LINE);
    chord_ui_center(226, &FONT_T, "PLAY ANY KEY / PAD TO START", CH_UI_ACCENT);
}

static void chord_ui_performance(const preset_t *pr)
{
    uint8_t sounding[16], shown[16];
    unsigned active = chord_ui_notes(pr, sounding);
    char name[16], notes_line[80];
    int first = 48 + 12 * st.octave, last, n, lo = 128, hi = -1;
    int preview = ui.rec_arm && ui.latched_chord.count;

    memcpy(shown, sounding, sizeof shown);
    copy_str(name, sizeof name, ui.chord_name);
    if (preview) {
        memset(shown, 0, sizeof shown);
        for (n = 0; n < ui.latched_chord.count; n++) {
            unsigned note = ui.latched_chord.notes[n];
            shown[note >> 3] |= (uint8_t)(1u << (note & 7));
        }
        cs_chord_format_name(&ui.latched_chord, name);
    } else if (active && !ui.scale_on && ui.keys_on && !ui.pads_on && !ui.arp_on) {
        cs_chord_t detected;
        cs_detect_manual_chord(sounding, &detected, name);
    }

    notes_line[0] = 0;
    for (n = 0; n < 128; n++) {
        if (chord_ui_held(shown, n)) {
            char note[8];
            unsigned used = chord_ui_length(notes_line);
            if (lo > n) lo = n;
            if (hi < n) hi = n;
            cs_note_name((uint32_t)n, note);
            if (used + chord_ui_length(note) + 3 < sizeof notes_line) {
                copy_str(notes_line + used, sizeof notes_line - (uint32_t)used, used ? "  " : "");
                used = chord_ui_length(notes_line);
                copy_str(notes_line + used, sizeof notes_line - (uint32_t)used, note);
            }
        }
    }
    if (hi >= 0)
        first = lo / 12 * 12;
    last = first + 24;
    while (last < hi)
        last += 12;
    if (last > 127)
        last = 127;
    if (hi >= 0) {
        ui.chord_view_first = (uint8_t)first;
        ui.chord_view_last = (uint8_t)last;
    } else if (name[0] && ui.chord_view_last) {
        /* Keep the last keys/pad's range after release, without leaving any keys lit. */
        first = ui.chord_view_first;
        last = ui.chord_view_last;
    }

    chord_ui_header(pr, ui.scale_on ? "EASY CHORD" : "CHORD");
    if (name[0]) {
        const felucca_font_t *font = text_w(&FONT_L, name) <= 216 ? &FONT_L : &FONT_M;
        chord_ui_center(43, font, name, C_WHITE);
    } else {
        chord_ui_center(45, &FONT_M, "PLAY A CHORD", C_WHITE);
    }
    if (!notes_line[0])
        copy_str(notes_line, sizeof notes_line, name[0] ? "LAST CHORD" : "Keys or assigned chord pads");
    if (text_w(&FONT_T, notes_line) > 216) {
        int_text((int32_t)active, notes_line);
        n = (int)chord_ui_length(notes_line);
        copy_str(notes_line + n, sizeof notes_line - (uint32_t)n, " NOTES");
    }
    chord_ui_center(81, text_w(&FONT_XS, notes_line) <= 216 ? &FONT_XS : &FONT_T,
                    notes_line, preview ? CH_UI_RED : CH_UI_MUTED);
    chord_ui_keyboard(12, 105, 216, 79, first, last, shown, 0);

    if (ui.chord_help == CHORD_HELP_ASSIGN) {
        chord_ui_center(202, &FONT_B, "SEQ REC  >  TAP A PAD", CH_UI_ACCENT);
        chord_ui_center(222, &FONT_XS, "Assign the active chord", CH_UI_MUTED);
    } else if (ui.rec_arm) {
        cv_rect(12, 201, 216, 19, RGB(68, 27, 29));
        chord_ui_center(203, &FONT_B, "TAP A PAD TO ASSIGN", CH_UI_RED);
        chord_ui_center(225, &FONT_T, "SEQ REC: CANCEL", CH_UI_MUTED);
    } else {
        unsigned p;
        cv_text(12, 201, &FONT_T, "PADS", CH_UI_MUTED);
        for (p = 0; p < SMK_NPADS; p++) {
            uint16_t color = ui.pads_on >> p & 1u ? CH_UI_ACCENT :
                pr->chords[p].count ? CH_UI_MOD : CH_UI_LINE;
            cv_rect(48 + (int)p * 11, 203, 8, 8, color);
        }
        if (plat_ms() < ui.msg_until && !chord_ui_equal(ui.msg[0], "CHORD") && !chord_ui_equal(ui.msg[0], "SCALE ON")) {
            char message[50];
            copy_str(message, sizeof message, ui.msg[0]);
            n = (int)chord_ui_length(message);
            copy_str(message + n, sizeof message - (uint32_t)n, "  ");
            n = (int)chord_ui_length(message);
            copy_str(message + n, sizeof message - (uint32_t)n, ui.msg[1]);
            chord_ui_center(222, &FONT_T, message, CH_UI_ACCENT);
        } else if (ui.scale_on && ui.page == PG_SCALE) {
            /* Scale-page knob roles stay active, just as before the performance layout. */
            const char *values[4] = {CS_ROOT_NAMES[st.scale_root], CS_SCALE_NAMES[st.scale_type],
                                     CS_VOICING_NAMES[st.voicing], CS_FLAVOR_NAMES[st.pad_flavor]};
            for (n = 0; n < 4; n++) {
                char label[20] = "K";
                int_text(n + 1, label + 1);
                copy_str(label + 2, sizeof label - 2, " ");
                copy_str(label + 3, sizeof label - 3, values[n]);
                cv_text(12 + n * 55, 224, &FONT_T, label, CH_UI_MUTED);
            }
        } else {
            cv_text(12, 224, &FONT_T, "SEQ REC: ASSIGN", CH_UI_MUTED);
            cv_text(154, 224, &FONT_T, latch_active() ? "LATCH ON" : "SCALE: EASY", CH_UI_MUTED);
        }
    }
}

static void draw_chord_view(const preset_t *pr)
{
    int y;
    /* Two 120-row transfers fit the existing canvas; no full-screen framebuffer or added pool RAM. */
    for (y = 0; y < 240; y += 120) {
        cv_begin(240, 120, C_BLACK);
        cv_oy = -y;
        if (ui.chord_help == CHORD_HELP_EASY)
            chord_ui_easy_help(pr);
        else
            chord_ui_performance(pr);
        cv_blit(0, (uint32_t)y);
    }
}
