/*
 * HY32D.h
 *
 * @date  14.02.2012
 * @author  Armin Joachimsmeyer
 * armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 */
#ifndef HY32D_h
#define HY32D_h

#include "thickline.h"
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
 * Backlight
 */
#define BACKLIGHT_START_VALUE 50
#define BACKLIGHT_MAX_VALUE 100
#define BACKLIGHT_MIN_VALUE 3
#define BACKLIGHT_DIM_VALUE 7
#define BACKLIGHT_DIM_DEFAULT_DELAY TWO_MINUTES

#define RGB(red, green, blue) (unsigned int) (((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3))

#define BLUEMASK 0x1F
#define COLOR_WHITE   RGB(255,255,255)
#define COLOR_BLACK   RGB(  0,  0, 16) // 16 because 0 is used as flag (e.g. in touch button for default color)
#define COLOR_RED     RGB(255,  0,  0)
#define COLOR_GREEN   RGB(  0,255,  0)
#define COLOR_BLUE    RGB(  0,  0,255)
#define COLOR_YELLOW  RGB(255,255,  0)
#define COLOR_MAGENTA RGB(255,  0,255)
#define COLOR_CYAN    RGB(  0,255,255)

#ifdef __cplusplus
extern "C" {
#endif
extern uint8_t DisplayBuffer1[DISPLAY_WIDTH];
extern uint8_t DisplayBuffer2[DISPLAY_WIDTH];
extern bool isInitializedHY32D;
extern volatile uint32_t sDrawLock;

void initHY32D(void);
void setDimDelayMillis(int32_t aTimeMillis);
void resetBacklightTimeout(void);
uint8_t getBacklightValue(void);
void callbackLCDDimming(void);
int clipBrightnessValue(int aBrightnessValue);

uint16_t getWidth(void);
uint16_t getHeight(void);

void setArea(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void setCursor(uint16_t aXStart, uint16_t aYStart);

void drawLineFastOneX(uint16_t x0, uint16_t y0, uint16_t y1, uint16_t color);
void drawThickLine(int16_t aXStart, int16_t aYStart, int16_t aXEnd, int16_t aYEnd, int16_t aThickness, uint8_t aThicknessMode,
		uint16_t aColor);
void drawRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void drawCircle(uint16_t x0, uint16_t y0, uint16_t radius, uint16_t color);
void fillCircle(uint16_t x0, uint16_t y0, uint16_t radius, uint16_t color);

int drawChar(uint16_t x, uint16_t y, char c, uint8_t size, uint16_t color, uint16_t bg_color);
int drawText(uint16_t x, uint16_t y, const char *s, uint8_t size, uint16_t color, uint16_t bg_color);
int drawTextVertical(uint16_t aXPos, uint16_t aYPos, const char *aStringPointer, uint8_t aSize, uint16_t aColor,
		uint16_t aBackgroundColor);
int drawInteger(uint16_t x, uint16_t y, int val, uint8_t base, uint8_t size, uint16_t color, uint16_t bg_color);

void clearDisplay(uint16_t color);
void drawPixel(uint16_t x0, uint16_t y0, uint16_t color);
uint16_t readPixel(uint16_t aXPos, uint16_t aYPos);
void fillRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
int drawMLText(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const char *s, uint8_t size, uint16_t color, uint16_t bg_color);
void storeScreenshot(void);
#ifdef __cplusplus
}
#endif

// Tests
void initalizeDisplay2(void);
void setGamma(int aIndex);
void testHY32D(int aLoopCount);
void drawGrayscale(uint16_t aXPos, uint16_t tYPos, uint16_t aHeight);
void writeCommand(int aRegisterAddress, int aRegisterValue);
void generateColorSpectrum(void);

/** @} */
/** @} */

#endif //HY32D_h
