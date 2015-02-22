/**
 * TouchDSOGui.cpp
 * @brief contains the gui related and flow control functions for DSO.
 *
 * @date 29.03.2012
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 */

#include "TouchDSO.h"
#include "Pages.h"
#include "Chart.h" // for adjustIntWithScaleFactor()
#include "misc.h"

#include <string.h>
#include <stdlib.h> // for abs()
extern "C" {
#include "ff.h"
}

const char StringLow[] = "low";
const char StringMid[] = "mid";
const char StringHigh[] = "high";
const char StringSmall[] = "small";
const char StringLarge[] = "large";

const char StringOn[] = "on";
const char StringOff[] = "off";

const char StringContinue[] = "Continue";

/*
 * COLORS
 */

//Line colors
#define COLOR_DATA_PICKER COLOR_YELLOW

// GUI element colors
#define COLOR_GUI_CONTROL COLOR_RED
#define COLOR_GUI_TRIGGER COLOR_BLUE
#define COLOR_GUI_SOURCE_TIMEBASE RGB(0x00,0x90,0x00)
#define COLOR_GUI_DISPLAY_CONTROL COLOR_YELLOW

// for slightly visible sliders
#define COLOR_DSO_SLIDER RGB(0xD8,0xE8,0xD8)

/******************************************************
 * GUI
 ******************************************************/
#define SLIDER_VPICKER_X (25 * TEXT_SIZE_11_WIDTH)
#define SLIDER_VPICKER_INFO_X_SMALL (INFO_LEFT_MARGIN +(22 * TEXT_SIZE_11_WIDTH))
#define SLIDER_VPICKER_INFO_X_LARGE (INFO_LEFT_MARGIN +(28 * TEXT_SIZE_11_WIDTH))
#define SLIDER_TLEVEL_X (13 * TEXT_SIZE_11_WIDTH)

/**********************
 * Buttons
 *********************/
static TouchButton * TouchButtonBack;

static TouchButton * TouchButtonStartStopDSOMeasurement;

static TouchButton * TouchButtonAutoTriggerOnOff;
const char AutoTriggerButtonStringAuto[] = "Trigger auto";
const char AutoTriggerButtonStringManual[] = "Trigger man";
const char AutoTriggerButtonStringOff[] = "Trigger off";

static TouchButton * TouchButtonAutoRangeOnOff;
const char AutoRangeButtonStringAuto[] = "Range auto";
const char AutoRangeButtonStringManual[] = "Range man";

static TouchButton * TouchButtonAutoOffsetOnOff;
const char AutoOffsetButtonString0[] = "Offset 0V";
const char AutoOffsetButtonStringAuto[] = "Offset auto";
const char AutoOffsetButtonStringMan[] = "Offset man";

static TouchButton * TouchButtonDrawModeLinePixel;
const char DrawModeButtonStringLine[] = "Line";
const char DrawModeButtonStringPixel[] = "Pixel";

// still experimental
static TouchButton * TouchButtonDrawModeTriggerLine;
const char DrawModeTriggerLineButtonString[] = "Trigg. line";

static TouchButton * TouchButtonSingleshot;
#define SINGLESHOT_PPRINT_VALUE_X (DISPLAY_WIDTH - TEXT_SIZE_11_WIDTH - 1)

static TouchButton * TouchButtonSlope;
char SlopeButtonString[] = "Slope \xD1"; // ascending
// the index of the slope indicator in char array
#define SLOPE_STRING_INDEX 6

static TouchButton *TouchButtonADS7846TestOnOff;
char StringSettingsButtonStringADS7846Test[] = "ADS7846";

static TouchButton *TouchButtonInfoSize;
char SettingsButtonStringInfoSize[] = "Info      ";
#define SettingsButtonStringInfoSizeChangeIndex 5

static TouchButton *TouchButtonChartHistory;
char ChartHistoryButtonString[] = "Hist     ";
#define ChartHistoryButtonStringChangeIndex 5
const char * const ChartHistoryButtonStrings[DRAW_HISTORY_LEVELS] = { StringOff, StringLow, StringMid, StringHigh };

static TouchButton * TouchButtonLoad;
static TouchButton * TouchButtonStore;
#define MODE_LOAD 0
#define MODE_STORE 1
static TouchButton * TouchButtonFFT;

static TouchButton * TouchButtonChannelSelect;

static TouchButton * TouchButtonDSOSettings;

static TouchButton * TouchButtonDSOMoreSettings;

static TouchButton * TouchButtonCalibrateVoltage;

static TouchButton * TouchButtonACRangeOnOff;

static TouchButton * TouchButtonShowPretriggerValuesOnOff;

static TouchButton ** TouchButtonsDSO[] = { &TouchButtonBack, &TouchButtonStartStopDSOMeasurement, &TouchButtonAutoTriggerOnOff,
        &TouchButtonAutoRangeOnOff, &TouchButtonAutoOffsetOnOff, &TouchButtonChannelSelect, &TouchButtonDrawModeLinePixel,
        &TouchButtonDrawModeTriggerLine, &TouchButtonDSOSettings, &TouchButtonDSOMoreSettings, &TouchButtonSingleshot,
        &TouchButtonSlope, &TouchButtonADS7846TestOnOff, &TouchButtonInfoSize, &TouchButtonChartHistory, &TouchButtonLoad,
        &TouchButtonStore, &TouchButtonFFT, &TouchButtonCalibrateVoltage, &TouchButtonACRangeOnOff,
        &TouchButtonShowPretriggerValuesOnOff };

static TouchSlider TouchSliderTriggerLevel;

static TouchSlider TouchSliderVoltagePicker;
uint8_t LastPickerValue;

/**********************************
 * Input channels
 **********************************/
const char StringChannel2[] = "Channel 2";
const char StringChannel3[] = "Channel 3";
const char StringChannel4[] = "Channel 4";
const char StringTemperature[] = "Temp";
const char StringVBattDiv2[] = "VBatt/2";
const char StringVRefint[] = "VRefint";
const char * const ADCInputMUXChannelStrings[ADC_CHANNEL_COUNT] = { StringChannel2, StringChannel3, StringChannel4,
        StringTemperature, StringVBattDiv2, StringVRefint };
char ADCInputMUXChannelChars[ADC_CHANNEL_COUNT] = { '1', '2', '3', 'T', 'B', 'R' };
uint8_t ADCInputMUXChannels[ADC_CHANNEL_COUNT] = { ADC1_INPUT1_CHANNEL, ADC1_INPUT2_CHANNEL, ADC1_INPUT3_CHANNEL, 0x10, 0x11, 0x12 };

/***********************
 *   Loop control
 ***********************/
#define MILLIS_BETWEEN_INFO_OUTPUT ONE_SECOND

/*******************************************************************************************
 * Function declaration section
 *******************************************************************************************/

//void readADS7846Channels(void);
//void acquireDataByDMA(void);
//bool computeTriggerAndOffsetAndRange(void);
void initDSOGUI(void);
//void activateCommonPartOfGui(void);
//void activateAnalysisOnlyPartOfGui(void);
//void activateRunningOnlyPartOfGui(void);
void drawCommonPartOfGui(void);
void drawAnalysisOnlyPartOfGui(void);
//void drawRunningOnlyPartOfGui(void);
void drawDSOSettingsPageGui(void);
void drawDSOMoreSettingsPageGui(void);
void showDSOSettingsPage(void);
void redrawDisplay(bool doClearbefore);
void redrawDisplay(void);
void longTouchDownHandlerDSO(struct XYPosition * const);
void swipeEndHandlerDSO(struct Swipe * const aSwipeInfo);
void TouchUpHandler(struct XYPosition * const aTochPosition);
//void doInfoSize(TouchButton * const aTheTouchedButton, int16_t aValue);
//void doTestTimeBase(TouchButton * const aTheTouchedButton, int16_t aValue);
//void doTriggerMode(TouchButton * const aTheTouchedButton, int16_t aValue);
//void doTriggerSlope(TouchButton * const aTheTouchedButton, int16_t aValue);
//void doAutoOffsetOnOff(TouchButton * const aTheTouchedButton, int16_t aValue);
//void doADCReference(TouchButton * const aTheTouchedButton, int16_t aValue);
//void doChannelSelect(TouchButton * const aTheTouchedButton, int16_t aValue);
//void doDrawMode(TouchButton * const aTheTouchedButton, int16_t aValue);
//void doDisplayMode(TouchButton * const aTheTouchedButton, int16_t aValue);
//void doSettings(TouchButton * const aTheTouchedButton, int16_t aValue);
//void doTriggerSingleshot(TouchButton * const aTheTouchedButton, int16_t aValue);
//void doStartStopDSO(TouchButton * const aTheTouchedButton, int16_t aValue);
//void doChartHistory(TouchButton * const aTheTouchedButton, int16_t aValue);
//void doADS7846Test(TouchButton * const aTheTouchedButton, int16_t aValue);
//static void doStoreLoadAcquisitionData(TouchButton * const aTheTouchedButton, int16_t aMode);
//void doRangeMode(TouchButton * const aTheTouchedButton, int16_t aValue);

