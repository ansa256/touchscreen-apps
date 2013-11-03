/**
 * TouchDSO.h
 *
 * @date 20.12.2012
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 *      Features:
 *      No dedicated hardware, just off the shelf components + c software
 *      --- 3 MSamples per second
 *      Automatic or manual range, trigger and offset value selection
 *      --- currently 10 screens data buffer
 *      Min, max, peak to peak and average display
 *      All settings can be changed during measurement by touching the (invisible) buttons
 *      --- 3 external + 3 internal channels (VBatt/2, VRefint + Temp) selectable
 *      Single shot function
 *      Display of pre trigger values
 *
 *      Build on:
 *      STM32F3-DISCOVERY http://www.watterott.com/de/STM32F3DISCOVERY      16 EUR
 *      HY32D LCD Display http://www.ebay.de/itm/250906574590               12 EUR
 *      Total                                                               28 EUR
 *
 */

#ifndef SIMPLETOUCHSCREENDSO_H_
#define SIMPLETOUCHSCREENDSO_H_

#include <stdint.h>

/*******************************************************************************************
 * Function declaration section
 *******************************************************************************************/
void initDSO(void);
void startDSO(void);
void loopDSO(void);
void stopDSO(void);

void autoACZeroCalibration(void);
void initAcquisition(void);
void startAcquisition(void);
void readADS7846Channels(void);
void acquireDataByDMA(void);

float getRawAttenuationFactor(int rangeIndex);
void setOffsetGridCount(int aOffsetGridCount);
void computeAutoTrigger(void);
void computeAutoRange(void);
void computeAutoOffset(void);
void setLevelAndHysteresis(int aRawTriggerValue, int aRawTriggerHysteresis);
bool changeInputRange(int aValue);
void adjustPreTriggerBuffer(uint16_t* tTempBuffer);
uint16_t computeNumberOfSamplesToTimeout(uint16_t aTimebaseIndex);
void findMinMaxAverage(void);

void drawGridLinesWithHorizLabelsAndTriggerLine(uint16_t aColor);
//void drawHorizontalLineLabels(void);
void drawMinMaxLines(void);

void drawTriggerLine(void);
void clearTriggerLine(uint8_t aTriggerLevelDisplayValue);
void printTriggerInfo(void);

void printInfo(void);
void clearInfo(const uint8_t aInfoLineNumber);
void drawDataBuffer(uint16_t aColor, uint16_t *aDataBufferPointer, uint16_t aClearBeforeColor);
void drawRemainingDataBufferValues(uint16_t aDrawColor);

void initScaleValuesForDisplay(void);
void testDSOConversions(void);
int getDisplayFrowRawInputValue(int aAdcValue);
int getDisplayFrowMultipleRawValues(uint16_t * aAdcValuePtr, int aCount);
int getXScaleCorrectedValue(int aValue);

void initRawToDisplayFactors(void);
int getRawOffsetValueFromGridCount(int aCount);
int getInputRawFromDisplayValue(int aValue);
float getFloatFromRawValue(int aValue);
float getFloatFromDisplayValue(uint8_t aValue);

/**********************
 * Display layout
 **********************/
#define DSO_DISPLAY_HEIGHT 240
#define DSO_DISPLAY_WIDTH 320
#define HORIZONTAL_GRID_COUNT 6
#define HORIZONTAL_GRID_HEIGHT (DSO_DISPLAY_HEIGHT / HORIZONTAL_GRID_COUNT) // 40
#define TIMING_GRID_WIDTH (DSO_DISPLAY_WIDTH / 10) // 32
#define DISPLAY_AC_ZERO_OFFSET_GRID_COUNT (-3)
#define DISPLAY_VALUE_FOR_ZERO (DSO_DISPLAY_HEIGHT - 2) // Zero line is not exactly at bottom of display to improve readability

#define INFO_UPPER_MARGIN 1
#define INFO_LEFT_MARGIN 4

#define SLIDER_VPICKER_X (22 * FONT_WIDTH)
#define SLIDER_TLEVEL_X (19 * FONT_WIDTH)

#define TRIGGER_LEVEL_INFO_LARGE_X INFO_LEFT_MARGIN + (11 * FONT_WIDTH)
#define TRIGGER_LEVEL_INFO_SMALL_X (INFO_LEFT_MARGIN + (33 * FONT_WIDTH))

#define TRIGGER_HIGH_DISPLAY_OFFSET 7 // for trigger state line

