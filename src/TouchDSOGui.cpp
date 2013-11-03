/**
 * TouchDSOGui.cpp
 * @brief contains the gui related and flow control functions for DSO.
 *
 * @date 29.03.2012
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 */

#include "TouchDSO.h"
#include "Pages.h"
#include "misc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // for abs()

#ifdef __cplusplus
extern "C" {

#include "stm32f30xPeripherals.h"
#include "timing.h"
#include "ff.h"

}
#endif

/*
 * COLORS
 */

//Line colors
#define COLOR_DATA_PICKER COLOR_YELLOW

// GUI element colors
#define COLOR_GUI_CONTROL RGB(0xC0,0x00,0x00)
#define COLOR_GUI_TRIGGER RGB(0x00,0x00,0xD0)
#define COLOR_GUI_SOURCE_TIMEBASE RGB(0x00,0x90,0x00)
#define COLOR_GUI_DISPLAY_CONTROL RGB(0xC8,0xC8,0x00)

#define COLOR_DSO_SLIDER RGB(0xD8,0xE8,0xD8)

/******************************************************
 * GUI
 ******************************************************/

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
const char AutoOffsetButtonStringAuto[] = "Offset auto";
const char AutoOffsetButtonString0[] = "Offset 0V";

static TouchButton * TouchButtonDrawMode;
const char DrawModeButtonStringLine[] = "Line";
const char DrawModeButtonStringPixel[] = "Pixel";

// TODO still experimental
static TouchButton * TouchButtonDrawModeTriggerLine;
const char DrawModeTriggerLineButtonStringOff[] = "T off";
const char DrawModeTriggerLineButtonStringOn[] = "T on";

static TouchButton * TouchButtonSingleshot;
#define SINGLESHOT_PPRINT_VALUE_X (DISPLAY_WIDTH - FONT_WIDTH - 1)

static TouchButton * TouchButtonSlope;
char SlopeButtonString[] = "Slope \xD1"; // ascending
// the index of the slope indicator in char array
#define SLOPE_STRING_INDEX 6

static TouchButton *TouchButtonADS7846Test;
char StringSettingsButtonStringADS7846Test[] = "ADS7846    ";
#define SettingsButtonStringADS7846TestChangeIndex 8

static TouchButton *TouchButtonInfoSize;
char SettingsButtonStringInfoSize[] = "Info      ";
#define SettingsButtonStringInfoSizeChangeIndex 5

static TouchButton *TouchButtonChartHistory;
char ChartHistoryButtonString[] = "Hist     ";
#define ChartHistoryButtonStringChangeIndex 5
const char * ChartHistoryButtonStrings[DRAW_HISTORY_LEVELS] = { StringOff, StringLow, StringMid, StringHigh };

static TouchButton * TouchButtonLoad;
static TouchButton * TouchButtonStore;
#define MODE_LOAD 0
#define MODE_STORE 1

static TouchButton * TouchButtonChannelSelect;
static TouchButton * TouchButtonDisplayMode;
static TouchButton * TouchButtonDSOSettings;

static TouchButton * TouchButtonCalibrateVoltage;

static TouchButton * TouchButtonACRange;
char ACRangeButtonStringDC[] = "+";
char ACRangeButtonStringAC[] = "\xF1"; // +- character

static TouchButton ** TouchButtonsDSO[] = { &TouchButtonBack, &TouchButtonStartStopDSOMeasurement, &TouchButtonAutoTriggerOnOff,
		&TouchButtonAutoRangeOnOff, &TouchButtonAutoOffsetOnOff, &TouchButtonChannelSelect, &TouchButtonDisplayMode,
		&TouchButtonDrawMode, &TouchButtonDrawModeTriggerLine, &TouchButtonDSOSettings, &TouchButtonSingleshot, &TouchButtonSlope,
		&TouchButtonADS7846Test, &TouchButtonInfoSize, &TouchButtonChartHistory, &TouchButtonLoad, &TouchButtonStore,
		&TouchButtonCalibrateVoltage, &TouchButtonACRange };

static TouchSlider TouchSliderTriggerLevel;

static TouchSlider TouchSliderVoltagePicker;
uint8_t LastPickerValue;

/**********************************
 * Input channels
 **********************************/
char StringChannel2[] = "Channel 2";
char StringChannel3[] = "Channel 3";
char StringChannel4[] = "Channel 4";
char StringTemperature[] = "Temp";
char StringVBattDiv2[] = "VBatt/2";
char StringVRefint[] = "VRefint";
char * ADCInputMUXChannelStrings[ADC_CHANNEL_COUNT] = { StringChannel2, StringChannel3, StringChannel4, StringTemperature,
		StringVBattDiv2, StringVRefint };
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
void drawSettingsOnlyPartOfGui(void);
void redrawDisplay(bool doClearbefore);
bool longTouchHandlerDSO(const int aTouchPositionX, const int aTouchPositionY, bool isLongTouch);
bool endTouchHandlerDSO(const uint32_t aMillisOfTouch, const int aTouchDeltaX, const int aTouchDeltaY);

void changeTimeBase(void);

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
}

void initCaptions(void) {

	strlcpy(&SettingsButtonStringInfoSize[SettingsButtonStringInfoSizeChangeIndex], StringLarge, sizeof StringLarge);
	TouchButtonInfoSize->setCaption(SettingsButtonStringInfoSize);

	strlcpy(&ChartHistoryButtonString[ChartHistoryButtonStringChangeIndex], StringOff, sizeof StringOff);
	TouchButtonChartHistory->setCaption(ChartHistoryButtonString);

	strlcpy(&StringSettingsButtonStringADS7846Test[SettingsButtonStringADS7846TestChangeIndex], StringOff, sizeof StringOff);
	TouchButtonADS7846Test->setCaption(StringSettingsButtonStringADS7846Test);
}

void startDSO(void) {

	initAcquisition();

	clearDisplay(COLOR_BACKGROUND_DSO);
	setDimDelayMillis(3 * BACKLIGHT_DIM_DEFAULT_DELAY);
	DisplayControl.DisplayBufferDrawMode = DRAW_MODE_LINE;

	DisplayControl.EraseColor = DisplayControl.EraseColors[0];
	DisplayControl.EraseColorIndex = 0;
	DisplayControl.XScale = 1;
	DisplayControl.DisplayIncrementPixel = DATABUFFER_DISPLAY_INCREMENT; // == getXScaleCorrectedValue(DATABUFFER_DISPLAY_INCREMENT);

	initDSOGUI();
	initCaptions();

	DisplayControl.DisplayMode = DISPLAY_MODE_SHOW_GUI_ELEMENTS;
	// show start elements
	drawCommonPartOfGui();
	drawAnalysisOnlyPartOfGui();

	TouchPanel.registerLongTouchCallback(&longTouchHandlerDSO, TOUCH_STANDARD_LONG_TOUCH_TIMEOUT_MILLIS);
	TouchPanel.registerEndTouchCallback(&endTouchHandlerDSO);
}