//uint16_t doTriggerLevel(TouchSlider * const aTheTouchedSlider, const uint16_t aValue);
//uint16_t doVoltagePicker(TouchSlider * const aTheTouchedSlider, const uint16_t aValue);

/*******************************************************************************************
 * Program code starts here
 * Setup section
 *******************************************************************************************/
void initDSO(void) {
    ADC_init();
    // cannot be done statically
    DisplayControl.EraseColors[0] = COLOR_BACKGROUND_DSO;
    DisplayControl.EraseColors[1] = COLOR_DATA_ERASE_LOW;
    DisplayControl.EraseColors[2] = COLOR_DATA_ERASE_MID;
    DisplayControl.EraseColors[3] = COLOR_DATA_ERASE_HIGH;
    DisplayControl.ShowFFT = false;
    DisplayControl.DatabufferPreTriggerDisplaySize = (2 * DATABUFFER_DISPLAY_RESOLUTION);
    initAcquisition();
}

void initCaptions(void) {

    strlcpy(&SettingsButtonStringInfoSize[SettingsButtonStringInfoSizeChangeIndex], StringLarge, sizeof StringLarge);
    TouchButtonInfoSize->setCaption(SettingsButtonStringInfoSize);

    strlcpy(&ChartHistoryButtonString[ChartHistoryButtonStringChangeIndex], StringOff, sizeof StringOff);
    TouchButtonChartHistory->setCaption(ChartHistoryButtonString);

}

void startDSO(void) {

    resetAcquisition();

    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DSO);
#ifdef LOCAL_DISPLAY_EXISTS
    setDimDelayMillis(3 * BACKLIGHT_DIM_DEFAULT_DELAY);
#endif
    DisplayControl.DisplayBufferDrawMode = DRAW_MODE_LINE;

    DisplayControl.EraseColor = DisplayControl.EraseColors[0];
    DisplayControl.EraseColorIndex = 0;
    DisplayControl.XScale = 0;
    DisplayControl.DisplayIncrementPixel = DATABUFFER_DISPLAY_INCREMENT; // == getXScaleCorrectedValue(DATABUFFER_DISPLAY_INCREMENT);

    DisplayControl.ShowFFT = false; // before initDSOGUI()
    initDSOGUI();
    initCaptions();

    // show start elements
    drawCommonPartOfGui();
    drawAnalysisOnlyPartOfGui();
    registerSimpleResizeAndReconnectCallback(&redrawDisplay);

    registerLongTouchDownCallback(&longTouchDownHandlerDSO, TOUCH_STANDARD_LONG_TOUCH_TIMEOUT_MILLIS);
    registerSwipeEndCallback(&swipeEndHandlerDSO);
    // use touch up for buttons in order not to interfere with long touch
    registerTouchUpCallback(&TouchUpHandler);
    // use touch down only for sliders
    registerTouchDownCallback(&simpleTouchDownHandlerOnlyForSlider);
}

void stopDSO(void) {
    DSO_setAttenuator(DSO_ATTENUATOR_SHORTCUT);

    // only here
    ADC_stopTimer6();

    // in case of exiting during a running acquisition
    ADC_StopConversion(DSO_ADC_ID );
    ADC_disableEOCInterrupt(DSO_ADC_ID );

    // free buttons
    for (unsigned int i = 0; i < sizeof(TouchButtonsDSO) / sizeof(TouchButtonsDSO[0]); ++i) {
        (*TouchButtonsDSO[i])->setFree();
    }
    registerLongTouchDownCallback(NULL, 0);
    registerSwipeEndCallback(NULL);
    registerTouchUpCallback(NULL);
    // restore old touch down handler
    registerTouchDownCallback(&simpleTouchDownHandler);
#ifdef LOCAL_DISPLAY_EXISTS
    setDimDelayMillis(BACKLIGHT_DIM_DEFAULT_DELAY);
#endif
}

/************************************************************************
 * main loop - 32 microseconds
 ************************************************************************/
