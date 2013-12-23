/*
 * PageTests.cpp
 *
 * @date 16.03.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#include "Pages.h"
#include "TouchDSO.h"

#include "misc.h"
#include "main.h"
#include <string.h>
#include <locale.h>

#ifdef __cplusplus
extern "C" {
#include "stm32f3_discovery.h"
#include "usb_misc.h"
#include "diskio.h"
#include "ff.h"

}
#endif

/* Private variables ---------------------------------------------------------*/
static TouchButton * TouchButtonBack;
static bool sBackButtonPressed;
static TouchButton * TouchButtonTestFont1;
static TouchButton * TouchButtonTestFont2;
static TouchButton * TouchButtonTestHY32D;
static TouchButton * TouchButtonTestGraphics;
static TouchButton * TouchButtonTestMMC;
static TouchButton * TouchButtonTestUSB;
static TouchButton * TouchButtonTestMisc;

static TouchButton * TouchButtonTestSystemInfo;
static TouchButton * TouchButtonTestExceptions;
static TouchButton * TouchButtonSettingsGamma1;
static TouchButton * TouchButtonSettingsGamma2;
// to call test functions
static TouchButton * TouchButtonTestFunction1;
static TouchButton * TouchButtonTestFunction2;
#define TEST_BUTTONS_NUMBER_TO_DISPLAY 11
static TouchButton ** TouchButtonsTestPage[] = { &TouchButtonTestFont1, &TouchButtonTestFont2, &TouchButtonTestHY32D,
		&TouchButtonTestMMC, &TouchButtonTestUSB, &TouchButtonTestSystemInfo, &TouchButtonTestExceptions, &TouchButtonTestMisc,
		&TouchButtonTestGraphics, &TouchButtonTestFunction1, &TouchButtonTestFunction2, &TouchButtonBack,
		&TouchButtonSettingsGamma1, &TouchButtonSettingsGamma2 };

int DebugValue1;
int DebugValue2;
int DebugValue3;
int DebugValue4;
int DebugValue5;

/* Private function prototypes -----------------------------------------------*/

bool checkTestsBackButton(void);
bool pickColorPeriodicCallbackHandler(const int aTouchPositionX, const int aTouchPositionY);
void displayTestsPage(void);

/* Private functions ---------------------------------------------------------*/

/**
 * for gamma tests in display test
 * @param aTheTouchedButton
 * @param aValue
 */
void doSetGamma(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	setGamma(aValue);
}