void stopDSO(void) {
	DSO_setAttenuator(DSO_ATTENUATOR_SHORTCUT);

	// only here
	ADC_stopTimer6();

	// in case of exiting during a running acquisition
	ADC_StopConversion(DSO_ADC_ID);
	ADC_disableEOCInterrupt(DSO_ADC_ID);

	// free buttons
	for (unsigned int i = 0; i < sizeof(TouchButtonsDSO) / sizeof(TouchButtonsDSO[0]); ++i) {
		(*TouchButtonsDSO[i])->setFree();
	}
	TouchPanel.registerLongTouchCallback(NULL, 0);
	TouchPanel.registerEndTouchCallback(NULL);
	setDimDelayMillis(BACKLIGHT_DIM_DEFAULT_DELAY);
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

	if (MeasurementControl.IsRunning) {
		if (MeasurementControl.ADS7846ChannelsActive) {
			readADS7846Channels();
			TouchPanel.rd_data();
			if (TouchPanel.mPressure > MIN_REASONABLE_PRESSURE) {
				TouchButton::checkAllButtons(TouchPanel.getXActual(), TouchPanel.getYActual(), true);
			}

		} else if (MeasurementControl.TimebaseFastDMAMode && !DataBufferControl.DataBufferFull) {
			/*
			 * Fast mode here
			 */
			acquireDataByDMA();
		}
		if (DataBufferControl.DataBufferFull) {
			/*
			 * Data (from InterruptServiceRoutine or DMA) is ready
			 */
			findMinMaxAverage();
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
					MeasurementControl.IsRunning = false;
					MeasurementControl.SingleShotMode = false;
					// delayed tone for stop
					FeedbackToneOK();
					DisplayControl.DisplayMode = DISPLAY_MODE_INFO_LINE;
					// draw grid lines and gui
					redrawDisplay(true);

					// print data in line (not pixel) mode
					int tTempDrawMode = DisplayControl.DisplayBufferDrawMode;
					DisplayControl.DisplayBufferDrawMode = DRAW_MODE_LINE;
					drawDataBuffer(COLOR_DATA_HOLD, DataBufferControl.DataBufferDisplayStart, 0);
					DisplayControl.DisplayBufferDrawMode = tTempDrawMode;
				}
			} else {
				if (MeasurementControl.ChangeRequestedFlags & CHANGE_REQUESTED_TIMEBASE) {
					changeTimeBase();
					MeasurementControl.ChangeRequestedFlags = 0;
				}

				// just process data and start new Acquisition
				int tLastTriggerDisplayValue = DisplayControl.TriggerLevelDisplayValue;
				computeAutoTrigger();
				computeAutoRange();
				computeAutoOffset();
				// handle trigger line
				DisplayControl.TriggerLevelDisplayValue = getDisplayFrowRawInputValue(MeasurementControl.RawTriggerLevel);
				if (tLastTriggerDisplayValue
						!= DisplayControl.TriggerLevelDisplayValue&& MeasurementControl.TriggerMode != TRIGGER_MODE_OFF) {
					clearTriggerLine(tLastTriggerDisplayValue);
					drawTriggerLine();
				}
				if (!DataBufferControl.DrawWhileAcquire) {
					// normal mode => clear old chart and draw new data
					drawDataBuffer(COLOR_DATA_RUN, DataBufferControl.DataBufferDisplayStart, DisplayControl.EraseColor);
				}
				// starts next acquisition
				startAcquisition();
			}
		}
		if (DataBufferControl.DrawWhileAcquire) {
			// detect end of pre trigger phase and adjust pre trigger buffer during acquisition
			if (MeasurementControl.TriggerPhaseJustEnded) {
				MeasurementControl.TriggerPhaseJustEnded = false;
				adjustPreTriggerBuffer(FourDisplayLinesBuffer);
				DataBufferControl.DataBufferNextDrawPointer = &DataBufferControl.DataBuffer[DATABUFFER_DISPLAY_START];
				DataBufferControl.NextDrawXValue = 0;
			}
			// check if new data available and draw in corresponding color
			if (MeasurementControl.ActualPhase < PHASE_POST_TRIGGER) {
				if (DataBufferControl.DataBufferWrapAround) {
					// wrap while searching trigger -> start from beginning of buffer
					DataBufferControl.DataBufferWrapAround = false;
					DataBufferControl.NextDrawXValue = 0;
					DataBufferControl.DataBufferNextDrawPointer = &DataBufferControl.DataBuffer[0];
				}
				drawRemainingDataBufferValues(COLOR_DATA_PRETRIGGER);
			} else {
				drawRemainingDataBufferValues(COLOR_DATA_RUN);
			}
		}
		if (MeasurementControl.SingleShotMode && MeasurementControl.ActualPhase < PHASE_POST_TRIGGER
				&& MillisSinceLastAction > MILLIS_BETWEEN_INFO_OUTPUT) {
			// output actual values for singleshot every second
			// TODO in singleshot analyze pre trigger buffer and set min, max average
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
		if (MeasurementControl.IsRunning && DisplayControl.DisplayMode <= DISPLAY_MODE_INFO_LINE) {
			// refresh Grid
			drawGridLinesWithHorizLabelsAndTriggerLine(COLOR_GRID_LINES);
		} else if (MeasurementControl.IsRunning && DisplayControl.DisplayMode == DISPLAY_MODE_SETTINGS) {
			// refresh buttons
			drawSettingsOnlyPartOfGui();
		}
		if (DisplayControl.DisplayMode == DISPLAY_MODE_INFO_LINE) {
			// print info
			printInfo();
			if (!MeasurementControl.IsRunning) {
				// show time
				if ((MeasurementControl.InfoSizeSmall)) {
					showRTCTimeEverySecond(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + 2 * FONT_HEIGHT, COLOR_RED, COLOR_BACKGROUND_DSO);
				} else {
					showRTCTimeEverySecond(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + 3 * FONT_HEIGHT, COLOR_RED, COLOR_BACKGROUND_DSO);
				}
			}
		}
	}
	/*
	 * sliders must always be checked, buttons are only checked at end of touch (EndTouchHandler was called) and no swipe was detected
	 */
	CheckTouchGeneric(false);
}
/* Main loop end */

