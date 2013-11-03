/*
 * PageSettins.cpp
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
#include "stm32f30x_it.h"
#include "stm32f30x_rtc.h"

}
#endif

/* Public variables ---------------------------------------------------------*/
unsigned long LastMillis = 0;
unsigned long LoopMillis = 0;

/* Private define ------------------------------------------------------------*/

#define MENU_TOP 15
#define MENU_LEFT 30
#define BACKGROUND_COLOR COLOR_WHITE

#define SET_DATE_STRING_INDEX 4 // index of variable part in StringSetDateCaption
/* Private variables ---------------------------------------------------------*/
char StringSetDateCaption[] = "Set clock  "; // spaces needed for string "minute"

TouchSlider TouchSliderBacklight;
static TouchButton * TouchButtonSetDate;
static TouchButton * TouchButtonTPCalibration;

// for misc testing purposes
TouchButtonAutorepeat * TouchButtonAutorepeatTest_Plus;
TouchButtonAutorepeat * TouchButtonAutorepeatTest_Minus;
void drawTestSettingElements(void);
void deinitTestSettingElements(void);
void initTestSettingElements(void);

TouchButtonAutorepeat * TouchButtonAutorepeatDate_Plus;
TouchButtonAutorepeat * TouchButtonAutorepeatDate_Minus;

TouchButtonAutorepeat * TouchButtonAutorepeatBacklight_Plus;
TouchButtonAutorepeat * TouchButtonAutorepeatBacklight_Minus;

void doTPCalibration(TouchButton * const aTheTouchedButton, int16_t aValue);

char StringTPCal[] = "TP-Calibr.";

void showSettingsPage(void) {
	drawBacklightElements();
	drawClockSettingElements();
	drawTestSettingElements();
	TouchButtonTPCalibration->drawButton();
	// display VDD voltage
	snprintf(StringBuffer, sizeof StringBuffer, "VRef=%1.6f 3V=%d", sVrefVoltage, sReading3Volt);
	drawText(INFO_X_POSITION, 2 + 2 * FONT_HEIGHT, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
	initMainHomeButton(true);
}

void startSettingsPage(void) {
	initBacklightElements();
	initClockSettingElements();
	initTestSettingElements();
	TouchButtonTPCalibration = TouchButton::allocAndInitSimpleButton(2 * (BUTTON_WIDTH_3_POS_2), BUTTON_HEIGHT_4_LINE_3,
			BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_RED, StringTPCal, 1, 0, &doTPCalibration);

	ADC_setRawToVoltFactor();
	showSettingsPage();
}

void loopSettingsPage(void) {
	showRTCTimeEverySecond(RTC_DEFAULT_X, RTC_DEFAULT_Y, RTC_DEFAULT_COLOR, COLOR_BACKGROUND_DEFAULT);
}

void stopSettingsPage(void) {
	TouchButtonTPCalibration->setFree();
	deinitClockSettingElements();
	deinitBacklightElements();
	deinitTestSettingElements();
}

void doTPCalibration(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	//Calibration Button pressed -> calibrate touch panel
	TouchPanel.doCalibration(false);
	showSettingsPage();
}

/*************************************************************
 * RTC and clock setting stuff
 *************************************************************/
uint8_t sSetDateMode;
void doSetDateMode(TouchButton * const aTheTouchedButton, int16_t aValue);
void doSetDate(TouchButton * const aTheTouchedButton, int16_t aValue);

void initClockSettingElements(void) {
	strncpy(&StringSetDateCaption[SET_DATE_STRING_INDEX], DateStrings[0], sizeof StringSecond);

	TouchButtonSetDate = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_3, BUTTON_WIDTH_3,
			BUTTON_HEIGHT_4, COLOR_RED, StringSetDateCaption, 1, 1, &doSetDateMode);
	// for RTC setting
	TouchButtonAutorepeatDate_Plus = TouchButtonAutorepeat::allocButton();
	TouchButtonAutorepeatDate_Plus->initSimpleButton(BUTTON_WIDTH_6_POS_4, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_6, BUTTON_HEIGHT_5,
			COLOR_RED, StringPlus, 2, 1, &doSetDate);

	TouchButtonAutorepeatDate_Minus = TouchButtonAutorepeat::allocButton();
	TouchButtonAutorepeatDate_Minus->initSimpleButton(BUTTON_WIDTH_6_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_6, BUTTON_HEIGHT_5,
			COLOR_RED, StringMinus, 2, -1, &doSetDate);
	sSetDateMode = 0;
}

void deinitClockSettingElements(void) {
	TouchButtonSetDate->setFree();
	TouchButtonAutorepeatDate_Plus->setFree();
	TouchButtonAutorepeatDate_Minus->setFree();
}

