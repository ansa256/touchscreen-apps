/**
 * @file TouchButton.h
 *
 * Renders touch buttons for lcd
 * A button can be a simple clickable text
 * or a box with or without text
 *
 * @date  30.01.2012
 * @author  Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 * 	40 Bytes per button on 32Bit ARM
 */

#ifndef TOUCHBUTTON_H_
#define TOUCHBUTTON_H_

#include "BlueDisplay.h"
#include <stdint.h>
/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Button
 * @{
 */

// 45 needed for AccuCap/settings/Numberpad
// 50 needed for DSO/settings/calibrate/Numberpad
#define NUMBER_OF_BUTTONS_IN_POOL 50

#define TOUCHBUTTON_DEFAULT_COLOR 			RGB( 180, 180, 180)
#define TOUCHBUTTON_DEFAULT_CAPTION_COLOR 	COLOR_BLACK
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
// return values for checkAllButtons()
#define NOT_TOUCHED false
#define BUTTON_TOUCHED 1
#define BUTTON_TOUCHED_AUTOREPEAT 2 // an autorepeat button was touched
class TouchButton {
public:

    TouchButton();
    // Pool stuff
    static TouchButton * allocButton(bool aOnlyAutorepeatButtons);
    static void freeButton(TouchButton * aTouchButton);
    static void infoButtonPool(char * aStringBuffer);
    static TouchButton * allocAndInitSimpleButton(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX,
            const uint16_t aHeightY, const uint16_t aButtonColor, const char *aCaption, const uint8_t aCaptionSize,
            const uint8_t aFlags, const int16_t aValue, void (*aOnTouchHandler)(TouchButton* const, int16_t));
    void setAllocated(bool isAllocated);
    void setFree(void);

    static void setDefaultTouchBorder(const uint8_t aDefaultTouchBorder);
    static void setDefaultButtonColor(const uint16_t aDefaultButtonColor);
    static void setDefaultCaptionColor(const uint16_t aDefaultCaptionColor);
    static int checkAllButtons(const int aTouchPositionX, const int aTouchPositionY, const bool doCallback);
    static void activateAllButtons(void);
    static void deactivateAllButtons(void);

    int8_t initSimpleButton(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX, const uint16_t aHeightY,
            const uint16_t aButtonColor, const char *aCaption, const uint8_t aCaptionSize, const int16_t aValue,
            void (*aOnTouchHandler)(TouchButton* const, int16_t));
    int8_t initButton(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX, const uint16_t aHeightY,
            const char *aCaption, const uint8_t aCaptionSize, const uint16_t aButtonColor, const uint16_t aCaptionColor,
            const int16_t aValue, void (*aOnTouchHandler)(TouchButton* const, int16_t));

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

    void setRedGreenButtonColor(void);
    void setRedGreenButtonColor(int16_t aValue);
    void setRedGreenButtonColorAndDraw(void);
    void setRedGreenButtonColorAndDraw(int16_t aValue);

    bool isAllocated(void) const;

private:
    // Pool stuff
    static TouchButton TouchButtonPool[NUMBER_OF_BUTTONS_IN_POOL];
    static bool sButtonPoolIsInitialized; // used by allocButton

    BlueDisplay * mDisplay; // The Display to use
    uint16_t mButtonColor;
    uint16_t mCaptionColor;
    uint16_t mPositionX;
    uint16_t mPositionY;
    uint16_t mWidth;
    uint16_t mHeight;
    uint8_t mCaptionSize;
    const char *mCaption; // Pointer to caption

protected:
    static uint16_t sDefaultButtonColor;
    static uint16_t sDefaultCaptionColor;

    static uint8_t sButtonCombinedPoolSize;
    static uint8_t sMinButtonPoolSize;

    static TouchButton *sListStart; // Root pointer to list of all buttons

    uint8_t mFlags; //Flag for: Autorepeat type, allocated, only caption
    TouchButton *mNextObject;

    int16_t mValue;
    void (*mOnTouchHandler)(TouchButton * const, int16_t);

};

// Handler for red green button
void doToggleRedGreenButton(TouchButton * const aTheTouchedButton, int16_t aValue);

/** @} */
/** @} */

#endif /* TOUCHBUTTON_H_ */
