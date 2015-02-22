/*
 * TouchLib.cpp
 *
 * @date 01.09.2014
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 *      License: GPL v3 (http://www.gnu.org/licenses/gpl.html)
 * @version 1.5.0
 */

#include "TouchLib.h"
#ifdef LOCAL_DISPLAY_EXISTS
#include "TouchButton.h"
#include "TouchSlider.h"
#include "ADS7846.h"
#endif

#include <stdio.h> // for printf
#include "stm32f3_discovery.h"  // For LEDx
#include <stdlib.h> // for NULL
extern "C" {
#include "USART_DMA.h"
#include "timing.h"
}

#ifndef DO_NOT_NEED_BASIC_TOUCH
struct XYPosition sDownPosition;
struct XYPosition sActualPosition;
struct XYPosition sUpPosition;
#endif

#ifdef LOCAL_DISPLAY_EXISTS
/*
 * helper variables
 */
//
bool sButtonTouched = false; // flag if autorepeat button was touched - to influence long button press handling
bool sAutorepeatButtonTouched = false; // flag if autorepeat button was touched - to influence long button press handling
bool sNothingTouched = false; // = !(sSliderTouched || sButtonTouched || sAutorepeatButtonTouched)
bool sSliderIsMoveTarget = false; // true if slider was touched by DOWN event

uint32_t sLongTouchDownTimeoutMillis;
/*
 * timer related callbacks
 */bool (*sPeriodicTouchCallback)(int const, int const) = NULL; // return parameter not yet used
uint32_t sPeriodicCallbackPeriodMillis;

struct BluetoothEvent localTouchEvent;
#endif

bool sTouchIsStillDown = false;
bool sDisableTouchUpOnce = false;

struct BluetoothEvent remoteTouchEvent;

void (*sTouchDownCallback)(struct XYPosition * const) = NULL;
void (*sLongTouchDownCallback)(struct XYPosition * const) = NULL;
void (*sTouchMoveCallback)(struct XYPosition * const) = NULL;
void (*sTouchUpCallback)(struct XYPosition * const) = NULL;
bool sTouchUpCallbackEnabled = false;

void (*sSwipeEndCallback)(struct Swipe * const) = NULL;
bool sSwipeEndCallbackEnabled = false;

void (*sConnectCallback)(struct XYSize * const) = NULL;
void (*sSimpleConnectCallback)(void) = NULL;
void (*sResizeAndReconnectCallback)(struct XYSize * const) = NULL;
void (*sSimpleResizeAndReconnectCallback)(void) = NULL;

bool sDisplayXYValuesEnabled = false;  // displays touch values on screen

#ifdef LOCAL_DISPLAY_EXISTS
/**
 * Callback routine for SysTick handler
 */
void callbackPeriodicTouch(void) {
    if (sTouchIsStillDown) {
        if (sPeriodicTouchCallback != NULL) {
            // do "normal" callback for autorepeat buttons
            sPeriodicTouchCallback(sActualPosition.PosX, sActualPosition.PosY);
        }
        if (sTouchIsStillDown) {
            // renew systic callback request
            registerDelayCallback(&callbackPeriodicTouch, sPeriodicCallbackPeriodMillis);
        }
    }
}

/**
 * Register a callback routine which is called every CallbackPeriod milliseconds while screen is touched
 */
void registerPeriodicTouchCallback(bool (*aPeriodicTouchCallback)(int const, int const), const uint32_t aCallbackPeriodMillis) {
    sPeriodicTouchCallback = aPeriodicTouchCallback;
    sPeriodicCallbackPeriodMillis = aCallbackPeriodMillis;
    changeDelayCallback(&callbackPeriodicTouch, aCallbackPeriodMillis);
}

/**
 * set CallbackPeriod
 */
void setPeriodicTouchCallbackPeriod(const uint32_t aCallbackPeriod) {
    sPeriodicCallbackPeriodMillis = aCallbackPeriod;
}

