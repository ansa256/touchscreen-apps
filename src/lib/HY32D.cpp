/**
 * @file HY32D.cpp
 *
 * @date 05.12.2012
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 *      based on MI0283QT2.cpp with license
 *      https://github.com/watterott/mSD-Shield/blob/master/src/license.txt
 */

#include "HY32D.h"
#include "misc.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {

#include "timing.h"
#include "ff.h"
#include "stm32f30xPeripherals.h"

}
#endif

/** @addtogroup Graphic_Library
 * @{
 */
/** @addtogroup HY32D_basic
 * @{
 */
#define LCD_GRAM_WRITE_REGISTER    0x22

// Buffer for raw display data of current chart
uint8_t DisplayBuffer1[DISPLAY_WIDTH];
uint8_t DisplayBuffer2[DISPLAY_WIDTH];
bool isInitializedHY32D = false;
volatile uint32_t sDrawLock = 0;
/*
 * For automatic LCD dimming
 */
int LCDBacklightValue = BACKLIGHT_START_VALUE;
int LCDLastBacklightValue; //! for state of backlight before dimming
int LCDDimDelay; //actual dim delay

//-------------------- Private functions --------------------
void drawStart(void);
inline void draw(uint16_t color);
inline void drawStop(void);
void writeCommand(int aRegisterAddress, int aRegisterValue);
bool initalizeDisplay(void);
uint16_t * fillDisplayLineBuffer(uint16_t * aBufferPtr, uint16_t yLineNumber);
void setBrightness(int power); //0-100

//-------------------- Public --------------------

void initHY32D(void) {
	//init pins
	HY32D_IO_initalize();
	// init PWM for background LED
	PWM_initalize();
	setBrightness(BACKLIGHT_START_VALUE);

	// deactivate read output control
	HY32D_RD_GPIO_PORT->BSRR = HY32D_RD_PIN;
	//initalize display
	if (initalizeDisplay()) {
		isInitializedHY32D = true;
	}
}

void setArea(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd) {
	if ((aXEnd >= DISPLAY_WIDTH) || (aYEnd >= DISPLAY_HEIGHT)) {
		assertFailedParamMessage((uint8_t *) __FILE__, __LINE__, aXEnd, aYEnd, StringEmpty);
	}

	writeCommand(0x44, aYStart + (aYEnd << 8)); //set ystart, yend
	writeCommand(0x45, aXStart); //set xStart
	writeCommand(0x46, aXEnd); //set xEnd
	// also set cursor to right start position
	writeCommand(0x4E, aYStart);
	writeCommand(0x4F, aXStart);
}

void setCursor(uint16_t aXStart, uint16_t aYStart) {
	writeCommand(0x4E, aYStart);
	writeCommand(0x4F, aXStart);
}

void clearDisplay(uint16_t color) {
	unsigned int size;
	setArea(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);

	drawStart();
	for (size = (DISPLAY_HEIGHT * DISPLAY_WIDTH); size != 0; size--) {
		HY32D_DATA_GPIO_PORT->ODR = color;
		// Latch data write
		HY32D_WR_GPIO_PORT->BRR = HY32D_WR_PIN;
		HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;

	}
	HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;
}

/**
 * set register adress to LCD_GRAM_READ/WRITE_REGISTER
 */
void drawStart(void) {
	// CS enable (low)
	HY32D_CS_GPIO_PORT->BRR = HY32D_CS_PIN;
	// Control enable (low)
	HY32D_DATA_CONTROL_GPIO_PORT->BRR = HY32D_DATA_CONTROL_PIN;
	// set value
	HY32D_DATA_GPIO_PORT->ODR = LCD_GRAM_WRITE_REGISTER;
	// Latch data write
	HY32D_WR_GPIO_PORT->BRR = HY32D_WR_PIN;
	HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;
	// Data enable (high)
	HY32D_DATA_CONTROL_GPIO_PORT->BSRR = HY32D_DATA_CONTROL_PIN;
}

inline void draw(uint16_t color) {
	// set value
	HY32D_DATA_GPIO_PORT->ODR = color;
	// Latch data write
	HY32D_WR_GPIO_PORT->BRR = HY32D_WR_PIN;
	HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;
}

void drawStop(void) {
	HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;
}

void fillRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
	uint32_t size;
	uint16_t tmp, i;

	if (x0 > x1) {
		tmp = x0;
		x0 = x1;
		x1 = tmp;
	}
	if (y0 > y1) {
		tmp = y0;
		y0 = y1;
		y1 = tmp;
	}

	if (x1 >= DISPLAY_WIDTH) {
		x1 = DISPLAY_WIDTH - 1;
	}
	if (y1 >= DISPLAY_HEIGHT) {
		y1 = DISPLAY_HEIGHT - 1;
	}

	setArea(x0, y0, x1, y1);

	drawStart();
	size = (uint32_t) (1 + (x1 - x0)) * (uint32_t) (1 + (y1 - y0));
	tmp = size / 8;
	if (tmp != 0) {
		for (i = tmp; i != 0; i--) {
			draw(color); //1
			draw(color); //2
			draw(color); //3
			draw(color); //4
			draw(color); //5
			draw(color); //6
			draw(color); //7
			draw(color); //8
		}
		for (i = size - tmp; i != 0; i--) {
			draw(color);
		}
	} else {
		for (i = size; i != 0; i--) {
			draw(color);
		}
	}
	drawStop();
}

