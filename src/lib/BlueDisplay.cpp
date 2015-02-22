/*
 * BlueDisplay.cpp
 * C stub for Android BlueDisplay app and the local MI0283QT2 Display from Watterott.
 * It implements a few display test functions.
 *
 *  Created on: 12.09.2014
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 *      License: LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 *
 */
#include "BlueDisplay.h"

#include "thickLine.h"
#include "USART_DMA.h"

#include <string.h>  // for strlen
extern "C" {
#include "stm32f30xPeripherals.h"
#include "timing.h"
}

const int FUNCTION_TAG_GLOBAL_SETTINGS = 0x08;
// Sub functions for GLOBAL_SETTINGS
const int SET_FLAGS_AND_SIZE = 0x00;
const int SET_CODEPAGE = 0x01;
const int SET_CHARACTER_CODE_MAPPING = 0x02;
const int SET_LONG_TOUCH_DOWN_TIMEOUT = 0x08;
// Sub functions for SET_FLAGS_AND_SIZE
const int BD_FLAG_FIRST_RESET_ALL = 0x01;
const int BD_FLAG_TOUCH_BASIC_DISABLE = 0x02;
const int BD_FLAG_TOUCH_MOVE_DISABLE = 0x04;
const int BD_FLAG_LONG_TOUCH_ENABLE = 0x08;
const int BD_FLAG_USE_MAX_SIZE = 0x10;

/*
 * Miscellaneous functions
 */
const int FUNCTION_TAG_GET_NUMBER = 0x0C;
const int FUNCTION_TAG_GET_TEXT = 0x0D;
const int FUNCTION_TAG_PLAY_TONE = 0x0E;

/*
 * Display functions
 */
const int FUNCTION_TAG_CLEAR_DISPLAY = 0x10;
// 3 parameter
const int FUNCTION_TAG_DRAW_PIXEL = 0x14;
// 6 parameter
const int FUNCTION_TAG_DRAW_CHAR = 0x16;
// 5 parameter
const int FUNCTION_TAG_DRAW_LINE_REL = 0x20;
const int FUNCTION_TAG_DRAW_LINE = 0x21;
const int FUNCTION_TAG_DRAW_RECT_REL = 0x24;
const int FUNCTION_TAG_FILL_RECT_REL = 0x25;
const int FUNCTION_TAG_DRAW_RECT = 0x26;
const int FUNCTION_TAG_FILL_RECT = 0x27;

const int FUNCTION_TAG_DRAW_CIRCLE = 0x28;
const int FUNCTION_TAG_FILL_CIRCLE = 0x29;

const int LAST_FUNCTION_TAG_WITHOUT_DATA = 0x5F;

// Function with variable data size
const int FUNCTION_TAG_DRAW_STRING = 0x60;
const int FUNCTION_TAG_DEBUG_STRING = 0x61;

const int FUNCTION_TAG_GET_NUMBER_WITH_SHORT_PROMPT = 0x64;

const int FUNCTION_TAG_DRAW_PATH = 0x68;
const int FUNCTION_TAG_FILL_PATH = 0x69;
const int FUNCTION_TAG_DRAW_CHART = 0x6A;

/*
 * Button functions
 */
const int FUNCTION_TAG_BUTTON_DRAW = 0x40;
const int FUNCTION_TAG_BUTTON_DRAW_CAPTION = 0x41;
const int FUNCTION_TAG_BUTTON_SETTINGS = 0x42;
const int FUNCTION_TAG_BUTTON_SET_COLOR_AND_VALUE_AND_DRAW = 0x43;

// static functions
const int FUNCTION_TAG_BUTTON_ACTIVATE_ALL = 0x48;
const int FUNCTION_TAG_BUTTON_DEACTIVATE_ALL = 0x49;
const int FUNCTION_TAG_BUTTON_GLOBAL_SETTINGS = 0x4A;

// Function with variable data size
const int FUNCTION_TAG_BUTTON_CREATE = 0x70;
const int FUNCTION_TAG_BUTTON_CREATE_32 = 0x71;
const int FUNCTION_TAG_BUTTON_SET_CAPTION = 0x72;
const int FUNCTION_TAG_BUTTON_SET_CAPTION_AND_DRAW_BUTTON = 0x73;

// Flags for BUTTON_SETTINGS
const int BUTTON_FLAG_SET_COLOR_BUTTON = 0x00;
const int BUTTON_FLAG_SET_COLOR_CAPTION = 0x01;
const int BUTTON_FLAG_SET_VALUE = 0x03;
const int BUTTON_FLAG_SET_POSITION = 0x04;
const int BUTTON_FLAG_SET_ACTIVE = 0x05;
const int BUTTON_FLAG_RESET_ACTIVE = 0x06;

// Flags for BUTTON_GLOBAL_SETTINGS
const int USE_UP_EVENTS_FOR_BUTTONS = 0x01;

// Values for global button flag
const int BUTTON_FLAG_DO_BEEP_ON_TOUCH = 0x01;
const int BUTTONS_SET_BEEP_TONE = 0x02;
const int BUTTONS_SET_BEEP_TONE_VOLUME_FOR_NO_VOLUME_CHANGE = 0xFF;

// no valid button number
#define NO_BUTTON 0xFF
#define NO_SLIDER 0xFF