void drawClockSettingElements(void) {
	TouchButtonSetDate->drawButton();
	RTC_DateTypeDef RTC_DateStructure;
	RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
	if (RTC_DateStructure.RTC_Year == 0) {
		RTC_DateStructure.RTC_Year = 13;
		RTC_SetDate(RTC_Format_BIN, &RTC_DateStructure);
	}
	sSetDateMode = 0;
}

void doSetDateMode(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	sSetDateMode++;
	if (sSetDateMode == 1) {
		TouchButtonAutorepeatDate_Plus->drawButton();
		TouchButtonAutorepeatDate_Minus->drawButton();
	} else if (sSetDateMode == sizeof DateStrings / sizeof DateStrings[0]) {
		TouchButtonAutorepeatDate_Plus->removeButton(COLOR_BACKGROUND_DEFAULT);
		TouchButtonAutorepeatDate_Minus->removeButton(COLOR_BACKGROUND_DEFAULT);
		sSetDateMode = 0;
	}

	strncpy(&StringSetDateCaption[SET_DATE_STRING_INDEX], DateStrings[sSetDateMode], sizeof StringSecond);
	aTheTouchedButton->setCaption(StringSetDateCaption);
	aTheTouchedButton->drawButton();
}

void doSetDate(TouchButton * const aTheTouchedButton, int16_t aValue) {
	assertParamMessage((sSetDateMode != 0), "Impossible mode", sSetDateMode);
	PWR_BackupAccessCmd(ENABLE);
	if (sSetDateMode < 4) {
		//set time
		RTC_TimeTypeDef RTC_TimeStructure;
		RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
		if (sSetDateMode == 1) {
			RTC_TimeStructure.RTC_Seconds += aValue;
		} else if (sSetDateMode == 2) {
			RTC_TimeStructure.RTC_Minutes += aValue;
		} else if (sSetDateMode == 3) {
			RTC_TimeStructure.RTC_Hours += aValue;
		}
		RTC_SetTime(RTC_Format_BIN, &RTC_TimeStructure);
	} else {
		RTC_DateTypeDef RTC_DateStructure;
		RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
		if (sSetDateMode == 4) {
			RTC_DateStructure.RTC_Date += aValue;
		} else if (sSetDateMode == 5) {
			RTC_DateStructure.RTC_Month += aValue;
		} else if (sSetDateMode == 6) {
			RTC_DateStructure.RTC_Year += aValue;
		}
		RTC_SetDate(RTC_Format_BIN, &RTC_DateStructure);
	}
	PWR_BackupAccessCmd(DISABLE);
	FeedbackToneOK();
	showRTCTime(RTC_DEFAULT_X, RTC_DEFAULT_Y, RTC_DEFAULT_COLOR, COLOR_BACKGROUND_DEFAULT);
}

static uint32_t MillisOfLastDrawRTCTime;
void showRTCTimeEverySecond(uint16_t x, uint16_t y, uint16_t aColor, uint16_t aBackColor) {
// count milliseconds for loop control
	uint32_t tMillis = getMillisSinceBoot() - MillisOfLastDrawRTCTime;
	if (tMillis > 1000) {
		MillisOfLastDrawRTCTime = getMillisSinceBoot();
		//RTC
		RTC_getTimeString(StringBuffer);
		drawText(x, y, StringBuffer, 1, aColor, aBackColor);
	}
}

void showRTCTime(uint16_t x, uint16_t y, uint16_t aColor, uint16_t aBackColor) {
	RTC_getTimeString(StringBuffer);
	drawText(x, y, StringBuffer, 1, aColor, aBackColor);
}

/*************************************************************
 * Backlight stuff
 *************************************************************/
void doChangeBacklight(TouchButton * const aTheTouchedButton, int16_t aValue);
const char * mapBacklightPowerValue(uint16_t aBrightness);

/**
 * create backlight slider and autorepeat buttons
 */