void drawCircle(uint16_t x0, uint16_t y0, uint16_t radius, uint16_t color) {
	int16_t err, x, y;

	err = -radius;
	x = radius;
	y = 0;

	while (x >= y) {
		drawPixel(x0 + x, y0 + y, color);
		drawPixel(x0 - x, y0 + y, color);
		drawPixel(x0 + x, y0 - y, color);
		drawPixel(x0 - x, y0 - y, color);
		drawPixel(x0 + y, y0 + x, color);
		drawPixel(x0 - y, y0 + x, color);
		drawPixel(x0 + y, y0 - x, color);
		drawPixel(x0 - y, y0 - x, color);

		err += y;
		y++;
		err += y;
		if (err >= 0) {
			x--;
			err -= x;
			err -= x;
		}
	}
}

void fillCircle(uint16_t x0, uint16_t y0, uint16_t radius, uint16_t color) {
	int16_t err, x, y;

	err = -radius;
	x = radius;
	y = 0;

	while (x >= y) {
		drawLine(x0 - x, y0 + y, x0 + x, y0 + y, color);
		drawLine(x0 - x, y0 - y, x0 + x, y0 - y, color);
		drawLine(x0 - y, y0 + x, x0 + y, y0 + x, color);
		drawLine(x0 - y, y0 - x, x0 + y, y0 - x, color);

		err += y;
		y++;
		err += y;
		if (err >= 0) {
			x--;
			err -= x;
			err -= x;
		}
	}
}

void drawPixel(uint16_t aXPos, uint16_t aYPos, uint16_t color) {
	if ((aXPos >= DISPLAY_WIDTH) || (aYPos >= DISPLAY_HEIGHT)) {
		return;
	}

// setCursor
	writeCommand(0x4E, aYPos);
	writeCommand(0x4F, aXPos);

	drawStart();
	draw(color);
	drawStop();
}

uint16_t readPixel(uint16_t aXPos, uint16_t aYPos) {
	if ((aXPos >= DISPLAY_WIDTH) || (aYPos >= DISPLAY_HEIGHT)) {
		return 0;
	}

// setCursor
	writeCommand(0x4E, aYPos);
	writeCommand(0x4F, aXPos);

	drawStart();
	uint16_t tValue = 0;
// set port pins to input
	HY32D_DATA_GPIO_PORT->MODER = 0x00000000;
	// Latch data read
	HY32D_WR_GPIO_PORT->BRR = HY32D_RD_PIN;
	// wait >250ns
	delayNanos(300);

	tValue = HY32D_DATA_GPIO_PORT->IDR;
	HY32D_WR_GPIO_PORT->BSRR = HY32D_RD_PIN;
// set port pins to output
	HY32D_DATA_GPIO_PORT->MODER = 0x55555555;
	HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;

	return tValue;
}

void drawRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
	fillRect(x0, y0, x0, y1, color);
	fillRect(x0, y1, x1, y1, color);
	fillRect(x1, y0, x1, y1, color);
	fillRect(x0, y0, x1, y0, color);
}

/*
 * Fast routine for drawing data charts
 * draws a line only from x to x+1
 * first pixel is omitted because it is drawn by preceding line
 * uses set area to speed up drawing
 */
void drawLineFastOneX(uint16_t x0, uint16_t y0, uint16_t y1, uint16_t color) {
	uint8_t i;
	bool up = true;
//calculate direction
	int16_t deltaY = y1 - y0;
	if (deltaY < 0) {
		deltaY = -deltaY;
		up = false;
	}
	if (deltaY <= 1) {
		// constant y or one pixel offset => no line needed
		drawPixel(x0 + 1, y1, color);
	} else {
		// draw line here
		// deltaY1 is == deltaYHalf for even numbers and deltaYHalf -1 for odd Numbers
		uint8_t deltaY1 = (deltaY - 1) >> 1;
		uint8_t deltaYHalf = deltaY >> 1;
		if (up) {
			// for odd numbers, first part of line is 1 pixel shorter than second
			if (deltaY1 > 0) {
				// first pixel was drawn by preceding line :-)
				setArea(x0, y0 + 1, x0, y0 + deltaY1);
				drawStart();
				for (i = deltaY1; i != 0; i--) {
					draw(color);
				}
				drawStop();
			}
			setArea(x0 + 1, y0 + deltaY1 + 1, x0 + 1, y1);
			drawStart();
			for (i = deltaYHalf + 1; i != 0; i--) {
				draw(color);
			}
			drawStop();
		} else {
			// for odd numbers, second part of line is 1 pixel shorter than first
			if (deltaYHalf > 0) {
				setArea(x0, y0 - deltaYHalf, x0, y0 - 1);
				drawStart();
				for (i = deltaYHalf; i != 0; i--) {
					draw(color);
				}
				drawStop();
			}
			setArea(x0 + 1, y1, x0 + 1, (y0 - deltaYHalf) - 1);
			drawStart();
			for (i = deltaY1 + 1; i != 0; i--) {
				draw(color);
			}
			drawStop();
		}
	}
}