/*
 * Slider functions
 */
const int FUNCTION_TAG_SLIDER_CREATE = 0x50;
const int FUNCTION_TAG_SLIDER_DRAW = 0x51;
const int FUNCTION_TAG_SLIDER_SETTINGS = 0x52;
const int FUNCTION_TAG_SLIDER_DRAW_BORDER = 0x53;

// Flags for SLIDER_SETTINGS
const int SLIDER_FLAG_SET_COLOR_THRESHOLD = 0x00;
const int SLIDER_FLAG_SET_COLOR_BAR_BACKGROUND = 0x01;
const int SLIDER_FLAG_SET_COLOR_BAR = 0x02;
const int SLIDER_FLAG_SET_VALUE_AND_DRAW_BAR = 0x03;
const int SLIDER_FLAG_SET_POSITION = 0x04;
const int SLIDER_FLAG_SET_ACTIVE = 0x05;
const int SLIDER_FLAG_RESET_ACTIVE = 0x06;

// static slider functions
const int FUNCTION_TAG_SLIDER_ACTIVATE_ALL = 0x58;
const int FUNCTION_TAG_SLIDER_DEACTIVATE_ALL = 0x59;
const int FUNCTION_TAG_SLIDER_GLOBAL_SETTINGS = 0x5A;

// Flags for slider options
const int TOUCHSLIDER_SHOW_BORDER = 0x01;
const int TOUCHSLIDER_VALUE_BY_CALLBACK = 0x02; // if set value will be set by callback handler
const int TOUCHSLIDER_IS_HORIZONTAL = 0x04;

//-------------------- Constructor --------------------

BlueDisplay::BlueDisplay(void) {
    mReferenceDisplaySize.XWidth = DISPLAY_DEFAULT_WIDTH;
    mReferenceDisplaySize.YHeight = DISPLAY_DEFAULT_HEIGHT;
    return;
}

// One instance of BlueDisplay called BlueDisplay1
BlueDisplay BlueDisplay1;

bool isDisplayAvailable = false;

void BlueDisplay::setFlags(uint16_t aFlags) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_SET_FLAGS, 1, aFlags);
    }
}

void BlueDisplay::setFlagsAndSize(uint16_t aFlags, uint16_t aWidth, uint16_t aHeight) {
    mReferenceDisplaySize.XWidth = aWidth;
    mReferenceDisplaySize.YHeight = aHeight;
    if (USART_isBluetoothPaired()) {
        if (aFlags & BD_FLAG_FIRST_RESET_ALL) {
            // reset local buttons to be synchronized
            BlueDisplay1.resetAllButtons();
            BlueDisplay1.resetAllSliders();
        }
        sendUSARTArgs(FUNCTION_TAG_GLOBAL_SETTINGS, 4, SET_FLAGS_AND_SIZE, aFlags, aWidth, aHeight);
    }
}

/*
 * index is from android.media.ToneGenerator see also
 * http://www.syakazuka.com/Myself/android/sound_test.html
 */
void BlueDisplay::playTone(uint8_t aToneIndex) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_PLAY_TONE, 1, aToneIndex);
    }
}

void BlueDisplay::playTone(void) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_PLAY_TONE, 1, TONE_DEFAULT);
    }
}

void BlueDisplay::setCharacterMapping(uint8_t aChar, uint16_t aUnicodeChar) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_GLOBAL_SETTINGS, 3, SET_CHARACTER_CODE_MAPPING, aChar, aUnicodeChar);
    }
}

void BlueDisplay::setLongTouchDownTimeout(uint16_t aLongTouchDownTimeoutMillis) {
#ifdef LOCAL_DISPLAY_EXISTS
        changeDelayCallback(&callbackLongTouchDownTimeout, aLongTouchDownTimeoutMillis);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_GLOBAL_SETTINGS, 2, SET_LONG_TOUCH_DOWN_TIMEOUT, aLongTouchDownTimeoutMillis);
    }
}

void BlueDisplay::clearDisplay(uint16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.clearDisplay(aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_CLEAR_DISPLAY, 1, aColor);
    }
}

extern "C" void clearDisplayC(uint16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.clearDisplay(aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_CLEAR_DISPLAY, 1, aColor);
    }
}

void BlueDisplay::drawPixel(uint16_t aXPos, uint16_t aYPos, uint16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.drawPixel(aXPos, aYPos, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_DRAW_PIXEL, 3, aXPos, aYPos, aColor);
    }
}

void BlueDisplay::drawLine(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.drawLine(aXStart, aYStart, aXEnd, aYEnd, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSART5Args(FUNCTION_TAG_DRAW_LINE, aXStart, aYStart, aXEnd, aYEnd, aColor);
    }
}

void BlueDisplay::drawLineRel(uint16_t aXStart, uint16_t aYStart, uint16_t aXDelta, uint16_t aYDelta, uint16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.drawLine(aXStart, aYStart, aXStart + aXDelta, aYStart + aYDelta, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSART5Args(FUNCTION_TAG_DRAW_LINE_REL, aXStart, aYStart, aXDelta, aYDelta, aColor);
    }
}

/**
 * Fast routine for drawing data charts
 * draws a line only from x to x+1
 * first pixel is omitted because it is drawn by preceding line
 * uses setArea instead if drawPixel to speed up drawing
 */
