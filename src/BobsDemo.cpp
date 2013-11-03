/**
 * BobsDemo.cpp
 *
 * @ brief demo playground for accelerator etc.
 *
 * @date 16.12.2012
 * @author Robert
 */
#include "stm32f30x.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "stm32f30xPeripherals.h"
#include "timing.h"
#ifdef __cplusplus
}
#endif

#include "Pages.h"

/**
 *
 */

#define ABS(x)         (x < 0) ? (-x) : x

#define DISPLAY_HEIGHT 240
#define DISPLAY_WIDTH 320

#define RADIUS_BALL 5
#define INCREMENT 2
#define COLOR_BALL COLOR_GREEN
int16_t LastX;
int16_t LastY;
int16_t AccumulatedValueX;
int16_t AccumulatedValueY;
int16_t direction;
static TouchButton * TouchButtonNextLevel;
bool GameActive;

#define HOME_X 220
#define HOME_Y 50
#define HOME_SIZE 2
#define HOME_MID_X (HOME_X + ((HOME_SIZE/2) * FONT_WIDTH))
#define HOME_MID_Y (HOME_Y + ((HOME_SIZE/2) * FONT_HEIGHT))
#define Y_RESOLUTION 64
#define X_RESOLUTION 64

void doNextLevel(TouchButton * const aTheTouchedButton, int16_t aValue);

void initBobsDemo(void) {

	initAcceleratorCompassChip();

	TouchButtonNextLevel = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
			BUTTON_HEIGHT_4, COLOR_CYAN, StringNext, 2, 0, &doNextLevel);
}

void startBobsDemo(void) {
	LastX = DISPLAY_WIDTH / 2;
	LastY = DISPLAY_HEIGHT / 2;
	AccumulatedValueX = 0;
	AccumulatedValueY = 0;
	clearDisplay(COLOR_BACKGROUND_DEFAULT);
	fillCircle(LastX, LastY, RADIUS_BALL, COLOR_BALL);
	direction = INCREMENT;

	TouchButtonMainHome->drawButton();
//	TouchButtonClearScreen.drawButton();
	//male Home
	drawChar(HOME_X, HOME_Y, 0xD3, HOME_SIZE, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
	GameActive = true;
}

void stopBobsDemo(void) {
	TouchButtonNextLevel->setFree();
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
/**
 * Bobs Demo
 */

void doNextLevel(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	startBobsDemo();
	//male Umrandung
	fillRect(HOME_X - 20, HOME_Y - 20, HOME_X, HOME_Y + 30, COLOR_BLUE);
	fillRect(HOME_X + FONT_WIDTH * HOME_SIZE, HOME_Y - 20, HOME_X + FONT_WIDTH * HOME_SIZE + 20, HOME_Y + 30, COLOR_BLUE);
	fillRect(HOME_X, HOME_Y - 40, HOME_X + FONT_WIDTH * HOME_SIZE, HOME_Y - 20, COLOR_BLUE);
}

void loopBobsDemo(void) {
	int16_t aAcceleratorRawDataBuffer[3];
	int16_t tNewX;
	int16_t tNewY;
	delayMillis(5);
	if (GameActive) {

		// berechne neue Position
		readAcceleratorZeroCompensated(aAcceleratorRawDataBuffer);

		/**
		 * X Achse
		 */
		AccumulatedValueX += aAcceleratorRawDataBuffer[0];
		tNewX = LastX + AccumulatedValueX / X_RESOLUTION;
		AccumulatedValueX = AccumulatedValueX % X_RESOLUTION;

		// prüfen ob am rechten Rand angekommen
		if (tNewX + RADIUS_BALL >= DISPLAY_WIDTH) {
			// stay
			tNewX = LastX;
		}
		// prüfen ob am linken Rand angekommen
		if (tNewX - RADIUS_BALL < 0) {
			tNewX = LastX;
		}

		/**
		 * Y Achse
		 */
		AccumulatedValueY += aAcceleratorRawDataBuffer[1];
		tNewY = LastY + AccumulatedValueY / Y_RESOLUTION;
		AccumulatedValueY = AccumulatedValueY % Y_RESOLUTION;

		// prüfen ob am unteren Rand angekommen
		if (tNewY + RADIUS_BALL >= DISPLAY_HEIGHT) {
			// stay
			tNewY = LastY;
		}
		// prüfen ob am oberen Rand angekommen
		if (tNewY - RADIUS_BALL < 0) {
			// stay
			tNewY = LastY;
		}

		// Prüfe ob Ziel berührt wird
		if (tNewX > (HOME_MID_X - RADIUS_BALL) && tNewX < (HOME_MID_X + RADIUS_BALL) && tNewY > (HOME_MID_Y - RADIUS_BALL)
				&& tNewY < (HOME_MID_Y + RADIUS_BALL)) {
			// Ziel berührt
			tone(440, 200);
			delayMillis(200);
			tone(660, 600);
			clearDisplay(COLOR_BACKGROUND_DEFAULT);
			TouchButtonMainHome->drawButton();
//			TouchButtonClearScreen.drawButton();
			drawText(0, 100, "Level complete", 2, COLOR_BLACK, COLOR_BACKGROUND_DEFAULT);
			TouchButtonNextLevel->drawButton();
			GameActive = false;
		} else {
			//
			// Ziel nicht berührt ->  lösche altes Bild male neuen Ball
			fillCircle(LastX, LastY, RADIUS_BALL, COLOR_BACKGROUND_DEFAULT);

			// Prüfe ob HomeKnopf berührt und gelöscht wird
			if (LastX + RADIUS_BALL >= TouchButtonMainHome->getPositionX()
					&& LastY + RADIUS_BALL >= TouchButtonMainHome->getPositionY()) {
				// male Knopf neu
				TouchButtonMainHome->drawButton();
			}
			// Prüfe ob ClearKnopf berührt wird
//			if (LastX - RADIUS_BALL <= TouchButtonClearScreen.getPositionXRight()
//					&& LastY + RADIUS_BALL >= TouchButtonClearScreen.getPositionY()) {
//				// male Knopf neu
//				TouchButtonClearScreen.drawButton();
//			}
			// Prüfe ob Ziel berührt wird
			if (LastX + RADIUS_BALL >= HOME_X && LastX - RADIUS_BALL <= (HOME_X + (FONT_WIDTH * HOME_SIZE))
					&& LastY + RADIUS_BALL >= HOME_Y && LastY - RADIUS_BALL <= (HOME_Y + (FONT_HEIGHT * HOME_SIZE))) {
				drawChar(HOME_X, HOME_Y, 0xD3, HOME_SIZE, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
			}
			fillCircle(tNewX, tNewY, RADIUS_BALL, COLOR_BALL);

			LastX = tNewX;
			LastY = tNewY;
		}
	}
}
#pragma GCC diagnostic pop

