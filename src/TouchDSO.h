/**
 * TouchDSO.h
 *
 * @date 20.12.2012
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
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

#ifdef LOCAL_DISPLAY_EXISTS
#include "ADS7846.h"
#endif

#include <stdint.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <arm_math.h> // for float32_t
#pragma GCC diagnostic pop

/*******************************************************************************************
 * Function declaration section
 *******************************************************************************************/
void initDSO(void);
void startDSO(void);
void loopDSO(void);
void stopDSO(void);

void autoACZeroCalibration(void);
void resetAcquisition(void);
void initAcquisition(void);
void startAcquisition(void);
void readADS7846Channels(void);

void changeTimeBase(bool aForceSetPrescaler);

void setOffsetGridCount(int aOffsetGridCount);
void computeAutoTrigger(void);
void computeAutoInputRange(void);
void computeAutoDisplayRange(void);
void computeAutoOffset(void);

void computePeriodFrequency(void);

void setTriggerLevelAndHysteresis(int aRawTriggerValue, int aRawTriggerHysteresis);
bool changeInputRange(int aValue);
int changeDisplayRange(int aValue);
bool setDisplayRange(int aNewDisplayRangeIndex, bool aClipToIndexInputRange);
void adjustPreTriggerBuffer(uint16_t* tTempBuffer);
uint16_t computeNumberOfSamplesToTimeout(uint16_t aTimebaseIndex);
void computeMinMaxAverageAndPeriodFrequency(void);
bool setInputRange(int aValue);
void setACMode(bool aACRangeEnable);

void drawGridLinesWithHorizLabelsAndTriggerLine(uint16_t aColor);
//void drawHorizontalLineLabels(void);
void drawMinMaxLines(void);

void drawTriggerLine(void);
void clearTriggerLine(uint8_t aTriggerLevelDisplayValue);
void printTriggerInfo(void);

void printInfo(void);
void clearInfo(void);
void clearDiplayedChart(void);
void drawDataBuffer(uint16_t *aDataBufferPointer, int aLength, uint16_t aColor, uint16_t aClearBeforeColor);
void drawRemainingDataBufferValues(uint16_t aDrawColor);

void initScaleValuesForDisplay(void);
void testDSOConversions(void);
int getDisplayFrowRawInputValue(int aAdcValue);
int getDisplayFrowMultipleRawValues(uint16_t * aAdcValuePtr, int aCount);

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
#define INFO_UPPER_MARGIN (1 + TEXT_SIZE_11_ASCEND)
#define INFO_LEFT_MARGIN 4

// Timebase stuff
#define CHANGE_REQUESTED_TIMEBASE 0x01
#define TIMEBASE_FAST_MODES 7 // first modes are fast DMA modes
#define TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE 17 // min index where chart is drawn while buffer is filled
#define TIMEBASE_NUMBER_OF_ENTRIES 21 // the number of different timebase provided - 1. entry is not uses until interleaved acquisition is implemented
#define TIMEBASE_NUMBER_OF_EXCACT_ENTRIES 8 // the number of exact float value for timebase because of granularity of clock division
#define TIMEBASE_NUMBER_OF_XSCALE_CORRECTION 5  // number of timebase which are simulated by display XSale factor
#define TIMEBASE_INDEX_MILLIS 11 // min index to switch to ms instead of ns display
#define TIMEBASE_INDEX_MICROS 2 // min index to switch to us instead of ns display
#define TIMEBASE_INDEX_START_VALUE 12
extern const uint8_t xScaleForTimebase[TIMEBASE_NUMBER_OF_XSCALE_CORRECTION];
extern const uint16_t TimebaseDivValues[TIMEBASE_NUMBER_OF_ENTRIES];
extern const float TimebaseExactDivValuesMicros[TIMEBASE_NUMBER_OF_EXCACT_ENTRIES];
extern const uint16_t TimebaseTimerDividerValues[TIMEBASE_NUMBER_OF_ENTRIES];
extern const uint16_t TimebaseTimerPrescalerDividerValues[TIMEBASE_NUMBER_OF_ENTRIES];
extern const uint16_t ADCClockPrescalerValues[TIMEBASE_NUMBER_OF_ENTRIES];
float getTimebaseExactValueMicros(int8_t aTimebaseIndex);