void BlueDisplay::drawLineFastOneX(uint16_t aXStart, uint16_t aYStart, uint16_t aYEnd, uint16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.drawLineFastOneX(aXStart, aYStart, aYEnd, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        // Just draw plain line, no need to speed up
        sendUSART5Args(FUNCTION_TAG_DRAW_LINE, aXStart, aYStart, aXStart + 1, aYEnd, aColor);
    }
}

void BlueDisplay::drawLineWithThickness(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, int16_t aThickness,
        uint8_t aThicknessMode, uint16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    drawThickLine(aXStart, aYStart, aXEnd, aYEnd, aThickness, aThicknessMode, aColor);
#endif
    sendUSARTArgs(FUNCTION_TAG_DRAW_LINE, 6, aXStart, aYStart, aXEnd, aYEnd, aColor, aThickness);
}

void BlueDisplay::drawRect(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor,
        uint16_t aStrokeWidth) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.drawRect(aXStart, aYStart, aXEnd - 1, aYEnd - 1, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_DRAW_RECT, 6, aXStart, aYStart, aXEnd, aYEnd, aColor, aStrokeWidth);
    }
}

void BlueDisplay::drawRectRel(uint16_t aXStart, uint16_t aYStart, uint16_t aWidth, uint16_t aHeight, uint16_t aColor,
        uint16_t aStrokeWidth) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.drawRect(aXStart, aYStart, aXStart + aWidth - 1, aYStart + aHeight - 1, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_DRAW_RECT_REL, 6, aXStart, aYStart, aWidth, aHeight, aColor, aStrokeWidth);
    }
}

void BlueDisplay::fillRect(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.fillRect(aXStart, aYStart, aXEnd, aYEnd, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSART5Args(FUNCTION_TAG_FILL_RECT, aXStart, aYStart, aXEnd, aYEnd, aColor);
    }
}

void BlueDisplay::fillRectRel(uint16_t aXStart, uint16_t aYStart, uint16_t aWidth, uint16_t aHeight, uint16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.fillRect(aXStart, aYStart, aXStart + aWidth - 1, aYStart + aHeight - 1, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSART5Args(FUNCTION_TAG_FILL_RECT_REL, aXStart, aYStart, aWidth, aHeight, aColor);
    }
}

void BlueDisplay::drawCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, uint16_t aColor, uint16_t aStrokeWidth) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.drawCircle(aXCenter, aYCenter, aRadius, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSART5Args(FUNCTION_TAG_DRAW_CIRCLE, aXCenter, aYCenter, aRadius, aColor, aStrokeWidth);
    }
}

void BlueDisplay::fillCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, uint16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.fillCircle(aXCenter, aYCenter, aRadius, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_FILL_CIRCLE, 4, aXCenter, aYCenter, aRadius, aColor);
    }
}

/**
 * @return start x for next character / x + (TEXT_SIZE_11_WIDTH * size)
 */
uint16_t BlueDisplay::drawChar(uint16_t aPosX, uint16_t aPosY, char aChar, uint8_t aCharSize, uint16_t aFGColor,
        uint16_t aBGColor) {
    uint16_t tRetValue = 0;
#ifdef LOCAL_DISPLAY_EXISTS
    tRetValue = LocalDisplay.drawChar(aPosX, aPosY - getTextAscend(aCharSize), aChar, getLocalTextSize(aCharSize), aFGColor,
            aBGColor);
#endif
    if (USART_isBluetoothPaired()) {
        tRetValue = aPosX + getTextWidth(aCharSize);
        sendUSARTArgs(FUNCTION_TAG_DRAW_CHAR, 6, aPosX, aPosY, aCharSize, aFGColor, aBGColor, aChar);
    }
    return tRetValue;
}

/**
 * @param aXStart left position
 * @param aYStart upper position
 * @param aStringPtr is (const char *) to avoid endless casts for string constants
 */
extern "C" void drawMLText(uint16_t aPosX, uint16_t aPosY, const char *aStringPtr, uint8_t aTextSize, uint16_t aColor,
        uint16_t aBGColor) {
    BlueDisplay1.drawMLText(aPosX, aPosY, aStringPtr, aTextSize, aColor, aBGColor);
}

void BlueDisplay::drawMLText(uint16_t aPosX, uint16_t aPosY, const char *aStringPtr, uint8_t aTextSize, uint16_t aColor,
        uint16_t aBGColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.drawMLText(aPosX, aPosY - getTextAscend(aTextSize), (char *) aStringPtr, getLocalTextSize(aTextSize), aColor,
            aBGColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSART5ArgsAndByteBuffer(FUNCTION_TAG_DRAW_STRING, aPosX, aPosY, aTextSize, aColor, aBGColor, (uint8_t*) aStringPtr,
                strlen(aStringPtr));
    }
}

/**
 * @param aXStart left position
 * @param aYStart upper position
 * @param aStringPtr is (const char *) to avoid endless casts for string constants
 * @return uint16_t start x for next character - next x Parameter
 */