/**
 * Callback routine for SysTick handler
 * Creates event if no Slider was touched and no swipe gesture was started
 * Disabling of touch up handling  (sDisableTouchUpOnce = false) must be done by called handler!!!
 */
void callbackLongTouchDownTimeout(void) {
    assert_param(sLongTouchDownCallback != NULL);
// No long touch if swipe is made or slider touched
    if (!sSliderIsMoveTarget) {
        /*
         * Check if a swipe is intended (position has moved over threshold).
         * If not, call long touch callback
         */
        if (abs(sDownPosition.PosX - sActualPosition.PosX) < TOUCH_SWIPE_THRESHOLD
                && abs(sDownPosition.PosY - sActualPosition.PosY) < TOUCH_SWIPE_THRESHOLD) {
            // fill up event
            localTouchEvent.EventData.TouchPosition = TouchPanel.mTouchLastPosition;
            localTouchEvent.EventType = EVENT_TAG_LONG_TOUCH_DOWN_CALLBACK_ACTION;
        }
    }
}
#endif

void registerConnectCallback(void (*aConnectCallback)(struct XYSize * const aMaxSizePtr)) {
    sConnectCallback = aConnectCallback;
}

void registerSimpleConnectCallback(void (*aConnectCallback)(void)) {
    sSimpleConnectCallback = aConnectCallback;
}

void registerResizeAndReconnectCallback(void (*aResizeAndReconnectCallback)(struct XYSize * const aActualSizePtr)) {
    sResizeAndReconnectCallback = aResizeAndReconnectCallback;
}

void registerSimpleResizeAndReconnectCallback(void (*aSimpleResizeAndReconnectCallback)(void)) {
    sSimpleResizeAndReconnectCallback = aSimpleResizeAndReconnectCallback;
}

#ifndef DO_NOT_NEED_BASIC_TOUCH
void registerTouchDownCallback(void (*aTouchDownCallback)(struct XYPosition * const aActualPositionPtr)) {
    sTouchDownCallback = aTouchDownCallback;
}

void registerTouchMoveCallback(void (*aTouchMoveCallback)(struct XYPosition * const aActualPositionPtr)) {
    sTouchMoveCallback = aTouchMoveCallback;
}

/**
 * Register a callback routine which is called when touch goes up
 */
void registerTouchUpCallback(void (*aTouchUpCallback)(struct XYPosition * const aActualPositionPtr)) {
    sTouchUpCallback = aTouchUpCallback;
    // disable next end touch since we are already in a touch handler and don't want the end of this touch to be propagated
    if (sTouchIsStillDown) {
        sDisableTouchUpOnce = true;
    }
    sTouchUpCallbackEnabled = (aTouchUpCallback != NULL);
}

/**
 * disable or enable end touch if callback pointer valid
 * used by numberpad
 * @param aTouchUpCallbackEnabled
 */
void setTouchUpCallbackEnabled(bool aTouchUpCallbackEnabled) {
    if (aTouchUpCallbackEnabled && sTouchUpCallback != NULL) {
        sTouchUpCallbackEnabled = true;
    } else {
        sTouchUpCallbackEnabled = false;
    }
}
#endif

/**
 * Register a callback routine which is only called after a timeout if screen is still touched
 */
void registerLongTouchDownCallback(void (*aLongTouchDownCallback)(struct XYPosition * const),
        const uint16_t aLongTouchDownTimeoutMillis) {
    sLongTouchDownCallback = aLongTouchDownCallback;
    sLongTouchDownTimeoutMillis = aLongTouchDownTimeoutMillis;
#ifdef LOCAL_DISPLAY_EXISTS
    if (aLongTouchDownCallback == NULL) {
        changeDelayCallback(&callbackLongTouchDownTimeout, DISABLE_TIMER_DELAY_VALUE); // housekeeping - disable timeout
    }
#endif
    BlueDisplay1.setLongTouchDownTimeout(aLongTouchDownTimeoutMillis);
}

/**
 * Register a callback routine which is called when touch goes up and swipe detected
 */