void doTestsButtons(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	clearDisplay(COLOR_BACKGROUND_DEFAULT);
	TouchButton::deactivateAllButtons();
	TouchButtonBack->drawButton();
	sBackButtonPressed = false;
	if (aTheTouchedButton == TouchButtonTestFont1) {
		uint16_t tXPos;
		uint16_t tYPos = 0;
		// printable ascii chars
		unsigned char tChar = FONT_START;
		for (int i = 6; i != 0; i--) {
			tXPos = 0;
			for (uint8_t j = 16; j != 0; j--) {
				tXPos = drawChar(tXPos, tYPos, tChar, 2, COLOR_BLACK, COLOR_YELLOW) + 4;
				tChar++;
			}
			tYPos += 2 * FONT_HEIGHT + 4;
		}
	} else if (aTheTouchedButton == TouchButtonTestFont2) {
		uint16_t tXPos;
		uint16_t tYPos = 0;
		// special chars
		unsigned char tChar = 0x80;
		for (int i = 8; i != 0; i--) {
			tXPos = 0;
			for (uint8_t j = 16; j != 0; j--) {
				tXPos = drawChar(tXPos, tYPos, tChar, 2, COLOR_BLACK, COLOR_YELLOW) + 4;
				tChar++;
			}
			tYPos += 2 * FONT_HEIGHT + 2;
		}
	}

	else if (aTheTouchedButton == TouchButtonTestHY32D) {
		generateColorSpectrum();
		TouchButtonSettingsGamma1->activate();
		TouchButtonSettingsGamma2->activate();
		while (!sBackButtonPressed) {
			if (CheckTouchGeneric(true) == GUI_TOUCH_NO_MATCH) {
				TouchPanel.registerPeriodicTouchCallback(&pickColorPeriodicCallbackHandler, 10);
			}
		}
	}

	else if (aTheTouchedButton == TouchButtonTestMMC) {
		int tRes = getCardInfo(StringBuffer, sizeof(StringBuffer));
		if (tRes == 0 || tRes > RES_PARERR) {
			drawMLText(0, 10, DISPLAY_WIDTH, 10 + 4 * FONT_HEIGHT, StringBuffer, 1, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
		} else {
			testAttachMMC();
		}
		if (tRes > RES_PARERR) {
			tRes = getFSInfo(StringBuffer, sizeof(StringBuffer));
			if (tRes == 0 || tRes > FR_INVALID_PARAMETER) {
				drawMLText(0, 20 + 4 * FONT_HEIGHT, DISPLAY_WIDTH, 20 + 14 * FONT_HEIGHT, StringBuffer, 1, COLOR_RED,
				COLOR_BACKGROUND_DEFAULT);
			}
		}
	}

	else if (aTheTouchedButton == TouchButtonTestUSB) {
		do {
			drawText(10, 10 + FONT_HEIGHT, getUSBDeviceState(), 1, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
		} while (!checkTestsBackButton());
	}

	else if (aTheTouchedButton == TouchButtonTestSystemInfo) {
		int tYPos = 2;
		unsigned int tControlRegisterContent = __get_CONTROL();
		snprintf(StringBuffer, sizeof StringBuffer, "Control=%#x xPSR=%#lx", tControlRegisterContent, __get_xPSR());
		drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
		tYPos += FONT_HEIGHT;
		snprintf(StringBuffer, sizeof StringBuffer, "MSP=%#lx PSP=%#lx", __get_MSP(), __get_PSP());
		drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);

// Clocks
		tYPos += FONT_HEIGHT;
		RCC_GetClocksFreq(&RCC_Clocks);
		snprintf(StringBuffer, sizeof StringBuffer, "SYSCLK_=%ldMHz HCLK=%ldMHz ADC12=%ldMHz",
				RCC_Clocks.SYSCLK_Frequency / 1000000, RCC_Clocks.HCLK_Frequency / 1000000,
				RCC_Clocks.ADC12CLK_Frequency / 1000000);
		drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
		tYPos += FONT_HEIGHT;
		snprintf(StringBuffer, sizeof StringBuffer, "PCLK1=%ldMHz PCLK2=%ldMHz I2C1=%ldMHz", RCC_Clocks.PCLK1_Frequency / 1000000,
				RCC_Clocks.PCLK2_Frequency / 1000000, RCC_Clocks.I2C1CLK_Frequency / 1000000);
		drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
		tYPos += FONT_HEIGHT;
		snprintf(StringBuffer, sizeof StringBuffer, "TIM1=%ldMHz TIM8=%ldMHz", RCC_Clocks.TIM1CLK_Frequency / 1000000,
				RCC_Clocks.TIM8CLK_Frequency / 1000000);
		drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
		tYPos += FONT_HEIGHT;
		snprintf(StringBuffer, sizeof StringBuffer, "USART1=%ldMHz USART2=%ldMHz USART3=%ldMHz USART4=%ldMHz",
				RCC_Clocks.USART1CLK_Frequency / 1000000, RCC_Clocks.USART2CLK_Frequency / 1000000,
				RCC_Clocks.USART3CLK_Frequency / 1000000, RCC_Clocks.UART4CLK_Frequency / 1000000);
		drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);

// Locale
		tYPos += 2 * FONT_HEIGHT;
		struct lconv * tLconfPtr = localeconv();
		snprintf(StringBuffer, sizeof StringBuffer, "Locale=%s decimalpoint=%s thousands=%s", setlocale(LC_ALL, NULL),
				tLconfPtr->decimal_point, tLconfPtr->thousands_sep);
		drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);

// Button Pool Info
		tYPos += 2 * FONT_HEIGHT;
		snprintf(StringBuffer, sizeof StringBuffer, "BUTTONS: poolsize=%d + %d", NUMBER_OF_BUTTONS_IN_POOL,
		NUMBER_OF_AUTOREPEAT_BUTTONS_IN_POOL);
		drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
		tYPos += FONT_HEIGHT;
		TouchButton::infoButtonPool(StringBuffer);
		drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);

