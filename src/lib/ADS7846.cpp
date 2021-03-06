/*
 * ADS7846.cpp
 *
 * @date 05.12.2012
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 *
 *      based on ADS7846.cpp with license
 *      https://github.com/watterott/mSD-Shield/blob/master/src/license.txt
 */

#include "ADS7846.h"
#include "BlueDisplay.h"

#include "misc.h"
#include <stdlib.h> // for abs()
extern "C" {
#include "timing.h"
#include "math.h"
#include "stm32f30xPeripherals.h"
#include "stm32f3_discovery.h"  // For LEDx}
#define TOUCH_DELAY_AFTER_READ_MILLIS 3
#define TOUCH_DEBOUNCE_DELAY_MILLIS 10 // wait for debouncing in ISR - minimum 8 ms
#define TOUCH_SWIPE_RESOLUTION_MILLIS 20

#define CMD_START       (0x80)
#define CMD_12BIT       (0x00)
#define CMD_8BIT        (0x08)
#define CMD_DIFF        (0x00)
#define CMD_SINGLE      (0x04)

#define CHANNEL_MASK 0x70
// Power modes
#define CMD_PWD         (0x00)
#define ADC_ON    		(0x01)
#define REF_ON    		(0x02)
#define CMD_ALWAYSON    (0x03)

// Set 2,5V reference on. Only useful when using readChannel(), otherwise take CMD_PWD!
#define POWER_MODE CMD_ALWAYSON

const char PosZ1[] = "Z Pos 1";
const char PosZ2[] = "Z Pos 2";
const char PosX[] = "X Pos";
const char PosY[] = "Y Pos";
const char Temperature0[] = "Temp. 0";
const char Temperature1[] = "Temp. 1";
const char Vcc[] = "VCC";
const char Aux[] = "Aux In";
const char * const ADS7846ChannelStrings[] = { PosZ1, PosZ2, PosX, PosY, Temperature0, Temperature1, Vcc, Aux };
const char ADS7846ChannelChars[] = { 'z', 'Z', 'X', 'Y', 't', 'T', 'V', 'A' };

// Channel number to text mapping
unsigned char ADS7846ChannelMapping[] = { 3, 4, 1, 5, 0, 7, 2, 6 };

// to start quick if backup battery was not assembled or empty
const CAL_MATRIX sInitalMatrix = { 320300, -1400, -52443300, -3500, 237700, -21783300, 1857905 };

void callbackPeriodicTouch(void);

//-------------------- Constructor --------------------

ADS7846::ADS7846(void) {
}

// One instance of ADS7846 called TouchPanel
ADS7846 TouchPanel;

//-------------------- Public --------------------

void ADS7846::init(void) {
    //init pins
    ADS7846_IO_initalize();

    //set vars
    tp_matrix.div = 0;
    mTouchActualPositionRaw.PosX = 0;
    mTouchActualPositionRaw.PosY = 0;
    mTouchLastCalibratedPositionRaw.PosX = 0;
    mTouchLastCalibratedPositionRaw.PosY = 0;
    mTouchActualPosition.PosX = 0; // set by calibrate called by rd_data
    mTouchActualPosition.PosY = 0;
    mPressure = 0;
    ADS7846TouchActive = false;
    ADS7846TouchStart = false;
    localTouchEvent.EventType = EVENT_TAG_NO_EVENT;
}

bool ADS7846::setCalibration(CAL_POINT *aTargetValues, CAL_POINT *aRawValues) {
    tp_matrix.div = ((aRawValues[0].x - aRawValues[2].x) * (aRawValues[1].y - aRawValues[2].y))
            - ((aRawValues[1].x - aRawValues[2].x) * (aRawValues[0].y - aRawValues[2].y));

    if (tp_matrix.div == 0) {
        return false;
    }

    tp_matrix.a = ((aTargetValues[0].x - aTargetValues[2].x) * (aRawValues[1].y - aRawValues[2].y))
            - ((aTargetValues[1].x - aTargetValues[2].x) * (aRawValues[0].y - aRawValues[2].y));

    tp_matrix.b = ((aRawValues[0].x - aRawValues[2].x) * (aTargetValues[1].x - aTargetValues[2].x))
            - ((aTargetValues[0].x - aTargetValues[2].x) * (aRawValues[1].x - aRawValues[2].x));

    tp_matrix.c = (aRawValues[2].x * aTargetValues[1].x - aRawValues[1].x * aTargetValues[2].x) * aRawValues[0].y
            + (aRawValues[0].x * aTargetValues[2].x - aRawValues[2].x * aTargetValues[0].x) * aRawValues[1].y
            + (aRawValues[1].x * aTargetValues[0].x - aRawValues[0].x * aTargetValues[1].x) * aRawValues[2].y;

    tp_matrix.d = ((aTargetValues[0].y - aTargetValues[2].y) * (aRawValues[1].y - aRawValues[2].y))
            - ((aTargetValues[1].y - aTargetValues[2].y) * (aRawValues[0].y - aRawValues[2].y));

    tp_matrix.e = ((aRawValues[0].x - aRawValues[2].x) * (aTargetValues[1].y - aTargetValues[2].y))
            - ((aTargetValues[0].y - aTargetValues[2].y) * (aRawValues[1].x - aRawValues[2].x));

    tp_matrix.f = (aRawValues[2].x * aTargetValues[1].y - aRawValues[1].x * aTargetValues[2].y) * aRawValues[0].y
            + (aRawValues[0].x * aTargetValues[2].y - aRawValues[2].x * aTargetValues[0].y) * aRawValues[1].y
            + (aRawValues[1].x * aTargetValues[0].y - aRawValues[0].x * aTargetValues[1].y) * aRawValues[2].y;

    return true;
}

void ADS7846::writeCalibration(CAL_MATRIX aMatrix) {
    PWR_BackupAccessCmd(ENABLE);
    // Write data
    RTC_WriteBackupRegister(RTC_BKP_DR1, aMatrix.a);
    RTC_WriteBackupRegister(RTC_BKP_DR2, aMatrix.b);
    RTC_WriteBackupRegister(RTC_BKP_DR3, aMatrix.c);
    RTC_WriteBackupRegister(RTC_BKP_DR4, aMatrix.d);
    RTC_WriteBackupRegister(RTC_BKP_DR5, aMatrix.e);
    RTC_WriteBackupRegister(RTC_BKP_DR6, aMatrix.f);
    RTC_WriteBackupRegister(RTC_BKP_DR7, aMatrix.div);
    PWR_BackupAccessCmd(DISABLE);
}

/**
 *
 * @return true if calibration data valid and matrix is set
 */
void ADS7846::readCalibration(CAL_MATRIX *aMatrix) {
    aMatrix->a = RTC_ReadBackupRegister(RTC_BKP_DR1 );
    aMatrix->b = RTC_ReadBackupRegister(RTC_BKP_DR2 );
    aMatrix->c = RTC_ReadBackupRegister(RTC_BKP_DR3 );
    aMatrix->d = RTC_ReadBackupRegister(RTC_BKP_DR4 );
    aMatrix->e = RTC_ReadBackupRegister(RTC_BKP_DR5 );
    aMatrix->f = RTC_ReadBackupRegister(RTC_BKP_DR6 );
    aMatrix->div = RTC_ReadBackupRegister(RTC_BKP_DR7 );
}

/**
 * touch panel calibration routine
 */
void ADS7846::doCalibration(bool aCheckRTC) {
    CAL_POINT tReferencePoints[3] = { CAL_POINT1, CAL_POINT2, CAL_POINT3 }; //calibration point positions
    CAL_POINT tRawPoints[3];

    if (aCheckRTC) {
        //calibration data in CMOS RAM?
        if (RTC_checkMagicNumber()) {
            readCalibration(&tp_matrix);
        } else {
            // not valid -> copy initial values to data structure
            writeCalibration(sInitalMatrix);
            readCalibration(&tp_matrix);
        }
        return;
    }

    //show calibration points
    for (uint8_t i = 0; i < 3;) {
        //clear screen and wait for touch release
        LocalDisplay.clearDisplay(COLOR_WHITE);
        LocalDisplay.drawText((DISPLAY_WIDTH / 2) - 50, (DISPLAY_HEIGHT / 2) - 10, (char*) StringCalibration, 1, COLOR_BLACK,
                COLOR_WHITE);

        //draw points
        LocalDisplay.drawCircle(tReferencePoints[i].x, tReferencePoints[i].y, 2, COLOR_BLACK);
        LocalDisplay.drawCircle(tReferencePoints[i].x, tReferencePoints[i].y, 5, COLOR_BLACK);
        LocalDisplay.drawCircle(tReferencePoints[i].x, tReferencePoints[i].y, 10, COLOR_RED);

        // wait for touch to become active
        while (!TouchPanel.wasTouched()) {
            delayMillis(5);
        }
        // wait for stabilizing data
        delayMillis(10);
        //get new data
        rd_data(4 * ADS7846_READ_OVERSAMPLING_DEFAULT);
        //press detected -> save point
        LocalDisplay.fillCircle(tReferencePoints[i].x, tReferencePoints[i].y, 2, COLOR_RED);
        tRawPoints[i].x = getXRaw();
        tRawPoints[i].y = getYRaw();
        // reset touched flag
        TouchPanel.wasTouched();
        i++;
    }

    //Calculate calibration matrix
    setCalibration(tReferencePoints, tRawPoints);
    if (tp_matrix.div != 0) {
        //save calibration matrix and flag for valid data
        RTC_setMagicNumber();
        writeCalibration(tp_matrix);
    }
    LocalDisplay.clearDisplay(COLOR_WHITE);

    return;
}

/**
 * convert raw to calibrated position in mTouchActualPosition
 * mTouchActualPositionRaw -> mTouchActualPosition
 */
void ADS7846::calibrate(void) {
    long x, y;

    //calc x pos
    if (mTouchActualPositionRaw.PosX != mTouchLastCalibratedPositionRaw.PosX) {
        mTouchLastCalibratedPositionRaw.PosX = mTouchActualPositionRaw.PosX;
        x = mTouchActualPositionRaw.PosX;
        y = mTouchActualPositionRaw.PosY;
        x = ((tp_matrix.a * x) + (tp_matrix.b * y) + tp_matrix.c) / tp_matrix.div;
        if (x < 0) {
            x = 0;
        } else if (x >= DISPLAY_WIDTH) {
            x = DISPLAY_WIDTH - 1;
        }
        mTouchActualPosition.PosX = x;
    }

    //calc y pos
    if (mTouchActualPositionRaw.PosY != mTouchLastCalibratedPositionRaw.PosY) {
        mTouchLastCalibratedPositionRaw.PosY = mTouchActualPositionRaw.PosY;
        x = mTouchActualPositionRaw.PosX;
        y = mTouchActualPositionRaw.PosY;
        y = ((tp_matrix.d * x) + (tp_matrix.e * y) + tp_matrix.f) / tp_matrix.div;
        if (y < 0) {
            y = 0;
        } else if (y >= DISPLAY_HEIGHT) {
            y = DISPLAY_HEIGHT - 1;
        }
        mTouchActualPosition.PosY = y;
    }

    return;
}

int ADS7846::getXRaw(void) {
    return mTouchActualPositionRaw.PosX;
}

int ADS7846::getYRaw(void) {
    return mTouchActualPositionRaw.PosY;
}

int ADS7846::getXActual(void) {
    return mTouchActualPosition.PosX;
}

int ADS7846::getYActual(void) {
    return mTouchActualPosition.PosY;
}

int ADS7846::getPressure(void) {
    return mPressure;
}

//
/**
 * read individual A/D channels like temperature or Vcc
 * @param channel
 * @param use12Bit
 * @param useDiffMode
 * @param numberOfReadingsToIntegrate
 * @return
 * @note
 */
uint16_t ADS7846::readChannel(uint8_t channel, bool use12Bit, bool useDiffMode, int numberOfReadingsToIntegrate) {
    channel <<= 4;
    // mask for channel 0 to 7
    channel &= CHANNEL_MASK;
    uint16_t tRetValue = 0;
    uint8_t low, high, i;

    //SPI speed-down
    uint16_t tPrescaler = SPI1_getPrescaler();
    SPI1_setPrescaler(SPI_BaudRatePrescaler_64 );

    // disable interrupts for some ms in order to wait for the interrupt line to go high
    ADS7846_disableInterrupt(); // only needed for X, Y + Z channel

    //read channel
    ADS7846_CSEnable();
    uint8_t tMode = CMD_SINGLE;
    if (useDiffMode) {
        tMode = CMD_DIFF;
    }
    for (i = numberOfReadingsToIntegrate; i != 0; i--) {
        if (use12Bit) {
            SPI1_sendReceiveFast(CMD_START | CMD_12BIT | tMode | channel);
            high = SPI1_sendReceiveFast(0);
            low = SPI1_sendReceiveFast(0);
            tRetValue += (high << 5) | (low >> 3);
        } else {
            SPI1_sendReceiveFast(CMD_START | CMD_8BIT | tMode | channel);
            tRetValue += SPI1_sendReceiveFast(0);
        }
    }
    ADS7846_CSDisable();
    // enable interrupts after some ms in order to wait for the interrupt line to go high - minimum 3 ms
    changeDelayCallback(&ADS7846_clearAndEnableInterrupt, TOUCH_DELAY_AFTER_READ_MILLIS);

    //restore SPI settings
    SPI1_setPrescaler(tPrescaler);

    return tRetValue / numberOfReadingsToIntegrate;
}

void ADS7846::rd_data(void) {
    rd_data(ADS7846_READ_OVERSAMPLING_DEFAULT);
}

/**
 * 3.3 ms at SPI_BaudRatePrescaler_256 and 16 times oversampling
 * 420 us at SPI_BaudRatePrescaler_64 and x4/y8 times oversampling
 * @param aOversampling for x, y is twofold oversampled
 */
void ADS7846::rd_data(int aOversampling) {
    uint8_t a, b;
    int i;
    uint32_t tXValue, tYValue;

//	Set_DebugPin();
    /*
     * SPI speed-down
     * datasheet says: optimal is CLK < 125kHz (40-80 kHz)
     * slowest frequency for SPI1 is 72 MHz / 256 = 280 kHz
     * but results for divider 64 looks as good as for 256
     * results for divider 16 are offsetted and not usable
     */
    uint16_t tPrescaler = SPI1_getPrescaler();
    SPI1_setPrescaler(SPI_BaudRatePrescaler_64 ); // 72 MHz / 256 = 280 kHz, /64 = 1,1Mhz

    //Disable interrupt because while ADS7846 is read int line goes low (active)
    ADS7846_disableInterrupt();

    //get pressure
    ADS7846_CSEnable();
    SPI1_sendReceiveFast(CMD_START | CMD_8BIT | CMD_DIFF | CMD_Z1_POS);
    a = SPI1_sendReceiveFast(0);
    SPI1_sendReceiveFast(CMD_START | CMD_8BIT | CMD_DIFF | CMD_Z2_POS);
    b = SPI1_sendReceiveFast(0);
    b = 127 - b; // 127 is maximum reading of CMD_Z2_POS!
    int tPressure = a + b;

    mPressure = 0;
    ADS7846TouchActive = false;

    if (tPressure >= MIN_REASONABLE_PRESSURE) {
        // n times oversampling
        int j = 0;
        for (tXValue = 0, tYValue = 0, i = aOversampling; i != 0; i--) {
            //get X data
            SPI1_sendReceiveFast(CMD_START | CMD_12BIT | CMD_DIFF | CMD_X_POS);
            a = SPI1_sendReceiveFast(0);
            b = SPI1_sendReceiveFast(0);
            uint32_t tX = (a << 5) | (b >> 3); //12bit: ((a<<5)|(b>>3)) //10bit: ((a<<3)|(b>>5))
            if (tX >= 4000) {
                // no reasonable value
                break;
            }
            tXValue += 4048 - tX;

            //get Y data twice, because it is noisier than Y
            SPI1_sendReceiveFast(CMD_START | CMD_12BIT | CMD_DIFF | CMD_Y_POS);
            a = SPI1_sendReceiveFast(0);
            b = SPI1_sendReceiveFast(0);
            uint32_t tY = (a << 5) | (b >> 3); //12bit: ((a<<5)|(b>>3)) //10bit: ((a<<3)|(b>>5))
            if (tY <= 100) {
                // no reasonable value
                break;
            }
            tYValue += tY;

            SPI1_sendReceiveFast(CMD_START | CMD_12BIT | CMD_DIFF | CMD_Y_POS);
            a = SPI1_sendReceiveFast(0);
            b = SPI1_sendReceiveFast(0);
            tY = (a << 5) | (b >> 3); //12bit: ((a<<5)|(b>>3)) //10bit: ((a<<3)|(b>>5))
            tYValue += tY;

            j += 2; // +2 to get 11 bit values at the end
        }
        if (j == (aOversampling * 2)) {
            // scale down to 11 bit because calibration does not work with 12 bit values
            tXValue /= j;
            tYValue /= 2 * j;

            // plausi check - is pressure still greater than 7/8 start pressure
            SPI1_sendReceiveFast(CMD_START | CMD_8BIT | CMD_DIFF | CMD_Z1_POS);
            a = SPI1_sendReceiveFast(0);
            SPI1_sendReceiveFast(CMD_START | CMD_8BIT | CMD_DIFF | CMD_Z2_POS);
            b = SPI1_sendReceiveFast(0);
            b = 127 - b; // 127 is maximum reading of CMD_Z2_POS!

            // plausi check - x raw value is from 130 to 3900 here x is (4048 - x)/2, y raw is from 150 to 3900 - low is upper right corner
            if (((a + b) > (tPressure - (tPressure >> 3))) && (tXValue >= 10) && (tYValue >= 10)) {
                mTouchActualPositionRaw.PosX = tXValue;
                mTouchActualPositionRaw.PosY = tYValue;
                calibrate();
                mPressure = tPressure;
                ADS7846TouchActive = true;
            }
        }
    }
    ADS7846_CSDisable();
    //restore SPI settings
    SPI1_setPrescaler(tPrescaler);
    // enable interrupts after some ms in order to wait for the interrupt line to go high  - minimum 3 ms (2ms give errors)
    changeDelayCallback(&ADS7846_clearAndEnableInterrupt, TOUCH_DELAY_AFTER_READ_MILLIS);
    return;
}

/**
 * Callback routine for SysTick handler
 */
void callbackADS7846MoveRecognition(void) {
    int tLevel = ADS7846_getInteruptLineLevel();
    if (!tLevel) {
        // pressed - line input is low, touch still active
        TouchPanel.rd_data(ADS7846_READ_OVERSAMPLING_DEFAULT);
        localTouchEvent.EventData.TouchPosition = TouchPanel.mTouchActualPosition;
        if (!TouchPanel.ADS7846TouchActive) {
            // went inactive just while rd_data()
            localTouchEvent.EventType = EVENT_TAG_TOUCH_ACTION_UP;
        } else {
            if (TouchPanel.mTouchLastPosition.PosX != TouchPanel.mTouchActualPosition.PosX
                    || TouchPanel.mTouchLastPosition.PosY != TouchPanel.mTouchActualPosition.PosY) {
                TouchPanel.mTouchLastPosition = TouchPanel.mTouchActualPosition;

                // avoid overwriting other (e.g long touch down) events
                if (localTouchEvent.EventType == EVENT_TAG_NO_EVENT) {
                    localTouchEvent.EventType = EVENT_TAG_TOUCH_ACTION_MOVE;
                }
            }
            // restart timer
            changeDelayCallback(&callbackADS7846MoveRecognition, TOUCH_SWIPE_RESOLUTION_MILLIS);
        }
    } else {
        // line level switched to inactive but callback was not disabled
        if (TouchPanel.ADS7846TouchActive) {
            TouchPanel.ADS7846TouchActive = false;
            localTouchEvent.EventData.TouchPosition = TouchPanel.mTouchLastPosition;
            localTouchEvent.EventType = EVENT_TAG_TOUCH_ACTION_UP;
        }
    }
}

/**
 * This handler is called on both edge of touch interrupt signal
 * actually the ADS7846 IRQ signal bounces on rising edge
 * This can happen up to 8 milliseconds after initial transition
 */
extern "C" void EXTI1_IRQHandler(void) {
    // wait for stable reading
    delayMillis(TOUCH_DEBOUNCE_DELAY_MILLIS);
    bool tLevel = ADS7846_getInteruptLineLevel();
    STM_EVAL_LEDOff(LED7); // GREEN RIGHT
    if (!tLevel) {
        /*
         * touchDown event - line input is low
         */
        TouchPanel.rd_data(ADS7846_READ_OVERSAMPLING_DEFAULT); // this disables interrupt for additional TOUCH_DELAY_AFTER_READ_MILLIS
        localTouchEvent.EventData.TouchPosition = TouchPanel.mTouchActualPosition;
        TouchPanel.mTouchLastPosition = TouchPanel.mTouchActualPosition; // for move detection
        localTouchEvent.EventType = EVENT_TAG_TOUCH_ACTION_DOWN;
        if (TouchPanel.ADS7846TouchActive) {
            TouchPanel.ADS7846TouchStart = true;
            /*
             * Enable move recognition by periodically reading actual position.
             */
            changeDelayCallback(&callbackADS7846MoveRecognition, TOUCH_SWIPE_RESOLUTION_MILLIS);
        } else {
            // line was active but values could not be read correctly (e.g. because of delay by higher prio interrupts)
            STM_EVAL_LEDToggle(LED4); // BLUE Back
        }
    } else {
        // touch released here
        changeDelayCallback(&callbackPeriodicTouch, DISABLE_TIMER_DELAY_VALUE); // disable periodic interrupts which can call handleTouchRelease
        if (TouchPanel.ADS7846TouchActive) {
            TouchPanel.ADS7846TouchActive = false;
            localTouchEvent.EventData.TouchPosition = TouchPanel.mTouchLastPosition;
            localTouchEvent.EventType = EVENT_TAG_TOUCH_ACTION_UP;
        }
    }
    resetBacklightTimeout();
    // Clear the EXTI line pending bit and enable new interrupt on other edge
    ADS7846_ClearITPendingBit();
}

/**
 * is called by main loops
 * returns only one times a "true" per touch
 */ //
bool ADS7846::wasTouched(void) {
    if (ADS7846TouchStart) {
        // reset => return only one true per touch
        ADS7846TouchStart = false;
        return true;
    }
    return false;
}