uint16_t BlueDisplay::drawText(uint16_t aPosX, uint16_t aPosY, const char *aStringPtr, uint8_t aTextSize, uint16_t aColor,
        uint16_t aBGColor) {
    uint16_t tRetValue = 0;
#ifdef LOCAL_DISPLAY_EXISTS
    tRetValue = LocalDisplay.drawText(aPosX, aPosY - getTextAscend(aTextSize), (char *) aStringPtr, getLocalTextSize(aTextSize),
            aColor, aBGColor);
#endif
    if (USART_isBluetoothPaired()) {
        tRetValue = aPosX + strlen(aStringPtr) * getTextWidth(aTextSize);
        sendUSART5ArgsAndByteBuffer(FUNCTION_TAG_DRAW_STRING, aPosX, aPosY, aTextSize, aColor, aBGColor, (uint8_t*) aStringPtr,
                strlen(aStringPtr));
    }
    return tRetValue;
}

extern "C" uint16_t drawTextC(uint16_t aXStart, uint16_t aYStart, const char *aStringPtr, uint8_t aFontSize, uint16_t aColor,
        uint16_t aBGColor) {
    return BlueDisplay1.drawText(aXStart, aYStart, (char *) aStringPtr, aFontSize, aColor, aBGColor);
}

/**
 * Fast routine for drawing data charts
 */
void BlueDisplay::drawChartByteBuffer(uint16_t aXOffset, uint16_t aYOffset, uint16_t aColor, uint16_t aClearBeforeColor,
        uint8_t *aByteBuffer, uint16_t aByteBufferLength) {
    if (USART_isBluetoothPaired()) {
        // Just draw plain line, no need to speed up
        sendUSART5ArgsAndByteBuffer(FUNCTION_TAG_DRAW_CHART, aXOffset, aYOffset, aColor, aClearBeforeColor, 0, aByteBuffer,
                aByteBufferLength);
    }
}

void BlueDisplay::setMaxDisplaySize(struct XYSize * const aMaxDisplaySizePtr) {
    mMaxDisplaySize.XWidth = aMaxDisplaySizePtr->XWidth;
    mMaxDisplaySize.YHeight = aMaxDisplaySizePtr->YHeight;
}

void BlueDisplay::setActualDisplaySize(struct XYSize * const aActualDisplaySizePrt) {
    mActualDisplaySize.XWidth = aActualDisplaySizePrt->XWidth;
    mActualDisplaySize.YHeight = aActualDisplaySizePrt->YHeight;
}

uint16_t BlueDisplay::getDisplayWidth(void) {
    return mReferenceDisplaySize.XWidth;
}

uint16_t BlueDisplay::getDisplayHeight(void) {
    return mReferenceDisplaySize.YHeight;
}

/*
 * Formula for Monospace Font on Android
 * TextSize * 0.6
 * Integer Formula: (TextSize *6)+4 / 10
 */
uint8_t getTextWidth(uint8_t aTextSize) {
    if (aTextSize == 11) {
        return TEXT_SIZE_11_WIDTH;
    }
    if (aTextSize == 22) {
        return TEXT_SIZE_22_WIDTH;
    }
    return ((aTextSize * 6) + 4) / 10;
}

/*
 * Formula for Monospace Font on Android
 * float: TextSize * 0.76
 * int: (TextSize * 195 + 128) >> 8
 */
uint8_t getTextAscend(uint8_t aTextSize) {
    if (aTextSize == TEXT_SIZE_11) {
        return TEXT_SIZE_11_ASCEND;
    }
    if (aTextSize == TEXT_SIZE_22) {
        return TEXT_SIZE_22_ASCEND;
    }
    uint16_t tRetvalue = aTextSize;
    tRetvalue = ((tRetvalue * 195) + 128) >> 8;
    return tRetvalue;
}

/*
 * Formula for Monospace Font on Android
 * float: TextSize * 0.24
 * int: (TextSize * 61 + 128) >> 8
 */
uint8_t getTextDecend(uint8_t aTextSize) {
    if (aTextSize == TEXT_SIZE_11) {
        return TEXT_SIZE_11_ASCEND;
    }
    if (aTextSize == TEXT_SIZE_22) {
        return TEXT_SIZE_22_ASCEND;
    }
    uint16_t tRetvalue = aTextSize;
    tRetvalue = ((tRetvalue * 61) + 128) >> 8;
    return tRetvalue;
}
/*
 * Ascend - Decent
 * is used to position text in the middle of a button
 * Formula for positioning:
 * Position = ButtonTop + (ButtonHeight + getTextAscendMinusDescend())/2
 */
uint16_t getTextAscendMinusDescend(uint8_t aTextSize) {
    if (aTextSize == TEXT_SIZE_11) {
        return TEXT_SIZE_11_ASCEND - TEXT_SIZE_11_DECEND;
    }
    if (aTextSize == TEXT_SIZE_22) {
        return TEXT_SIZE_22_ASCEND - TEXT_SIZE_22_DECEND;
    }
    uint16_t tRetvalue = aTextSize;
    tRetvalue = ((tRetvalue * 133) + 128) >> 8;
    return tRetvalue;
}

/*
 * (Ascend -Decent)/2
 */
uint8_t getTextMiddle(uint8_t aTextSize) {
    if (aTextSize == TEXT_SIZE_11) {
        return (TEXT_SIZE_11_ASCEND - TEXT_SIZE_11_DECEND) / 2;
    }
    if (aTextSize == TEXT_SIZE_22) {
        return (TEXT_SIZE_22_ASCEND - TEXT_SIZE_22_DECEND) / 2;
    }
    uint16_t tRetvalue = aTextSize;
    tRetvalue = ((tRetvalue * 66) + 128) >> 8;
    return tRetvalue;
}