// Lock info
		tYPos += FONT_HEIGHT;
		snprintf(StringBuffer, sizeof StringBuffer, "LOCK count=%d", sLockCount);
		drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);

// Debug Info - just in case
		tYPos += FONT_HEIGHT;
		snprintf(StringBuffer, sizeof StringBuffer, "Dbg: 1=%#X, 1=%d 2=%#X", DebugValue1, DebugValue1, DebugValue2);
		drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
		tYPos += FONT_HEIGHT;
		snprintf(StringBuffer, sizeof StringBuffer, "Dbg: 3=%#X 4=%#X 5=%#X", DebugValue3, DebugValue4, DebugValue5);
		drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);

	} else if (aTheTouchedButton == TouchButtonTestExceptions) {
		assertParamMessage((false), 0x12345678, "Error Message");
		delayMillis(2000);
		snprintf(StringBuffer, sizeof StringBuffer, "Dummy %#lx", *((volatile uint32_t *) (0xFFFFFFFF)));
	} else if (aTheTouchedButton == TouchButtonTestFunction1) {
		// Call Function 1
		initalizeDisplay2();
	} else if (aTheTouchedButton == TouchButtonTestFunction2) {
		STM_EVAL_LEDToggle(LED3); // RED BACK
	}

	else if (aTheTouchedButton == TouchButtonTestMisc) {
		int YPos = 10;
		float tTest = 1.23456789;
		snprintf(StringBuffer, sizeof StringBuffer, "5.2f=%5.2f 0.3f=%0.3f 4f=%4f", tTest, tTest, tTest);
		drawText(10, YPos, StringBuffer, 1, COLOR_RED, COLOR_BACKGROUND_DEFAULT);

		YPos += FONT_HEIGHT;
		tTest = -1.23456789;
		snprintf(StringBuffer, sizeof StringBuffer, "5.2f=%5.2f 0.3f=%0.3f 4f=%4f", tTest, tTest, tTest);
		drawText(10, YPos, StringBuffer, 1, COLOR_RED, COLOR_BACKGROUND_DEFAULT);

		YPos += FONT_HEIGHT;
		tTest = 12345.6789;
		snprintf(StringBuffer, sizeof StringBuffer, "7.2f=%7.2f 7f=%7f", tTest, tTest);
		drawText(10, YPos, StringBuffer, 1, COLOR_RED, COLOR_BACKGROUND_DEFAULT);

		YPos += FONT_HEIGHT;
		tTest = 0.0123456789;
		snprintf(StringBuffer, sizeof StringBuffer, "3.2f=%3.2f 0.4f=%0.4f 4f=%4f", tTest, tTest, tTest);
		drawText(10, YPos, StringBuffer, 1, COLOR_RED, COLOR_BACKGROUND_DEFAULT);

		YPos += FONT_HEIGHT;
		uint16_t t16 = 0x02CF;
		snprintf(StringBuffer, sizeof StringBuffer, "#x=%#x X=%X 5x=%5x 0#8X=%0#8X", t16, t16, t16, t16);
		drawText(10, YPos, StringBuffer, 1, COLOR_RED, COLOR_BACKGROUND_DEFAULT);

		YPos += FONT_HEIGHT;
		displayTimings(YPos);

		testDSOConversions();

		do {
			testTimingsLoop(1);
		} while (!checkTestsBackButton());

	} else if (aTheTouchedButton == TouchButtonTestGraphics) {
		TouchButtonBack->activate();
		testHY32D(1);
		TouchButtonBack->drawButton();
	}
}

