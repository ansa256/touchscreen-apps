/**
 * @file GuiDemo.cpp
 *
 * @date 31.01.2012
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 *
 *      Demo of the libs:
 *      TouchButton
 *      TouchSlider
 *      Chart
 *
 *      and the "Apps" ;-)
 *      Draw lines
 *      Game of life
 *      Show ADS7846 A/D channels
 *      Display font
 *
 *      For STM32F3 Discovery
 *
 */

#include "Pages.h"

#include "GameOfLife.h"
#include "Chart.h"
#include "misc.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {

#include "stm32f30xPeripherals.h"
#include "timing.h"

}
#endif

/** @addtogroup Gui_Demo
 * @{
 */

/*
 * LCD and touch panel stuff
 */

#define BACKGROUND_COLOR COLOR_WHITE

void createDemoButtonsAndSliders(void);

// Global button handler
void doGuiDemoButtons(TouchButton * const aTheTochedButton, int16_t aValue);

//Global slider handler
uint16_t doSliders(TouchSlider * const aTheTochedSlider, uint16_t aSliderValue);

static TouchButton * TouchButtonBack;

static TouchButton * TouchButtonContinue;

/*
 * Game of life stuff
 */
static TouchButton * TouchButtonDemoSettings;
void showGolSettings(void);

static TouchButton * TouchButtonGolDying;
static TouchSlider TouchSliderGolDying;
void doGolDying(TouchButton * const aTheTouchedButton, int16_t aValue);
void syncronizeGolSliderAndButton(void);
const char * mapValueGol(uint16_t aValue);

static TouchButton * TouchButtonNew;
void doNew(TouchButton * const aTheTouchedButton, int16_t aValue);
void doContinue(TouchButton * const aTheTouchedButton, int16_t aValue);
void initNewGameOfLife(void);

static TouchSlider TouchSliderGolSpeed;

const char StringGOL[] = "GameOfLife";
// to slow down game of life
unsigned long GolDelay = 0;
bool GolShowDying = true;
bool GolRunning = false;
bool GolInitialized = false;
#define GOL_DIE_MAX 20

/**
 * Menu stuff
 */
void showGuiDemoMenu(void);

static TouchButton * TouchButtonChartDemo;
void showCharts(void);
Chart ChartExample;

// space between buttons
#define APPLICATION_MENU 0
#define APPLICATION_SETTINGS 1
#define APPLICATION_DRAW 2
#define APPLICATION_GAME_OF_LIFE 3
#define APPLICATION_CHART 4
#define APPLICATION_ADS7846_CHANNELS 5

int mActualApplication = APPLICATION_MENU;

/*
 * Settings stuff
 */
void showSettings(void);

static TouchButton * TouchButtonCalibration;
static TouchButton * TouchButtonGameOfLife;
const char StringTPCalibration[] = "TP-Calibration";

static TouchSlider TouchSliderDemo1;
static TouchSlider TouchSliderAction;
static TouchSlider TouchSliderActionWithoutBorder;
unsigned int ActionSliderValue = 0;
#define ACTION_SLIDER_MAX 100
bool ActionSliderUp = true;

/*
 * ADS7846 channels stuff
 */
static TouchButton * TouchButtonADS7846Channels;
void ADS7846DisplayChannels(void);
void doADS7846Channels(TouchButton * const aTheTouchedButton, int16_t aValue);

void initGuiDemo(void) {
}

static TouchButton ** TouchButtonsGuiDemo[] = { &TouchButtonDemoSettings, &TouchButtonChartDemo, &TouchButtonGameOfLife,
		&TouchButtonADS7846Channels, &TouchButtonBack, &TouchButtonCalibration, &TouchButtonGolDying, &TouchButtonNew,
		&TouchButtonContinue };

void startGuiDemo(void) {
	createDemoButtonsAndSliders();
	initBacklightElements();

	showGuiDemoMenu();
	MillisLastLoop = getMillisSinceBoot();
}

