/**
 * PageFrequencyGenerator.cpp
 *
 * @date 26.10.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 */

#include "Pages.h"
#include "misc.h"
#include <math.h>

const char String10[] = "10";
const char String20[] = "20";
const char String50[] = "50";
const char String100[] = "100";
const char String200[] = "200";
const char String500[] = "500";
const char String1000[] = "1000";
const char String1k[] = "1k";

/* Private define ------------------------------------------------------------*/

#define BACKGROUND_COLOR COLOR_WHITE
#define FREQ_SLIDER_SIZE 5 // size of border
#define FREQ_SLIDER_MAX_VALUE 300 /* size of slider */

#define NUMBER_OF_FIXED_FREQUENCY_BUTTONS 10

#define FREQUENCY_X_POS ((18 - sizeof(StringFrequency)) * TEXT_SIZE_11_WIDTH)

#define SLIDER_POS_X 5

/* Private variables ---------------------------------------------------------*/

const char StringFrequency[] = "Frequency";

static TouchButton * TouchButton1;
static TouchButton * TouchButton2;
static TouchButton * TouchButton5;
static TouchButton * TouchButton10;
static TouchButton * TouchButton20;
static TouchButton * TouchButton50;
static TouchButton * TouchButton100;
static TouchButton * TouchButton200;
static TouchButton * TouchButton500;
static TouchButton * TouchButton1k;
static TouchButton * TouchButtonmHz;
static TouchButton * TouchButtonHz;
static TouchButton * TouchButtonkHz;
static TouchButton * TouchButtonMHz;

static TouchButton * TouchButtonStartStop;
static TouchButton * TouchButtonSetFrequency;
static TouchButton ** TouchButtonsFreqGen[] = { &TouchButton1, &TouchButton2, &TouchButton5, &TouchButton10, &TouchButton20,
        &TouchButton50, &TouchButton100, &TouchButton200, &TouchButton500, &TouchButton1k, &TouchButtonmHz, &TouchButtonHz,
        &TouchButtonkHz, &TouchButtonMHz, &TouchButtonSetFrequency, &TouchButtonStartStop };

const uint16_t Frequency[NUMBER_OF_FIXED_FREQUENCY_BUTTONS] = { 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000 };
const char * const FrequencyStrings[NUMBER_OF_FIXED_FREQUENCY_BUTTONS] = { String1, String2, String5, String10, String20, String50,
        String100, String200, String500, String1k };

// Start values for 10 kHz
static float sFrequency = 10;
static uint8_t sFrequencyFactorIndex = 2; // for kHz

const char FrequencyFactorChars[4] = { 'm', ' ', 'k', 'M' };
// factor for mHz/Hz/kHz/MHz - times 100 because of mHz handling
// 1 -> 1 mHz, 1000 -> 1 Hz, 1000000 -> 1 kHz
static uint32_t sFrequencyFactorTimes1000;

/* Private function prototypes -----------------------------------------------*/
void drawFreqGenPage(void);
uint16_t doFrequencySlider(TouchSlider * const aTheTouchedSlider, uint16_t aValue);

/* Private functions ---------------------------------------------------------*/

void setFrequencyFactor(int aIndexValue) {
    sFrequencyFactorIndex = aIndexValue;
    int tFactor = 1;
    while (aIndexValue > 0) {
        tFactor *= 1000;
        aIndexValue--;
    }
    sFrequencyFactorTimes1000 = tFactor;
}

void initFreqGenPage(void) {
    HIRES_Timer_initialize(72000);
}

/**
 * Computes Autoreload value for synthesizer
 * from 8,381 mHz (0xFFFFFFFF) to 18MHz (0x02)
 * and prints frequency value
 * @param aSliderValue
 * @param aSetTimer
 */
