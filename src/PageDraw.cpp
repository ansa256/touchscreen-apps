/*
 * PageDraw.cpp
 *
 * @date 16.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 */

#include "Pages.h"
#include "misc.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {

#include "stm32f30xPeripherals.h"
#include "timing.h"

}
#endif

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static unsigned int sLastX, sLastY;
static uint16_t sDrawColor = COLOR_BLACK;

static TouchButton * TouchButtonClear;

#define NUMBER_OF_DRAW_COLORS 5
static TouchButton * TouchButtonsDrawColor[NUMBER_OF_DRAW_COLORS];
static const uint16_t DrawColors[NUMBER_OF_DRAW_COLORS] = { COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_YELLOW };

void initDrawPage(void) {
}

void displayDrawPage(void) {
	clearDisplay(COLOR_BACKGROUND_DEFAULT);
	for (uint8_t i = 0; i < 5; ++i) {
		TouchButtonsDrawColor[i]->drawButton();
	}
	TouchButtonClear->drawButton();
	TouchButtonMainHome->drawButton();
}

void doDrawClear(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	displayDrawPage();
}

static void doDrawColor(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	sDrawColor = DrawColors[aValue];
}

static bool drawLinePeriodicCallbackHandler(const int aTouchPositionX, const int aTouchPositionY) {
	drawLine(sLastX, sLastY, aTouchPositionX, aTouchPositionY, sDrawColor);
	sLastX = aTouchPositionX;
	sLastY = aTouchPositionY;
	TouchPanel.printTPData(30, 2, COLOR_BLACK, COLOR_BACKGROUND_DEFAULT);
	return true;
}

void startDrawPage(void) {
	// Color buttons
	uint16_t tPosY = 0;
	for (uint8_t i = 0; i < 5; ++i) {
		TouchButtonsDrawColor[i] = TouchButton::allocButton(false);
		TouchButtonsDrawColor[i]->initButton(0, tPosY, 30, 30, NULL, 1, 0, DrawColors[i], 0, i, &doDrawColor);
		tPosY += 30;
	}

	TouchButtonClear = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
			BUTTON_HEIGHT_4, COLOR_RED, StringClear, 2, 0, &doDrawClear);

	initMainHomeButtonWithPosition(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, false);
	displayDrawPage();
}

void loopDrawPage(int aGuiTouchState) {
	if (aGuiTouchState == GUI_TOUCH_NO_MATCH) {
		int x = TouchPanel.getXActual();
		int y = TouchPanel.getYActual();
		// start draw line
		TouchPanel.registerPeriodicTouchCallback(&drawLinePeriodicCallbackHandler, 10);
		drawPixel(x, y, sDrawColor);
		sLastX = x;
		sLastY = y;
	}
}

void stopDrawPage(void) {
	// free buttons
	for (unsigned int i = 0; i < NUMBER_OF_DRAW_COLORS; ++i) {
		(TouchButtonsDrawColor[i])->setFree();
	}
	TouchButtonClear->setFree();
}

