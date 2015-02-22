/*
 * PageAcceleratorCompassGyroDemo.cpp
 * @brief shows graphic output of the accelerator, gyroscope and compass of the STM32F3-Discovery board
 *
 * @date 02.01.2013
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 *      Source based on STM code samples
 * @version 1.2.0
 */

#include "Pages.h"
#include "misc.h"

extern "C" {
#include "usb_misc.h"
#include "hw_config.h" // for USB
#include "usb_lib.h"
#include "usb_pwr.h"
#include "stm32f30x.h"
#include "stm32f3_discovery.h"
#include "l3gdc20_lsm303dlhc_utils.h"
#include "stm32f3_discovery_leds_buttons.h"
#include "math.h"
}

#define COLOR_ACC_GYRO_BACKGROUND COLOR_CYAN

#define COMPASS_RADIUS 40
#define COMPASS_MID_X (COMPASS_RADIUS + 10)
#define COMPASS_MID_Y (BlueDisplay1.getDisplayHeight() / 2)

#define GYRO_MID_X BUTTON_WIDTH_3_POS_3
#define GYRO_MID_Y (BUTTON_HEIGHT_4_LINE_4 - BUTTON_DEFAULT_SPACING)

#define VERTICAL_SLIDER_NULL_VALUE 90
#define HORIZONTAL_SLIDER_NULL_VALUE 160

#define TEXT_START_Y (10 + TEXT_SIZE_11_ASCEND) // Space for slider
#define TEXT_START_X 10 // Space for slider
static struct ThickLine AcceleratorLine;
static struct ThickLine CompassLine;
static struct ThickLine GyroYawLine;

TouchButtonAutorepeat * TouchButtonAutorepeatAccScalePlus;
TouchButtonAutorepeat * TouchButtonAutorepeatAccScaleMinus;

static TouchButton * TouchButtonClearScreen;

void doSetZero(TouchButton * const aTheTouchedButton, int16_t aValue);
static TouchButton * TouchButtonSetZero;

void doChangeAccScale(TouchButton * const aTheTouchedButton, int16_t aValue);
int sAcceleratorScale;

void doSetZero(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    int16_t tBufferRaw[3];
    // wait for end of touch vibration
    delayMillis(300);
    setZeroAcceleratorGyroValue();
    readGyroscopeRaw(tBufferRaw);
}

void drawAccDemoGui(void) {
    TouchButtonMainHome->drawButton();
    TouchButtonClearScreen->drawButton();
    TouchButtonAutorepeatAccScalePlus->drawButton();
    TouchButtonAutorepeatAccScaleMinus->drawButton();
    BlueDisplay1.drawCircle(COMPASS_MID_X, COMPASS_MID_Y, COMPASS_RADIUS, COLOR_BLACK, 1);

    TouchButtonSetZero->drawButton();

    /*
     * Vertical slider
     */
    TouchSliderVertical.initSlider(1, 10, 2, 2 * VERTICAL_SLIDER_NULL_VALUE, VERTICAL_SLIDER_NULL_VALUE, VERTICAL_SLIDER_NULL_VALUE,
            NULL, TOUCHSLIDER_DEFAULT_TOUCH_BORDER, TOUCHSLIDER_SHOW_VALUE, NULL, NULL);
    TouchSliderVertical.setBarColor(COLOR_RED);
    TouchSliderVertical.setBarThresholdColor(COLOR_GREEN);

//	TouchSliderVertical2.initSlider(50, 40, 2, 2 * VERTICAL_SLIDER_NULL_VALUE, VERTICAL_SLIDER_NULL_VALUE,
//			VERTICAL_SLIDER_NULL_VALUE, NULL, TOUCHSLIDER_DEFAULT_TOUCH_BORDER, TOUCHSLIDER_SHOW_VALUE, NULL, NULL);
//	TouchSliderVertical2.setBarColor(COLOR_RED);
//	TouchSliderVertical2.setBarThresholdColor(COLOR_GREEN);

// Horizontal slider
    TouchSliderHorizontal.initSlider(0, 1, 2, 2 * HORIZONTAL_SLIDER_NULL_VALUE, HORIZONTAL_SLIDER_NULL_VALUE,
            HORIZONTAL_SLIDER_NULL_VALUE, NULL, TOUCHSLIDER_DEFAULT_TOUCH_BORDER,
            TOUCHSLIDER_SHOW_VALUE | TOUCHSLIDER_IS_HORIZONTAL, NULL, NULL);
    TouchSliderHorizontal.setBarColor(COLOR_RED);
    TouchSliderHorizontal.setBarThresholdColor(COLOR_GREEN);

    TouchSliderVertical.drawBar();
    TouchSliderHorizontal.drawBar();

}

