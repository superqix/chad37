/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Michael Wilson */
#ifndef EASY_CHORD_H
#define EASY_CHORD_H
#include <stdint.h>
#define EASY_CHORD_MAX_TONES 5
enum easy_chord_type { EASY_CHORD_MAJOR, EASY_CHORD_MINOR, EASY_CHORD_DOM7, EASY_CHORD_MAJ7,
    EASY_CHORD_MIN7, EASY_CHORD_MAJ9, EASY_CHORD_MIN9, EASY_CHORD_ADD9, EASY_CHORD_SUS4,
    EASY_CHORD_7SUS4, EASY_CHORD_DIM7, EASY_CHORD_TYPES };
struct easy_chord {
    uint8_t root, type, tone_count, tones[EASY_CHORD_MAX_TONES];
};
/* held is a 128-bit set of MIDI notes. Fills out highest root first and returns the count,
 * stopping at capacity. */
unsigned easy_chord_resolve(const uint8_t held[16], struct easy_chord *out, unsigned capacity);
/* Fills tones in ascending order and returns the count. */
unsigned easy_chord_tones(unsigned root, unsigned type, uint8_t tones[EASY_CHORD_MAX_TONES]);
const char *easy_chord_suffix(unsigned type);
/* Writes a readout such as "C# Min7". */
void easy_chord_name(unsigned root, unsigned type, char name[12]);
#endif