void ComputePeriodAndSetTimer(bool aSetSlider) {
    float tPeriod = (36000000000 / sFrequencyFactorTimes1000) / sFrequency; // 3600... because timer runs in toggle mode
    uint32_t tPeriodInt = tPeriod;
    if (tPeriodInt < 2) {
        tPeriodInt = 2;
    }
    HIRES_Timer_SetReloadValue(tPeriodInt);
    // recompute exact frequency for given integer period
    tPeriod = tPeriodInt;
    float tFrequency = (36000000000 / sFrequencyFactorTimes1000) / tPeriod;
    snprintf(StringBuffer, sizeof StringBuffer, "%8.3f%cHz", tFrequency, FrequencyFactorChars[sFrequencyFactorIndex]);
    BlueDisplay1.drawText(FREQUENCY_X_POS - 2 * TEXT_SIZE_11_WIDTH, TEXT_SIZE_22_HEIGHT + 4 + TEXT_SIZE_22_ASCEND, StringBuffer,
            TEXT_SIZE_22, COLOR_RED, BACKGROUND_COLOR);
// output period
//	snprintf(StringBuffer, sizeof StringBuffer, "%#010lX", tPeriodInt);
//	BlueDisplay1.drawText(SLIDER_POS_X, BUTTON_HEIGHT_4_LINE_2 + BUTTON_HEIGHT_6 + BUTTON_DEFAULT_SPACING_HALF, StringBuffer, 1, COLOR_BLUE,
//	BACKGROUND_COLOR);
    if (aSetSlider) {
        uint16_t tValue = log10f(sFrequency) * 100;
        TouchSliderHorizontal.setActualValueAndDrawBar(tValue);
    }
}

uint16_t doFrequencySlider(TouchSlider * const aTheTouchedSlider, uint16_t aValue) {
    float tValue = aValue;
    tValue = tValue / (FREQ_SLIDER_MAX_VALUE / 3); // gives 0-3
    sFrequency = pow10f(tValue);
    ComputePeriodAndSetTimer(false);
    return aValue;
}

void doFreqGenStartStop(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    aValue = !aValue;
    if (aValue) {
        aTheTouchedButton->setCaption(StringStop);
        HIRES_Timer_Start();
    } else {
        aTheTouchedButton->setCaption(StringStart);
        HIRES_Timer_Stop();
    }
    aTheTouchedButton->setRedGreenButtonColorAndDraw(aValue);
}

void doSetFixedFrequency(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    sFrequency = aValue;
    ComputePeriodAndSetTimer(true);
}

/**
 * changes the unit (mHz - MHz)
 * @param aTheTouchedButton
 * @param aValue
 */
void doChangeFrequencyFactor(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    setFrequencyFactor(aValue);
    ComputePeriodAndSetTimer(true);
}

/**
 * gets frequency value from numberpad
 * @param aTheTouchedButton
 * @param aValue
 */
void doSetFrequency(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, COLOR_BLUE);
    // check for cancel
    if (!isnan(tNumber)) {
        sFrequency = tNumber;
    }
    drawFreqGenPage();
    ComputePeriodAndSetTimer(true);
}

/**
 * draws slider, text and buttons
 */
void drawFreqGenPage(void) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    TouchButtonMainHome->drawButton();

    BlueDisplay1.drawText(FREQUENCY_X_POS, 2 + TEXT_SIZE_22_ASCEND, StringFrequency, TEXT_SIZE_22, COLOR_GREEN, BACKGROUND_COLOR);

    // slider + labels
    TouchSliderHorizontal.drawSlider();
    BlueDisplay1.drawChar(SLIDER_POS_X, TouchSliderHorizontal.getPositionYBottom() + 2 + TEXT_SIZE_11_ASCEND, '1', TEXT_SIZE_11,
            COLOR_BLUE, BACKGROUND_COLOR);
    BlueDisplay1.drawText(TouchSliderHorizontal.getPositionXRight() - 4 * TEXT_SIZE_11_WIDTH,
            TouchSliderHorizontal.getPositionYBottom() + 2 + TEXT_SIZE_11_ASCEND, String1000, TEXT_SIZE_11, COLOR_BLUE,
            BACKGROUND_COLOR);

    // all frequency page buttons
    for (unsigned int i = 0; i < sizeof(TouchButtonsFreqGen) / sizeof(TouchButtonsFreqGen[0]); ++i) {
        (*TouchButtonsFreqGen[i])->drawButton();
    }
}