/**
 * init hardware and thick lines
 */
void initAcceleratorCompass(void) {

    AcceleratorLine.StartX = BlueDisplay1.getDisplayWidth() / 2;
    AcceleratorLine.StartY = BlueDisplay1.getDisplayHeight() / 2;
    AcceleratorLine.EndX = BlueDisplay1.getDisplayWidth() / 2;
    AcceleratorLine.EndY = BlueDisplay1.getDisplayHeight() / 2;
    AcceleratorLine.Thickness = 2;
    AcceleratorLine.ThicknessMode = LINE_THICKNESS_DRAW_CLOCKWISE;
    AcceleratorLine.Color = COLOR_RED;
    AcceleratorLine.BackgroundColor = COLOR_ACC_GYRO_BACKGROUND;

    CompassLine.StartX = COMPASS_MID_X;
    CompassLine.StartY = COMPASS_MID_Y;
    CompassLine.EndX = COMPASS_MID_X;
    CompassLine.EndY = COMPASS_MID_Y;
    CompassLine.Thickness = 5;
    CompassLine.ThicknessMode = LINE_THICKNESS_MIDDLE;
    CompassLine.Color = COLOR_BLACK;
    CompassLine.BackgroundColor = COLOR_ACC_GYRO_BACKGROUND;

    GyroYawLine.StartX = GYRO_MID_X;
    GyroYawLine.StartY = GYRO_MID_Y;
    GyroYawLine.EndX = GYRO_MID_X;
    GyroYawLine.EndY = GYRO_MID_Y;
    GyroYawLine.Thickness = 3;
    GyroYawLine.ThicknessMode = LINE_THICKNESS_MIDDLE;
    GyroYawLine.Color = COLOR_BLUE;
    GyroYawLine.BackgroundColor = COLOR_ACC_GYRO_BACKGROUND;

}

void doClearAcceleratorCompassScreen(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    BlueDisplay1.clearDisplay(aValue);
    drawAccDemoGui();
}

void startAcceleratorCompass(void) {
    USB_ChangeToJoystick();
    BlueDisplay1.clearDisplay(COLOR_ACC_GYRO_BACKGROUND);

    // 4. row
    TouchButtonAutorepeatAccScalePlus = TouchButtonAutorepeat::allocAndInitSimpleButton(BUTTON_WIDTH_6_POS_2,
            BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_6, BUTTON_HEIGHT_4, COLOR_RED, StringPlus, TEXT_SIZE_22, 1, &doChangeAccScale);
    TouchButtonAutorepeatAccScalePlus->setButtonAutorepeatTiming(600, 100, 10, 20);

    TouchButtonAutorepeatAccScaleMinus = TouchButtonAutorepeat::allocAndInitSimpleButton(0, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_6,
            BUTTON_HEIGHT_4, COLOR_RED, StringMinus, TEXT_SIZE_22, -1, &doChangeAccScale);
    TouchButtonAutorepeatAccScaleMinus->setButtonAutorepeatTiming(600, 100, 10, 20);

    TouchButtonSetZero = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
            BUTTON_HEIGHT_4, COLOR_RED, StringZero, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doSetZero);

    // Clear button - lower left corner
    TouchButtonClearScreen = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
            BUTTON_HEIGHT_4, COLOR_RED, StringClear, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, COLOR_BACKGROUND_DEFAULT,
            &doClearAcceleratorCompassScreen);

    drawAccDemoGui();
    registerSimpleResizeAndReconnectCallback(&drawAccDemoGui);

    sAcceleratorScale = 9;
    MillisSinceLastAction = 0;

    SPI1_setPrescaler(SPI_BaudRatePrescaler_8 );
}

