/*
 * MI0283QT2.h
 *
 * @date  14.02.2012
 * @author  Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 */

#ifndef MI0283QT2_h
#define MI0283QT2_h

#include "fonts.h"
#include <stdbool.h>

/** @addtogroup Graphic_Library
 * @{
 */
/** @addtogroup HY32D_basic
 * @{
 */

// Landscape format
#define DISPLAY_HEIGHT  240
#define DISPLAY_WIDTH   320

/*
 * Backlight values in percent
 */
#define BACKLIGHT_START_VALUE 50
#define BACKLIGHT_MAX_VALUE 100
#define BACKLIGHT_MIN_VALUE 0
#define BACKLIGHT_DIM_VALUE 7
#define BACKLIGHT_DIM_DEFAULT_DELAY TWO_MINUTES

#ifdef __cplusplus
class MI0283QT2 {

public:
    MI0283QT2();
    void init(void);

    void clearDisplay(uint16_t color);
    void drawPixel(uint16_t aXPos, uint16_t aYPos, uint16_t aColor);
    void drawCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, uint16_t aColor);
    void fillCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, uint16_t aColor);
    void fillRect(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor);
    uint16_t drawChar(uint16_t aPosX, uint16_t aPosY, char aChar, uint8_t aCharSize, uint16_t aFGColor, uint16_t aBGColor);
    uint16_t drawText(uint16_t aXStart, uint16_t aYStart, char *aStringPtr, uint8_t aSize, uint16_t aColor, uint16_t aBGColor);
    void drawTextVertical(uint16_t aXPos, uint16_t aYPos, const char *aStringPointer, uint8_t aSize, uint16_t aColor,
            uint16_t aBackgroundColor);
    void drawLine(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor);
    void drawLineFastOneX(uint16_t x0, uint16_t y0, uint16_t y1, uint16_t color);
    void drawRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
    uint16_t drawMLText(uint16_t aPosX, uint16_t aPosY, const char *aStringPtr, uint8_t aTextSize, uint16_t aColor,
            uint16_t aBGColor);

private:

};

// The instance provided by the class itself
extern MI0283QT2 LocalDisplay;
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern bool isInitializedMI0283QT2;
extern volatile uint32_t sDrawLock;

void setDimDelayMillis(int32_t aTimeMillis);
void resetBacklightTimeout(void);
uint8_t getBacklightValue(void);
void callbackLCDDimming(void);
int clipBrightnessValue(int aBrightnessValue);

uint16_t getWidth(void);
uint16_t getHeight(void);

void setArea(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

void setCursor(uint16_t aXStart, uint16_t aYStart);

int drawNText(uint16_t x, uint16_t y, const char *s, int aNumberOfCharacters, uint8_t size, uint16_t color, uint16_t bg_color);

uint16_t drawInteger(uint16_t x, uint16_t y, int val, uint8_t base, uint8_t size, uint16_t color, uint16_t bg_color);

uint16_t readPixel(uint16_t aXPos, uint16_t aYPos);
void storeScreenshot(void);

#ifdef __cplusplus
}
#endif

// Tests
void initalizeDisplay2(void);
void setGamma(int aIndex);
void writeCommand(int aRegisterAddress, int aRegisterValue);

/** @} */
/** @} */

#endif //MI0283QT2_h
