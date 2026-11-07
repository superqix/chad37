/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Michael Wilson */
/* Host unit tests for app/chord_scale.c: manual fingered chord detection, Easy Chord with scale-root
 * offset and voicing, Scale-mode keybed mapping (white keys in key, black keys muted), diatonic scale
 * pad chords, non-blocking auto-strum queue, and arpeggiator patterns + external MIDI clock sync. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "chord_scale.h"

typedef struct {
    uint8_t st, d1, d2;
} ev_t;

static ev_t log_ev[256];
static uint32_t log_n;

static uint32_t mock_send(uint32_t st, uint32_t d1, uint32_t d2, uint32_t to)
{
    (void)to;
    if (log_n < 256u) {
        log_ev[log_n].st = (uint8_t)st;
        log_ev[log_n].d1 = (uint8_t)d1;
        log_ev[log_n].d2 = (uint8_t)d2;
        log_n++;
    }
    return NR_ALL_PORTS;
}

static void hold_note(uint8_t held[16], int note)
{
    held[note >> 3] |= (uint8_t)(1u << (note & 7));
}

static void test_manual_chord_and_voicing(void)
{
    uint8_t held[16] = {0};
    cs_chord_t ch;
    char name[16];

    /* C4 - E4 - G4 -> C Maj */
    hold_note(held, 60);
    hold_note(held, 64);
    hold_note(held, 67);
    assert(cs_detect_manual_chord(held, &ch, name) == 3);
    assert(!strcmp(name, "C Maj"));
    assert(ch.count == 3 && ch.notes[0] == 60 && ch.notes[1] == 64 && ch.notes[2] == 67);

    /* Voicings on C Maj (60, 64, 67):
     *   1st Inv -> (64, 67, 72)
     *   2nd Inv -> (67, 72, 76)
     *   Spread  -> (60, 67, 76) */
    {
        uint8_t t1[3] = {60, 64, 67}, t2[3] = {60, 64, 67}, ts[3] = {60, 64, 67};
        cs_apply_voicing(t1, 3, VOICING_INV1);
        assert(t1[0] == 64 && t1[1] == 67 && t1[2] == 72);
        cs_apply_voicing(t2, 3, VOICING_INV2);
        assert(t2[0] == 67 && t2[1] == 72 && t2[2] == 76);
        cs_apply_voicing(ts, 3, VOICING_SPREAD);
        assert(ts[0] == 60 && ts[1] == 67 && ts[2] == 76);
    }

    /* Easy Chord with Scale Root offset (+2 = D) and 1st Inversion:
     * Holding C4 (60) + B3 (59, W1 below C4 -> Maj7) transposed by +2 -> D Maj7 (62, 66, 69, 73),
     * 1st Inv -> (66, 69, 73, 74). */
    memset(held, 0, sizeof held);
    hold_note(held, 60);
    hold_note(held, 59);
    assert(cs_resolve_easy_chord(held, 2, VOICING_INV1, &ch, name) == 1);
    assert(!strcmp(name, "D Maj7"));
    assert(ch.count == 4 && ch.notes[0] == 66 && ch.notes[1] == 69 && ch.notes[2] == 73 && ch.notes[3] == 74);
    printf("chord_scale: manual chord detection, Easy Chord offset, and voicings         ok\n");
}

static void test_scale_keys_and_diatonic_pads(void)
{
    cs_chord_t ch;
    char name[16];

    /* Scale-only keybed in D Natural Minor (root = 2, scale = SCALE_NAT_MINOR, octave = 0):
     * Key 0 (C3 white) -> D3 (50), Key 1 (C#3 black) -> -1 (muted), Key 2 (D3 white) -> E3 (52),
     * Key 4 (E3 white) -> F3 (53), Key 5 (F3 white) -> G3 (55). */
    assert(cs_scale_key_note(0, 0, 2, SCALE_NAT_MINOR) == 50);
    assert(cs_scale_key_note(1, 0, 2, SCALE_NAT_MINOR) == -1);
    assert(cs_scale_key_note(2, 0, 2, SCALE_NAT_MINOR) == 52);
    assert(cs_scale_key_note(3, 0, 2, SCALE_NAT_MINOR) == -1);
    assert(cs_scale_key_note(4, 0, 2, SCALE_NAT_MINOR) == 53);
    assert(cs_scale_key_note(5, 0, 2, SCALE_NAT_MINOR) == 55);

    /* Diatonic scale pads in C Major (root = 0, SCALE_MAJOR, VOICING_ROOT, FLAVOR_7THS):
     *   Bottom row P8 (degree I triad)  -> C Maj (48, 52, 55)
     *   Bottom row P9 (degree II triad) -> D Min (50, 53, 57)
     *   Top row P0 (degree I 7th)       -> C Maj7 (48, 52, 55, 59)
     *   Top row P4 (degree V 7th)       -> G 7 (55, 59, 62, 65) */
    cs_scale_pad_chord(8, 0, 0, SCALE_MAJOR, VOICING_ROOT, FLAVOR_7THS, &ch, name);
    assert(!strcmp(name, "C Maj") && ch.count == 3 && ch.notes[0] == 48 && ch.notes[1] == 52 && ch.notes[2] == 55);

    cs_scale_pad_chord(9, 0, 0, SCALE_MAJOR, VOICING_ROOT, FLAVOR_7THS, &ch, name);
    assert(!strcmp(name, "D Min") && ch.count == 3 && ch.notes[0] == 50 && ch.notes[1] == 53 && ch.notes[2] == 57);

    cs_scale_pad_chord(0, 0, 0, SCALE_MAJOR, VOICING_ROOT, FLAVOR_7THS, &ch, name);
    assert(!strcmp(name, "C Maj7") && ch.count == 4 && ch.notes[3] == 59);

    cs_scale_pad_chord(4, 0, 0, SCALE_MAJOR, VOICING_ROOT, FLAVOR_7THS, &ch, name);
    assert(!strcmp(name, "G 7") && ch.count == 4 && ch.notes[0] == 55 && ch.notes[3] == 65);
    printf("chord_scale: scale white-key mapping, black-key mute, and diatonic pads      ok\n");
}