void sendCursorMovementOverUSB() {
    uint8_t HID_Buffer[4] = { 0 };
    /**
     * Send over USB
     */
    if (isUSBReady()) {
        HID_Buffer[1] = 0;
        HID_Buffer[2] = 0;
        /* LEFT Direction */
        if ((AcceleratorGyroscopeCompassRawDataBuffer[0] / sAcceleratorScale) < -2) {
            HID_Buffer[1] -= CURSOR_STEP;
        }
        /* RIGHT Direction */
        if ((AcceleratorGyroscopeCompassRawDataBuffer[0] / sAcceleratorScale) > 2) {
            HID_Buffer[1] += CURSOR_STEP;
        }
        /* UP Direction */
        if ((AcceleratorGyroscopeCompassRawDataBuffer[1] / sAcceleratorScale) < -2) {
            HID_Buffer[2] += CURSOR_STEP;
        }
        /* DOWN Direction */
        if ((AcceleratorGyroscopeCompassRawDataBuffer[1] / sAcceleratorScale) > 2) {
            HID_Buffer[2] -= CURSOR_STEP;
        }
        /* Update the cursor position */
        if ((HID_Buffer[1] != 0) || (HID_Buffer[2] != 0)) {
            /* Reset the control token to inform upper layer that a transfer is ongoing */
            USB_PacketSent = 0;

            /* Copy mouse position info in ENDP1 Tx Packet Memory Area*/
            USB_SIL_Write(EP1_IN, HID_Buffer, 4);

            /* Enable endpoint for transmission */
            SetEPTxValid(ENDP1 );
        }
    }
}

void loopAcceleratorGyroCompassDemo(void) {

    /* Wait 50 ms for data ready */
    uint32_t tMillis = getMillisSinceBoot();
    MillisSinceLastAction += tMillis - MillisLastLoop;
    MillisLastLoop = tMillis;
    if (MillisSinceLastAction > 50) {
        MillisSinceLastAction = 0;
        /**
         * Accelerator data
         */
        readAcceleratorZeroCompensated(&AcceleratorGyroscopeCompassRawDataBuffer[0]);
        snprintf(StringBuffer, sizeof StringBuffer, "Accelerator X=%5d Y=%5d Z=%5d", AcceleratorGyroscopeCompassRawDataBuffer[0],
                AcceleratorGyroscopeCompassRawDataBuffer[1], AcceleratorGyroscopeCompassRawDataBuffer[2]);
        BlueDisplay1.drawText(TEXT_START_X, TEXT_START_Y, StringBuffer, TEXT_SIZE_11, COLOR_BLACK, COLOR_GREEN);
        BlueDisplay1.refreshVector(&AcceleratorLine, (AcceleratorGyroscopeCompassRawDataBuffer[0] / sAcceleratorScale),
                -(AcceleratorGyroscopeCompassRawDataBuffer[1] / sAcceleratorScale));
        BlueDisplay1.drawPixel(AcceleratorLine.StartX, AcceleratorLine.StartY, COLOR_BLACK);

        /**
         * Send over USB
         */
        sendCursorMovementOverUSB();
        /**
         * Compass data
         */
        readCompassRaw(AcceleratorGyroscopeCompassRawDataBuffer);
        snprintf(StringBuffer, sizeof StringBuffer, "Compass X=%5d Y=%5d Z=%5d", AcceleratorGyroscopeCompassRawDataBuffer[0],
                AcceleratorGyroscopeCompassRawDataBuffer[1], AcceleratorGyroscopeCompassRawDataBuffer[2]);
        BlueDisplay1.drawText(TEXT_START_X, TEXT_SIZE_11_HEIGHT + TEXT_START_Y, StringBuffer, TEXT_SIZE_11, COLOR_BLACK,
                COLOR_GREEN);
        // values can reach 800
        BlueDisplay1.refreshVector(&CompassLine, (AcceleratorGyroscopeCompassRawDataBuffer[0] >> 5),
                -(AcceleratorGyroscopeCompassRawDataBuffer[2] >> 5));
        BlueDisplay1.drawPixel(CompassLine.StartX, CompassLine.StartY, COLOR_RED);

        /**
         * Gyroscope data
         */
        readGyroscopeZeroCompensated(AcceleratorGyroscopeCompassRawDataBuffer);
        snprintf(StringBuffer, sizeof StringBuffer, "Gyroscope R=%5d P=%5d Y=%5d", AcceleratorGyroscopeCompassRawDataBuffer[0],
                AcceleratorGyroscopeCompassRawDataBuffer[1], AcceleratorGyroscopeCompassRawDataBuffer[2]);
        BlueDisplay1.drawText(TEXT_START_X, 2 * TEXT_SIZE_11_HEIGHT + TEXT_START_Y, StringBuffer, TEXT_SIZE_11, COLOR_BLACK,
                COLOR_GREEN);

        TouchSliderHorizontal.setActualValueAndDrawBar(
                (AcceleratorGyroscopeCompassRawDataBuffer[0] / 4) + HORIZONTAL_SLIDER_NULL_VALUE);
        TouchSliderVertical.setActualValueAndDrawBar(
                (AcceleratorGyroscopeCompassRawDataBuffer[1] / 4) + VERTICAL_SLIDER_NULL_VALUE);

        BlueDisplay1.refreshVector(&GyroYawLine, -AcceleratorGyroscopeCompassRawDataBuffer[2] / 8, -(2 * COMPASS_RADIUS));
        BlueDisplay1.drawPixel(GyroYawLine.StartX, GyroYawLine.StartY, COLOR_RED);
    }
    checkAndHandleEvents();
}

