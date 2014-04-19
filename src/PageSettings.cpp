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
#include "myprint.h"

#include <string.h>

extern "C" {
#include "usb_misc.h"
#include "usb_pwr.h"
#include "stm32f30x_it.h"
#include "stm32f30x_rtc.h"
}

const char StringPrint[] = "Print";

// date strings
const char StringClock[] = "clock";
const char StringYear[] = "year";
const char StringMonth[] = "month";
const char StringDay[] = "day";
const char StringHour[] = "hour";
const char StringMinute[] = "minute";
const char String_min[] = "min";
const char StringSecond[] = "second";
const char * const DateStrings[] = { StringClock, StringSecond, StringMinute, StringHour, StringDay, StringMonth, StringYear };

/* Public variables ---------------------------------------------------------*/
unsigned long LastMillis = 0;
unsigned long LoopMillis = 0;

/* Private define ------------------------------------------------------------*/

#define MENU_TOP 15
#define MENU_LEFT 30
#define BACKGROUND_COLOR COLOR_WHITE

#define PRINT_MODE_DISABLED 0 //false
#define PRINT_MODE_ENABLED_SCREEN 1 //true
#define PRINT_MODE_ENABLED_USB 2
static uint8_t sPrintMode = PRINT_MODE_DISABLED;

#define SET_DATE_STRING_INDEX 4 // index of variable part in StringSetDateCaption
/* Private variables ---------------------------------------------------------*/
char StringSetDateCaption[] = "Set clock  "; // spaces needed for string "minute"

TouchSlider TouchSliderBacklight;
static TouchButton * TouchButtonTogglePrintMode;
static TouchButton * TouchButtonToggleTouchXYDisplay;
static TouchButton * TouchButtonSetDate;
static TouchButton * TouchButtonTPCalibration;

// for misc testing purposes

void doTogglePrintEnable(TouchButton * const aTheTouchedButton, int16_t aValue);
void doToggleTouchXYDisplay(TouchButton * const aTheTouchedButton, int16_t aValue);

TouchButtonAutorepeat * TouchButtonAutorepeatDate_Plus;
TouchButtonAutorepeat * TouchButtonAutorepeatDate_Minus;

TouchButtonAutorepeat * TouchButtonAutorepeatBacklight_Plus;
TouchButtonAutorepeat * TouchButtonAutorepeatBacklight_Minus;

void doTPCalibration(TouchButton * const aTheTouchedButton, int16_t aValue);

char StringTPCal[] = "TP-Calibr.";

/*********************************************
 * Print stuff
 *********************************************/

void setPrintCaptionAndColor(int aPrintMode) {
    if (aPrintMode == PRINT_MODE_ENABLED_USB) {
        TouchButtonTogglePrintMode->setCaption("USB");
        TouchButtonTogglePrintMode->setColor(COLOR_YELLOW);
        TouchButtonTogglePrintMode->drawButton();
    } else {
        TouchButtonTogglePrintMode->setCaption(StringPrint);
        TouchButtonTogglePrintMode->setRedGreenButtonColorAndDraw(aPrintMode);
    }
}

/**
 * @param aTheTouchedButton
 * @param aValue 0,1,2=USB Mode
 */
void doTogglePrintEnable(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();

    aValue++;
    if (aValue == PRINT_MODE_ENABLED_SCREEN) {
        setPrintStatus(aValue);
    } else if (aValue == PRINT_MODE_ENABLED_USB) {
        USB_ChangeToCDC();
        aTheTouchedButton->setValue(aValue);
    } else {
        // USB Mode -> No print
        aValue = PRINT_MODE_DISABLED;
        USB_PowerOff();
        setPrintStatus(aValue);
    }
    setPrintCaptionAndColor(aValue);
    sPrintMode = aValue;
}

void showSettingsPage(void) {
    drawBacklightElements();
    drawClockSettingElements();
    TouchButtonTPCalibration->drawButton();
    TouchButtonTogglePrintMode->drawButton();
    TouchButtonToggleTouchXYDisplay->drawButton();
    // display VDD voltage
    snprintf(StringBuffer, sizeof StringBuffer, "VRef=%1.6f", sVrefVoltage);
    drawText(2, DISPLAY_HEIGHT - 3 * FONT_HEIGHT, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
    snprintf(StringBuffer, sizeof StringBuffer, "3V=%d", sReading3Volt);
    drawText(2, DISPLAY_HEIGHT - 2 * FONT_HEIGHT, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
    initMainHomeButton(true);
}

void startSettingsPage(void) {
    initBacklightElements();
    initClockSettingElements();

    //1. row
    int tPosY = 0;
    TouchButtonToggleTouchXYDisplay = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
            BUTTON_HEIGHT_4, COLOR_BLACK, "Touch X Y", 1, TouchPanel.getDisplayXYValuesFlag(), &doToggleTouchXYDisplay);
    TouchButtonToggleTouchXYDisplay->setRedGreenButtonColorAndDraw();

    //2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonTogglePrintMode = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
            BUTTON_HEIGHT_4, COLOR_BLACK, StringPrint, 2, sPrintMode, &doTogglePrintEnable);
    setPrintCaptionAndColor(sPrintMode);

    TouchButtonTPCalibration = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3,
            BUTTON_HEIGHT_4, COLOR_RED, StringTPCal, 1, 0, &doTPCalibration);

    //3. row

    ADC_setRawToVoltFactor();
    showSettingsPage();
}

