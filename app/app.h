/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Michael Wilson */
/* Chordpad37: a MIDI controller for the M-Vave SMK-37, on fm1-base. This header is the app's half of the contract with
 * the platform (base/device/unity.c, base/host/plat_host.c). */
#pragma once
#include <stdint.h>
#include "plat.h"

#if !BOARD_SMK37
#error "Chordpad37 builds for the SMK-37 only (BOARD=smk37)"
#endif

#define APP_NAME "CHORDPAD"
#define APP_SPLASH_RGB RGB(242, 120, 72)
#define APP_TAGLINE "a midi controller"
#ifndef APP_VERSION
#define APP_VERSION "0.1"                /* (tools/build.py --release passes its own) */
#endif
#define APP_BLOCK 256u                   /* frames per audio block: 5.8 ms at 44.1 kHz */
#define APP_GAIN(m) app_gain(m)
uint32_t app_gain(uint32_t master);

/* the store: 8 presets + global header in OBJ_PRESETS, the panel calibration (the platform's) and Bluetooth's
 * address and pairing. The SMK-37 build puts the objects in its own window by number, so the numbers stay; it holds
 * 9 objects at most */
enum { OBJ_PRESETS, OBJ_PANEL, OBJ_BLE, OBJ_NOBJ };
#define OBJ_SETTINGS OBJ_PRESETS
#define APP_STORE_MAP {0xFC000u, 0xDC000u, 0xDE000u}

/* the contract: the platform calls these */
void app_init(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a, const char *b);

#ifdef PLAT_HOST
#define APP_HOST_EXPECT                  /* the host simulator's checks (app/tests/ *.txt) */
#define APP_HOST_REBOOT                  /* clear volatile UI state across a simulated reboot */
int app_expect(const char *what, int *got);
void app_host_reboot(void);
#endif
