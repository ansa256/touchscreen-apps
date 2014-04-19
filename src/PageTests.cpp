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

#include "myprint.h"
#include "misc.h"
#include "main.h"
#include <string.h>
#include <locale.h>

extern "C" {
#include "stm32f3_discovery.h"
#include "l3gdc20_lsm303dlhc_utils.h"
#include "usb_misc.h"
#include "diskio.h"
#include "ff.h"
}

/* Private variables ---------------------------------------------------------*/
static TouchButton * TouchButtonBack;
static bool sBackButtonPressed;

static TouchButton * TouchButtonTestGraphics;
static TouchButton * TouchButtonTestUSB;
static TouchButton * TouchButtonTestMisc;

static TouchButton * TouchButtonTestExceptions;
// to call test functions
static TouchButton * TouchButtonTestFunction1;
static TouchButton * TouchButtonTestFunction2;
static TouchButton * TouchButtonTestFunction3;

TouchButtonAutorepeat * TouchButtonAutorepeatTest_Plus;
TouchButtonAutorepeat * TouchButtonAutorepeatTest_Minus;

#define TEST_BUTTONS_NUMBER_TO_DISPLAY 9
static TouchButton ** TouchButtonsTestPage[] = { &TouchButtonTestUSB, &TouchButtonTestExceptions, &TouchButtonTestMisc,
        &TouchButtonTestGraphics, &TouchButtonTestFunction1, &TouchButtonTestFunction2, &TouchButtonTestFunction3,
        (TouchButton **)&TouchButtonAutorepeatTest_Plus, (TouchButton **)&TouchButtonAutorepeatTest_Minus, &TouchButtonBack };

/* Private function prototypes -----------------------------------------------*/

bool checkTestsBackButton(void);
void displayTestPage(void);

/* Private functions ---------------------------------------------------------*/

/*********************************************
 * Test stuff
 *********************************************/
