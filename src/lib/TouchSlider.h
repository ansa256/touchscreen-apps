/**
 *
 * @file TouchSlider.h
 *
 * @date 31.01.2012
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 *
 */

#ifndef TOUCHSLIDER_H_
#define TOUCHSLIDER_H_

#include <stdint.h>

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Slider
  * @{
  */
/**
 * @name SliderOptions
 * Options for TouchSlider::initSlider() - they can be or-ed together
 * @{
 */

#define TOUCHSLIDER_VERTICAL_SHOW_NOTHING 0x00
#define TOUCHSLIDER_SHOW_BORDER 0x01
#define TOUCHSLIDER_SHOW_VALUE 0x02
#define TOUCHSLIDER_IS_HORIZONTAL 0x04
#define TOUCHSLIDER_HORIZONTAL_VALUE_LEFT 0x08
/** @} */

#define TOUCHSLIDER_DEFAULT_SLIDER_COLOR 		RGB( 180, 180, 180)
#define TOUCHSLIDER_DEFAULT_BAR_COLOR 			COLOR_GREEN
#define TOUCHSLIDER_DEFAULT_BAR_THRESHOLD_COLOR COLOR_RED
#define TOUCHSLIDER_DEFAULT_BAR_BACK_COLOR 		COLOR_WHITE
#define TOUCHSLIDER_DEFAULT_CAPTION_COLOR 		COLOR_RED
#define TOUCHSLIDER_DEFAULT_VALUE_COLOR 		COLOR_BLUE
#define TOUCHSLIDER_DEFAULT_CAPTION_VALUE_BACK_COLOR 	COLOR_WHITE
#define TOUCHSLIDER_SIZE_FACTOR 				2 /** Factor for long border size -> pixel */
#define TOUCHSLIDER_OVERALL_SIZE_FACTOR 		(3 * TOUCHSLIDER_SIZE_FACTOR) /** factor for size -> pixel */
#define TOUCHSLIDER_DEFAULT_SIZE 				4
#define TOUCHSLIDER_MAX_SIZE 				    20 /** global max value of size parameter */
#define TOUCHSLIDER_DEFAULT_TOUCH_BORDER 		4 /** extension of touch region in pixel */
#define TOUCHSLIDER_DEFAULT_SHOW_CAPTION		true
#define TOUCHSLIDER_DEFAULT_SHOW_VALUE			true
#define TOUCHSLIDER_DEFAULT_MAX_VALUE           160
#define TOUCHSLIDER_DEFAULT_THRESHOLD_VALUE     100

#define TOUCHSLIDER_MAX_DISPLAY_VALUE			DISPLAY_WIDTH //! maximum value which can be displayed
/**
 * @name SliderErrorCodes
 * @{
 */
#define TOUCHSLIDER_ERROR_ACTUAL_VALUE 	-1
#define TOUCHSLIDER_ERROR_CAPTION_LENGTH -2
#define TOUCHSLIDER_ERROR_CAPTION_HEIGTH -4
#define TOUCHSLIDER_ERROR_VALUE_TOO_HIGH -8
#define TOUCHSLIDER_ERROR_X_RIGHT 		-16
#define TOUCHSLIDER_ERROR_Y_BOTTOM		-32
#define TOUCHSLIDER_ERROR_NOT_INITIALIZED -64
/** @} */

class TouchSlider {
public:
	/*
	 * Static functions
	 */
	TouchSlider();

	static void resetDefaults(void);
	static void setDefaults(const int8_t aDefaultTouchBorder, const uint16_t aDefaultSliderColor, const uint16_t aDefaultBarColor,
			const uint16_t aDefaultBarThresholdColor, const uint16_t aDefaultBarBackgroundColor,
			const uint16_t aDefaultCaptionColor, const uint16_t aDefaultValueColor,
			const uint16_t aDefaultValueCaptionBackgroundColor);
	static void setDefaultSliderColor(const uint16_t aDefaultSliderColor);
	static void setDefaultBarColor(const uint16_t aDefaultBarColor);
	static bool checkAllSliders(const int aTouchPositionX, const int aTouchPositionY);
	static bool checkActualSlider(const int aTouchPositionX, const int aTouchPositionY);
	static void deactivateAllSliders(void);
	static void activateAllSliders(void);
	/*
	 * Member functions
	 */
	int initSlider(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aSize, const uint16_t aMaxValue,
			const uint16_t aThresholdValue, const uint16_t aInitalValue, const char * aCaption, const int8_t aTouchBorder,
			const uint8_t aOptions, uint16_t (*aOnChangeHandler)(TouchSlider * const, const uint16_t),
			const char * (*aValueHandler)(uint16_t));
	void initSliderColors(const uint16_t aSliderColor, const uint16_t aBarColor, const uint16_t aBarThresholdColor,
			const uint16_t aBarBackgroundColor, const uint16_t aCaptionColor, const uint16_t aValueColor,
			const uint16_t aValueCaptionBackgroundColor);
	void drawSlider(void);
	bool checkSlider(const uint16_t aPositionX, const uint16_t aPositionY);
	void drawBar(void);
	void drawBorder(void);
	int16_t getActualValue(void) const;
	void setActualValue(int16_t actualValue);
	void setActualValueAndDraw(int16_t actualValue);
	void setActualValueAndDrawBar(int16_t actualValue);
	uint16_t getPositionXRight(void) const;
	uint16_t getPositionYBottom(void) const;
	void activate(void);
	void deactivate(void);
	uint16_t getBarColor(void) const;
	void setSliderColor(uint16_t sliderColor);
	void setBarColor(uint16_t barColor);
	void setBarThresholdColor(uint16_t barThresholdColor);
	void printCaption(void);
	uint16_t printValue(void);

private:
	// Start of list of touch slider
	static TouchSlider * sListStart;
	/*
	 * Defaults
	 */
	static uint16_t sDefaultSliderColor;
	static uint16_t sDefaultBarColor;
	static uint16_t sDefaultBarThresholdColor;
	static uint16_t sDefaultBarBackgroundColor;
	static uint16_t sDefaultCaptionColor;
	static uint16_t sDefaultValueColor;
	static uint16_t sDefaultValueCaptionBackgroundColor;
	static int8_t sDefaultTouchBorder;

	static TouchSlider * mLastSliderTouched; // For callback
	/*
	 * The Slider
	 */
	uint16_t mPositionX; // X left
	uint16_t mPositionXRight;
	uint16_t mPositionY; // Y top
	uint16_t mPositionYBottom;
	uint16_t mMaxValue; //aMaxValue serves also as height
	uint16_t mThresholdValue; // Value for color change
	uint16_t mSize; // Size of border and bar
	const char* mCaption; // No caption if NULL
	uint8_t mTouchBorder; // extension of touch region
	uint8_t mOptions;
	bool mIsActive;

	/*
	 * The Value
	 */
	uint16_t mActualTouchValue;
	// This value can be different from mActualTouchValue and is provided by callback handler
	int16_t mActualValue;
	// Colors
	uint16_t mSliderColor;
	uint16_t mBarColor;
	uint16_t mBarThresholdColor;
	uint16_t mBarBackgroundColor;
	uint16_t mCaptionColor;
	uint16_t mValueColor;
	uint16_t mValueCaptionBackgroundColor;
	// misc
	TouchSlider* mNextObject;
	uint16_t (*mOnChangeHandler)(TouchSlider* const, uint16_t);
	const char* (*mValueHandler)(uint16_t);

	int checkParameterValues();

};
/** @} */
/** @} */

#endif /* TOUCHSLIDER_H_ */
