/*
 * Pages.h
 *
 * @date 16.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 */

#ifndef PAGES_H_
#define PAGES_H_

#include "HY32D.h"
#ifdef __cplusplus
#include "ADS7846.h"
#include "TouchSlider.h"
#include "TouchButton.h"
#include "TouchButtonAutorepeat.h"
// for NAN in numberpad
#include <math.h>
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

/*
 * for loop timings
 */
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

/**
 * from SettingsPage
 */
extern TouchSlider TouchSliderBacklight;
// Default screen position for time string
#define RTC_DEFAULT_X 2
#define RTC_DEFAULT_Y (DISPLAY_HEIGHT - FONT_HEIGHT - 1)
#define RTC_DEFAULT_COLOR COLOR_RED

#define BACKLIGHT_CONTROL_X 30
#define BACKLIGHT_CONTROL_Y 4
#define INFO_X_POSITION 100
#define COLOR_PAGE_INFO COLOR_RED

void initBacklightElements(void);
void deinitBacklightElements(void);
void drawBacklightElements(void);
uint16_t doBacklightSlider(TouchSlider * const aTheTouchedSlider, const uint16_t aBrightness);
uint8_t getBacklightValue(void);
void setBacklightValue(uint8_t aBacklightValue);

void initClockSettingElements(void);
void deinitClockSettingElements(void);
void drawClockSettingElements(void);

void startSettingsPage(void);
void loopSettingsPage(void);
void stopSettingsPage(void);

void showRTCTime(uint16_t x, uint16_t y, uint16_t aColor, uint16_t aBackColor);
void showRTCTimeEverySecond(uint16_t x, uint16_t y, uint16_t aColor, uint16_t aBackColor);

void setActualBacklightValue(uint8_t aBacklightValue, bool aDoDraw);
extern "C" uint8_t getActualBacklightValue(void);

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
void stopDACPage(void);

/**
 * From FrequencyGenerator page
 */
void initFreqGenPage(void);
void startFreqGenPage(void);
void loopFreqGenPage(void);
void stopFreqGenPage(void);
/**
 * From PageAcceleratorCompassDemo
 */
void initAcceleratorCompass(void);
void startAcceleratorCompass(void);
void loopAcceleratorGyroCompassDemo(void);
void stopAcceleratorCompass(void);
void readAcceleratorRaw(int16_t* aAcceleratorRawData);
void readAcceleratorZeroCompensated(int16_t* aAcceleratorRawData);
void readGyroscopeRaw(int16_t* aGyroscopeRawData);
void readGyroscopeZeroCompensated(int16_t* aGyroscopeRawData);
void setZeroAcceleratorGyroValue(void);

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
void initTestsPage(void);
void startTestsPage(void);
void loopTestsPage(int aGuiToucheState);
void stopTestsPage(void);

extern int DebugValue1;
extern int DebugValue2;
extern int DebugValue3;
extern int DebugValue4;
extern int DebugValue5;

/**
 * From Draw page
 */
void startDrawPage(void);
void loopDrawPage(int aGuiTouchState);
void stopDrawPage(void);

#else //cplusplus
// for timing.c
void setActualBacklightValue(uint8_t aBacklightValue, bool aDoDraw);
uint8_t getActualBacklightValue(void);
#endif

#endif /* PAGES_H_ */