bool pickColorPeriodicCallbackHandler(const int aTouchPositionX, const int aTouchPositionY) {
	uint16_t tColor = readPixel(aTouchPositionX, aTouchPositionY);
	snprintf(StringBuffer, sizeof StringBuffer, "%3d %3d R=0x%2x G=0x%2x B=0x%2x", aTouchPositionX, aTouchPositionY,
			(tColor >> 11) << 3, ((tColor >> 5) & 0x3F) << 2, (tColor & 0x1F) << 3);
	drawText(2, DISPLAY_HEIGHT - FONT_HEIGHT, StringBuffer, 1, COLOR_WHITE, COLOR_BLACK);
	// show color
	fillRect(DISPLAY_WIDTH - 30, DISPLAY_HEIGHT - 30, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, tColor);
	return true; // return parameter not yet used
}

/**
 * Calls callback routine of TestsBackButton if button was touched
 * @param DoNotCheckTouchPanelWasTouched
 * @return true if touched
 */bool checkTestsBackButton(void) {
	if (TouchPanel.wasTouched()) {
		return TouchButtonBack->checkButton(TouchPanel.getXActual(), TouchPanel.getYActual(), true);
	}
	return false;
}

/**
 * switch back to tests menu
 */
void doTestsBackButton(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	displayTestsPage();
	sBackButtonPressed = true;
}

void initTestsPage(void) {
}

void displayTestsPage(void) {
	clearDisplay(COLOR_BACKGROUND_DEFAULT);
	for (unsigned int i = 0; i < TEST_BUTTONS_NUMBER_TO_DISPLAY; ++i) {
		(*TouchButtonsTestPage[i])->drawButton();
	}
	TouchButtonMainHome->drawButton();
}

/**
 * allocate and position all BUTTONS
 */
void startTestsPage(void) {

	TouchButton::setDefaultButtonColor(COLOR_GREEN);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"

	TouchButtonTestFont1 = TouchButton::allocAndInitSimpleButton(0, 0, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "Font 1", 2, 0,
			&doTestsButtons);
	TouchButtonTestFont2 = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, 0, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
			"Font 2", 2, 0, &doTestsButtons);

	TouchButtonTestHY32D = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, 0, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
			"Display Test", 1, 0, &doTestsButtons);

	// 2. row
	TouchButtonTestMMC = TouchButton::allocAndInitSimpleButton(0, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
			"MMC Infos", 1, 0, &doTestsButtons);

	TouchButtonTestUSB = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_3,
	BUTTON_HEIGHT_4, 0, "USB Infos", 1, 0, &doTestsButtons);

	TouchButtonTestSystemInfo = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_3,
	BUTTON_HEIGHT_4, 0, "System Infos", 1, 0, &doTestsButtons);

	// 3. row
	TouchButtonTestExceptions = TouchButton::allocAndInitSimpleButton(0, BUTTON_HEIGHT_4_LINE_3, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
			"Exceptions", 1, 0, &doTestsButtons);

	TouchButtonTestMisc = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_3, BUTTON_WIDTH_3,
	BUTTON_HEIGHT_4, 0, "Misc Test", 1, 0, &doTestsButtons);

	TouchButtonTestGraphics = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_3, BUTTON_WIDTH_3,
	BUTTON_HEIGHT_4, 0, "Graph. Test", 1, 0, &doTestsButtons);

	// 4. row
	TouchButtonTestFunction1 = TouchButton::allocAndInitSimpleButton(0, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
			"Function 1", 1, 0, &doTestsButtons);

	TouchButtonTestFunction2 = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
	BUTTON_HEIGHT_4, 0, "Function 2", 1, 0, &doTestsButtons);

	// button for sub pages
	TouchButtonSettingsGamma1 = TouchButton::allocAndInitSimpleButton(0, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
	NULL, 0, 0, &doSetGamma);
	TouchButtonSettingsGamma2 = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
	BUTTON_HEIGHT_4, 0, NULL, 0, 1, &doSetGamma);
	TouchButtonBack = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
	BUTTON_HEIGHT_4, COLOR_RED, StringBack, 2, -1, &doTestsBackButton);

#pragma GCC diagnostic pop

	displayTestsPage();
}

/**
 * cleanup on leaving this page
 */
void stopTestsPage(void) {
	// free buttons
	for (unsigned int i = 0; i < sizeof(TouchButtonsTestPage) / sizeof(TouchButtonsTestPage[0]); ++i) {
		(*TouchButtonsTestPage[i])->setFree();
	}
}