void loopDSO(void) {
    uint32_t tMillis, tMillisOfLoop;

    tMillis = getMillisSinceBoot();
    tMillisOfLoop = tMillis - MillisLastLoop;
    MillisLastLoop = tMillis;
    MillisSinceLastAction += tMillisOfLoop;

    if (MeasurementControl.isRunning) {
#ifdef LOCAL_DISPLAY_EXISTS
        if (MeasurementControl.ADS7846ChannelsAsDatasource) {
            readADS7846Channels();
            // check if button pressed - to process stop and channel buttons here
            TouchPanel.rd_data();
            if (TouchPanel.mPressure > MIN_REASONABLE_PRESSURE) {
                TouchButton::checkAllButtons(TouchPanel.getXActual(), TouchPanel.getYActual(), true);
            }
        }
#endif
        if (DataBufferControl.DataBufferFull) {
            /*
             * Data (from InterruptServiceRoutine or DMA) is ready
             */
            computeMinMaxAverageAndPeriodFrequency();
            if (!(MeasurementControl.TimebaseFastDMAMode || DataBufferControl.DrawWhileAcquire)) {
                // align pre trigger buffer after acquisition
                adjustPreTriggerBuffer(FourDisplayLinesBuffer);
            }
            if (MeasurementControl.StopRequested) {
                if (DataBufferControl.DataBufferEndPointer == &DataBufferControl.DataBuffer[DATABUFFER_DISPLAY_END]
                        && !MeasurementControl.StopAcknowledged) {
                    // Stop requested, but DataBufferEndPointer (for ISR) has the value of a regular acquisition or dma has not realized stop
                    // -> start new last acquisition
                    startAcquisition();
                } else {
                    /*
                     * do stop handling here (in thread mode)
                     */
                    MeasurementControl.StopRequested = false;
                    MeasurementControl.isRunning = false;
                    MeasurementControl.isSingleShotMode = false;
                    // delayed tone for stop
                    FeedbackToneOK();
                    DisplayControl.showInfoMode = LONG_INFO;
                    TouchButtonFFT->setRedGreenButtonColor(false); // leads to DisplayControl.ShowFFT = false;

                    DisplayControl.DisplayBufferDrawMode = DRAW_MODE_LINE;
                    // draw grid lines and gui
                    redrawDisplay();
                }
            } else {
                if (MeasurementControl.ChangeRequestedFlags & CHANGE_REQUESTED_TIMEBASE) {
                    changeTimeBase(false);
                    MeasurementControl.ChangeRequestedFlags = 0;
                }
                /*
                 * Normal loop-> process data, draw new chart, and start next acquisition
                 */
                int tLastTriggerDisplayValue = DisplayControl.TriggerLevelDisplayValue;
                computeAutoTrigger();
                computeAutoInputRange();
                computeAutoDisplayRange();
                // handle trigger line
                DisplayControl.TriggerLevelDisplayValue = getDisplayFrowRawInputValue(MeasurementControl.RawTriggerLevel);
                if (tLastTriggerDisplayValue
                        != DisplayControl.TriggerLevelDisplayValue&& MeasurementControl.TriggerMode != TRIGGER_MODE_OFF) {
                    clearTriggerLine(tLastTriggerDisplayValue);
                    drawTriggerLine();
                }

                if (!DataBufferControl.DrawWhileAcquire) {
                    // normal mode => clear old chart and draw new data
                    drawDataBuffer(DataBufferControl.DataBufferDisplayStart, DSO_DISPLAY_WIDTH, COLOR_DATA_RUN,
                            DisplayControl.EraseColor);
                    draw128FFTValuesFast(COLOR_FFT_DATA, DataBufferControl.DataBufferDisplayStart);
                }
                startAcquisition();
            }
        }
        if (DataBufferControl.DrawWhileAcquire) {
            /*
             * Draw while acquire mode
             */
            if (MeasurementControl.ChangeRequestedFlags & CHANGE_REQUESTED_TIMEBASE) {
                // do it here and not at end of display
                ADC_StopConversion(DSO_ADC_ID );
                changeTimeBase(false);
                MeasurementControl.ChangeRequestedFlags = 0;
                clearDiplayedChart();
                ADC_StartConversion(DSO_ADC_ID );
            }
            // detect end of pre trigger phase and adjust pre trigger buffer during acquisition
            if (MeasurementControl.TriggerPhaseJustEnded) {
                MeasurementControl.TriggerPhaseJustEnded = false;
                adjustPreTriggerBuffer(FourDisplayLinesBuffer);
                DataBufferControl.DataBufferNextDrawPointer = &DataBufferControl.DataBuffer[DATABUFFER_DISPLAY_START];
                DataBufferControl.NextDrawXValue = 0;
            }
            // check if new data available and draw in corresponding color
            if (MeasurementControl.TriggerActualPhase < PHASE_POST_TRIGGER) {
                if (DataBufferControl.DataBufferPreTriggerAreaWrapAround) {
                    // wrap while searching trigger -> start from beginning of buffer
                    DataBufferControl.DataBufferPreTriggerAreaWrapAround = false;
                    DataBufferControl.NextDrawXValue = 0;
                    DataBufferControl.DataBufferNextDrawPointer = &DataBufferControl.DataBuffer[0];
                }
                drawRemainingDataBufferValues(COLOR_DATA_PRETRIGGER);
            } else {
                drawRemainingDataBufferValues(COLOR_DATA_RUN);
            }
        }
        /*
         * Display pre trigger data for single shot mode
         */
        if (MeasurementControl.isSingleShotMode && MeasurementControl.TriggerActualPhase < PHASE_POST_TRIGGER
                && MillisSinceLastAction > MILLIS_BETWEEN_INFO_OUTPUT) {
            // output actual values for single shot every second
            if (!DataBufferControl.DrawWhileAcquire) {
                if (MeasurementControl.TimebaseFastDMAMode) {
                    // copy data in ISR, otherwise it might be overwritten more than once while copying here
                    MeasurementControl.doPretriggerCopyForDisplay = true;
                    // wait for ISR to finish copy
                    while (MeasurementControl.doPretriggerCopyForDisplay) {
                        ;
                    }
                } else {
                    memcpy(&FourDisplayLinesBuffer[0], &DataBufferControl.DataBuffer[0],
                            DATABUFFER_PRE_TRIGGER_SIZE * sizeof(DataBufferControl.DataBuffer[0]));
                }
                drawDataBuffer(&FourDisplayLinesBuffer[0], DATABUFFER_PRE_TRIGGER_SIZE, COLOR_DATA_PRETRIGGER,
                        DisplayControl.EraseColors[0]);
            }
            printInfo();
            MillisSinceLastAction = 0;
        }
    } // running mode end

    /**
     * do every second:
     * - refresh screen
     * - restore grid
     * - restore buttons
     * - print info
     * - show time
     */
    if (MillisSinceLastAction > MILLIS_BETWEEN_INFO_OUTPUT) {
        MillisSinceLastAction = 0;
        if (MeasurementControl.isRunning) {
            if (DisplayControl.DisplayPage == CHART) {
                drawGridLinesWithHorizLabelsAndTriggerLine(COLOR_GRID_LINES);
                if (DisplayControl.showInfoMode == LONG_INFO) {
                    printInfo();
                }
            } else if (DisplayControl.DisplayPage == SETTINGS) {
                // refresh buttons
                drawDSOSettingsPageGui();
            } else if (DisplayControl.DisplayPage == MORE_SETTINGS) {
                // refresh buttons
                drawDSOMoreSettingsPageGui();
            }

        } else if (DisplayControl.DisplayPage == CHART && DisplayControl.showInfoMode == LONG_INFO) {
            // show time
            if ((MeasurementControl.InfoSizeSmall)) {
                showRTCTimeEverySecond(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + 2 * TEXT_SIZE_11_HEIGHT, COLOR_RED,
                        COLOR_BACKGROUND_DSO);
            } else {
                showRTCTimeEverySecond(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + 3 * TEXT_SIZE_11_HEIGHT, COLOR_RED,
                        COLOR_BACKGROUND_DSO);
            }
        }
    }
    checkAndHandleEvents();
}
/* Main loop end */

bool checkDatabufferPointerForDrawing(void) {
    bool tReturn = true;
    // check for DataBufferDisplayStart
    if (DataBufferControl.DataBufferDisplayStart
            > DataBufferControl.DataBufferEndPointer - (adjustIntWithScaleFactor(DSO_DISPLAY_WIDTH, DisplayControl.XScale) - 1)) {
        DataBufferControl.DataBufferDisplayStart = (uint16_t *) DataBufferControl.DataBufferEndPointer
                - (adjustIntWithScaleFactor(DSO_DISPLAY_WIDTH, DisplayControl.XScale) - 1);
        // Check begin - if no screen full of data acquired (by forced stop)
        if ((DataBufferControl.DataBufferDisplayStart < &DataBufferControl.DataBuffer[0])) {
            DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[0];
        }
        tReturn = false;
    }
    return tReturn;
}

/**
 * Changes timebase
 * sometimes interrupts just stops after the first reading because of setting
 * ADC_SetClockPrescaler per interrupt here, so delay it to the start of a new acquisition
 *
 * @param aValue
 * @return
 */
int changeTimeBaseValue(int aChangeValue) {
    uint8_t tFeedbackType = FEEDBACK_TONE_NO_ERROR;
    int tOldIndex = MeasurementControl.TimebaseIndex;
    // positive value means increment timebase index!
    int tNewIndex = tOldIndex + aChangeValue;
    // do not decrement below 1 or increment above TIMEBASE_NUMBER_OF_ENTRIES -1
    // skip 1. entry because it makes no sense yet (wait until interleaving acquisition is implemented :-))
    if (tNewIndex < 1) {
        tNewIndex = 1;
    }
    if (tNewIndex > TIMEBASE_NUMBER_OF_ENTRIES - 1) {
        tNewIndex = TIMEBASE_NUMBER_OF_ENTRIES - 1;
    }
    if (tNewIndex == tOldIndex) {
        tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
    } else {
        // signal to main loop in thread mode
        MeasurementControl.TimebaseNewIndex = tNewIndex;
        MeasurementControl.ChangeRequestedFlags |= CHANGE_REQUESTED_TIMEBASE;
    }
    return tFeedbackType;
}

/**
 * Handler for input range up / down
 */
int changeRangeHandler(int aValue) {
    int tFeedbackType = FEEDBACK_TONE_NO_ERROR;
    if (!changeInputRange(aValue)) {
        tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
    }
    return tFeedbackType;
}

/**
 * Handler for offset up / down
 */
int changeOffsetGridCount(int aValue) {
    setOffsetGridCount(MeasurementControl.OffsetGridCount + aValue);
    drawGridLinesWithHorizLabelsAndTriggerLine(COLOR_GRID_LINES);
    return FEEDBACK_TONE_NO_ERROR;
}

/*
 * changes x resolution and draws data - only for analyze mode
 */
int changeXScale(int aValue) {
    int tFeedbackType = FEEDBACK_TONE_NO_ERROR;
    DisplayControl.XScale += aValue;

    if (DisplayControl.XScale < -DATABUFFER_DISPLAY_RESOLUTION_FACTOR) {
        tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
        DisplayControl.XScale = -DATABUFFER_DISPLAY_RESOLUTION_FACTOR;
    }
    DisplayControl.DisplayIncrementPixel = adjustIntWithScaleFactor(DATABUFFER_DISPLAY_INCREMENT, DisplayControl.XScale);
    printInfo();

    // Check end
    if (!checkDatabufferPointerForDrawing()) {
        tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
    }
    // do tone before draw
    FeedbackTone(tFeedbackType);
    tFeedbackType = FEEDBACK_TONE_NO_TONE;
    drawDataBuffer(DataBufferControl.DataBufferDisplayStart, DSO_DISPLAY_WIDTH, COLOR_DATA_HOLD, DisplayControl.EraseColors[0]);

    return tFeedbackType;
}

/**
 * set start of display in data buffer and draws data - only for analyze mode
 * @param aValue number of DisplayControl.DisplayIncrementPixel to scroll
 */
int scrollDisplay(int aValue) {
    uint8_t tFeedbackType = FEEDBACK_TONE_NO_ERROR;
    if (DisplayControl.DisplayPage == CHART) {
        DataBufferControl.DataBufferDisplayStart += aValue * DisplayControl.DisplayIncrementPixel;
        // Check begin
        if ((DataBufferControl.DataBufferDisplayStart < &DataBufferControl.DataBuffer[0])) {
            DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[0];
            tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
        }
        // Check end
        if (!checkDatabufferPointerForDrawing()) {
            tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
        }
        // delete old graph and draw new one
        drawDataBuffer(DataBufferControl.DataBufferDisplayStart, DSO_DISPLAY_WIDTH, COLOR_DATA_HOLD, DisplayControl.EraseColors[0]);
    }
    return tFeedbackType;
}