void loopGuiDemo(int aGuiTouchState) {

	// count milliseconds for loop control
	uint32_t tMillis = getMillisSinceBoot();
	MillisSinceLastAction += tMillis - MillisLastLoop;
	MillisLastLoop = tMillis;

	if (aGuiTouchState == GUI_TOUCH_NO_MATCH) {
		/*
		 * switch for different "apps"
		 */
		switch (mActualApplication) {
		case APPLICATION_GAME_OF_LIFE:
			if (GolRunning) {
				showGolSettings();
			}
			break;
		}
	}

	/*
	 * switch for different "apps"
	 */
	switch (mActualApplication) {
	case APPLICATION_SETTINGS:
		// Moving slider bar :-)
		if (MillisSinceLastAction >= 20) {
			MillisSinceLastAction = 0;
			if (ActionSliderUp) {
				ActionSliderValue++;
				if (ActionSliderValue == ACTION_SLIDER_MAX) {
					ActionSliderUp = false;
				}
			} else {
				ActionSliderValue--;
				if (ActionSliderValue == 0) {
					ActionSliderUp = true;
				}
			}
			TouchSliderAction.setActualValueAndDraw(ActionSliderValue);
			TouchSliderActionWithoutBorder.setActualValueAndDraw(ACTION_SLIDER_MAX - ActionSliderValue);
		}
		break;

	case APPLICATION_GAME_OF_LIFE:
		if (GolRunning && MillisSinceLastAction >= GolDelay) {
			//game of live "app"
			play_gol();
			drawGenerationText();
			draw_gol();
			MillisSinceLastAction = 0;
		}
		break;
	}

	/*
	 * Actions independent of touch
	 */
	if (mActualApplication == APPLICATION_ADS7846_CHANNELS) {
		ADS7846DisplayChannels();
	}

	if (mActualApplication == APPLICATION_MENU) {
		showRTCTimeEverySecond(10, DISPLAY_HEIGHT - FONT_HEIGHT - 1, COLOR_RED, BACKGROUND_COLOR);
	}
}

void stopGuiDemo(void) {
	// free buttons
	for (unsigned int i = 0; i < sizeof(TouchButtonsGuiDemo) / sizeof(TouchButtonsGuiDemo[0]); ++i) {
		(*TouchButtonsGuiDemo[i])->setFree();
	}
	deinitBacklightElements();

}

/*
 * buttons are positioned relative to sliders and vice versa
 */
void createDemoButtonsAndSliders(void) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
	/*
	 * create and position all BUTTONS initially
	 */
	TouchButton::setDefaultButtonColor(COLOR_RED);
	TouchSlider::resetDefaults();

	// HOME text button - lower right corner
	TouchButtonBack = TouchButton::allocAndInitSimpleButton(DISPLAY_WIDTH - (8 * FONT_WIDTH), DISPLAY_HEIGHT - (2 * FONT_HEIGHT), 0,
			0, COLOR_RED, StringBack, 2, 0, &doGuiDemoButtons);

	TouchButtonGameOfLife = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_2_POS_2, 0, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, 0,
			StringGOL, 1, 0, &doGuiDemoButtons);

	TouchButtonCalibration = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_2_POS_2, 0, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, 0,
			StringTPCalibration, 1, 0, &doGuiDemoButtons);

	TouchButtonChartDemo = TouchButton::allocAndInitSimpleButton(0, 0, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, 0, "Chart", 2, 0,
			&doGuiDemoButtons);

	TouchButtonADS7846Channels = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_2_POS_2, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_2,
			BUTTON_HEIGHT_4, 0, "ADS7846", 2, 0, &doADS7846Channels);

	TouchButtonDemoSettings = TouchButton::allocAndInitSimpleButton(0, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, 0,
			StringSettings, 2, 0, &doGuiDemoButtons);

	TouchButtonNew = TouchButton::allocAndInitSimpleButton(0, DISPLAY_HEIGHT - (2 * FONT_HEIGHT), 0, 0, 0, "New", 2, 0, &doNew);

	TouchButtonContinue = TouchButton::allocAndInitSimpleButton(TouchButtonNew->getPositionXRight() + 45,
			DISPLAY_HEIGHT - (2 * FONT_HEIGHT), 0, 0, 0, StringContinue, 2, 0, &doContinue);

	/*
	 * Slider
	 */

	TouchSliderGolSpeed.initSlider(80, 70, TOUCHSLIDER_DEFAULT_SIZE, 75, 75, 75, "Gol-Speed", TOUCHSLIDER_DEFAULT_TOUCH_BORDER,
			TOUCHSLIDER_SHOW_BORDER | TOUCHSLIDER_SHOW_VALUE | TOUCHSLIDER_IS_HORIZONTAL, &doSliders, &mapValueGol);