bool checkDatabufferPointerForDrawing(void) {
	bool tReturn = true;
	// check for DataBufferDisplayStart
	if (DataBufferControl.DataBufferDisplayStart
			> DataBufferControl.DataBufferEndPointer - (getXScaleCorrectedValue(DSO_DISPLAY_WIDTH) - 1)) {
		DataBufferControl.DataBufferDisplayStart = (uint16_t *) DataBufferControl.DataBufferEndPointer
				- (getXScaleCorrectedValue(DSO_DISPLAY_WIDTH) - 1);
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
int setNewTimeBaseValue(int aValue) {
	uint8_t tFeedbackType = FEEDBACK_TONE_NO_ERROR;
	int tOldIndex = MeasurementControl.IndexTimebase;
	// positive value means increment timebase index!
	int tNewIndex = tOldIndex + aValue;
	// do not decrement below 0 or increment above TIMEBASE_NUMBER_OF_ENTRIES -1
	if (tNewIndex < 0) {
		tNewIndex = 0;
	}
	if (tNewIndex > TIMEBASE_NUMBER_OF_ENTRIES - 1) {
		tNewIndex = TIMEBASE_NUMBER_OF_ENTRIES - 1;
	}
	if (tNewIndex == tOldIndex) {
		tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
	} else {
		// signal to main loop in thread mode
		MeasurementControl.NewIndexTimebase = tNewIndex;
		MeasurementControl.ChangeRequestedFlags |= CHANGE_REQUESTED_TIMEBASE;
	}
	return tFeedbackType;
}

/**
 * Real change is done here
 */
void changeTimeBase(void) {
	int tOldIndex = MeasurementControl.IndexTimebase;
	MeasurementControl.IndexTimebase = MeasurementControl.NewIndexTimebase;
	ADC_SetTimer6Period(TimebaseTimerDividerValues[MeasurementControl.IndexTimebase],
			TimebaseTimerPrescalerDividerValues[MeasurementControl.IndexTimebase]);
	// so set matching sampling times for channel
	ADC_SetChannelSampleTime(DSO_ADC_ID, ADCInputMUXChannels[MeasurementControl.IndexADCInputMUXChannel],
			(MeasurementControl.IndexTimebase < TIMEBASE_FAST_MODES));

	if (ADCClockPrescalerValues[MeasurementControl.IndexTimebase] != ADCClockPrescalerValues[tOldIndex]) {
		// stop and start is really needed :-(
		//TODO which of the debug statement is necessary for the correct function
		DebugValue1 = DSO_ADC_ID->CR; // it is 0x10000001
		uint32_t tDuration = getSysticValue();
		// first disable ADC otherwise sometimes interrupts just stops after the first reading
		ADC_disableAndWait(DSO_ADC_ID);
		DebugValue2 = DSO_ADC_ID->ISR; // Check ARDY flag (it is 0x0B or 0x1B)
		ADC_SetClockPrescaler(ADCClockPrescalerValues[MeasurementControl.IndexTimebase]);
		delayMillis(2); // does it help ???
		ADC_enableAndWait(DSO_ADC_ID);
		DebugValue3 = getSysticValue() - tDuration; // it gives 0xA8, 0x1B5
	}

	if (MeasurementControl.IndexTimebase < TIMEBASE_NUMBER_OF_XSCALE_CORRECTION) {
		DisplayControl.XScale = xScaleForTimebase[MeasurementControl.IndexTimebase];
	} else {
		DisplayControl.XScale = 1;
	}
	DataBufferControl.DrawWhileAcquire = (MeasurementControl.IndexTimebase >= TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE);
	MeasurementControl.TriggerTimeoutSampleOrLoopCount = computeNumberOfSamplesToTimeout(MeasurementControl.IndexTimebase);
	printInfo();
}
/**
 * Handler for range up / down
 */
int changeRange(int aValue) {
	int tFeedbackType = FEEDBACK_TONE_NO_ERROR;
	if (!changeInputRange(aValue)) {
		tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
	}
	return tFeedbackType;
}

/**
 * Handler for range up / down
 */
int changeOffsetGridCount(int aValue) {
	setOffsetGridCount(MeasurementControl.OffsetGridCount + aValue);
	return FEEDBACK_TONE_NO_ERROR;
}

int changeXScale(int aValue) {
	int tFeedbackType = FEEDBACK_TONE_NO_ERROR;
	DisplayControl.XScale += aValue;

	if (DisplayControl.XScale == 0) {
		// down 3,2,1 -> -2,-3
		DisplayControl.XScale = -2;
	} else if (DisplayControl.XScale == -1) {
		// up -3,-2 -> 1,2
		DisplayControl.XScale = 1;
	} else if (DisplayControl.XScale < -DATABUFFER_DISPLAY_RESOLUTION_FACTOR) {
		tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
		DisplayControl.XScale = -DATABUFFER_DISPLAY_RESOLUTION_FACTOR;
	}
	DisplayControl.DisplayIncrementPixel = getXScaleCorrectedValue(DATABUFFER_DISPLAY_INCREMENT);

	if (!MeasurementControl.IsRunning) {
		if (DisplayControl.DisplayMode == DISPLAY_MODE_INFO_LINE) {
			printInfo();
		}
		// Check end
		if (!checkDatabufferPointerForDrawing()) {
			tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
		}
		// do tone before draw
		FeedbackTone(tFeedbackType);
		tFeedbackType = FEEDBACK_TONE_NO_TONE;
		drawDataBuffer(COLOR_DATA_HOLD, DataBufferControl.DataBufferDisplayStart, DisplayControl.EraseColor);
	}
	return tFeedbackType;
}

/**
 * set start of display in data buffer
 * @param aValue number of DisplayControl.DisplayIncrementPixel to scroll
 */
int scrollDisplay(int aValue) {
	uint8_t tFeedbackType = FEEDBACK_TONE_NO_ERROR;
	if (DisplayControl.DisplayMode != DISPLAY_MODE_SETTINGS) {
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
		drawDataBuffer(COLOR_DATA_HOLD, DataBufferControl.DataBufferDisplayStart, DisplayControl.EraseColor);
	}
	return tFeedbackType;
}

/**
 * Shows gui on long touch
 * @param aTouchPositionX
 * @param aTouchPositionY
 * @param isLongTouch
 * @return
 */

bool longTouchHandlerDSO(const int aTouchPositionX, const int aTouchPositionY, bool isLongTouch) {
	if (isLongTouch && !sSliderTouched && DisplayControl.DisplayMode != DISPLAY_MODE_SETTINGS) {
		// No long touch if swipe is made
		if (TouchPanel.getSwipeAmount() < TIMING_GRID_WIDTH) {
			// long touch action here
			DisplayControl.DisplayMode = DISPLAY_MODE_SHOW_GUI_ELEMENTS;
			redrawDisplay(false);
			sDisableEndTouchOnce = true;
		}
	}
	return true;
}

/**
 * responsible for swipe detection and dispatching
 * @param aMillisOfTouch
 * @param aTouchDeltaX
 * @param aTouchDeltaY
 * @return
 */

bool endTouchHandlerDSO(const uint32_t aMillisOfTouch, const int aTouchDeltaX, const int aTouchDeltaY) {
	if (!sSliderTouched) {
		if (TouchPanel.getSwipeAmount() < TIMING_GRID_WIDTH) {
			if (!sDisableEndTouchOnce) {
				// no swipe, no long touch action -> let loop check gui
				sCheckButtonsForEndTouch = true;
			}
			sDisableEndTouchOnce = false;
		} else if (DisplayControl.DisplayMode != DISPLAY_MODE_SETTINGS) {
			int tFeedbackType = FEEDBACK_TONE_NO_ERROR;
			int tTouchDeltaXGrid = aTouchDeltaX / TIMING_GRID_WIDTH;
			if (MeasurementControl.IsRunning) {
				if (abs(aTouchDeltaX) > abs(aTouchDeltaY)) {
					// Horizontal swipe -> timebase
					tFeedbackType = setNewTimeBaseValue(tTouchDeltaXGrid / 2);
				} else {
					// Vertical swipe
					int tTouchDeltaYGrid = aTouchDeltaY / TIMING_GRID_WIDTH;
					if (!MeasurementControl.RangeAutomatic && MeasurementControl.OffsetAutomatic) {
						if (TouchPanel.getXFirst() > BUTTON_WIDTH_5_POS_5) {
							//offset
							tFeedbackType = changeOffsetGridCount(tTouchDeltaYGrid);
						} else {
							//range
							tFeedbackType = changeRange(-tTouchDeltaYGrid / 2);
						}
					} else if (!MeasurementControl.RangeAutomatic) {
						//range
						tFeedbackType = changeRange(-tTouchDeltaYGrid / 2);
					} else if (MeasurementControl.OffsetAutomatic) {
						//offset
						tFeedbackType = changeOffsetGridCount(tTouchDeltaYGrid);
					}
				}
			} else {
				if (TouchPanel.getYFirst() > BUTTON_HEIGHT_4_LINE_4) {
					// TODO lock if time is drawn
					// scroll can be done here in interrupt context, because no drawing is done in thread (except date :-((( - have to check...)
					tFeedbackType = scrollDisplay(tTouchDeltaXGrid);
				} else {
					// scale
					tFeedbackType = changeXScale(-(tTouchDeltaXGrid / 2));
				}
			}
			FeedbackTone(tFeedbackType);
		}
	}
	return true;
}

/************************************************************************
 * GUI handler section
 ************************************************************************/
/*
 * toggle between automatic and manual trigger voltage value
 * and switch between XScale and Range buttons
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
	if (DisplayControl.DisplayMode >= DISPLAY_MODE_SHOW_GUI_ELEMENTS) {
		aTheTouchedButton->drawButton();
	} else if (DisplayControl.DisplayMode == DISPLAY_MODE_INFO_LINE) {
		printInfo();
	}
}

void doRangeMode(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	MeasurementControl.RangeAutomatic = !MeasurementControl.RangeAutomatic;
	if (MeasurementControl.RangeAutomatic) {
		aTheTouchedButton->setCaption(AutoRangeButtonStringAuto);
	} else {
		aTheTouchedButton->setCaption(AutoRangeButtonStringManual);
	}
	if (DisplayControl.DisplayMode >= DISPLAY_MODE_SHOW_GUI_ELEMENTS) {
		aTheTouchedButton->drawButton();
	}
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

	if (DisplayControl.DisplayMode >= DISPLAY_MODE_SHOW_GUI_ELEMENTS) {
		aTheTouchedButton->drawCaption();
	} else {
		DisplayControl.DisplayMode = DISPLAY_MODE_INFO_LINE;
		printInfo();
	}
}

/*
 * toggle between auto and 0 Volt offset
 */
void doAutoOffsetOnOff(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	MeasurementControl.OffsetAutomatic = !MeasurementControl.OffsetAutomatic;
	if (MeasurementControl.OffsetAutomatic) {
		aTheTouchedButton->setCaption(AutoOffsetButtonStringAuto);
	} else {
		aTheTouchedButton->setCaption(AutoOffsetButtonString0);
		// disable auto offset
		if (MeasurementControl.ACRange) {
			// Zero line is at grid 3
			MeasurementControl.OffsetGridCount = DISPLAY_AC_ZERO_OFFSET_GRID_COUNT;
			MeasurementControl.RawOffsetValueForDisplayRange = getRawOffsetValueFromGridCount(DISPLAY_AC_ZERO_OFFSET_GRID_COUNT);
		} else {
			MeasurementControl.RawOffsetValueForDisplayRange = 0;
			MeasurementControl.OffsetGridCount = 0;
		}
		MeasurementControl.IndexDisplayRange = MeasurementControl.IndexInputRange;
		MeasurementControl.InputRangeIndexOtherThanDisplayRange = false;
	}
	if (DisplayControl.DisplayMode >= DISPLAY_MODE_SHOW_GUI_ELEMENTS) {
		aTheTouchedButton->drawButton();
	} else {
		DisplayControl.DisplayMode = DISPLAY_MODE_INFO_LINE;
		printInfo();
	}
}

/*
 * Cycle through all external and internal adc channels
 */
void doChannelSelect(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	MeasurementControl.IndexADCInputMUXChannel++;
	if (MeasurementControl.ADS7846ChannelsActive) {
		// ADS7846 channels
		if (MeasurementControl.IndexADCInputMUXChannel >= ADS7846_CHANNEL_COUNT) {
			// wrap to first channel
			MeasurementControl.IndexADCInputMUXChannel = 0;
		}
		aTheTouchedButton->setCaption(ADS7846ChannelStrings[MeasurementControl.IndexADCInputMUXChannel]);

	} else {
		if (MeasurementControl.IndexADCInputMUXChannel >= ADC_CHANNEL_COUNT) {
			// wrap to first channel
			MeasurementControl.IndexADCInputMUXChannel = 0;
		}
		ADC_SetChannelSampleTime(DSO_ADC_ID, ADCInputMUXChannels[MeasurementControl.IndexADCInputMUXChannel],
				MeasurementControl.TimebaseFastDMAMode);

// set channel number in caption
		aTheTouchedButton->setCaption(ADCInputMUXChannelStrings[MeasurementControl.IndexADCInputMUXChannel]);
	}
	if (DisplayControl.DisplayMode >= DISPLAY_MODE_SHOW_GUI_ELEMENTS) {
		aTheTouchedButton->drawButton();
	} else {
		DisplayControl.DisplayMode = DISPLAY_MODE_INFO_LINE;
		clearInfo(2);
		printInfo();
	}
}

/*
 * Toggle between pixel and line draw mode (for data chart)
 */
void doDrawMode(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
// erase old chart in old mode
	if (DisplayControl.DisplayMode < DISPLAY_MODE_SHOW_GUI_ELEMENTS) {
		drawDataBuffer(DisplayControl.EraseColor, NULL, 0);
	}
// switch mode
	if (DisplayControl.DisplayBufferDrawMode & DRAW_MODE_LINE) {
		DisplayControl.DisplayBufferDrawMode &= ~DRAW_MODE_LINE;
		aTheTouchedButton->setCaption(DrawModeButtonStringPixel);
	} else {
		aTheTouchedButton->setCaption(DrawModeButtonStringLine);
		DisplayControl.DisplayBufferDrawMode = DRAW_MODE_LINE;
	}
	if (DisplayControl.DisplayMode >= DISPLAY_MODE_SHOW_GUI_ELEMENTS) {
		// show new caption
		aTheTouchedButton->drawButton();
	}
}

/*
 * Switch trigger line mode on and off (for data chart)
 */
void doDrawModeTriggerLine(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();

// switch mode
	if (DisplayControl.DisplayBufferDrawMode & DRAW_MODE_TRIGGER) {
		DisplayControl.DisplayBufferDrawMode &= ~DRAW_MODE_TRIGGER;
		aTheTouchedButton->setCaption(DrawModeTriggerLineButtonStringOff);
	} else {
		// erase old chart in old mode
		if (DisplayControl.DisplayMode < DISPLAY_MODE_SHOW_GUI_ELEMENTS) {
			drawDataBuffer(DisplayControl.EraseColor, NULL, 0);
		}
		aTheTouchedButton->setCaption(DrawModeTriggerLineButtonStringOn);
		DisplayControl.DisplayBufferDrawMode |= DRAW_MODE_TRIGGER;
	}
	if (DisplayControl.DisplayMode >= DISPLAY_MODE_SHOW_GUI_ELEMENTS) {
		// show new caption
		aTheTouchedButton->drawButton();
	}
}

/**
 * @param aValue
 * @return aValue divided by XScale for XScale > 1, else aValue times XScale
 */
int getXScaleCorrectedValue(int aValue) {
	if (DisplayControl.XScale != 1) {
		if (DisplayControl.XScale > 1) {
			aValue = aValue / DisplayControl.XScale;
		} else {
			aValue = aValue * -DisplayControl.XScale;
		}
	}
	return aValue;
}

void doACRange(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	MeasurementControl.ACRange = !MeasurementControl.ACRange;
	DSO_setACRange(MeasurementControl.ACRange);
	if (MeasurementControl.ACRange) {
		// Zero line is at grid 3
		MeasurementControl.OffsetGridCount = DISPLAY_AC_ZERO_OFFSET_GRID_COUNT;
		MeasurementControl.RawOffsetValueForDisplayRange = getRawOffsetValueFromGridCount(DISPLAY_AC_ZERO_OFFSET_GRID_COUNT);
		aTheTouchedButton->setCaption(ACRangeButtonStringAC);
	} else {
		MeasurementControl.OffsetGridCount = 0;
		MeasurementControl.RawOffsetValueForDisplayRange = 0;
		aTheTouchedButton->setCaption(ACRangeButtonStringDC);
	}

	aTheTouchedButton->drawCaption();
}

/*
 * Button handler for "display" button
 * while running switch between upper info line on/off
 * while stopped switch between:
 * 1. chart+ voltage picker
 * 2. chart + info line + grid + voltage picker
 */
void doDisplayMode(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	DisplayControl.DisplayMode++;
// Wrap display mode
	if (DisplayControl.DisplayMode > DISPLAY_MODE_INFO_LINE) {
		DisplayControl.DisplayMode = DISPLAY_MODE_ONLY_DATA;
	}
	if (MeasurementControl.IsRunning) {
		if (DisplayControl.DisplayMode == DISPLAY_MODE_ONLY_DATA) {
			redrawDisplay(true);
		} else {
			// DISPLAY_MODE_INFO_LINE
			printInfo();
		}
	} else {
		/*
		 * analysis mode here
		 */
		if (DisplayControl.DisplayMode == DISPLAY_MODE_ONLY_DATA) {
			// draw grid lines
			drawGridLinesWithHorizLabelsAndTriggerLine(COLOR_GRID_LINES);
			// only data chart and grid + labels
			redrawDisplay(true);
			// flag to avoid initial clearing of (not existent) picker line
			LastPickerValue = 0xFF;
		} else {
			// DISPLAY_MODE_INFO_LINE
			// from only chart to chart + grid + info
			printInfo();
			drawMinMaxLines();
		}
	}
}

/*
 * show gui of settings screen
 */
void doSettings(TouchButton * const aTheTouchedButton, int16_t aValue) {
	static uint8_t sLastMode;
	FeedbackToneOK();
	if (DisplayControl.DisplayMode == DISPLAY_MODE_SETTINGS) {
		// Back
		DisplayControl.DisplayMode = sLastMode;
		redrawDisplay(true);
	} else {
		// show settings page
		sLastMode = DisplayControl.DisplayMode;
		// deactivate all gui and show setting elements
		TouchButton::deactivateAllButtons();
		TouchSlider::deactivateAllSliders();
		clearDisplay(COLOR_BACKGROUND_DSO);
		DisplayControl.DisplayMode = DISPLAY_MODE_SETTINGS;
		drawSettingsOnlyPartOfGui();
	}
}

/**
 * set to singleshot mode and draw an indicating "S"
 */
void doTriggerSingleshot(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	MeasurementControl.SingleShotMode = true;
	clearDisplay(COLOR_BACKGROUND_DSO);
	drawGridLinesWithHorizLabelsAndTriggerLine(COLOR_GRID_LINES);

// prepare info output - at least 1 sec later
	MillisSinceLastAction = 0;
	MeasurementControl.RawValueMax = 0;
	MeasurementControl.RawValueMin = 0;
	MeasurementControl.RawValueAverage = 0;
	if (!MeasurementControl.IsRunning) {
		// Start a new single shot
		startAcquisition();
		// must be after startAcquisition()
		MeasurementControl.IsRunning = true;
	}
}

void doStartStopDSO(TouchButton * const aTheTouchedButton, int16_t aValue) {
	if (MeasurementControl.IsRunning) {
		/*
		 * Stop here
		 * for the last measurement read full buffer size
		 * Do this asynchronously to the interrupt routine in order to extend a running or started acquisition
		 * stop single shot mode
		 */
		// first extends end marker for ISR
		uint16_t * tEndPointer = &DataBufferControl.DataBuffer[DATABUFFER_SIZE - 1];
		DataBufferControl.DataBufferEndPointer = tEndPointer;

//		if (MeasurementControl.SingleShotMode) {
//			MeasurementControl.ActualPhase = PHASE_POST_TRIGGER;
//		}
		// in SingleShotMode Stop is always requested
		if (MeasurementControl.StopRequested && !MeasurementControl.SingleShotMode) {
			// for stop requested 2 times -> stop immediately
			tEndPointer = DataBufferControl.DataBufferNextPointer;
			DataBufferControl.DataBufferEndPointer = tEndPointer;
			// clear trailing buffer space not used
			memset(tEndPointer, 0, ((uint8_t*) &DataBufferControl.DataBuffer[DATABUFFER_SIZE]) - ((uint8_t*) tEndPointer));
		}
		// TODO does this stop the SingleShotMode?
		MeasurementControl.SingleShotMode = false; // return to continuous  mode
		MeasurementControl.StopRequested = true;
		// settings button has different positions on acquisition and analysis page
		TouchButtonDSOSettings->setPositionX(BUTTON_WIDTH_3_POS_2);
		// no feedback tone, it kills the timing!
	} else {
		/*
		 *  Start here in normal mode (reset single shot mode) and positive XScale
		 */
		FeedbackToneOK();
		TouchButtonDSOSettings->setPositionX(BUTTON_WIDTH_3_POS_3);

		DisplayControl.DisplayMode = DISPLAY_MODE_INFO_LINE;
		MeasurementControl.SingleShotMode = false; // return to continuous  mode
		MeasurementControl.TimestampLastRangeChange = 0; // enable direct range change at start

		// reset xScale to regular value
		if (MeasurementControl.IndexTimebase < TIMEBASE_NUMBER_OF_XSCALE_CORRECTION) {
			DisplayControl.XScale = xScaleForTimebase[MeasurementControl.IndexTimebase];
		} else {
			DisplayControl.XScale = 1;
		}

		startAcquisition();
		// must be after startAcquisition()
		MeasurementControl.IsRunning = true;

		redrawDisplay(true);
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
	if (MeasurementControl.IsRunning && DisplayControl.DisplayMode < DISPLAY_MODE_SETTINGS) {
		// clear history on screen
		redrawDisplay(true);
	} else {
		aTheTouchedButton->drawButton();
	}
}

/*
 * Toggles ADS7846Test on / off
 */
void doADS7846Test(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	MeasurementControl.ADS7846ChannelsActive = !MeasurementControl.ADS7846ChannelsActive;
	MeasurementControl.IndexADCInputMUXChannel = 0;
	if (MeasurementControl.ADS7846ChannelsActive) {
		strlcpy(&StringSettingsButtonStringADS7846Test[SettingsButtonStringADS7846TestChangeIndex], StringOn, sizeof StringOn);
	} else {
		strlcpy(&StringSettingsButtonStringADS7846Test[SettingsButtonStringADS7846TestChangeIndex], StringOff, sizeof StringOff);
	}
	aTheTouchedButton->setCaption(StringSettingsButtonStringADS7846Test);
	aTheTouchedButton->drawButton();
	MeasurementControl.OffsetAutomatic = true;
	TouchButtonAutoOffsetOnOff->setCaption(AutoOffsetButtonStringAuto);
}

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
		setLevelAndHysteresis(tRawLevel, TRIGGER_HYSTERESIS_MANUAL);
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
		drawLine(0, tYpos, DSO_DISPLAY_WIDTH - 1, tYpos, COLOR_BACKGROUND_DSO);
		if (DisplayControl.DisplayMode == DISPLAY_MODE_INFO_LINE) {
			// restore grid at old y position
			for (int tXPos = TIMING_GRID_WIDTH - 1; tXPos < DSO_DISPLAY_WIDTH - 1; tXPos += TIMING_GRID_WIDTH) {
				drawPixel(tXPos, tYpos, COLOR_GRID_LINES);
			}
		}
		if (!MeasurementControl.IsRunning) {
			// restore graph
			uint8_t* ScreenBufferPointer = &DisplayBuffer1[0];
			for (int i = 0; i < DSO_DISPLAY_WIDTH; ++i) {
				int tValueByte = *ScreenBufferPointer++;
				if (tValueByte == tYpos) {
					drawPixel(i, tValueByte, COLOR_DATA_HOLD);
				}
			}
		}
	}
// draw new line
	int tValue = DISPLAY_VALUE_FOR_ZERO - aValue;
	drawLine(0, tValue, DSO_DISPLAY_WIDTH - 1, tValue, COLOR_DATA_PICKER);
	LastPickerValue = aValue;

	float tVoltage = getFloatFromDisplayValue(tValue);
	snprintf(StringBuffer, sizeof StringBuffer, "%6.*fV", RangePrecision[MeasurementControl.IndexDisplayRange] + 1, tVoltage);

	int tYPos = INFO_UPPER_MARGIN + FONT_HEIGHT;
	if (!MeasurementControl.InfoSizeSmall) {
		tYPos += FONT_HEIGHT;
	}

// print value
	drawText(INFO_LEFT_MARGIN + SLIDER_VPICKER_X, tYPos, StringBuffer, 1, COLOR_BLACK, COLOR_INFO_BACKGROUND);
	return aValue;
}

static void doStoreLoadAcquisitionData(TouchButton * const aTheTouchedButton, int16_t aMode) {
	int tFeedbackType = FEEDBACK_TONE_LONG_ERROR;
	if (!MeasurementControl.IsRunning && MICROSD_isCardInserted()) {
		FIL tFile;
		FRESULT tOpenResult;
		UINT tCount;
		snprintf(StringBuffer, sizeof StringBuffer, "DSO-data.bin");

		if (aMode == MODE_LOAD) {
			// Load new data from file
			tOpenResult = f_open(&tFile, StringBuffer, FA_OPEN_EXISTING | FA_READ);
			if (tOpenResult == FR_OK) {
				f_read(&tFile, &MeasurementControl, sizeof(MeasurementControl), &tCount);
				f_read(&tFile, &DataBufferControl, sizeof(DataBufferControl), &tCount);
				f_close(&tFile);
				// redraw display corresponding to new values
				drawDataBuffer(COLOR_DATA_RUN, DataBufferControl.DataBufferDisplayStart, DisplayControl.EraseColor);
				printInfo();
				tFeedbackType = FEEDBACK_TONE_NO_ERROR;
			}
		} else {
			// Store
			tOpenResult = f_open(&tFile, StringBuffer, FA_CREATE_ALWAYS | FA_WRITE);
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
	clearDisplay(COLOR_BACKGROUND_DSO);

	drawText(FONT_WIDTH, FONT_HEIGHT, "Enter actual value", 2, COLOR_RED, COLOR_BACKGROUND_DSO);
	float tOriginalValue = getFloatFromRawValue(MeasurementControl.RawValueAverage);
	snprintf(StringBuffer, sizeof StringBuffer, "Current=%fV", tOriginalValue);
	drawText(FONT_WIDTH, 4 * FONT_HEIGHT, StringBuffer, 2, COLOR_BLACK, COLOR_INFO_BACKGROUND);

// wait for touch to become active
	while (!TouchPanel.wasTouched()) {
		delayMillis(10);
	}

	FeedbackToneOK();
	float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, COLOR_GUI_SOURCE_TIMEBASE);
// check for cancel
	if (!isnan(tNumber)) {
		float tOldValue = getRawAttenuationFactor(MeasurementControl.IndexInputRange);
		float tNewValue = tOldValue * (tNumber / tOriginalValue);
		RawAttenuationFactor[RangeToRawAttenuationIndexMapping[MeasurementControl.IndexInputRange]] = tNewValue;
		initRawToDisplayFactors();

		clearDisplay(COLOR_BACKGROUND_DSO);
		snprintf(StringBuffer, sizeof StringBuffer, "Old value=%f", tOldValue);
		drawText(FONT_WIDTH, FONT_HEIGHT, StringBuffer, 2, COLOR_BLACK, COLOR_INFO_BACKGROUND);
		snprintf(StringBuffer, sizeof StringBuffer, "New value=%f", tNewValue);
		drawText(FONT_WIDTH, 4 * FONT_HEIGHT, StringBuffer, 2, COLOR_BLACK, COLOR_INFO_BACKGROUND);

		// wait for touch to become active
		while (!TouchPanel.wasTouched()) {
			delayMillis(10);
		}
		FeedbackToneOK();
	}
	clearDisplay(COLOR_BACKGROUND_DSO);
	drawSettingsOnlyPartOfGui();
}

/***********************************************************************
 * GUI initialisation and drawing stuff
 ***********************************************************************/
void initDSOGUI(void) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"

	/*
	 * Control group
	 */

// initialize Main Home button
	TouchButtonMainHome->initSimpleButton(0, 0, BUTTON_WIDTH_5, BUTTON_HEIGHT_4, COLOR_GUI_CONTROL, StringHomeChar, 2, 1,
			&doMainMenuHomeButton);

// big start stop button
	TouchButtonStartStopDSOMeasurement = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_2,
	BUTTON_WIDTH_3, (2 * BUTTON_HEIGHT_4) + BUTTON_DEFAULT_SPACING, COLOR_GUI_CONTROL, "Start/Stop", 1, 0, &doStartStopDSO);

// Button for Singleshot
	TouchButtonSingleshot = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, 0, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
	COLOR_GUI_CONTROL, "Singleshot", 1, 0, &doTriggerSingleshot);