int drawChar(uint16_t x, uint16_t y, char c, uint8_t size, uint16_t color, uint16_t bg_color) {
	/*
	 * check if a draw in routine which uses setArea() is already executed
	 */
	uint32_t tLock;
	do {
		tLock = __LDREXW(&sDrawLock);
		tLock++;
	} while (__STREXW(tLock, &sDrawLock));

	if (tLock != 1) {
		// here in ISR, but interrupted process was still in drawText()
		sLockCount++;
		// first approach skip drawing and return -1
		return -1;
	}
	int tRetValue;
#if FONT_WIDTH <= 8
	uint8_t data, mask;
#elif FONT_WIDTH <= 16
	uint16_t data, mask;
#elif FONT_WIDTH <= 32
	uint32_t data, mask;
#endif
	uint8_t i, j, width, height;
	const uint8_t *ptr;
// characters below 20 are not printable
	if (c < 0x20) {
		c = 0x20;
	}
	i = (uint8_t) c;
#if FONT_WIDTH <= 8
	ptr = &font[(i - FONT_START) * (8 * FONT_HEIGHT / 8)];
#elif FONT_WIDTH <= 16
	ptr = &font_PGM[(i-FONT_START)*(16*FONT_HEIGHT/8)];
#elif FONT_WIDTH <= 32
	ptr = &font_PGM[(i-FONT_START)*(32*FONT_HEIGHT/8)];
#endif
	width = FONT_WIDTH;
	height = FONT_HEIGHT;

	if (size <= 1) {
		tRetValue = x + width;
		if ((tRetValue - 1) >= DISPLAY_WIDTH) {
			tRetValue = DISPLAY_WIDTH + 1;
		} else if ((y + height - 1) >= DISPLAY_HEIGHT) {
			tRetValue = DISPLAY_WIDTH + 1;
		} else {

			setArea(x, y, (x + width - 1), (y + height - 1));

			drawStart();
			for (; height != 0; height--) {
#if FONT_WIDTH <= 8
				data = *ptr;
				ptr += 1;
#elif FONT_WIDTH <= 16
				data = read_word(ptr); ptr+=2;
#elif FONT_WIDTH <= 32
				data = read_dword(ptr); ptr+=4;
#endif
				for (mask = (1 << (width - 1)); mask != 0; mask >>= 1) {
					if (data & mask) {
						draw(color);
					} else {
						draw(bg_color);
					}
				}
			}
			drawStop();
		}
	} else {
		tRetValue = x + (width * size);
		if ((tRetValue - 1) >= DISPLAY_WIDTH) {
			tRetValue = DISPLAY_WIDTH + 1;
		} else if ((y + (height * size) - 1) >= DISPLAY_HEIGHT) {
			tRetValue = DISPLAY_WIDTH + 1;
		} else {

			setArea(x, y, (x + (width * size) - 1), (y + (height * size) - 1));

			drawStart();
			for (; height != 0; height--) {
#if FONT_WIDTH <= 8
				data = *ptr;
				ptr += 1;
#elif FONT_WIDTH <= 16
				data = pgm_read_word(ptr); ptr+=2;
#elif FONT_WIDTH <= 32
				data = pgm_read_dword(ptr); ptr+=4;
#endif
				for (i = size; i != 0; i--) {
					for (mask = (1 << (width - 1)); mask != 0; mask >>= 1) {
						if (data & mask) {
							for (j = size; j != 0; j--) {
								draw(color);
							}
						} else {
							for (j = size; j != 0; j--) {
								draw(bg_color);
							}
						}
					}
				}
			}
			drawStop();
		}
	}
	sDrawLock = 0;
	return tRetValue;
}

/**
 *
 * @param x left position
 * @param y upper position
 * @param s String
 * @param size Font size
 * @param color
 * @param bg_color
 * @return
 */
int drawText(uint16_t x, uint16_t y, const char *s, uint8_t size, uint16_t color, uint16_t bg_color) {
	while (*s != 0) {
		x = drawChar(x, y, (char) *s++, size, color, bg_color);
		if (x > DISPLAY_WIDTH) {
			break;
		}
	}
	return x;
}

int drawTextVertical(uint16_t aXPos, uint16_t aYPos, const char *aStringPointer, uint8_t aSize, uint16_t aColor,
		uint16_t aBackgroundColor) {
	while (*aStringPointer != 0) {
		drawChar(aXPos, aYPos, (char) *aStringPointer++, aSize, aColor, aBackgroundColor);
		aYPos += FONT_HEIGHT * aSize;
		if (aYPos > DISPLAY_HEIGHT) {
			break;
		}
	}

	return aYPos;
}