void stopAcceleratorCompass(void) {
    USB_PowerOff();
    TouchButtonSetZero->setFree();
    TouchButtonClearScreen->setFree();
    TouchButtonAutorepeatAccScalePlus->setFree();
    TouchButtonAutorepeatAccScaleMinus->setFree();
}

void doChangeAccScale(TouchButton * const aTheTouchedButton, int16_t aValue) {
    int tFeedbackType = FEEDBACK_TONE_NO_ERROR;
    sAcceleratorScale += aValue;
    if (sAcceleratorScale < 1) {
        sAcceleratorScale = 1;
        tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
    }
    snprintf(StringBuffer, sizeof StringBuffer, "Scale=%2u", sAcceleratorScale);
    BlueDisplay1.drawText(10, BUTTON_HEIGHT_4_LINE_4 - TEXT_SIZE_11_DECEND - 4, StringBuffer, TEXT_SIZE_11, COLOR_BLUE,
            COLOR_ACC_GYRO_BACKGROUND);
    FeedbackTone(tFeedbackType);
}

void DemoPitchRoll(void) {
    uint8_t i = 0;
    float AccBuffer[3] = { 0.0f };
    float fNormAcc, fSinRoll, fCosRoll, fSinPitch, fCosPitch = 0.0f, RollAng = 0.0f, PitchAng = 0.0f;

    /* Wait for data ready */
    delayMillis(50);

    /* Read Accelerometer data */
    Demo_CompassReadAcc(AccBuffer);

    for (i = 0; i < 3; i++)
        AccBuffer[i] /= 100.0f;

    fNormAcc = sqrt((AccBuffer[0] * AccBuffer[0]) + (AccBuffer[1] * AccBuffer[1]) + (AccBuffer[2] * AccBuffer[2]));

    fSinRoll = -AccBuffer[1] / fNormAcc;
    fCosRoll = sqrt(1.0 - (fSinRoll * fSinRoll));
    fSinPitch = AccBuffer[0] / fNormAcc;
    fCosPitch = sqrt(1.0 - (fSinPitch * fSinPitch));
    if (fSinRoll > 0) {
        if (fCosRoll > 0) {
            RollAng = acos(fCosRoll) * 180 / PI;
        } else {
            RollAng = acos(fCosRoll) * 180 / PI + 180;
        }
    } else {
        if (fCosRoll > 0) {
            RollAng = acos(fCosRoll) * 180 / PI + 360;
        } else {
            RollAng = acos(fCosRoll) * 180 / PI + 180;
        }
    }

    if (fSinPitch > 0) {
        if (fCosPitch > 0) {
            PitchAng = acos(fCosPitch) * 180 / PI;
        } else {
            PitchAng = acos(fCosPitch) * 180 / PI + 180;
        }
    } else {
        if (fCosPitch > 0) {
            PitchAng = acos(fCosPitch) * 180 / PI + 360;
        } else {
            PitchAng = acos(fCosPitch) * 180 / PI + 180;
        }
    }

    if (RollAng >= 360) {
        RollAng = RollAng - 360;
    }

    if (PitchAng >= 360) {
        PitchAng = PitchAng - 360;
    }

    allLedsOff();
    if (fSinRoll > 0.2f) {
        STM_EVAL_LEDOn(LED7);
    } else if (fSinRoll < -0.2f) {
        STM_EVAL_LEDOn(LED6);
    }
    if (fSinPitch > 0.2f) {
        STM_EVAL_LEDOn(LED3);
    } else if (fSinPitch < -0.2f) {
        STM_EVAL_LEDOn(LED10);
    }
    checkAndHandleEvents();
}

