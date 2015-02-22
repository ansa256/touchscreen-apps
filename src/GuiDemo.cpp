/**
 * @file GuiDemo.cpp
 *
 * @date 31.01.2012
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 *
 *      Demo of the libs:
 *      TouchButton
 *      TouchSlider
 *      Chart
 *
 *      and the "Apps" ;-)
 *      Draw lines
 *      Game of life
 *      Show ADS7846 A/D channels
 *      Display font
 *
 *      For STM32F3 Discovery
 *
 */

#include "Pages.h"
#ifdef LOCAL_DISPLAY_EXISTS
#include "ADS7846.h"
#endif

#include "GameOfLife.h"
#include "Chart.h"
#include "misc.h"

/** @addtogroup Gui_Demo
 * @{
 */

/*
 * LCD and touch panel stuff
 */

#define COLOR_DEMO_BACKGROUND COLOR_WHITE

void createDemoButtonsAndSliders(void);

// Global button handler
void doGuiDemoButtons(TouchButton * const aTheTochedButton, int16_t aValue);

//Global slider handler
uint16_t doSliders(TouchSlider * const aTheTochedSlider, uint16_t aSliderValue);

static TouchButton * TouchButtonBack;

static TouchButton * TouchButtonContinue;

/*
 * Game of life stuff
 */
static TouchButton * TouchButtonDemoSettings;
void showGolSettings(void);

static TouchButton * TouchButtonGolDying;
static TouchSlider TouchSliderGolDying;
void doGolDying(TouchButton * const aTheTouchedButton, int16_t aValue);
void syncronizeGolSliderAndButton(bool aValue);
const char * mapValueGol(uint16_t aValue);

static TouchButton * TouchButtonNew;
void doNew(TouchButton * const aTheTouchedButton, int16_t aValue);
void doContinue(TouchButton * const aTheTouchedButton, int16_t aValue);
void initNewGameOfLife(void);

static TouchSlider TouchSliderGolSpeed;

const char StringGOL[] = "GameOfLife";
// to slow down game of life
unsigned long GolDelay = 0;
bool GolShowDying = true;
bool GolRunning = false;
bool GolInitialized = false;
#define GOL_DIE_MAX 20

/**
 * Menu stuff
 */
void showGuiDemoMenu(void);

static TouchButton * TouchButtonChartDemo;
void showCharts(void);

// space between buttons
#define APPLICATION_MENU 0
#define APPLICATION_SETTINGS 1
#define APPLICATION_DRAW 2
#define APPLICATION_GAME_OF_LIFE 3
#define APPLICATION_CHART 4
#define APPLICATION_ADS7846_CHANNELS 5

int mActualApplication = APPLICATION_MENU;

/*
 * Settings stuff
 */
void showSettings(void);

static TouchButton * TouchButtonGameOfLife;
const char StringTPCalibration[] = "TP-Calibration";

static TouchSlider TouchSliderDemo1;
static TouchSlider TouchSliderAction;
static TouchSlider TouchSliderActionWithoutBorder;
unsigned int ActionSliderValue = 0;
#define ACTION_SLIDER_MAX 100
bool ActionSliderUp = true;

#ifdef LOCAL_DISPLAY_EXISTS
static TouchButton * TouchButtonCalibration;

/*
 * ADS7846 channels stuff
 */
static TouchButton * TouchButtonADS7846Channels;
void ADS7846DisplayChannels(void);
void doADS7846Channels(TouchButton * const aTheTouchedButton, int16_t aValue);
#endif

void initGuiDemo(void) {
}

static TouchButton ** TouchButtonsGuiDemo[] = { &TouchButtonDemoSettings, &TouchButtonChartDemo, &TouchButtonGameOfLife,
        &TouchButtonBack, &TouchButtonGolDying, &TouchButtonNew, &TouchButtonContinue
#ifdef LOCAL_DISPLAY_EXISTS
        , &TouchButtonCalibration, &TouchButtonADS7846Channels
#endif
        };

void startGuiDemo(void) {
    createDemoButtonsAndSliders();

#ifdef LOCAL_DISPLAY_EXISTS
    initBacklightElements();
#endif
    showGuiDemoMenu();
    MillisLastLoop = getMillisSinceBoot();
}