// Triggering stuff
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
// Range Stuff
#define NUMBER_OF_ADC_RANGES 6 // from 0.06V total - 0,02V to 3V total - 0.5V/div (40 pixel)
#define NUMBER_OF_RANGES (NUMBER_OF_ADC_RANGES + 6)  // up to 120V total - 40V/div
#define NUMBER_OF_HARDWARE_RANGES 5  // different hardware attenuator levels = *2 , *1 , /4, /40, /100
extern const float ScaleVoltagePerDiv[NUMBER_OF_RANGES];
extern const uint8_t RangePrecision[NUMBER_OF_RANGES];
#define DSO_SCALE_FACTOR_SHIFT ADC_SCALE_FACTOR_SHIFT  // 18  2**18 = 0x40000 or 262144
extern int ScaleFactorRawToDisplayShift18[NUMBER_OF_RANGES];
#define DSO_INPUT_TO_DISPLAY_SHIFT 12 // 14  2**12 = 0x1000 or 4096
extern float actualDSORawToVoltFactor;

// Offset
#define OFFSET_MODE_0_VOLT 0
#define OFFSET_MODE_AUTOMATIC 1 // Changing input + display range in this mode makes no sense.
#define OFFSET_MODE_MANUAL 2    // Mode for changing display range and offset.
// Attenuator stuff
#define DSO_ATTENUATOR_BASE_GAIN 2 // Gain if attenuator is at level 1
#define DSO_ATTENUATOR_SHORTCUT 0 // line setting for attenuator makes shortcut to ground
extern bool isAttenuatorAvailable;
extern float RawAttenuationFactor[NUMBER_OF_RANGES];
extern const uint8_t AttenuatorHardwareValue[NUMBER_OF_RANGES];
extern int FactorFromInputToDisplayRangeShift12;
extern uint16_t RawDSOReadingACZero;

// ADC channel stuff
#define START_ADC_CHANNEL_INDEX 0  // see also ChannelSelectButtonString
#define ADC_CHANNEL_NO_ATTENUATOR_INDEX 2  // see also ChannelSelectButtonString
#define NO_ATTENUATOR_MIN_RANGE_INDEX 1
#define NO_ATTENUATOR_MAX_RANGE_INDEX 5
#define ADC_CHANNEL_COUNT 6 // The number of ADC channel
extern const char * const ADCInputMUXChannelStrings[ADC_CHANNEL_COUNT];
extern char ADCInputMUXChannelChars[ADC_CHANNEL_COUNT];
extern uint8_t ADCInputMUXChannels[ADC_CHANNEL_COUNT];

#define FFT_SIZE 256 // for radix4 - can be 64, 256, 1024 etc.
void computeFFT(uint16_t * aDataBufferPointer, float32_t *aFFTBuffer);
void draw128FFTValuesFast(uint16_t aColor, uint16_t * aDataBufferPointer);
void clearFFTValuesOnDisplay(void);
void drawFFT(void);
extern uint8_t DisplayBufferFFT[FFT_SIZE / 2];

struct MeasurementControlStruct {
    bool isRunning;
    volatile uint8_t ChangeRequestedFlags; // GUI (Event) -> Thread (main loop) - change of clock prescaler requested from GUI
    volatile bool StopRequested; // GUI -> Thread
    volatile bool StopAcknowledged; // true if DMA made its last acquisition before stop
    // Info size
    bool InfoSizeSmall;
    // Input select
#ifdef LOCAL_DISPLAY_EXISTS
    bool ADS7846ChannelsAsDatasource;
#endif
    volatile bool TriggerPhaseJustEnded; // ADC-ISR -> Thread - signal for draw while acquire

    // Read phase for ISR and single shot mode see SEGMENT_...
    volatile uint8_t TriggerActualPhase; // ADC-ISR internal and -> Thread
    volatile bool isSingleShotMode; // GUI
    volatile bool doPretriggerCopyForDisplay; // signal from loop to DMA ISR to copy the pre trigger area for display - useful for single shot

