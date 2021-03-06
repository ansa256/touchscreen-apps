/*
 * PageNumberPad.cpp
 *
 * @date 26.02.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 */

#include "Pages.h"
#include "misc.h"
#include <string.h>
#include <stdlib.h>  //strtod
#include <locale.h>

const char String0[] = "0";
const char String1[] = "1";
const char String2[] = "2";
const char String3[] = "3";
const char String4[] = "4";
const char String5[] = "5";
const char String6[] = "6";
const char String7[] = "7";
const char String8[] = "8";
const char String9[] = "9";
const char StringEnterChar[] = "\xD6";
const char StringCancel[] = "Cancel";

static TouchButton * TouchButtonNumberPad0;
static TouchButton * TouchButtonNumberPad1;
static TouchButton * TouchButtonNumberPad2;
static TouchButton * TouchButtonNumberPad3;
static TouchButton * TouchButtonNumberPad4;
static TouchButton * TouchButtonNumberPad5;
static TouchButton * TouchButtonNumberPad6;
static TouchButton * TouchButtonNumberPad7;
static TouchButton * TouchButtonNumberPad8;
static TouchButton * TouchButtonNumberPad9;
static TouchButton * TouchButtonNumberPadDecimalSeparator;
static TouchButton * TouchButtonNumberPadBack;
static TouchButton * TouchButtonNumberPadClear;
static TouchButton * TouchButtonNumberPadSign;
static TouchButton * TouchButtonNumberPadEnter;
static TouchButton * TouchButtonNumberPadCancel;
static TouchButton ** TouchButtonsNumberPad[] = { &TouchButtonNumberPad7, &TouchButtonNumberPad8, &TouchButtonNumberPad9,
        &TouchButtonNumberPadBack, &TouchButtonNumberPad4, &TouchButtonNumberPad5, &TouchButtonNumberPad6,
        &TouchButtonNumberPadClear, &TouchButtonNumberPad1, &TouchButtonNumberPad2, &TouchButtonNumberPad3,
        &TouchButtonNumberPadEnter, &TouchButtonNumberPad0, &TouchButtonNumberPadSign, &TouchButtonNumberPadDecimalSeparator,
        &TouchButtonNumberPadCancel };

/*************************************************************
 * Numberpad stuff
 *************************************************************/
void drawNumberPadValue(char * aNumberPadBuffer);
void doNumberPad(TouchButton * const aTheTouchedButton, int16_t aValue);

// for Touch numberpad
#define SIZEOF_NUMBERPADBUFFER 9
#define NUMBERPADBUFFER_INDEX_LAST_CHAR (SIZEOF_NUMBERPADBUFFER - 2)
extern char NumberPadBuffer[SIZEOF_NUMBERPADBUFFER];
char NumberPadBuffer[SIZEOF_NUMBERPADBUFFER];
int sNumberPadBufferSignIndex;  // Main pointer to NumberPadBuffer

static volatile bool numberpadInputHasFinished;
static volatile bool numberpadInputHasCanceled;

/**
 * allocates and draws the buttons
 */
