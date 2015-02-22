/*
 * Pages.cpp
 *
 *  Created on: 15.12.2013
 *      Author: Armin
 */

#include "utils.h"
#include "misc.h"
#include "BlueDisplay.h"

extern "C" {
#include "timing.h"
#include "stm32f30xPeripherals.h"
}

static uint8_t LastSecond;
/**
 * Refreshes display of time/date every second
 * @param x
 * @param y
 * @param aColor
 * @param aBackColor
 */
void showRTCTimeEverySecond(uint16_t x, uint16_t y, uint16_t aColor, uint16_t aBackColor) {
    uint8_t tSecond = RTC_getSecond();
    if (LastSecond != tSecond) {
        LastSecond = tSecond;
        //RTC
        RTC_getTimeString(StringBuffer);
        BlueDisplay1.drawText(x, y, StringBuffer, TEXT_SIZE_11, aColor, aBackColor);

    }
}

/*
 * shows RTC time if year != 0 or if aForceDisplay == true
 */
void showRTCTime(uint16_t x, uint16_t y, uint16_t aColor, uint16_t aBackColor, bool aForceDisplay) {
    RTC_getTimeString(StringBuffer);
    if (RTC_DateIsValid || aForceDisplay) {
        BlueDisplay1.drawText(x, y, StringBuffer, TEXT_SIZE_11, aColor, aBackColor);
    }
}

void formatThousandSeparator(char * aDestPointer, char * aSeparatorAddress) {
    // set separator for thousands
    char* tSrcPtr = aDestPointer + 1;
    while (tSrcPtr <= aSeparatorAddress) {
        *aDestPointer++ = *tSrcPtr++;
    }
    *aSeparatorAddress = THOUSANDS_SEPARATOR;
}
