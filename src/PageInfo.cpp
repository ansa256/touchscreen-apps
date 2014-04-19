/*
 * PageInfo.cpp
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
#include "diskio.h"
#include "ff.h"
}

/* Private variables ---------------------------------------------------------*/
static TouchButton * TouchButtonBack;
static bool sBackButtonPressed;
static TouchButton * TouchButtonInfoFont1;
static TouchButton * TouchButtonInfoFont2;
static TouchButton * TouchButtonInfoColors;
static TouchButton * TouchButtonInfoMMC;
static TouchButton * TouchButtonInfoMandelbrot;

static TouchButton * TouchButtonInfoSystem;

// Buttons for a single info page
static TouchButton * TouchButtonSettingsGamma1;
static TouchButton * TouchButtonSettingsGamma2;

#define INFO_BUTTONS_NUMBER_TO_DISPLAY 6
static TouchButton ** TouchButtonsInfoPage[] = { &TouchButtonInfoFont1, &TouchButtonInfoFont2, &TouchButtonInfoColors,
        &TouchButtonInfoMMC, &TouchButtonInfoSystem, &TouchButtonInfoMandelbrot, &TouchButtonBack, &TouchButtonSettingsGamma1,
        &TouchButtonSettingsGamma2 };

/* Private function prototypes -----------------------------------------------*/

bool checkInfoBackButton(void);
bool pickColorPeriodicCallbackHandler(const int aTouchPositionX, const int aTouchPositionY);
void displayInfoPage(void);

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
            drawPixel(x, y, RGB(i*8,i*18,i*13));
        }
    }
}

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
    snprintf(StringBuffer, sizeof StringBuffer, "SYSCLK_=%ldMHz HCLK=%ldMHz", RCC_Clocks.SYSCLK_Frequency / 1000000,
            RCC_Clocks.HCLK_Frequency / 1000000);
    drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
    tYPos += FONT_HEIGHT;
    snprintf(StringBuffer, sizeof StringBuffer, "ADC12=%ldMHz ADC34=%ldMHz", RCC_Clocks.ADC12CLK_Frequency / 1000000,
            RCC_Clocks.ADC34CLK_Frequency / 1000000);
    drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
    tYPos += FONT_HEIGHT;
    snprintf(StringBuffer, sizeof StringBuffer, "PCLK1=%ldMHz PCLK2=%ldMHz I2C1=%ldMHz", RCC_Clocks.PCLK1_Frequency / 1000000,
            RCC_Clocks.PCLK2_Frequency / 1000000, RCC_Clocks.I2C1CLK_Frequency / 1000000);
    drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
    tYPos += FONT_HEIGHT;
    snprintf(StringBuffer, sizeof StringBuffer, "TIM1=%ldMHz TIM8=%ldMHz - NVIC lines %ld", RCC_Clocks.TIM1CLK_Frequency / 1000000,
            RCC_Clocks.TIM8CLK_Frequency / 1000000, ((SCnSCB ->ICTR & SCnSCB_ICTR_INTLINESNUM_Msk)+ 1)*32);
    drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
    tYPos += FONT_HEIGHT;
    snprintf(StringBuffer, sizeof StringBuffer, "USART1=%ldMHz USART2=%ldMHz USART3=%ldMHz USART4=%ldMHz",
            RCC_Clocks.USART1CLK_Frequency / 1000000, RCC_Clocks.USART2CLK_Frequency / 1000000,
            RCC_Clocks.USART3CLK_Frequency / 1000000, RCC_Clocks.UART4CLK_Frequency / 1000000);
    drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
    // Priorities
    tYPos += FONT_HEIGHT;
    uint32_t tPreemptPriority;
    uint32_t tSubPriority;
    NVIC_DecodePriority(NVIC_GetPriority(ADC1_2_IRQn), NVIC_PriorityGroup_2 >> 8, &tPreemptPriority, &tSubPriority);
    int tIndex = snprintf(StringBuffer, sizeof StringBuffer, "Prios: ADC=%ld/%ld", tPreemptPriority, tSubPriority);
    NVIC_DecodePriority(NVIC_GetPriority(SysTick_IRQn), NVIC_PriorityGroup_2 >> 8, &tPreemptPriority, &tSubPriority);
    tIndex += snprintf(&StringBuffer[tIndex], (sizeof StringBuffer) - tIndex, " SysTic=%ld/%ld", tPreemptPriority, tSubPriority);
    NVIC_DecodePriority(NVIC_GetPriority(EXTI1_IRQn), NVIC_PriorityGroup_2 >> 8, &tPreemptPriority, &tSubPriority);
    snprintf(&StringBuffer[tIndex], (sizeof StringBuffer) - tIndex, " EXTI1=%ld/%ld", tPreemptPriority, tSubPriority);
    drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
    tYPos += FONT_HEIGHT;
    NVIC_DecodePriority(NVIC_GetPriority(USB_LP_CAN1_RX0_IRQn), NVIC_PriorityGroup_2 >> 8, &tPreemptPriority, &tSubPriority);
    tIndex = snprintf(StringBuffer, sizeof StringBuffer, " USB=%ld/%ld", tPreemptPriority, tSubPriority);
    NVIC_DecodePriority(NVIC_GetPriority(EXTI0_IRQn), NVIC_PriorityGroup_2 >> 8, &tPreemptPriority, &tSubPriority);
    tIndex += snprintf(&StringBuffer[tIndex], (sizeof StringBuffer) - tIndex, " User=%ld/%ld", tPreemptPriority, tSubPriority);
    drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
    // Compile time
    tYPos += FONT_HEIGHT + FONT_HEIGHT / 2;
    snprintf(StringBuffer, sizeof StringBuffer, "Compiled at: %s %s", __DATE__, __TIME__);
    drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
    // Locale
    //        tYPos += 2 * FONT_HEIGHT;
    //        struct lconv * tLconfPtr = localeconv();
    //        snprintf(StringBuffer, sizeof StringBuffer, "Locale=%s decimalpoint=%s thousands=%s", setlocale(LC_ALL, NULL),
    //                tLconfPtr->decimal_point, tLconfPtr->thousands_sep);
    //        drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
    // Button Pool Info
    tYPos += FONT_HEIGHT;
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
    //        tYPos += FONT_HEIGHT;
    //        snprintf(StringBuffer, sizeof StringBuffer, "Dbg: 3=%#X 4=%#X 5=%#X", DebugValue3, DebugValue4, DebugValue5);
    //        drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
    return tYPos;
}