/******************************
 * Measurement control values
 ******************************/
#define CHANGE_REQUESTED_TIMEBASE 0x01

#define TIMEBASE_FAST_MODES 5 // first modes are fast free running modes
#define TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE 15 // min index where chart is drawn while buffer is filled
#define TIMEBASE_NUMBER_OF_ENTRIES 19 // the number of different timebase provided
#define TIMEBASE_NUMBER_OF_EXCACT_ENTRIES 6 // the number of exact float value for timebase because of granularity of clock division
#define TIMEBASE_NUMBER_OF_XSCALE_CORRECTION 3  // number of timebase which are simulated by display XSale factor
#define TIMEBASE_INDEX_MILLIS 9 // min index to switch to ms instead of us display
#define TIMEBASE_INDEX_START_VALUE 10
extern const uint8_t xScaleForTimebase[TIMEBASE_NUMBER_OF_XSCALE_CORRECTION];
extern const uint16_t TimebaseDivValues[TIMEBASE_NUMBER_OF_ENTRIES];
extern const float TimebaseExactDivValues[TIMEBASE_NUMBER_OF_EXCACT_ENTRIES];
extern const uint16_t TimebaseTimerDividerValues[TIMEBASE_NUMBER_OF_ENTRIES];
extern const uint16_t TimebaseTimerPrescalerDividerValues[TIMEBASE_NUMBER_OF_ENTRIES];
extern const uint16_t ADCClockPrescalerValues[TIMEBASE_NUMBER_OF_ENTRIES];

/****************************************
 * Automatic triggering and range stuff
 ****************************************/
#define TRIGGER_MODE_AUTOMATIC 0
#define TRIGGER_MODE_MANUAL 1
#define TRIGGER_MODE_OFF 2
#define TRIGGER_HYSTERESIS_MANUAL 2 // value for effective trigger hysteresis in manual trigger mode
// States of tTriggerStatus
#define TRIGGER_START 0 // No trigger condition met
#define TRIGGER_BEFORE_THRESHOLD 1 // slope condition met, wait to go beyond threshold hysteresis
#define TRIGGER_OK 2 // Trigger condition met
#define PHASE_PRE_TRIGGER 0 // load pre trigger values
#define PHASE_SEARCH_TRIGGER 1 // wait for trigger condition
#define PHASE_POST_TRIGGER 2 // trigger found -> acquire data
#define TRIGGER_TIMEOUT_MILLIS 200 // Milliseconds to wait for trigger
#define TRIGGER_TIMEOUT_MIN_SAMPLES (6 * TIMING_GRID_WIDTH) // take at least this amount of samples to find trigger
#define DSO_ATTENUATOR_BASE_GAIN 2 // Gain if attenuator is at level 1
#define DSO_ATTENUATOR_SHORTCUT 0 // line setting for attenuator makes shortcut to ground
#define NUMBER_OF_ADC_RANGES 6 // from 0.06V total - 0,02V to 3V total - 0.5V/div (40 pixel)
#define NUMBER_OF_RANGES (NUMBER_OF_ADC_RANGES + 6)  // up to 120V total - 40V/div
#define NUMBER_OF_HARDWARE_RANGES 5  // different hardware attenuator levels = *2 , *1 , /4, /40, /100
extern const float ScaleVoltagePerDiv[NUMBER_OF_RANGES];
extern const uint8_t RangePrecision[NUMBER_OF_RANGES];
#define DSO_SCALE_FACTOR_SHIFT ADC_SCALE_FACTOR_SHIFT  // 18  2**18 = 0x40000 or 262144
extern int ScaleFactorRawToDisplayShift18[NUMBER_OF_RANGES];
extern float RawAttenuationFactor[NUMBER_OF_HARDWARE_RANGES];
extern const uint8_t RangeToRawAttenuationIndexMapping[NUMBER_OF_RANGES];
extern const uint8_t AttenuatorHardwareValue[NUMBER_OF_RANGES];
extern float actualDSORawToVoltFactor;

#define DSO_INPUT_TO_DISPLAY_SHIFT 12 // 14  2**12 = 0x1000 or 4096
extern int FactorFromInputToDisplayRangeShift12;

extern uint16_t RawDSOReadingACZero;

