/*
 * PageMainMenu.cpp
 *
 * @date 17.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 *
 * While exploring the capabilities of the STM32F3 Discovery coupled with a 3.2 HY32D touch screen I build several apps:
 -DSO
 -Frequency synthesizer
 -DAC triangle generator
 -Visualization app for accelerator, gyroscope and compass data
 -Simple drawing app
 -Screenshot to MicroSd card
 -Color picker
 -Game of life
 -System info and test app

 And a C library for drawing thick lines (extension of Bresenham algorithm)
 */

#include "Pages.h"

#include "misc.h"

#include "TouchButton.h"
//#include "TouchSlider.h"
#include "TouchDSO.h"
#include "AccuCapacity.h"
#include "GuiDemo.h"

#ifdef __cplusplus
extern "C" {

#include "diskio.h"
#include "ff.h"

#include "stm32f3_discovery_leds_buttons.h"

}
#endif

/* Private define ------------------------------------------------------------*/
#define MENU_TOP 15
#define MENU_LEFT 30
#define MAIN_MENU_COLOR COLOR_RED

/* Public variables ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static TouchButton * TouchButtonDSO;
static TouchButton * TouchButtonBob;
static TouchButton * TouchButtonDemo;
static TouchButton * TouchButtonAccelerator;
static TouchButton * TouchButtonDraw;
static TouchButton * TouchButtonDatalogger;
static TouchButton * TouchButtonDac;
static TouchButton * TouchButtonFrequencyGenerator;
static TouchButton * TouchButtonTest;
static TouchButton * TouchButtonIR;
static TouchButton * TouchButtonMainSettings;
TouchButton * TouchButtonMainHome; // for other pages to get back
static TouchButton ** TouchButtonsMainMenu[] = { &TouchButtonDSO, &TouchButtonBob, &TouchButtonDemo, &TouchButtonAccelerator,
		&TouchButtonDraw, &TouchButtonDatalogger, &TouchButtonDac, &TouchButtonFrequencyGenerator, &TouchButtonTest,
		&TouchButtonMainSettings, &TouchButtonIR, &TouchButtonMainHome }; // TouchButtonMainHome must be last element of array

/**
 * Touch slider
 */
TouchSlider TouchSliderVertical;
TouchSlider TouchSliderVertical2;
TouchSlider TouchSliderHorizontal;
TouchSlider TouchSliderHorizontal2;

uint32_t MillisLastLoop;
uint32_t MillisSinceLastAction;

// flag if application loop is still active
volatile bool sInApplication;

void displayMainMenuPage(void);

/* Private functions ---------------------------------------------------------*/
void doMainMenuButtons(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	TouchButton::deactivateAllButtons();
	if (aTheTouchedButton == TouchButtonDSO) {
		startDSO();
		sInApplication = true;
		while (sInApplication) {
			loopDSO();
		}
		stopDSO();
		return;
	} else if (aTheTouchedButton == TouchButtonDemo) {
		startGuiDemo();
		sInApplication = true;
		while (sInApplication) {
			loopGuiDemo(CheckTouchGeneric(true));
		}
		stopGuiDemo();
		return;
	} else if (aTheTouchedButton == TouchButtonMainSettings) {
		// Settings button pressed
		clearDisplay(COLOR_BACKGROUND_DEFAULT);
		startSettingsPage();
		sInApplication = true;
		while (sInApplication) {
			CheckTouchGeneric(true);
			loopSettingsPage();
		}
		stopSettingsPage();
		return;
	} else if (aTheTouchedButton == TouchButtonAccelerator) {
		startAcceleratorCompass();
		sInApplication = true;
		while (sInApplication) {
			loopAcceleratorGyroCompassDemo();
			CheckTouchGeneric(true);
		}
		stopAcceleratorCompass();
		return;
	} else if (aTheTouchedButton == TouchButtonDatalogger) {
		startAccuCapacity();
		sInApplication = true;
		while (sInApplication) {
			loopAccuCapacity();
		}
		stopAccuCapacity();
		return;
	} else if (aTheTouchedButton == TouchButtonDac) {
		startDACPage();
		sInApplication = true;
		while (sInApplication) {
			CheckTouchGeneric(true);
		}
		stopDACPage();
		return;
	} else if (aTheTouchedButton == TouchButtonFrequencyGenerator) {
		startFreqGenPage();
		sInApplication = true;
		while (sInApplication) {
			CheckTouchGeneric(true);
			//loopFreqGenPage();
		}
		stopFreqGenPage();
		return;
	} else if (aTheTouchedButton == TouchButtonDraw) {
		startDrawPage();
		sInApplication = true;
		while (sInApplication) {
			loopDrawPage(CheckTouchGeneric(true));
		}
		stopDrawPage();
		return;
	} else if (aTheTouchedButton == TouchButtonBob) {
		clearDisplay(COLOR_GREEN);
		SPI1_setPrescaler(SPI_BaudRatePrescaler_8);
		initBobsDemo();
		sInApplication = true;
		startBobsDemo();
		while (sInApplication) {
			loopBobsDemo();
			CheckTouchGeneric(true);
		}
		stopBobsDemo();
		return;
	} else if (aTheTouchedButton == TouchButtonTest) {
		sInApplication = true;
		startTestsPage();
		while (sInApplication) {
			CheckTouchGeneric(true);
		}
		stopTestsPage();
		return;
	} else if (aTheTouchedButton == TouchButtonIR) {
		sInApplication = true;
		startIRPage();
		while (sInApplication) {
			loopIRPage();
			CheckTouchGeneric(true);
		}
		stopIRPage();
		return;
	}
}