/*
 * fast divide by 11 for MI0283QT2 driver arguments
 */
uint8_t getLocalTextSize(uint8_t aTextSize) {
    if (aTextSize <= 11) {
        return 1;
    }
    if (aTextSize == 22) {
        return 2;
    }
    return aTextSize / 11;
}

/*****************************************************************************
 * Vector for ThickLine
 *****************************************************************************/

/**
 * aNewRelEndX + Y are new x and y values relative to start point
 */
void BlueDisplay::refreshVector(struct ThickLine * aLine, int16_t aNewRelEndX, int16_t aNewRelEndY) {
    int16_t tNewEndX = aLine->StartX + aNewRelEndX;
    int16_t tNewEndY = aLine->StartY + aNewRelEndY;
    if (aLine->EndX != tNewEndX || aLine->EndX != tNewEndY) {
        //clear old line
        drawLineWithThickness(aLine->StartX, aLine->StartY, aLine->EndX, aLine->EndY, aLine->Thickness, aLine->ThicknessMode,
                aLine->BackgroundColor);
        // Draw new line
        /**
         * clipping
         */
        if (tNewEndX < 0) {
            tNewEndX = 0;
        } else if (tNewEndX > mReferenceDisplaySize.XWidth - 1) {
            tNewEndX = mReferenceDisplaySize.XWidth - 1;
        }
        aLine->EndX = tNewEndX;

        if (tNewEndY < 0) {
            tNewEndY = 0;
        } else if (tNewEndY > mReferenceDisplaySize.YHeight - 1) {
            tNewEndY = mReferenceDisplaySize.YHeight - 1;
        }
        aLine->EndY = tNewEndY;

        drawLineWithThickness(aLine->StartX, aLine->StartY, tNewEndX, tNewEndY, aLine->Thickness, aLine->ThicknessMode,
                aLine->Color);
    }
}

/*****************************************************************************
 * Display and drawing tests
 *****************************************************************************/
/**
 * Draws a star consisting of 4 lines each quadrant
 */
void BlueDisplay::drawStar(int aXPos, int aYPos, int tOffsetCenter, int tLength, int tOffsetDiagonal, int aLengthDiagonal,
        int aColor) {

    int X = aXPos + tOffsetCenter;
    // first right then left lines
    for (int i = 0; i < 2; i++) {
        drawLineRel(X, aYPos, tLength, 0, aColor);
        drawLineRel(X, aYPos - tOffsetDiagonal, tLength, -aLengthDiagonal, aColor); // < 45 degree
        drawLineRel(X, aYPos + tOffsetDiagonal, tLength, aLengthDiagonal, aColor); // < 45 degree
        X = aXPos - tOffsetCenter;
        tLength = -tLength;
    }

    int Y = aYPos + tOffsetCenter;
    // first lower then upper lines
    for (int i = 0; i < 2; i++) {
        drawLineRel(aXPos, Y, 0, tLength, aColor);
        drawLineRel(aXPos - tOffsetDiagonal, Y, -aLengthDiagonal, tLength, aColor);
        drawLineRel(aXPos + tOffsetDiagonal, Y, aLengthDiagonal, tLength, aColor);
        Y = aYPos - tOffsetCenter;
        tLength = -tLength;
    }

    X = aXPos + tOffsetCenter;
    int tLengthDiagonal = tLength;
    for (int i = 0; i < 2; i++) {
        drawLineRel(X, aYPos - tOffsetCenter, tLength, -tLengthDiagonal, aColor); // 45
        drawLineRel(X, aYPos + tOffsetCenter, tLength, tLengthDiagonal, aColor); // 45
        X = aXPos - tOffsetCenter;
        tLength = -tLength;
    }

    drawPixel(aXPos, aYPos, COLOR_BLUE);
}

/**
 * Draws a greyscale and 3 color bars
 */
void BlueDisplay::drawGreyscale(uint16_t aXPos, uint16_t tYPos, uint16_t aHeight) {
    uint16_t tY;
    for (int i = 0; i < 256; ++i) {
        tY = tYPos;
        drawLineRel(aXPos, tY, 0, aHeight, RGB(i, i, i));
        tY += aHeight;
        drawLineRel(aXPos, tY, 0, aHeight, RGB((0xFF - i), (0xFF - i), (0xFF - i)));
        tY += aHeight;
        drawLineRel(aXPos, tY, 0, aHeight, RGB(i, 0, 0));
        tY += aHeight;
        drawLineRel(aXPos, tY, 0, aHeight, RGB(0, i, 0));
        tY += aHeight;
        // For Test purposes: fillRectRel instead of drawLineRel gives missing pixel on different scale factors
        fillRectRel(aXPos, tY, 1, aHeight, RGB(0, 0, i));
        aXPos++;
    }
}

/**
 * Draws test page and a greyscale bar
 */
