/**
 * Slider with value as unsigned byte value
 *
 * @file
 * @date 30.01.2012
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 *
 *
 *  LCD interface used:
 * 		- getHeight()
 * 		- getWidth()
 * 		- mDisplay->fillRect()
 * 		- mDisplay->drawText()
 *
 */

#include "TouchSlider.h"

#include "TouchLib.h"

#include "misc.h"
#include <stdio.h>
#include <string.h>

extern "C" {
#include "assert.h"
}

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Slider
 * @{
 */

TouchSlider * TouchSlider::sListStart = NULL;
uint16_t TouchSlider::sDefaultSliderColor;
uint16_t TouchSlider::sDefaultBarColor;
uint16_t TouchSlider::sDefaultBarThresholdColor;
uint16_t TouchSlider::sDefaultBarBackgroundColor;
uint16_t TouchSlider::sDefaultCaptionColor;
uint16_t TouchSlider::sDefaultValueColor;
uint16_t TouchSlider::sDefaultValueCaptionBackgroundColor;
int8_t TouchSlider::sDefaultTouchBorder;

//TouchSlider * TouchSlider::mLastSliderTouched = NULL;

TouchSlider::TouchSlider(void) {
    mDisplay = &BlueDisplay1;
    mNextObject = NULL;
    if (sListStart == NULL) {
        // first button
        resetDefaults();
        sListStart = this;
    } else {
        // put object in button list
        TouchSlider * tObjectPointer = sListStart;
        // search last list element
        while (tObjectPointer->mNextObject != NULL) {
            tObjectPointer = tObjectPointer->mNextObject;
        }
        //insert actual button in last element
        tObjectPointer->mNextObject = this;
    }
    mBarThresholdColor = sDefaultBarThresholdColor;
    mBarBackgroundColor = sDefaultBarBackgroundColor;
    mCaptionColor = sDefaultCaptionColor;
    mValueColor = sDefaultValueColor;
    mValueCaptionBackgroundColor = sDefaultValueCaptionBackgroundColor;
    mTouchBorder = sDefaultTouchBorder;
    mXOffsetValue = 0;
}

/**
 * Static initialization of slider default colors
 */
void TouchSlider::setDefaults(const int8_t aDefaultTouchBorder, const uint16_t aDefaultSliderColor, const uint16_t aDefaultBarColor,
        const uint16_t aDefaultBarThresholdColor, const uint16_t aDefaultBarBackgroundColor, const uint16_t aDefaultCaptionColor,
        const uint16_t aDefaultValueColor, const uint16_t aDefaultValueCaptionBackgroundColor) {
    sDefaultSliderColor = aDefaultSliderColor;
    sDefaultBarColor = aDefaultBarColor;
    sDefaultBarThresholdColor = aDefaultBarThresholdColor;
    sDefaultBarBackgroundColor = aDefaultBarBackgroundColor;
    sDefaultCaptionColor = aDefaultCaptionColor;
    sDefaultValueColor = aDefaultValueColor;
    sDefaultValueCaptionBackgroundColor = aDefaultValueCaptionBackgroundColor;
    sDefaultTouchBorder = aDefaultTouchBorder;
}

/**
 * Static initialization of slider default colors
 */
void TouchSlider::resetDefaults(void) {
    sDefaultSliderColor = TOUCHSLIDER_DEFAULT_SLIDER_COLOR;
    sDefaultBarColor = TOUCHSLIDER_DEFAULT_BAR_COLOR;
    sDefaultBarThresholdColor = TOUCHSLIDER_DEFAULT_BAR_THRESHOLD_COLOR;
    sDefaultBarBackgroundColor = TOUCHSLIDER_DEFAULT_BAR_BACK_COLOR;
    sDefaultCaptionColor = TOUCHSLIDER_DEFAULT_CAPTION_COLOR;
    sDefaultValueColor = TOUCHSLIDER_DEFAULT_VALUE_COLOR;
    sDefaultValueCaptionBackgroundColor = TOUCHSLIDER_DEFAULT_CAPTION_VALUE_BACK_COLOR;
    sDefaultTouchBorder = TOUCHSLIDER_DEFAULT_TOUCH_BORDER;
}

void TouchSlider::setDefaultSliderColor(const uint16_t aDefaultSliderColor) {
    sDefaultSliderColor = aDefaultSliderColor;
}

void TouchSlider::setDefaultBarColor(const uint16_t aDefaultBarColor) {
    sDefaultBarColor = aDefaultBarColor;
}

void TouchSlider::initSliderColors(const uint16_t aSliderColor, const uint16_t aBarColor, const uint16_t aBarThresholdColor,
        const uint16_t aBarBackgroundColor, const uint16_t aCaptionColor, const uint16_t aValueColor,
        const uint16_t aValueCaptionBackgroundColor) {
    mSliderColor = aSliderColor;
    mBarColor = aBarColor;
    mBarThresholdColor = aBarThresholdColor;
    mBarBackgroundColor = aBarBackgroundColor;
    mCaptionColor = aCaptionColor;
    mValueColor = aValueColor;
    mValueCaptionBackgroundColor = aValueCaptionBackgroundColor;
}

void TouchSlider::setValueAndCaptionBackgroundColor(const uint16_t aValueCaptionBackgroundColor) {
    mValueCaptionBackgroundColor = aValueCaptionBackgroundColor;
}

void TouchSlider::setValueColor(const uint16_t aValueColor) {
    mValueColor = aValueColor;
}

/**
 * Static convenience method - activate all sliders
 * @see deactivateAllSliders()
 */
void TouchSlider::activateAllSliders(void) {
    TouchSlider * tObjectPointer = sListStart;
    while (tObjectPointer != NULL) {
        tObjectPointer->activate();
        tObjectPointer = tObjectPointer->mNextObject;
    }
}

/**
 * Static convenience method - deactivate all sliders
 * @see activateAllSliders()
 */
void TouchSlider::deactivateAllSliders(void) {
    TouchSlider * tObjectPointer = sListStart;
    while (tObjectPointer != NULL) {
        tObjectPointer->deactivate();
        tObjectPointer = tObjectPointer->mNextObject;
    }
}

/**
 * @brief initialization with all parameters except color
 * @param aPositionX determines upper left corner
 * @param aPositionY determines upper left corner
 * @param aSize size of border and bar in pixel * #TOUCHSLIDER_OVERALL_SIZE_FACTOR
 * @param aMaxValue size of slider bar in pixel
 * @param aThresholdValue value where color of bar changes from #TOUCHSLIDER_DEFAULT_BAR_COLOR to #TOUCHSLIDER_DEFAULT_BAR_THRESHOLD_COLOR
 * @param aInitalValue
 * @param aCaption if NULL no caption is drawn
 * @param aTouchBorder border in pixel, where touches are recognized
 * @param aOptions see #TOUCHSLIDER_SHOW_BORDER etc.
 * @param aOnChangeHandler - if NULL no update of bar is done on touch
 * @param aValueHandler Handler to convert actualValue to string - if NULL sprintf("%3d", mActualValue) is used
 *  * @return false if parameters are not consistent ie. are internally modified
 *  or if not enough space to draw caption or value.
 */
void TouchSlider::initSlider(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aSize, const uint16_t aMaxValue,
        const uint16_t aThresholdValue, const uint16_t aInitalValue, const char * aCaption, const int8_t aTouchBorder,
        const uint8_t aOptions, uint16_t (*aOnChangeHandler)(TouchSlider * const, const uint16_t),
        const char * (*aValueHandler)(uint16_t)) {

    mSliderColor = sDefaultSliderColor;
    mBarColor = sDefaultBarColor;
    /*
     * Copy parameter
     */
    mPositionX = aPositionX;
    mPositionY = aPositionY;
    mOptions = aOptions;
    mCaption = aCaption;
    mSize = aSize;
    mMaxValue = aMaxValue;
    mActualValue = aInitalValue;
    mThresholdValue = aThresholdValue;
    mTouchBorder = aTouchBorder;
    mOnChangeHandler = aOnChangeHandler;
    mValueHandler = aValueHandler;
    if (mValueHandler != NULL) {
        mOptions |= TOUCHSLIDER_SHOW_VALUE;
    }

    checkParameterValues();

    uint8_t tSizeOfBorders = 2 * mSize;
    if (!(mOptions & TOUCHSLIDER_SHOW_BORDER)) {
        tSizeOfBorders = 0;
    }

    /*
     * compute lower right corner and validate
     */
    if (mOptions & TOUCHSLIDER_IS_HORIZONTAL) {
        mPositionXRight = mPositionX + mMaxValue + tSizeOfBorders - 1;
        if (mPositionXRight >= mDisplay->getDisplayWidth()) {
            // simple fallback
            mSize = 1;
            mPositionX = 0;
            failParamMessage(mPositionXRight, "XRight wrong");
            mPositionXRight = mDisplay->getDisplayWidth() - 1;
        }
        mPositionYBottom = mPositionY + ((tSizeOfBorders + mSize) * TOUCHSLIDER_SIZE_FACTOR) - 1;
        if (mPositionYBottom >= mDisplay->getDisplayHeight()) {
            // simple fallback
            mSize = 1;
            mPositionY = 0;
            failParamMessage(mPositionYBottom, "YBottom wrong");
            mPositionYBottom = mDisplay->getDisplayHeight() - 1;
        }

    } else {
        mPositionXRight = mPositionX + ((tSizeOfBorders + mSize) * TOUCHSLIDER_SIZE_FACTOR) - 1;
        if (mPositionXRight >= mDisplay->getDisplayWidth()) {
            // simple fallback
            mSize = 1;
            mPositionX = 0;
            failParamMessage(mPositionXRight, "XRight wrong");
            mPositionXRight = mDisplay->getDisplayWidth() - 1;
        }
        mPositionYBottom = mPositionY + mMaxValue + tSizeOfBorders - 1;
        if (mPositionYBottom >= mDisplay->getDisplayHeight()) {
            // simple fallback
            mSize = 1;
            mPositionY = 0;
            failParamMessage(mPositionYBottom, "YBottom wrong");
            mPositionYBottom = mDisplay->getDisplayHeight() - 1;
        }
    }
}

void TouchSlider::drawSlider(void) {
    mIsActive = true;

    if ((mOptions & TOUCHSLIDER_SHOW_BORDER)) {
        drawBorder();
    }

    // Fill middle bar with initial value
    drawBar();
    printCaption();
    // Print value as string
    printValue();
}

void TouchSlider::drawBorder(void) {
    if (mOptions & TOUCHSLIDER_IS_HORIZONTAL) {
        // Create value bar upper border
        mDisplay->fillRectRel(mPositionX, mPositionY, mMaxValue + (2 * mSize), TOUCHSLIDER_SIZE_FACTOR * mSize, mSliderColor);
        // Create value bar lower border
        mDisplay->fillRectRel(mPositionX, mPositionY + (2 * TOUCHSLIDER_SIZE_FACTOR * mSize), mMaxValue + (2 * mSize),
                TOUCHSLIDER_SIZE_FACTOR * mSize, mSliderColor);

        // Create left border
        mDisplay->fillRectRel(mPositionX, mPositionY + (TOUCHSLIDER_SIZE_FACTOR * mSize), mSize, TOUCHSLIDER_SIZE_FACTOR * mSize,
                mSliderColor);
        // Create right border
        mDisplay->fillRectRel(mPositionXRight - mSize + 1, mPositionY + (TOUCHSLIDER_SIZE_FACTOR * mSize), mSize,
                TOUCHSLIDER_SIZE_FACTOR * mSize, mSliderColor);
    } else {
        // Create left border
        mDisplay->fillRectRel(mPositionX, mPositionY, TOUCHSLIDER_SIZE_FACTOR * mSize, mMaxValue + (2 * mSize), mSliderColor);
        // Create right border
        mDisplay->fillRectRel(mPositionX + (2 * TOUCHSLIDER_SIZE_FACTOR * mSize), mPositionY, TOUCHSLIDER_SIZE_FACTOR * mSize,
                mMaxValue + (2 * mSize), mSliderColor);

        // Create value bar upper border
        mDisplay->fillRectRel(mPositionX + (TOUCHSLIDER_SIZE_FACTOR * mSize), mPositionY, TOUCHSLIDER_SIZE_FACTOR * mSize, mSize,
                mSliderColor);
        // Create value bar lower border
        mDisplay->fillRectRel(mPositionX + (TOUCHSLIDER_SIZE_FACTOR * mSize), mPositionYBottom - mSize + 1,
                TOUCHSLIDER_SIZE_FACTOR * mSize, mSize, mSliderColor);
    }
}

/*
 * (re)draws the middle bar according to actual value
 */
void TouchSlider::drawBar(void) {
    int tActualValue = mActualValue;
    if (tActualValue > mMaxValue) {
        tActualValue = mMaxValue;
    }
    if (tActualValue < 0) {
        tActualValue = 0;
    }

    uint8_t tBorderSize = 0;
    if ((mOptions & TOUCHSLIDER_SHOW_BORDER)) {
        tBorderSize = mSize;
    }

// draw background bar
    if (tActualValue < mMaxValue) {
        if (mOptions & TOUCHSLIDER_IS_HORIZONTAL) {
            mDisplay->fillRectRel(mPositionX + tBorderSize + tActualValue, mPositionY + (tBorderSize * TOUCHSLIDER_SIZE_FACTOR),
                    mMaxValue - tActualValue, mSize * TOUCHSLIDER_SIZE_FACTOR, mBarBackgroundColor);
        } else {
            mDisplay->fillRectRel(mPositionX + (tBorderSize * TOUCHSLIDER_SIZE_FACTOR), mPositionY + tBorderSize,
                    mSize * TOUCHSLIDER_SIZE_FACTOR, mMaxValue - tActualValue, mBarBackgroundColor);
        }
    }

// Draw value bar
    if (tActualValue > 0) {
        int tColor = mBarColor;
        if (tActualValue > mThresholdValue) {
            tColor = mBarThresholdColor;
        }
        if (mOptions & TOUCHSLIDER_IS_HORIZONTAL) {
            mDisplay->fillRectRel(mPositionX + tBorderSize, mPositionY + (tBorderSize * TOUCHSLIDER_SIZE_FACTOR), tActualValue,
                    mSize * TOUCHSLIDER_SIZE_FACTOR, tColor);
        } else {
            mDisplay->fillRectRel(mPositionX + (tBorderSize * TOUCHSLIDER_SIZE_FACTOR),
                    mPositionYBottom - tBorderSize - tActualValue + 1, mSize * TOUCHSLIDER_SIZE_FACTOR, tActualValue, tColor);
        }
    }
}

/**
 * Print caption in the middle below slider
 */
void TouchSlider::printCaption(void) {
    if (mCaption == NULL) {
        return;
    }
    int tCaptionLengthPixel = strlen(mCaption) * TEXT_SIZE_11_WIDTH;
    if (tCaptionLengthPixel == 0) {
        mCaption = NULL;
    }

    int tSliderWidthPixel;
    if (mOptions & TOUCHSLIDER_IS_HORIZONTAL) {
        tSliderWidthPixel = mMaxValue;
        if ((mOptions & TOUCHSLIDER_SHOW_BORDER)) {
            tSliderWidthPixel += 2 * mSize;
        }
    } else {
        tSliderWidthPixel = mSize * TOUCHSLIDER_SIZE_FACTOR;
        if ((mOptions & TOUCHSLIDER_SHOW_BORDER)) {
            tSliderWidthPixel = 3 * tSliderWidthPixel;
        }
    }
    /**
     * try to position the string in the middle below slider
     */
    int tCaptionPositionX = mPositionX + (tSliderWidthPixel / 2) - (tCaptionLengthPixel / 2);
    // adjust for left and right display border
    if (tCaptionPositionX < 0) {
        tCaptionPositionX = 0;
    }
    if (tCaptionPositionX + tCaptionLengthPixel >= mDisplay->getDisplayWidth()) {
        tCaptionPositionX = mDisplay->getDisplayWidth() - tCaptionLengthPixel;
    }

// space for caption?
    int tCaptionPositionY = mPositionYBottom + mSize + getTextAscend(TEXT_SIZE_11);
    if (tCaptionPositionY > mDisplay->getDisplayHeight() - TEXT_SIZE_11_DECEND) {
        // fallback
        failParamMessage(tCaptionPositionY, "Caption Bottom wrong");
        tCaptionPositionY = mDisplay->getDisplayHeight() - TEXT_SIZE_11_DECEND;
    }

    mDisplay->drawText(tCaptionPositionX, tCaptionPositionY, (char *) mCaption, TEXT_SIZE_11, mCaptionColor,
            mValueCaptionBackgroundColor);
}

/**
 * Print value left aligned to slider below caption or beneath if TOUCHSLIDER_HORIZONTAL_VALUE_LEFT
 */
int TouchSlider::printValue(void) {
    if (!(mOptions & TOUCHSLIDER_SHOW_VALUE)) {
        return 0;
    }
    const char * pValueAsString;
    int tValuePositionY = mPositionYBottom + mSize + getTextAscend(TEXT_SIZE_11);
    if (mCaption != NULL && !((mOptions & TOUCHSLIDER_IS_HORIZONTAL) && !(mOptions & TOUCHSLIDER_HORIZONTAL_VALUE_BELOW_TITLE))) {
        tValuePositionY += TEXT_SIZE_11_HEIGHT;
    }

    if (tValuePositionY > mDisplay->getDisplayHeight() - TEXT_SIZE_11_DECEND) {
        // fallback
        failParamMessage(tValuePositionY, "Value Bottom wrong");
        tValuePositionY = mDisplay->getDisplayHeight() - TEXT_SIZE_11_DECEND;
    }
    pValueAsString = StringBuffer;
    if (mValueHandler == NULL) {
        snprintf(StringBuffer, sizeof StringBuffer, "%3d", mActualValue);
    } else {
        // mValueHandler has to provide the char array
        pValueAsString = mValueHandler(mActualValue);
    }
    return mDisplay->drawText(mPositionX + mXOffsetValue, tValuePositionY, pValueAsString, TEXT_SIZE_11, mValueColor,
            mValueCaptionBackgroundColor);
}

/**
 * Check if touch event is in slider
 * - if yes - set bar and value call callback function and return true
 * - if no - return false
 */

bool TouchSlider::checkSlider(const uint16_t aTouchPositionX, const uint16_t aTouchPositionY) {
    int tPositionBorderX = mPositionX - mTouchBorder;
    if (mTouchBorder > mPositionX) {
        tPositionBorderX = 0;
    }
    int tPositionBorderY = mPositionY - mTouchBorder;
    if (mTouchBorder > mPositionY) {
        tPositionBorderY = 0;
    }
    if (!mIsActive || aTouchPositionX < tPositionBorderX || aTouchPositionX > mPositionXRight + mTouchBorder
            || aTouchPositionY < tPositionBorderY || aTouchPositionY > mPositionYBottom + mTouchBorder) {
        return false;
    }
    int tTinyBorderSize = 0;
    if ((mOptions & TOUCHSLIDER_SHOW_BORDER)) {
        tTinyBorderSize = mSize;
    }
    /*
     *  Touch position is in slider (plus additional touch border) here
     */
    int tActualTouchValue;
    if (mOptions & TOUCHSLIDER_IS_HORIZONTAL) {
        if (aTouchPositionX < (mPositionX + tTinyBorderSize)) {
            tActualTouchValue = 0;
        } else if (aTouchPositionX > (mPositionXRight - tTinyBorderSize)) {
            tActualTouchValue = mMaxValue;
        } else {
            tActualTouchValue = aTouchPositionX - mPositionX - tTinyBorderSize + 1;
        }
    } else {
// adjust value according to size of upper and lower border
        if (aTouchPositionY > (mPositionYBottom - tTinyBorderSize)) {
            tActualTouchValue = 0;
        } else if (aTouchPositionY < (mPositionY + tTinyBorderSize)) {
            tActualTouchValue = mMaxValue;
        } else {
            tActualTouchValue = mPositionYBottom - tTinyBorderSize - aTouchPositionY + 1;
        }
    }

    if (tActualTouchValue != mActualTouchValue) {
        mActualTouchValue = tActualTouchValue;
        if (mOnChangeHandler != NULL) {
            // call change handler and take the result as new value
            tActualTouchValue = mOnChangeHandler(this, tActualTouchValue);

            if (tActualTouchValue == mActualValue) {
                // value returned is equal displayed value - do nothing
                return true;
            }
            // display value changed - check, store and redraw
            if (tActualTouchValue > mMaxValue) {
                tActualTouchValue = mMaxValue;
            }
            mActualValue = tActualTouchValue;
            drawBar();
            printValue();
        }
    }
    return true;
}

/**
 * Static convenience method - checks all buttons in  array sManagesButtonsArray for events.
 */

bool TouchSlider::checkAllSliders(const int aTouchPositionX, const int aTouchPositionY) {
    TouchSlider * tObjectPointer = sListStart;

// walk through list of active elements
    while (tObjectPointer != NULL) {
        if (tObjectPointer->mIsActive && tObjectPointer->checkSlider(aTouchPositionX, aTouchPositionY)) {
            return true;
        }
        tObjectPointer = tObjectPointer->mNextObject;
    }
    return false;
}

/**
 * Checks the slider last touched for callback by SysTick timer.
 */

//bool TouchSlider::checkActualSlider(const int aTouchPositionX, const int aTouchPositionY) {
//    assert_param(mLastSliderTouched != NULL);
//    if (mLastSliderTouched->mIsActive && mLastSliderTouched->checkSlider(aTouchPositionX, aTouchPositionY)) {
//        return true;
//    }
//    return false;
//}
int16_t TouchSlider::getActualValue() const {
    return mActualValue;
}

void TouchSlider::setActualValue(int16_t actualValue) {
    mActualValue = actualValue;
}

void TouchSlider::setActualValueAndDraw(int16_t actualValue) {
    mActualValue = actualValue;
    drawBar();
    printValue();
}

void TouchSlider::setActualValueAndDrawBar(int16_t actualValue) {
    mActualValue = actualValue;
    drawBar();
}

void TouchSlider::setXOffsetValue(int16_t aXOffsetValue) {
    mXOffsetValue = aXOffsetValue;
}

uint16_t TouchSlider::getPositionXRight() const {
    return mPositionXRight;
}

uint16_t TouchSlider::getPositionYBottom() const {
    return mPositionYBottom;
}

void TouchSlider::activate(void) {
    mIsActive = true;
}
void TouchSlider::deactivate(void) {
    mIsActive = false;
}

int TouchSlider::checkParameterValues(void) {
    /*
     * Check and copy parameter
     */
    int tRetValue = 0;

    if (mSize == 0) {
        mSize = TOUCHSLIDER_DEFAULT_SIZE;
    } else if (mSize > TOUCHSLIDER_MAX_SIZE) {
        mSize = TOUCHSLIDER_MAX_SIZE;
    }
    if (mMaxValue == 0) {
        mMaxValue = TOUCHSLIDER_DEFAULT_MAX_VALUE;
    }
    if (mActualValue > mMaxValue) {
        tRetValue = TOUCHSLIDER_ERROR_ACTUAL_VALUE;
        mActualValue = mMaxValue;
    }

    return tRetValue;
}

void TouchSlider::setBarThresholdColor(uint16_t barThresholdColor) {
    mBarThresholdColor = barThresholdColor;
}

void TouchSlider::setSliderColor(uint16_t sliderColor) {
    mSliderColor = sliderColor;
}

void TouchSlider::setBarColor(uint16_t barColor) {
    mBarColor = barColor;
}
/** @} */
/** @} */
