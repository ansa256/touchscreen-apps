/**
 * @file myprint.cpp
 *
 * @date 05.1.2014
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 */

#include "BlueDisplay.h"
#include "misc.h" // for StringBuffer
#include "myprint.h"
#include <stdio.h> // for sprintf
#include <stdarg.h>

uint8_t sPrintSize = 1;
uint16_t sPrintColor = COLOR_WHITE;
uint16_t sPrintBackgroundColor = COLOR_BLACK;
int sPrintX = 0;
int sPrintY = 0;
bool sClearOnNewScreen = true;
bool sPrintEnabled = false;

void printSetOptions(uint8_t aPrintSize, uint16_t aPrintColor, uint16_t aPrintBackgroundColor, bool aClearOnNewScreen) {
    sPrintSize = aPrintSize;
    sPrintColor = aPrintColor;
    sPrintBackgroundColor = aPrintBackgroundColor;
    sClearOnNewScreen = aClearOnNewScreen;
}

void setPrintStatus(bool aNewStatus) {
    sPrintEnabled = aNewStatus;
}

bool getPrintStatus(void) {
    return sPrintEnabled;
}

void printEnable(void) {
    sPrintEnabled = true;
}

void printDisable(void) {
    sPrintEnabled = false;
}

void printClearScreen(void) {
    if (!sPrintEnabled) {
        return;
    }
    clearDisplayC(sPrintBackgroundColor);
    sPrintX = 0;
    sPrintY = 0;
}

/**
 *
 * @param aPosX in pixel coordinates
 * @param aPosY in pixel coordinates
 */
void printSetPosition(int aPosX, int aPosY) {
    sPrintX = aPosX;
    sPrintY = aPosY;
}

void printSetPositionLineColumn(int aColumnNumber, int aLineNumber) {
    sPrintX = aColumnNumber * TEXT_SIZE_11_WIDTH;
    if (sPrintX >= (DISPLAY_DEFAULT_WIDTH - TEXT_SIZE_11_WIDTH)) {
        sPrintX = 0;
    }
    sPrintY = aLineNumber * TEXT_SIZE_11_HEIGHT;
    if (sPrintY >= (DISPLAY_DEFAULT_HEIGHT - TEXT_SIZE_11_HEIGHT)) {
        sPrintY = 0;
    }
}

int printNewline(void) {
    if (!sPrintEnabled) {
        return -1;
    }
    int tPrintY = sPrintY + TEXT_SIZE_11_HEIGHT;
    if (tPrintY >= DISPLAY_DEFAULT_HEIGHT) {
        // wrap around to top of screen
        tPrintY = 0;
        if (sClearOnNewScreen) {
            clearDisplayC(sPrintBackgroundColor);
        }
    }
    sPrintX = 0;
    return tPrintY;
}

int myPrintf(const char *aFormat, ...) {
    va_list argp;
    va_start(argp, aFormat);
    int tLength = vsnprintf(StringBuffer, sizeof StringBuffer, aFormat, argp);
    va_end(argp);
    myPrint(StringBuffer, tLength);
    return tLength;
}

/**
 * Prints string starting at actual display position and sets new position
 * Handles leading spaces, newlines and word wrap
 * @param StringPointer
 * @param aLength
 */
void myPrint(const char * StringPointer, int aLength) {
    if (!sPrintEnabled) {
        return;
    }
    char tChar;
    const char *tWordStart = StringPointer;
    const char *tPrintBufferStart = StringPointer;
    const char *tPrintBufferEnd = StringPointer + aLength;
    int tLineLengthInChars = DISPLAY_DEFAULT_WIDTH / (TEXT_SIZE_11_WIDTH * sPrintSize);
    bool doFlushAndNewline = false;
    int tColumn = sPrintX / TEXT_SIZE_11_WIDTH;
    while (true) {
        tChar = *StringPointer++;

        // check for terminate condition
        if (tChar == '\0' || StringPointer > tPrintBufferEnd) {
            // flush "buffer"
            sPrintX = drawNText(sPrintX, sPrintY, tPrintBufferStart, StringPointer - tPrintBufferStart, sPrintSize, sPrintColor,
                    sPrintBackgroundColor);
            // handling of newline - after end of string
            if (sPrintX >= DISPLAY_DEFAULT_WIDTH) {
                sPrintY = printNewline();
            }
            break;
        }
        if (tChar == '\n') {
            // new line -> start of a new word
            tWordStart = StringPointer;
            // signal flush and newline
            doFlushAndNewline = true;
        } else if (tChar == '\r') {
            // skip but start of a new word
            tWordStart = StringPointer;
        } else if (tChar == ' ') {
            // start of a new word
            tWordStart = StringPointer;
            if (tColumn == 0) {
                // skip from printing if first character in line
                tPrintBufferStart = StringPointer;
            }
        } else {
            if (tColumn >= (DISPLAY_DEFAULT_WIDTH / TEXT_SIZE_11_WIDTH)) {
                // character does not fit in line -> print it at next line
                doFlushAndNewline = true;
                int tWordlength = (StringPointer - tWordStart);
                if (tWordlength > tLineLengthInChars) {
                    //word too long for a line just print char on next line
                    // just draw "buffer" to old line, make newline and process character again
                    StringPointer--;
                } else {
                    // draw buffer till word start, make newline and process word again
                    StringPointer = tWordStart;
                }
            }
        }
        if (doFlushAndNewline) {
            drawNText(sPrintX, sPrintY, tPrintBufferStart, StringPointer - tPrintBufferStart, sPrintSize, sPrintColor,
                    sPrintBackgroundColor);
            tPrintBufferStart = StringPointer;
            sPrintY = printNewline();
            sPrintX = 0; // set it explicitly since compiler may hold sPrintX in register
            tColumn = 0;
            doFlushAndNewline = false;
        }
        tColumn += sPrintSize;
    }
}