void loopGuiDemo(void) {
    // count milliseconds for loop control
    uint32_t tMillis = getMillisSinceBoot();
    MillisSinceLastAction += tMillis - MillisLastLoop;
    MillisLastLoop = tMillis;

    if (sNothingTouched) {
        /*
         * switch for different "apps"
         */
        switch (mActualApplication) {
        case APPLICATION_GAME_OF_LIFE:
            if (GolRunning) {
                // switch back to settings page
                showGolSettings();
            }
            break;
        }
    }

    /*
     * switch for different "apps"
     */
    switch (mActualApplication) {
    case APPLICATION_SETTINGS:
        // Moving slider bar :-)
        if (MillisSinceLastAction >= 20) {
            MillisSinceLastAction = 0;
            if (ActionSliderUp) {
                ActionSliderValue++;
                if (ActionSliderValue == ACTION_SLIDER_MAX) {
                    ActionSliderUp = false;
                }
            } else {
                ActionSliderValue--;
                if (ActionSliderValue == 0) {
                    ActionSliderUp = true;
                }
            }
            TouchSliderAction.setActualValueAndDraw(ActionSliderValue);
            TouchSliderActionWithoutBorder.setActualValueAndDraw(ACTION_SLIDER_MAX - ActionSliderValue);
        }
        break;

    case APPLICATION_GAME_OF_LIFE:
        if (GolRunning && MillisSinceLastAction >= GolDelay) {
            //game of live "app"
            play_gol();
            drawGenerationText();
            draw_gol();
            MillisSinceLastAction = 0;
        }
        break;

    case APPLICATION_MENU:
        break;
    }

    /*
     * Actions independent of touch
     */
#ifdef LOCAL_DISPLAY_EXISTS
    if (mActualApplication == APPLICATION_ADS7846_CHANNELS) {
        ADS7846DisplayChannels();
    }
#endif
    checkAndHandleEvents();
}

void stopGuiDemo(void) {
    // free buttons
    for (unsigned int i = 0; i < sizeof(TouchButtonsGuiDemo) / sizeof(TouchButtonsGuiDemo[0]); ++i) {
        (*TouchButtonsGuiDemo[i])->setFree();
    }
#ifdef LOCAL_DISPLAY_EXISTS
    deinitBacklightElements();
#endif
}

/*
 * buttons are positioned relative to sliders and vice versa
 */
void createDemoButtonsAndSliders(void) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
    /*
     * create and position all BUTTONS initially
     */
    TouchButton::setDefaultButtonColor(COLOR_RED);
    TouchSlider::resetDefaults();

    //1. row
    int tPosY = 0;
    TouchButtonChartDemo = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, 0, "Chart",
            TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doGuiDemoButtons);

    // Back text button for sub pages
    TouchButtonBack = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_RED,
            StringBack, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doGuiDemoButtons);

    // 2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonGameOfLife = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, 0, StringGOL,
            TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doGuiDemoButtons);

    // 3. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonDemoSettings = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, 0, StringSettings,
            TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doGuiDemoButtons);

#ifdef LOCAL_DISPLAY_EXISTS
    // 4. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonADS7846Channels = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, 0, "ADS7846",
            TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doADS7846Channels);

    // sub pages
    TouchButtonCalibration = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_2_POS_2, tPosY, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, 0,
            StringTPCalibration, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doGuiDemoButtons);
#endif
    TouchButtonNew = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, "New", TEXT_SIZE_22,
            BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doNew);

    TouchButtonContinue = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0,
            StringContinue, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doContinue);

    /*
     * Slider
     */
    TouchSliderGolSpeed.initSlider(70, BUTTON_HEIGHT_4 + BUTTON_DEFAULT_SPACING_HALF, TOUCHSLIDER_DEFAULT_SIZE, 75, 75, 75,
            "Gol-Speed", TOUCHSLIDER_DEFAULT_TOUCH_BORDER,
            TOUCHSLIDER_SHOW_BORDER | TOUCHSLIDER_SHOW_VALUE | TOUCHSLIDER_IS_HORIZONTAL | TOUCHSLIDER_HORIZONTAL_VALUE_BELOW_TITLE,
            &doSliders, &mapValueGol);
// slider as switch
    TouchSliderGolDying.initSlider(120, TouchSliderGolSpeed.getPositionYBottom() + TEXT_SIZE_22_HEIGHT + 4,
            TOUCHSLIDER_DEFAULT_SIZE, GOL_DIE_MAX, GOL_DIE_MAX, GOL_DIE_MAX, "Show dying", 20, TOUCHSLIDER_SHOW_BORDER, &doSliders,
            NULL);

// ON OFF button relative to slider
    TouchButtonGolDying = TouchButton::allocAndInitSimpleButton(120,
            TouchSliderGolDying.getPositionYBottom() + TEXT_SIZE_11_HEIGHT + 8, 25, 25, 0, NULL, TEXT_SIZE_11,
            BUTTON_FLAG_DO_BEEP_ON_TOUCH, false, &doGolDying);