void registerSwipeEndCallback(void (*aSwipeEndCallback)(struct Swipe * const)) {
    sSwipeEndCallback = aSwipeEndCallback;
    // disable next end touch since we are already in a touch handler and don't want the end of this touch to be propagated
    if (sTouchIsStillDown) {
        sDisableTouchUpOnce = true;
    }
    sSwipeEndCallbackEnabled = (aSwipeEndCallback != NULL);
}

void setSwipeEndCallbackEnabled(bool aSwipeEndCallbackEnabled) {
    if (aSwipeEndCallbackEnabled && sSwipeEndCallback != NULL) {
        sSwipeEndCallbackEnabled = true;
    } else {
        sSwipeEndCallbackEnabled = false;
    }
}
/*
 * Delay, which also checks for events
 */
void delayMillisWithCheckAndHandleEvents(int32_t aTimeMillis) {
    aTimeMillis /= 16;
    for (int32_t i = 0; i < aTimeMillis; ++i) {
        delayMillis(16);
        checkAndHandleEvents();
    }
}

/**
 * Is called by thread main loops
 * Checks also for reconnect and resize events
 */
void checkAndHandleEvents(void) {
#ifdef LOCAL_DISPLAY_EXISTS
    resetTouchFlags();
    if (localTouchEvent.EventType != EVENT_TAG_NO_EVENT) {
        handleEvent(&localTouchEvent);
    }
#endif
    /*
     * check USART buffer, which in turn calls handleEvent() if event was received
     */
    checkAndHandleMessageReceived();
}

/**
 * Interprets the event type and manage the callbacks and flags
 * is indirectly called by thread in main loop
 */