void BlueDisplay::testDisplay(void) {
    clearDisplay(COLOR_WHITE);

    fillRectRel(0, 0, 2, 2, COLOR_RED);
    fillRectRel(mReferenceDisplaySize.XWidth - 3, 0, 3, 3, COLOR_GREEN);
    fillRectRel(0, mReferenceDisplaySize.YHeight - 4, 4, 4, COLOR_BLUE);
    fillRectRel(mReferenceDisplaySize.XWidth - 3, mReferenceDisplaySize.YHeight - 3, 3, 3, COLOR_BLACK);

    fillRectRel(2, 2, 4, 4, COLOR_RED);
    fillRectRel(10, 20, 10, 20, COLOR_RED);
    drawRectRel(8, 18, 14, 24, COLOR_BLUE, 1);
    drawCircle(15, 30, 5, COLOR_BLUE, 1);
    fillCircle(20, 10, 10, COLOR_BLUE);

    drawLineRel(0, mReferenceDisplaySize.YHeight - 1, mReferenceDisplaySize.XWidth, -mReferenceDisplaySize.YHeight, COLOR_GREEN);
    drawLineRel(6, 6, mReferenceDisplaySize.XWidth - 9, mReferenceDisplaySize.YHeight - 9, COLOR_BLUE);
    drawChar(50, TEXT_SIZE_11_ASCEND, 'y', TEXT_SIZE_11, COLOR_GREEN, COLOR_YELLOW);
    drawText(0, 50 + TEXT_SIZE_11_ASCEND, StringCalibration, TEXT_SIZE_11, COLOR_BLACK, COLOR_WHITE);
    drawText(0, 50 + TEXT_SIZE_11_HEIGHT + TEXT_SIZE_11_ASCEND, StringCalibration, TEXT_SIZE_11, COLOR_WHITE, COLOR_BLACK);
    // TODO Test
    drawLineOverlap(120, 140, 180, 125, LINE_OVERLAP_MAJOR, COLOR_RED);
    drawLineOverlap(120, 143, 180, 128, LINE_OVERLAP_MINOR, COLOR_RED);
    drawLineOverlap(120, 146, 180, 131, LINE_OVERLAP_BOTH, COLOR_RED);

    fillRectRel(100, 100, 10, 5, COLOR_RED);
    fillRectRel(90, 95, 10, 5, COLOR_RED);
    fillRectRel(100, 90, 10, 10, COLOR_BLACK);
    fillRectRel(95, 100, 5, 5, COLOR_BLACK);

    drawStar(200, 120, 4, 6, 2, 2, COLOR_BLACK);
    drawStar(250, 120, 8, 12, 4, 4, COLOR_BLACK);

    uint16_t DeltaSmall = 20;
    uint16_t DeltaBig = 100;
    uint16_t tYPos = 30;

    drawLineWithThickness(140, tYPos, 140 + DeltaBig, tYPos - DeltaSmall, 9, LINE_THICKNESS_DRAW_CLOCKWISE, COLOR_GREEN);
    drawPixel(140, tYPos, COLOR_BLUE);

    tYPos += 5;
    drawLineWithThickness(60, tYPos, 60 + DeltaBig, tYPos + DeltaSmall, 9, LINE_THICKNESS_DRAW_CLOCKWISE, COLOR_GREEN);
    drawPixel(100, tYPos + 5, COLOR_BLUE);

    tYPos += 40;
    drawLineWithThickness(10, tYPos, 10 + DeltaSmall, tYPos + DeltaBig, 4, LINE_THICKNESS_MIDDLE, COLOR_GREEN);
    drawPixel(10, tYPos, COLOR_BLUE);

    drawLineWithThickness(70, tYPos, 70 - DeltaSmall, tYPos + DeltaBig, 4, LINE_THICKNESS_MIDDLE, COLOR_GREEN);
    drawPixel(70, tYPos, COLOR_BLUE);

    tYPos += 10;
    drawLineWithThickness(140, tYPos, 140 - DeltaSmall, tYPos - DeltaSmall, 3, LINE_THICKNESS_MIDDLE, COLOR_GREEN);
    drawPixel(140, tYPos, COLOR_BLUE);

    drawLineWithThickness(150, tYPos, 150 + DeltaSmall, tYPos - DeltaSmall, 3, LINE_THICKNESS_MIDDLE, COLOR_GREEN);
    drawPixel(150, tYPos, COLOR_BLUE);

    drawLineWithThickness(190, tYPos, 190 - DeltaSmall, tYPos - DeltaSmall, 3, LINE_THICKNESS_DRAW_CLOCKWISE, COLOR_GREEN);
    drawPixel(190, tYPos, COLOR_BLUE);

    drawLineWithThickness(200, tYPos, 200 + DeltaSmall, tYPos - DeltaSmall, 3, LINE_THICKNESS_DRAW_CLOCKWISE, COLOR_GREEN);
    drawPixel(200, tYPos, COLOR_BLUE);

    drawGreyscale(5, 180, 10);
}

#define COLOR_SPECTRUM_SEGMENTS 6 // red->yellow, yellow-> green, green-> cyan, cyan-> blue, blue-> magent, magenta-> red
#define COLOR_RESOLUTION 32 // 5 bit for 16 bit color (green really has 6 bit, but dont use it)
const uint16_t colorIncrement[COLOR_SPECTRUM_SEGMENTS] = { 1 << 6, 0x1F << 11, 1, 0x3FF << 6, 1 << 11, 0xFFFF };

