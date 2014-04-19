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
#include <string.h>

/* Private define ------------------------------------------------------------*/
#define DAC_DHR12R2_ADDRESS      0x40007414
#define DAC_DHR8R1_ADDRESS       0x40007410

#define BACKGROUND_COLOR COLOR_WHITE

#define SET_DATE_STRING_INDEX 4 // index of variable part in StringSetDateCaption
#define VERTICAL_SLIDER_MAX_VALUE 180
#define DAC_SLIDER_SIZE 5

#define DAC_START_AMPLITUDE 2048
#define DAC_START_FREQ_SLIDER_VALUE (8 * 20) // 8 = log 2(256)
/* Private variables ---------------------------------------------------------*/
const uint16_t Sine12bit[32] = { 2047, 2447, 2831, 3185, 3498, 3750, 3939, 4056, 4095, 4056, 3939, 3750, 3495, 3185, 2831, 2447,
        2047, 1647, 1263, 909, 599, 344, 155, 38, 0, 38, 155, 344, 599, 909, 1263, 1647 };

static TouchButton * TouchButtonStartStop;
static TouchButton * TouchButtonSetWaveform;

static TouchButtonAutorepeat * TouchButtonAutorepeatFrequencyPlus;
static TouchButtonAutorepeat * TouchButtonAutorepeatFrequencyMinus;

const char StringTriangle[] = "Triangle";
const char StringNoise[] = "Noise";
const char StringSine[] = "Sine";
const char * const WaveformStrings[] = { StringTriangle, StringNoise, StringSine };

static uint16_t sAmplitude;
static uint32_t sFrequency;
static uint16_t sLastFrequencySliderValue;
static uint32_t sPeriodMicros;
static uint32_t sTimerReloadValue;
static uint16_t sOffset = 0;
static uint16_t sLastOffsetSliderValue = 0;
static uint16_t sLastAmplitudeSliderValue = VERTICAL_SLIDER_MAX_VALUE - 20;

#define WAVEFORM_TRIANGLE 0
#define WAVEFORM_NOISE 1
#define WAVEFORM_SINE 2
#define WAVEFORM_MAX WAVEFORM_NOISE
#define MAX_SLIDER_VALUE 240

#define FREQUENCY_SLIDER_START_X 37

static uint8_t sActualWaveform = WAVEFORM_TRIANGLE;

// shifted to get better resolution
uint32_t actualTimerAutoreload = 0x100;

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