void doInfoButtons(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    clearDisplay(COLOR_BACKGROUND_DEFAULT);
    TouchButton::deactivateAllButtons();
    TouchButtonBack->drawButton();
    sBackButtonPressed = false;
    if (aTheTouchedButton == TouchButtonInfoFont1) {
        uint16_t tXPos;
        uint16_t tYPos = 6 * FONT_HEIGHT;
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
    } else if (aTheTouchedButton == TouchButtonInfoFont2) {
        uint16_t tXPos;
        uint16_t tYPos = 2 * FONT_HEIGHT + 4;
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

    else if (aTheTouchedButton == TouchButtonInfoColors) {
        generateColorSpectrum();
        TouchButtonSettingsGamma1->activate();
        TouchButtonSettingsGamma2->activate();
        while (!sBackButtonPressed) {
            if (CheckTouchGeneric(true) == GUI_TOUCH_NO_MATCH) {
                TouchPanel.registerPeriodicTouchCallback(&pickColorPeriodicCallbackHandler, 10);
            }
        }
    }

    else if (aTheTouchedButton == TouchButtonInfoMMC) {
        int tRes = getCardInfo(StringBuffer, sizeof(StringBuffer));
        if (tRes == 0 || tRes > RES_PARERR) {
            drawMLText(0, 10, DISPLAY_WIDTH, 10 + 4 * FONT_HEIGHT, StringBuffer, 1, COLOR_RED, COLOR_BACKGROUND_DEFAULT, false);
        } else {
            testAttachMMC();
        }
        if (tRes > RES_PARERR) {
            tRes = getFSInfo(StringBuffer, sizeof(StringBuffer));
            if (tRes == 0 || tRes > FR_INVALID_PARAMETER) {
                drawMLText(0, 20 + 4 * FONT_HEIGHT, DISPLAY_WIDTH, 20 + 14 * FONT_HEIGHT, StringBuffer, 1, COLOR_RED,
                        COLOR_BACKGROUND_DEFAULT, false);
            }
        }
    }
    /*
     * System info
     */
    else if (aTheTouchedButton == TouchButtonInfoSystem) {
        showSystemInfo();
        // Temperature
        int tYPos = showSystemInfo();
        tYPos += 2 * FONT_HEIGHT;
        int tTempValueAt30Degree = *(uint16_t*) ((0x1FFFF7B8)); //0x6BE
        int tTempValueAt110Degree = *(uint16_t*) ((0x1FFFF7C2)); //0x509
        snprintf(StringBuffer, sizeof StringBuffer, "30\xF8=%#x 110\xF8=%#x", tTempValueAt30Degree, tTempValueAt110Degree);
        drawText(2, tYPos, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
        int tTemperatureFiltered = ADC_getTemperatureMilligrades() / 10;
        int tTemperatureFiltered2 = tTemperatureFiltered;
        do {
            int tTemp = ADC_getTemperatureMilligrades() / 10;
            // filter 16 samples
            tTemperatureFiltered = (tTemperatureFiltered * 15 + tTemp + 8) / 16;
            // filter 32 samples
            tTemperatureFiltered2 = (tTemperatureFiltered2 * 31 + tTemp + 16) / 32;
            snprintf(StringBuffer, sizeof StringBuffer, "%5.2f\xF8" "C %2d.%02d\xF8" "C %2d.%02d %2d.%02d", readLSM303TempFloat(),
                    tTemp / 100, tTemp % 100, tTemperatureFiltered / 100, tTemperatureFiltered % 100, tTemperatureFiltered2 / 100,
                    tTemperatureFiltered2 % 100);
            drawText(2, DISPLAY_HEIGHT - FONT_HEIGHT, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
            delayMillis(200);
        } while (!checkInfoBackButton());
    } else if (aTheTouchedButton == TouchButtonInfoMandelbrot) {
        int i = 1;
        do {
            Mandel(320, 240, 160, 120, i);
            i += 10;
            if (i > 1000)
                i = 0;
        } while (!checkInfoBackButton());
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
 * Calls callback routine of InfoBackButton if button was touched
 * @param DoNotCheckTouchPanelWasTouched
 * @return true if touched
 */ //
bool checkInfoBackButton(void) {
    if (TouchPanel.wasTouched()) {
        return TouchButtonBack->checkButton(TouchPanel.getXActual(), TouchPanel.getYActual(), true);
    }
    return false;
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
    clearDisplay(COLOR_BACKGROUND_DEFAULT);
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
    TouchButtonInfoFont1 = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "Font 1", 2, 0,
            &doInfoButtons);

    TouchButtonInfoFont2 = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
            "Font 2", 2, 0, &doInfoButtons);

    TouchButtonBack = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_4_POS_4, tPosY, BUTTON_WIDTH_4, BUTTON_HEIGHT_4, COLOR_RED,
            StringBack, 2, -1, &doInfoBackButton);

    // 2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonInfoMMC = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "MMC Infos", 1, 0,
            &doInfoButtons);

    TouchButtonInfoSystem = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
            "System Infos", 1, 0, &doInfoButtons);

    TouchButtonInfoColors = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
            "Colors", 1, 0, &doInfoButtons);

    // 3. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonInfoMandelbrot = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "Mandelbrot", 1,
            0, &doInfoButtons);

    // 4. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;

    // button for sub pages
    TouchButtonSettingsGamma1 = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, NULL, 0, 0,
            &doSetGamma);
    TouchButtonSettingsGamma2 = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            0, NULL, 0, 1, &doSetGamma);

#pragma GCC diagnostic pop

    displayInfoPage();
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