extern "C" void handleEvent(struct BluetoothEvent * aEvent) {
    uint8_t tEventType = aEvent->EventType;
    // avoid using event twice
    aEvent->EventType = EVENT_TAG_NO_EVENT;
#ifndef DO_NOT_NEED_BASIC_TOUCH
    if(tEventType <= EVENT_TAG_TOUCH_ACTION_MOVE && sDisplayXYValuesEnabled) {
        printTPData(30, 2 + TEXT_SIZE_11_ASCEND, COLOR_BLACK, COLOR_WHITE);
    }
    if (tEventType == EVENT_TAG_TOUCH_ACTION_DOWN) {
        // must initialize all positions here!
        sDownPosition = aEvent->EventData.TouchPosition;
        sActualPosition = aEvent->EventData.TouchPosition;
        STM_EVAL_LEDOn(LED9); // BLUE Front
        sTouchIsStillDown = true;
#ifdef LOCAL_DISPLAY_EXISTS
        // start timeout for long touch if it is local event
        if (sLongTouchDownCallback != NULL && aEvent != &remoteTouchEvent) {
            changeDelayCallback(&callbackLongTouchDownTimeout, sLongTouchDownTimeoutMillis); // enable timeout
        }
#endif
        if (sTouchDownCallback != NULL) {
            sTouchDownCallback(&aEvent->EventData.TouchPosition);
        }

    } else if (tEventType == EVENT_TAG_TOUCH_ACTION_MOVE) {
        if (sTouchMoveCallback != NULL) {
            sTouchMoveCallback(&aEvent->EventData.TouchPosition);
        }
        sActualPosition = aEvent->EventData.TouchPosition;

    } else if (tEventType == EVENT_TAG_TOUCH_ACTION_UP) {
        sUpPosition = aEvent->EventData.TouchPosition;
        STM_EVAL_LEDOff(LED9); // BLUE Front
        sTouchIsStillDown = false;
#ifdef LOCAL_DISPLAY_EXISTS
        // may set sDisableTouchUpOnce
        handleLocalSwipeDetection();
#endif
        if (sDisableTouchUpOnce) {
            sDisableTouchUpOnce = false;
            return;
        }
        if (sTouchUpCallback != NULL) {
            sTouchUpCallback(&aEvent->EventData.TouchPosition);
        }

    } else if (tEventType == EVENT_TAG_TOUCH_ACTION_ERROR) {
        // try to reset touch state
        STM_EVAL_LEDOff(LED9); // BLUE Front
        sUpPosition = aEvent->EventData.TouchPosition;
        sTouchIsStillDown = false;
    } else
#endif

    if (tEventType == EVENT_TAG_BUTTON_CALLBACK_ACTION) {
        sTouchIsStillDown = false; // to disable local touch up detection
        void (*tCallback)(uint16_t, int16_t) = (void (*)(uint16_t, int16_t)) aEvent->EventData.CallbackInfo.Handler;
        tCallback(aEvent->EventData.CallbackInfo.ObjectIndex, aEvent->EventData.CallbackInfo.HandlerValue.Int16Value);

    } else if (tEventType == EVENT_TAG_SLIDER_CALLBACK_ACTION) {
        sTouchIsStillDown = false; // to disable local touch up detection
        void (*tCallback)(uint16_t, int16_t) = (void (*)(uint16_t, int16_t))aEvent->EventData.CallbackInfo.Handler;
        tCallback(aEvent->EventData.CallbackInfo.ObjectIndex, aEvent->EventData.CallbackInfo.HandlerValue.Int16Value);

    } else if (tEventType == EVENT_TAG_NUMBER_CALLBACK) {
        void (*tCallback)(float) = (void (*)(float))aEvent->EventData.CallbackInfo.Handler;
        tCallback(aEvent->EventData.CallbackInfo.HandlerValue.FloatValue);

    } else if (tEventType == EVENT_TAG_SWIPE_CALLBACK_ACTION) {
        sTouchIsStillDown = false;
        if (sSwipeEndCallback != NULL) {
            // compute it locally - no need to send it over the line
            if (aEvent->EventData.SwipeInfo.SwipeMainDirectionIsX) {
                aEvent->EventData.SwipeInfo.TouchDeltaAbsMax = abs(aEvent->EventData.SwipeInfo.TouchDeltaX);
            } else {
                aEvent->EventData.SwipeInfo.TouchDeltaAbsMax = abs(aEvent->EventData.SwipeInfo.TouchDeltaY);
            }
            sSwipeEndCallback(&(aEvent->EventData.SwipeInfo));
        }

    } else if (tEventType == EVENT_TAG_LONG_TOUCH_DOWN_CALLBACK_ACTION) {
        if (sLongTouchDownCallback != NULL) {
            sLongTouchDownCallback(&(aEvent->EventData.TouchPosition));
        }
        sDisableTouchUpOnce = true;

    } else if (tEventType == EVENT_TAG_CONNECTION_BUILD_UP) {
        BlueDisplay1.setMaxDisplaySize(&aEvent->EventData.DisplaySize);
        if (sSimpleConnectCallback != NULL) {
            sSimpleConnectCallback();
        } else if (sConnectCallback != NULL) {
            sConnectCallback(&aEvent->EventData.DisplaySize);
        }
        // also handle as resize
        tEventType = EVENT_TAG_RESIZE_ACTION;
    }
    if (tEventType == EVENT_TAG_RESIZE_ACTION) {
        BlueDisplay1.setActualDisplaySize(&aEvent->EventData.DisplaySize);
        if (sSimpleResizeAndReconnectCallback != NULL) {
            sSimpleResizeAndReconnectCallback();
        } else if (sResizeAndReconnectCallback != NULL) {
            sResizeAndReconnectCallback(&aEvent->EventData.DisplaySize);
        }
    }
}

#ifdef LOCAL_DISPLAY_EXISTS
void resetTouchFlags(void) {
    sButtonTouched = false;
    sAutorepeatButtonTouched = false;
    sNothingTouched = false;
}

/**
 * Called at Touch Up
 * Handle long callback delay and compute swipe info
 */