/**
 *
 * @param x0 left position
 * @param y0 upper position
 * @param x1 right position
 * @param y1 lower position
 * @param s
 * @param size Font size
 * @param color
 * @param bg_color
 * @return
 */
extern "C" int drawMLText(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const char *s, uint8_t size, uint16_t color,
		uint16_t bg_color) {
	uint16_t x = x0, y = y0, wlen, llen;
	char c;
	const char *wstart;

	fillRect(x0, y0, x1, y1, bg_color);

	llen = (x1 - x0) / (FONT_WIDTH * size); //line length in chars
	wstart = s;
	while (*s && (y < y1)) {
		c = *s++;
		if (c == '\n') {
			//new line
			x = x0;
			y += (FONT_HEIGHT * size) + 1;
			continue;
		} else if (c == '\r') {
			//skip
			continue;
		}

		if (c == ' ') {
			//start of a new word
			wstart = s;
			if (x == x0) {
				//do nothing
				continue;
			}
		}

		if (c) {
			if ((x + (FONT_WIDTH * size)) > x1) {
				//new line
				if (c == ' ') {
					//do not start with space
					x = x0;
					y += (FONT_HEIGHT * size) + 1;
				} else {
					wlen = (s - wstart);
					if (wlen > llen) {
						//word too long
						x = x0;
						y += (FONT_HEIGHT * size) + 1;
						if (y < y1) {
							x = drawChar(x, y, c, size, color, bg_color);
						}
					} else {
						fillRect(x - (wlen * FONT_WIDTH * size), y, x1, (y + (FONT_HEIGHT * size)), bg_color); //clear word
						x = x0;
						y += (FONT_HEIGHT * size) + 1;
						s = wstart;
					}
				}
			} else {
				x = drawChar(x, y, c, size, color, bg_color);
			}
		}
	}

	return x;
}

int drawInteger(uint16_t x, uint16_t y, int val, uint8_t base, uint8_t size, uint16_t color, uint16_t bg_color) {
	char tmp[16 + 1];
	switch (base) {
	case 8:
		sprintf(tmp, "%o", (uint) val);
		break;
	case 10:
		sprintf(tmp, "%i", (uint) val);
		break;
	case 16:
		sprintf(tmp, "%x", (uint) val);
		break;
	}

	return drawText(x, y, tmp, size, color, bg_color);
}

void setBrightness(int power) {
	PWM_setOnRatio(power);
	LCDLastBacklightValue = power;
}

/**
 * Value for lcd backlight dimming delay
 */
void setDimDelayMillis(int32_t aTimeMillis) {
	changeDelayCallback(&callbackLCDDimming, aTimeMillis);
	LCDDimDelay = aTimeMillis;
}

/**
 * restore backlight to value before dimming
 */
void resetBacklightTimeout(void) {
	if (LCDLastBacklightValue != LCDBacklightValue) {
		setBrightness(LCDBacklightValue);
	}
	changeDelayCallback(&callbackLCDDimming, LCDDimDelay);
}

uint8_t getBacklightValue(void) {
	return LCDBacklightValue;
}

void setBacklightValue(uint8_t aBacklightValue) {
	LCDBacklightValue = clipBrightnessValue(aBacklightValue);
	setBrightness(LCDBacklightValue);
}

/**
 * Callback routine for SysTick handler
 * Dim LCD after period of touch inactivity
 */
void callbackLCDDimming(void) {
	if (LCDBacklightValue > BACKLIGHT_DIM_VALUE) {
		LCDLastBacklightValue = LCDBacklightValue;
		setBrightness(BACKLIGHT_DIM_VALUE);
	}
}

int clipBrightnessValue(int aBrightnessValue) {
	if (aBrightnessValue > BACKLIGHT_MAX_VALUE) {
		aBrightnessValue = BACKLIGHT_MAX_VALUE;
	}
	if (aBrightnessValue < BACKLIGHT_MIN_VALUE) {
		aBrightnessValue = BACKLIGHT_MIN_VALUE;
	}
	return aBrightnessValue;
}

void writeCommand(int aRegisterAddress, int aRegisterValue) {
// CS enable (low)
	HY32D_CS_GPIO_PORT->BRR = HY32D_CS_PIN;
// Control enable (low)
	HY32D_DATA_CONTROL_GPIO_PORT->BRR = HY32D_DATA_CONTROL_PIN;
// set value
	HY32D_DATA_GPIO_PORT->ODR = aRegisterAddress;
// Latch data write
	HY32D_WR_GPIO_PORT->BRR = HY32D_WR_PIN;
	HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;

// Data enable (high)
	HY32D_DATA_CONTROL_GPIO_PORT->BSRR = HY32D_DATA_CONTROL_PIN;
// set value
	HY32D_DATA_GPIO_PORT->ODR = aRegisterValue;
// Latch data write
	HY32D_WR_GPIO_PORT->BRR = HY32D_WR_PIN;
	HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;

// CS disable (high)
	HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;
	return;
}