// self moving sliders
    TouchSliderActionWithoutBorder.initSlider(180, BUTTON_HEIGHT_4_LINE_2, 10, ACTION_SLIDER_MAX, ACTION_SLIDER_MAX, 0, NULL, 3,
            TOUCHSLIDER_SHOW_VALUE, NULL, NULL);
    TouchSliderAction.initSlider(TouchSliderActionWithoutBorder.getPositionXRight() + BUTTON_DEFAULT_SPACING,
            BUTTON_HEIGHT_4_LINE_2 - 4, 10, ACTION_SLIDER_MAX, ACTION_SLIDER_MAX, 0, NULL, 0,
            TOUCHSLIDER_SHOW_BORDER | TOUCHSLIDER_SHOW_VALUE, NULL, NULL);

#pragma GCC diagnostic pop

}

uint16_t doSliders(TouchSlider * const aTheTouchedSlider, uint16_t aSliderValue) {
    if (aTheTouchedSlider == &TouchSliderGolSpeed) {
        aSliderValue = (aSliderValue / 25) * 25;
    }
    if (aTheTouchedSlider == &TouchSliderGolDying) {
        bool tValue = (aSliderValue > (GOL_DIE_MAX / 2));
        syncronizeGolSliderAndButton(tValue);
    }
    return aSliderValue;
}

void doGuiDemoButtons(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    TouchButton::deactivateAllButtons();
    TouchSlider::deactivateAllSliders();
    if (aTheTouchedButton == TouchButtonChartDemo) {
        showCharts();
        return;
    }

#ifdef LOCAL_DISPLAY_EXISTS

    if (aTheTouchedButton == TouchButtonCalibration) {
        //Calibration Button pressed -> calibrate touch panel
        TouchPanel.doCalibration(false);
        showSettings();
        return;
    }
#endif

    if (aTheTouchedButton == TouchButtonGameOfLife) {
        // Game of Life button pressed
        showGolSettings();
        mActualApplication = APPLICATION_GAME_OF_LIFE;
        return;
    }
    if (aTheTouchedButton == TouchButtonDemoSettings) {
        // Settings button pressed
        showSettings();
        return;
    }
    if (aTheTouchedButton == TouchButtonBack) {
        // Home button pressed
        showGuiDemoMenu();
        return;
    }
}

char StringSlowest[] = "slowest";
char StringSlow[] = "slow   ";
char StringNormal[] = "normal ";
char StringFast[] = "fast   ";

// value map function for game of life speed slider
const char * mapValueGol(uint16_t aValue) {
    aValue = aValue / 25;
    switch (aValue) {
    case 0:
        GolDelay = 8000;
        return StringSlowest;
    case 1:
        GolDelay = 2000;
        return StringSlow;
    case 2:
        GolDelay = 500;
        return StringNormal;
    case 3:
        GolDelay = 0;
        return StringFast;
    }
    return StringFast;
}

void doGolDying(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    aValue = !aValue;
    syncronizeGolSliderAndButton(aValue);
}

void showGolSettings(void) {
    /*
     * Settings button pressed
     */
    GolRunning = false;
    BlueDisplay1.clearDisplay(COLOR_DEMO_BACKGROUND);
    TouchButtonBack->drawButton();
    TouchSliderGolDying.drawSlider();
    syncronizeGolSliderAndButton(false);
    TouchSliderGolSpeed.drawSlider();
// redefine Buttons
    TouchButtonNew->drawButton();
    TouchButtonContinue->drawButton();
}

void doNew(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
// deactivate gui elements
    TouchButton::deactivateAllButtons();
    TouchSlider::deactivateAllSliders();
    initNewGameOfLife();
    MillisSinceLastAction = GolDelay;
    GolRunning = true;
}

void doContinue(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
// deactivate gui elements
    TouchButton::deactivateAllButtons();
    TouchSlider::deactivateAllSliders();
    if (!GolInitialized) {
        initNewGameOfLife();
    } else {
        ClearScreenAndDrawGameOfLifeGrid();
    }
    MillisSinceLastAction = GolDelay;
    GolRunning = true;
}

void initNewGameOfLife(void) {
    GolInitialized = true;
    init_gol();
}

