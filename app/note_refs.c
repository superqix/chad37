/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Michael Wilson */
#include "note_refs.h"

void note_refs_init(note_refs_t *nr, note_refs_send_fn send_fn)
{
    uint32_t p, c, n;
    for (c = 0; c < NR_NCHAN; c++)
        for (n = 0; n < NR_NNOTES; n++)
            nr->refs[c][n] = 0;
    for (p = 0; p < NR_NPORTS; p++)
        for (c = 0; c < NR_NCHAN; c++) {
            nr->off_pend[p][c][0] = 0;
            nr->off_pend[p][c][1] = 0;
        }
    nr->send = send_fn;
}

void note_refs_on(note_refs_t *nr, uint8_t ch, uint8_t note, uint8_t vel)
{
    uint32_t c = ch & 15u, n = note & 127u, v = vel < 1u ? 1u : vel > 127u ? 127u : vel;
    if (nr->refs[c][n] == 0u) {
        uint32_t ok = nr->send ? nr->send(0x90u | c, n, v, NR_ALL_PORTS) : NR_ALL_PORTS, p;
        nr->refs[c][n] = 1u;
        for (p = 0; p < NR_NPORTS; p++)
            if (ok >> p & 1u)
                nr->off_pend[p][c][n >> 6] &= ~((uint64_t)1u << (n & 63u));
    } else if (nr->refs[c][n] < 255u) {
        nr->refs[c][n]++;
    }
}

void note_refs_off(note_refs_t *nr, uint8_t ch, uint8_t note)
{
    uint32_t c = ch & 15u, n = note & 127u;
    if (nr->refs[c][n] == 0u)
        return;
    nr->refs[c][n]--;
    if (nr->refs[c][n] == 0u) {
        uint32_t ok = nr->send ? nr->send(0x80u | c, n, 0u, NR_ALL_PORTS) : NR_ALL_PORTS;
        uint32_t miss = NR_ALL_PORTS & ~ok, p;
        for (p = 0; p < NR_NPORTS; p++)
            if (miss >> p & 1u)
                nr->off_pend[p][c][n >> 6] |= (uint64_t)1u << (n & 63u);
    }
}

void note_refs_morph(note_refs_t *nr, uint8_t ch, const uint8_t *old_notes, uint8_t n_old,
                     const uint8_t *new_notes, uint8_t n_new, uint8_t vel)
{
    uint8_t i;
    for (i = 0; i < n_new; i++)
        note_refs_on(nr, ch, new_notes[i], vel);
    for (i = 0; i < n_old; i++)
        note_refs_off(nr, ch, old_notes[i]);
}

void note_refs_flush(note_refs_t *nr)
{
    uint32_t p, c, w, b;
    for (p = 0; p < NR_NPORTS; p++) {
        uint32_t to = 1u << p;
        for (c = 0; c < NR_NCHAN; c++) {
            for (w = 0; w < 2u; w++) {
                uint64_t m = nr->off_pend[p][c][w];
                for (b = 0; m && b < 64u; b++) {
                    uint32_t n;
                    if (!(m >> b & 1u))
                        continue;
                    n = (w << 6) + b;
                    if (nr->refs[c][n] == 0u && nr->send && !nr->send(0x80u | c, n, 0u, to))
                        goto next_port;
                    nr->off_pend[p][c][w] &= ~((uint64_t)1u << b);
                    m &= ~((uint64_t)1u << b);
                }
            }
        }
    next_port:;
    }
}

void note_refs_all_off(note_refs_t *nr)
{
    uint32_t c, n;
    note_refs_flush(nr);
    for (c = 0; c < NR_NCHAN; c++)
        for (n = 0; n < NR_NNOTES; n++)
            if (nr->refs[c][n] > 0u) {
                nr->refs[c][n] = 1u;
                note_refs_off(nr, (uint8_t)c, (uint8_t)n);
            }
}

uint32_t note_refs_active(const note_refs_t *nr)
{
    uint32_t c, n, cnt = 0;
    for (c = 0; c < NR_NCHAN; c++)
        for (n = 0; n < NR_NNOTES; n++)
            if (nr->refs[c][n] > 0u)
                cnt++;
    return cnt;
}

int note_refs_pending(const note_refs_t *nr)
{
    uint32_t p, c;
    for (p = 0; p < NR_NPORTS; p++)
        for (c = 0; c < NR_NCHAN; c++)
            if (nr->off_pend[p][c][0] | nr->off_pend[p][c][1])
                return 1;
    return 0;
}