/*
 * Handler for "empty" touch
 * Use touch up in order not to interfere with long touch
 * Switch between upper info line short/long/off
 */
void TouchUpHandler(struct XYPosition * const aTouchPosition) {
    // first check for buttons
    if (!TouchButton::checkAllButtons(aTouchPosition->PosX, aTouchPosition->PosY, true)) {
        if (DisplayControl.DisplayPage == CHART) {
// Wrap display mode
            uint8_t tNewMode = DisplayControl.showInfoMode + 1;
            // Wrap display mode
            if (tNewMode > LONG_INFO) {
                tNewMode = NO_INFO;
                // erase former (short) info line
                clearInfo();
            }
            DisplayControl.showInfoMode = static_cast<InfoModeEnum>(tNewMode);
            if (tNewMode != NO_INFO) {
                printInfo();
            } else if (!MeasurementControl.isRunning) {
                redrawDisplay();
            }
        }
    }
}

/**
 * Shows gui on long touch
 */
void longTouchDownHandlerDSO(struct XYPosition * const aTouchPosition) {
    if (DisplayControl.DisplayPage == CHART) {
        if (MeasurementControl.isRunning) {
            // Show settings page
            drawDSOSettingsPageGui();
        } else {
            // clear screen and show only gui
            DisplayControl.DisplayPage = START;
            redrawDisplay(true);
        }
    } else if (DisplayControl.DisplayPage == START) {
        // only chart (and voltage picker)
        DisplayControl.DisplayPage = CHART;
        redrawDisplay(true);
    }
}

/**
 * responsible for swipe detection and dispatching
 */
void swipeEndHandlerDSO(struct Swipe * const aSwipeInfo) {
    // we have only vertical sliders so allow horizontal swipes even if starting at slider position
    if (aSwipeInfo->TouchDeltaAbsMax < TIMING_GRID_WIDTH) {
        return;
    }
    if (DisplayControl.DisplayPage == CHART && aSwipeInfo->TouchDeltaAbsMax >= TIMING_GRID_WIDTH) {
        bool tDeltaGreaterThanOneGrid = (aSwipeInfo->TouchDeltaAbsMax / (2 * TIMING_GRID_WIDTH)) != 0; // otherwise functions below are called with zero
        int tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
        int tTouchDeltaXGrid = aSwipeInfo->TouchDeltaX / TIMING_GRID_WIDTH;
        if (MeasurementControl.isRunning) {
            /*
             * Running mode
             */
            if (aSwipeInfo->SwipeMainDirectionIsX) {
                // Horizontal swipe -> timebase
                if (tDeltaGreaterThanOneGrid) {
                    tFeedbackType = changeTimeBaseValue(-tTouchDeltaXGrid / 2);
                }
            } else {
                // Vertical swipe
                int tTouchDeltaYGrid = aSwipeInfo->TouchDeltaY / TIMING_GRID_WIDTH;
                if ((!MeasurementControl.RangeAutomatic && MeasurementControl.OffsetMode == OFFSET_MODE_AUTOMATIC)
                        || MeasurementControl.OffsetMode == OFFSET_MODE_MANUAL) {
                    // decide which swipe to perform according to x position of swipe
                    if (aSwipeInfo->TouchStartX > BUTTON_WIDTH_3_POS_2) {
                        //offset
                        tFeedbackType = changeOffsetGridCount(tTouchDeltaYGrid);
                    } else if (tDeltaGreaterThanOneGrid) {
                        //range manual has priority if range manual AND OFFSET_MODE_MANUAL selected
                        if (!MeasurementControl.RangeAutomatic) {
                            tFeedbackType = changeRangeHandler(tTouchDeltaYGrid / 2);
                        } else {
                            tFeedbackType = changeDisplayRange(tTouchDeltaYGrid / 2);
                        }
                    }
                } else if (!MeasurementControl.RangeAutomatic) {
                    //range manual
                    if (tDeltaGreaterThanOneGrid) {
                        tFeedbackType = changeRangeHandler(tTouchDeltaYGrid / 2);
                    }
                } else if (MeasurementControl.OffsetMode != OFFSET_MODE_0_VOLT) {
                    //offset mode != 0 Volt
                    tFeedbackType = changeOffsetGridCount(tTouchDeltaYGrid);
                }
            }
        } else {
            /*
             * Analyze Mode
             */
            if (aSwipeInfo->TouchStartY > BUTTON_HEIGHT_4_LINE_4) {
                tFeedbackType = scrollDisplay(-tTouchDeltaXGrid);
            } else {
                // scale
                if (tDeltaGreaterThanOneGrid) {
                    tFeedbackType = changeXScale(tTouchDeltaXGrid / 2);
                }
            }
        }
        FeedbackTone(tFeedbackType);
    }
}

/************************************************************************
 * GUI handler section
 ************************************************************************/
/*
 * step from automatic to manual trigger voltage value to trigger off
 */
void doTriggerMode(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    MeasurementControl.TriggerMode++;
    if (MeasurementControl.TriggerMode > TRIGGER_MODE_OFF) {
        MeasurementControl.TriggerMode = TRIGGER_MODE_AUTOMATIC;
        aTheTouchedButton->setCaption(AutoTriggerButtonStringAuto);
    } else if (MeasurementControl.TriggerMode == TRIGGER_MODE_MANUAL) {
        aTheTouchedButton->setCaption(AutoTriggerButtonStringManual);
    } else {
        // Trigger off
        aTheTouchedButton->setCaption(AutoTriggerButtonStringOff);
    }
    aTheTouchedButton->drawButton();
}

void doRangeMode(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    MeasurementControl.RangeAutomatic = !MeasurementControl.RangeAutomatic;
    if (MeasurementControl.RangeAutomatic) {
        aTheTouchedButton->setCaption(AutoRangeButtonStringAuto);
    } else {
        aTheTouchedButton->setCaption(AutoRangeButtonStringManual);
    }
    aTheTouchedButton->drawButton();
}

/*
 * toggle between ascending and descending trigger slope
 */
void doTriggerSlope(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    MeasurementControl.TriggerSlopeRising = (!MeasurementControl.TriggerSlopeRising);
    uint8_t tChar = 0xD2; //descending
    if (MeasurementControl.TriggerSlopeRising) {
        tChar = 0xD1; //ascending
    }
    SlopeButtonString[SLOPE_STRING_INDEX] = tChar;
    aTheTouchedButton->setCaption(SlopeButtonString);

    aTheTouchedButton->drawCaption();
}

/*
 * step from 0 Volt  to auto to manual offset
 */
void doOffsetMode(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    MeasurementControl.OffsetMode++;
    if (MeasurementControl.OffsetMode > OFFSET_MODE_MANUAL) {
        MeasurementControl.OffsetMode = OFFSET_MODE_0_VOLT;
        aTheTouchedButton->setCaption(AutoOffsetButtonString0);
        setACMode(MeasurementControl.isACMode);
        MeasurementControl.DisplayRangeIndex = MeasurementControl.InputRangeIndex;
        MeasurementControl.InputRangeIndexOtherThanDisplayRange = false;
    } else if (MeasurementControl.OffsetMode == OFFSET_MODE_AUTOMATIC) {
        aTheTouchedButton->setCaption(AutoOffsetButtonStringAuto);
    } else {
        aTheTouchedButton->setCaption(AutoOffsetButtonStringMan);
    }
    aTheTouchedButton->drawButton();
}

void doACModeOnOff(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    if (isAttenuatorAvailable) {
        aValue = !aValue;
        setACMode(aValue);
        if (aValue) {
            aTheTouchedButton->setCaption(StringPlusMinus);
        } else {
            aTheTouchedButton->setCaption(StringPlus);
        }
        aTheTouchedButton->setValue(aValue); // next value parameter for doACRange
        aTheTouchedButton->drawCaption();
    }
}

/*
 * Cycle through all external and internal adc channels
 */
