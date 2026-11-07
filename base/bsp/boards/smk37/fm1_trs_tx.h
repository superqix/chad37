/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Michael Wilson
 * Hardware pin and register mapping (UART1 TX on PC7 at 31,250 baud) credited to
 * M. Geisler (genkigenki/makkuro), fm1-to-smk37. */
/* M-Vave SMK-37 TRS MIDI OUT (BOARD_SMK37; boards/smk37): UART1 TX routed to PC7 via Output Channel 1
 * (IOMAP_CON3 bits [23:20] = 1, bit 7 = 0) at 31,250 baud (PLL48M / 4 / 384). Non-blocking 256-byte ring
 * drained one byte at a time from the 10 kHz TIMER5 scan ISR (100 us tick; 320 us per byte at 31,250 baud),
 * so multi-note chords serialize at full MIDI wire rate without blocking the main loop or audio ISR.
 * RX remains disabled (PH8 is a key-matrix row on the SMK-37).
 *
 *   smk_trs_tx_init()          configure PC7, IOMAP_CON3 and UART1 at 31,250 baud (before TIMER5 starts)
 *   smk_trs_midi_send(pkt)     enqueue a 4-byte USB-MIDI packet: 1 queued, 0 ignored, -1 full
 *   smk_trs_tx_tick()          10 kHz TIMER5 ISR: feed next byte when the shift register finishes (TPND)
 *   smk_trs_tx_pending()       1 while bytes remain in the ring or the last byte is still shifting out
 */
#pragma once
#include <stdint.h>
#include "fm1_cc.h"
#include "fm1_gpio.h"

#ifndef FELUCCA_LOADER

#ifndef SMK_TRS_TX_HOST_TEST
#define SMK_UT1_CON0       (*(volatile uint32_t *)0x12100u)
#define SMK_UT1_CON1       (*(volatile uint32_t *)0x12104u)
#define SMK_UT1_BAUD       (*(volatile uint32_t *)0x12108u)
#define SMK_UT1_BUF        (*(volatile uint8_t *)0x1210Cu)
#define SMK_TRS_CLK_CON1   (*(volatile uint32_t *)0x10010u)   /* [11:10] UART clock: 1 = PLL48M */
#define SMK_TRS_IOMAP_CON3 (*(volatile uint32_t *)0x51028u)   /* b7 UT1 fixed IO; [23:20] output ch 1 source */
#endif

#define SMK_TRS_TXQ 256u                                      /* power of two: 85 3-byte MIDI messages */

static struct {
    uint8_t q[SMK_TRS_TXQ];
    volatile uint32_t w, r;
    volatile uint8_t busy;
} smk_trs;

#ifndef SMK_TRS_TX_HOST_TEST
FM1_INLINE void smk_trs_tx_init(void)
{
    uint32_t m = 1u << 7;                                     /* PC7 */
    smk_trs.w = smk_trs.r = 0;
    smk_trs.busy = 0;
    SMK_UT1_CON0 = 0x3400u;                                   /* off, pending flags cleared */
    SMK_UT1_CON1 = 0;
    SMK_TRS_CLK_CON1 = (SMK_TRS_CLK_CON1 & ~(3u << 10)) | (1u << 10);
    /* Route UART1 TX (1) to Output Channel 1 (IOMAP_CON3[23:20]) and clear UT1 fixed-pin mode (bit 7). */
    SMK_TRS_IOMAP_CON3 = (SMK_TRS_IOMAP_CON3 & ~(0x80u | (0xFu << 20))) | (1u << 20);
    /* Connect PC7 to Output Channel 1: DIR = 0, PU = 1, PD = 1, {OUT, DIE} = 0b01. */
    FM1_PR(FM1_PC, FM1_OUT) &= ~m;
    FM1_PR(FM1_PC, FM1_DIE) |= m;
    FM1_PR(FM1_PC, FM1_PU) |= m;
    FM1_PR(FM1_PC, FM1_PD) |= m;
    FM1_PR(FM1_PC, FM1_DIR) &= ~m;
    SMK_UT1_BAUD = 48000000u / 31250u / 4u - 1u;              /* 383 */
    __asm__ volatile("csync" ::: "memory");
    SMK_UT1_CON0 = 0x2001u;                                   /* CLR_TPND | UTEN (TX only, polled) */
}

FM1_INLINE int smk_trs_hw_done(void) { return (SMK_UT1_CON0 & 0x8000u) != 0u; }

FM1_INLINE void smk_trs_hw_put(uint8_t b)
{
    SMK_UT1_CON0 |= 0x2000u;                                  /* clear TPND before starting next frame */
    SMK_UT1_BUF = b;
    __asm__ volatile("csync" ::: "memory");
}
#endif

FM1_INLINE void smk_trs_tx_tick(void)
{
    if (smk_trs.busy) {
        if (!smk_trs_hw_done())
            return;
        smk_trs.busy = 0;
    }
    if (smk_trs.r != smk_trs.w) {
        uint8_t b = smk_trs.q[smk_trs.r & (SMK_TRS_TXQ - 1u)];
        smk_trs.r++;
        smk_trs.busy = 1;
        smk_trs_hw_put(b);
    }
}

static int smk_trs_midi_send(uint32_t pkt)                    /* caller holds plat_lock() or runs in TIMER5 */
{
    static const uint8_t LEN[16] = {0, 0, 2, 3, 3, 1, 2, 3, 3, 3, 3, 3, 2, 2, 3, 1};
    uint32_t n = LEN[pkt & 0xFu], w = smk_trs.w, i;
    if (!n)
        return 0;
    if (SMK_TRS_TXQ - (w - smk_trs.r) < n)
        return -1;
    for (i = 0; i < n; i++)
        smk_trs.q[(w + i) & (SMK_TRS_TXQ - 1u)] = (uint8_t)(pkt >> (8u * (i + 1u)));
    __asm__ volatile("" ::: "memory");
    smk_trs.w = w + n;
    smk_trs_tx_tick();
    return 1;
}

FM1_INLINE int smk_trs_tx_pending(void)
{
    return smk_trs.r != smk_trs.w || smk_trs.busy;
}

#endif /* !FELUCCA_LOADER */