// slider as switch
	TouchSliderGolDying.initSlider(100, TouchSliderGolSpeed.getPositionYBottom() + (3 * FONT_HEIGHT), TOUCHSLIDER_DEFAULT_SIZE,
			GOL_DIE_MAX, GOL_DIE_MAX, GOL_DIE_MAX, "Show dying", 20, TOUCHSLIDER_SHOW_BORDER, &doSliders, NULL);

// ON OFF button relative to slider
	TouchButtonGolDying = TouchButton::allocAndInitSimpleButton(100, TouchSliderGolDying.getPositionYBottom() + FONT_HEIGHT + 10,
			25, 25, 0, NULL, 1, 0, &doGolDying);

// self moving sliders
	TouchSliderActionWithoutBorder.initSlider(180, BUTTON_HEIGHT_4 + 30, 10, ACTION_SLIDER_MAX, ACTION_SLIDER_MAX, 0, NULL, 3,
			TOUCHSLIDER_SHOW_VALUE, NULL, NULL);
	TouchSliderAction.initSlider(TouchSliderActionWithoutBorder.getPositionXRight() + 15, BUTTON_HEIGHT_4 + 25, 10,
			ACTION_SLIDER_MAX, ACTION_SLIDER_MAX, 0, NULL, 0, TOUCHSLIDER_SHOW_BORDER | TOUCHSLIDER_SHOW_VALUE, NULL, NULL);

#pragma GCC diagnostic pop

}

uint16_t doSliders(TouchSlider * const aTheTouchedSlider, uint16_t aSliderValue) {
	if (aTheTouchedSlider == &TouchSliderGolSpeed) {
		aSliderValue = (aSliderValue / 25) * 25;
	}
	if (aTheTouchedSlider == &TouchSliderGolDying) {
		if (aSliderValue > (GOL_DIE_MAX / 2)) {
			GolShowDying = true;
			aSliderValue = GOL_DIE_MAX;
		} else {
			GolShowDying = false;
			aSliderValue = 0;
		}
		syncronizeGolSliderAndButton();
	}
	return aSliderValue;
}

void doGuiDemoButtons(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	TouchButton::deactivateAllButtons();
	TouchSlider::deactivateAllSliders();
	if (aTheTouchedButton == TouchButtonChartDemo) {
		showCharts();
		return;
	}

	if (aTheTouchedButton == TouchButtonCalibration) {
		//Calibration Button pressed -> calibrate touch panel
		TouchPanel.doCalibration(false);
		showSettings();
		return;
	}
	if (aTheTouchedButton == TouchButtonGameOfLife) {
		// Game of Life button pressed
		showGolSettings();
		mActualApplication = APPLICATION_GAME_OF_LIFE;
		return;
	}
	if (aTheTouchedButton == TouchButtonDemoSettings) {
		// Settings button pressed
		showSettings();
		return;
	}
	if (aTheTouchedButton == TouchButtonBack) {
		// Home button pressed
		showGuiDemoMenu();
		return;
	}
}

void doADS7846Channels(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	TouchButton::deactivateAllButtons();
	mActualApplication = APPLICATION_ADS7846_CHANNELS;
	clearDisplay(BACKGROUND_COLOR);
	uint16_t tPosY = 30;
	// draw text
	for (uint8_t i = 0; i < 8; ++i) {
		drawText(90, tPosY, (char *) ADS7846ChannelStrings[i], 2, COLOR_RED, BACKGROUND_COLOR);
		tPosY += FONT_HEIGHT * 2;
	}
	TouchButtonBack->drawButton();

}

char StringSlowest[] = "slowest";
char StringSlow[] = "slow   ";
char StringNormal[] = "normal ";
char StringFast[] = "fast   ";

// value map function for game of life speed slider
const char * mapValueGol(uint16_t aValue) {
	aValue = aValue / 25;
	switch (aValue) {
	case 0:
		GolDelay = 8000;
		return StringSlowest;
	case 1:
		GolDelay = 2000;
		return StringSlow;
	case 2:
		GolDelay = 500;
		return StringNormal;
	case 3:
		GolDelay = 0;
		return StringFast;
	}
	return StringFast;
}