void drawNumberPad(uint16_t aXStart, uint16_t aYStart, uint16_t aButtonColor) {
    aYStart += (3 * TEXT_SIZE_11_HEIGHT);
    TouchButton::setDefaultButtonColor(aButtonColor);
    TouchButtonNumberPad7 = TouchButton::allocAndInitSimpleButton(aXStart, aYStart, BUTTON_WIDTH_6, BUTTON_HEIGHT_6, 0, String7,
            TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0x37, &doNumberPad);
    TouchButtonNumberPad8 = TouchButton::allocAndInitSimpleButton(aXStart + BUTTON_WIDTH_6_POS_2, aYStart, BUTTON_WIDTH_6,
            BUTTON_HEIGHT_6, 0, String8, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0x38, &doNumberPad);
    TouchButtonNumberPad9 = TouchButton::allocAndInitSimpleButton(aXStart + BUTTON_WIDTH_6_POS_3, aYStart, BUTTON_WIDTH_6,
            BUTTON_HEIGHT_6, 0, String9, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0x39, &doNumberPad);
    TouchButtonNumberPadBack = TouchButton::allocAndInitSimpleButton(aXStart + BUTTON_WIDTH_6_POS_4, aYStart, BUTTON_WIDTH_6,
            BUTTON_HEIGHT_6, 0, StringLessThan, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0x60, &doNumberPad);

    aYStart += BUTTON_HEIGHT_6 + BUTTON_DEFAULT_SPACING;
    TouchButtonNumberPad4 = TouchButton::allocAndInitSimpleButton(aXStart, aYStart, BUTTON_WIDTH_6, BUTTON_HEIGHT_6, 0, String4,
            TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0x34, &doNumberPad);
    TouchButtonNumberPad5 = TouchButton::allocAndInitSimpleButton(aXStart + BUTTON_WIDTH_6_POS_2, aYStart, BUTTON_WIDTH_6,
            BUTTON_HEIGHT_6, 0, String5, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0x35, &doNumberPad);
    TouchButtonNumberPad6 = TouchButton::allocAndInitSimpleButton(aXStart + BUTTON_WIDTH_6_POS_3, aYStart, BUTTON_WIDTH_6,
            BUTTON_HEIGHT_6, 0, String6, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0x36, &doNumberPad);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
    TouchButtonNumberPadClear = TouchButton::allocAndInitSimpleButton(aXStart + BUTTON_WIDTH_6_POS_4, aYStart, BUTTON_WIDTH_6,
            BUTTON_HEIGHT_6, 0, "C", TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0x43, &doNumberPad);
#pragma GCC diagnostic pop
    aYStart += BUTTON_HEIGHT_6 + BUTTON_DEFAULT_SPACING;
    TouchButtonNumberPad1 = TouchButton::allocAndInitSimpleButton(aXStart, aYStart, BUTTON_WIDTH_6, BUTTON_HEIGHT_6, 0, String1,
            TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0x31, &doNumberPad);
    TouchButtonNumberPad2 = TouchButton::allocAndInitSimpleButton(aXStart + BUTTON_WIDTH_6_POS_2, aYStart, BUTTON_WIDTH_6,
            BUTTON_HEIGHT_6, 0, String2, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0x32, &doNumberPad);
    TouchButtonNumberPad3 = TouchButton::allocAndInitSimpleButton(aXStart + BUTTON_WIDTH_6_POS_3, aYStart, BUTTON_WIDTH_6,
            BUTTON_HEIGHT_6, 0, String3, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0x33, &doNumberPad);
    //Enter Button
    // size is 2 * BUTTON_WIDTH_6 or less if not enough space on screen
    int tWidth = 2 * BUTTON_WIDTH_6;
    if ((tWidth + aXStart + BUTTON_WIDTH_6_POS_4)> BlueDisplay1.getDisplayWidth()) {
        tWidth = BlueDisplay1.getDisplayWidth() - (aXStart + BUTTON_WIDTH_6_POS_4);
    }
    TouchButtonNumberPadEnter = TouchButton::allocAndInitSimpleButton(aXStart + BUTTON_WIDTH_6_POS_4, aYStart, tWidth,
            (2 * BUTTON_HEIGHT_6) + BUTTON_DEFAULT_SPACING, 0, StringEnterChar, 44, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0x0D,
            &doNumberPad);

    aYStart += BUTTON_HEIGHT_6 + BUTTON_DEFAULT_SPACING;
    TouchButtonNumberPad0 = TouchButton::allocAndInitSimpleButton(aXStart, aYStart, BUTTON_WIDTH_6, BUTTON_HEIGHT_6, 0, String0,
            TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0x30, &doNumberPad);
    TouchButtonNumberPadSign = TouchButton::allocAndInitSimpleButton(aXStart + BUTTON_WIDTH_6_POS_2, aYStart, BUTTON_WIDTH_6,
            BUTTON_HEIGHT_6, 0, StringPlusMinus, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0xF3, &doNumberPad);
    struct lconv * tLconfPtr = localeconv();
    TouchButtonNumberPadDecimalSeparator = TouchButton::allocAndInitSimpleButton(aXStart + BUTTON_WIDTH_6_POS_3, aYStart,
            BUTTON_WIDTH_6, BUTTON_HEIGHT_6, 0, tLconfPtr->decimal_point, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH,
            tLconfPtr->decimal_point[0], &doNumberPad);
    TouchButtonNumberPadCancel = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
            BUTTON_HEIGHT_4, 0, StringCancel, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0xFF, &doNumberPad);
    for (unsigned int i = 0; i < sizeof(TouchButtonsNumberPad) / sizeof(TouchButtonsNumberPad[0]); ++i) {
        (*TouchButtonsNumberPad[i])->drawButton();
    }
}

