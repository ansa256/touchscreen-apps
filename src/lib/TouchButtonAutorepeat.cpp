/*
 * TouchButtonAutorepeat.cpp
 *
 * Extension of TouchButton
 * implements autorepeat feature for touch buttons
 *
 * @date  30.02.2012
 * @author  Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 * 		52 Byte per autorepeat button on 32Bit ARM
 *
 */

#include "TouchButtonAutorepeat.h"
#include <stm32f30x.h>
#include <stdlib.h> // for NULL
#include "TouchLib.h"

extern "C" {
#include "timing.h"
}

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Button
 * @{
 */
#define AUTOREPEAT_BUTTON_STATE_FIRST_DELAY 0
#define AUTOREPEAT_BUTTON_STATE_FIRST_PERIOD 1
#define AUTOREPEAT_BUTTON_STATE_SECOND_PERIOD 2

TouchButtonAutorepeat TouchButtonAutorepeat::TouchButtonAutorepeatPool[NUMBER_OF_AUTOREPEAT_BUTTONS_IN_POOL];
uint8_t TouchButtonAutorepeat::sState;
uint16_t TouchButtonAutorepeat::sCount;

unsigned long TouchButtonAutorepeat::sMillisOfLastCall;
unsigned long TouchButtonAutorepeat::sMillisSinceFirstTouch;
unsigned long TouchButtonAutorepeat::sMillisSinceLastCallback;

TouchButtonAutorepeat * TouchButtonAutorepeat::mLastAutorepeatButtonTouched = NULL;
bool TouchButtonAutorepeat::sButtonAutorepeatPoolIsInitialized = false;

TouchButtonAutorepeat::TouchButtonAutorepeat() {
    mFlags |= FLAG_IS_AUTOREPEAT_BUTTON;
}

TouchButtonAutorepeat * TouchButtonAutorepeat::allocAndInitSimpleButton(const uint16_t aPositionX, const uint16_t aPositionY,
        const uint16_t aWidthX, const uint8_t aHeightY, const uint16_t aButtonColor, const char * aCaption,
        const uint8_t aCaptionSize, const int16_t aValue, void (*aOnTouchHandler)(TouchButton * const, int16_t)) {
    TouchButtonAutorepeat * tButton = allocButton();
    tButton->initButton(aPositionX, aPositionY, aWidthX, aHeightY, aCaption, aCaptionSize, aButtonColor,
            sDefaultCaptionColor, aValue, aOnTouchHandler);
    return tButton;
}

/**
 * after aMillisFirstDelay milliseconds a callback is done every aMillisFirstRate milliseconds for aFirstCount times
 * after this a callback is done every aMillisSecondRate milliseconds
 */
void TouchButtonAutorepeat::setButtonAutorepeatTiming(const uint16_t aMillisFirstDelay, const uint16_t aMillisFirstRate,
        uint16_t aFirstCount, const uint16_t aMillisSecondRate) {
    mMillisFirstDelay = aMillisFirstDelay;
    mMillisFirstRate = aMillisFirstRate;
    if (aFirstCount < 1) {
        aFirstCount = 1;
    }
    mFirstCount = aFirstCount;
    mMillisSecondRate = aMillisSecondRate;
    // replace standard button handler
    if (mOnTouchHandler != ((void (*)(TouchButton*, int16_t)) &autorepeatTouchHandler)) {
        mOnTouchHandlerAutorepeat = mOnTouchHandler;
        mOnTouchHandler = ((void (*)(TouchButton*, int16_t)) &autorepeatTouchHandler);
    }
}

/**
 * Touch handler for button object which implements the autorepeat functionality
 */
void TouchButtonAutorepeat::autorepeatTouchHandler(TouchButtonAutorepeat * const aTheTouchedButton, int16_t aButtonValue) {
    // First touch here => register callback
    mLastAutorepeatButtonTouched = aTheTouchedButton;
    registerPeriodicTouchCallback(&TouchButtonAutorepeat::autorepeatButtonTimerHandler,
            aTheTouchedButton->mMillisFirstDelay);
    aTheTouchedButton->mOnTouchHandlerAutorepeat(aTheTouchedButton, aButtonValue);
    sState = AUTOREPEAT_BUTTON_STATE_FIRST_DELAY;
    sCount = aTheTouchedButton->mFirstCount;
}

/**
 * callback function for SysTick timer.
 */

bool TouchButtonAutorepeat::autorepeatButtonTimerHandler(const int aTouchPositionX, const int aTouchPositionY) {
    assert_param(mLastAutorepeatButtonTouched != NULL);

    bool tDoCallback = true;
    TouchButtonAutorepeat * tTouchedButton = mLastAutorepeatButtonTouched;
    if (!tTouchedButton->checkButtonInArea(aTouchPositionX, aTouchPositionY)) {
        return false;
    }
    switch (sState) {
    case AUTOREPEAT_BUTTON_STATE_FIRST_DELAY:
        setPeriodicTouchCallbackPeriod(tTouchedButton->mMillisFirstRate);
        sState++;
        sCount--;
        break;
    case AUTOREPEAT_BUTTON_STATE_FIRST_PERIOD:
        if (sCount == 0) {
            setPeriodicTouchCallbackPeriod(tTouchedButton->mMillisSecondRate);
            sCount = 0;
        } else {
            sCount--;
        }
        break;
    case AUTOREPEAT_BUTTON_STATE_SECOND_PERIOD:
        // just for fun
        sCount++;
        break;
    default:
        tDoCallback = false;
        break;
    }

    if (tDoCallback) {
        tTouchedButton->mOnTouchHandlerAutorepeat(tTouchedButton, tTouchedButton->mValue);
        return true;
    }
    return false;
}

/**
 * Static convenience method - checks all autorepeat buttons for matching touch position.
 */

int TouchButtonAutorepeat::checkAllButtons(const int aTouchPositionX, const int aTouchPositionY, const bool doCallback) {
    TouchButtonAutorepeat * tObjectPointer = (TouchButtonAutorepeat*) sListStart;

    // walk through list of active elements
    while (tObjectPointer != NULL) {
        if ((tObjectPointer->mFlags & FLAG_IS_AUTOREPEAT_BUTTON) && (tObjectPointer->mFlags & FLAG_IS_ACTIVE)
                && tObjectPointer->checkButton(aTouchPositionX, aTouchPositionY, doCallback)) {
            return BUTTON_TOUCHED_AUTOREPEAT;
        }
        tObjectPointer = (TouchButtonAutorepeat*) tObjectPointer->mNextObject;
    }
    return NOT_TOUCHED;
}

/**
 * Allocate / get first button from the unallocated buttons
 * @retval Button pointer or message and NULL pointer if no button available
 */
TouchButtonAutorepeat * TouchButtonAutorepeat::allocButton() {
    if (!sButtonAutorepeatPoolIsInitialized) {
        // mark pool as free
        for (int i = 0; i < NUMBER_OF_AUTOREPEAT_BUTTONS_IN_POOL; ++i) {
            TouchButtonAutorepeatPool[i].mFlags &= ~FLAG_IS_ALLOCATED;
        }
        sButtonAutorepeatPoolIsInitialized = true;
        TouchButton::sButtonCombinedPoolSize += NUMBER_OF_AUTOREPEAT_BUTTONS_IN_POOL;
    }

    return (TouchButtonAutorepeat*) TouchButton::allocButton(true);
}

/** @} */
/** @} */