    // Trigger
    volatile bool TriggerSlopeRising; // GUI -> ADC-ISR
    volatile uint16_t RawTriggerLevel; // GUI -> ADC-ISR
    uint16_t RawTriggerLevelHysteresis; // ADC-ISR internal

    uint8_t TriggerMode; // GUI -> ADC-ISR - TRIGGER_MODE_AUTOMATIC, MANUAL, OFF
    uint8_t OffsetMode; //OFFSET_MODE_0_VOLT, OFFSET_MODE_AUTOMATIC, OFFSET_MODE_MANUAL
    uint8_t TriggerStatus; // Set by ISR: see TRIGGER_START etc.
    uint16_t TriggerSampleCount; // ISR: for checking trigger timeout
    uint16_t TriggerTimeoutSampleOrLoopCount; // ISR max samples / DMA max number of loops before trigger timeout
    uint16_t RawValueBeforeTrigger; // only single shot mode: to show actual value during wait for trigger

    // computed values from display buffer
    float PeriodMicros;
    uint32_t FrequencyHertz;
    float FrequencyHertzAtMaxFFTBin;
    float MaxFFTValue;

    // Statistics (for auto range/offset/trigger)
    uint16_t RawValueMin;
    uint16_t RawValueMax;

    uint16_t RawValueAverage;

    // Timebase
    bool TimebaseFastDMAMode;
    int8_t TimebaseNewIndex; // set by touch handler
    int8_t TimebaseIndex;
    uint8_t ADCInputMUXChannelIndex;

    // Range
    bool RangeAutomatic; // [RANGE_MODE_AUTOMATIC, MANUAL]
    bool isACMode; // false: unipolar mode => 0V probe input -> 0V ADC input  - true: AC range => 0V probe input -> 1.5V ADC input
    uint16_t RawDSOReadingACZero;
    int InputRangeIndex; // index including attenuator ranges  [0 to NUMBER_OF_RANGES]
    uint32_t TimestampLastRangeChange;

    // Offset
    // Offset in ADC Reading. Is multiple of reading / div
    signed short RawOffsetValueForDisplayRange; // to be subtracted from adjusted (by FactorFromInputToDisplayRange) raw value
    uint16_t RawValueOffsetClippingLower; // ADC raw lower clipping value for offset mode
    uint16_t RawValueOffsetClippingUpper; // ADC raw upper clipping value for offset mode
    // number of lowest horizontal grid to display for auto offset
    int16_t OffsetGridCount;
    int DisplayRangeIndex; // = InputRangeIndex if offset is zero else it may be smaller to magnify signal  [0 to NUMBER_OF_RANGES]
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
extern unsigned int sDatabufferPreDisplaySize;
#define DATABUFFER_DISPLAY_START (DATABUFFER_PRE_TRIGGER_SIZE - DisplayControl.DatabufferPreTriggerDisplaySize)
#define DATABUFFER_DISPLAY_END (DATABUFFER_DISPLAY_START + DSO_DISPLAY_WIDTH - 1)
#define DATABUFFER_POST_TRIGGER_START (&DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE])
#define DATABUFFER_POST_TRIGGER_SIZE (DATABUFFER_SIZE - DATABUFFER_PRE_TRIGGER_SIZE)
#define DATABUFFER_INVISIBLE_RAW_VALUE 0x1000 // Value for invalid data in/from pretrigger area
#define DISPLAYBUFFER_INVISIBLE_VALUE 0xFF // Value for invisible data in display buffer. Used if raw value was DATABUFFER_INVISIBLE_RAW_VALUE
struct DataBufferStruct {
    volatile bool DataBufferFull; // ISR -> main loop
    bool DrawWhileAcquire;
    volatile bool DataBufferPreTriggerAreaWrapAround; // ISR -> draw-while-acquire mode
    uint8_t InputRangeIndexUsed; // index used for acquisition of buffer data

