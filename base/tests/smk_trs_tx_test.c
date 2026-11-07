/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Michael Wilson */
/* Host unit test for the M-Vave SMK-37 TRS MIDI OUT driver (boards/smk37/fm1_trs_tx.h):
 *   cc -O2 -w -DBOARD_SMK37=1 -Ibsp/chip -Ibsp/boards/smk37 -o smk_trs_tx_test tests/smk_trs_tx_test.c */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SMK_TRS_TX_HOST_TEST 1
static uint8_t wire[1024];
static uint32_t wire_n;
static int hw_done_flag;
static int smk_trs_hw_done(void) { return hw_done_flag; }
static void smk_trs_hw_put(uint8_t b)
{
    hw_done_flag = 0;
    if (wire_n < sizeof wire)
        wire[wire_n++] = b;
}

#include "fm1_trs_tx.h"

static int fails;
static void check(const char *what, int ok)
{
    printf("smk trs tx: %-72s %s\n", what, ok ? "ok" : "FAIL");
    if (!ok)
        fails++;
}

static void reset_tx(void)
{
    memset(&smk_trs, 0, sizeof smk_trs);
    wire_n = 0;
    hw_done_flag = 0;
}

/* Simulate n 100 us TIMER5 ticks; at 31,250 baud (320 us/byte), TPND rises on the 4th tick after each byte starts. */
static uint32_t run_ticks_until_idle(uint32_t max_ticks)
{
    uint32_t t, sub = 0;
    for (t = 0; t < max_ticks && smk_trs_tx_pending(); t++) {
        if (smk_trs.busy && ++sub >= 4u) {
            hw_done_flag = 1;
            sub = 0;
        }
        smk_trs_tx_tick();
    }
    return t;
}

#define PK3(st, d1, d2) ((uint32_t)((st) >> 4) | (uint32_t)(st) << 8 | (uint32_t)(d1) << 16 | (uint32_t)(d2) << 24)
#define PK2(st, d1)     ((uint32_t)((st) >> 4) | (uint32_t)(st) << 8 | (uint32_t)(d1) << 16)
#define RT1(st)         (0x0Fu | (uint32_t)(st) << 8)

int main(void)
{
    /* 1. Packet framing by CIN (1, 2, and 3 byte messages; reserved CINs 0 and 1 ignored) */
    reset_tx();
    {
        static const uint8_t want[] = {
            0x90, 60, 100,   /* Note On */
            0xC0, 42,        /* Program Change (2 bytes) */
            0xD0, 88,        /* Channel Pressure (2 bytes) */
            0xF8,            /* Timing Clock (1 byte) */
            0xE0, 0x00, 0x40 /* Pitch Bend (3 bytes) */
        };
        int r0 = smk_trs_midi_send(0x00u);
        int r1 = smk_trs_midi_send(0x01u);
        int ok = (r0 == 0 && r1 == 0 && wire_n == 0 && !smk_trs_tx_pending());
        ok &= smk_trs_midi_send(PK3(0x90, 60, 100)) == 1;
        ok &= smk_trs_midi_send(PK2(0xC0, 42)) == 1;
        ok &= smk_trs_midi_send(PK2(0xD0, 88)) == 1;
        ok &= smk_trs_midi_send(RT1(0xF8)) == 1;
        ok &= smk_trs_midi_send(PK3(0xE0, 0x00, 0x40)) == 1;
        run_ticks_until_idle(200);
        ok &= wire_n == sizeof want && memcmp(wire, want, sizeof want) == 0 && !smk_trs_tx_pending();
        check("1/2/3-byte USB-MIDI CIN serialization and reserved CIN ignore", ok);
    }

    /* 2. 5-note chord serialization timing: 15 bytes drained at 100 us ticks in <= 6.0 ms (60 ticks) */
    reset_tx();
    {
        static const uint8_t notes[5] = {48, 60, 64, 67, 71};
        uint32_t i, ticks;
        int ok = 1;
        for (i = 0; i < 5u; i++)
            ok &= smk_trs_midi_send(PK3(0x90, notes[i], 96)) == 1;
        /* First byte is already in the UART shift register before the next TIMER5 tick */
        ok &= wire_n == 1u && wire[0] == 0x90u && smk_trs.busy == 1u;
        ticks = run_ticks_until_idle(200);
        ok &= wire_n == 15u && ticks == 60u && !smk_trs_tx_pending();
        for (i = 0; i < 5u; i++)
            ok &= wire[3u * i] == 0x90u && wire[3u * i + 1u] == notes[i] && wire[3u * i + 2u] == 96u;
        check("5-note chord starts immediately and drains in 6.0 ms (60 x 100 us ticks)", ok);
    }

    /* 3. Ring capacity & atomic overflow (never writes a partial 3-byte message) */
    reset_tx();
    {
        /* Enqueue 1 byte first (goes immediately into the shift register, leaving all 256 ring slots free),
         * then 85 3-byte messages (255 bytes in the ring, 1 slot left). */
        uint32_t i;
        int ok = smk_trs_midi_send(RT1(0xFA)) == 1;
        for (i = 0; i < 85u; i++)
            ok &= smk_trs_midi_send(PK3(0x80, i & 127u, 0)) == 1;
        /* Only 1 byte remains in the ring: a 3-byte Note-On must fail atomically (-1) without corrupting the ring */
        ok &= smk_trs_midi_send(PK3(0x90, 60, 100)) == -1;
        ok &= smk_trs_midi_send(PK2(0xC0, 5)) == -1;
        /* A 1-byte real-time message still fits in the last byte slot, then the ring is completely full */
        ok &= smk_trs_midi_send(RT1(0xFC)) == 1;
        ok &= smk_trs_midi_send(RT1(0xF8)) == -1;
        run_ticks_until_idle(2000);
        ok &= wire_n == 1u + 85u * 3u + 1u && wire[0] == 0xFAu && wire[wire_n - 1u] == 0xFCu;
        for (i = 0; i < 85u; i++) {
            uint32_t off = 1u + 3u * i;
            ok &= wire[off] == 0x80u && wire[off + 1u] == (i & 127u) && wire[off + 2u] == 0u;
        }
        check("256-byte ring rejects partial multi-byte packets on overflow", ok);
    }

    return fails != 0;
}
