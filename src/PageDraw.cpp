/*
 * PageDraw.cpp
 *
 * @date 16.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 */

#include "Pages.h"
#include "TouchLib.h"

#include "misc.h"
#include <string.h>

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static struct XYPosition sLastPos;
static uint16_t sDrawColor = COLOR_BLACK;
static bool mButtonTouched;

static TouchButton * TouchButtonClear;
#define NUMBER_OF_DRAW_COLORS 5
static TouchButton * TouchButtonsDrawColor[NUMBER_OF_DRAW_COLORS];
static const uint16_t DrawColors[NUMBER_OF_DRAW_COLORS] = { COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_YELLOW };

void initDrawPage(void) {
}

void displayDrawPage(void) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
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

/*
 * Position changed -> draw line
 */
void drawPageTouchMoveCallbackHandler(struct XYPosition * const aActualPositionPtr) {
    if (!mButtonTouched) {
        BlueDisplay1.drawLine(sLastPos.PosX, sLastPos.PosY, aActualPositionPtr->PosX, aActualPositionPtr->PosY, sDrawColor);
        sLastPos.PosX = aActualPositionPtr->PosX;
        sLastPos.PosY = aActualPositionPtr->PosY;
    }
}

/*
 * Touch is going down on canvas -> draw starting point
 */
void drawPageTouchDownCallbackHandler(struct XYPosition * const aActualPositionPtr) {
    // first check buttons
    mButtonTouched = TouchButton::checkAllButtons(aActualPositionPtr->PosX, aActualPositionPtr->PosY, true);
    if (!mButtonTouched) {
        int x = aActualPositionPtr->PosX;
        int y = aActualPositionPtr->PosY;
        BlueDisplay1.drawPixel(x, y, sDrawColor);
        sLastPos.PosX = x;
        sLastPos.PosY = y;
    }
}

void startDrawPage(void) {
    // Color buttons
    uint16_t tPosY = 0;
    for (uint8_t i = 0; i < 5; ++i) {
        TouchButtonsDrawColor[i] = TouchButton::allocButton(false);
        TouchButtonsDrawColor[i]->initButton(0, tPosY, 30, 30, NULL, TEXT_SIZE_11, DrawColors[i], COLOR_BLACK, i, &doDrawColor);
        tPosY += 30;
    }

    TouchButtonClear = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
            BUTTON_HEIGHT_4, COLOR_RED, StringClear, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doDrawClear);

    registerTouchDownCallback(&drawPageTouchDownCallbackHandler);
    // for sub pages
    registerTouchMoveCallback(&drawPageTouchMoveCallbackHandler);

    registerSimpleResizeAndReconnectCallback(&displayDrawPage);
    displayDrawPage();
    // to avoid first line because of moves after touch of the button starting this page
    mButtonTouched = true;
}

void loopDrawPage(void) {
    checkAndHandleEvents();
}

void stopDrawPage(void) {
// free buttons
    for (unsigned int i = 0; i < NUMBER_OF_DRAW_COLORS; ++i) {
        (TouchButtonsDrawColor[i])->setFree();
    }
    TouchButtonClear->setFree();

    registerTouchDownCallback(&simpleTouchDownHandler);
    // for sub pages
    registerTouchMoveCallback(&simpleTouchMoveHandlerForSlider);
}