/**
 * generates a full color spectrum beginning with a black line,
 * increasing saturation to full colors and then fading to a white line
 * customized for a 320 x 240 display
 */
void BlueDisplay::generateColorSpectrum(void) {
    clearDisplay(COLOR_WHITE);
    uint16_t tColor;
    uint16_t tXPos;
    uint16_t tDelta;
    uint16_t tError;

    uint16_t tColorChangeAmount;
    uint16_t tYpos = mReferenceDisplaySize.YHeight;
    uint16_t tColorLine;
    for (int line = 4; line < mReferenceDisplaySize.YHeight + 4; ++line) {
        tColorLine = line / 4;
        // colors for line 31 and 32 are identical
        if (tColorLine >= COLOR_RESOLUTION) {
            // line 32 to 63 full saturated basic colors to pure white
            tColorChangeAmount = ((2 * COLOR_RESOLUTION) - 1) - tColorLine; // 31 - 0
            tColor = 0x1f << 11 | (tColorLine - COLOR_RESOLUTION) << 6 | (tColorLine - COLOR_RESOLUTION);
        } else {
            // line 0 - 31 pure black to full saturated basic colors
            tColor = tColorLine << 11; // RED
            tColorChangeAmount = tColorLine; // 0 - 31
        }
        tXPos = 0;
        tYpos--;
        for (int i = 0; i < COLOR_SPECTRUM_SEGMENTS; ++i) {
            tDelta = colorIncrement[i];
//          tError = COLOR_RESOLUTION / 2;
//          for (int j = 0; j < COLOR_RESOLUTION; ++j) {
//              // draw start value + 31 slope values
//              _drawPixel(tXPos++, tYpos, tColor);
//              tError += tColorChangeAmount;
//              if (tError > COLOR_RESOLUTION) {
//                  tError -= COLOR_RESOLUTION;
//                  tColor += tDelta;
//              }
//          }
            tError = ((mReferenceDisplaySize.XWidth / COLOR_SPECTRUM_SEGMENTS) - 1) / 2;
            for (int j = 0; j < (mReferenceDisplaySize.XWidth / COLOR_SPECTRUM_SEGMENTS) - 1; ++j) {
                drawPixel(tXPos++, tYpos, tColor);
                tError += tColorChangeAmount;
                if (tError > ((mReferenceDisplaySize.XWidth / COLOR_SPECTRUM_SEGMENTS) - 1)) {
                    tError -= ((mReferenceDisplaySize.XWidth / COLOR_SPECTRUM_SEGMENTS) - 1);
                    tColor += tDelta;
                }
            }
            // draw greyscale in the last 8 pixel :-)
//          _drawPixel(mReferenceDisplaySize.XWidth - 2, tYpos, (tColorLine & 0x3E) << 10 | tColorLine << 5 | tColorLine >> 1);
//          _drawPixel(mReferenceDisplaySize.XWidth - 1, tYpos, (tColorLine & 0x3E) << 10 | tColorLine << 5 | tColorLine >> 1);
            drawLine(mReferenceDisplaySize.XWidth - 8, tYpos, mReferenceDisplaySize.XWidth - 1, tYpos,
                    (tColorLine & 0x3E) << 10 | tColorLine << 5 | tColorLine >> 1);

        }
    }
}

/***************************************************************************************************************************************************
 *
 * BUTTONS
 *
 **************************************************************************************************************************************************/

uint16_t sButtonIndex = 0;
void BlueDisplay::resetAllButtons(void) {
    sButtonIndex = 0;
}

uint16_t BlueDisplay::createButton(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX,
        const uint16_t aHeightY, const uint16_t aButtonColor, const char * aCaption, const uint8_t aCaptionSize,
        const uint8_t aFlags, const int16_t aValue, void (*aOnTouchHandler)(uint8_t, int16_t)) {
    uint16_t tButtonNumber = NO_BUTTON;
    if (USART_isBluetoothPaired()) {
        tButtonNumber = sButtonIndex++;
        sendUSARTArgsAndByteBuffer(FUNCTION_TAG_BUTTON_CREATE, 9, tButtonNumber, aPositionX, aPositionY, aWidthX, aHeightY,
                aButtonColor, aCaptionSize | (aFlags << 8), aValue, aOnTouchHandler, strlen(aCaption), aCaption);
    }
    return tButtonNumber;
}

void BlueDisplay::drawButton(uint8_t aButtonNumber) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_BUTTON_DRAW, 1, aButtonNumber);
    }
}

void BlueDisplay::drawButtonCaption(uint8_t aButtonNumber) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_BUTTON_DRAW_CAPTION, 1, aButtonNumber);
    }
}

void BlueDisplay::setButtonCaption(uint8_t aButtonNumber, const char * aCaption, bool doDrawButton) {
    if (USART_isBluetoothPaired()) {
        uint8_t tFunctionCode = FUNCTION_TAG_BUTTON_SET_CAPTION;
        if (doDrawButton) {
            tFunctionCode = FUNCTION_TAG_BUTTON_SET_CAPTION_AND_DRAW_BUTTON;
        }
        sendUSARTArgsAndByteBuffer(tFunctionCode, 1, aButtonNumber, strlen(aCaption), aCaption);
    }
}