void loopSettingsPage(void) {
    showRTCTimeEverySecond(RTC_DEFAULT_X, RTC_DEFAULT_Y, RTC_DEFAULT_COLOR, COLOR_BACKGROUND_DEFAULT);
}

void stopSettingsPage(void) {
    TouchButtonTPCalibration->setFree();
    TouchButtonTogglePrintMode->setFree();
    TouchButtonToggleTouchXYDisplay->setFree();
    deinitClockSettingElements();
    deinitBacklightElements();
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

    TouchButtonAutorepeatDate_Plus->setButtonAutorepeatTiming(500, 300, 5, 100);
    TouchButtonAutorepeatDate_Minus->setButtonAutorepeatTiming(500, 300, 5, 100);
    sSetDateMode = 0;
}

void deinitClockSettingElements(void) {
    TouchButtonSetDate->setFree();
    TouchButtonAutorepeatDate_Plus->setFree();
    TouchButtonAutorepeatDate_Minus->setFree();
}

void drawClockSettingElements(void) {
    TouchButtonSetDate->drawButton();
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

uint8_t sDateTimeMaxValues[] = { 59, 59, 23, 31, 12, 99 };
void checkAndSetDateTimeElement(uint8_t * aElementPointer, uint8_t SetDateMode, int16_t aValue) {
    SetDateMode--;
    uint8_t tNewValue = *aElementPointer + aValue;
    uint8_t tMaxValue = sDateTimeMaxValues[SetDateMode];
    if (tNewValue == __UINT8_MAX__) {
        // underflow
        tNewValue = tMaxValue;
    } else if (tNewValue > tMaxValue) {
        //overflow
        tNewValue = 0;
    }
    if ((SetDateMode == 3 || SetDateMode == 4)) {
        if (tNewValue == 0) {
            // no 0th day and month
            if (aValue > 0) {
                // increment
                tNewValue++;
            } else {
                // decrement from 1 to zero -> set to max
                tNewValue = tMaxValue;
            }
        }

    }
    *aElementPointer = tNewValue;
}

void doSetDate(TouchButton * const aTheTouchedButton, int16_t aValue) {
    assertParamMessage((sSetDateMode != 0), sSetDateMode, "Impossible mode");
    PWR_BackupAccessCmd(ENABLE);
    uint8_t * tElementPointer;
    if (sSetDateMode < 4) {
        //set time
        RTC_TimeTypeDef RTC_TimeStructure;
        RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
        if (sSetDateMode == 1) {
            tElementPointer = &RTC_TimeStructure.RTC_Seconds;
        } else if (sSetDateMode == 2) {
            tElementPointer = &RTC_TimeStructure.RTC_Minutes;
        } else {
            tElementPointer = &RTC_TimeStructure.RTC_Hours;
        }
        checkAndSetDateTimeElement(tElementPointer, sSetDateMode, aValue);
        RTC_SetTime(RTC_Format_BIN, &RTC_TimeStructure);
    } else {
        RTC_DateTypeDef RTC_DateStructure;
        RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
        if (sSetDateMode == 4) {
            tElementPointer = &RTC_DateStructure.RTC_Date;
        } else if (sSetDateMode == 5) {
            tElementPointer = &RTC_DateStructure.RTC_Month;
        } else {
            tElementPointer = &RTC_DateStructure.RTC_Year;
        }
        checkAndSetDateTimeElement(tElementPointer, sSetDateMode, aValue);
        RTC_SetDate(RTC_Format_BIN, &RTC_DateStructure);
    }
    PWR_BackupAccessCmd(DISABLE);
    FeedbackToneOK();
    showRTCTime(RTC_DEFAULT_X, RTC_DEFAULT_Y, RTC_DEFAULT_COLOR, COLOR_BACKGROUND_DEFAULT, true);
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

uint16_t doBacklightSlider(TouchSlider * const aTheTouchedSlider, uint16_t aValue) {
    setBacklightValue(aValue);
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

/**
 *
 * @param aTheTouchedButton
 * @param aValue assume as boolean here
 */
void doToggleTouchXYDisplay(TouchButton * const aTheTouchedButton, int16_t aValue) {
    TouchPanel.setDisplayXYValuesFlag(!aValue);
    doToggleRedGreenButton(aTheTouchedButton, aValue);
}

