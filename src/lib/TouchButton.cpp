/**
 * @file TouchButton.cpp
 *
 * Renders touch buttons for lcd
 * A button can be a simple clickable text
 * or a box with or without text or even an invisible touch area
 *
 * @date  30.01.2012
 * @author  Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 *  LCD interface used:
 * 		mDisplay->fillRect()
 * 		mDisplay->drawText()
 * 		DISPLAY_WIDTH
 * 		DISPLAY_HEIGHT
 *
 *
 */

#include "TouchButton.h"

#include "TouchButtonAutorepeat.h"
#include "TouchLib.h"
#include <stm32f30x.h>
#include <stdio.h>
#include <string.h> // for strlen
extern "C" {
#include "stm32f30xPeripherals.h"// for FeedbackToneOK
}

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Button
 * @{
 */
/**
 * The preallocated pool of buttons for this application
 */
TouchButton TouchButton::TouchButtonPool[NUMBER_OF_BUTTONS_IN_POOL];
uint8_t TouchButton::sButtonCombinedPoolSize = NUMBER_OF_BUTTONS_IN_POOL; // NUMBER_OF_AUTOREPEAT_BUTTONS_IN_POOL is added while initializing autorepeat pool
uint8_t TouchButton::sMinButtonPoolSize = NUMBER_OF_BUTTONS_IN_POOL;
bool TouchButton::sButtonPoolIsInitialized = false;

TouchButton * TouchButton::sListStart = NULL;
uint16_t TouchButton::sDefaultButtonColor = TOUCHBUTTON_DEFAULT_COLOR;
uint16_t TouchButton::sDefaultCaptionColor = TOUCHBUTTON_DEFAULT_CAPTION_COLOR;

TouchButton::TouchButton() {
    mDisplay = &BlueDisplay1;
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
        const uint16_t aHeightY, const uint16_t aButtonColor, const char * aCaption, const uint8_t aCaptionSize,
        const int16_t aValue, void (*aOnTouchHandler)(TouchButton * const, int16_t)) {
    return initButton(aPositionX, aPositionY, aWidthX, aHeightY, aCaption, aCaptionSize, aButtonColor, sDefaultCaptionColor, aValue,
            aOnTouchHandler);
}

/**
 * Set parameters for touch button
 * if aWidthX == 0 render only text no background box
 * if aCaptionSize == 0 don't render anything, just check touch area -> transparent button ;-)
 */
int8_t TouchButton::initButton(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX,
        const uint16_t aHeightY, const char * aCaption, const uint8_t aCaptionSize, const uint16_t aButtonColor,
        const uint16_t aCaptionColor, const int16_t aValue, void (*aOnTouchHandler)(TouchButton * const, int16_t)) {

    mWidth = aWidthX;
    mHeight = aHeightY;
    mButtonColor = aButtonColor;
    if (aButtonColor == 0) {
        mButtonColor = sDefaultButtonColor;
    }
    mCaptionColor = aCaptionColor;
    mCaption = aCaption;
    mCaptionSize = aCaptionSize;
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
        const uint16_t aHeightY, const uint16_t aButtonColor, const char * aCaption, const uint8_t aCaptionSize,
        const uint8_t aFlags, const int16_t aValue, void (*aOnTouchHandler)(TouchButton * const, int16_t)) {
    TouchButton * tButton = allocButton(false);

    tButton->mWidth = aWidthX;
    tButton->mHeight = aHeightY;
    tButton->mButtonColor = aButtonColor;
    if (aButtonColor == 0) {
        tButton->mButtonColor = sDefaultButtonColor;
    }
    tButton->mCaptionColor = sDefaultCaptionColor;
    tButton->mCaption = aCaption;
    tButton->mCaptionSize = aCaptionSize;
    tButton->mOnTouchHandler = aOnTouchHandler;
    tButton->mValue = aValue;
    tButton->setPosition(aPositionX, aPositionY);
    return tButton;
}

int8_t TouchButton::setPosition(const uint16_t aPositionX, const uint16_t aPositionY) {
    int8_t tRetValue = 0;
    mPositionX = aPositionX;
    mPositionY = aPositionY;

    // check values
    if (aPositionX + mWidth > mDisplay->getDisplayWidth()) {
        mWidth = mDisplay->getDisplayWidth() - aPositionX;
        failParamMessage(aPositionX + mWidth, "XRight wrong");
        tRetValue = TOUCHBUTTON_ERROR_X_RIGHT;
    }
    if (aPositionY + mHeight > mDisplay->getDisplayHeight()) {
        mHeight = mDisplay->getDisplayHeight() - mPositionY;
        failParamMessage(aPositionY + mHeight, "YBottom wrong");
        tRetValue = TOUCHBUTTON_ERROR_Y_BOTTOM;
    }
    return tRetValue;
}

/**
 * renders the button on lcd
 */
int8_t TouchButton::drawButton() {
    // Draw rect
    mDisplay->fillRectRel(mPositionX, mPositionY, mWidth, mHeight, mButtonColor);
    return drawCaption();
}

/**
 * deactivates the button and redraws its screen space with @a aBackgroundColor
 */
void TouchButton::removeButton(const uint16_t aBackgroundColor) {
    mFlags &= ~FLAG_IS_ACTIVE;
    // Draw rect
    mDisplay->fillRectRel(mPositionX, mPositionY, mWidth, mHeight, aBackgroundColor);

}

/**
 * draws the caption of a button
 * @retval 0 or error number #TOUCHBUTTON_ERROR_CAPTION_TOO_LONG etc.
 */
int TouchButton::drawCaption(void) {
    mFlags |= FLAG_IS_ACTIVE;
    int tRetValue = 0;
    if (mCaptionSize > 0) { // dont render anything if caption size == 0
        if (mCaption != NULL) {
            int tXCaptionPosition;
            int tYCaptionPosition;
            // try to position the string in the middle of the box
            int tLength = getTextWidth(mCaptionSize) * strlen(mCaption);
            if (tLength >= mWidth) {
                // String too long here
                tXCaptionPosition = mPositionX;
                tRetValue = TOUCHBUTTON_ERROR_CAPTION_TOO_LONG;
            } else {
                tXCaptionPosition = mPositionX + ((mWidth - tLength) / 2);
            }

            if (mCaptionSize >= mHeight) {
                // Font height to big
                tYCaptionPosition = mPositionY;
                tRetValue = TOUCHBUTTON_ERROR_CAPTION_TOO_HIGH;
            } else {
                tYCaptionPosition = mPositionY + ((mHeight + getTextAscendMinusDescend(mCaptionSize)) / 2);
            }
            mDisplay->drawText(tXCaptionPosition, tYCaptionPosition, mCaption, mCaptionSize, mCaptionColor, mButtonColor);

        }
    }
    return tRetValue;
}

/**
 * Check if touch event is in button area
 * if yes - return true
 * if no - return false
 */bool TouchButton::checkButtonInArea(const uint16_t aTouchPositionX, const uint16_t aTouchPositionY) {
    if (aTouchPositionX < mPositionX || aTouchPositionX > mPositionX + mWidth || aTouchPositionY < mPositionY
            || aTouchPositionY > (mPositionY + mHeight)) {
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
                sAutorepeatButtonTouched = true;
                return BUTTON_TOUCHED_AUTOREPEAT;
            }
            sButtonTouched = true;
            return BUTTON_TOUCHED;
        }
        tObjectPointer = tObjectPointer->mNextObject;
    }
    sAutorepeatButtonTouched = false;
    sButtonTouched = false;
    return NOT_TOUCHED;
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

const char * TouchButton::getCaption() const {
    return mCaption;
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
    return mPositionX + mWidth - 1;
}

uint16_t TouchButton::getPositionYBottom() const {
    return mPositionY + mHeight - 1;
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

/*
 * value 0 -> red, 1 -> green
 */
void TouchButton::setRedGreenButtonColor(int16_t aValue) {
    if (aValue != 0) {
        // map to boolean
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
 * Plain util c++ functions
 ****************************************/

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