uint16_t readCommand(int aRegisterAddress) {
// CS enable (low)
	HY32D_CS_GPIO_PORT->BRR = HY32D_CS_PIN;
// Control enable (low)
	HY32D_DATA_CONTROL_GPIO_PORT->BRR = HY32D_DATA_CONTROL_PIN;
// set value
	HY32D_DATA_GPIO_PORT->ODR = aRegisterAddress;
// Latch data write
	HY32D_WR_GPIO_PORT->BRR = HY32D_WR_PIN;
	HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;

// Data enable (high)
	HY32D_DATA_CONTROL_GPIO_PORT->BSRR = HY32D_DATA_CONTROL_PIN;
	// set port pins to input
	HY32D_DATA_GPIO_PORT->MODER = 0x00000000;
	// Latch data read
	HY32D_WR_GPIO_PORT->BRR = HY32D_RD_PIN;
	// wait >250ns
	delayNanos(300);
	uint16_t tValue = HY32D_DATA_GPIO_PORT->IDR;
	HY32D_WR_GPIO_PORT->BSRR = HY32D_RD_PIN;
// set port pins to output
	HY32D_DATA_GPIO_PORT->MODER = 0x55555555;
// CS disable (high)
	HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;
	return tValue;
}

bool initalizeDisplay(void) {
// Reset is done by hardware reset button
// Original Code
	writeCommand(0x0000, 0x0001); // Enable LCD Oscillator
	delayMillis(10);
	// Check Device Code - 0x8989
	if (readCommand(0x0000) != 0x8989) {
		return false;
	}

	writeCommand(0x0003, 0xA8A4); // Power control A=fosc/4 - 4= Small to medium
	writeCommand(0x000C, 0x0000); // VCIX2 only bit [2:0]
	writeCommand(0x000D, 0x080C); // VLCD63 only bit [3:0]
	writeCommand(0x000E, 0x2B00);
	writeCommand(0x001E, 0x00B0); // Bit7 + VcomH bit [5:0]
	writeCommand(0x0001, 0x293F); // reverse 320

	writeCommand(0x0002, 0x0600); // LCD driver AC setting
	writeCommand(0x0010, 0x0000); // Exit sleep mode
	delayMillis(50);

	writeCommand(0x0011, 0x6038); // 6=65k Color, 38=draw direction -> 3=horizontal increment, 8=vertical increment
//	writeCommand(0x0016, 0xEF1C); // 240 pixel
	writeCommand(0x0017, 0x0003);
	writeCommand(0x0007, 0x0133); // 1=the 2-division LCD drive is performed, 8 Color mode, grayscale
	writeCommand(0x000B, 0x0000);
	writeCommand(0x000F, 0x0000); // Gate Scan Position start (0-319)
	writeCommand(0x0041, 0x0000); // Vertical Scroll Control
	writeCommand(0x0042, 0x0000);
//	writeCommand(0x0048, 0x0000); // 0 is default 1st Screen driving position
//	writeCommand(0x0049, 0x013F); // 13F is default
//	writeCommand(0x004A, 0x0000); // 0 is default 2nd Screen driving position
//	writeCommand(0x004B, 0x0000);  // 13F is default

	delayMillis(10);
//gamma control
	writeCommand(0x0030, 0x0707);
	writeCommand(0x0031, 0x0204);
	writeCommand(0x0032, 0x0204);
	writeCommand(0x0033, 0x0502);
	writeCommand(0x0034, 0x0507);
	writeCommand(0x0035, 0x0204);
	writeCommand(0x0036, 0x0204);
	writeCommand(0x0037, 0x0502);
	writeCommand(0x003A, 0x0302);
	writeCommand(0x003B, 0x0302);

	writeCommand(0x0025, 0x8000); // Frequency Control 8=65Hz 0=50HZ E=80Hz
	return true;
}

/*
 * not checked after reset, only after first calling initalizeDisplay() above
 */