void doChannelSelect(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    MeasurementControl.ADCInputMUXChannelIndex++;
#ifdef LOCAL_DISPLAY_EXISTS
    if (MeasurementControl.ADS7846ChannelsAsDatasource) {
        // ADS7846 channels
        if (MeasurementControl.ADCInputMUXChannelIndex >= ADS7846_CHANNEL_COUNT) {
            // wrap to first channel connected to attenuator and restore ACRange
            MeasurementControl.ADCInputMUXChannelIndex = 0;
            MeasurementControl.isACMode = DSO_getACMode();
        }
        aTheTouchedButton->setCaption(ADS7846ChannelStrings[MeasurementControl.ADCInputMUXChannelIndex]);

    } else {
#endif
        if (MeasurementControl.ADCInputMUXChannelIndex >= ADC_CHANNEL_COUNT) {
            // wrap to first channel and set AttenuatorAvailable
            MeasurementControl.ADCInputMUXChannelIndex = 0;
            isAttenuatorAvailable = true;
        }
        // compare with == because value is only incremented by 1
        if (MeasurementControl.ADCInputMUXChannelIndex == ADC_CHANNEL_NO_ATTENUATOR_INDEX) {
            // no attenuator available -> switch to dc range for correct display
            doACModeOnOff(TouchButtonACRangeOnOff, true);
            isAttenuatorAvailable = false;
            setInputRange(NO_ATTENUATOR_MAX_RANGE_INDEX);
        }
        ADC_SetChannelSampleTime(DSO_ADC_ID, ADCInputMUXChannels[MeasurementControl.ADCInputMUXChannelIndex],
                MeasurementControl.TimebaseFastDMAMode);

// set channel number in caption
        aTheTouchedButton->setCaption(ADCInputMUXChannelStrings[MeasurementControl.ADCInputMUXChannelIndex]);
#ifdef LOCAL_DISPLAY_EXISTS
    }
#endif
    if (DisplayControl.DisplayPage == SETTINGS) {
        aTheTouchedButton->drawButton();
    } else {
        DisplayControl.showInfoMode = LONG_INFO;
        clearInfo();
        printInfo();
    }
}

/*
 * Toggle between pixel and line draw mode (for data chart)
 */
void doDrawMode(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
// erase old chart in old mode
    drawDataBuffer(NULL, DSO_DISPLAY_WIDTH, DisplayControl.EraseColor, 0);
// switch mode
    if (DisplayControl.DisplayBufferDrawMode & DRAW_MODE_LINE) {
        DisplayControl.DisplayBufferDrawMode &= ~DRAW_MODE_LINE;
        aTheTouchedButton->setCaption(DrawModeButtonStringPixel);
    } else {
        aTheTouchedButton->setCaption(DrawModeButtonStringLine);
        DisplayControl.DisplayBufferDrawMode = DRAW_MODE_LINE;
    }
    // show new caption
    aTheTouchedButton->drawButton();
}

/*
 * Switch trigger line mode on and off (for data chart)
 */
void doDrawModeTriggerLine(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    aValue = !aValue;

// switch mode
    if (aValue) {
        // erase old chart in old mode
        drawDataBuffer(NULL, DSO_DISPLAY_WIDTH, DisplayControl.EraseColor, 0);
        DisplayControl.DisplayBufferDrawMode |= DRAW_MODE_TRIGGER;
    } else {
        DisplayControl.DisplayBufferDrawMode &= ~DRAW_MODE_TRIGGER;
    }
    aTheTouchedButton->setRedGreenButtonColorAndDraw(aValue);
}

void doShowPretriggerValuesOnOff(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    aValue = !aValue;
    DisplayControl.DatabufferPreTriggerDisplaySize = 0;
    if (aValue) {
        DisplayControl.DatabufferPreTriggerDisplaySize = (2 * DATABUFFER_DISPLAY_RESOLUTION);
    }
    aTheTouchedButton->setRedGreenButtonColorAndDraw(aValue);
}

/*
 * show gui of settings screen
 */
void doSettings(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    // show settings page
    showDSOSettingsPage();
}

/*
 * show gui of more settings screen
 */
void doMoreSettings(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    // show more settings page
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DSO);
    drawDSOMoreSettingsPageGui();
}

/*
 * handle back button on settings pages
 */
void doSettingsBack(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();

    if (DisplayControl.DisplayPage == SETTINGS) {
        DisplayControl.DisplayPage = CHART;
        // Back
        redrawDisplay();
    } else {
        // show settings page
        showDSOSettingsPage();
    }
}

/**
 * prepare values for running mode
 */
void prepareForStart(void) {
    DisplayControl.showInfoMode = LONG_INFO;
    MeasurementControl.TimestampLastRangeChange = 0; // enable direct range change at start

    // reset xScale to regular value
    if (MeasurementControl.TimebaseIndex < TIMEBASE_NUMBER_OF_XSCALE_CORRECTION) {
        DisplayControl.XScale = xScaleForTimebase[MeasurementControl.TimebaseIndex];
    } else {
        DisplayControl.XScale = 0;
    }

    startAcquisition();
    // must be after startAcquisition()
    MeasurementControl.isRunning = true;

    redrawDisplay();
}

/**
 * start singleshot aquisition
 */
void doStartSingleshot(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();

    // deactivate button
    aTheTouchedButton->deactivate();

// prepare info output - at least 1 sec later
    MillisSinceLastAction = 0;
    MeasurementControl.RawValueMax = 0;
    MeasurementControl.RawValueMin = 0;
    MeasurementControl.RawValueAverage = 0;
    MeasurementControl.isSingleShotMode = true;
    prepareForStart();
}

void doStartStopDSO(TouchButton * const aTheTouchedButton, int16_t aValue) {
    if (MeasurementControl.isRunning) {
        /*
         * Stop here
         * for the last measurement read full buffer size
         * Do this asynchronously to the interrupt routine in order to extend a running or started acquisition
         * stop single shot mode
         */
        // first extends end marker for ISR to end of buffer instead of end of display
        DataBufferControl.DataBufferEndPointer = &DataBufferControl.DataBuffer[DATABUFFER_SIZE - 1];
//		if (MeasurementControl.SingleShotMode) {
//			MeasurementControl.ActualPhase = PHASE_POST_TRIGGER;
//		}
        // in SingleShotMode stop is directly requested
        if (MeasurementControl.StopRequested && !MeasurementControl.isSingleShotMode) {
            // for stop requested 2 times -> stop immediately
            uint16_t * tEndPointer = DataBufferControl.DataBufferNextInPointer;
            DataBufferControl.DataBufferEndPointer = tEndPointer;
            // clear trailing buffer space not used
            memset(tEndPointer, DATABUFFER_INVISIBLE_RAW_VALUE,
                    ((uint8_t*) &DataBufferControl.DataBuffer[DATABUFFER_SIZE]) - ((uint8_t*) tEndPointer));
//            for (int i = &DataBufferControl.DataBuffer[DATABUFFER_SIZE] - tEndPointer; i > 0; --i) {
//                *tEndPointer++ = DATABUFFER_INVISIBLE_RAW_VALUE;
//            }
        }
        // return to continuous  mode
        MeasurementControl.isSingleShotMode = false;
        MeasurementControl.StopRequested = true;
        // set value for horizontal scrolling
        DisplayControl.DisplayIncrementPixel = adjustIntWithScaleFactor(DATABUFFER_DISPLAY_INCREMENT, DisplayControl.XScale);
        // no feedback tone, it kills the timing!
    } else {
        /*
         *  Start here in normal mode (reset single shot mode)
         */
        FeedbackToneOK();
        MeasurementControl.isSingleShotMode = false; // return to continuous  mode
        DisplayControl.DisplayPage = CHART;
        prepareForStart();
    }
}

/*
 * Cycle through 3 history modes ( => use 3 different erase colors)
 */
void doChartHistory(TouchButton * const aTheTouchedButton, int16_t aValue) {

    FeedbackToneOK();
    DisplayControl.EraseColorIndex++;
    if (DisplayControl.EraseColorIndex >= DRAW_HISTORY_LEVELS) {
        DisplayControl.EraseColorIndex = 0;
    }
    strlcpy(&ChartHistoryButtonString[ChartHistoryButtonStringChangeIndex],
            ChartHistoryButtonStrings[DisplayControl.EraseColorIndex],
            strlen(ChartHistoryButtonStrings[DisplayControl.EraseColorIndex]) + 1);
    aTheTouchedButton->setCaption(ChartHistoryButtonString);
    DisplayControl.EraseColor = DisplayControl.EraseColors[DisplayControl.EraseColorIndex];
    if (MeasurementControl.isRunning && DisplayControl.DisplayPage == CHART) {
        // clear history on screen
        redrawDisplay();
    } else {
        aTheTouchedButton->drawButton();
    }
}

