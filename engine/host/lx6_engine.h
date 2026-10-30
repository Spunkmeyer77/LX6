/*
 * lx6_engine.h - C interface between the LX-6 plugin and the LXR sound engine.
 *
 * Part of LX-6, based on the open-source Erica Synths LXR firmware.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* the engine renders this many stereo frames per call */
#define LX6_BLOCK_FRAMES 64
/* the engine's fixed internal sample rate (the LXR hardware's real rate) */
#define LX6_ENGINE_RATE 44140.0

#define LX6_NUM_VOICES 6
#define LX6_NUM_TRACKS 7   /* 6 voices + open hi-hat */
#define LX6_NUM_SOUND_PARAMS 247

void lx6_engine_init(void);

/* render one block into the LXR's two stereo outputs (interleaved L/R,
 * LX6_BLOCK_FRAMES frames each). Advances the engine's millisecond clock. */
void lx6_engine_render(int16_t* out1, int16_t* out2);

/* set one LXR sound parameter (Parameters.h numbering, 0-246) */
void lx6_engine_setParam(uint16_t paramNr, uint8_t value);
uint8_t lx6_engine_getParam(uint16_t paramNr);

/* trigger a track (0-5 voices, 6 = open hi-hat), velocity 1-127 */
void lx6_engine_trigger(uint8_t track, uint8_t velocity, uint8_t note);

/* sync LFOs to the host tempo */
void lx6_engine_setBpm(uint16_t bpm);

/* load the factory "Init" kit into the engine */
void lx6_engine_loadInitKit(void);

/* implemented by the plugin: MIDI the engine wants to send */
void lx6_engine_onMidiOut(uint8_t status, uint8_t data1, uint8_t data2);

#ifdef __cplusplus
}
#endif