void showSettings(void) {
    /*
     * Settings button pressed
     */
    BlueDisplay1.clearDisplay(COLOR_DEMO_BACKGROUND);
    TouchButtonBack->drawButton();
#ifdef LOCAL_DISPLAY_EXISTS
    TouchButtonCalibration->drawButton();
    drawBacklightElements();
#endif
    TouchSliderGolDying.drawSlider();
    syncronizeGolSliderAndButton(false);
    TouchSliderGolSpeed.drawSlider();
    TouchSliderAction.drawSlider();
    TouchSliderActionWithoutBorder.drawSlider();
    mActualApplication = APPLICATION_SETTINGS;
}

void showGuiDemoMenu(void) {
    TouchButtonBack->deactivate();

    BlueDisplay1.clearDisplay(COLOR_DEMO_BACKGROUND);
    TouchButtonMainHome->drawButton();
    TouchButtonDemoSettings->drawButton();
    TouchButtonChartDemo->drawButton();
#ifdef LOCAL_DISPLAY_EXISTS
    TouchButtonADS7846Channels->drawButton();
#endif
    TouchButtonGameOfLife->drawButton();
    GolInitialized = false;
    mActualApplication = APPLICATION_MENU;
}

void showFont(void) {
    BlueDisplay1.clearDisplay(COLOR_DEMO_BACKGROUND);
    TouchButtonBack->drawButton();

    uint16_t tXPos;
    uint16_t tYPos = 10;
    unsigned char tChar = 0x20;
    for (uint8_t i = 14; i != 0; i--) {
        tXPos = 10;
        for (uint8_t j = 16; j != 0; j--) {
            tXPos = BlueDisplay1.drawChar(tXPos, tYPos, tChar, TEXT_SIZE_11, COLOR_BLACK, COLOR_YELLOW) + 4;
            tChar++;
        }
        tYPos += TEXT_SIZE_11_HEIGHT + 4;
    }
}

/*
 * Show charts features
 */
void showCharts(void) {
    BlueDisplay1.clearDisplay(COLOR_DEMO_BACKGROUND);
    TouchButtonBack->drawButton();
    showChartDemo();
    mActualApplication = APPLICATION_CHART;
}

void syncronizeGolSliderAndButton(bool aValue) {
    GolShowDying = aValue;
    TouchButtonGolDying->setRedGreenButtonColorAndDraw(aValue);
    if (aValue == false) {
        TouchSliderGolDying.setActualValueAndDraw(0);
    } else {
        TouchSliderGolDying.setActualValueAndDraw(GOL_DIE_MAX);
    }
}

#ifdef LOCAL_DISPLAY_EXISTS
void doADS7846Channels(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    TouchButton::deactivateAllButtons();
    mActualApplication = APPLICATION_ADS7846_CHANNELS;
    BlueDisplay1.clearDisplay(COLOR_DEMO_BACKGROUND);
    uint16_t tPosY = 30;
    // draw text
    for (uint8_t i = 0; i < 8; ++i) {
        BlueDisplay1.drawText(90, tPosY, (char *) ADS7846ChannelStrings[i], TEXT_SIZE_22, COLOR_RED, COLOR_DEMO_BACKGROUND);
        tPosY += TEXT_SIZE_22_HEIGHT;
    }
    TouchButtonBack->drawButton();
}

#define BUTTON_CHECK_INTERVAL 20
void ADS7846DisplayChannels(void) {
    static uint8_t aButtonCheckInterval = 0;
    uint16_t tPosY = 30;
    int16_t tTemp;
    bool tUseDiffMode = true;
    bool tUse12BitMode = false;

    for (uint8_t i = 0; i < 8; ++i) {
        if (i == 4) {
            tUseDiffMode = false;
        }
        if (i == 2) {
            tUse12BitMode = true;
        }
        tTemp = TouchPanel.readChannel(ADS7846ChannelMapping[i], tUse12BitMode, tUseDiffMode, 2);
        snprintf(StringBuffer, sizeof StringBuffer, "%04u", tTemp);
        BlueDisplay1.drawText(15, tPosY, StringBuffer, TEXT_SIZE_22, COLOR_RED, COLOR_DEMO_BACKGROUND);
        tPosY += TEXT_SIZE_22_HEIGHT;
    }
    aButtonCheckInterval++;
    if (aButtonCheckInterval >= BUTTON_CHECK_INTERVAL) {
        TouchPanel.rd_data();
        if (TouchPanel.mPressure > MIN_REASONABLE_PRESSURE) {
            TouchButtonBack->checkButton(TouchPanel.getXActual(), TouchPanel.getYActual(), true);
        }

    }
}
#endif

/**
 * @}
 */
