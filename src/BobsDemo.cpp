/**
 * BobsDemo.cpp
 *
 * @ brief demo playground for accelerator etc.
 *
 * @date 16.12.2012
 * @author Robert
 */
#include "Pages.h"
#include "stm32f30x.h"

extern "C" {
#include "l3gdc20_lsm303dlhc_utils.h"
}

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
#define HOME_FONT_SIZE TEXT_SIZE_22
#define HOME_MID_X 220
#define HOME_MID_Y 50
#define Y_RESOLUTION 64
#define X_RESOLUTION 64

void doNextLevel(TouchButton * const aTheTouchedButton, int16_t aValue);
int sHomeMidX = HOME_MID_X;
int sHomeMidY = HOME_MID_Y;

void initBobsDemo(void) {

    initAcceleratorCompassChip();

    TouchButtonNextLevel = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
            BUTTON_HEIGHT_4, COLOR_CYAN, StringNext, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doNextLevel);
}

void startBobsDemo(void) {
    LastX = DISPLAY_WIDTH / 2;
    LastY = DISPLAY_HEIGHT / 2;
    AccumulatedValueX = 0;
    AccumulatedValueY = 0;
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    BlueDisplay1.fillCircle(LastX, LastY, RADIUS_BALL, COLOR_BALL);
    direction = INCREMENT;

    TouchButtonMainHome->drawButton();
    // male Home
    BlueDisplay1.drawChar(HOME_X - (TEXT_SIZE_22_WIDTH / 2), HOME_Y - (TEXT_SIZE_22_HEIGHT / 2) + TEXT_SIZE_22_ASCEND, 0xD3,
            HOME_FONT_SIZE, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
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
    // left
    BlueDisplay1.fillRectRel(sHomeMidX - 20, sHomeMidY - 20, 20 - (TEXT_SIZE_22_WIDTH / 2), 30, COLOR_BLUE);
    // right
    BlueDisplay1.fillRectRel(sHomeMidX + (TEXT_SIZE_22_WIDTH / 2), sHomeMidY - 20, 20 - (TEXT_SIZE_22_WIDTH / 2), 30, COLOR_BLUE);
    // top
    BlueDisplay1.fillRectRel(sHomeMidX - (TEXT_SIZE_22_WIDTH / 2), sHomeMidY - 40, TEXT_SIZE_22_WIDTH, 25, COLOR_BLUE);
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
        if (tNewX > (sHomeMidX - RADIUS_BALL) && tNewX < (sHomeMidX + RADIUS_BALL) && tNewY > (sHomeMidY - RADIUS_BALL)
                && tNewY < (sHomeMidY + RADIUS_BALL)) {
            // Ziel berührt
            tone(440, 200);
            delayMillis(200);
            tone(660, 600);
            BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
            TouchButtonMainHome->drawButton();
//			TouchButtonClearScreen.drawButton();
            BlueDisplay1.drawText(0, 100, "Level complete", TEXT_SIZE_22, COLOR_BLACK, COLOR_BACKGROUND_DEFAULT);
            TouchButtonNextLevel->drawButton();
            GameActive = false;
        } else {
            //
            // Ziel nicht berührt ->  lösche altes Bild male neuen Ball
            BlueDisplay1.fillCircle(LastX, LastY, RADIUS_BALL, COLOR_BACKGROUND_DEFAULT);

            // Prüfe ob HomeKnopf berührt und gelöscht wird
            if (LastX + RADIUS_BALL >= TouchButtonMainHome->getPositionX()
                    && LastY - RADIUS_BALL <= TouchButtonMainHome->getPositionYBottom()) {
                // male Knopf neu
                TouchButtonMainHome->drawButton();
            }
            // Prüfe ob ClearKnopf berührt wird
//			if (LastX - RADIUS_BALL <= TouchButtonClearScreen.getPositionXRight()
//					&& LastY + RADIUS_BALL >= TouchButtonClearScreen.getPositionY()) {
//				// male Knopf neu
//				TouchButtonClearScreen.drawButton();
//			}
            // Prüfe ob Ziel Icon berührt wird
            if (LastX + RADIUS_BALL >= sHomeMidX - (TEXT_SIZE_22_WIDTH / 2)
                    && LastX - RADIUS_BALL <= (sHomeMidX + (TEXT_SIZE_22_WIDTH / 2))
                    && LastY + RADIUS_BALL >= sHomeMidY - (TEXT_SIZE_22_HEIGHT / 2)
                    && LastY - RADIUS_BALL <= (sHomeMidY + (TEXT_SIZE_22_HEIGHT / 2))) {
                BlueDisplay1.drawChar(HOME_X - (TEXT_SIZE_22_WIDTH / 2), HOME_Y - (TEXT_SIZE_22_HEIGHT / 2) + TEXT_SIZE_22_ASCEND, 0xD3,
                        HOME_FONT_SIZE, COLOR_RED, COLOR_BACKGROUND_DEFAULT);            }
            BlueDisplay1.fillCircle(tNewX, tNewY, RADIUS_BALL, COLOR_BALL);

            LastX = tNewX;
            LastY = tNewY;
        }
    }
    checkAndHandleEvents();
}
#pragma GCC diagnostic pop

