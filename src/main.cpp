/*
 * main.cpp
 *
 * @date 17.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 */

// file:///E:/WORKSPACE_STM32/TouchScreenApps/html/index.html
/* Includes ------------------------------------------------------------------*/
#include "Pages.h"

extern "C" {
#include "stm32f3_discovery_leds_buttons.h"
#include "stm32f30x.h"
#include "stm32f3_discovery.h"
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include "ff.h"
#include "diskio.h"
#include "usb_misc.h"
#include "USART_DMA.h"
}

#include "TouchDSO.h"
#ifdef LOCAL_DISPLAY_EXISTS
#include "ADS7846.h"
#endif
#include "AccuCapacity.h"
#include "GuiDemo.h"

void connectCallbackHandler(void) {
    BlueDisplay1.setFlagsAndSize(BD_FLAG_USE_MAX_SIZE, 320, 240);
    BlueDisplay1.setCharacterMapping(0x81, 0x03A9); // Omega in UTF16
    BlueDisplay1.setCharacterMapping(0xD3, 0x2302); // Home in UTF16
    BlueDisplay1.setCharacterMapping(0xF8, 0x2103); // Degree Celsius in UTF16
    BlueDisplay1.setCharacterMapping(0xD6, 0x21B2); // Enter in UTF16
    BlueDisplay1.setCharacterMapping(0xD1, 0x21E7); // Ascending in UTF16
    BlueDisplay1.setCharacterMapping(0xD2, 0x21E9); // Descending in UTF16
    BlueDisplay1.setCharacterMapping(0xE0, 0x2195); // UP/Down in UTF16
}

FATFS Fatfs[1];
RCC_ClocksTypeDef RCC_Clocks;
int main(void) {

    /* 2 bit for pre-emption priority, 2 bits for subpriority */
    NVIC_SetPriorityGrouping(NVIC_PriorityGroup_2 >> 8);

    RCC_GetClocksFreq(&RCC_Clocks);

    /* SysTick end of count event each ms */
    // sets Prio to 0x0F => Group=3 Sub=3 which is lowest possible prio for Systick Interrupt
    SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);

    // sets systic prio to 1 / 0
    NVIC_SetPriority(SysTick_IRQn, SYS_TICK_INTERRUPT_PRIO);

    initializeLEDs();

    /**
     * sets USB_LP_CAN1_RX0_IRQn Prio 2 of 0-3
     * USBWakeUp_IRQn Prio 1 of 0-3
     * USER_BUTTON_EXTI_IRQn 0 of 0-3
     */
    initUSB_CDC();

    /**
     * User Button
     * set USER_BUTTON_EXTI_IRQn to lowest possible prio 3 of 0-3 / subprio 3 of 0-3
     */
    STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_EXTI);

    RTC_initialize_LSE();
    initalizeTone();
#ifdef LOCAL_DISPLAY_EXISTS
    //init display early because assertions and timeouts want to be displayed :-)
    LocalDisplay.init();
    setDimDelayMillis(BACKLIGHT_DIM_DEFAULT_DELAY);
#endif
    // initialize SPI for peripherals
    SPI1_initialize();

    Debug_IO_initalize();

    Bluetoth_IO_initalize();
    USART3_initialize(115200);
    USART3_DMA_initialize();

    // sets EXTI4 to 3 3  - redefine user button prio to 0/3
    MICROSD_IO_initalize();

    // Setup ADC
    ADC12_init();
    tone(2000, 100);

    // Setup peripherals
    initAcceleratorCompassChip();
    GyroscopeAndSPIInitialize();
    Gyroscope_IO_initialize();

    if (isDisplayAvailable) {
        registerSimpleConnectCallback(&connectCallbackHandler);

        BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DEFAULT);
#ifdef LOCAL_DISPLAY_EXISTS
        //init touch controller
        TouchPanel.init();

        /**
         * Touch-panel calibration
         */
        TouchPanel.rd_data();
        // check if panel connected
        if (TouchPanel.getPressure() > MIN_REASONABLE_PRESSURE && TouchPanel.getPressure() < MAX_REASONABLE_PRESSURE) {
            // panel pressed
            TouchPanel.doCalibration(false); //don't check EEPROM for calibration data
        } else {
            TouchPanel.doCalibration(true); //check EEPROM for calibration data
        }
#endif

        initMainMenuPage();

        /**
         * ADC1_2_IRQn 0 (highest) of 0-3 /subprio 1 of 0-3
         */
        initDSO();
        initGuiDemo();
        initAcceleratorCompass();
        initAccuCapacity();
        initInfoPage();
        initDACPage();
        initFreqGenPage();
        //setZeroAcceleratorGyroValue();

        /* Just to be sure... */
        NVIC_SetPriorityGrouping(NVIC_PriorityGroup_2 >> 8);
        RCC_GetClocksFreq(&RCC_Clocks);

        if (MICROSD_isCardInserted()) {
            // delayed execution of MMC test
            registerDelayCallback(&testAttachMMC, 200);
        }

        // init button pools
        TouchButton * tTouchButton = TouchButton::allocButton(false);
        tTouchButton->setFree();
        TouchButtonAutorepeat * tTouchButtonAutorepeat = TouchButtonAutorepeat::allocButton();
        tTouchButtonAutorepeat->setFree();

        struct lconv * tLconfPtr = localeconv();
        //*tLconfPtr->decimal_point = ','; does not work because string is constant and in flash rom
        tLconfPtr->decimal_point = (char*) StringComma;
        //tLconfPtr->thousands_sep = (char*) StringDot; // does not affect sprintf()

        /* Infinite loop */
        while (1) {
            startMainMenuPage();
            while (1) {
                loopMainMenuPage();
            }
            stopMainMenuPage();
        }
    } else {
        while (1) {
            // no display attached
            CycleLEDs();
        }
    }
}

