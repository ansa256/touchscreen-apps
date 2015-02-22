/*
 * PageInfo.cpp
 *
 * @date 16.03.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#include "MI0283QT2.h"
#include "Pages.h"
#include "TouchLib.h"
#include "TouchDSO.h"

#include "myprint.h"
#include "misc.h"
#include "main.h"
#include <string.h>
#include <locale.h>

extern "C" {
#include "stm32f3_discovery.h"
#include "l3gdc20_lsm303dlhc_utils.h"
#include "diskio.h"
#include "ff.h"
#include "usb_misc.h"

}

/* Private variables ---------------------------------------------------------*/
static TouchButton * TouchButtonBack;
static bool sBackButtonPressed;
static TouchButton * TouchButtonInfoFont1;
static TouchButton * TouchButtonInfoFont2;
static TouchButton * TouchButtonInfoColors;
static TouchButton * TouchButtonInfoMMC;
static TouchButton * TouchButtonInfoUSB;

static TouchButton * TouchButtonInfoSystem;

// Buttons for a single info page
static TouchButton * TouchButtonSettingsGamma1;
static TouchButton * TouchButtonSettingsGamma2;

#define INFO_BUTTONS_NUMBER_TO_DISPLAY 6 // Number of buttons on main info page, without back button
static TouchButton ** TouchButtonsInfoPage[] = { &TouchButtonInfoFont1, &TouchButtonInfoFont2, &TouchButtonInfoColors,
        &TouchButtonInfoMMC, &TouchButtonInfoSystem, &TouchButtonInfoUSB, &TouchButtonBack, &TouchButtonSettingsGamma1,
        &TouchButtonSettingsGamma2 };

/* Private function prototypes -----------------------------------------------*/

void pickColorPeriodicCallbackHandler(struct XYPosition * const aActualPositionPtr);
void displayInfoPage(void);

/* Private functions ---------------------------------------------------------*/

/**
 * for gamma change in color page
 * @param aTheTouchedButton
 * @param aValue
 */
void doSetGamma(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    setGamma(aValue);
}