void startFreqGenPage(void) {

    // Frequency slider for 1 to 1000 at top of screen
    TouchSlider::setDefaultBarColor(TOUCHSLIDER_DEFAULT_BAR_COLOR);
    TouchSlider::setDefaultSliderColor(COLOR_BLUE);
    TouchSliderHorizontal.initSlider(SLIDER_POS_X, 4 * TEXT_SIZE_11_HEIGHT + 6, FREQ_SLIDER_SIZE, FREQ_SLIDER_MAX_VALUE,
            FREQ_SLIDER_MAX_VALUE, 0, NULL, TOUCHSLIDER_DEFAULT_TOUCH_BORDER, TOUCHSLIDER_SHOW_BORDER | TOUCHSLIDER_IS_HORIZONTAL,
            &doFrequencySlider, NULL);

// Fixed frequency buttons next
    int tXPos = 0;
    for (int i = 0; i < NUMBER_OF_FIXED_FREQUENCY_BUTTONS; ++i) {
        (*TouchButtonsFreqGen[i]) = TouchButton::allocAndInitSimpleButton(tXPos,
                BlueDisplay1.getDisplayHeight() - BUTTON_HEIGHT_4 - BUTTON_HEIGHT_5 - BUTTON_HEIGHT_6
                        - (2 * BUTTON_DEFAULT_SPACING), BUTTON_WIDTH_10, BUTTON_HEIGHT_6, COLOR_BLUE, FrequencyStrings[i],
                TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, Frequency[i], &doSetFixedFrequency);
        tXPos += BUTTON_WIDTH_10 + BUTTON_DEFAULT_SPACING_QUARTER;
    }

    // Range next
    tXPos = 0;
    int tYPos = BlueDisplay1.getDisplayHeight() - BUTTON_HEIGHT_4 - BUTTON_HEIGHT_5 - BUTTON_DEFAULT_SPACING;
    TouchButtonmHz = TouchButton::allocAndInitSimpleButton(tXPos, tYPos, BUTTON_WIDTH_4, BUTTON_HEIGHT_5, COLOR_GREEN, "mHz",
            TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doChangeFrequencyFactor);
    tXPos += BUTTON_WIDTH_4 + BUTTON_DEFAULT_SPACING;
    TouchButtonHz = TouchButton::allocAndInitSimpleButton(tXPos, tYPos, BUTTON_WIDTH_4, BUTTON_HEIGHT_5, COLOR_GREEN, "Hz",
            TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 1, &doChangeFrequencyFactor);
    tXPos += BUTTON_WIDTH_4 + BUTTON_DEFAULT_SPACING;
    TouchButtonkHz = TouchButton::allocAndInitSimpleButton(tXPos, tYPos, BUTTON_WIDTH_4, BUTTON_HEIGHT_5, COLOR_GREEN, "kHz",
            TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 2, &doChangeFrequencyFactor);
    tXPos += BUTTON_WIDTH_4 + BUTTON_DEFAULT_SPACING;
    TouchButtonMHz = TouchButton::allocAndInitSimpleButton(tXPos, tYPos, BUTTON_WIDTH_4, BUTTON_HEIGHT_5, COLOR_GREEN, "MHz",
            TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 3, &doChangeFrequencyFactor);

    // Start + Frequency + Home last
    TouchButtonStartStop = TouchButton::allocAndInitSimpleButton(0, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            COLOR_RED, StringStop, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, true, &doFreqGenStartStop);
    TouchButtonStartStop->setRedGreenButtonColor();

    TouchButtonSetFrequency = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
            BUTTON_HEIGHT_4, COLOR_BLUE, "Hz", TEXT_SIZE_33, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doSetFrequency);

    drawFreqGenPage();
    setFrequencyFactor(sFrequencyFactorIndex);
    ComputePeriodAndSetTimer(true);
    HIRES_Timer_Start();
    registerSimpleResizeAndReconnectCallback(&drawFreqGenPage);
}

void loopFreqGenPage(void) {
//	drawInteger(0, BUTTON_HEIGHT_4_LINE_2 + BUTTON_HEIGHT_6 + BUTTON_DEFAULT_SPACING_HALF + TEXT_SIZE_11_HEIGHT, HIRES_Timer_GetCounter(),
//			16, 1, COLOR_BLACK, BACKGROUND_COLOR);
    checkAndHandleEvents();
}

void stopFreqGenPage(void) {
// free buttons
    for (unsigned int i = 0; i < sizeof(TouchButtonsFreqGen) / sizeof(TouchButtonsFreqGen[0]); ++i) {
        (*TouchButtonsFreqGen[i])->setFree();
    }
}