struct MeasurementControlStruct { // Used by:
	bool IsRunning;
	volatile uint8_t ChangeRequestedFlags; // Gui ISR -> main loop - change of clock prescaler requested from GUI
	volatile bool StopRequested; // Gui ISR -> main loop
	volatile bool StopAcknowledged; // true if DMA made its last acquisition before stop
	// Info size
	bool InfoSizeSmall;
	// Input select
	bool ADS7846ChannelsActive;

	volatile bool TriggerPhaseJustEnded; // ISR -> main loop - signal for draw while acquire

	// Read phase for ISR and single shot mode see SEGMENT_...
	uint8_t ActualPhase; // ISR
	bool SingleShotMode; // ISR + GUI

	// Trigger
	bool TriggerSlopeRising; // ISR
	volatile uint16_t RawTriggerLevel; // ISR + GUI
	uint16_t RawTriggerLevelHysteresis; // ISR

	uint8_t TriggerMode; // Thread TRIGGER_MODE_AUTOMATIC, MANUAL, OFF
	bool OffsetAutomatic; // Thread offset = 0 Volt
	uint8_t TriggerStatus; // ISR see TRIGGER_START etc.
	uint16_t TriggerSampleCount; // ISR for checking trigger timeout
	uint16_t TriggerTimeoutSampleOrLoopCount; // ISR max samples / DMA max number of loops  before trigger timeout
	uint16_t RawValueBeforeTrigger; // only single shot mode: to show actual value during wait for trigger

	// computed values from display buffer
	float PeriodMicros;
	uint16_t FrequencyHertz;

	// Statistics (for auto range/offset/trigger)
	uint16_t RawValueMin;
	uint16_t RawValueMax;
	uint16_t RawValueAverage;

	// Timebase
	bool TimebaseFastDMAMode;
	int8_t NewIndexTimebase; // set by touch handler
	int8_t IndexTimebase;
	uint8_t IndexADCInputMUXChannel;

	int actualDSOReadingACZeroForInputRange; // factor according to input range = RawDSOReadingACZero * RawAttenuationFactor[MeasurementControl.IndexInputRange];

	// Range
	bool RangeAutomatic; // RANGE_MODE_AUTOMATIC, MANUAL
	bool ACRange; // false: unipolar range => 0V probe input -> 0V ADC input  - true: AC range => 0V probe input -> 1.5V ADC input
	int IndexInputRange; // index including attenuator ranges 0 -> NUMBER_OF_RANGES
	uint32_t TimestampLastRangeChange;

	// Offset
	// Offset in ADC Reading. Is multiple of reading / div
	signed short RawOffsetValueForDisplayRange; // to be subtracted from adjusted (by FactorFromInputToDisplayRange) raw value
	uint16_t RawValueOffsetClippingLower; // ADC raw lower clipping value for offset mode
	uint16_t RawValueOffsetClippingUpper; // ADC raw upper clipping value for offset mode
	// number of lowest horizontal grid to display for auto offset
	int16_t OffsetGridCount;
	int IndexDisplayRange; // == InputRangeIndex if offset is zero else smaller to magnify signal in conjunction with  0 -> NUMBER_OF_RANGES
	bool InputRangeIndexOtherThanDisplayRange;
};
extern struct MeasurementControlStruct MeasurementControl;

/*
 * Data buffer
 */
#define DATABUFFER_DISPLAY_RESOLUTION_FACTOR 10
#define DATABUFFER_DISPLAY_RESOLUTION (DSO_DISPLAY_WIDTH / DATABUFFER_DISPLAY_RESOLUTION_FACTOR)     // Base value for other (32)
#define DATABUFFER_DISPLAY_INCREMENT DATABUFFER_DISPLAY_RESOLUTION // increment value for display scroll
#define DATABUFFER_SIZE_FACTOR 10
#define DATABUFFER_SIZE (DSO_DISPLAY_WIDTH * DATABUFFER_SIZE_FACTOR)
#define DATABUFFER_PRE_TRIGGER_SIZE (5 * DATABUFFER_DISPLAY_RESOLUTION)
#define DATABUFFER_PRE_TRIGGER_DISPLAY_SIZE (2 * DATABUFFER_DISPLAY_RESOLUTION)
//#define DATABUFFER_POST_TRIGGER_DISPLAY_SIZE ((10 - 2) * DATABUFFER_DISPLAY_INCREMENT)
#define DATABUFFER_DISPLAY_START (DATABUFFER_PRE_TRIGGER_SIZE - DATABUFFER_PRE_TRIGGER_DISPLAY_SIZE)
#define DATABUFFER_DISPLAY_END (DATABUFFER_DISPLAY_START + DSO_DISPLAY_WIDTH - 1)
#define DATABUFFER_POST_TRIGGER_START (&DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE])
#define DATABUFFER_POST_TRIGGER_SIZE (DATABUFFER_SIZE - DATABUFFER_PRE_TRIGGER_SIZE)

