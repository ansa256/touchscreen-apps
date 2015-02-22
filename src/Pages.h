/*
 * Pages.h
 *
 * @date 16.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 */

#ifndef PAGES_H_
#define PAGES_H_

#include "BlueDisplay.h"
#include "USART_DMA.h" // for checkAndHandleMessagesReceived()

#include "utils.h"
#include "TouchLib.h"
#include "TouchSlider.h"
#include "TouchButton.h"
#include "TouchButtonAutorepeat.h"
// for NAN in numberpad
#include <math.h>
#include <stdio.h> /* for sprintf */

extern "C" {
#include "timing.h"
#include "stm32f30xPeripherals.h"
}

#endif

/**
 * from MainMenuPage
 */
#define COLOR_BACKGROUND_DEFAULT COLOR_WHITE

#ifdef __cplusplus
extern TouchButton * TouchButtonMainHome;

extern TouchButtonAutorepeat TouchButtonAutorepeatPlus;
extern TouchButtonAutorepeat TouchButtonAutorepeatMinus;

extern TouchSlider TouchSliderVertical;
extern TouchSlider TouchSliderVertical2;
extern TouchSlider TouchSliderHorizontal;
extern TouchSlider TouchSliderHorizontal2;

// for loop timings
extern uint32_t MillisLastLoop;
extern uint32_t MillisSinceLastAction;

void initMainMenuPage(void);
void startMainMenuPage(void);
void loopMainMenuPage(void);
void stopMainMenuPage(void);
void doMainMenuHomeButton(TouchButton * const aTheTouchedButton, int16_t aValue);
void backToMainMenu(void);
void doClearScreen(TouchButton * const aTheTouchedButton, int16_t aValue);
void initMainHomeButtonWithPosition(const uint16_t aPositionX, const uint16_t aPositionY, bool doDraw);
void initMainHomeButton(bool doDraw);

#define COLOR_PAGE_INFO COLOR_RED

#ifdef LOCAL_DISPLAY_EXISTS
/**
 * from SettingsPage
 */
extern TouchSlider TouchSliderBacklight;

#define BACKLIGHT_CONTROL_X 30
#define BACKLIGHT_CONTROL_Y 4

void initBacklightElements(void);
void deinitBacklightElements(void);
void drawBacklightElements(void);
uint16_t doBacklightSlider(TouchSlider * const aTheTouchedSlider, const uint16_t aBrightness);
uint8_t getBacklightValue(void);
void setBacklightValue(uint8_t aBacklightValue);
#endif

void initClockSettingElements(void);
void deinitClockSettingElements(void);
void drawClockSettingElements(void);

void startSettingsPage(void);
void loopSettingsPage(void);
void stopSettingsPage(void);

/**
 * from  numberpad page
 */
#define NUMBERPAD_DEFAULT_X 60
float getNumberFromNumberPad(uint16_t aXStart, uint16_t aYStart, uint16_t aButtonColor);

/**
 * From DAC page
 */
void initDACPage(void);
void startDACPage(void);
void loopDACPage(void);
void stopDACPage(void);

/**
 * From FrequencyGenerator page
 */
void initFreqGenPage(void);
void startFreqGenPage(void);
void loopFreqGenPage(void);
void stopFreqGenPage(void);

/**
 * From IR page
 */
void startIRPage(void);
void loopIRPage(void);
void stopIRPage(void);

/**
 * From PageAcceleratorCompassDemo
 */
void initAcceleratorCompass(void);
void startAcceleratorCompass(void);
void loopAcceleratorGyroCompassDemo(void);
void stopAcceleratorCompass(void);

/**
 * From BobsDemo
 */
void initBobsDemo(void);
void startBobsDemo(void);
void loopBobsDemo(void);
void stopBobsDemo(void);

/**
 * From Tests page
 */
void initInfoPage(void);
void startTestsPage(void);
void loopTestsPage(void);
void stopTestsPage(void);

/**
 * From Info page
 */
void initInfoPage(void);
void startInfoPage(void);
void loopInfoPage(void);
void stopInfoPage(void);

/**
 * From Draw page
 */
void startDrawPage(void);
void loopDrawPage(void);
void stopDrawPage(void);

#ifdef __cplusplus
extern "C" {
#endif
void setActualBacklightValue(uint8_t aBacklightValue, bool aDoDraw);
#ifdef __cplusplus
}
#endif

#endif /* PAGES_H_ */
