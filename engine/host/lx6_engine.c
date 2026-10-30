/*
 * lx6_engine.c - runs the LXR firmware sound engine inside a plugin.
 *
 * Mirrors what soundengine_init()/soundengine_process() and the preset
 * manager do on the hardware, minus the codec, SD card and front panel.
 *
 * Part of LX-6, based on the open-source Erica Synths LXR firmware.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "lx6_engine.h"
#include "stm32f7xx_hal.h"
#include "Parameters.h"
#include "Soundengine/DSPAudio/mixer.h"
#include "Soundengine/DSPAudio/DrumVoice.h"
#include "Soundengine/DSPAudio/Snare.h"
#include "Soundengine/DSPAudio/HiHat.h"
#include "Soundengine/DSPAudio/CymbalVoice.h"
#include "Soundengine/DSPAudio/modulationNode.h"
#include "Soundengine/MIDI/MidiMessages.h"
#include "Soundengine/MIDI/MidiParser.h"
#include "Soundengine/MIDI/ParameterArray.h"
#include "Soundengine/MIDI/MidiVoiceControl.h"
#include "Soundengine/Sequencer/sequencer.h"
#include "Soundengine/Sequencer/EuklidGenerator.h"
#include "Preset/InitKit.h"
#include "menu/menu.h"
#include "menu/CcNr2Text.h"
#include "frontPanelParser_ui.h"

extern uint8_t activeParameters[];
extern volatile uint32_t systick_ticks;
void frontParser_parseUartData(unsigned char data);

/* the mixer writes OUTPUT_DMA_SIZE stereo frames to two stereo buffers */
static int16_t lx6_out1[OUTPUT_DMA_SIZE * 2];
static int16_t lx6_out2[OUTPUT_DMA_SIZE * 2];
static double lx6_msAccum = 0.0;

static void front_send(uint8_t status, uint8_t d1, uint8_t d2)
{
	frontParser_parseUartData(status);
	frontParser_parseUartData(d1);
	frontParser_parseUartData(d2);
}

void lx6_engine_init(void)
{
	mixer_init();
	parameterArray_init();
	euklid_init();
	for (int i = 0; i < 6; i++)
		modNode_init(&velocityModulators[i]);

	seq_init();
	seq_setBpm(120);
	memset(midi_MidiChannels, 0, 8);

	initDrumVoice();
	Snare_init();
	HiHat_init();
	Cymbal_init();

	lx6_engine_loadInitKit();
}

void lx6_engine_render(int16_t* out1, int16_t* out2)
{
	/* the firmware fills its 2nd buffer with DAC1 (out 1) and the 1st with DAC2 (out 2) */
	mixer_calcNextSampleBlock(lx6_out2, lx6_out1);
	memcpy(out1, lx6_out1, sizeof(lx6_out1));
	memcpy(out2, lx6_out2, sizeof(lx6_out2));

	/* keep the firmware's millisecond clock running (LFO sync, timers) */
	lx6_msAccum += (double)OUTPUT_DMA_SIZE * 1000.0 / LX6_ENGINE_RATE;
	while (lx6_msAccum >= 1.0) { systick_ticks++; lx6_msAccum -= 1.0; }
}

static void sendModTarget(uint8_t frontStatus, uint8_t slot, uint8_t modTargetIdx)
{
	const uint8_t n = getNumModTargets();
	if (modTargetIdx >= n) modTargetIdx = 0;
	const uint8_t value = (uint8_t)modTargets[modTargetIdx].param;
	const uint8_t upper = (uint8_t)(((value & 0x80) >> 7) | ((slot & 0x3f) << 1));
	const uint8_t lower = value & 0x7f;
	front_send(frontStatus, upper, lower);
}

void lx6_engine_setParam(uint16_t paramNr, uint8_t value)
{
	if (paramNr >= END_OF_SOUND_PARAMETERS) return;
	activeParameters[paramNr] = value;

	/* velocity and LFO destinations are stored as mod-target indices */
	if (paramNr >= PAR_VEL_DEST_1 && paramNr <= PAR_VEL_DEST_6) {
		sendModTarget(CC_VELO_TARGET, (uint8_t)(paramNr - PAR_VEL_DEST_1), value);
		return;
	}
	if (paramNr >= PAR_TARGET_LFO1 && paramNr <= PAR_TARGET_LFO6) {
		sendModTarget(CC_LFO_TARGET, (uint8_t)(paramNr - PAR_TARGET_LFO1), value);
		return;
	}

	MidiMsg msg = {0};
	if (paramNr < 128) {
		msg.status = MIDI_CC;
		msg.data1 = (uint8_t)((paramNr + 1) & 0x7f);
	} else {
		msg.status = MIDI_CC2;
		msg.data1 = (uint8_t)((paramNr - 128) & 0x7f);
	}
	msg.data2 = value;
	midiParser_ccHandler(msg, 1, 0, 0.f);
}

uint8_t lx6_engine_getParam(uint16_t paramNr)
{
	return paramNr < END_OF_SOUND_PARAMETERS ? activeParameters[paramNr] : 0;
}

void lx6_engine_trigger(uint8_t track, uint8_t velocity, uint8_t note)
{
	if (track > 6) return;
	voiceControl_noteOff(track);
	voiceControl_noteOn(track, note, velocity);
}

void lx6_engine_setBpm(uint16_t bpm)
{
	if (bpm < 20) bpm = 20;
	if (bpm != seq_getBpm())
		seq_setBpm(bpm);
}

void lx6_engine_loadInitKit(void)
{
	/* the init kit starts with an 8 byte name, then the sound parameters */
	for (int i = 8; i < END_OF_SOUND_PARAMETERS + 8 && i < INIT_KIT_SIZE; i++)
		activeParameters[i - 8] = initKit[i];

	for (uint16_t p = 0; p < END_OF_SOUND_PARAMETERS; p++)
		lx6_engine_setParam(p, activeParameters[p]);
}
