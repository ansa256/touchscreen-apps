/*
 * PageTests.cpp
 *
 * @date 16.03.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#include "MI0283QT2.h"
#include "Pages.h"
#include "TouchDSO.h"
#include "Chart.h"

#include "myprint.h"
#include "misc.h"
#include "main.h"
#include <string.h>
#include <locale.h>
#include <stdlib.h> // for srand
extern "C" {
#include "stm32f3_discovery.h"
#include "l3gdc20_lsm303dlhc_utils.h"
#include "diskio.h"
#include "ff.h"
#include "USART_DMA.h"

}

/* Private variables ---------------------------------------------------------*/
static TouchButton * TouchButtonBack;
static bool sBackButtonPressed;

static TouchButton * TouchButtonTestGraphics;

static TouchButton * TouchButtonTestMisc;

static TouchButton * TouchButtonTestExceptions;
// to call test functions
static TouchButton * TouchButtonTestFunction1;
static TouchButton * TouchButtonTestFunction2;
static TouchButton * TouchButtonTestFunction3;
static TouchButton * TouchButtonTestMandelbrot;

TouchButtonAutorepeat * TouchButtonAutorepeatTest_Plus;
TouchButtonAutorepeat * TouchButtonAutorepeatTest_Minus;

TouchButton * TouchButtonPlus;
TouchButton * TouchButtonMinus;

#define TEST_BUTTONS_NUMBER_TO_DISPLAY 11 // Number of buttons on main test page, without back button
static TouchButton ** TouchButtonsTestPage[] = { &TouchButtonTestMandelbrot, &TouchButtonTestExceptions, &TouchButtonTestMisc,
        &TouchButtonTestGraphics, &TouchButtonTestFunction1, &TouchButtonTestFunction2, &TouchButtonTestFunction3,
        (TouchButton **) &TouchButtonAutorepeatTest_Plus, (TouchButton **) &TouchButtonAutorepeatTest_Minus, &TouchButtonPlus,
        &TouchButtonMinus, &TouchButtonBack };

/* Private function prototypes -----------------------------------------------*/

void displayTestsPage(void);
void testChart(void);

/* Private functions ---------------------------------------------------------*/
void Mandel(uint16_t size_x, uint16_t size_y, uint16_t offset_x, uint16_t offset_y, uint16_t zoom) {
    float tmp1, tmp2;
    float num_real, num_img;
    float radius;
    uint8_t i;
    uint16_t x, y;
    for (y = 0; y < size_y; y++) {
        for (x = 0; x < size_x; x++) {
            num_real = y - offset_y;
            num_real = num_real / zoom;
            num_img = x - offset_x;
            num_img = num_img / zoom;
            i = 0;
            radius = 0;
            while ((i < 256 - 1) && (radius < 4)) {
                tmp1 = num_real * num_real;
                tmp2 = num_img * num_img;
                num_img = 2 * num_real * num_img + 0.6;
                num_real = tmp1 - tmp2 - 0.4;
                radius = tmp1 + tmp2;
                i++;
            }
            BlueDisplay1.drawPixel(x, y, RGB(i*8,i*18,i*13));
        }
    }
}

/*********************************************
 * Test stuff
 *********************************************/
static int testValueIntern = 0;
int testValueForExtern = 0x0000;
#define MAX_TEST_VALUE 7
void drawTestvalue(void) {
    snprintf(StringBuffer, sizeof StringBuffer, "MMC SPI_BRPrescaler=%3d", 0x01 << (testValueIntern + 1));
    BlueDisplay1.drawText(0, BUTTON_HEIGHT_4_LINE_4 - TEXT_SIZE_11_DECEND, StringBuffer, TEXT_SIZE_11, COLOR_BLUE, COLOR_WHITE);
}

void doSetTestvalue(TouchButton * const aTheTouchedButton, int16_t aValue) {
    int tFeedbackType = FEEDBACK_TONE_NO_ERROR;
    int tTestValue = testValueIntern + aValue;
    if (tTestValue < 0) {
        tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
        tTestValue = 0;
    } else if (tTestValue > MAX_TEST_VALUE) {
        tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
        tTestValue = MAX_TEST_VALUE;
    }
    FeedbackTone(tFeedbackType);
    testValueIntern = tTestValue;
    testValueForExtern = testValueIntern << 3;
    drawTestvalue();
}

