/**
 * @file TouchButton.h
 *
 * Renders touch buttons for lcd
 * A button can be a simple clickable text
 * or a box with or without text
 *
 * @date  30.01.2012
 * @author  Armin Joachimsmeyer
 * armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 * 	40 Bytes per button on 32Bit ARM
 */

#ifndef TOUCHBUTTON_H_
#define TOUCHBUTTON_H_

#include "HY32D.h"
#include <stdint.h>
/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Button
 * @{
 */

// 45 needed for AccuCap/settings/Numberpad
// 47 needed for DSO/settings/calibrate/Numberpad
#define NUMBER_OF_BUTTONS_IN_POOL 47

#define BUTTON_DEFAULT_SPACING 16
#define BUTTON_DEFAULT_SPACING_HALF 8
#define BUTTON_DEFAULT_SPACING_QUARTER 4

#define BUTTON_WIDTH_2 152 // for 2 buttons horizontal - 19 characters
#define BUTTON_WIDTH_2_POS_2 (BUTTON_WIDTH_2 + BUTTON_DEFAULT_SPACING)

#define BUTTON_WIDTH_3 96 // for 3 buttons horizontal - 12 characters
#define BUTTON_WIDTH_3_POS_2 (BUTTON_WIDTH_3 + BUTTON_DEFAULT_SPACING)
#define BUTTON_WIDTH_3_POS_3 (DISPLAY_WIDTH - BUTTON_WIDTH_3)

#define BUTTON_WIDTH_4 68 // for 4 buttons horizontal - 8 characters
#define BUTTON_WIDTH_4_POS_2 (BUTTON_WIDTH_4 + BUTTON_DEFAULT_SPACING)
#define BUTTON_WIDTH_4_POS_3 (2*(BUTTON_WIDTH_4 + BUTTON_DEFAULT_SPACING))
#define BUTTON_WIDTH_4_POS_4 (DISPLAY_WIDTH - BUTTON_WIDTH_4))

#define BUTTON_WIDTH_5 51 // for 5 buttons horizontal 51,2  - 6 characters
#define BUTTON_WIDTH_5_POS_2 (BUTTON_WIDTH_5 + BUTTON_DEFAULT_SPACING)
#define BUTTON_WIDTH_5_POS_3 (2*(BUTTON_WIDTH_5 + BUTTON_DEFAULT_SPACING))
#define BUTTON_WIDTH_5_POS_4 (3*(BUTTON_WIDTH_5 + BUTTON_DEFAULT_SPACING))
#define BUTTON_WIDTH_5_POS_5 (DISPLAY_WIDTH - BUTTON_WIDTH_5)

#define BUTTON_WIDTH_6 40 // for 6 buttons horizontal
#define BUTTON_WIDTH_6_POS_2 (BUTTON_WIDTH_6 + BUTTON_DEFAULT_SPACING)
#define BUTTON_WIDTH_6_POS_3 (2*(BUTTON_WIDTH_6 + BUTTON_DEFAULT_SPACING))
#define BUTTON_WIDTH_6_POS_4 (3*(BUTTON_WIDTH_6 + BUTTON_DEFAULT_SPACING))
#define BUTTON_WIDTH_6_POS_5 (4*(BUTTON_WIDTH_6 + BUTTON_DEFAULT_SPACING))
#define BUTTON_WIDTH_6_POS_6 (DISPLAY_WIDTH - BUTTON_WIDTH_6)

#define BUTTON_WIDTH_8 33 // for 8 buttons horizontal
#define BUTTON_WIDTH_10 28 // for 10 buttons horizontal

#define BUTTON_HEIGHT_4 48 // for 4 buttons vertical
#define BUTTON_HEIGHT_4_LINE_2 (BUTTON_HEIGHT_4 + BUTTON_DEFAULT_SPACING)
#define BUTTON_HEIGHT_4_LINE_3 (2*(BUTTON_HEIGHT_4 + BUTTON_DEFAULT_SPACING))
#define BUTTON_HEIGHT_4_LINE_4 (DISPLAY_HEIGHT - BUTTON_HEIGHT_4)

#define BUTTON_HEIGHT_5 35 // for 5 buttons vertical
#define BUTTON_HEIGHT_5_LINE_2 (BUTTON_HEIGHT_5 + BUTTON_DEFAULT_SPACING)
#define BUTTON_HEIGHT_5_LINE_3 (2*(BUTTON_HEIGHT_5 + BUTTON_DEFAULT_SPACING))
#define BUTTON_HEIGHT_5_LINE_4 (3*(BUTTON_HEIGHT_5 + BUTTON_DEFAULT_SPACING))
#define BUTTON_HEIGHT_5_LINE_5 (DISPLAY_HEIGHT - BUTTON_HEIGHT_5)

#define BUTTON_HEIGHT_6 26 // for 6 buttons vertical 26,66..
#define TOUCHBUTTON_DEFAULT_COLOR 			RGB( 180, 180, 180)
#define TOUCHBUTTON_DEFAULT_CAPTION_COLOR 	COLOR_BLACK
#define TOUCHBUTTON_DEFAULT_TOUCH_BORDER 	2 // extension of touch region
// Error codes
#define TOUCHBUTTON_ERROR_X_RIGHT 			-1
#define TOUCHBUTTON_ERROR_Y_BOTTOM 			-2
#define TOUCHBUTTON_ERROR_CAPTION_TOO_LONG	-3
#define TOUCHBUTTON_ERROR_CAPTION_TOO_HIGH	-4
#define TOUCHBUTTON_ERROR_NOT_INITIALIZED   -64

#define ERROR_STRING_X_RIGHT 			"X right > 320"
#define ERROR_STRING_Y_BOTTOM 			"Y bottom > 240"
#define ERROR_STRING_CAPTION_TOO_LONG	"Caption too long"
#define ERROR_STRING_CAPTION_TOO_HIGH	"Caption too high"
#define ERROR_STRING_NOT_INITIALIZED   -64

#define FLAG_IS_AUTOREPEAT_BUTTON 0x01 // since compiler flag "-fno-rtti" disables typeid()
#define FLAG_IS_ALLOCATED 0x02 // for use with get and releaseButton
#define FLAG_IS_ACTIVE 0x04 // Button is checked by checkAllButtons() etc.
#define FLAG_IS_ONLY_CAPTION 0x08 // Button is as big as caption
// return values for checkAllButtons()
#define BUTTON_NOT_TOUCHED 0
#define BUTTON_TOUCHED 1
#define BUTTON_TOUCHED_AUTOREPEAT 2 // an autorepeat button was touched

int CheckTouchGeneric(bool aCheckAlsoPlainButtons);

/*
 * helper variables for swipe recognition with longTouchHandler and endTouchHandler
 */
extern int sLastGuiTouchState;
extern bool sAutorepeatButtonTouched;
extern bool sSliderTouched;
extern bool sEndTouchProcessed;

extern volatile bool sDisableEndTouchOnce;
extern volatile bool sCheckButtonsForEndTouch;

#define GUI_NO_TOUCH 0			// No touch happened
#define GUI_TOUCH_NO_MATCH 1	// Touch happened but no Gui element matched
#define GUI_TOUCH_MATCH 2		// Touch happened and Gui element matched (was touched)
class TouchButton {
public:

	TouchButton();
	// Pool stuff
	static TouchButton * allocButton(bool aOnlyAutorepeatButtons);
	static void freeButton(TouchButton * aTouchButton);
	static void infoButtonPool(char * aStringBuffer);
	static TouchButton * allocAndInitSimpleButton(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX,
			const uint8_t aHeightY, const uint16_t aButtonColor, const char *aCaption, const uint8_t aCaptionSize,
			const int16_t aValue, void (*aOnTouchHandler)(TouchButton* const, int16_t));
	void setAllocated(bool isAllocated);
	void setFree(void);

	static void setDefaultTouchBorder(const uint8_t aDefaultTouchBorder);
	static void setDefaultButtonColor(const uint16_t aDefaultButtonColor);
	static void setDefaultCaptionColor(const uint16_t aDefaultCaptionColor);
	static int checkAllButtons(const int aTouchPositionX, const int aTouchPositionY, const bool doCallback);
	static void activateAllButtons(void);
	static void deactivateAllButtons(void);

	int8_t initSimpleButton(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX, const uint8_t aHeightY,
			const uint16_t aButtonColor, const char *aCaption, const uint8_t aCaptionSize, const int16_t aValue,
			void (*aOnTouchHandler)(TouchButton* const, int16_t));
	int8_t initButton(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX, const uint8_t aHeightY,
			const char *aCaption, const uint8_t aCaptionSize, const uint8_t aTouchBorder, const uint16_t aButtonColor,
			const uint16_t aCaptionColor, const int16_t aValue, void (*aOnTouchHandler)(TouchButton* const, int16_t));

	bool checkButton(const int aTouchPositionX, const int aTouchPositionY, const bool doCallback);

	bool checkButtonInArea(const uint16_t aTouchPositionX, const uint16_t aTouchPositionY);
	int8_t drawButton(void);
	void removeButton(const uint16_t aBackgroundColor);
	int drawCaption(void);
	int8_t setPosition(const uint16_t aPositionX, const uint16_t aPositionY);
	void setColor(const uint16_t aColor);
	void setCaption(const char *aCaption);
	void setCaptionColor(const uint16_t aColor);
	void setValue(const int16_t aValue);
	const char *getCaption(void) const;
	uint16_t getValue() const;
	uint16_t getPositionX(void) const;
	uint16_t getPositionY(void) const;
	uint16_t getPositionXRight(void) const;
	uint16_t getPositionYBottom(void) const;
	int8_t setPositionX(const uint16_t aPositionX);
	int8_t setPositionY(const uint16_t aPositionY);
	void activate(void);
	void deactivate(void);
	uint8_t getTouchBorder(void) const;
	void setTouchBorder(const uint8_t touchBorder);
	void setTouchHandler(void (*aOnTouchHandler)(TouchButton* const, int16_t));

	bool isAllocated(void) const;

private:
	// Pool stuff
	static TouchButton TouchButtonPool[NUMBER_OF_BUTTONS_IN_POOL];
	static bool sButtonPoolIsInitialized; // used by allocButton

	uint16_t mButtonColor;
	uint16_t mCaptionColor;
	uint16_t mPositionX;
	uint16_t mPositionY;
	uint16_t mWidth;
	uint8_t mHeight;
	uint8_t mCaptionSize;
	uint8_t mTouchBorder;
	uint16_t mPositionXRight;
	uint16_t mPositionYBottom;
	const char *mCaption; // Pointer to caption

protected:
	static uint16_t sDefaultButtonColor;
	static uint16_t sDefaultCaptionColor;
	static uint8_t sDefaultTouchBorder;

	static uint8_t sButtonCombinedPoolSize;
	static uint8_t sMinButtonPoolSize;

	static TouchButton *sListStart; // Root pointer to list of all buttons

	uint8_t mFlags; //Flag for: Autorepeat type, allocated, only caption
	TouchButton *mNextObject;

	int16_t mValue;
	uint8_t getCaptionLength(char * aCaptionPointer) const;
	void (*mOnTouchHandler)(TouchButton * const, int16_t);

};
/** @} */
/** @} */

#endif /* TOUCHBUTTON_H_ */