void doGolDying(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	/*
	 * GolDying button pressed
	 */
	if (GolShowDying == false) {
		GolShowDying = true;
	} else {
		GolShowDying = false;
	}
	syncronizeGolSliderAndButton();
}

void showGolSettings(void) {
	/*
	 * Settings button pressed
	 */
	GolRunning = false;
	clearDisplay(BACKGROUND_COLOR);
	TouchButtonBack->drawButton();
	TouchSliderGolDying.drawSlider();
	TouchButtonGolDying->drawButton();
	syncronizeGolSliderAndButton();
	TouchSliderGolSpeed.drawSlider();
// redefine Buttons
	TouchButtonNew->drawButton();
	TouchButtonContinue->drawButton();
}

void doNew(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
// deactivate gui elements
	TouchButton::deactivateAllButtons();
	TouchSlider::deactivateAllSliders();
	initNewGameOfLife();
	MillisSinceLastAction = GolDelay;
	GolRunning = true;
}

void doContinue(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
// deactivate gui elements
	TouchButton::deactivateAllButtons();
	TouchSlider::deactivateAllSliders();
	if (!GolInitialized) {
		initNewGameOfLife();
	} else {
		ClearScreenAndDrawGameOfLifeGrid();
	}
	MillisSinceLastAction = GolDelay;
	GolRunning = true;
}

void initNewGameOfLife(void) {
	GolInitialized = true;
	init_gol();
}

void showSettings(void) {
	/*
	 * Settings button pressed
	 */
	clearDisplay(BACKGROUND_COLOR);
	TouchButtonBack->drawButton();
	TouchButtonCalibration->drawButton();
	TouchSliderGolDying.drawSlider();
	TouchButtonGolDying->drawButton();
	syncronizeGolSliderAndButton();
	drawBacklightElements();
	TouchSliderGolSpeed.drawSlider();
	TouchSliderAction.drawSlider();
	TouchSliderActionWithoutBorder.drawSlider();
	mActualApplication = APPLICATION_SETTINGS;
}

void showGuiDemoMenu(void) {
	TouchButtonBack->deactivate();

	clearDisplay(BACKGROUND_COLOR);
	TouchButtonMainHome->drawButton();
	TouchButtonDemoSettings->drawButton();
	TouchButtonChartDemo->drawButton();
	TouchButtonADS7846Channels->drawButton();
	TouchButtonGameOfLife->drawButton();
	GolInitialized = false;
	mActualApplication = APPLICATION_MENU;
}

void showFont(void) {
	clearDisplay(BACKGROUND_COLOR);
	TouchButtonBack->drawButton();

	uint16_t tXPos;
	uint16_t tYPos = 10;
	unsigned char tChar = FONT_START;
	for (uint8_t i = 14; i != 0; i--) {
		tXPos = 10;
		for (uint8_t j = 16; j != 0; j--) {
			tXPos = drawChar(tXPos, tYPos, tChar, 1, COLOR_BLACK, COLOR_YELLOW) + 4;
			tChar++;
		}
		tYPos += FONT_HEIGHT + 4;
	}
}