void initalizeDisplay2(void) {
// Reset is done by hardware reset button
	delayMillis(1);
	writeCommand(0x0011, 0x6838); // 6=65k Color, 8 = OE defines the display window 0 =the display window is defined by R4Eh and R4Fh.
	//writeCommand(0x0011, 0x6038); // 6=65k Color, 8 = OE defines the display window 0 =the display window is defined by R4Eh and R4Fh.
//Entry Mode setting
	writeCommand(0x0002, 0x0600); // LCD driver AC setting
	writeCommand(0x0012, 0x6CEB); // RAM data write
	// power control
	writeCommand(0x0003, 0xA8A4);
	writeCommand(0x000C, 0x0000); //VCIX2 only bit [2:0]
	writeCommand(0x000D, 0x080C); // VLCD63 only bit [3:0] ==
//	writeCommand(0x000D, 0x000C); // VLCD63 only bit [3:0]
	writeCommand(0x000E, 0x2B00); // ==
	writeCommand(0x001E, 0x00B0); // Bit7 + VcomH bit [5:0] ==
	writeCommand(0x0001, 0x293F); // reverse 320

	// compare register
	//writeCommand(0x0005, 0x0000);
	//writeCommand(0x0006, 0x0000);

	//writeCommand(0x0017, 0x0103); //Vertical Porch
	delayMillis(1);

	delayMillis(30);
//gamma control
	writeCommand(0x0030, 0x0707);
	writeCommand(0x0031, 0x0204);
	writeCommand(0x0032, 0x0204);
	writeCommand(0x0033, 0x0502);
	writeCommand(0x0034, 0x0507);
	writeCommand(0x0035, 0x0204);
	writeCommand(0x0036, 0x0204);
	writeCommand(0x0037, 0x0502);
	writeCommand(0x003A, 0x0302);
	writeCommand(0x003B, 0x0302);

	writeCommand(0x002F, 0x12BE);
	writeCommand(0x0023, 0x0000);
	delayMillis(1);
	writeCommand(0x0024, 0x0000);
	delayMillis(1);
	writeCommand(0x0025, 0x8000);

	writeCommand(0x004e, 0x0000); // RAM address set
	writeCommand(0x004f, 0x0000);
	return;
}

void drawGrayscale(uint16_t aXPos, uint16_t tYPos, uint16_t aHeight) {
	uint16_t tY;
	for (int i = 0; i < 256; ++i) {
		tY = tYPos;
		fillRect(aXPos, tY, aXPos, tY + aHeight, RGB(i, i, i));
		tY += aHeight + 1;
		fillRect(aXPos, tY, aXPos, tY + aHeight, RGB((0xFF - i), (0xFF - i), (0xFF - i)));
		tY += aHeight + 1;
		fillRect(aXPos, tY, aXPos, tY + aHeight, RGB(i, 0, 0));
		tY += aHeight + 1;
		fillRect(aXPos, tY, aXPos, tY + aHeight, RGB(0, i, 0));
		tY += aHeight + 1;
		fillRect(aXPos, tY, aXPos, tY + aHeight, RGB(0, 0, i));
		aXPos++;
	}
}

void setGamma(int aIndex) {
	switch (aIndex) {
	case 0:
		//old gamma
		writeCommand(0x0030, 0x0707);
		writeCommand(0x0031, 0x0204);
		writeCommand(0x0032, 0x0204);
		writeCommand(0x0033, 0x0502);
		writeCommand(0x0034, 0x0507);
		writeCommand(0x0035, 0x0204);
		writeCommand(0x0036, 0x0204);
		writeCommand(0x0037, 0x0502);
		writeCommand(0x003A, 0x0302);
		writeCommand(0x003B, 0x0302);
		break;
	case 1:
		// new gamma
		writeCommand(0x0030, 0x0707);
		writeCommand(0x0031, 0x0704);
		writeCommand(0x0032, 0x0204);
		writeCommand(0x0033, 0x0201);
		writeCommand(0x0034, 0x0203);
		writeCommand(0x0035, 0x0204);
		writeCommand(0x0036, 0x0204);
		writeCommand(0x0037, 0x0502);
		writeCommand(0x003A, 0x0302);
		writeCommand(0x003B, 0x0500);
		break;
	default:
		break;
	}
}

