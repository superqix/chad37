/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Michael Wilson */
/* Pure chord detection, scale mapping, diatonic chord generation, voicing/inversion,
 * non-blocking auto-strum queue, and arpeggiator step logic for Chad37 (M4).
 * Hardware-independent so tests/run_host_tests.sh can verify all M4 music logic on the host. */
#pragma once
#include <stdint.h>
#include "easy_chord.h"
#include "note_refs.h"

#define CS_CHORD_NOTES 6u                /* a chord's own tones: what detection finds and a pad stores */
#define CS_MAX_NOTES   12u               /* a voiced chord as played: VOICING_DOUBLE adds an octave below */
#define CS_NPADS       16u

enum {
    SCALE_MAJOR,
    SCALE_NAT_MINOR,
    SCALE_HARM_MINOR,
    SCALE_DORIAN,
    SCALE_PHRYGIAN,
    SCALE_LYDIAN,
    SCALE_MIXOLYDIAN,
    SCALE_PENT_MAJOR,
    SCALE_PENT_MINOR,
    NSCALES
};

enum {
    VOICING_ROOT,
    VOICING_INV1,
    VOICING_INV2,
    VOICING_SPREAD,
    VOICING_DOUBLE,                      /* "2X": the chord, and the same chord an octave below */
    NVOICINGS
};

enum {
    FLAVOR_TRIADS,
    FLAVOR_7THS,
    FLAVOR_9THS,
    NFLAVORS
};

enum {
    STRUM_UP,
    STRUM_DOWN,
    STRUM_ALT,
    NSTRUM_DIRS
};

enum {
    ARP_UP,
    ARP_DOWN,
    ARP_UP_DOWN,
    ARP_RANDOM,
    ARP_CHORD,
    NARP_MODES
};

enum {
    ARP_DIV_1_4,
    ARP_DIV_1_8,
    ARP_DIV_1_16,
    ARP_DIV_1_8T,
    NARP_DIVS
};

typedef struct {
    uint8_t count;                       /* 0 = unassigned, 1..CS_MAX_NOTES = notes[] count */
    uint8_t root;                        /* MIDI root note or pitch class */
    uint8_t type;                        /* easy_chord_type, or 0xFE custom, 0xFF single note */
    uint8_t notes[CS_MAX_NOTES];
} cs_chord_t;

/* A chord as a pad stores it (presets): unvoiced, at most CS_CHORD_NOTES tones. Its layout is the store's */
typedef struct {
    uint8_t count;                       /* 0 = unassigned, 1..CS_CHORD_NOTES */
    uint8_t root;
    uint8_t type;
    uint8_t notes[CS_CHORD_NOTES];
} cs_pad_chord_t;
_Static_assert(sizeof(cs_pad_chord_t) == 9u, "the preset store's pad chord layout");

void cs_pad_chord_store(cs_pad_chord_t *dst, const cs_chord_t *src);   /* the first CS_CHORD_NOTES tones */
void cs_pad_chord_load(cs_chord_t *dst, const cs_pad_chord_t *src);

/* Names for UI display */
extern const char *const CS_ROOT_NAMES[12];
extern const char *const CS_SCALE_NAMES[NSCALES];
extern const char *const CS_VOICING_NAMES[NVOICINGS];
extern const char *const CS_FLAVOR_NAMES[NFLAVORS];
extern const char *const CS_STRUM_DIR_NAMES[NSTRUM_DIRS];
extern const char *const CS_ARP_MODE_NAMES[NARP_MODES];
extern const char *const CS_ARP_DIV_NAMES[NARP_DIVS];

/* Format a single MIDI note (0..127) as e.g. "C4" or "F#3". */
void cs_note_name(uint32_t note, char *buf);

/* Detect a chord from a set of held MIDI notes (128-bit mask: bit n in held[n >> 3]).
 * Stores up to 6 ascending notes into out->notes, identifies root/type, and formats name (up to 16 chars).
 * Returns the number of held notes (0 if none). */
uint8_t cs_detect_manual_chord(const uint8_t held[16], cs_chord_t *out, char name[16]);

/* Format the display name of any cs_chord_t into name[16]. */
void cs_chord_format_name(const cs_chord_t *ch, char name[16]);

/* Apply voicing/inversion (VOICING_*) in-place to tones[0..count-1] and sort ascending; tones holds CS_MAX_NOTES.
 * Returns the new count (VOICING_DOUBLE adds each tone an octave below, skipping any below 0 or already there). */
