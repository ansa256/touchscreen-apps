/*
 * Pages.cpp
 *
 *  Created on: 15.12.2013
 *      Author: Armin
 */

#include "utils.h"
#include "misc.h"

extern "C" {
#include "timing.h"
#include "stm32f30xPeripherals.h"
}

static uint32_t MillisOfLastDrawRTCTime;
/**
 * refreshs display of time/date every second if RTC available
 * @param x
 * @param y
 * @param aColor
 * @param aBackColor
 */
void showRTCTimeEverySecond(uint16_t x, uint16_t y, uint16_t aColor, uint16_t aBackColor) {
// count milliseconds for loop control
	uint32_t tMillis = getMillisSinceBoot() - MillisOfLastDrawRTCTime;
	if (tMillis > 1000) {
		MillisOfLastDrawRTCTime = getMillisSinceBoot();
		//RTC
		RTC_getTimeString(StringBuffer);
		if (RTC_DateIsValid) {
			drawText(x, y, StringBuffer, 1, aColor, aBackColor);
		}
	}
}

void showRTCTime(uint16_t x, uint16_t y, uint16_t aColor, uint16_t aBackColor, bool aForceDisplay) {
	RTC_getTimeString(StringBuffer);
	if (RTC_DateIsValid || aForceDisplay) {
		drawText(x, y, StringBuffer, 1, aColor, aBackColor);
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