struct DataBufferStruct {
	volatile bool DataBufferFull; // ISR -> main loop
	bool DrawWhileAcquire;
	volatile bool DataBufferWrapAround; // ISR -> draw-while-acquire mode
	uint8_t IndexInputRangeUsed; // index used for acquisition of buffer data

	uint16_t * DataBufferPreTriggerNextPointer; // pointer to next pre trigger value in DataBuffer - set only once at end of search trigger phase
	uint16_t * DataBufferNextPointer; // set by ISR - read by draw-while-acquire mode
	volatile uint16_t * DataBufferNextDrawPointer; // for draw-while-acquire mode
	uint16_t NextDrawXValue; // for draw-while-acquire mode
	// to detect end of acquisition in interrupt service routine
	volatile uint16_t * DataBufferEndPointer; // pointer to last valid data in databuffer | Thread -> ISR (for stop)

	// Pointer for horizontal scrolling
	uint16_t * DataBufferDisplayStart;
	/**
	 * consists of 2 regions - first pre trigger region, second data region
	 * display region starts in pre trigger region
	 */
	uint16_t DataBuffer[DATABUFFER_SIZE];
};
extern struct DataBufferStruct DataBufferControl;

/**
 * ADC channel infos
 */
#define START_ADC_CHANNEL_INDEX 0  // see also ChannelSelectButtonString
#define ADC_CHANNEL_COUNT 6 // The number of ADC channel
extern char * ADCInputMUXChannelStrings[ADC_CHANNEL_COUNT];
extern char ADCInputMUXChannelChars[ADC_CHANNEL_COUNT];
extern uint8_t ADCInputMUXChannels[ADC_CHANNEL_COUNT];

/*
 * Display control
 * while running switch between upper info line on/off
 * while stopped switch between chart / t+info line and gui
 */
#define DISPLAY_MODE_ONLY_DATA 0 // only grid and chart and trigger line
#define DISPLAY_MODE_INFO_LINE 1 // + info (and min + max lines)
#define DISPLAY_MODE_SHOW_GUI_ELEMENTS 2  // show active gui elements
#define DISPLAY_MODE_SETTINGS 3  // Settings menu
// Modes for DisplayBufferDrawMode
#define DRAW_MODE_LINE 0x01    // draw as line - otherwise draw only measurement pixel
#define DRAW_MODE_TRIGGER 0x02 // Trigger state is displayed
#define SCALE_CHANGE_DELAY_MILLIS 2000
#define DRAW_HISTORY_LEVELS 4
struct DisplayControlStruct {
	uint8_t TriggerLevelDisplayValue; // For clearing old line of manual trigger level setting
	uint8_t DisplayBufferDrawMode;
	uint8_t DisplayMode;
	int8_t XScale; // 2 means 2 Pixel per data value, -2 means 2 data values per pixel
	uint16_t DisplayIncrementPixel; // corresponds to XScale
	uint16_t EraseColors[DRAW_HISTORY_LEVELS];
	uint16_t EraseColor;
	uint8_t EraseColorIndex;
};
extern DisplayControlStruct DisplayControl;

/*
 * COLORS
 */
#define COLOR_BACKGROUND_DSO COLOR_WHITE
// Data colors
#define COLOR_DATA_RUN COLOR_BLUE
#define COLOR_DATA_TRIGGER COLOR_BLACK		// color for trigger status line
#define COLOR_DATA_PRETRIGGER COLOR_GREEN   // color for pre trigger data in draw while acquire mode
#define COLOR_DATA_HOLD COLOR_RED
#define COLOR_GRID_LINES RGB(0x00,0x98,0x00)
#define COLOR_INFO_BACKGROUND RGB(0xC8,0xC8,0x00)

// to see old chart values
#define COLOR_DATA_ERASE_LOW RGB(0xC0,0xFF,0xC0)
#define COLOR_DATA_ERASE_MID RGB(0x80,0xE0,0x80)
#define COLOR_DATA_ERASE_HIGH RGB(0x40,0xC0,0x40)
#endif /* SIMPLETOUCHSCREENDSO_H_ */