void BlueDisplay::setButtonValue(uint8_t aButtonNumber, const int16_t aValue) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_BUTTON_SETTINGS, 3, aButtonNumber, BUTTON_FLAG_SET_VALUE, aValue);
    }
}

void BlueDisplay::setButtonColor(uint8_t aButtonNumber, const int16_t aButtonColor) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_BUTTON_SETTINGS, 3, aButtonNumber, BUTTON_FLAG_SET_COLOR_BUTTON, aButtonColor);
    }
}

/*
 * value 0 -> red, 1 -> green
 */
void BlueDisplay::setRedGreenButtonColorAndDraw(uint8_t aButtonNumber, int16_t aValue) {
    uint16_t tColor;
    if (aValue != 0) {
        // map to boolean
        aValue = true;
        tColor = COLOR_GREEN;
    } else {
        tColor = COLOR_RED;
    }
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_BUTTON_SET_COLOR_AND_VALUE_AND_DRAW, 3, aButtonNumber, tColor, aValue);
    }
}

void BlueDisplay::activateButton(uint8_t aButtonNumber) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_BUTTON_SETTINGS, 2, aButtonNumber, BUTTON_FLAG_SET_ACTIVE);
    }
}

void BlueDisplay::deactivateButton(uint8_t aButtonNumber) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_BUTTON_SETTINGS, 2, aButtonNumber, BUTTON_FLAG_RESET_ACTIVE);
    }
}

void BlueDisplay::setButtonsFlags(uint16_t aFlags) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_BUTTON_GLOBAL_SETTINGS, 1, aFlags);
    }
}

void BlueDisplay::activateAllButtons(void) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_BUTTON_ACTIVATE_ALL, 0);
    }
}

void BlueDisplay::deactivateAllButtons(void) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_BUTTON_DEACTIVATE_ALL, 0);
    }
}

/***************************************************************************************************************************************************
 *
 * SLIDER
 *
 **************************************************************************************************************************************************/

uint8_t sSliderIndex = 0;

void BlueDisplay::resetAllSliders(void) {
    sSliderIndex = 0;
}

/**
 * @brief initialization with all parameters except color
 * @param aPositionX determines upper left corner
 * @param aPositionY determines upper left corner
 * @param aBarWidth width of bar (and border) in pixel * #TOUCHSLIDER_OVERALL_SIZE_FACTOR
 * @param aBarLength size of slider bar in pixel
 * @param aThresholdValue value where color of bar changes from #TOUCHSLIDER_DEFAULT_BAR_COLOR to #TOUCHSLIDER_DEFAULT_BAR_THRESHOLD_COLOR
 * @param aInitalValue
 * @param aSliderColor
 * @param aBarColor
 * @param aOptions see #TOUCHSLIDER_SHOW_BORDER etc.
 * @param aCaption if NULL no caption is drawn
 * @param aOnChangeHandler - if NULL no update of bar is done on touch
 * @param aValueHandler Handler to convert actualValue to string - if NULL sprintf("%3d", mActualValue) is used
 *  * @return false if parameters are not consistent ie. are internally modified
 *  or if not enough space to draw caption or value.
 */
uint8_t BlueDisplay::createSlider(const uint16_t aPositionX, const uint16_t aPositionY, const uint8_t aBarWidth,
        const uint16_t aBarLength, const uint16_t aThresholdValue, const int16_t aInitalValue, const uint16_t aSliderColor,
        const uint16_t aBarColor, const uint8_t aOptions, void (*aOnChangeHandler)(const uint8_t, const int16_t)) {
    uint8_t tSliderNumber = NO_SLIDER;

    if (USART_isBluetoothPaired()) {
        tSliderNumber = sSliderIndex++;
        sendUSARTArgs(FUNCTION_TAG_SLIDER_CREATE, 11, tSliderNumber, aPositionX, aPositionY, aBarWidth, aBarLength, aThresholdValue,
                aInitalValue, aSliderColor, aBarColor, aOptions, aOnChangeHandler);
    }
    return tSliderNumber;
}

void BlueDisplay::drawSlider(uint8_t aSliderNumber) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_SLIDER_DRAW, 1, aSliderNumber);
    }
}

void BlueDisplay::drawSliderBorder(uint8_t aSliderNumber) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_SLIDER_DRAW_BORDER, 1, aSliderNumber);
    }
}

void BlueDisplay::setSliderActualValueAndDraw(uint8_t aSliderNumber, int16_t aActualValue) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_SLIDER_SETTINGS, 3, aSliderNumber, SLIDER_FLAG_SET_VALUE_AND_DRAW_BAR, aActualValue);
    }
}

void BlueDisplay::activateSlider(uint8_t aSliderNumber) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_SLIDER_SETTINGS, 2, aSliderNumber, SLIDER_FLAG_SET_ACTIVE);
    }
}

void BlueDisplay::deactivateSlider(uint8_t aSliderNumber) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_SLIDER_SETTINGS, 2, aSliderNumber, SLIDER_FLAG_RESET_ACTIVE);
    }
}

void BlueDisplay::activateAllSliders(void) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_SLIDER_ACTIVATE_ALL, 0);
    }
}

void BlueDisplay::deactivateAllSliders(void) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_TAG_SLIDER_DEACTIVATE_ALL, 0);
    }
}