int showSystemInfo() {
    int tYPos = 2 + TEXT_SIZE_11_ASCEND;
    unsigned int tControlRegisterContent = __get_CONTROL();
    snprintf(StringBuffer, sizeof StringBuffer, "Control=%#x xPSR=%#lx", tControlRegisterContent, __get_xPSR());
    BlueDisplay1.drawText(2, tYPos, StringBuffer, TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE);
    tYPos += TEXT_SIZE_11_HEIGHT;
    snprintf(StringBuffer, sizeof StringBuffer, "MSP=%#lx PSP=%#lx", __get_MSP(), __get_PSP());
    BlueDisplay1.drawText(2, tYPos, StringBuffer, TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE);
    // Clocks
    tYPos += TEXT_SIZE_11_HEIGHT;
    RCC_GetClocksFreq(&RCC_Clocks);
    snprintf(StringBuffer, sizeof StringBuffer, "SYSCLK_=%ldMHz HCLK=%ldMHz", RCC_Clocks.SYSCLK_Frequency / 1000000,
            RCC_Clocks.HCLK_Frequency / 1000000);
    BlueDisplay1.drawText(2, tYPos, StringBuffer, TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE);
    tYPos += TEXT_SIZE_11_HEIGHT;
    snprintf(StringBuffer, sizeof StringBuffer, "ADC12=%ldMHz ADC34=%ldMHz", RCC_Clocks.ADC12CLK_Frequency / 1000000,
            RCC_Clocks.ADC34CLK_Frequency / 1000000);
    BlueDisplay1.drawText(2, tYPos, StringBuffer, TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE);
    tYPos += TEXT_SIZE_11_HEIGHT;
    snprintf(StringBuffer, sizeof StringBuffer, "PCLK1=%ldMHz PCLK2=%ldMHz I2C1=%ldMHz", RCC_Clocks.PCLK1_Frequency / 1000000,
            RCC_Clocks.PCLK2_Frequency / 1000000, RCC_Clocks.I2C1CLK_Frequency / 1000000);
    BlueDisplay1.drawText(2, tYPos, StringBuffer, TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE);
    tYPos += TEXT_SIZE_11_HEIGHT;
    snprintf(StringBuffer, sizeof StringBuffer, "TIM1=%ldMHz TIM8=%ldMHz - NVIC lines %ld", RCC_Clocks.TIM1CLK_Frequency / 1000000,
            RCC_Clocks.TIM8CLK_Frequency / 1000000, ((SCnSCB ->ICTR & SCnSCB_ICTR_INTLINESNUM_Msk)+ 1)*32);
    BlueDisplay1.drawText(2, tYPos, StringBuffer, TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE);
    tYPos += TEXT_SIZE_11_HEIGHT;
    snprintf(StringBuffer, sizeof StringBuffer, "USART1=%ldMHz USART2=%ldMHz USART3=%ldMHz USART4=%ldMHz",
            RCC_Clocks.USART1CLK_Frequency / 1000000, RCC_Clocks.USART2CLK_Frequency / 1000000,
            RCC_Clocks.USART3CLK_Frequency / 1000000, RCC_Clocks.UART4CLK_Frequency / 1000000);
    BlueDisplay1.drawText(2, tYPos, StringBuffer, TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE);
    // Priorities
    tYPos += TEXT_SIZE_11_HEIGHT;
    uint32_t tPreemptPriority;
    uint32_t tSubPriority;
    NVIC_DecodePriority(NVIC_GetPriority(ADC1_2_IRQn), NVIC_PriorityGroup_2 >> 8, &tPreemptPriority, &tSubPriority);
    int tIndex = snprintf(StringBuffer, sizeof StringBuffer, "Prios: ADC=%ld/%ld", tPreemptPriority, tSubPriority);
    NVIC_DecodePriority(NVIC_GetPriority(SysTick_IRQn), NVIC_PriorityGroup_2 >> 8, &tPreemptPriority, &tSubPriority);
    tIndex += snprintf(&StringBuffer[tIndex], (sizeof StringBuffer) - tIndex, " SysTic=%ld/%ld", tPreemptPriority, tSubPriority);
    NVIC_DecodePriority(NVIC_GetPriority(EXTI1_IRQn), NVIC_PriorityGroup_2 >> 8, &tPreemptPriority, &tSubPriority);
    snprintf(&StringBuffer[tIndex], (sizeof StringBuffer) - tIndex, " EXTI1=%ld/%ld", tPreemptPriority, tSubPriority);
    BlueDisplay1.drawText(2, tYPos, StringBuffer, TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE);
    tYPos += TEXT_SIZE_11_HEIGHT;
    NVIC_DecodePriority(NVIC_GetPriority(USB_LP_CAN1_RX0_IRQn), NVIC_PriorityGroup_2 >> 8, &tPreemptPriority, &tSubPriority);
    tIndex = snprintf(StringBuffer, sizeof StringBuffer, " USB=%ld/%ld", tPreemptPriority, tSubPriority);
    NVIC_DecodePriority(NVIC_GetPriority(EXTI0_IRQn), NVIC_PriorityGroup_2 >> 8, &tPreemptPriority, &tSubPriority);
    tIndex += snprintf(&StringBuffer[tIndex], (sizeof StringBuffer) - tIndex, " User=%ld/%ld", tPreemptPriority, tSubPriority);
    BlueDisplay1.drawText(2, tYPos, StringBuffer, TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE);
    // Compile time
    tYPos += TEXT_SIZE_11_HEIGHT + TEXT_SIZE_11_HEIGHT / 2;
    snprintf(StringBuffer, sizeof StringBuffer, "Compiled at: %s %s", __DATE__, __TIME__);
    BlueDisplay1.drawText(2, tYPos, StringBuffer, TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE);
    // Locale
    //        tYPos += 2 * TEXT_SIZE_11_HEIGHT;
    //        struct lconv * tLconfPtr = localeconv();
    //        snprintf(StringBuffer, sizeof StringBuffer, "Locale=%s decimalpoint=%s thousands=%s", setlocale(LC_ALL, NULL),
    //                tLconfPtr->decimal_point, tLconfPtr->thousands_sep);
    //        BlueDisplay1.drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
    // Button Pool Info
    tYPos += TEXT_SIZE_11_HEIGHT;
    snprintf(StringBuffer, sizeof StringBuffer, "BUTTONS: poolsize=%d + %d", NUMBER_OF_BUTTONS_IN_POOL,
            NUMBER_OF_AUTOREPEAT_BUTTONS_IN_POOL);
    BlueDisplay1.drawText(2, tYPos, StringBuffer, TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE);
    tYPos += TEXT_SIZE_11_HEIGHT;
    TouchButton::infoButtonPool(StringBuffer);
    BlueDisplay1.drawText(2, tYPos, StringBuffer, TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE);
    // Lock info
    tYPos += TEXT_SIZE_11_HEIGHT;
    snprintf(StringBuffer, sizeof StringBuffer, "LOCK count=%d", sLockCount);
    BlueDisplay1.drawText(2, tYPos, StringBuffer, TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE);
    // Debug Info - just in case
    tYPos += TEXT_SIZE_11_HEIGHT;
    snprintf(StringBuffer, sizeof StringBuffer, "Dbg: 1=%#X, 1=%d 2=%#X", DebugValue1, DebugValue1, DebugValue2);
    BlueDisplay1.drawText(2, tYPos, StringBuffer, TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE);
    //        tYPos += TEXT_SIZE_11_HEIGHT;
    //        snprintf(StringBuffer, sizeof StringBuffer, "Dbg: 3=%#X 4=%#X 5=%#X", DebugValue3, DebugValue4, DebugValue5);
    //        BlueDisplay1.drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
    return tYPos;
}