/**
 * misc tests
 */

//FIL File1, File2; /* File objects */
void doTestButton(TouchButton * const aTheTouchedButton, int16_t aValue) {
	TouchButton::deactivateAllButtons();
	FeedbackToneOK();
	clearDisplay(COLOR_WHITE);
	drawLine(10, 10, 20, 16, COLOR_BLACK);
	drawLine(10, 11, 20, 17, COLOR_BLACK);
	drawLine(9, 11, 19, 17, COLOR_BLACK);

	drawLine(10, 80, 80, 17, COLOR_BLACK);
	drawLine(10, 79, 80, 16, COLOR_BLACK);
	drawLine(11, 80, 81, 17, COLOR_BLACK);

//	clearDisplay(COLOR_WHITE);
//	BYTE b1 = f_open(&File1, "test.asc", FA_OPEN_EXISTING | FA_READ);
//	snprintf(StringBuffer, sizeof StringBuffer, "Open Result of test.asc: %u\n", b1);
//	drawText(0, 0, StringBuffer, 1, COLOR_RED, COLOR_WHITE);
//
//	b1 = f_open(&File2, "test2.asc", FA_CREATE_ALWAYS | FA_WRITE);
//	snprintf(StringBuffer, sizeof StringBuffer, "Open Result of test2.asc: %u\n", b1);
//	drawText(0, FONT_HEIGHT, StringBuffer, 1, COLOR_RED, COLOR_WHITE);
//
//	f_close(&File1);
//	f_close(&File2);

//	FeedbackToneOK();
	TouchButtonMainHome->drawButton();
	return;
}

/**
 * switch back to main menu
 */
void doMainMenuHomeButton(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	backToMainMenu();
}

/**
 * clear display handler
 */
void doClearScreen(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	clearDisplay(aValue);
}

/**
 * ends loops in button handler
 */
void backToMainMenu(void) {
	sInApplication = false;
	TouchButton::deactivateAllButtons();
	TouchSlider::deactivateAllSliders();
	displayMainMenuPage();
}

/* Public functions ---------------------------------------------------------*/
void initMainMenuPage(void) {
}

void startMainMenuPage(void) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
	/**
	 * create and position all BUTTONS initially
	 */
	TouchButton::setDefaultButtonColor(MAIN_MENU_COLOR);
	int tPosY = 0;
	TouchButtonDSO = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "DSO", 2, 0,
			&doMainMenuButtons);

	TouchButtonDemo = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "Demo",
			2, 0, &doMainMenuButtons);

	TouchButtonMainSettings = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, 0, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
			StringSettings, 1, 0, &doMainMenuButtons);

// 2. row
	tPosY += BUTTON_HEIGHT_4_LINE_2;
	TouchButtonIR = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "IR", 2, 0,
			&doMainMenuButtons);

	TouchButtonAccelerator = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
			"Accelerator", 1, 0, &doMainMenuButtons);

	TouchButtonDraw = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "Draw",
			2, 0, &doMainMenuButtons);
//3. row
	tPosY += BUTTON_HEIGHT_4_LINE_2;
	TouchButtonDac = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "DAC", 2, 0,
			&doMainMenuButtons);

	TouchButtonDatalogger = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
			"AccuCapacity", 1, 0, &doMainMenuButtons);

	TouchButtonTest = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
			StringTest, 2, 0, &doMainMenuButtons);

//4. row
	TouchButtonFrequencyGenerator = TouchButton::allocAndInitSimpleButton(0, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
	BUTTON_HEIGHT_4, 0, "Frequency", 1, 0, &doMainMenuButtons);

	TouchButtonBob = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
	BUTTON_HEIGHT_4, 0, "Bob", 2, 0, &doMainMenuButtons);

#pragma GCC diagnostic pop
	TouchButtonMainHome = TouchButton::allocButton(false);
	displayMainMenuPage();
}

void displayMainMenuPage(void) {
	clearDisplay(COLOR_BACKGROUND_DEFAULT);
	initMainHomeButton(false);
	// draw all buttons except last
	for (unsigned int i = 0; i < (sizeof(TouchButtonsMainMenu) / sizeof(TouchButtonsMainMenu[0]) - 1); ++i) {
		(*TouchButtonsMainMenu[i])->drawButton();
	}
}

// HOME button - lower right corner
void initMainHomeButton(bool doDraw) {
	initMainHomeButtonWithPosition(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, doDraw);
}

// HOME button
void initMainHomeButtonWithPosition(const uint16_t aPositionX, const uint16_t aPositionY, bool doDraw) {
	TouchButtonMainHome->initSimpleButton(aPositionX, aPositionY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, MAIN_MENU_COLOR, StringHome, 2,
			0, &doMainMenuHomeButton);

	if (doDraw) {
		TouchButtonMainHome->drawButton();
	}
}

void loopMainMenuPage(void) {
	CheckTouchGeneric(true);
	showRTCTimeEverySecond(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4 - FONT_HEIGHT, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
}

void stopMainMenuPage(void) {
	// free buttons
	for (unsigned int i = 0; i < sizeof(TouchButtonsMainMenu) / sizeof(TouchButtonsMainMenu[0]); ++i) {
		(*TouchButtonsMainMenu[i])->setFree();
	}
}