#ifdef LOCAL_DISPLAY_EXISTS
/*
 * Toggles ADS7846Test on / off
 */
void doADS7846TestOnOff(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    aValue = !aValue;
    MeasurementControl.ADS7846ChannelsAsDatasource = aValue;
    MeasurementControl.ADCInputMUXChannelIndex = 0;
    if (aValue) {
        // ADS7846 Test on
        doACModeOnOff(TouchButtonACRangeOnOff, true);
        isAttenuatorAvailable = false;
        setInputRange(NO_ATTENUATOR_MAX_RANGE_INDEX);
    } else {
        isAttenuatorAvailable = true; // for channel index 0
    }
    aTheTouchedButton->setRedGreenButtonColorAndDraw(aValue);
}
#endif

/*
 * Toggles infosize small/large
 */
void doInfoSize(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    MeasurementControl.InfoSizeSmall = !MeasurementControl.InfoSizeSmall;
    if (MeasurementControl.InfoSizeSmall) {
        strlcpy(&SettingsButtonStringInfoSize[SettingsButtonStringInfoSizeChangeIndex], StringSmall, sizeof StringSmall);
    } else {
        strlcpy(&SettingsButtonStringInfoSize[SettingsButtonStringInfoSizeChangeIndex], StringLarge, sizeof StringLarge);
    }
    aTheTouchedButton->setCaption(SettingsButtonStringInfoSize);
    aTheTouchedButton->drawButton();
}

/**
 *
 * @param aTheTouchedSlider
 * @param aValue range is 0 to DISPLAY_VALUE_FOR_ZERO
 * @return
 */
uint16_t doTriggerLevel(TouchSlider * const aTheTouchedSlider, uint16_t aValue) {
// in auto-trigger mode show only computed value and do not modify it
    if (MeasurementControl.TriggerMode == TRIGGER_MODE_AUTOMATIC) {
        // computed value already shown :-)
        return aValue;
    }
// to get display value take DISPLAY_VALUE_FOR_ZERO - aValue and vice versa
    int tValue = DISPLAY_VALUE_FOR_ZERO - aValue;
    if (DisplayControl.TriggerLevelDisplayValue == tValue) {
        // no change -> return
        return aValue;
    }

// clear old trigger line
    clearTriggerLine(DisplayControl.TriggerLevelDisplayValue);

// store actual display value
    DisplayControl.TriggerLevelDisplayValue = tValue;

    if (MeasurementControl.TriggerMode == TRIGGER_MODE_MANUAL) {
        // modify trigger values according to display value
        int tRawLevel = getInputRawFromDisplayValue(tValue);
        setTriggerLevelAndHysteresis(tRawLevel, TRIGGER_HYSTERESIS_MANUAL);
    }
// draw new line
    drawTriggerLine();
// print value
    printTriggerInfo();
    return DISPLAY_VALUE_FOR_ZERO - tValue;
}

/*
 * The value printed has a resolution of 0,00488 * scale factor
 */
uint16_t doVoltagePicker(TouchSlider * const aTheTouchedSlider, uint16_t aValue) {
    if (LastPickerValue == aValue) {
        return aValue;
    }
    if (LastPickerValue != 0xFF) {
        // clear old line
        int tYpos = DISPLAY_VALUE_FOR_ZERO - LastPickerValue;
        BlueDisplay1.drawLine(0, tYpos, DSO_DISPLAY_WIDTH - 1, tYpos, COLOR_BACKGROUND_DSO);
        if (DisplayControl.showInfoMode == LONG_INFO) {
            // restore grid at old y position
            for (int tXPos = TIMING_GRID_WIDTH - 1; tXPos < DSO_DISPLAY_WIDTH - 1; tXPos += TIMING_GRID_WIDTH) {
                BlueDisplay1.drawPixel(tXPos, tYpos, COLOR_GRID_LINES);
            }
        }
        if (!MeasurementControl.isRunning) {
            // restore graph
            uint8_t* ScreenBufferPointer = &DisplayBuffer[0];
            for (int i = 0; i < DSO_DISPLAY_WIDTH; ++i) {
                int tValueByte = *ScreenBufferPointer++;
                if (tValueByte == tYpos) {
                    BlueDisplay1.drawPixel(i, tValueByte, COLOR_DATA_HOLD);
                }
            }
        }
    }
// draw new line
    int tValue = DISPLAY_VALUE_FOR_ZERO - aValue;
    BlueDisplay1.drawLine(0, tValue, DSO_DISPLAY_WIDTH - 1, tValue, COLOR_DATA_PICKER);
    LastPickerValue = aValue;

    float tVoltage = getFloatFromDisplayValue(tValue);
    snprintf(StringBuffer, sizeof StringBuffer, "%6.*fV", RangePrecision[MeasurementControl.DisplayRangeIndex] + 1, tVoltage);

    int tYPos = INFO_UPPER_MARGIN + TEXT_SIZE_11_HEIGHT;
    if (!MeasurementControl.InfoSizeSmall) {
        tYPos += TEXT_SIZE_11_HEIGHT;
    }
    int tXPos = SLIDER_VPICKER_INFO_X_LARGE;
    if (MeasurementControl.InfoSizeSmall) {
        tXPos = SLIDER_VPICKER_INFO_X_SMALL;
    }

// print value
    BlueDisplay1.drawText(tXPos, tYPos, StringBuffer, TEXT_SIZE_11, COLOR_BLACK, COLOR_INFO_BACKGROUND);
    return aValue;
}

/**
 * 3ms for FFT, 9ms complete with -OS
 * @param aTheTouchedButton
 * @param aMode
 */
void doShowFFT(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    aValue = !aValue;
    DisplayControl.ShowFFT = aValue;
    aTheTouchedButton->setRedGreenButtonColor(aValue);

    if (MeasurementControl.isRunning) {
        if (DisplayControl.DisplayPage == SETTINGS) {
            aTheTouchedButton->drawButton();
        }
        if (aValue) {
            // initialize FFTDisplayBuffer
            memset(&DisplayBufferFFT[0], DSO_DISPLAY_HEIGHT - 1, sizeof(DisplayBufferFFT));
        } else {
            clearFFTValuesOnDisplay();
        }
    } else if (DisplayControl.DisplayPage == CHART) {
        if (aValue) {
            // compute and draw FFT
            drawFFT();
        } else {
            // show graph data
            redrawDisplay();
        }
    }
}

const char sDSODataFileName[] = "DSO-data.bin";
void doStoreLoadAcquisitionData(TouchButton * const aTheTouchedButton, int16_t aMode) {
    int tFeedbackType = FEEDBACK_TONE_LONG_ERROR;
    if (!MeasurementControl.isRunning && MICROSD_isCardInserted()) {
        FIL tFile;
        FRESULT tOpenResult;
        UINT tCount;

        if (aMode == MODE_LOAD) {
            // Load new data from file
            tOpenResult = f_open(&tFile, sDSODataFileName, FA_OPEN_EXISTING | FA_READ);
            if (tOpenResult == FR_OK) {
                f_read(&tFile, &MeasurementControl, sizeof(MeasurementControl), &tCount);
                f_read(&tFile, &DataBufferControl, sizeof(DataBufferControl), &tCount);
                f_close(&tFile);
                DisplayControl.showInfoMode = LONG_INFO;
                // redraw display corresponding to new values
                redrawDisplay();
                printInfo();
                tFeedbackType = FEEDBACK_TONE_NO_ERROR;
            }
        } else {
            // Store
            tOpenResult = f_open(&tFile, sDSODataFileName, FA_CREATE_ALWAYS | FA_WRITE);
            if (tOpenResult == FR_OK) {
                f_write(&tFile, &MeasurementControl, sizeof(MeasurementControl), &tCount);
                f_write(&tFile, &DataBufferControl, sizeof(DataBufferControl), &tCount);
                f_close(&tFile);
                tFeedbackType = FEEDBACK_TONE_NO_ERROR;
            }
        }
    }
    FeedbackTone(tFeedbackType);
}

void doVoltageCalibration(TouchButton * const aTheTouchedButton, int16_t aValue) {
    FeedbackToneOK();
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DSO);

    BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND, "Enter actual value", TEXT_SIZE_22,
            COLOR_RED, COLOR_BACKGROUND_DSO);
    float tOriginalValue = getFloatFromRawValue(MeasurementControl.RawValueAverage);
    snprintf(StringBuffer, sizeof StringBuffer, "Current=%fV", tOriginalValue);
    BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, 4 * TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND, StringBuffer, TEXT_SIZE_22,
            COLOR_BLACK, COLOR_INFO_BACKGROUND);

