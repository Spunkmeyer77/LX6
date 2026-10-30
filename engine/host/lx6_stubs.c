/*
 * lx6_stubs.c - host replacements for the LXR firmware's hardware layer
 * (front panel, LEDs, LCD menu, SD presets, trigger outs, UART/USB).
 *
 * Part of LX-6, based on the open-source Erica Synths LXR firmware.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "stm32f7xx_hal.h"
#include "Parameters.h"
#include "Soundengine/MIDI/MidiMessages.h"
#include "Preset/PresetManager.h"
#include "menu/menu.h"
#include "lx6_engine.h"

/* ---- globals the firmware expects ---- */
uint8_t activeParameters[NUM_PARAMS];
RNG_HandleTypeDef hrng;
uint8_t menu_activeBar = 0;
uint8_t menu_activePage = 0;
uint8_t menu_loadedPresetNr[NUM_CURRENT];
uint8_t menu_playedPattern = 0;
uint8_t menu_selectedPresetNr[NUM_PRESET_LOCATIONS];
uint8_t menu_shownPattern = 0;
uint8_t menu_songMenuPosition = 0;
uint8_t midi_process_block = 0;
volatile uint32_t systick_ticks = 0;
uint8_t trigger_clockReceived = 0;
uint8_t trigger_dividerClockOut1 = 0;
uint8_t trigger_prescalerClockInput = 0;

/* ---- random numbers (hardware RNG on the STM32) ---- */
static uint32_t lx6_rng_state = 0x12345678u;
uint32_t HAL_RNG_GetRandomNumber(RNG_HandleTypeDef* h)
{
	(void)h;
	/* xorshift32 */
	uint32_t x = lx6_rng_state;
	x ^= x << 13; x ^= x >> 17; x ^= x << 5;
	lx6_rng_state = x;
	return x;
}

/* ---- front panel / UI: nothing to do on the host ---- */
float adc_getFaderVolume(uint8_t idx) { (void)idx; return 1.f; }
uint8_t buttonHandler_getMode() { return 0; }
void led_blinkHandler() {}
void led_clearActive_step() {}
void led_clearAllBlinkLeds() {}
void led_clearSequencerLeds() {}
uint8_t led_idx2StepLed(uint8_t idx) { return idx; }
void led_initPatternSelectLeds() {}
void led_resetBlinkTimer() {}
void led_setActive_step(uint8_t stepNr) { (void)stepNr; }
void led_setBlinkLed(const uint8_t ledNr, const uint8_t onOff) { (void)ledNr; (void)onOff; }
void led_setValue(uint8_t val, uint8_t ledNr) { (void)val; (void)ledNr; }
void led_updateSeqLeds() {}
uint8_t menu_getActiveVoice() { return 0; }
uint8_t menu_getSubPage() { return 0; }
uint8_t menu_getViewedPattern() { return menu_shownPattern; }
void menu_repaint() {}
void menu_repaintAll() {}
void menu_setShownPattern(uint8_t patternNr) { menu_shownPattern = patternNr; }
void menu_switchSubPage(uint8_t subPageNr, uint8_t toggle) { (void)subPageNr; (void)toggle; }
void preset_loadDrumsetRam(uint8_t presetNr, uint8_t isMorph, uint8_t sendImmediate) { (void)presetNr; (void)isMorph; (void)sendImmediate; }
void preset_touchTimer(PresetTimer type) { (void)type; }
void trigger_clockTick(uint8_t onOff, uint8_t cnt) { (void)onOff; (void)cnt; }
void trigger_reset(uint8_t value) { (void)value; }
void uart_clearFrontFifo() {}
void uart_sendFrontpanelByte(uint8_t data) { (void)data; }
void uart_sendFrontpanelSysExByte(uint8_t data) { (void)data; }

/* ---- MIDI the engine sends out: forwarded to the plugin ---- */
void uart_sendMidi(MidiMsg msg) { lx6_engine_onMidiOut(msg.status, msg.data1, msg.data2); }
void usb_sendMidi(MidiMsg msg) { (void)msg; }