static void test_auto_strum(void)
{
    note_refs_t nr;
    cs_strum_t st;
    const uint8_t notes[3] = {60, 64, 67};

    note_refs_init(&nr, mock_send);
    cs_strum_init(&st);
    log_n = 0;

    /* Trigger 3-note chord with 30 ms Up strum at t = 100 ms:
     *   t = 100: 60 fires immediately
     *   t = 130: 64 fires
     *   t = 140: release before 67 (t = 160) fires -> 67 is cancelled and only 60 & 64 send Note-Off! */
    cs_strum_trigger(&st, &nr, 0, 0, notes, 3, 100, 30, STRUM_UP, 100);
    assert(log_n == 1 && log_ev[0].st == 0x90 && log_ev[0].d1 == 60);
    assert(cs_strum_pending(&st) == 1);

    cs_strum_tick(&st, &nr, 129);
    assert(log_n == 1);

    cs_strum_tick(&st, &nr, 130);
    assert(log_n == 2 && log_ev[1].st == 0x90 && log_ev[1].d1 == 64);

    /* Early release at t = 140 ms cancels the scheduled 67 at t = 160 ms */
    cs_strum_release_src(&st, &nr, 0);
    assert(!cs_strum_pending(&st));
    assert(note_refs_active(&nr) == 0);
    assert(log_n == 4 && log_ev[2].st == 0x80 && log_ev[3].st == 0x80);

    cs_strum_tick(&st, &nr, 200);
    assert(log_n == 4 && note_refs_active(&nr) == 0);
    printf("chord_scale: non-blocking auto-strum and early release cancellation          ok\n");
}

static void test_arpeggiator(void)
{
    note_refs_t nr;
    cs_arp_t arp;
    cs_chord_t chords[2];
    uint8_t pads[2] = {0, 1};

    memset(chords, 0, sizeof chords);
    chords[0].count = 3;
    chords[0].notes[0] = 60;
    chords[0].notes[1] = 64;
    chords[0].notes[2] = 67;
    chords[1].count = 3;
    chords[1].notes[0] = 62;
    chords[1].notes[1] = 65;
    chords[1].notes[2] = 69;

    note_refs_init(&nr, mock_send);
    cs_arp_init(&arp);
    log_n = 0;

    /* 120 BPM, 1/8 division -> 250 ms step period, 50% gate -> 125 ms gate off */
    cs_arp_tick(&arp, &nr, 0, &chords[0], &pads[0], 1, 100, 120, ARP_DIV_1_8, ARP_UP, 1, 50, 1000);
    assert(log_n == 1 && log_ev[0].st == 0x90 && log_ev[0].d1 == 60);

    cs_arp_tick(&arp, &nr, 0, &chords[0], &pads[0], 1, 100, 120, ARP_DIV_1_8, ARP_UP, 1, 50, 1125);
    assert(log_n == 2 && log_ev[1].st == 0x80 && log_ev[1].d1 == 60);

    cs_arp_tick(&arp, &nr, 0, &chords[0], &pads[0], 1, 100, 120, ARP_DIV_1_8, ARP_UP, 1, 50, 1250);
    assert(log_n == 3 && log_ev[2].st == 0x90 && log_ev[2].d1 == 64);

    cs_arp_tick(&arp, &nr, 0, &chords[0], &pads[0], 1, 100, 120, ARP_DIV_1_8, ARP_UP, 1, 50, 1500);
    assert(log_n == 5 && log_ev[4].st == 0x90 && log_ev[4].d1 == 67);

    /* ARP_CHORD progression mode across two held pads (C Maj -> D Min) */
    cs_arp_stop(&arp, &nr);
    log_n = 0;
    cs_arp_tick(&arp, &nr, 0, chords, pads, 2, 100, 120, ARP_DIV_1_8, ARP_CHORD, 1, 50, 2000);
    assert(log_n == 3 && arp.active_pad == 0 && log_ev[0].d1 == 60 && log_ev[1].d1 == 64 && log_ev[2].d1 == 67);

    cs_arp_tick(&arp, &nr, 0, chords, pads, 2, 100, 120, ARP_DIV_1_8, ARP_CHORD, 1, 50, 2250);
    assert(arp.active_pad == 1 && note_refs_active(&nr) == 3);

    cs_arp_stop(&arp, &nr);
    assert(note_refs_active(&nr) == 0);
    printf("chord_scale: arpeggiator Up and full-chord progression modes                 ok\n");
}

int main(void)
{
    test_manual_chord_and_voicing();
    test_scale_keys_and_diatonic_pads();
    test_auto_strum();
    test_arpeggiator();
    return 0;
}