void drawNumberPadValue(char * aNumberPadBuffer) {
    BlueDisplay1.drawText(TouchButtonNumberPad7->getPositionX(),
            TouchButtonNumberPad7->getPositionY() - (3 * TEXT_SIZE_11_HEIGHT) + TEXT_SIZE_22_ASCEND, aNumberPadBuffer, TEXT_SIZE_22,
            COLOR_PAGE_INFO, COLOR_WHITE);
}

/**
 * set buffer to 0 and display it
 */
void clearNumberPadBuffer(void) {
    for (int i = 0; i <= NUMBERPADBUFFER_INDEX_LAST_CHAR - 1; ++i) {
        NumberPadBuffer[i] = 0x20;
    }
    NumberPadBuffer[NUMBERPADBUFFER_INDEX_LAST_CHAR] = 0x30;
    NumberPadBuffer[SIZEOF_NUMBERPADBUFFER - 1] = 0x00;
    sNumberPadBufferSignIndex = NUMBERPADBUFFER_INDEX_LAST_CHAR;
    drawNumberPadValue(NumberPadBuffer);
}

void doNumberPad(TouchButton * const aTheTouchedButton, int16_t aValue) {
    uint8_t tFeedbackType = FEEDBACK_TONE_NO_ERROR;
    if (aValue == 0x60) { // "<"
        if (sNumberPadBufferSignIndex < NUMBERPADBUFFER_INDEX_LAST_CHAR) {
            // copy all chars one right
            for (int i = SIZEOF_NUMBERPADBUFFER - 2; i > 0; i--) {
                NumberPadBuffer[i] = NumberPadBuffer[i - 1];
            }
            NumberPadBuffer[sNumberPadBufferSignIndex++] = 0x20;
        } else {
            tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
        }
    } else if (aValue == 0xF3) { // Change sign
        if (NumberPadBuffer[sNumberPadBufferSignIndex] == 0x2D) {
            // clear minus sign
            NumberPadBuffer[sNumberPadBufferSignIndex] = 0x20;
        } else {
            NumberPadBuffer[sNumberPadBufferSignIndex] = 0x2D;
        }
    } else if (aValue == 0x43) { // Clear "C"
        clearNumberPadBuffer();
    } else if (aValue == 0x0D) { // Enter
        numberpadInputHasFinished = true;
    } else if (aValue == 0xFF) { // Cancel
        numberpadInputHasFinished = true;
        numberpadInputHasCanceled = true;
    } else {
        // plain numbers and decimal separator
        if (sNumberPadBufferSignIndex > 0) {
            if (sNumberPadBufferSignIndex < NUMBERPADBUFFER_INDEX_LAST_CHAR) {
                // copy all chars one left
                for (int i = 0; i < NUMBERPADBUFFER_INDEX_LAST_CHAR; ++i) {
                    NumberPadBuffer[i] = NumberPadBuffer[i + 1];
                }
            }
            sNumberPadBufferSignIndex--;
            // set number at last (rightmost) position
            NumberPadBuffer[NUMBERPADBUFFER_INDEX_LAST_CHAR] = aValue;
        } else {
            tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
        }
    }
    FeedbackTone(tFeedbackType);
    drawNumberPadValue(NumberPadBuffer);
}

/**
 * clears display, shows numberpad and returns with a float
 */
float getNumberFromNumberPad(uint16_t aXStart, uint16_t aYStart, uint16_t aButtonColor) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    TouchButton::deactivateAllButtons();
    // disable temporarily the end callback function
    setTouchUpCallbackEnabled(false);
    drawNumberPad(aXStart, aYStart, aButtonColor);
    clearNumberPadBuffer();
    numberpadInputHasFinished = false;
    numberpadInputHasCanceled = false;
    while (!numberpadInputHasFinished) {
        checkAndHandleEvents();
    }
    // free numberpad buttons
    for (unsigned int i = 0; i < sizeof(TouchButtonsNumberPad) / sizeof(TouchButtonsNumberPad[0]); ++i) {
        (*TouchButtonsNumberPad[i])->setFree();
    }
    // to avoid end touch handling of releasing OK or Cancel button of numberpad
    sDisableTouchUpOnce = true;
    setTouchUpCallbackEnabled(true);
    if (numberpadInputHasCanceled) {
        return NAN;
    } else {
        return strtod(NumberPadBuffer, 0);
    }
}