void testHY32D(int aLoopCount) {
	clearDisplay(COLOR_WHITE);

	fillRect(0, 0, 2, 2, COLOR_RED);
	fillRect(DISPLAY_WIDTH - 3, 0, DISPLAY_WIDTH - 1, 2, COLOR_GREEN);
	fillRect(0, DISPLAY_HEIGHT - 3, 2, DISPLAY_HEIGHT - 1, COLOR_BLUE);
	fillRect(DISPLAY_WIDTH - 3, DISPLAY_HEIGHT - 3, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, COLOR_BLACK);

	fillRect(2, 2, 6, 6, COLOR_RED);
	fillRect(10, 20, 20, 40, COLOR_RED);
	drawCircle(15, 30, 5, COLOR_BLUE);
	fillCircle(20, 10, 10, COLOR_BLUE);

	drawLine(0, DISPLAY_HEIGHT - 1, DISPLAY_WIDTH - 1, 0, COLOR_GREEN);
	drawLine(2, 2, DISPLAY_WIDTH - 3, DISPLAY_HEIGHT - 3, COLOR_BLUE);
	drawRect(8, 18, 22, 42, COLOR_BLUE);
	drawChar(50, 0, 'y', 1, COLOR_GREEN, COLOR_YELLOW);
	drawText(0, 50, StringCalibration, 1, COLOR_BLACK, COLOR_WHITE);
	drawText(0, 62, StringCalibration, 1, COLOR_WHITE, COLOR_BLACK);
	drawLineOverlap(120, 140, 180, 125, LINE_OVERLAP_MAJOR, COLOR_RED);
	drawLineOverlap(120, 143, 180, 128, LINE_OVERLAP_MINOR, COLOR_RED);
	drawLineOverlap(120, 146, 180, 131, LINE_OVERLAP_BOTH, COLOR_RED);

	uint16_t DeltaSmall = 20;
	uint16_t DeltaBig = 100;

	drawThickLine(10, 60, 10 + DeltaSmall, 60 + DeltaBig, 4, LINE_THICKNESS_MIDDLE, COLOR_GREEN);
	drawPixel(10, 60, COLOR_BLUE);

	drawThickLine(70, 70, 70 - DeltaSmall, 70 + DeltaBig, 4, LINE_THICKNESS_MIDDLE, COLOR_GREEN);
	drawPixel(70, 70, COLOR_BLUE);

	drawThickLine(100, 30, 100 + DeltaBig, 30 + DeltaSmall, 9, LINE_THICKNESS_DRAW_CLOCKWISE, COLOR_GREEN);
	drawPixel(100, 30, COLOR_BLUE);

	drawThickLine(140, 25, 140 + DeltaBig, 25 - DeltaSmall, 9, LINE_THICKNESS_DRAW_CLOCKWISE, COLOR_GREEN);
	drawPixel(140, 25, COLOR_BLUE);

	drawThickLine(140, 80, 140 - DeltaSmall, 80 - DeltaSmall, 3, LINE_THICKNESS_MIDDLE, COLOR_GREEN);
	drawPixel(140, 80, COLOR_BLUE);

	drawThickLine(150, 80, 150 + DeltaSmall, 80 - DeltaSmall, 3, LINE_THICKNESS_MIDDLE, COLOR_GREEN);
	drawPixel(150, 80, COLOR_BLUE);

	drawThickLine(190, 80, 190 - DeltaSmall, 80 - DeltaSmall, 3, LINE_THICKNESS_DRAW_CLOCKWISE, COLOR_GREEN);
	drawPixel(190, 80, COLOR_BLUE);

	drawThickLine(200, 80, 200 + DeltaSmall, 80 - DeltaSmall, 3, LINE_THICKNESS_DRAW_CLOCKWISE, COLOR_GREEN);
	drawPixel(200, 80, COLOR_BLUE);

	drawGrayscale(5, 180, 10);
	for (int i = 0; i < aLoopCount; ++i) {
		setGamma(1);
		delayMillis(1000);
		setGamma(0);
		delayMillis(1000);
	}
}

#define COLOR_SPECTRUM_SEGMENTS 6 // red->yellow, yellow-> green, green-> cyan, cyan-> blue, blue-> magent, magenta-> red
#define COLOR_RESOLUTION 32 // 5 bit for 16 bit color (green really has 6 bit, but dont use it)
const uint16_t colorIncrement[COLOR_SPECTRUM_SEGMENTS] = { 1 << 6, 0x1F << 11, 1, 0x3FF << 6, 1 << 11, 0xFFFF };
/**
 * generates a full color spectrum beginning with a black line,
 * increasing saturation to full colors and then fading to a white line
 * customized for a 320 x 240 display
 */
void generateColorSpectrum(void) {
	clearDisplay(COLOR_WHITE);
	uint16_t tColor;
	uint16_t tXPos;
	uint16_t tDelta;
	uint16_t tError;

	uint16_t tColorChangeAmount;
	uint16_t tYpos = DISPLAY_HEIGHT;
	uint16_t tColorLine;
	for (int line = 4; line < DISPLAY_HEIGHT + 4; ++line) {
		tColorLine = line / 4;
		// colors for line 31 and 32 are identical
		if (tColorLine >= COLOR_RESOLUTION) {
			// line 32 to 63 full saturated basic colors to pure white
			tColorChangeAmount = ((2 * COLOR_RESOLUTION) - 1) - tColorLine; // 31 - 0
			tColor = 0x1f << 11 | (tColorLine - COLOR_RESOLUTION) << 6 | (tColorLine - COLOR_RESOLUTION);
		} else {
			// line 0 - 31 pure black to full saturated basic colors
			tColor = tColorLine << 11; // RED
			tColorChangeAmount = tColorLine; // 0 - 31
		}
		tXPos = 0;
		tYpos--;
		for (int i = 0; i < COLOR_SPECTRUM_SEGMENTS; ++i) {
			tDelta = colorIncrement[i];
//			tError = COLOR_RESOLUTION / 2;
//			for (int j = 0; j < COLOR_RESOLUTION; ++j) {
//				// draw start value + 31 slope values
//				drawPixel(tXPos++, tYpos, tColor);
//				tError += tColorChangeAmount;
//				if (tError > COLOR_RESOLUTION) {
//					tError -= COLOR_RESOLUTION;
//					tColor += tDelta;
//				}
//			}
			tError = ((DISPLAY_WIDTH / COLOR_SPECTRUM_SEGMENTS) - 1) / 2;
			for (int j = 0; j < (DISPLAY_WIDTH / COLOR_SPECTRUM_SEGMENTS) - 1; ++j) {
				drawPixel(tXPos++, tYpos, tColor);
				tError += tColorChangeAmount;
				if (tError > ((DISPLAY_WIDTH / COLOR_SPECTRUM_SEGMENTS) - 1)) {
					tError -= ((DISPLAY_WIDTH / COLOR_SPECTRUM_SEGMENTS) - 1);
					tColor += tDelta;
				}
			}
			// draw greyscale in the last 8 pixel :-)
//			drawPixel(DISPLAY_WIDTH - 2, tYpos, (tColorLine & 0x3E) << 10 | tColorLine << 5 | tColorLine >> 1);
//			drawPixel(DISPLAY_WIDTH - 1, tYpos, (tColorLine & 0x3E) << 10 | tColorLine << 5 | tColorLine >> 1);
			drawLine(DISPLAY_WIDTH - 8, tYpos, DISPLAY_WIDTH - 1, tYpos,
					(tColorLine & 0x3E) << 10 | tColorLine << 5 | tColorLine >> 1);

		}
	}
}