// Button for settings pages
	TouchButtonDSOSettings = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, 0, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
	COLOR_GUI_CONTROL, StringSettings, 1, 0, &doSettings);
// Back button for settings page
	TouchButtonBack = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
	BUTTON_HEIGHT_4, COLOR_GUI_CONTROL, StringBack, 1, 0, &doSettings);

	/*
	 * Settings page trigger offset + range group
	 */
// Button for slope page
	TouchButtonSlope = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, 0, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
	COLOR_GUI_TRIGGER, SlopeButtonString, 1, 0, &doTriggerSlope);

// Button for auto trigger on off
	TouchButtonAutoTriggerOnOff = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_2,
	BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_GUI_TRIGGER, AutoTriggerButtonStringAuto, 1, 0, &doTriggerMode);

// Button for auto offset on off
	TouchButtonAutoOffsetOnOff = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_3, BUTTON_WIDTH_3,
	BUTTON_HEIGHT_4, COLOR_GUI_TRIGGER, AutoOffsetButtonString0, 1, 0, &doAutoOffsetOnOff);

// AutoRange on off
	TouchButtonAutoRangeOnOff = TouchButton::allocAndInitSimpleButton(0, BUTTON_HEIGHT_4_LINE_3, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
	COLOR_GUI_TRIGGER, AutoRangeButtonStringAuto, 1, 0, &doRangeMode);