static int testValueIntern = 0;
int testValueForExtern = 0x0000;
#define MAX_TEST_VALUE 7
void drawTestvalue(void) {
    snprintf(StringBuffer, sizeof StringBuffer, "MMC SPI_BaudRatePrescaler=%3d", 0x01 << (testValueIntern + 1));
    drawText(0, BUTTON_HEIGHT_4_LINE_4 - FONT_HEIGHT, StringBuffer, 1, COLOR_BLUE, COLOR_WHITE);
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
void showPrintfExamples() {
    int YPos = BUTTON_HEIGHT_4_LINE_2;
    float tTest = 1.23456789;
    snprintf(StringBuffer, sizeof StringBuffer, "5.2f=%5.2f 0.3f=%0.3f 4f=%4f", tTest, tTest, tTest);
    drawText(10, YPos, StringBuffer, 1, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
    YPos += FONT_HEIGHT;
    tTest = -1.23456789;
    snprintf(StringBuffer, sizeof StringBuffer, "5.2f=%5.2f 0.3f=%0.3f 4f=%4f", tTest, tTest, tTest);
    drawText(10, YPos, StringBuffer, 1, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
    YPos += FONT_HEIGHT;
    tTest = 12345.6789;
    snprintf(StringBuffer, sizeof StringBuffer, "7.0f=%7.0f 7f=%7f", tTest, tTest);
    drawText(10, YPos, StringBuffer, 1, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
    YPos += FONT_HEIGHT;
    tTest = 0.0123456789;
    snprintf(StringBuffer, sizeof StringBuffer, "3.2f=%3.2f 0.4f=%0.4f 4f=%4f", tTest, tTest, tTest);
    drawText(10, YPos, StringBuffer, 1, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
    YPos += FONT_HEIGHT;
    uint16_t t16 = 0x02CF;
    snprintf(StringBuffer, sizeof StringBuffer, "#x=%#x X=%X 5x=%5x 0#8X=%0#8X", t16, t16, t16, t16);
    drawText(10, YPos, StringBuffer, 1, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
    YPos += 2 * FONT_HEIGHT;
    displayTimings(YPos);
}

void doTestButtons(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    clearDisplay(COLOR_BACKGROUND_DEFAULT);
    TouchButton::deactivateAllButtons();
    TouchButtonBack->drawButton();
    sBackButtonPressed = false;
    /*
     * USB info and CDC loopback function
     */
    if (aTheTouchedButton == TouchButtonTestUSB) {
        bool tOriginalState = getPrintStatus();
        printEnable();
        printSetPosition(0, BUTTON_HEIGHT_4_LINE_2 + 4 * FONT_HEIGHT);
        getUSB_StaticInfos(StringBuffer, sizeof StringBuffer);
        // 3 lines
        drawMLText(10, BUTTON_HEIGHT_4_LINE_2, DISPLAY_WIDTH - 1, BUTTON_HEIGHT_4_LINE_2 + 2 * FONT_HEIGHT, StringBuffer, 1,
                COLOR_RED, COLOR_BACKGROUND_DEFAULT, false);
        USB_ChangeToCDC();
        do {
            uint8_t * tRecData = CDC_Loopback();
            if (tRecData != NULL) {
                myPrint((const char*) tRecData, CDC_RX_BUFFER_SIZE);
            }
            drawText(10, BUTTON_HEIGHT_4_LINE_2 - FONT_HEIGHT, getUSBDeviceState(), 1, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
        } while (!checkTestsBackButton());
        setPrintStatus(tOriginalState);

// Exceptions
    } else if (aTheTouchedButton == TouchButtonTestExceptions) {
        assertParamMessage((false), 0x12345678, "Error Message");
        delayMillis(2000);
        snprintf(StringBuffer, sizeof StringBuffer, "Dummy %#lx", *((volatile uint32_t *) (0xFFFFFFFF)));

        // Miscellaneous tests
    } else if (aTheTouchedButton == TouchButtonTestMisc) {
        showPrintfExamples();
        do {
            testTimingsLoop(1);
        } while (!checkTestsBackButton());

    } else if (aTheTouchedButton == TouchButtonTestGraphics) {
        TouchButtonBack->activate();
        testHY32D(1);
        TouchButtonBack->drawButton();
        /**
         * Test functions
         */
    } else if (aTheTouchedButton == TouchButtonTestFunction1) {
        do {
        } while (!checkTestsBackButton());
    } else if (aTheTouchedButton == TouchButtonTestFunction2) {

    } else if (aTheTouchedButton == TouchButtonTestFunction3) {

    }
}

/**
 * Calls callback routine of TestsBackButton if button was touched
 * @param DoNotCheckTouchPanelWasTouched
 * @return true if touched
 */ //
bool checkTestsBackButton(void) {
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
    displayTestPage();
    sBackButtonPressed = true;
}

void initTestPage(void) {
}

void displayTestPage(void) {
    clearDisplay(COLOR_BACKGROUND_DEFAULT);
    for (unsigned int i = 0; i < TEST_BUTTONS_NUMBER_TO_DISPLAY; ++i) {
        (*TouchButtonsTestPage[i])->drawButton();
    }
    TouchButtonMainHome->drawButton();
    drawTestvalue();
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
    TouchButtonTestUSB = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
            "USB Infos", 1, 0, &doTestButtons);

    TouchButtonBack = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_4_POS_4, tPosY, BUTTON_WIDTH_4, BUTTON_HEIGHT_4, COLOR_RED,
            StringBack, 2, -1, &doTestsBackButton);

    // 2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonTestExceptions = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "Exceptions", 1,
            0, &doTestButtons);

    TouchButtonTestMisc = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
            "Misc Test", 1, 0, &doTestButtons);

    TouchButtonTestGraphics = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
            "Graph. Test", 1, 0, &doTestButtons);

    // 3. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonTestFunction1 = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "Function 1", 1,
            0, &doTestButtons);

    TouchButtonTestFunction2 = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            0, "Function 2", 1, 0, &doTestButtons);

    TouchButtonTestFunction3 = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            0, "Function 3", 1, 0, &doTestButtons);

    // 4. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonAutorepeatTest_Plus = TouchButtonAutorepeat::allocButton();
    TouchButtonAutorepeatTest_Plus->initSimpleButton(0, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_5, COLOR_BLUE,
            StringPlus, 2, 1, &doSetTestvalue);

    TouchButtonAutorepeatTest_Minus = TouchButtonAutorepeat::allocButton();
    TouchButtonAutorepeatTest_Minus->initSimpleButton(BUTTON_WIDTH_6_POS_2, tPosY, BUTTON_WIDTH_6, BUTTON_HEIGHT_5, COLOR_BLUE,
            StringMinus, 2, -1, &doSetTestvalue);

#pragma GCC diagnostic pop

    displayTestPage();
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
