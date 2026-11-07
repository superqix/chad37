/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Michael Wilson */
/* Host unit test for app/note_refs.c: reference counting across overlapping notes, chord morphing,
 * per-transport Note-Off overflow tracking and recovery, and all-notes-off cleanup. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "note_refs.h"

typedef struct {
    uint8_t st, d1, d2, to;
} ev_t;

static ev_t log_ev[256];
static uint32_t log_n;
static uint32_t accept_mask = NR_ALL_PORTS;

static uint32_t mock_send(uint32_t st, uint32_t d1, uint32_t d2, uint32_t to)
{
    uint32_t ok = to & accept_mask;
    if (ok && log_n < 256u) {
        log_ev[log_n].st = (uint8_t)st;
        log_ev[log_n].d1 = (uint8_t)d1;
        log_ev[log_n].d2 = (uint8_t)d2;
        log_ev[log_n].to = (uint8_t)ok;
        log_n++;
    }
    return ok;
}

static void reset_log(void)
{
    memset(log_ev, 0, sizeof log_ev);
    log_n = 0;
    accept_mask = NR_ALL_PORTS;
}

static void test_overlap_and_morph(void)
{
    note_refs_t nr;
    const uint8_t c_maj[3] = {60, 64, 67};
    const uint8_t c_maj7[4] = {60, 64, 67, 71};
    const uint8_t a_min[3] = {57, 60, 64};

    reset_log();
    note_refs_init(&nr, mock_send);

    /* Overlapping presses of middle C: only one Note-On and one Note-Off on the final release. */
    note_refs_on(&nr, 0, 60, 100);
    note_refs_on(&nr, 0, 60, 110);
    assert(log_n == 1 && log_ev[0].st == 0x90 && log_ev[0].d1 == 60 && log_ev[0].d2 == 100);
    assert(note_refs_active(&nr) == 1);

    note_refs_off(&nr, 0, 60);
    assert(log_n == 1 && note_refs_active(&nr) == 1);

    note_refs_off(&nr, 0, 60);
    assert(log_n == 2 && log_ev[1].st == 0x80 && log_ev[1].d1 == 60 && log_ev[1].d2 == 0);
    assert(note_refs_active(&nr) == 0);

    /* Extra release when already at 0 is a no-op. */
    note_refs_off(&nr, 0, 60);
    assert(log_n == 2);

    /* Chord morphing: C -> Cmaj7 adds only B4 (71); Cmaj7 -> Am adds A3 (57) and releases G4 (67) & B4 (71). */
    reset_log();
    note_refs_morph(&nr, 0, 0, 0, c_maj, 3, 95);
    assert(log_n == 3 && note_refs_active(&nr) == 3);

    reset_log();
    note_refs_morph(&nr, 0, c_maj, 3, c_maj7, 4, 95);
    assert(log_n == 1 && log_ev[0].st == 0x90 && log_ev[0].d1 == 71);
    assert(note_refs_active(&nr) == 4);

    reset_log();
    note_refs_morph(&nr, 0, c_maj7, 4, a_min, 3, 90);
    assert(log_n == 3);
    assert(log_ev[0].st == 0x90 && log_ev[0].d1 == 57 && log_ev[0].d2 == 90);
    assert(log_ev[1].st == 0x80 && log_ev[1].d1 == 67);
    assert(log_ev[2].st == 0x80 && log_ev[2].d1 == 71);
    assert(note_refs_active(&nr) == 3);

    reset_log();
    note_refs_all_off(&nr);
    assert(log_n == 3 && note_refs_active(&nr) == 0 && !note_refs_pending(&nr));
    printf("note_refs: overlap and chord morphing                                          ok\n");
}

static void test_transport_overflow_recovery(void)
{
    note_refs_t nr;
    reset_log();
    note_refs_init(&nr, mock_send);

    note_refs_on(&nr, 1, 64, 100);
    note_refs_on(&nr, 1, 67, 100);
    assert(note_refs_active(&nr) == 2);

    /* Stall port 1 (BLE) while ports 0 (USB) and 2 (TRS) accept Note-Offs. */
    reset_log();
    accept_mask = (1u << 0) | (1u << 2);
    note_refs_off(&nr, 1, 64);
    note_refs_off(&nr, 1, 67);
    assert(note_refs_active(&nr) == 0);
    assert(note_refs_pending(&nr) == 1);
    assert(log_n == 2 && log_ev[0].to == 0x05 && log_ev[1].to == 0x05);

    /* Flush while port 1 is still stalled keeps pending Note-Offs queued. */
    log_n = 0;
    note_refs_flush(&nr);
    assert(log_n == 0 && note_refs_pending(&nr) == 1);

    /* Unstall port 1: flush delivers the two missing Note-Offs to port 1 only. */
    accept_mask = NR_ALL_PORTS;
    note_refs_flush(&nr);
    assert(note_refs_pending(&nr) == 0);
    assert(log_n == 2);
    assert(log_ev[0].st == 0x81 && log_ev[0].d1 == 64 && log_ev[0].to == (1u << 1));
    assert(log_ev[1].st == 0x81 && log_ev[1].d1 == 67 && log_ev[1].to == (1u << 1));
    printf("note_refs: per-transport Note-Off overflow and recovery                        ok\n");
}

int main(void)
{
    test_overlap_and_morph();
    test_transport_overflow_recovery();
    return 0;
}