// Button for AC range
	TouchButtonACRange = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3 - BUTTON_WIDTH_4, 0, BUTTON_WIDTH_4, BUTTON_HEIGHT_4,
	COLOR_GUI_TRIGGER, ACRangeButtonStringAC, 2, 0, &doACRange);

// Button for ADS7846 channel
	TouchButtonADS7846Test = TouchButton::allocAndInitSimpleButton(0, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
	COLOR_GUI_SOURCE_TIMEBASE, StringSettingsButtonStringADS7846Test, 1, 0, &doADS7846Test);

// Buttons for voltage calibration
	TouchButtonCalibrateVoltage = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_3,
	BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_GUI_SOURCE_TIMEBASE, "Calibrate U", 1, 0, &doVoltageCalibration);

	/*
	 * Source + Load/Store group
	 */
// Button for channel select
	TouchButtonChannelSelect = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, 0, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
	COLOR_GUI_SOURCE_TIMEBASE, ADCInputMUXChannelStrings[MeasurementControl.IndexADCInputMUXChannel], 1,
			MeasurementControl.IndexADCInputMUXChannel, &doChannelSelect);

	TouchButtonStore = TouchButton::allocAndInitSimpleButton(0, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_5, BUTTON_HEIGHT_4,
	COLOR_GUI_SOURCE_TIMEBASE, StringStore, 1, MODE_STORE, &doStoreLoadAcquisitionData);
	TouchButtonLoad = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_5_POS_2, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_5,
	BUTTON_HEIGHT_4, COLOR_GUI_SOURCE_TIMEBASE, StringLoad, 1, MODE_LOAD, &doStoreLoadAcquisitionData);

	/*
	 * Display group
	 */