uint8_t cs_apply_voicing(uint8_t *tones, uint8_t count, uint8_t voicing);

/* Resolve Easy Chord from raw held keybed notes (128-bit mask), transpose by scale_root (0..11),
 * and apply voicing (VOICING_*). Fills *out and writes chord name to name[16]. Returns 1 if a chord is held. */
int cs_resolve_easy_chord(const uint8_t raw_held[16], uint8_t scale_root, uint8_t voicing,
                          cs_chord_t *out, char name[16]);

/* Map a physical keybed key (0..36, where 0 is C3) in Scale-only mode (CHORD off, SCALE on).
 * White keys map to sequential scale degrees in key; black keys return -1 (muted). */
int cs_scale_key_note(uint32_t key_idx, int8_t octave, uint8_t scale_root, uint8_t scale_type);

/* Generate the diatonic scale chord for pad p (0..7 = top row degrees I..VII, I';
 * 8..15 = bottom row degrees I..VII, I' triads), shaped by scale_root, scale_type, voicing, and flavor. */
void cs_scale_pad_chord(uint32_t pad, int8_t octave, uint8_t scale_root, uint8_t scale_type,
                        uint8_t voicing, uint8_t flavor, cs_chord_t *out, char name[16]);

/* ----------------------------------------------------------- Auto-Strum --- */
#define CS_STRUM_SRC_KEYS 16u
#define CS_STRUM_NSRC     17u            /* 0..15 = pads P0..P15, 16 = keybed chord */
#define CS_STRUM_QMAX     64u            /* room for doubled (2X) chords on several sources */

typedef struct {
    uint8_t active;
    uint8_t src;
    uint8_t ch;
    uint8_t note;
    uint8_t vel;
    uint32_t fire_ms;
} cs_strum_ev_t;

typedef struct {
    uint8_t fired_notes[CS_STRUM_NSRC][CS_MAX_NOTES];
    uint8_t fired_count[CS_STRUM_NSRC];
    uint8_t fired_ch[CS_STRUM_NSRC];
    cs_strum_ev_t q[CS_STRUM_QMAX];
    uint8_t alt_flip;
} cs_strum_t;

void cs_strum_init(cs_strum_t *st);
void cs_strum_trigger(cs_strum_t *st, note_refs_t *nr, uint8_t src, uint8_t ch,
                      const uint8_t *notes, uint8_t count, uint8_t vel,
                      uint8_t strum_ms, uint8_t strum_dir, uint32_t now_ms);
void cs_strum_release_src(cs_strum_t *st, note_refs_t *nr, uint8_t src);
void cs_strum_release_all(cs_strum_t *st, note_refs_t *nr);
void cs_strum_tick(cs_strum_t *st, note_refs_t *nr, uint32_t now_ms);
int cs_strum_pending(const cs_strum_t *st);

/* ---------------------------------------------------------- Arpeggiator --- */
#define CS_ARP_MAX_POOL   48u
#define CS_ARP_MAX_CHORDS 17u

typedef struct {
    uint8_t sounding_notes[CS_MAX_NOTES];
    uint8_t sounding_count;
    uint8_t sounding_ch;
    uint8_t active_pad;                  /* 0..15 while a pad chord is sounding in ARP_CHORD, else 0xFF */
    uint8_t step_idx;
    int8_t dir;                          /* +1 or -1 for ARP_UP_DOWN */
    uint8_t was_holding;
    uint8_t gate_open;
    uint32_t step_start_ms;
    uint32_t gate_off_ms;
    uint32_t next_step_ms;
    uint32_t rng;
    /* External MIDI clock (0xF8) sync state */
    uint32_t ext_clk_last_ms;
    uint8_t ext_clk_ticks;
    uint8_t ext_step_due;
} cs_arp_t;

void cs_arp_init(cs_arp_t *arp);
void cs_arp_stop(cs_arp_t *arp, note_refs_t *nr);
void cs_arp_midi_clock(cs_arp_t *arp, uint8_t status_byte, uint8_t arp_div, uint32_t now_ms);
void cs_arp_tick(cs_arp_t *arp, note_refs_t *nr, uint8_t ch,
                 const cs_chord_t *chords, const uint8_t *chord_pad_idx, uint8_t n_chords,
                 uint8_t vel, uint8_t bpm, uint8_t arp_div, uint8_t arp_mode,
                 uint8_t arp_oct, uint8_t arp_gate, uint32_t now_ms);
