/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Michael Wilson */
/* Reference-counted MIDI note ownership and per-transport Note-Off overflow recovery.
 *
 * Multiple sources (keys, overlapping chord pads, easy-chord morphs, strums, arpeggiator steps)
 * may hold the same (channel, note) simultaneously. A Note-On goes out only on the 0 -> 1
 * transition, and a Note-Off goes out only on the 1 -> 0 transition.
 *
 * Each transport (USB, BLE, TRS) has a finite output queue; if a Note-Off is rejected by one
 * transport while accepted by the others, the dropped (port, channel, note) is recorded in
 * off_pend and retried independently on subsequent flushes without stranding notes on any port. */
#pragma once
#include <stdint.h>

#define NR_NPORTS 3u
#define NR_NCHAN  16u
#define NR_NNOTES 128u
#define NR_ALL_PORTS ((1u << NR_NPORTS) - 1u)

typedef uint32_t (*note_refs_send_fn)(uint32_t st, uint32_t d1, uint32_t d2, uint32_t to);

typedef struct {
    uint8_t refs[NR_NCHAN][NR_NNOTES];
    uint64_t off_pend[NR_NPORTS][NR_NCHAN][2];
    note_refs_send_fn send;
} note_refs_t;

void note_refs_init(note_refs_t *nr, note_refs_send_fn send_fn);
void note_refs_on(note_refs_t *nr, uint8_t ch, uint8_t note, uint8_t vel);
void note_refs_off(note_refs_t *nr, uint8_t ch, uint8_t note);
void note_refs_morph(note_refs_t *nr, uint8_t ch, const uint8_t *old_notes, uint8_t n_old,
                     const uint8_t *new_notes, uint8_t n_new, uint8_t vel);
void note_refs_flush(note_refs_t *nr);
void note_refs_all_off(note_refs_t *nr);
uint32_t note_refs_active(const note_refs_t *nr);
int note_refs_pending(const note_refs_t *nr);