// Button for chart history (erase color)
	TouchButtonChartHistory = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
	BUTTON_HEIGHT_4, COLOR_GUI_DISPLAY_CONTROL, ChartHistoryButtonString, 1, 0, &doChartHistory);

// Button for info size channel
	TouchButtonInfoSize = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, 0, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
	COLOR_GUI_DISPLAY_CONTROL, SettingsButtonStringInfoSize, 1, 0, &doInfoSize);

// Button for switching draw mode - line/pixel
	TouchButtonDrawMode = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_3,
	BUTTON_HEIGHT_4, COLOR_GUI_DISPLAY_CONTROL, DrawModeButtonStringLine, 1, 0, &doDrawMode);

	TouchButtonDrawModeTriggerLine = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3 - BUTTON_WIDTH_4, BUTTON_HEIGHT_4_LINE_2,
	BUTTON_WIDTH_4, BUTTON_HEIGHT_4, COLOR_GUI_DISPLAY_CONTROL, DrawModeTriggerLineButtonStringOff, 1, 0, &doDrawModeTriggerLine);

// Button for switching display mode
	TouchButtonDisplayMode = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3,
	BUTTON_HEIGHT_4, COLOR_GUI_DISPLAY_CONTROL, "Display", 1, 0, &doDisplayMode);

	/*
	 * SLIDER
	 */
	TouchSlider::resetDefaults();
	/*
	 * Backlight slider
	 */
	TouchSliderBacklight.initSlider(0, 0, TOUCHSLIDER_DEFAULT_SIZE, BACKLIGHT_MAX_VALUE, BACKLIGHT_MAX_VALUE, getBacklightValue(),
	NULL, TOUCHSLIDER_DEFAULT_TOUCH_BORDER, TOUCHSLIDER_SHOW_BORDER, &doBacklightSlider, NULL);

