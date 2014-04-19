/**
 * @file TouchButton.cpp
 *
 * Renders touch buttons for lcd
 * A button can be a simple clickable text
 * or a box with or without text or even an invisible touch area
 *
 * @date  30.01.2012
 * @author  Armin Joachimsmeyer
 * armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 *  LCD interface used:
 * 		fillRect()
 * 		drawText()
 * 		DISPLAY_WIDTH
 * 		DISPLAY_HEIGHT
 * 		FONT_WIDTH
 * 		FONT_HEIGHT
 *
 *
 */

#include "TouchButton.h"
#include "TouchButtonAutorepeat.h"
#include <stm32f30x.h>
#include <stdio.h>

extern "C" {
#include "stm32f30xPeripherals.h"// for FeedbackToneOK
}

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Button
 * @{
 */
TouchButton * TouchButton::sListStart = NULL;
uint8_t TouchButton::sDefaultTouchBorder = TOUCHBUTTON_DEFAULT_TOUCH_BORDER;
uint16_t TouchButton::sDefaultButtonColor = TOUCHBUTTON_DEFAULT_COLOR;
uint16_t TouchButton::sDefaultCaptionColor = TOUCHBUTTON_DEFAULT_CAPTION_COLOR;

TouchButton TouchButton::TouchButtonPool[NUMBER_OF_BUTTONS_IN_POOL];
uint8_t TouchButton::sButtonCombinedPoolSize = NUMBER_OF_BUTTONS_IN_POOL; // NUMBER_OF_AUTOREPEAT_BUTTONS_IN_POOL is added while initializing autorepeat pool
uint8_t TouchButton::sMinButtonPoolSize = NUMBER_OF_BUTTONS_IN_POOL;
bool TouchButton::sButtonPoolIsInitialized = false;

TouchButton::TouchButton() {
    mFlags = 0;
    // buttons are allocated by default!
    mFlags |= FLAG_IS_ALLOCATED;
    mNextObject = NULL;
    if (sListStart == NULL) {
        // first button
        sListStart = this;
    } else {
        // put object in button list
        TouchButton * tObjectPointer = sListStart;
        // search last list element
        while (tObjectPointer->mNextObject != NULL) {
            tObjectPointer = tObjectPointer->mNextObject;
        }
        //insert actual button in last element
        tObjectPointer->mNextObject = this;
    }
}

void TouchButton::setDefaultTouchBorder(const uint8_t aDefaultTouchBorder) {
    sDefaultTouchBorder = aDefaultTouchBorder;
}
void TouchButton::setDefaultButtonColor(const uint16_t aDefaultButtonColor) {
    sDefaultButtonColor = aDefaultButtonColor;
}
void TouchButton::setDefaultCaptionColor(const uint16_t aDefaultCaptionColor) {
    sDefaultCaptionColor = aDefaultCaptionColor;
}

/**
 * Set parameters (except colors and touch border size) for touch button
 * if aWidthX == 0 render only text no background box
 * if aCaptionSize == 0 don't render anything, just check touch area -> transparent button ;-)
 */
int8_t TouchButton::initSimpleButton(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX,
        const uint8_t aHeightY, const uint16_t aButtonColor, const char * aCaption, const uint8_t aCaptionSize,
        const int16_t aValue, void (*aOnTouchHandler)(TouchButton * const, int16_t)) {
    return initButton(aPositionX, aPositionY, aWidthX, aHeightY, aCaption, aCaptionSize, sDefaultTouchBorder, aButtonColor,
            sDefaultCaptionColor, aValue, aOnTouchHandler);
}

/**
 * Set parameters for touch button
 * if aWidthX == 0 render only text no background box
 * if aCaptionSize == 0 don't render anything, just check touch area -> transparent button ;-)
 */
int8_t TouchButton::initButton(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX, const uint8_t aHeightY,
        const char * aCaption, const uint8_t aCaptionSize, const uint8_t aTouchBorder, const uint16_t aButtonColor,
        const uint16_t aCaptionColor, const int16_t aValue, void (*aOnTouchHandler)(TouchButton * const, int16_t)) {

    mWidth = aWidthX;
    mHeight = aHeightY;
    mButtonColor = aButtonColor;
    if (aButtonColor == 0) {
        mButtonColor = sDefaultButtonColor;
    }
    mCaptionColor = aCaptionColor;
    mTouchBorder = aTouchBorder;
    mCaption = aCaption;
    mCaptionSize = aCaptionSize;
    if (aWidthX == 0) {
        mFlags |= FLAG_IS_ONLY_CAPTION;
    } else {
        mFlags &= ~FLAG_IS_ONLY_CAPTION;
    }
    mOnTouchHandler = aOnTouchHandler;
    mValue = aValue;
    return setPosition(aPositionX, aPositionY);
}

/**
 *
 * @param aPositionX
 * @param aPositionY
 * @param aWidthX 		if aWidthX == 0 render only text no background box
 * @param aHeightY
 * @param aButtonColor	if 0 take sDefaultButtonColor
 * @param aCaption
 * @param aCaptionSize	if aCaptionSize == 0 don't render anything, just check touch area -> transparent button ;-)
 * @param aValue
 * @param aOnTouchHandler
 * @return pointer to allocated button
 */
TouchButton * TouchButton::allocAndInitSimpleButton(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX,
        const uint8_t aHeightY, const uint16_t aButtonColor, const char * aCaption, const uint8_t aCaptionSize,
        const int16_t aValue, void (*aOnTouchHandler)(TouchButton * const, int16_t)) {
    TouchButton * tButton = allocButton(false);

    tButton->mWidth = aWidthX;
    tButton->mHeight = aHeightY;
    tButton->mButtonColor = aButtonColor;
    if (aButtonColor == 0) {
        tButton->mButtonColor = sDefaultButtonColor;
    }
    tButton->mCaptionColor = sDefaultCaptionColor;
    tButton->mTouchBorder = sDefaultTouchBorder;
    tButton->mCaption = aCaption;
    tButton->mCaptionSize = aCaptionSize;
    if (aWidthX == 0) {
        if (aCaptionSize == 0) {
            failParamMessage(aCaptionSize, "CaptionSize and WidthX are 0");
        }
        tButton->mFlags |= FLAG_IS_ONLY_CAPTION;
    } else {
        tButton->mFlags &= ~FLAG_IS_ONLY_CAPTION;
    }
    tButton->mOnTouchHandler = aOnTouchHandler;
    tButton->mValue = aValue;
    tButton->setPosition(aPositionX, aPositionY);
    return tButton;
}

int8_t TouchButton::setPosition(const uint16_t aPositionX, const uint16_t aPositionY) {
    int8_t tRetValue = 0;
    mPositionX = aPositionX;
    mPositionY = aPositionY;
    if (mFlags & FLAG_IS_ONLY_CAPTION) {
        // print only string, no enclosing box
        uint8_t tLength = getCaptionLength((char *) mCaption);
        mPositionXRight = aPositionX + tLength - 1;
        mPositionYBottom = aPositionY + (FONT_HEIGHT * mCaptionSize) - 1;
    } else {
        mPositionXRight = aPositionX + mWidth - 1;
        mPositionYBottom = aPositionY + mHeight - 1;
    }

    // check values
    if (mPositionXRight > DISPLAY_WIDTH) {
        mPositionXRight = DISPLAY_WIDTH - 1;
        failParamMessage(mPositionXRight, "XRight wrong");
        tRetValue = TOUCHBUTTON_ERROR_X_RIGHT;
    }
    if (mPositionYBottom >= DISPLAY_HEIGHT) {
        mPositionYBottom = DISPLAY_HEIGHT - 1;
        failParamMessage(mPositionYBottom, "YBottom wrong");
        tRetValue = TOUCHBUTTON_ERROR_Y_BOTTOM;
    }
    return tRetValue;
}

/**
 * renders the button on lcd
 */
int8_t TouchButton::drawButton() {
    if (!(mFlags & FLAG_IS_ONLY_CAPTION)) {
        // Draw rect
        fillRect(mPositionX, mPositionY, mPositionXRight, mPositionYBottom, mButtonColor);
    }
    return drawCaption();
}

/**
 * deactivates the button and redraws its screen space with @a aBackgroundColor
 */
void TouchButton::removeButton(const uint16_t aBackgroundColor) {
    mFlags &= ~FLAG_IS_ACTIVE;
    // Draw rect
    fillRect(mPositionX, mPositionY, mPositionXRight, mPositionYBottom, aBackgroundColor);

}

/**
 * draws the caption of a button
 * @retval 0 or error number #TOUCHBUTTON_ERROR_CAPTION_TOO_LONG etc.
 */
int TouchButton::drawCaption(void) {
    mFlags |= FLAG_IS_ACTIVE;
    int tRetValue = 0;
    if (mCaptionSize > 0) { // dont render anything if caption size == 0
        if (mFlags & FLAG_IS_ONLY_CAPTION) {
            // print only string
            drawText(mPositionX, mPositionY, mCaption, mCaptionSize, mCaptionColor, mButtonColor);
        } else if (mCaption != NULL) {
            int tXCaptionPosition;
            int tYCaptionPosition;
            // try to position the string in the middle of the box
            int tLength = getCaptionLength((char *) mCaption);
            if (mPositionX + tLength >= mPositionXRight) {
                // String too long here
                tXCaptionPosition = mPositionX;
                tRetValue = TOUCHBUTTON_ERROR_CAPTION_TOO_LONG;
            } else {
                tXCaptionPosition = mPositionX + 1 + ((mWidth - tLength) / 2);
            }

            if (mPositionY + (FONT_HEIGHT * mCaptionSize) >= mPositionYBottom) {
                // Font height to big
                tYCaptionPosition = mPositionY;
                tRetValue = TOUCHBUTTON_ERROR_CAPTION_TOO_HIGH;
            } else {
                tYCaptionPosition = mPositionY + 1 + ((mHeight - (FONT_HEIGHT * mCaptionSize)) / 2);
            }
            drawText(tXCaptionPosition, tYCaptionPosition, mCaption, mCaptionSize, mCaptionColor, mButtonColor);

        }
    }
    return tRetValue;
}

/**
 * Check if touch event is in button area
 * if yes - return true
 * if no - return false
 */bool TouchButton::checkButtonInArea(const uint16_t aTouchPositionX, const uint16_t aTouchPositionY) {
    int tPositionBorderX = mPositionX - mTouchBorder;
    if (mTouchBorder > mPositionX) {
        tPositionBorderX = 0;
    }
    int tPositionBorderY = mPositionY - mTouchBorder;
    if (mTouchBorder > mPositionY) {
        tPositionBorderY = 0;
    }
    if (aTouchPositionX < tPositionBorderX || aTouchPositionX > mPositionXRight + mTouchBorder || aTouchPositionY < tPositionBorderY
            || aTouchPositionY > mPositionYBottom + mTouchBorder) {
        return false;
    }
    return true;
}

/**
 * Check if touch event is in button area
 * if yes - call callback function and return true
 * if no - return false
 */bool TouchButton::checkButton(const int aTouchPositionX, const int aTouchPositionY, const bool doCallback) {
    if ((mFlags & FLAG_IS_ACTIVE) && mOnTouchHandler != NULL && checkButtonInArea(aTouchPositionX, aTouchPositionY)) {
        /*
         *  Touch position is in button - call callback function
         */
        if (doCallback) {
            mOnTouchHandler(this, mValue);
        }
        return true;
    }
    return false;
}

/**
 * Static convenience method - checks all buttons for matching touch position.
 */

int TouchButton::checkAllButtons(const int aTouchPositionX, const int aTouchPositionY, const bool doCallback) {
    TouchButton * tObjectPointer = sListStart;

    // walk through list of active elements
    while (tObjectPointer != NULL) {
        if ((tObjectPointer->mFlags & FLAG_IS_ACTIVE)
                && tObjectPointer->checkButton(aTouchPositionX, aTouchPositionY, doCallback)) {
            if (tObjectPointer->mFlags & FLAG_IS_AUTOREPEAT_BUTTON) {
                return BUTTON_TOUCHED_AUTOREPEAT;
            }
            return BUTTON_TOUCHED;
        }
        tObjectPointer = tObjectPointer->mNextObject;
    }
    return BUTTON_NOT_TOUCHED;
}

/**
 * Static convenience method - deactivate all buttons (e.g. before switching screen)
 */
void TouchButton::deactivateAllButtons() {
    TouchButton * tObjectPointer = sListStart;
    // walk through list
    while (tObjectPointer != NULL) {
        tObjectPointer->deactivate();
        tObjectPointer = tObjectPointer->mNextObject;
    }
}

/**
 * Static convenience method - activate all buttons
 */
void TouchButton::activateAllButtons() {
    TouchButton * tObjectPointer = sListStart;
    // walk through list
    while (tObjectPointer != NULL) {
        tObjectPointer->activate();
        tObjectPointer = tObjectPointer->mNextObject;
    }
}

uint8_t TouchButton::getCaptionLength(char * aCaptionPointer) const {
    int tLength = 0;
    while (*aCaptionPointer++ != 0) {
        tLength += (FONT_WIDTH * mCaptionSize);
    }
    return tLength;
}

const char * TouchButton::getCaption() const {
    return mCaption;
}

uint8_t TouchButton::getTouchBorder() const {
    return mTouchBorder;
}

uint16_t TouchButton::getValue() const {
    return mValue;
}
/*
 * Set caption
 */
void TouchButton::setCaption(const char * aCaption) {
    mCaption = aCaption;
}

/*
 * changes box color and redraws button
 */
void TouchButton::setColor(const uint16_t aColor) {
    mButtonColor = aColor;
}

void TouchButton::setCaptionColor(const uint16_t aColor) {
    mCaptionColor = aColor;
}

void TouchButton::setValue(const int16_t aValue) {
    mValue = aValue;

}
uint16_t TouchButton::getPositionX() const {
    return mPositionX;
}

int8_t TouchButton::setPositionX(const uint16_t aPositionX) {
    return setPosition(aPositionX, mPositionY);
}

uint16_t TouchButton::getPositionY() const {
    return mPositionY;
}

int8_t TouchButton::setPositionY(const uint16_t aPositionY) {
    return setPosition(mPositionX, aPositionY);
}

uint16_t TouchButton::getPositionXRight() const {
    return mPositionXRight;
}

uint16_t TouchButton::getPositionYBottom() const {
    return mPositionYBottom;
}

/*
 * activate for touch checking
 */
void TouchButton::activate() {
    mFlags |= FLAG_IS_ACTIVE;
    ;
}

/*
 * deactivate for touch checking
 */
void TouchButton::deactivate() {
    mFlags &= ~FLAG_IS_ACTIVE;
}

void TouchButton::setTouchBorder(uint8_t touchBorder) {
    mTouchBorder = touchBorder;
}

void TouchButton::setTouchHandler(void (*aOnTouchHandler)(TouchButton* const, int16_t)) {
    mOnTouchHandler = aOnTouchHandler;
}

void TouchButton::setRedGreenButtonColor() {
    if (mValue) {
        mButtonColor = COLOR_GREEN;
    } else {
        mButtonColor = COLOR_RED;
    }
}
void TouchButton::setRedGreenButtonColor(int16_t aValue) {
    if (aValue != 0) {
        aValue = true;
        mButtonColor = COLOR_GREEN;
    } else {
        mButtonColor = COLOR_RED;
    }
    mValue = aValue;
}

void TouchButton::setRedGreenButtonColorAndDraw() {
    setRedGreenButtonColor();
    drawButton();
}

void TouchButton::setRedGreenButtonColorAndDraw(int16_t aValue) {
    if (aValue != 0) {
        aValue = true;
    }
    mValue = aValue;
    setRedGreenButtonColorAndDraw();
}

/****************************
 * Pool functions
 ****************************/

/**
 * Allocate / get first button from the unallocated buttons
 * @retval Button pointer or message and NULL pointer if no button available
 */
TouchButton * TouchButton::allocButton(bool aOnlyAutorepeatButtons) {
    TouchButton * tButtonPointer = sListStart;
    if (!sButtonPoolIsInitialized) {
        // mark pool as free
        for (int i = 0; i < NUMBER_OF_BUTTONS_IN_POOL; ++i) {
            TouchButtonPool[i].mFlags &= ~FLAG_IS_ALLOCATED;
        }
        sButtonPoolIsInitialized = true;
    }
    // walk through list
    while (tButtonPointer != NULL
            && ((tButtonPointer->mFlags & FLAG_IS_ALLOCATED)
                    || (aOnlyAutorepeatButtons && !(tButtonPointer->mFlags & FLAG_IS_AUTOREPEAT_BUTTON))
                    || (!aOnlyAutorepeatButtons && (tButtonPointer->mFlags & FLAG_IS_AUTOREPEAT_BUTTON)))) {
        tButtonPointer = tButtonPointer->mNextObject;
    }
    if (tButtonPointer == NULL) {
        if (aOnlyAutorepeatButtons) {
            failParamMessage(sButtonCombinedPoolSize, "No autorepeat button available");
        } else {
            failParamMessage(sButtonCombinedPoolSize, "No button available");
        }
        // to avoid NULL pointer
        tButtonPointer = sListStart;
        sButtonCombinedPoolSize++;
    } else {
        tButtonPointer->mFlags |= FLAG_IS_ALLOCATED;
    }

    sButtonCombinedPoolSize--;
    if (sButtonCombinedPoolSize < sMinButtonPoolSize) {
        sMinButtonPoolSize = sButtonCombinedPoolSize;
    }
    return tButtonPointer;
}

/**
 * Deallocates / free a button and put it back to button bool
 */
void TouchButton::setFree(void) {
    assertParamMessage((this != NULL), sButtonCombinedPoolSize, "Button handle is null");
    sButtonCombinedPoolSize++;
    mFlags &= ~FLAG_IS_ALLOCATED;
    mFlags &= ~FLAG_IS_ACTIVE;
}

/**
 * free / release one button to the unallocated buttons
 */
void TouchButton::freeButton(TouchButton * aTouchButton) {
    TouchButton * tObjectPointer = sListStart;
// walk through list
    while (tObjectPointer != NULL && tObjectPointer != aTouchButton) {
        tObjectPointer = tObjectPointer->mNextObject;
    }
    if (tObjectPointer == NULL) {
        failParamMessage(aTouchButton, "Button not found in list");
    } else {
        sButtonCombinedPoolSize++;
        tObjectPointer->mFlags &= ~FLAG_IS_ALLOCATED;
        tObjectPointer->mFlags &= ~FLAG_IS_ACTIVE;
    }
}
/**
 * prints amount of total, free and active buttons
 * @param aStringBuffer the string buffer for the result
 */
void TouchButton::infoButtonPool(char * aStringBuffer) {
    TouchButton * tObjectPointer = sListStart;
    int tTotal = 0;
    int tFree = 0;
    int tActive = 0;
// walk through list
    while (tObjectPointer != NULL) {
        tTotal++;
        if (!(tObjectPointer->mFlags & FLAG_IS_ALLOCATED)) {
            tFree++;
        }
        if (tObjectPointer->mFlags & FLAG_IS_ACTIVE) {
            tActive++;
        }
        tObjectPointer = tObjectPointer->mNextObject;
    }
    // output real and computed free values
    sprintf(aStringBuffer, "total=%2d free=%2d |%2d min=%2d active=%2d ", tTotal, tFree, sButtonCombinedPoolSize,
            sMinButtonPoolSize, tActive);
}

/****************************************
 * Plain util functions
 ****************************************/
#include "TouchSlider.h"

/*
 * helper variables for CheckTouchGeneric and swipe recognition with longTouchHandler and endTouchHandler
 */
int sLastGuiTouchState;
bool sAutorepeatButtonTouched = false; // flag if autorepeat button was touched - to influence long button press handling
bool sSliderTouched = false; // flag if slider was touched - to influence long button press handling
bool sEndTouchProcessed = false; // flag if end touch was done

volatile bool sCheckButtonsForEndTouch = false; // EndTouchHandler called from ISR signals with "true" thread to check GUI

/**
 * first check all buttons and then all sliders
 * if aCheckAlsoPlainButtons false then only elements are checked which internally require autorepeat
 * and therefore are not compatible with swipes
 * @param aCheckAlsoPlainButtons if false only sliders and autorepeat buttons are checked
 * @return sLastGuiTouchState
 */
int CheckTouchGeneric(bool aCheckAlsoPlainButtons) {
    int tReturn = GUI_NO_TOUCH;
    int tGuiTouched = BUTTON_NOT_TOUCHED;

    if (sCheckButtonsForEndTouch) {
        sCheckButtonsForEndTouch = false;
        tGuiTouched = TouchButton::checkAllButtons(TouchPanel.getXActual(), TouchPanel.getYActual(), true);
        if (tGuiTouched == BUTTON_TOUCHED_AUTOREPEAT) {
            sAutorepeatButtonTouched = true;
        }
        sEndTouchProcessed = true;
    } else {
        if (TouchPanel.wasTouched()) {
            // read it here, not at top of function, to avoid race conditions
            int x = TouchPanel.getXActual();
            int y = TouchPanel.getYActual();
            sSliderTouched = false;
            sAutorepeatButtonTouched = false;
            tReturn = GUI_TOUCH_NO_MATCH;
            /*
             * check if button or slider is touched
             */
            if (aCheckAlsoPlainButtons) {
                tGuiTouched = TouchButton::checkAllButtons(x, y, true);
                if (tGuiTouched == BUTTON_TOUCHED_AUTOREPEAT) {
                    sAutorepeatButtonTouched = true;
                }
            } else {
                // check only autorepeat buttons
                tGuiTouched = TouchButtonAutorepeat::checkAllButtons(x, y, true);
                if (tGuiTouched == BUTTON_TOUCHED_AUTOREPEAT) {
                    sAutorepeatButtonTouched = true;
                }
            }
            if (!tGuiTouched) {
                tGuiTouched = TouchSlider::checkAllSliders(x, y);
                // set flag that slider was touched
                sSliderTouched = tGuiTouched;
            }
        }
    }
    if (tGuiTouched != BUTTON_NOT_TOUCHED) {
        tReturn = GUI_TOUCH_MATCH;
    }
    sLastGuiTouchState = tReturn;
    return tReturn;
}

/**
 *
 * @param aTheTouchedButton
 * @param aValue assume as boolean here
 */
void doToggleRedGreenButton(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    aValue = !aValue;
    aTheTouchedButton->setRedGreenButtonColorAndDraw(aValue);
}

/** @} */
/** @} */
