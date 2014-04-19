/*
 * misc.h
 *
 * @date 21.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#ifndef MISC_H_
#define MISC_H_

#include "HY32D.h" // for DISPLAY_WIDTH
// for IN_INTERRUPT_SERVICE_ROUTINE
#ifdef __cplusplus
#include "TouchButton.h"
extern "C" {
#endif
void FaultHandler(unsigned int * aFaultArgs);
#ifdef __cplusplus
}
#endif

extern int sLockCount; // counts skipped drawChars because of display locks

extern int DebugValue1;
extern int DebugValue2;
extern int DebugValue3;
extern int DebugValue4;
extern int DebugValue5;

/**
 * Miscellaneous buffers
 */
// for snprintf
#define SIZEOF_STRINGBUFFER 240
extern char StringBuffer[SIZEOF_STRINGBUFFER];

// stores a displayline e.g. for screenshot - +1 because first value is scrap
#define SIZEOF_DISPLAYLINE_BUFFER (4 * DISPLAY_WIDTH)
extern uint16_t FourDisplayLinesBuffer[SIZEOF_DISPLAYLINE_BUFFER];

/**
 * Strings
 */
// from misc.cpp
extern const char StringEmpty[1];
extern const char StringSpace[2];
extern const char StringPlus[2];
extern const char StringMinus[2];
extern const char StringPlusMinus[]; // +- character
extern const char StringGreaterThan[2];
extern const char StringLessThan[2];
extern const char StringComma[2];
extern const char StringDot[2];
extern const char StringHomeChar[2];
extern const char StringDoubleHorizontalArrow[3];

// from PageNumberPad.cpp
extern const char String0[2];
extern const char String1[2];
extern const char String2[2];
extern const char String3[2];
extern const char String4[2];
extern const char String5[2];
extern const char String6[2];
extern const char String7[2];
extern const char String8[2];
extern const char String9[2];
extern const char StringEnterChar[2];
extern const char StringCancel[7];
extern const char StringSign[2];

// from PageFrequencyGenerator.cpp
extern const char String10[3];
extern const char String20[3];
extern const char String50[3];
extern const char String100[4];
extern const char String200[4];
extern const char String500[4];
extern const char String1000[5];
extern const char String1k[3];

// from misc.cpp
extern const char StringStart[6];
extern const char StringStop[5];
extern const char StringClear[6];
extern const char StringBack[5];
extern const char StringHome[5];
extern const char StringSettings[10];
extern const char StringMore[5];
extern const char StringCalibration[12];
extern const char StringLoad[5];
extern const char StringStore[6];
extern const char StringTest[5];
extern const char StringInfo[5];
extern const char StringError[6];
extern const char StringOK[3];
extern const char StringZero[5];

// from TouchDSOGui.cpp
extern const char StringLow[4];
extern const char StringMid[4];
extern const char StringHigh[5];
extern const char StringSmall[6];
extern const char StringLarge[6];
extern const char StringOn[3];
extern const char StringOff[4];
extern const char StringContinue[9];

// from AccuCapacity.cpp
extern const char StringMin[4];
extern const char StringMax[4];
extern const char StringVolt[5];
extern const char StringMilliOhm[5];
extern const char StringExtern[7];
extern const char StringMain[5];
extern const char StringNext[5];
extern const char StringExport[7];

// from PageSettins.cpp
extern const char StringClock[6];
extern const char StringYear[5];
extern const char StringMonth[6];
extern const char StringDay[4];
extern const char StringHour[5];
extern const char String_min[4];
extern const char StringMinute[7];
extern const char StringSecond[7];
extern const char * const DateStrings[7];
extern const char StringPrint[6];

#define IN_INTERRUPT_SERVICE_ROUTINE ((__get_IPSR() & 0xFF) != 0)

__attribute__( ( always_inline ))        static inline uint32_t getLR14(void) {
    uint32_t result;
    asm volatile ("MOV %0, lr" : "=r" (result) );
    return (result);
}

#endif /* MISC_H_ */
