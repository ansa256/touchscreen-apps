/**
 * @file TouchButtonAutorepeat.h
 *
 * Extension of ToucButton
 * implements autorepeat feature for touchbuttons
 *
 * @date  30.02.2012
 * @author  Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#ifndef TOUCHBUTTON_AUTOREPEAT_H_
#define TOUCHBUTTON_AUTOREPEAT_H_

#include "TouchButton.h"

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Button
 * @{
 */

// 6 are used in Page Setting
#define NUMBER_OF_AUTOREPEAT_BUTTONS_IN_POOL 6

class TouchButtonAutorepeat: public TouchButton {
public:

// static because only one touch possible
	static unsigned long sMillisOfLastCall;
	static unsigned long sMillisSinceFirstTouch;
	static unsigned long sMillisSinceLastCallback;

	static TouchButtonAutorepeat * allocButton(void);
	static TouchButtonAutorepeat * allocAndInitSimpleButton(const uint16_t aPositionX, const uint16_t aPositionY,
			const uint16_t aWidthX, const uint8_t aHeightY, const uint16_t aButtonColor, const char *aCaption,
			const uint8_t aCaptionSize, const int16_t aValue, void (*aOnTouchHandler)(TouchButton* const, int16_t));

	static int checkAllButtons(const int aTouchPositionX, const int aTouchPositionY, const bool doCallback);
	static void autorepeatTouchHandler(TouchButtonAutorepeat * const aTheTouchedButton, int16_t aButtonValue);
	static bool autorepeatButtonTimerHandler(const int aTouchPositionX, const int aTouchPositionY);
	void setButtonAutorepeatTiming(const uint16_t aMillisFirstDelay, const uint16_t aMillisFirstRate, const uint16_t aFirstCount,
			const uint16_t aMillisSecondRate);

	/*
	 * Static functions
	 */
	TouchButtonAutorepeat();

private:
	static TouchButtonAutorepeat TouchButtonAutorepeatPool[NUMBER_OF_AUTOREPEAT_BUTTONS_IN_POOL];
	static bool sButtonAutorepeatPoolIsInitialized; // used by allocButton

	uint16_t mMillisFirstDelay;
	uint16_t mMillisFirstRate;
	uint16_t mFirstCount;
	uint16_t mMillisSecondRate;
	void (*mOnTouchHandlerAutorepeat)(TouchButton * const, int16_t);
	static uint8_t sState;
	static uint16_t sCount;
	static TouchButtonAutorepeat * mLastAutorepeatButtonTouched; // For callback

};
/** @} */
/** @} */
#endif /* TOUCHBUTTON_AUTOREPEAT_H_ */