// make slider slightly visible
	TouchSlider::setDefaultSliderColor(COLOR_DSO_SLIDER);
	TouchSlider::setDefaultBarColor(COLOR_BACKGROUND_DSO);

// invisible slider for voltage picker
	TouchSliderVoltagePicker.initSlider(SLIDER_VPICKER_X - FONT_WIDTH + 4, 0, 1, DISPLAY_VALUE_FOR_ZERO, DSO_DISPLAY_HEIGHT - 1, 0,
	NULL, 4 * TOUCHSLIDER_DEFAULT_TOUCH_BORDER, TOUCHSLIDER_VERTICAL_SHOW_NOTHING, &doVoltagePicker, NULL);

// invisible slider for trigger level
	TouchSliderTriggerLevel.initSlider(SLIDER_TLEVEL_X - 4, 0, 1, DISPLAY_VALUE_FOR_ZERO, DSO_DISPLAY_HEIGHT - 1, 0, NULL,
			4 * TOUCHSLIDER_DEFAULT_TOUCH_BORDER, TOUCHSLIDER_VERTICAL_SHOW_NOTHING, &doTriggerLevel, NULL);

#pragma GCC diagnostic pop
}

void activateCommonPartOfGui(void) {
	TouchSliderVoltagePicker.activate();
	TouchSliderVoltagePicker.drawBorder();

	TouchButtonMainHome->activate();

	TouchButtonDSOSettings->activate();
	TouchButtonStartStopDSOMeasurement->activate();
	TouchButtonDisplayMode->activate();
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

	TouchButtonChannelSelect->activate();
	TouchButtonChartHistory->activate();
}