void drawBaudrate(uint32_t aBaudRate) {
    snprintf(StringBuffer, sizeof StringBuffer, "BR=%6lu", aBaudRate);
    BlueDisplay1.drawText(220, BUTTON_HEIGHT_4_LINE_4 - TEXT_SIZE_11_DECEND, StringBuffer, TEXT_SIZE_11, COLOR_BLUE, COLOR_WHITE);
}

void doChangeBaudrate(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    uint32_t tBaudRate = getUSART3BaudRate();
    uint32_t tBaudRateDelta = tBaudRate / 256;
    if (aValue < 0) {
        // decrease baud rate
        tBaudRate -= tBaudRateDelta;
    } else {
        // increase baud rate
        tBaudRate += tBaudRateDelta;
    }
    setUSART3BaudRate(tBaudRate);
    drawBaudrate(tBaudRate);
}

void showPrintfExamples() {
    int YPos = BUTTON_HEIGHT_4_LINE_2;
    float tTest = 1.23456789;
    snprintf(StringBuffer, sizeof StringBuffer, "5.2f=%5.2f 0.3f=%0.3f 4f=%4f", tTest, tTest, tTest);
    BlueDisplay1.drawText(10, YPos, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
    YPos += TEXT_SIZE_11_HEIGHT;
    tTest = -1.23456789;
    snprintf(StringBuffer, sizeof StringBuffer, "5.2f=%5.2f 0.3f=%0.3f 4f=%4f", tTest, tTest, tTest);
    BlueDisplay1.drawText(10, YPos, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
    YPos += TEXT_SIZE_11_HEIGHT;
    tTest = 12345.6789;
    snprintf(StringBuffer, sizeof StringBuffer, "7.0f=%7.0f 7f=%7f", tTest, tTest);
    BlueDisplay1.drawText(10, YPos, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
    YPos += TEXT_SIZE_11_HEIGHT;
    tTest = 0.0123456789;
    snprintf(StringBuffer, sizeof StringBuffer, "3.2f=%3.2f 0.4f=%0.4f 4f=%4f", tTest, tTest, tTest);
    BlueDisplay1.drawText(10, YPos, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
    YPos += TEXT_SIZE_11_HEIGHT;
    uint16_t t16 = 0x02CF;
    snprintf(StringBuffer, sizeof StringBuffer, "#x=%#x X=%X 5x=%5x 0#8X=%0#8X", t16, t16, t16, t16);
    BlueDisplay1.drawText(10, YPos, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
    YPos += 2 * TEXT_SIZE_11_HEIGHT;
    displayTimings(YPos);
}

void doTestButtons(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    // Function which does not need a new screen
    if (aTheTouchedButton == TouchButtonTestFunction1) {
        setUSART3BaudRate(230400);
        return;
    }
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    TouchButton::deactivateAllButtons();
    TouchButtonBack->drawButton();
    sBackButtonPressed = false;

// Exceptions
    if (aTheTouchedButton == TouchButtonTestExceptions) {
        assertParamMessage((false), 0x12345678, "Error Message");
        delayMillis(2000);
        snprintf(StringBuffer, sizeof StringBuffer, "Dummy %#lx", *((volatile uint32_t *) (0xFFFFFFFF)));

        // Miscellaneous tests
    } else if (aTheTouchedButton == TouchButtonTestMisc) {
        showPrintfExamples();
        do {
            testTimingsLoop(1);
            checkAndHandleEvents();
        } while (!sBackButtonPressed);

    } else if (aTheTouchedButton == TouchButtonTestMandelbrot) {
        int i = 1;
        do {
            Mandel(320, 240, 160, 120, i);
            i += 10;
            if (i > 1000)
                i = 0;
            checkAndHandleEvents();
        } while (!sBackButtonPressed);
    } else if (aTheTouchedButton == TouchButtonTestGraphics) {
        TouchButtonBack->activate();
        BlueDisplay1.testDisplay();
        TouchButtonBack->drawButton();
        /**
         * Test functions which needs a new screen
         */
    } else if (aTheTouchedButton == TouchButtonTestFunction3) {
        do {
            delayMillisWithCheckAndHandleEvents(1000);
        } while (!sBackButtonPressed);
    } else if (aTheTouchedButton == TouchButtonTestFunction2) {
        STM_EVAL_LEDOff(LED7); // GREEN RIGHT
//        do {
//        checkAndHandleTouchEvents();
//        } while (!sBackButtonPressed);

    }
}

/**
 * switch back to tests menu
 */
void doTestsBackButton(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    displayTestsPage();
    sBackButtonPressed = true;
}

void displayTestsPage(void) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    for (unsigned int i = 0; i < TEST_BUTTONS_NUMBER_TO_DISPLAY; ++i) {
        (*TouchButtonsTestPage[i])->drawButton();
    }
    TouchButtonMainHome->drawButton();
    drawTestvalue();
    drawBaudrate(getUSART3BaudRate());
}

void initTestPage(void) {
}

/**
 * allocate and position all BUTTONS
 */
void startTestsPage(void) {
    printSetPositionLineColumn(0, 0);

    TouchButton::setDefaultButtonColor(COLOR_GREEN);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"

    int tPosY = 0;
    //1. row
    TouchButtonTestMandelbrot = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "Mandelbrot",
            TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    TouchButtonBack = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_RED,
            StringBack, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, -1, &doTestsBackButton);

    // 2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonTestExceptions = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "Exceptions",
            TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    TouchButtonTestMisc = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
            "Misc Test", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    TouchButtonTestGraphics = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
            "Graph. Test", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    // 3. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonTestFunction1 = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "Baud",
            TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    TouchButtonTestFunction2 = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            0, "LED reset", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    TouchButtonTestFunction3 = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            0, "Func 3", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doTestButtons);

    // 4. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonAutorepeatTest_Plus = TouchButtonAutorepeat::allocButton();
    TouchButtonAutorepeatTest_Plus->initSimpleButton(0, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_5, COLOR_BLUE, StringPlus,
            TEXT_SIZE_22, 1, &doSetTestvalue);

    TouchButtonAutorepeatTest_Minus = TouchButtonAutorepeat::allocButton();
    TouchButtonAutorepeatTest_Minus->initSimpleButton(BUTTON_WIDTH_6_POS_2, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_5, COLOR_BLUE,
            StringMinus, TEXT_SIZE_22, -1, &doSetTestvalue);

    TouchButtonPlus = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_6_POS_4, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_5, 0, "+",
            TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 1, &doChangeBaudrate);

    TouchButtonMinus = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_6_POS_6, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_5, 0, "-",
            TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, -1, &doChangeBaudrate);

#pragma GCC diagnostic pop

    registerSimpleResizeAndReconnectCallback(&displayTestsPage);
    displayTestsPage();
}

void loopTestsPage(void) {
    checkAndHandleMessageReceived();
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

void testChart(void) {
    Chart ChartTest;

    ChartTest.disableXLabel();
    ChartTest.disableYLabel();
    ChartTest.initChartColors(COLOR_RED, COLOR_RED, CHART_DEFAULT_GRID_COLOR, COLOR_RED, COLOR_WHITE);
    ChartTest.initChart(4, BlueDisplay1.getDisplayHeight() - 10, 310, 120, 1, false, 20, 20);
//    ChartTest.drawAxesAndGrid();
//generate random data

    srand(120);
    uint8_t * tPtr = (uint8_t *) &FourDisplayLinesBuffer[0];
    for (int i = 0; i < 320; i++) {
        *tPtr++ = 30 + (rand() >> 27);
    }
    BlueDisplay1.drawText(10, 10, "x", TEXT_SIZE_22, COLOR_BLACK, COLOR_WHITE);
    ChartTest.drawChartDataDirect((uint8_t *) &FourDisplayLinesBuffer, 320, CHART_MODE_AREA);
}