/**
 * reads a display line in BMP 16 Bit format. ie. only 5 bit for green
 */
uint16_t * fillDisplayLineBuffer(uint16_t * aBufferPtr, uint16_t yLineNumber) {
// set area is needed!
	setArea(0, yLineNumber, DISPLAY_WIDTH - 1, yLineNumber);
	drawStart();
	uint16_t tValue = 0;
// set port pins to input
	HY32D_DATA_GPIO_PORT->MODER = 0x00000000;
	for (int i = 0; i <= DISPLAY_WIDTH; ++i) {
		// Latch data read
		HY32D_WR_GPIO_PORT->BRR = HY32D_RD_PIN;
		// wait >250ns (and process former value)
		if (i > 1) {
			// skip inital value (=0) and first reading from display (is from last read => scrap)
			// shift red and green one bit down so that every color has 5 bits
			tValue = (tValue & BLUEMASK) | ((tValue >> 1) & ~BLUEMASK);
			*aBufferPtr++ = tValue;
		}
		tValue = HY32D_DATA_GPIO_PORT->IDR;
		HY32D_WR_GPIO_PORT->BSRR = HY32D_RD_PIN;
	}
// last value
	tValue = (tValue & BLUEMASK) | ((tValue >> 1) & ~BLUEMASK);
	*aBufferPtr++ = tValue;
// set port pins to output
	HY32D_DATA_GPIO_PORT->MODER = 0x55555555;
	HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;
	return aBufferPtr;
}

extern "C" void storeScreenshot(void) {
	uint8_t tFeedbackType = FEEDBACK_TONE_LONG_ERROR;
	if (MICROSD_isCardInserted()) {

		FIL tFile;
		FRESULT tOpenResult;
		UINT tCount;
//	int filesize = 54 + 2 * DISPLAY_WIDTH * DISPLAY_HEIGHT;

		unsigned char bmpfileheader[14] = { 'B', 'M', 54, 88, 02, 0, 0, 0, 0, 0, 54, 0, 0, 0 };
		unsigned char bmpinfoheader[40] = { 40, 0, 0, 0, 64, 1, 0, 0, 240, 0, 0, 0, 1, 0, 16, 0 };

//	bmpfileheader[2] = (unsigned char) (filesize);
//	bmpfileheader[3] = (unsigned char) (filesize >> 8);
//	bmpfileheader[4] = (unsigned char) (filesize >> 16);
//	bmpfileheader[5] = (unsigned char) (filesize >> 24);

		RTC_getDateStringForFile(StringBuffer);
		strcat(StringBuffer, ".bmp");
		tOpenResult = f_open(&tFile, StringBuffer, FA_CREATE_ALWAYS | FA_WRITE);
		uint16_t * tBufferPtr;
		if (tOpenResult == FR_OK) {
			f_write(&tFile, bmpfileheader, 14, &tCount);
			f_write(&tFile, bmpinfoheader, 40, &tCount);
			// from left to right and from bottom to top
			for (int i = DISPLAY_HEIGHT - 1; i >= 0;) {
				tBufferPtr = &FourDisplayLinesBuffer[0];
				// write 4 lines at a time to speed up I/O
				tBufferPtr = fillDisplayLineBuffer(tBufferPtr, i--);
				tBufferPtr = fillDisplayLineBuffer(tBufferPtr, i--);
				tBufferPtr = fillDisplayLineBuffer(tBufferPtr, i--);
				fillDisplayLineBuffer(tBufferPtr, i--);
				// write a display line
				f_write(&tFile, &FourDisplayLinesBuffer[0], DISPLAY_WIDTH * 8, &tCount);
			}
			f_close(&tFile);
			tFeedbackType = FEEDBACK_TONE_NO_ERROR;
		}
	}
	FeedbackTone(tFeedbackType);
}
/** @} */
/** @} */

