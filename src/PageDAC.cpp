/*
 * PageDAC.cpp
 *
 * @date 16.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 */

#include "Pages.h"
#include "misc.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {

#include "stm32f30xPeripherals.h"
#include "timing.h"

}
#endif

/* Private define ------------------------------------------------------------*/
#define DAC_DHR12R2_ADDRESS      0x40007414
#define DAC_DHR8R1_ADDRESS       0x40007410

#define MENU_TOP 15
#define MENU_LEFT 30
#define BACKGROUND_COLOR COLOR_WHITE

#define SET_DATE_STRING_INDEX 4 // index of variable part in StringSetDateCaption
#define VERTICAL_SLIDER_MAX_VALUE 180
#define DAC_SLIDER_SIZE 5

#define DAC_START_PERIOD 256
#define DAC_START_AMPLITUDE 2048
#define DAC_START_FREQ_SLIDER_VALUE (8 * 20) // 8 = log 2(256)
/* Private variables ---------------------------------------------------------*/
const uint16_t Sine12bit[32] = { 2047, 2447, 2831, 3185, 3498, 3750, 3939, 4056, 4095, 4056, 3939, 3750, 3495, 3185, 2831, 2447,
		2047, 1647, 1263, 909, 599, 344, 155, 38, 0, 38, 155, 344, 599, 909, 1263, 1647 };

static TouchButton * TouchButtonStartStop;
static TouchButton * TouchButtonSetWaveform;
const char StringTriangle[] = "Triangle";
const char StringNoise[] = "Noise";
const char StringSine[] = "Sine";
const char * const WaveformStrings[] = { StringTriangle, StringNoise, StringSine };

static uint16_t sAmplitude;
static uint32_t sFrequency;
static uint16_t sLastFrequencySliderValue = DAC_START_FREQ_SLIDER_VALUE;
static uint32_t sPeriodMicros;
static uint16_t sOffset = 0;
static uint16_t sLastOffsetSliderValue = 0;
static uint16_t sLastAmplitudeSliderValue = VERTICAL_SLIDER_MAX_VALUE - 20;
static bool sDacIsStopped = true;

#define WAVEFORM_TRIANGLE 0
#define WAVEFORM_NOISE 1
#define WAVEFORM_SINE 2
#define WAVEFORM_MAX WAVEFORM_NOISE
static uint8_t sActualWaveform = WAVEFORM_TRIANGLE;

// shifted to get better resolution
uint32_t actualTimerAutoreload = 0x100;

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

static void doChangeDACFrequency(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	sActualWaveform++;
	if (sActualWaveform == WAVEFORM_NOISE) {
		DAC_ModeNoise();
		DAC_Start();
	} else if (sActualWaveform > WAVEFORM_MAX) {
		sActualWaveform = WAVEFORM_TRIANGLE;
		DAC_ModeTriangle();
		DAC_Start();
	}
	aTheTouchedButton->setCaption(WaveformStrings[sActualWaveform]);
	aTheTouchedButton->drawButton();
}

void initDACPage(void) {
	DAC_init();
	DAC_TriangleAmplitude(10); //log2(DAC_START_AMPLITUDE)
	sAmplitude = DAC_START_AMPLITUDE;
	DAC_Timer_initialize(DAC_START_PERIOD);
	sFrequency = 72000000 / (2 * DAC_START_AMPLITUDE * DAC_START_PERIOD);
	sPeriodMicros = (2 * DAC_START_AMPLITUDE * DAC_START_PERIOD) / 72;
}

/**
 * Computes Autoreload value for DAC timer, minimum is 4
 * @param aSliderValue
 * @param aSetTimer
 */
static void ComputeFrequencyAndSetTimer(uint16_t aSliderValue, bool aSetTimer) {
	// logarithmic scale starting with 4
	unsigned int tValue = 1 << ((aSliderValue / 20) + 2);
	// between two logarithmic values we have a linear scale ;-)
	tValue += (tValue * (aSliderValue % 20)) / 20;
	if (aSetTimer) {
		DAC_Timer_SetReloadValue(tValue);
	}
	// Frequency is 72Mhz / Divider
	sFrequency = 72000000 / (2 * sAmplitude * tValue);
	sPeriodMicros = (2 * sAmplitude * tValue) / 72;

}
uint16_t doDACVolumeSlider(TouchSlider * const aTheTouchedSlider, uint16_t aAmplitude) {
	DAC_TriangleAmplitude(aAmplitude / 16);
	sLastAmplitudeSliderValue = (aAmplitude / 16) * 16;
	return sLastAmplitudeSliderValue;
}