// wait for touch to become active
    do {
        checkAndHandleEvents();
        delayMillis(10);
    } while (!sTouchIsStillDown);

    FeedbackToneOK();
    float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, COLOR_GUI_SOURCE_TIMEBASE);
// check for cancel
    if (!isnan(tNumber) && tNumber != 0) {
        float tOldValue = RawAttenuationFactor[MeasurementControl.InputRangeIndex];
        float tNewValue = tOldValue * (tNumber / tOriginalValue);
        RawAttenuationFactor[MeasurementControl.InputRangeIndex] = tNewValue;
        initRawToDisplayFactors();

        BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DSO);
        snprintf(StringBuffer, sizeof StringBuffer, "Old value=%f", tOldValue);
        BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND, StringBuffer, TEXT_SIZE_22,
                COLOR_BLACK, COLOR_INFO_BACKGROUND);
        snprintf(StringBuffer, sizeof StringBuffer, "New value=%f", tNewValue);
        BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, 4 * TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND, StringBuffer, TEXT_SIZE_22,
                COLOR_BLACK, COLOR_INFO_BACKGROUND);

        // wait for touch to become active
        do {
            delayMillis(10);
            checkAndHandleEvents();
        } while (!sNothingTouched);

        sDisableTouchUpOnce = true;
        FeedbackToneOK();
    }

    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DSO);
    drawDSOMoreSettingsPageGui();
}

/***********************************************************************
 * GUI initialisation and drawing stuff
 ***********************************************************************/
void initDSOGUI(void) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"

    int tPosY = 0;

    /*
     * Start page
     */

    // 1. row
    // Button for Singleshot
    TouchButtonSingleshot = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            COLOR_GUI_CONTROL, "Singleshot", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doStartSingleshot);

    // standard main home button here

    // 2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    TouchButtonStore = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_5, BUTTON_HEIGHT_4, COLOR_GUI_SOURCE_TIMEBASE,
            StringStore, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, MODE_STORE, &doStoreLoadAcquisitionData);

    TouchButtonLoad = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_5_POS_2, tPosY, BUTTON_WIDTH_5, BUTTON_HEIGHT_4,
            COLOR_GUI_SOURCE_TIMEBASE, StringLoad, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, MODE_LOAD,
            &doStoreLoadAcquisitionData);

    // big start stop button
    TouchButtonStartStopDSOMeasurement = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3,
            (2 * BUTTON_HEIGHT_4) + BUTTON_DEFAULT_SPACING, COLOR_GUI_CONTROL, "Start/Stop", TEXT_SIZE_11,
            BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doStartStopDSO);

    // 4. row
    tPosY += 2 * BUTTON_HEIGHT_4_LINE_2;
    // Button for show FFT
    TouchButtonFFT = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_GREEN, "FFT",
            TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, DisplayControl.ShowFFT, &doShowFFT);
    TouchButtonFFT->setRedGreenButtonColor();

    // Button for settings pages
    TouchButtonDSOSettings = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            COLOR_GUI_CONTROL, StringSettings, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doSettings);

    /*
     * Settings page
     */
    // 1. row
    tPosY = 0;
    // Button for AC range
    TouchButtonACRangeOnOff = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3 - BUTTON_WIDTH_4, tPosY, BUTTON_WIDTH_4,
            BUTTON_HEIGHT_4, COLOR_GUI_TRIGGER, StringPlusMinus, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH,
            MeasurementControl.isACMode, &doACModeOnOff);
    if (!MeasurementControl.isACMode) {
        TouchButtonACRangeOnOff->setCaption(StringPlus);
    }

    // Button for auto trigger on off
    TouchButtonAutoTriggerOnOff = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
            BUTTON_HEIGHT_4, COLOR_GUI_TRIGGER, AutoTriggerButtonStringAuto, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0,
            &doTriggerMode);

    // Back button for both settings pages
    TouchButtonBack = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            COLOR_GUI_CONTROL, StringBack, TEXT_SIZE_22, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doSettingsBack);

    // 2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    // Button for slope page
    TouchButtonSlope = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3 - BUTTON_WIDTH_4, tPosY, BUTTON_WIDTH_4,
            BUTTON_HEIGHT_4, COLOR_GUI_TRIGGER, SlopeButtonString, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doTriggerSlope);

    // AutoRange on off
    TouchButtonAutoRangeOnOff = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            COLOR_GUI_TRIGGER, AutoRangeButtonStringAuto, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doRangeMode);

    // Button for auto offset on off
    TouchButtonAutoOffsetOnOff = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            COLOR_GUI_TRIGGER, AutoOffsetButtonString0, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doOffsetMode);

    // 3. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    // Button for pretrigger area show
    TouchButtonShowPretriggerValuesOnOff = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            COLOR_BLACK, "Pretrigger", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH,
            (DisplayControl.DatabufferPreTriggerDisplaySize != 0), &doShowPretriggerValuesOnOff);
    TouchButtonShowPretriggerValuesOnOff->setRedGreenButtonColor();

    // Button for channel select
    TouchButtonChannelSelect = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            COLOR_GUI_SOURCE_TIMEBASE, ADCInputMUXChannelStrings[MeasurementControl.ADCInputMUXChannelIndex], TEXT_SIZE_11,
            BUTTON_FLAG_DO_BEEP_ON_TOUCH, MeasurementControl.ADCInputMUXChannelIndex, &doChannelSelect);

    // 4. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    // Button for chart history (erase color)
    TouchButtonChartHistory = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            COLOR_GUI_DISPLAY_CONTROL, ChartHistoryButtonString, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doChartHistory);

    // Button for more-settings pages
    TouchButtonDSOMoreSettings = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            COLOR_GUI_CONTROL, StringMore, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doMoreSettings);

    /*
     * More Settings page
     */
    // 1. row
    tPosY = 0;
    // Button for info size
    TouchButtonInfoSize = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            COLOR_GUI_DISPLAY_CONTROL, SettingsButtonStringInfoSize, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doInfoSize);

    // Button for switching draw mode - line/pixel
    TouchButtonDrawModeLinePixel = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
            BUTTON_HEIGHT_4, COLOR_GUI_DISPLAY_CONTROL, DrawModeButtonStringLine, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0,
            &doDrawMode);

    // 2. row
    tPosY += BUTTON_HEIGHT_4_LINE_2;
    // Buttons for voltage calibration
    TouchButtonCalibrateVoltage = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
            COLOR_GUI_SOURCE_TIMEBASE, "Calibrate U", TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH, 0, &doVoltageCalibration);

    // Button for trigger line mode
    TouchButtonDrawModeTriggerLine = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tPosY, BUTTON_WIDTH_3,
            BUTTON_HEIGHT_4, COLOR_GUI_DISPLAY_CONTROL, DrawModeTriggerLineButtonString, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH,
            DisplayControl.DisplayBufferDrawMode & DRAW_MODE_TRIGGER, &doDrawModeTriggerLine);
    TouchButtonDrawModeTriggerLine->setRedGreenButtonColor();

    TouchSlider::resetDefaults();

#ifdef LOCAL_DISPLAY_EXISTS
    // 4. row
    tPosY += 2 * BUTTON_HEIGHT_4_LINE_2;
    // Button for ADS7846 channel
    TouchButtonADS7846TestOnOff = TouchButton::allocAndInitSimpleButton(0, tPosY, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_BLACK,
            StringSettingsButtonStringADS7846Test, TEXT_SIZE_11, BUTTON_FLAG_DO_BEEP_ON_TOUCH,
            MeasurementControl.ADS7846ChannelsAsDatasource, &doADS7846TestOnOff);
    TouchButtonADS7846TestOnOff->setRedGreenButtonColor();

    /*
     * Backlight slider
     */
    TouchSliderBacklight.initSlider(0, 0, TOUCHSLIDER_DEFAULT_SIZE, BACKLIGHT_MAX_VALUE, BACKLIGHT_MAX_VALUE, getBacklightValue(),
            NULL, TOUCHSLIDER_DEFAULT_TOUCH_BORDER, TOUCHSLIDER_SHOW_BORDER, &doBacklightSlider, NULL);
#endif

    /*
     * SLIDER
     */

// make slider slightly visible
    TouchSlider::setDefaultSliderColor(COLOR_DSO_SLIDER);
    TouchSlider::setDefaultBarColor(COLOR_BACKGROUND_DSO);