/**
 * draws elements active while measurement is stopped and data is shown
 */
void drawCommonPartOfGui(void) {
	TouchSliderVoltagePicker.drawBorder();

	TouchButtonMainHome->drawButton();

	TouchButtonDSOSettings->drawButton();
	TouchButtonStartStopDSOMeasurement->drawButton();
	TouchButtonDisplayMode->drawButton();
}

/**
 * draws elements active while measurement is stopped and data is shown
 */
void drawAnalysisOnlyPartOfGui(void) {
	TouchButtonSingleshot->drawButton();
	TouchButtonStore->drawButton();
	TouchButtonLoad->drawButton();

	drawText(0, BUTTON_HEIGHT_4_LINE_3 + BUTTON_DEFAULT_SPACING, "\xAE-Scale-\xAF", 2, COLOR_YELLOW, COLOR_BACKGROUND_DSO);
	drawText(0, BUTTON_HEIGHT_4_LINE_4 + BUTTON_DEFAULT_SPACING, "\xAE-Scroll-\xAF", 2, COLOR_GREEN, COLOR_BACKGROUND_DSO);
}

/**
 * draws elements active while measurement is running
 */
void drawRunningOnlyPartOfGui(void) {
	if (MeasurementControl.TriggerMode == TRIGGER_MODE_MANUAL) {
		TouchSliderTriggerLevel.drawBorder();
	}

	TouchButtonChannelSelect->drawButton();
	TouchButtonDSOSettings->drawButton();
	TouchButtonChartHistory->drawButton();

	drawText(0, BUTTON_HEIGHT_4_LINE_4 + BUTTON_DEFAULT_SPACING, "\xAE-TB-\xAF", 2, COLOR_GUI_SOURCE_TIMEBASE,
	COLOR_BACKGROUND_DSO);
	if (!MeasurementControl.RangeAutomatic) {
		drawTextVertical(FONT_WIDTH, BUTTON_HEIGHT_4_LINE_2 - BUTTON_DEFAULT_SPACING, "\xD4Range\xD5", 2, COLOR_GUI_TRIGGER,
		COLOR_BACKGROUND_DSO);
	}
	if (MeasurementControl.OffsetAutomatic) {
		drawTextVertical(BUTTON_WIDTH_3_POS_3 - 2 * FONT_WIDTH, BUTTON_HEIGHT_4_LINE_2 - BUTTON_DEFAULT_SPACING, "\xD4Offset\xD5",
				2, COLOR_GUI_TRIGGER, COLOR_BACKGROUND_DSO);
	}
}

/**
 * draws elements active for settings page
 */
void drawSettingsOnlyPartOfGui(void) {
	TouchButtonACRange->drawButton();
	TouchButtonSlope->drawButton();
	TouchButtonInfoSize->drawButton();

	TouchButtonDrawModeTriggerLine->drawButton();
	TouchButtonAutoTriggerOnOff->drawButton();
	TouchButtonDrawMode->drawButton();

	TouchButtonAutoRangeOnOff->drawButton();
	TouchButtonAutoOffsetOnOff->drawButton();
	TouchButtonCalibrateVoltage->drawButton();

	TouchButtonADS7846Test->drawButton();
	TouchButtonChartHistory->drawButton();
	TouchButtonBack->drawButton();

	TouchSliderBacklight.drawSlider();
}

/**
 * Clears and redraws all elements according to IsRunning/DisplayMode
 * @param doClearbefore if display mode = SHOW_GUI_* the display is cleared anyway
 */
void redrawDisplay(bool doClearbefore) {
	TouchButton::deactivateAllButtons();
	TouchSlider::deactivateAllSliders();
	if (doClearbefore) {
		clearDisplay(COLOR_BACKGROUND_DSO);
	}
	if (MeasurementControl.IsRunning) {
		if (DisplayControl.DisplayMode == DISPLAY_MODE_SHOW_GUI_ELEMENTS) {
			// show gui elements
			drawCommonPartOfGui();
			drawRunningOnlyPartOfGui();
		} else {
			activateCommonPartOfGui();
			activateRunningOnlyPartOfGui();
			// show grid and labels
			drawGridLinesWithHorizLabelsAndTriggerLine(COLOR_GRID_LINES);

			if (DisplayControl.DisplayMode == DISPLAY_MODE_INFO_LINE) {
				// info
				printInfo();
			}
		}
	} else {
		// measurement stopped -> analysis mode
		if (DisplayControl.DisplayMode >= DISPLAY_MODE_SHOW_GUI_ELEMENTS) {
			// show analysis gui elements
			drawCommonPartOfGui();
			drawAnalysisOnlyPartOfGui();
		} else {
			activateCommonPartOfGui();
			activateAnalysisOnlyPartOfGui();
			// show grid and labels and chart
			drawGridLinesWithHorizLabelsAndTriggerLine(COLOR_GRID_LINES);
			drawDataBuffer(COLOR_DATA_HOLD, DataBufferControl.DataBufferDisplayStart, 0);

			if (DisplayControl.DisplayMode == DISPLAY_MODE_INFO_LINE) {
				// info and analysis lines
				printInfo();
				drawMinMaxLines();
			}
		}

	}
}