    uint16_t * DataBufferPreTriggerNextPointer; // pointer to next pre trigger value in DataBuffer - set only once at end of search trigger phase
    uint16_t * DataBufferNextInPointer; // used by ISR as main databuffer pointer - also read by draw-while-acquire mode
    volatile uint16_t * DataBufferNextDrawPointer; // for draw-while-acquire mode
    uint16_t NextDrawXValue; // for draw-while-acquire mode
    // to detect end of acquisition in interrupt service routine
    volatile uint16_t * DataBufferEndPointer; // pointer to last valid data in databuffer | Thread -> ISR (for stop)

    // Pointer for horizontal scrolling - use value 2 divs before trigger point to show pre trigger values
    uint16_t * DataBufferDisplayStart;
    /**
     * consists of 2 regions - first pre trigger region, second data region
     * display region starts in pre trigger region
     */
    uint16_t DataBuffer[DATABUFFER_SIZE];
};
extern struct DataBufferStruct DataBufferControl;

/*
 * Display control
 * while running switch between upper info line on/off
 * while stopped switch between chart / t+info line and gui
 */

/*
 * only grid and chart and trigger line
 * + info (and min + max lines)
 * + show active gui elements
 */
enum InfoModeEnum {
    NO_INFO, LONG_INFO
};

enum DisplayPageEnum {
    START, CHART, SETTINGS, MORE_SETTINGS
};

// Modes for DisplayBufferDrawMode
#define DRAW_MODE_LINE 0x01    // draw as line - otherwise draw only measurement pixel
#define DRAW_MODE_TRIGGER 0x02 // Trigger state is displayed
#define SCALE_CHANGE_DELAY_MILLIS 2000
#define DRAW_HISTORY_LEVELS 4 // 0 = No history, 3 = history high
struct DisplayControlStruct {
    uint8_t TriggerLevelDisplayValue; // For clearing old line of manual trigger level setting
    uint8_t DisplayBufferDrawMode;
    InfoModeEnum showInfoMode; // ONLY_DATA | PLUS_INFO
    DisplayPageEnum DisplayPage; // START, CHART, SETTINGS, MORE_SETTINGS
    bool ShowFFT;

    /**
     * XScale > 1 : expansion by factor XScale
     * XScale == 1 : expansion by 1.5
     * XScale == 0 : identity
     * XScale == -1 : compression by 1.5
     * XScale < -1 : compression by factor -XScale
     */
    int8_t XScale; // Factor for X Data expansion(>0) or compression(<0). 2->display 1 value 2 times -2->display average of 2 values etc.
    uint16_t DisplayIncrementPixel; // corresponds to XScale

    unsigned int DatabufferPreTriggerDisplaySize;

    // for recognizing that MeasurementControl values changed => clear old grid labels
    int16_t LastOffsetGridCount;
    int LastDisplayRangeIndex;

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
#define COLOR_DATA_RUN_CLIPPING COLOR_RED
#define COLOR_DATA_TRIGGER COLOR_BLACK		// color for trigger status line
#define COLOR_DATA_PRETRIGGER COLOR_GREEN   // color for pre trigger data in draw while acquire mode
#define COLOR_FFT_DATA COLOR_BLUE
#define COLOR_DATA_HOLD COLOR_RED
#define COLOR_GRID_LINES RGB(0x00,0x98,0x00)
#define COLOR_INFO_BACKGROUND RGB(0xC8,0xC8,0x00)

// to see old chart values
#define COLOR_DATA_ERASE_LOW RGB(0xC0,0xFF,0xC0)
#define COLOR_DATA_ERASE_MID RGB(0x80,0xE0,0x80)
#define COLOR_DATA_ERASE_HIGH RGB(0x40,0xC0,0x40)

struct FFTInfoStruct {
    float MaxValue; // max bin value for y scaling
    int MaxIndex;   // index of MaxValue
    uint32_t TimeElapsedMillis; // milliseconds of computing last fft
};
extern FFTInfoStruct FFTInfo;
extern uint8_t DisplayBuffer[DSO_DISPLAY_WIDTH];

#endif /* SIMPLETOUCHSCREENDSO_H_ */