void handleLocalSwipeDetection(void) {
    if (sLongTouchDownCallback != NULL) {
        //disable local long touch callback
        changeDelayCallback(&callbackLongTouchDownTimeout, DISABLE_TIMER_DELAY_VALUE);
    }
    if (sSwipeEndCallbackEnabled && !sSliderIsMoveTarget) {
        if (abs(sDownPosition.PosX - sActualPosition.PosX) >= TOUCH_SWIPE_THRESHOLD
                || abs(sDownPosition.PosY - sActualPosition.PosY) >= TOUCH_SWIPE_THRESHOLD) {
            /*
             * Swipe recognized here
             * compute SWIPE data and call callback handler
             */
            struct Swipe tSwipeInfo;
            tSwipeInfo.TouchStartX = sDownPosition.PosX;
            tSwipeInfo.TouchStartY = sDownPosition.PosY;
            tSwipeInfo.TouchDeltaX = sUpPosition.PosX - sDownPosition.PosX;
            uint16_t tTouchDeltaXAbs = abs(tSwipeInfo.TouchDeltaX);
            tSwipeInfo.TouchDeltaY = sUpPosition.PosY - sDownPosition.PosY;
            uint16_t tTouchDeltaYAbs = abs(tSwipeInfo.TouchDeltaY);
            if (tTouchDeltaXAbs >= tTouchDeltaYAbs) {
                // X direction
                tSwipeInfo.SwipeMainDirectionIsX = true;
                tSwipeInfo.TouchDeltaAbsMax = tTouchDeltaXAbs;
            } else {
                tSwipeInfo.SwipeMainDirectionIsX = false;
                tSwipeInfo.TouchDeltaAbsMax = tTouchDeltaYAbs;
            }
            sSwipeEndCallback(&tSwipeInfo);
            sDisableTouchUpOnce = true;
        }
    }
    sSliderIsMoveTarget = false;
}

/**
 *
 * @param aActualPositionPtr
 * @return
 */
void simpleTouchDownHandler(struct XYPosition * const aActualPositionPtr) {
    if (TouchSlider::checkAllSliders(aActualPositionPtr->PosX, aActualPositionPtr->PosY)) {
        sSliderIsMoveTarget = true;
    } else {
        if (!TouchButton::checkAllButtons(aActualPositionPtr->PosX, aActualPositionPtr->PosY, true)) {
            sNothingTouched = true;
        }
    }
}

void simpleTouchHandlerOnlyForButtons(struct XYPosition * const aActualPositionPtr) {
    if (!TouchButton::checkAllButtons(aActualPositionPtr->PosX, aActualPositionPtr->PosY, true)) {
        sNothingTouched = true;
    }
}

void simpleTouchDownHandlerOnlyForSlider(struct XYPosition * const aActualPositionPtr) {
    if (TouchSlider::checkAllSliders(aActualPositionPtr->PosX, aActualPositionPtr->PosY)) {
        sSliderIsMoveTarget = true;
    } else {
        sNothingTouched = true;
    }
}

void simpleTouchMoveHandlerForSlider(struct XYPosition * const aActualPositionPtr) {
    TouchSlider::checkAllSliders(aActualPositionPtr->PosX, aActualPositionPtr->PosY);
}

/**
 * flag for show touchpanel data on screen
 */
void setDisplayXYValuesFlag(bool aEnableDisplay) {
    sDisplayXYValuesEnabled = aEnableDisplay;
}

bool getDisplayXYValuesFlag(void) {
    return sDisplayXYValuesEnabled;
}

/**
 * show touchpanel data on screen
 */
void printTPData(int x, int y, uint16_t aColor, uint16_t aBackColor) {
    snprintf(StringBuffer, sizeof StringBuffer, "X:%03i Y:%03i", sActualPosition.PosX, sActualPosition.PosY);
    BlueDisplay1.drawText(x, y, StringBuffer, TEXT_SIZE_11, aColor, aBackColor);
}
#endif

/**
 * return pointer to end touch callback function
 */

void (*
        getTouchUpCallback(void))(struct XYPosition * const) {
            return sTouchUpCallback;
        }