void doInfoButtons(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    TouchButton::deactivateAllButtons();
    TouchButtonBack->drawButton();
    sBackButtonPressed = false;
    if (aTheTouchedButton == TouchButtonInfoFont1) {
        uint16_t tXPos;
        uint16_t tYPos = 6 * TEXT_SIZE_11_HEIGHT;
        // printable ascii chars
        unsigned char tChar = FONT_START;
        for (int i = 6; i != 0; i--) {
            tXPos = 0;
            for (uint8_t j = 16; j != 0; j--) {
                tXPos = BlueDisplay1.drawChar(tXPos, tYPos, tChar, TEXT_SIZE_22, COLOR_BLACK, COLOR_YELLOW) + 4;
                tChar++;
            }
            tYPos += TEXT_SIZE_22_HEIGHT + 4;
        }
    } else if (aTheTouchedButton == TouchButtonInfoFont2) {
        uint16_t tXPos;
        uint16_t tYPos = TEXT_SIZE_22_HEIGHT + 4;
        // special chars
        unsigned char tChar = 0x80;
        for (int i = 8; i != 0; i--) {
            tXPos = 0;
            for (uint8_t j = 16; j != 0; j--) {
                tXPos = BlueDisplay1.drawChar(tXPos, tYPos, tChar, TEXT_SIZE_22, COLOR_BLACK, COLOR_YELLOW) + 4;
                tChar++;
            }
            tYPos += TEXT_SIZE_22_HEIGHT + 2;
        }
    }

    else if (aTheTouchedButton == TouchButtonInfoColors) {
        BlueDisplay1.generateColorSpectrum();
        TouchButtonSettingsGamma1->activate();
        TouchButtonSettingsGamma2->activate();
        registerTouchDownCallback(&pickColorPeriodicCallbackHandler);
        registerTouchMoveCallback(&pickColorPeriodicCallbackHandler);
        while (!sBackButtonPressed) {
            checkAndHandleEvents();
        }
        registerTouchDownCallback(&simpleTouchDownHandler);
        // for sub pages
        registerTouchMoveCallback(&simpleTouchMoveHandlerForSlider);
    }

    else if (aTheTouchedButton == TouchButtonInfoMMC) {
        int tRes = getCardInfo(StringBuffer, sizeof(StringBuffer));
        if (tRes == 0 || tRes > RES_PARERR) {
            drawMLText(0, 10, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
        } else {
            testAttachMMC();
        }
        if (tRes > RES_PARERR) {
            tRes = getFSInfo(StringBuffer, sizeof(StringBuffer));
            if (tRes == 0 || tRes > FR_INVALID_PARAMETER) {
                drawMLText(0, 20 + 4 * TEXT_SIZE_11_HEIGHT, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_BACKGROUND_DEFAULT);
            }
        }
        // was overwritten by MLText
        TouchButtonBack->drawButton();
    }
    /*
     * USB info
     */
    else if (aTheTouchedButton == TouchButtonInfoUSB) {
        bool tOriginalState = getPrintStatus();
        printEnable();
        printSetPosition(0, BUTTON_HEIGHT_4_LINE_2 + 4 * TEXT_SIZE_11_HEIGHT);
        getUSB_StaticInfos(StringBuffer, sizeof StringBuffer);
        // 3 lines
        BlueDisplay1.drawMLText(10, BUTTON_HEIGHT_4_LINE_2 + TEXT_SIZE_11_ASCEND, StringBuffer, TEXT_SIZE_11, COLOR_RED,
                COLOR_BACKGROUND_DEFAULT);
        USB_ChangeToCDC();
        do {
            uint8_t * tRecData = CDC_Loopback();
            if (tRecData != NULL) {
                myPrint((const char*) tRecData, CDC_RX_BUFFER_SIZE);
            }
            BlueDisplay1.drawText(10, BUTTON_HEIGHT_4_LINE_2 - TEXT_SIZE_11_DECEND, getUSBDeviceState(), TEXT_SIZE_11, COLOR_RED,
                    COLOR_BACKGROUND_DEFAULT);
            delayMillisWithCheckAndHandleEvents(500);
        } while (!sBackButtonPressed);
        setPrintStatus(tOriginalState);
    }
    /*
     * System info
     */
    else if (aTheTouchedButton == TouchButtonInfoSystem) {
        showSystemInfo();
        /*
         * show temperature
         */
        int tYPos = showSystemInfo();
        tYPos += 2 * TEXT_SIZE_11_HEIGHT;
        int tTempValueAt30Degree = *(uint16_t*) ((0x1FFFF7B8));    //0x6BE
        int tTempValueAt110Degree = *(uint16_t*) ((0x1FFFF7C2));    //0x509
        snprintf(StringBuffer, sizeof StringBuffer, "30\xF8=%#x 110\xF8=%#x", tTempValueAt30Degree, tTempValueAt110Degree);
        BlueDisplay1.drawText(2, tYPos, StringBuffer, TEXT_SIZE_11, COLOR_PAGE_INFO, COLOR_WHITE);
        int tTemperatureFiltered = ADC_getTemperatureMilligrades() / 10;
        int tTemperatureFiltered2 = tTemperatureFiltered;
        do {
            int tTemp = ADC_getTemperatureMilligrades() / 10;
            // filter 16 samples
            tTemperatureFiltered = (tTemperatureFiltered * 15 + tTemp + 8) / 16;
            // filter 32 samples
            tTemperatureFiltered2 = (tTemperatureFiltered2 * 31 + tTemp + 16) / 32;
            snprintf(StringBuffer, sizeof StringBuffer, "%5.2f\xB0" "C %2d.%02d\xB0" "C %2d.%02d %2d.%02d", readLSM303TempFloat(),
                    tTemp / 100, tTemp % 100, tTemperatureFiltered / 100, tTemperatureFiltered % 100, tTemperatureFiltered2 / 100,
                    tTemperatureFiltered2 % 100);
            BlueDisplay1.drawText(2, BlueDisplay1.getDisplayHeight() - TEXT_SIZE_11_DECEND, StringBuffer, TEXT_SIZE_11,
                    COLOR_PAGE_INFO, COLOR_YELLOW);
            delayMillisWithCheckAndHandleEvents(500);
        } while (!sBackButtonPressed);
    }
}

// TODO implement readPixel
void pickColorPeriodicCallbackHandler(struct XYPosition * const aActualPositionPtr) {
    // first check button
    if (!TouchButton::checkAllButtons(aActualPositionPtr->PosX, aActualPositionPtr->PosY, true)) {
        uint16_t tColor = readPixel(aActualPositionPtr->PosX, aActualPositionPtr->PosY);
        snprintf(StringBuffer, sizeof StringBuffer, "%3d %3d R=0x%2x G=0x%2x B=0x%2x", aActualPositionPtr->PosX,
                aActualPositionPtr->PosY, (tColor >> 11) << 3, ((tColor >> 5) & 0x3F) << 2, (tColor & 0x1F) << 3);
        BlueDisplay1.drawText(2, BlueDisplay1.getDisplayHeight() - TEXT_SIZE_11_DECEND, StringBuffer, TEXT_SIZE_11, COLOR_WHITE,
                COLOR_BLACK);
// show color
        BlueDisplay1.fillRect(BlueDisplay1.getDisplayWidth() - 30, BlueDisplay1.getDisplayHeight() - 30,
                BlueDisplay1.getDisplayWidth() - 1, BlueDisplay1.getDisplayHeight() - 1, tColor);
    }
}

/**
 * switch back to info menu
 */
void doInfoBackButton(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    displayInfoPage();
    sBackButtonPressed = true;
}

void initInfoPage(void) {
}

void displayInfoPage(void) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
    for (unsigned int i = 0; i < INFO_BUTTONS_NUMBER_TO_DISPLAY; ++i) {
        (*TouchButtonsInfoPage[i])->drawButton();
    }
    TouchButtonMainHome->drawButton();
}