const char * mapDACVolumeValue(uint16_t aAmplitude) {
	unsigned int tValue = (0x01 << ((aAmplitude / 16) + 1)) - 1;
	sAmplitude = tValue + 1;
	// update frequency
	ComputeFrequencyAndSetTimer(TouchSliderHorizontal.getActualValue(), false);
	TouchSliderHorizontal.printValue();
	snprintf(StringBuffer, sizeof StringBuffer, "%4u", tValue);
	return StringBuffer;
}

uint16_t doDACOffsetSlider(TouchSlider * const aTheTouchedSlider, uint16_t aOffset) {
	sLastOffsetSliderValue = aOffset;
//	if (sActualWaveform == WAVEFORM_TRIANGLE) {
	sOffset = aOffset * (4096 / VERTICAL_SLIDER_MAX_VALUE);
	DAC_SetOutputValue(sOffset);
//	}
	return aOffset;
}

const char * mapDACOffsetValue(uint16_t aOffset) {
	unsigned int tValue = aOffset * (4096 / VERTICAL_SLIDER_MAX_VALUE);
	snprintf(StringBuffer, sizeof StringBuffer, "%3x", tValue);
	return StringBuffer;
}

uint16_t doDACFrequencySlider(TouchSlider * const aTheTouchedSlider, uint16_t aFrequency) {
	sLastFrequencySliderValue = aFrequency;
	ComputeFrequencyAndSetTimer(aFrequency, true);
	return aFrequency;
}

const char * mapDACFrequencyValue(uint16_t aFrequency) {
	if (sActualWaveform == WAVEFORM_TRIANGLE) {
		snprintf(StringBuffer, sizeof StringBuffer, "%7luHz", sFrequency);
		return StringBuffer;
	}
	return StringEmpty;
}

void doDACStop(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	if (sDacIsStopped) {
		aTheTouchedButton->setCaption(StringStop);
		DAC_Start();
	} else {
		DAC_Stop();
		aTheTouchedButton->setCaption(StringStart);
	}
	sDacIsStopped = !sDacIsStopped;
	aTheTouchedButton->drawButton();
}

void startDACPage(void) {
	clearDisplay(COLOR_BACKGROUND_DEFAULT);

	TouchButtonSetWaveform = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_3,
			BUTTON_HEIGHT_4, COLOR_RED, StringTriangle, 1, 1, &doChangeDACFrequency);
	TouchButtonSetWaveform->drawButton();

	TouchButtonStartStop = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_3, BUTTON_WIDTH_3,
			BUTTON_HEIGHT_4, COLOR_RED, StringStop, 2, 0, &doDACStop);
	TouchButtonStartStop->drawButton();

	TouchSlider::setDefaultBarColor(TOUCHSLIDER_DEFAULT_BAR_COLOR);
	TouchSlider::setDefaultSliderColor(COLOR_BLUE);
	//Amplitude slider
	TouchSliderVertical.initSlider(5, 20, DAC_SLIDER_SIZE, VERTICAL_SLIDER_MAX_VALUE, VERTICAL_SLIDER_MAX_VALUE,
			sLastAmplitudeSliderValue, "Amplitude", TOUCHSLIDER_DEFAULT_TOUCH_BORDER,
			TOUCHSLIDER_SHOW_BORDER | TOUCHSLIDER_SHOW_VALUE, &doDACVolumeSlider, &mapDACVolumeValue);

	//Offset slider
	TouchSliderVertical2.initSlider(DISPLAY_WIDTH - (TOUCHSLIDER_OVERALL_SIZE_FACTOR * DAC_SLIDER_SIZE) - 1, 20, DAC_SLIDER_SIZE,
			VERTICAL_SLIDER_MAX_VALUE, VERTICAL_SLIDER_MAX_VALUE / 2, sLastOffsetSliderValue, "Offset",
			TOUCHSLIDER_DEFAULT_TOUCH_BORDER, TOUCHSLIDER_SHOW_BORDER | TOUCHSLIDER_SHOW_VALUE, &doDACOffsetSlider,
			&mapDACOffsetValue);

	// Frequency slider
	TouchSliderHorizontal.initSlider(37, 5, DAC_SLIDER_SIZE, 240, 240, sLastFrequencySliderValue, "Frequency",
			TOUCHSLIDER_DEFAULT_TOUCH_BORDER,
			TOUCHSLIDER_SHOW_BORDER | TOUCHSLIDER_SHOW_VALUE | TOUCHSLIDER_IS_HORIZONTAL | TOUCHSLIDER_HORIZONTAL_VALUE_LEFT,
			&doDACFrequencySlider, &mapDACFrequencyValue);

	TouchSliderVertical.drawSlider();
	TouchSliderVertical2.drawSlider();
	TouchSliderHorizontal.drawSlider();
	initMainHomeButtonWithPosition(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, true);
	DAC_Start();
	sDacIsStopped = false;
}

void stopDACPage(void) {
	TouchButtonStartStop->setFree();
	TouchButtonSetWaveform->setFree();
}