void showCharts(void) {
	clearDisplay(BACKGROUND_COLOR);
	TouchButtonBack->drawButton();
	unsigned int i;

	/*
	 * 1. Chart: pixel with grid and no labels, 120 values
	 */
	ChartExample.disableXLabel();
	ChartExample.disableYLabel();
	ChartExample.initChartColors(COLOR_RED, COLOR_RED, CHART_DEFAULT_GRID_COLOR, COLOR_RED, BACKGROUND_COLOR);
	ChartExample.initChart(5, 100, 120, 90, 2, true, 20, 20);
	ChartExample.drawAxesAndGrid();

//generate random data
	srand(120);
	char * tPtr = StringBuffer;
	for (i = 0; i < sizeof StringBuffer; i++) {
		*tPtr++ = 30 + (rand() >> 27);
	}
	ChartExample.drawChartDataDirect((uint8_t *) &StringBuffer, sizeof StringBuffer, CHART_MODE_PIXEL);

	/*
	 * 2. Chart: with grid with (negative) integer labels, 140 values
	 */
	ChartExample.initXLabelInt(0, 12, 1, 2);
	ChartExample.initYLabelInt(-20, 20, 20 / 15, 3);
	ChartExample.initChart(170, 100, 140, 88, 2, true, 30, 15);
	ChartExample.drawAxesAndGrid();
// new data
	uint16_t * tPtr2 = FourDisplayLinesBuffer;
	uint16_t tVal = 0;
	for (i = 0; i < sizeof FourDisplayLinesBuffer / 2; i++) {
		tVal += rand() >> 30;
		*tPtr2++ = tVal;
	}
	tPtr2--;
	*tPtr2 = 30;
	ChartExample.setDataColor(COLOR_BLUE);
	ChartExample.drawChartData((int16_t *) FourDisplayLinesBuffer, (int16_t *) &FourDisplayLinesBuffer[SIZEOF_DISPLAYLINE_BUFFER],
			CHART_MODE_LINE);

	/*
	 * 3. Chart: without grid with float labels, 100 values
	 */
	ChartExample.initXLabelFloat(0, 0.5, 1, 3, 1);
	ChartExample.initYLabelFloat(0, 0.2, 1, 3, 1);
	ChartExample.initChart(30, DISPLAY_HEIGHT - 20, 100, 90, 2, false, 40, 20);
	ChartExample.initChartColors(COLOR_BLUE, COLOR_RED, COLOR_BLACK, COLOR_RED, BACKGROUND_COLOR);
	ChartExample.drawAxesAndGrid();

	/*
	 * 4. Chart: with grid with x integer and y float labels, 140 values area mode
	 */
	ChartExample.initXLabelInt(0, 20, 1, 2);
	ChartExample.initYLabelFloat(0, 0.3, 0.9 / 60, 3, 1); // display 0.9 for raw value of 60
	ChartExample.initChart(170, DISPLAY_HEIGHT - 40, 140, 70, 2, true, 30, 16);
	ChartExample.drawAxesAndGrid();
	ChartExample.drawChartData((int16_t *) FourDisplayLinesBuffer, (int16_t *) &FourDisplayLinesBuffer[SIZEOF_DISPLAYLINE_BUFFER],
			CHART_MODE_AREA);

	mActualApplication = APPLICATION_CHART;
}

void syncronizeGolSliderAndButton(void) {
	if (GolShowDying == false) {
		TouchButtonGolDying->setColor(TOUCHBUTTON_DEFAULT_COLOR);
		TouchButtonGolDying->drawButton();
		TouchSliderGolDying.setActualValueAndDraw(0);
	} else {
		TouchButtonGolDying->setColor(TOUCHSLIDER_DEFAULT_BAR_COLOR);
		TouchButtonGolDying->drawButton();
		TouchSliderGolDying.setActualValueAndDraw(GOL_DIE_MAX);
	}
}

#define BUTTON_CHECK_INTERVAL 20
void ADS7846DisplayChannels(void) {
	static uint8_t aButtonCheckInterval = 0;
	uint16_t tPosY = 30;
	int16_t tTemp;
	bool tUseDiffMode = true;
	bool tUse12BitMode = false;

	for (uint8_t i = 0; i < 8; ++i) {
		if (i == 4) {
			tUseDiffMode = false;
		}
		if (i == 2) {
			tUse12BitMode = true;
		}
		tTemp = TouchPanel.readChannel(ADS7846ChannelMapping[i], tUse12BitMode, tUseDiffMode, 8);
		snprintf(StringBuffer, sizeof StringBuffer, "%04u", tTemp);
		drawText(15, tPosY, StringBuffer, 2, COLOR_RED, BACKGROUND_COLOR);
		tPosY += FONT_HEIGHT * 2;
	}
	aButtonCheckInterval++;
	if (aButtonCheckInterval >= BUTTON_CHECK_INTERVAL) {
		TouchPanel.rd_data();
		if (TouchPanel.mPressure > MIN_REASONABLE_PRESSURE) {
			TouchButtonBack->checkButton(TouchPanel.getXActual(), TouchPanel.getYActual(), true);
		}

	}
}

/**
 * @}
 */