/**
 * allocate and position all BUTTONS
 */
void startInfoPage(void) {
    printSetPositionLineColumn(0, 0);

    TouchButton::setDefaultButtonColor(COLOR_GREEN);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"

    int tPosY = 0;
//1. row
    TouchButtonInfoFont1 = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "Font 1",
            TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);

    TouchButtonInfoFont2 = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
            "Font 2", TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);

    TouchButtonBack = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_RED,
            StringBack, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, -1, &doInfoBackButton);

// 2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonInfoMMC = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "MMC Infos",
            TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);

    TouchButtonInfoSystem = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
            "System Infos", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);

    TouchButtonInfoColors = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
            "Colors", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);

// 3. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonInfoUSB = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "USB Infos",
            TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doInfoButtons);

// 4. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;

// button for sub pages
    TouchButtonSettingsGamma1 = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, NULL, 0,
            BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doSetGamma);
    TouchButtonSettingsGamma2 = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            0, NULL, 0, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 1, &doSetGamma);

#pragma GCC diagnostic pop

    registerSimpleResizeAndReconnectCallback(&displayInfoPage);
    displayInfoPage();
}

void loopInfoPage(void) {
    checkAndHandleEvents();
}

/**
 * cleanup on leaving this page
 */
void stopInfoPage(void) {
// free buttons
    for (unsigned int i = 0; i < sizeof(TouchButtonsInfoPage) / sizeof(TouchButtonsInfoPage[0]); ++i) {
        (*TouchButtonsInfoPage[i])->setFree();
    }
}