static void doChangeDACWaveform(TouchButton * const aTheTouchedButton, int16_t aValue) {
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

/**
 * Computes ComputeReloadValue value for DAC timer, minimum is 4
 * @param aSliderValue
 */
static unsigned int ComputeReloadValue(uint16_t aSliderValue) {
    // highest frequency right
    aSliderValue = MAX_SLIDER_VALUE - aSliderValue;
    // logarithmic scale starting with 4
    unsigned int tValue = 1 << ((aSliderValue / 20) + 2);
    // between two logarithmic values we have a linear scale ;-)
    tValue += (tValue * (aSliderValue % 20)) / 20;
    return tValue;
}

/**
 * Computes Autoreload value for DAC timer, minimum is 4
 * @param aSliderValue
 * @param aSetTimer
 */
static void ComputeFrequencyAndSetTimer(uint16_t aReloadValue, bool aSetTimer) {
    if (aSetTimer) {
        sTimerReloadValue = aReloadValue;
        DAC_Timer_SetReloadValue(aReloadValue);
    }
    // Frequency is 72Mhz / Divider
    sFrequency = 72000000 / (2 * sAmplitude * aReloadValue);
    sPeriodMicros = (2 * sAmplitude * aReloadValue) / 72;
    /*
     * Print values
     */
    snprintf(StringBuffer, sizeof StringBuffer, "%7luHz", sFrequency);
    drawText(FREQUENCY_SLIDER_START_X + 3, TouchSliderHorizontal.getPositionYBottom() + 6, StringBuffer, 1, COLOR_BLUE,
            BACKGROUND_COLOR);
    if (sPeriodMicros > 100000) {
        snprintf(StringBuffer, sizeof StringBuffer, "%5lums", sPeriodMicros / 1000);
    } else {
        snprintf(StringBuffer, sizeof StringBuffer, "%5luus", sPeriodMicros);
    }
    drawText(TouchSliderHorizontal.getPositionXRight() - 8 * FONT_WIDTH, TouchSliderHorizontal.getPositionYBottom() + 6,
            StringBuffer, 1, COLOR_BLUE, BACKGROUND_COLOR);
}

static void doChangeDACFrequency(TouchButton * const aTheTouchedButton, int16_t aValue) {

    uint32_t tTimerReloadValue = sTimerReloadValue + aValue;
    if (tTimerReloadValue < DAC_TIMER_MIN_RELOAD_VALUE) {
        FeedbackTone(FEEDBACK_TONE_SHORT_ERROR);
        return;
    }
    FeedbackToneOK();
    ComputeFrequencyAndSetTimer(tTimerReloadValue, true);
    int tNewSliderValue = sLastFrequencySliderValue;
    // check if slider value must be updated
    if (aValue > 0) {
        // increase period, decrease frequency
        tNewSliderValue--;
        if (ComputeReloadValue(tNewSliderValue) <= sTimerReloadValue) {
            do {
                tNewSliderValue--;
            } while (ComputeReloadValue(tNewSliderValue) <= sTimerReloadValue);
            tNewSliderValue++;
            sLastFrequencySliderValue = tNewSliderValue;
            TouchSliderHorizontal.setActualValueAndDrawBar(tNewSliderValue);
            TouchSliderHorizontal.printValue();
        }
    } else {
        tNewSliderValue++;
        if (ComputeReloadValue(tNewSliderValue) >= sTimerReloadValue) {
            do {
                tNewSliderValue++;
            } while (ComputeReloadValue(tNewSliderValue) >= sTimerReloadValue);
            tNewSliderValue--;
            sLastFrequencySliderValue = tNewSliderValue;
            TouchSliderHorizontal.setActualValueAndDrawBar(tNewSliderValue);
            TouchSliderHorizontal.printValue();
        }
    }
}

void initDACPage(void) {
    DAC_init();
    DAC_TriangleAmplitude(10); //log2(DAC_START_AMPLITUDE)
    sAmplitude = DAC_START_AMPLITUDE;
    DAC_Timer_initialize(0xFF); // Don't really need value here
    sLastFrequencySliderValue = DAC_START_FREQ_SLIDER_VALUE;
    ComputeFrequencyAndSetTimer(ComputeReloadValue(sLastFrequencySliderValue), false);
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
    ComputeFrequencyAndSetTimer(ComputeReloadValue(TouchSliderHorizontal.getActualValue()), false);
    TouchSliderHorizontal.printValue(); // print new value for frequency slider
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
    float tOffsetVoltage = sVrefVoltage * aOffset / VERTICAL_SLIDER_MAX_VALUE;
    snprintf(StringBuffer, sizeof StringBuffer, "%0.2fV", tOffsetVoltage);
    return StringBuffer;
}

uint16_t doDACFrequencySlider(TouchSlider * const aTheTouchedSlider, uint16_t aFrequency) {
    sLastFrequencySliderValue = aFrequency;
    ComputeFrequencyAndSetTimer(ComputeReloadValue(aFrequency), true);
    return aFrequency;
}

void doDACStop(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    aValue = !aValue;
    if (aValue) {
        aTheTouchedButton->setCaption(StringStop);
        DAC_Start();
    } else {
        DAC_Stop();
        aTheTouchedButton->setCaption(StringStart);
    }
    aTheTouchedButton->setRedGreenButtonColorAndDraw(aValue);
}

void startDACPage(void) {
    clearDisplay(COLOR_BACKGROUND_DEFAULT);
    TouchSlider::setDefaultBarColor(TOUCHSLIDER_DEFAULT_BAR_COLOR);
    TouchSlider::setDefaultSliderColor(COLOR_BLUE);

    //1. row
    int tPosY = 0;
    TouchButtonStartStop = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            COLOR_RED, StringStop, 2, true, &doDACStop);
    TouchButtonStartStop->setRedGreenButtonColor();
    initMainHomeButtonWithPosition(BUTTON_WIDTH_3_POS_3, 0, true);

    //2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    // Frequency slider
    TouchSliderHorizontal.initSlider(FREQUENCY_SLIDER_START_X, tPosY, DAC_SLIDER_SIZE, MAX_SLIDER_VALUE, MAX_SLIDER_VALUE,
            sLastFrequencySliderValue, "Frequency", TOUCHSLIDER_DEFAULT_TOUCH_BORDER,
            TOUCHSLIDER_SHOW_BORDER | TOUCHSLIDER_IS_HORIZONTAL, &doDACFrequencySlider, NULL);

    // 3. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonSetWaveform = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            COLOR_RED, StringTriangle, 1, 1, &doChangeDACWaveform);

    TouchButtonAutorepeatFrequencyMinus = TouchButtonAutorepeat::allocAndInitSimpleButton(FREQUENCY_SLIDER_START_X + 6, tPosY,
            BUTTON_WIDTH_5, BUTTON_HEIGHT_4, COLOR_RED, StringMinus, 2, 1, &doChangeDACFrequency);
    TouchButtonAutorepeatFrequencyPlus = TouchButtonAutorepeat::allocAndInitSimpleButton(230, tPosY, BUTTON_WIDTH_5,
            BUTTON_HEIGHT_4, COLOR_RED, StringPlus, 2, -1, &doChangeDACFrequency);
    TouchButtonAutorepeatFrequencyMinus->setButtonAutorepeatTiming(600, 100, 10, 20);
    TouchButtonAutorepeatFrequencyPlus->setButtonAutorepeatTiming(600, 100, 10, 20);

//Amplitude slider
    TouchSliderVertical.initSlider(5, 20, DAC_SLIDER_SIZE, VERTICAL_SLIDER_MAX_VALUE, VERTICAL_SLIDER_MAX_VALUE,
            sLastAmplitudeSliderValue, "Amplitude", TOUCHSLIDER_DEFAULT_TOUCH_BORDER,
            TOUCHSLIDER_SHOW_BORDER | TOUCHSLIDER_SHOW_VALUE, &doDACVolumeSlider, &mapDACVolumeValue);

//Offset slider
    TouchSliderVertical2.initSlider(DISPLAY_WIDTH - (TOUCHSLIDER_OVERALL_SIZE_FACTOR * DAC_SLIDER_SIZE) - 1, 20, DAC_SLIDER_SIZE,
            VERTICAL_SLIDER_MAX_VALUE, VERTICAL_SLIDER_MAX_VALUE / 2, sLastOffsetSliderValue, "Offset",
            TOUCHSLIDER_DEFAULT_TOUCH_BORDER, TOUCHSLIDER_SHOW_BORDER | TOUCHSLIDER_SHOW_VALUE, &doDACOffsetSlider,
            &mapDACOffsetValue);
    TouchSliderVertical2.setXOffsetValue(-(2 * FONT_WIDTH));

// draw buttons and slider
    TouchButtonSetWaveform->drawButton();
    TouchButtonStartStop->drawButton();
    TouchButtonAutorepeatFrequencyPlus->drawButton();
    TouchButtonAutorepeatFrequencyMinus->drawButton();

    TouchSliderVertical.drawSlider();
    TouchSliderVertical2.drawSlider();
    TouchSliderHorizontal.drawSlider();
    ComputeFrequencyAndSetTimer(ComputeReloadValue(sLastFrequencySliderValue), true);
    DAC_Start();
}

void stopDACPage(void) {
    TouchButtonStartStop->setFree();
    TouchButtonSetWaveform->setFree();
    TouchButtonAutorepeatFrequencyPlus->setFree();
    TouchButtonAutorepeatFrequencyMinus->setFree();
}