// invisible slider for voltage picker
    TouchSliderVoltagePicker.initSlider(SLIDER_VPICKER_X - TEXT_SIZE_11_WIDTH + 4, 0, 2, DISPLAY_VALUE_FOR_ZERO,
            DSO_DISPLAY_HEIGHT - 1, 0, NULL, 4 * TOUCHSLIDER_DEFAULT_TOUCH_BORDER, TOUCHSLIDER_VERTICAL_SHOW_NOTHING,
            &doVoltagePicker, NULL);

// invisible slider for trigger level
    TouchSliderTriggerLevel.initSlider(SLIDER_TLEVEL_X - 4, 0, 2, DISPLAY_VALUE_FOR_ZERO, DSO_DISPLAY_HEIGHT - 1, 0, NULL,
            4 * TOUCHSLIDER_DEFAULT_TOUCH_BORDER, TOUCHSLIDER_VERTICAL_SHOW_NOTHING, &doTriggerLevel, NULL);

#pragma GCC diagnostic pop
}

void activateCommonPartOfGui(void) {
    TouchSliderVoltagePicker.activate();
    TouchSliderVoltagePicker.drawBorder();

    TouchButtonMainHome->activate();

    TouchButtonDSOSettings->activate();
    TouchButtonStartStopDSOMeasurement->activate();
    TouchButtonFFT->activate();
}

/**
 * activate elements active while measurement is stopped and data is shown
 */
void activateAnalysisOnlyPartOfGui(void) {
    TouchButtonSingleshot->activate();

    TouchButtonStore->activate();
    TouchButtonLoad->activate();
}

/**
 * activate elements active while measurement is running
 */
void activateRunningOnlyPartOfGui(void) {
    if (MeasurementControl.TriggerMode == TRIGGER_MODE_MANUAL) {
        TouchSliderTriggerLevel.activate();
        TouchSliderTriggerLevel.drawBorder();
    }

    TouchButtonChartHistory->activate();
}

/**
 * draws elements active while measurement is stopped and data is shown
 */
void drawCommonPartOfGui(void) {
    TouchSliderVoltagePicker.drawBorder();

    //1. Row
    TouchButtonMainHome->drawButton();
    TouchButtonDSOSettings->drawButton();

    //2. Row
    TouchButtonStartStopDSOMeasurement->drawButton();

    // 4. Row
    TouchButtonFFT->drawButton();
}

/**
 * draws elements active while measurement is stopped and data is shown
 */
void drawAnalysisOnlyPartOfGui(void) {
    //1. Row
    TouchButtonSingleshot->drawButton();

    //2. Row
    TouchButtonStore->drawButton();
    TouchButtonLoad->drawButton();

    BlueDisplay1.drawText(BUTTON_WIDTH_3, BUTTON_HEIGHT_4_LINE_3 + TEXT_SIZE_22_ASCEND, "\xABScale\xBB", TEXT_SIZE_22, COLOR_YELLOW,
            COLOR_BACKGROUND_DSO);
    BlueDisplay1.drawText(BUTTON_WIDTH_3, BUTTON_HEIGHT_4_LINE_4 + BUTTON_DEFAULT_SPACING + TEXT_SIZE_22_ASCEND, "\xABScroll\xBB",
            TEXT_SIZE_22, COLOR_GREEN, COLOR_BACKGROUND_DSO);
}

/**
 * draws elements active while measurement is running
 */
void drawRunningOnlyPartOfGui(void) {
    if (MeasurementControl.TriggerMode == TRIGGER_MODE_MANUAL) {
        TouchSliderTriggerLevel.drawBorder();
    }

    // 4. Row
    TouchButtonChartHistory->drawButton();

    BlueDisplay1.drawText(BUTTON_WIDTH_8, BUTTON_HEIGHT_4_LINE_4 - TEXT_SIZE_22_DECEND, "\xABTimeBase\xBB", TEXT_SIZE_22,
            COLOR_GUI_SOURCE_TIMEBASE, COLOR_BACKGROUND_DSO);

// TODO replace by draw ML text
//    if (!MeasurementControl.RangeAutomatic || MeasurementControl.OffsetMode == OFFSET_MODE_MANUAL) {
//        BlueDisplay1.drawTextVertical(TEXT_SIZE_11_WIDTH, BUTTON_HEIGHT_4_LINE_2 - BUTTON_DEFAULT_SPACING, "\xD4Range\xD5",
//                TEXT_SIZE_22, COLOR_GUI_TRIGGER, COLOR_BACKGROUND_DSO);
//    }
//    if (MeasurementControl.OffsetMode != OFFSET_MODE_0_VOLT) {
//        BlueDisplay1.drawTextVertical(BUTTON_WIDTH_3_POS_3 - TEXT_SIZE_22_WIDTH, BUTTON_HEIGHT_4_LINE_2 - BUTTON_DEFAULT_SPACING,
//                "\xD4Offset\xD5", TEXT_SIZE_22, COLOR_GUI_TRIGGER, COLOR_BACKGROUND_DSO);
//    }
}

/**
 * draws elements active for settings page
 */
void drawDSOSettingsPageGui(void) {
    DisplayControl.DisplayPage = SETTINGS;
    // deactivate all gui and show setting elements
    TouchButton::deactivateAllButtons();
    TouchSlider::deactivateAllSliders();

    //1. Row
    TouchButtonACRangeOnOff->drawButton();
    TouchButtonAutoTriggerOnOff->drawButton();
    TouchButtonDSOMoreSettings->drawButton();

    //2. Row
    TouchButtonSlope->drawButton();
    TouchButtonAutoRangeOnOff->drawButton();
    TouchButtonAutoOffsetOnOff->drawButton();

    //3. Row
    TouchButtonShowPretriggerValuesOnOff->drawButton();
    TouchButtonChannelSelect->drawButton();

    // 4. Row
    TouchButtonFFT->drawButton();
    TouchButtonChartHistory->drawButton();
    TouchButtonBack->drawButton();

#ifdef LOCAL_DISPLAY_EXISTS
    TouchSliderBacklight.drawSlider();
#endif
}

void showDSOSettingsPage(void) {
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DSO);
    drawDSOSettingsPageGui();
}

void drawDSOMoreSettingsPageGui(void) {
    DisplayControl.DisplayPage = MORE_SETTINGS;
    // deactivate all gui and show more_setting elements
    TouchButton::deactivateAllButtons();
    TouchSlider::deactivateAllSliders();

    //1. Row
    TouchButtonInfoSize->drawButton();
    TouchButtonDrawModeLinePixel->drawButton();
    TouchButtonDrawModeTriggerLine->drawButton();

    //2. Row
    TouchButtonCalibrateVoltage->drawButton();

    // 4. Row
    TouchButtonADS7846TestOnOff->drawButton();
    TouchButtonBack->drawButton();
}

void redrawDisplay(void) {
    redrawDisplay(true);
}
/**
 * Clears and redraws all elements according to IsRunning/DisplayMode
 * @param doClearbefore if display mode = SHOW_GUI_* the display is cleared anyway
 */
void redrawDisplay(bool doClearbefore) {
    TouchButton::deactivateAllButtons();
    TouchSlider::deactivateAllSliders();
    if (doClearbefore) {
        BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DSO);
    }
    if (MeasurementControl.isRunning) {
        if (DisplayControl.DisplayPage >= SETTINGS) {
            drawDSOSettingsPageGui();
        } else {
            activateCommonPartOfGui();
            activateRunningOnlyPartOfGui();
            // initialize FFTDisplayBuffer
            memset(&DisplayBufferFFT[0], DSO_DISPLAY_HEIGHT - 1, sizeof(DisplayBufferFFT));
        }
        // show grid and labels - not really needed, since after MILLIS_BETWEEN_INFO_OUTPUT it is done by loop
        drawGridLinesWithHorizLabelsAndTriggerLine(COLOR_GRID_LINES);
        printInfo();
    } else {
        // measurement stopped -> analysis mode
        if (DisplayControl.DisplayPage == START) {
            // show analysis gui elements
            drawCommonPartOfGui();
            drawAnalysisOnlyPartOfGui();
        } else if (DisplayControl.DisplayPage == CHART) {
            activateCommonPartOfGui();
            activateAnalysisOnlyPartOfGui();
            // show grid and labels and chart
            drawGridLinesWithHorizLabelsAndTriggerLine(COLOR_GRID_LINES);
            drawMinMaxLines();
            drawDataBuffer(DataBufferControl.DataBufferDisplayStart, DSO_DISPLAY_WIDTH, COLOR_DATA_HOLD, 0);
            printInfo();
        } else {
            drawDSOSettingsPageGui();
        }
    }
}