void initBacklightElements(void) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"

	TouchSlider::resetDefaults();

	TouchButtonAutorepeatBacklight_Plus = TouchButtonAutorepeat::allocAndInitSimpleButton(BACKLIGHT_CONTROL_X, BACKLIGHT_CONTROL_Y,
			TOUCHSLIDER_DEFAULT_SIZE * TOUCHSLIDER_OVERALL_SIZE_FACTOR, 2 * FONT_HEIGHT, COLOR_RED, StringPlus, 1, 1,
			&doChangeBacklight);
	/*
	 * Backlight slider
	 */
	TouchSliderBacklight.initSlider(BACKLIGHT_CONTROL_X, TouchButtonAutorepeatBacklight_Plus->getPositionYBottom() + 4,
			TOUCHSLIDER_DEFAULT_SIZE, BACKLIGHT_MAX_VALUE, BACKLIGHT_MAX_VALUE, getBacklightValue(), "Backlight",
			TOUCHSLIDER_DEFAULT_TOUCH_BORDER, TOUCHSLIDER_SHOW_BORDER | TOUCHSLIDER_SHOW_VALUE, &doBacklightSlider,
			&mapBacklightPowerValue);

	TouchButtonAutorepeatBacklight_Minus = TouchButtonAutorepeat::allocAndInitSimpleButton(BACKLIGHT_CONTROL_X,
			TouchSliderBacklight.getPositionYBottom() + 30, TOUCHSLIDER_DEFAULT_SIZE * TOUCHSLIDER_OVERALL_SIZE_FACTOR,
			2 * FONT_HEIGHT, COLOR_RED, StringMinus, 1, -1, &doChangeBacklight);

	TouchButtonAutorepeatBacklight_Plus->setButtonAutorepeatTiming(600, 100, 10, 20);
	TouchButtonAutorepeatBacklight_Minus->setButtonAutorepeatTiming(600, 100, 10, 20);

#pragma GCC diagnostic pop
}

void deinitBacklightElements(void) {
	TouchButtonAutorepeatBacklight_Plus->setFree();
	TouchButtonAutorepeatBacklight_Minus->setFree();
	TouchSliderBacklight.deactivate();
}

void drawBacklightElements(void) {
	TouchButtonAutorepeatBacklight_Plus->drawButton();
	TouchSliderBacklight.drawSlider();
	TouchButtonAutorepeatBacklight_Minus->drawButton();
}

uint16_t doBacklightSlider(TouchSlider * const aTheTouchedSlider, uint16_t aBacklightValue) {
	setBacklightValue(aBacklightValue);
	return getBacklightValue();
}

const char * mapBacklightPowerValue(uint16_t aBrightness) {
	snprintf(StringBuffer, sizeof StringBuffer, "%03u", getBacklightValue());
	return StringBuffer;
}

void setActualBacklightValue(uint8_t aBacklightValue, bool aDoDraw) {
	setBacklightValue(aBacklightValue);
	if (aDoDraw) {
		TouchSliderBacklight.setActualValueAndDraw(getBacklightValue());
	} else {
		TouchSliderBacklight.setActualValue(getBacklightValue());
	}
}

void doChangeBacklight(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	setActualBacklightValue(getBacklightValue() + aValue, true);
}

/*********************************************
 * Test stuff
 *********************************************/
static int testValueIntern = 0;
extern "C" int testValueForExtern = 0x0000;
#define MAX_TEST_VALUE 7
void drawTestvalue(void) {
	snprintf(StringBuffer, sizeof StringBuffer, "MMC SPI_BaudRatePrescaler=%3d", 0x01 << (testValueIntern + 1));
	drawText(70, 2 + 4 * FONT_HEIGHT, StringBuffer, 1, COLOR_BLUE, COLOR_WHITE);
}

void doSetTestvalue(TouchButton * const aTheTouchedButton, int16_t aValue) {
	int tFeedbackType = FEEDBACK_TONE_NO_ERROR;
	int tTestValue = testValueIntern + aValue;
	if (tTestValue < 0) {
		tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
		tTestValue = 0;
	} else if (tTestValue > MAX_TEST_VALUE) {
		tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
		tTestValue = MAX_TEST_VALUE;
	}
	FeedbackTone(tFeedbackType);
	testValueIntern = tTestValue;
	testValueForExtern = testValueIntern << 3;
	drawTestvalue();
}

void initTestSettingElements(void) {
	// for misc  settings
	TouchButtonAutorepeatTest_Plus = TouchButtonAutorepeat::allocButton();
	TouchButtonAutorepeatTest_Plus->initSimpleButton(BUTTON_WIDTH_6_POS_3, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_6, BUTTON_HEIGHT_4,
			COLOR_BLUE, StringPlus, 2, 1, &doSetTestvalue);

	TouchButtonAutorepeatTest_Minus = TouchButtonAutorepeat::allocButton();
	TouchButtonAutorepeatTest_Minus->initSimpleButton(BUTTON_WIDTH_6_POS_4, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_6, BUTTON_HEIGHT_4,
			COLOR_BLUE, StringMinus, 2, -1, &doSetTestvalue);
	sSetDateMode = 0;
	drawTestvalue();
}

void deinitTestSettingElements(void) {
	TouchButtonAutorepeatTest_Plus->setFree();
	TouchButtonAutorepeatTest_Minus->setFree();
}

void drawTestSettingElements(void) {
	TouchButtonAutorepeatTest_Plus->drawButton();
	TouchButtonAutorepeatTest_Minus->drawButton();
}

