/**
 * TouchDSOAcquisition.cpp
 *
 * @date 29.03.2012
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 *		2 acquisition methods are implemented (all triggered by timer)
 *		1. Fast
 *      	Data is copied by DMA
 *      	    During copy, the filled data buffer is searched for trigger condition.
 *      	    If no trigger condition met, DMA is restarted and search continues.
 *      	    Timeout by TriggerTimeoutSampleOrLoopCount.
 *		2. Normal
 *			Data is acquired by interrupt service routine (ISR)
 *      	Acquisitions has 3 phases
 *      		1. One pre trigger buffer is filled with DATABUFFER_PRE_TRIGGER_SIZE samples (not in draw while acquire display mode!)
 *      		2. Trigger condition is searched and cyclic pre trigger buffer is filled -> Timeout by TriggerTimeoutSampleOrLoopCount
 *      		3. Data buffer is filled with data -> Size is DISPLAY_WIDTH - DATABUFFER_DISPLAY_SHOW_PRE_TRIGGER_SIZE while running
 *      											or DATABUFFER_SIZE - DATABUFFER_PRE_TRIGGER_SIZE if stopped,

 *
 *		2 display methods implemented
 *		1. Normal for fast timebase values
 *			Acquisition stops after buffer full.
 *			Main loop aligns pre trigger area.
 *			New range, trigger, offset and average are computed
 *			Data is displayed.
 *			New acquisition is started.
 *		2. Draw while acquire for slow timebase
 *			As long as data is written to buffer it is displayed on screen by the main loop.
 *			If trigger condition is met, pre trigger area is aligned and redrawn.
 *			If stop is requested, samples are acquired until end of display buffer reached.
 *			If stop is requested twice, sampling is stopped immediately if end of display reached.
 *
 *		Timing of a complete acquisition and display cycle is app. 5-10ms /200-100Hz
 *		depending on display content and with compiler option -Os (optimize size)
 *		or 17 ms / 60Hz unoptimized
 */

#include "Pages.h"

#include "Chart.h" // for adjustIntWithScaleFactor()
#include "TouchDSO.h"
#include "stdlib.h"
#include <string.h> // for memcpy
extern "C" {
#include "stm32f3_discovery.h"  // For LEDx
}

/*****************************
 * Timebase stuff
 *****************************/

// first 3 entries are realized by xScale > 1 and not by higher ADC clock -> see TIMEBASE_INDEX_MILLIS
const uint8_t xScaleForTimebase[TIMEBASE_NUMBER_OF_XSCALE_CORRECTION] = { 31, 12, 6, 3, 2 }; // for GUI and frequency
const uint16_t TimebaseDivValues[TIMEBASE_NUMBER_OF_ENTRIES] = { 200, 500, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1, 2, 5, 10, 20, 50,
        100, 200, 500, 1000 }; // only for Display - in nanoseconds/microseconds/milliseconds

// (1/72) * TimebaseTimerDividerValues * (GridSize =32) / xScaleForTimebase - for frequency
const float TimebaseExactDivValuesMicros[TIMEBASE_NUMBER_OF_EXCACT_ENTRIES] = { 0.200716845878, 0.518518518, 1.037037037,
        2.074074074, 4.8888888, 9.7777777, 20, 49.777777 };
float getTimebaseExactValueMicros(int8_t aTimebaseIndex) {
    float tResult = TimebaseDivValues[aTimebaseIndex];
    if (aTimebaseIndex >= TIMEBASE_INDEX_MILLIS) {
        tResult *= 1000;
    } else if (aTimebaseIndex < TIMEBASE_NUMBER_OF_EXCACT_ENTRIES) {
        tResult = TimebaseExactDivValuesMicros[aTimebaseIndex];
        if (aTimebaseIndex < TIMEBASE_NUMBER_OF_XSCALE_CORRECTION) {
            tResult *= xScaleForTimebase[aTimebaseIndex];
        }
    }
    return tResult;
}

// Timer cannot divide by 1!
// ADC needs at last 14 cycles for 12 bit conversion and fastest sample time of 1.5 cycles => 72/14 = 5MSamples
// Auto reload / timing values for Timer - Formula for grid timing with prescaler==9 is value*4 i.e. 50=>200us
const uint16_t TimebaseTimerDividerValues[TIMEBASE_NUMBER_OF_ENTRIES] = { 14, 14, 14/*xscale=6 6.222us */, 14, 22,
        22/*xscale=1 9.777us 22.5*/, 45, 112/*49.777us 112.5*/, 25, 50, 125, 250, 500, 1250, 2500, 5000, 12500, 25000, 50000, 125,
        250 };

// 9 => 1/8 us resolution (72/9), 1 => 13.8888889 us resolution
// prescaler can divide by 1 (no division)
const uint16_t TimebaseTimerPrescalerDividerValues[TIMEBASE_NUMBER_OF_ENTRIES] = { 1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9000, 9000 };

const uint16_t ADCClockPrescalerValues[TIMEBASE_NUMBER_OF_ENTRIES] = { 0, 0, 0, 0, 0, 0/*72 MHz*/, 1/*36 MHz*/, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 11, 11, 11, 11 };

#define TRIGGER_HYSTERESIS_MAX 0x50 // max value for automatic effective trigger hysteresis
/************************
 * Measurement control
 ************************/
struct MeasurementControlStruct MeasurementControl;
#define ADC_TIMEOUT 2
#define MIN_SAMPLES_PER_PERIOD_FOR_RELIABLE_FREQUENCY_VALUE 3

/*
 * Data buffer
 */
struct DataBufferStruct DataBufferControl;

/*
 * FFT info
 */
FFTInfoStruct FFTInfo;

/**
 * Attenuator (Hardware) related stuff
 */
/*
 * real attenuation * real amplification (DSO_ATTENUATOR_BASE_GAIN = 2)
 * 0x00-0x02 -> 1MOhm - 0 Ohm
 * 0x03 -> 1MOhm - 1MOhm
 * 0x04 -> 1MOhm - 150kOhm || 3MOhm = 142857 Ohm
 * 0x05 -> 1MOhm - 12kOhm
 * 0x06 -> 1MOhm - 5kOhm
 * 0x07 -> 1MOhm - NC
 */bool isAttenuatorAvailable = true; // attenuator is only attached to channel 2 + 3
float RawAttenuationFactor[NUMBER_OF_RANGES] = { 0.5, 1, 1, 1, 1, 1, 4.05837, 4.05837, 41.6666, 41.6666, 41.6666, 100 };
// Mapping for external attenuator hardware
#define ATTENUATOR_INFINITE_VALUE 0 // value for which attenuator shortens the input
const uint8_t AttenuatorHardwareValue[NUMBER_OF_RANGES] = { 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05 };
// reasonable minimal display range until we see the native ADC resolution (12bit->8bit gives factor 16 gives 4 ranges less (which are factor20-25))
const uint8_t PossibleMinimumRange[NUMBER_OF_RANGES] = { 0, 1, 1, 1, 1, 1, 3, 3, 6, 6, 6, 7 };

/**
 * Virtual ADC values for full scale (DisplayHeight) - used for SetAutoRange() and computeAutoDisplayRange()
 */
int MaxPeakToPeakValue[NUMBER_OF_RANGES]; // for a virtual 18 bit ADC

void autoACZeroCalibration(void);

/*******************************************************************************************
 * Program code starts here
 *******************************************************************************************/

void initMaxPeakToPeakValues(void) {
    // maximal virtual reading for 6 div's
    MaxPeakToPeakValue[11] = sReading3Volt * 2 * ScaleVoltagePerDiv[11]; // 50V / div
    MaxPeakToPeakValue[10] = sReading3Volt * 2 * ScaleVoltagePerDiv[10]; // 20V / div
    MaxPeakToPeakValue[9] = sReading3Volt * 2 * ScaleVoltagePerDiv[9]; // 10V / div
    MaxPeakToPeakValue[8] = sReading3Volt * 2 * ScaleVoltagePerDiv[8]; // 5V / div
    MaxPeakToPeakValue[7] = sReading3Volt * 2 * ScaleVoltagePerDiv[7]; // 2V / div
    MaxPeakToPeakValue[6] = sReading3Volt * 2 * ScaleVoltagePerDiv[6]; // 1V / div
    MaxPeakToPeakValue[5] = sReading3Volt * 2 * ScaleVoltagePerDiv[5]; // 0,5V / div (=40 pixel) -> 3V total
    MaxPeakToPeakValue[4] = sReading3Volt * 2 * ScaleVoltagePerDiv[4]; // 0,2V / div -> 1.2V total
    MaxPeakToPeakValue[3] = sReading3Volt * 2 * ScaleVoltagePerDiv[3]; // 0,1V / div -> 0.6V total
    MaxPeakToPeakValue[2] = sReading3Volt * 2 * ScaleVoltagePerDiv[2]; // 0,05V / div -> 0.3V total
    MaxPeakToPeakValue[1] = sReading3Volt * 2 * ScaleVoltagePerDiv[1]; // 0,02V / div ->  0.12V total
    MaxPeakToPeakValue[0] = sReading3Volt * 2 * ScaleVoltagePerDiv[0]; // 0,01V / div ->  0.6V total
}

/************************************************************************
 * Measurement section
 ************************************************************************/

/**
 * called once at boot time
 */
void initAcquisition(void) {

    MeasurementControl.InfoSizeSmall = false;
    DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[DATABUFFER_DISPLAY_START];

    // Set attenuator to maximum attenuation
    DSO_setAttenuator(AttenuatorHardwareValue[NUMBER_OF_RANGES - 1]);

    // AC range on
    setACMode(true);

    // Timebase
    MeasurementControl.TimebaseFastDMAMode = false;
    MeasurementControl.TimebaseIndex = TIMEBASE_INDEX_START_VALUE;
    DataBufferControl.DrawWhileAcquire = (TIMEBASE_INDEX_START_VALUE >= TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE); // false here
}

/**
 * called at every start of application (startDSO())
 */
void resetAcquisition(void) {
    MeasurementControl.isRunning = false;
    MeasurementControl.isSingleShotMode = false;
#ifdef LOCAL_DISPLAY_EXISTS
    MeasurementControl.ADS7846ChannelsAsDatasource = false;
#endif

    // Calibration
    ADC_setRawToVoltFactor();
    initMaxPeakToPeakValues();
    initRawToDisplayFactors();
    autoACZeroCalibration(); // sets MeasurementControl.DSOReadingACZero

    // Timebase
    MeasurementControl.TimebaseNewIndex = MeasurementControl.TimebaseIndex;
    changeTimeBase(true);

    // Channel
    MeasurementControl.ADCInputMUXChannelIndex = START_ADC_CHANNEL_INDEX;
    isAttenuatorAvailable = true;

    // Trigger
    MeasurementControl.TriggerSlopeRising = true;
    MeasurementControl.TriggerMode = TRIGGER_MODE_AUTOMATIC;

    setTriggerLevelAndHysteresis(10, 5); // must be initialized to enable auto triggering

    // Set attenuator / range
    MeasurementControl.InputRangeIndex = NUMBER_OF_RANGES - 1; // max attenuation
    MeasurementControl.DisplayRangeIndex = NUMBER_OF_RANGES - 1;
    MeasurementControl.InputRangeIndexOtherThanDisplayRange = false;
    DSO_setAttenuator(AttenuatorHardwareValue[NUMBER_OF_RANGES - 1]);
    MeasurementControl.TimestampLastRangeChange = 0; // enable direct range change at start
    MeasurementControl.RangeAutomatic = true;
    MeasurementControl.OffsetMode = OFFSET_MODE_0_VOLT;

    // init variables etc. uses IndexDisplayRange
    setACMode(MeasurementControl.isACMode);
}

/*
 * prepares all variables for new acquisition
 * switches between fast an interrupt mode depending on TIMEBASE_FAST_MODES
 * sets ADC status register including prescaler
 * setup new interrupt cycle only if not to be stopped
 */
void startAcquisition(void) {

    // start with waiting for triggering condition
    MeasurementControl.TriggerActualPhase = PHASE_PRE_TRIGGER;
    MeasurementControl.TriggerSampleCount = 0;
    MeasurementControl.TriggerStatus = TRIGGER_START;
    MeasurementControl.doPretriggerCopyForDisplay = false;

    MeasurementControl.TimebaseFastDMAMode = false;
    if (MeasurementControl.TimebaseIndex < TIMEBASE_FAST_MODES) {
        // TimebaseFastFreerunningMode must be set only here at beginning of acquisition
        MeasurementControl.TimebaseFastDMAMode = true;
    } else if (MeasurementControl.TimebaseIndex >= TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE) {
        MeasurementControl.TriggerActualPhase = PHASE_SEARCH_TRIGGER;
        DataBufferControl.DataBufferNextDrawPointer = &DataBufferControl.DataBuffer[0];
        DataBufferControl.NextDrawXValue = 0;
        MeasurementControl.TriggerPhaseJustEnded = false;
        DataBufferControl.DataBufferPreTriggerAreaWrapAround = false;
    }

    // start and end pointer
    // end pointer should be computed here because of trigger mode
    DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[DATABUFFER_DISPLAY_START];
    DataBufferControl.DataBufferEndPointer = &DataBufferControl.DataBuffer[DATABUFFER_DISPLAY_END];
    DataBufferControl.DataBufferNextInPointer = &DataBufferControl.DataBuffer[0];

    if (MeasurementControl.TriggerMode == TRIGGER_MODE_OFF) {
        MeasurementControl.TriggerActualPhase = PHASE_POST_TRIGGER;
        // no trigger => no pretrigger area
        DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[0];
        DataBufferControl.DataBufferEndPointer = &DataBufferControl.DataBuffer[DSO_DISPLAY_WIDTH - 1];
    }

    if (MeasurementControl.isSingleShotMode) {
        // Start and request immediate stop
        MeasurementControl.StopRequested = true;
    }
    if (MeasurementControl.StopRequested) {
        // last acquisition or single shot mode -> use whole data buffer
        DataBufferControl.DataBufferEndPointer = &DataBufferControl.DataBuffer[DATABUFFER_SIZE - 1];
    }

#ifdef LOCAL_DISPLAY_EXISTS
    if (MeasurementControl.ADS7846ChannelsAsDatasource) {
        // to skip pretrigger and avoid pre trigger buffer adjustment
        DataBufferControl.DataBufferNextInPointer = DataBufferControl.DataBufferDisplayStart;
        DataBufferControl.DataBufferPreTriggerNextPointer = &DataBufferControl.DataBuffer[0];
        // return here because of no interrupt handling for ADS7846Channels
        return;
    }
#endif

    MeasurementControl.StopAcknowledged = false;
    DataBufferControl.InputRangeIndexUsed = MeasurementControl.InputRangeIndex;

    DataBufferControl.DataBufferFull = false;

    /*
     * Start ADC + timer
     */
    ADC_startTimer6();
    if (!MeasurementControl.TimebaseFastDMAMode) {
        ADC_enableEOCInterrupt(DSO_ADC_ID );
        // starts conversion at next timer edge
        ADC_StartConversion(DSO_ADC_ID );
    } else {
        ADC_DMA_start((uint32_t) &DataBufferControl.DataBuffer[0], DATABUFFER_SIZE);
    }
}

/*
 * Calibration
 * 1. Tune the hardware amplification so that VRef gives a reading of 0x3FE.
 * 2. Attach probe to VRef or other constant voltage source.
 * 3. Set DC range and get data for 10 ms and process auto range.
 * 4. If range is suitable (i.e highest range for attenuation) output tone and wait for 100ms
 * 							otherwise output message go to 3.
 * 5. Acquire data for 100ms (this reduces 50Hz and 60Hz interference)
 * 4. If p2p < threshold then switch range else output message "ripple too high"
 * 5. wait for 100 ms for range switch to settle and acquire data for 100ms in new range.
 * 6. If p2p < threshold compute attenuation factor and store in CMOS Ram
 * 7. Output Message "Complete"
 */

/**
 * compute how many samples are needed for TRIGGER_TIMEOUT_MILLIS (cur. 200ms) but use at least TRIGGER_TIMEOUT_MIN_SAMPLES samples
 * @param aTimebaseIndex
 * @return
 */
uint16_t computeNumberOfSamplesToTimeout(int8_t aTimebaseIndex) {
    uint32_t tCount = DSO_DISPLAY_WIDTH * 225 * TRIGGER_TIMEOUT_MILLIS
            / (TimebaseTimerDividerValues[aTimebaseIndex] * TimebaseTimerPrescalerDividerValues[aTimebaseIndex]);
    if (tCount < TRIGGER_TIMEOUT_MIN_SAMPLES) {
        tCount = TRIGGER_TIMEOUT_MIN_SAMPLES;
    }
    if (aTimebaseIndex < TIMEBASE_FAST_MODES) {
        // compute loop count instead of sample count
        tCount /= (DATABUFFER_SIZE - DSO_DISPLAY_WIDTH);
    }
    if (tCount > 0xFFFF) {
        tCount = 0xFFFF;
    }
    return tCount;
}

#ifdef LOCAL_DISPLAY_EXISTS
/*
 * read ADS7846 channels
 */
void readADS7846Channels(void) {

    uint16_t tValue;
    uint16_t *DataPointer = DataBufferControl.DataBufferNextInPointer;
    tValue = TouchPanel.readChannel(ADS7846ChannelMapping[MeasurementControl.ADCInputMUXChannelIndex], true, false, 0x1);
    while (DataPointer <= DataBufferControl.DataBufferEndPointer) {
        tValue = TouchPanel.readChannel(ADS7846ChannelMapping[MeasurementControl.ADCInputMUXChannelIndex], true, false, 0x1);
        *DataPointer++ = tValue;
    }
    DataBufferControl.DataBufferFull = true;
}
#endif

/*
 * called by half transfer interrupt
 */
void DMACheckForTriggerCondition(void) {
    if (MeasurementControl.TriggerMode != TRIGGER_MODE_OFF) {

        uint16_t tValue;
        uint8_t tTriggerStatus = TRIGGER_START;
        bool tFalling = !MeasurementControl.TriggerSlopeRising;
        uint16_t tActualCompareValue = MeasurementControl.RawTriggerLevelHysteresis;
        // start after pre trigger values
        uint16_t * tDMAMemoryAddress = &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE];
        uint16_t * tEndMemoryAddress = &DataBufferControl.DataBuffer[DATABUFFER_SIZE / 2];
        do {
            clearSystic();
            while (tDMAMemoryAddress < tEndMemoryAddress) { // 37 instructions at -o0 if nothing found
                /*
                 * scan from end of pre trigger to half of buffer for trigger condition
                 */
                tValue = *tDMAMemoryAddress++;
                bool tValueGreaterRef = (tValue > tActualCompareValue);
                tValueGreaterRef = tValueGreaterRef ^ tFalling; // change value if tFalling == true
                if (tTriggerStatus == TRIGGER_START) {
                    // rising slope - wait for value below 1. threshold
                    // falling slope - wait for value above 1. threshold
                    if (!tValueGreaterRef) {
                        tTriggerStatus = TRIGGER_BEFORE_THRESHOLD;
                        tActualCompareValue = MeasurementControl.RawTriggerLevel;
                    }
                } else {
                    // rising slope - wait for value to rise above 2. threshold
                    // falling slope - wait for value to go below 2. threshold
                    if (tValueGreaterRef) {
                        tTriggerStatus = TRIGGER_OK;
                        MeasurementControl.TriggerStatus = TRIGGER_OK;
                        break;
                    }
                }
            }

            if (tTriggerStatus == TRIGGER_OK) {
                // trigger found -> exit from loop
                break;
            }
            /*
             * New DMA Transfer must be started if no full screen left - see setting below "if (tCount <..."
             */
            if (tEndMemoryAddress
                    >= &DataBufferControl.DataBuffer[DATABUFFER_SIZE
                            - (DSO_DISPLAY_WIDTH - DisplayControl.DatabufferPreTriggerDisplaySize) - 1]
                    || (MeasurementControl.isSingleShotMode)) {
                // No trigger found and not enough buffer left for a screen of data
                // or on singleshot mode, less than a half buffer left -> restart DMA
                if (!MeasurementControl.isSingleShotMode) {
                    // No timeout in single shot mode
                    if (MeasurementControl.TriggerSampleCount++ > MeasurementControl.TriggerTimeoutSampleOrLoopCount) {
                        // Trigger condition not met and timeout reached
                        //STM_EVAL_LEDToggle(LED6); // GREEN LEFT
                        // timeout exit from loop
                        break;
                    }
                }
                // copy pretrigger data for display in loop
                if (MeasurementControl.doPretriggerCopyForDisplay) {
                    memcpy(&FourDisplayLinesBuffer[0], &DataBufferControl.DataBuffer[0],
                            DATABUFFER_PRE_TRIGGER_SIZE * sizeof(DataBufferControl.DataBuffer[0]));
                    MeasurementControl.doPretriggerCopyForDisplay = false;
                }

                // restart DMA, leave ISR and wait for new interrupt
                ADC_DMA_start((uint32_t) &DataBufferControl.DataBuffer[0], DATABUFFER_SIZE);
                // reset transfer complete status before return since this interrupt may happened and must be re enabled
                DMA_ClearITPendingBit(DMA1_IT_TC1 );
                DataBufferControl.DataBufferFull = false;
                return;
            } else {
                // set new tEndMemoryAddress and check if it is before last possible trigger position
                // appr. 77063 cycles / 1 ms until we get here first time (1440 values checked 53 cycles/value at -o0)
                uint32_t tCount = DSO_DMA_CHANNEL ->CNDTR;
                if (tCount < DSO_DISPLAY_WIDTH - DisplayControl.DatabufferPreTriggerDisplaySize) {
                    tCount = DSO_DISPLAY_WIDTH - DisplayControl.DatabufferPreTriggerDisplaySize;
                }
                tEndMemoryAddress = &DataBufferControl.DataBuffer[DATABUFFER_SIZE - tCount - 1];
            }
        } while (true); // end with break above

        /*
         * set pointer for display of data
         */
        if (tTriggerStatus != TRIGGER_OK) {
            // timeout here
            tDMAMemoryAddress = &DataBufferControl.DataBuffer[DisplayControl.DatabufferPreTriggerDisplaySize];
        }

        // correct start by DisplayControl.XScale - XScale is known to be >=0 here
        DataBufferControl.DataBufferDisplayStart = tDMAMemoryAddress - 1
                - adjustIntWithScaleFactor(DisplayControl.DatabufferPreTriggerDisplaySize, DisplayControl.XScale);
        if (MeasurementControl.StopRequested) {
            MeasurementControl.StopAcknowledged = true;
        } else {
            // set end pointer to end of display for reproducible min max + average findings - XScale is known to be >=0 here
            int tAdjust = adjustIntWithScaleFactor(DSO_DISPLAY_WIDTH - DisplayControl.DatabufferPreTriggerDisplaySize - 1,
                    DisplayControl.XScale);
            DataBufferControl.DataBufferEndPointer = tDMAMemoryAddress + tAdjust;
        }
    }
}

extern "C" void DMA1_Channel1_IRQHandler(void) {

    // Test on DMA Transfer Complete interrupt
    if (DMA_GetITStatus(DMA1_IT_TC1 )) {
        /* Clear DMA  Transfer Complete interrupt pending bit */
        DMA_ClearITPendingBit(DMA1_IT_TC1 );
        DataBufferControl.DataBufferFull = true;
        ADC_StopConversion(DSO_ADC_ID );
    }
    // Test on DMA Transfer Error interrupt
    if (DMA_GetITStatus(DMA1_IT_TE1 )) {
        failParamMessage(DSO_DMA_CHANNEL ->CPAR, "DMA Error");
        DMA_ClearITPendingBit(DMA1_IT_TE1 );
    }
    // Test on DMA Transfer Half interrupt
    if (DMA_GetITStatus(DMA1_IT_HT1 )) {
        DMA_ClearITPendingBit(DMA1_IT_HT1 );
        DMACheckForTriggerCondition();
    }
}

/**
 * Interrupt service routine for adc interrupt
 * app. 3.5 microseconds when compiled with no optimizations.
 * With optimizations it works up to 50us/div
 * first value which meets trigger condition is stored at position DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE]
 */

extern "C" void ADC1_2_IRQHandler(void) {

    uint16_t tValue = DSO_ADC_ID ->DR;

    /*
     * read at least DATABUFFER_PRE_TRIGGER_SIZE values in pre trigger phase
     */
    if (MeasurementControl.TriggerActualPhase == PHASE_PRE_TRIGGER) {
        // store value
        *DataBufferControl.DataBufferNextInPointer++ = tValue;
        MeasurementControl.TriggerSampleCount++;
        if (MeasurementControl.TriggerSampleCount >= DATABUFFER_PRE_TRIGGER_SIZE) {
            // now we have read at least DATABUFFER_PRE_TRIGGER_SIZE values => start search for trigger
            if (MeasurementControl.TriggerMode == TRIGGER_MODE_OFF) {
                MeasurementControl.TriggerActualPhase = PHASE_POST_TRIGGER;
                return;
            } else {
                MeasurementControl.TriggerActualPhase = PHASE_SEARCH_TRIGGER;
            }
            MeasurementControl.TriggerSampleCount = 0;
            DataBufferControl.DataBufferNextInPointer = &DataBufferControl.DataBuffer[0];
        }
        return;
    }
    if (MeasurementControl.TriggerActualPhase == PHASE_SEARCH_TRIGGER) {
        bool tTriggerFound = false;
        /*
         * Trigger detection here
         */
        uint8_t tTriggerStatus = MeasurementControl.TriggerStatus;

        if (MeasurementControl.TriggerSlopeRising) {
            if (tTriggerStatus == TRIGGER_START) {
                // rising slope - wait for value below 1. threshold
                if (tValue < MeasurementControl.RawTriggerLevelHysteresis) {
                    MeasurementControl.TriggerStatus = TRIGGER_BEFORE_THRESHOLD;
                }
            } else {
                // rising slope - wait for value to rise above 2. threshold
                if (tValue > MeasurementControl.RawTriggerLevel) {
                    // start reading into buffer
                    tTriggerFound = true;
                    MeasurementControl.TriggerStatus = TRIGGER_OK;
                }
            }
        } else {
            if (tTriggerStatus == TRIGGER_START) {
                // falling slope - wait for value above 1. threshold
                if (tValue > MeasurementControl.RawTriggerLevelHysteresis) {
                    MeasurementControl.TriggerStatus = TRIGGER_BEFORE_THRESHOLD;
                }
            } else {
                // falling slope - wait for value to go below 2. threshold
                if (tValue < MeasurementControl.RawTriggerLevel) {
                    // start reading into buffer
                    tTriggerFound = true;
                    MeasurementControl.TriggerStatus = TRIGGER_OK;
                }
            }
        }

        uint16_t * tDataBufferPointer = DataBufferControl.DataBufferNextInPointer;
        if (!tTriggerFound) {
            /*
             * store value - check for wrap around in pre trigger area
             */
            // store value
            *tDataBufferPointer++ = tValue;
            MeasurementControl.TriggerSampleCount++;
            // detect end of pre trigger buffer
            if (tDataBufferPointer >= &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE]) {
                // wrap around - for draw while acquire
                DataBufferControl.DataBufferPreTriggerAreaWrapAround = true;
                tDataBufferPointer = &DataBufferControl.DataBuffer[0];
            }
            // prepare for next
            DataBufferControl.DataBufferNextInPointer = tDataBufferPointer;

            if (MeasurementControl.isSingleShotMode) {
                // No timeout in single shot mode
                MeasurementControl.RawValueBeforeTrigger = tValue;
                return;
            }
            /*
             * Trigger timeout handling
             */
            if (MeasurementControl.TriggerSampleCount < MeasurementControl.TriggerTimeoutSampleOrLoopCount) {
                /*
                 * Trigger condition not met and timeout not reached
                 */
                return;
            }
        }
        // Trigger found or timeout -> reset trigger flag and initialize max and min and set data buffer
        MeasurementControl.TriggerActualPhase = PHASE_POST_TRIGGER;
        DataBufferControl.DataBufferPreTriggerNextPointer = tDataBufferPointer;
        /*
         * only needed for draw while acquire
         * set flag for main loop to detect end of trigger phase
         */
        MeasurementControl.TriggerPhaseJustEnded = true;
        tDataBufferPointer = &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE];
        // store value
        *tDataBufferPointer++ = tValue;
        DataBufferControl.DataBufferNextInPointer = tDataBufferPointer;
        return;
    }

    /*
     * detect end of buffer
     */
    uint16_t * tDataBufferPointer = DataBufferControl.DataBufferNextInPointer;
    if (tDataBufferPointer <= DataBufferControl.DataBufferEndPointer) {
        // store display value
        *tDataBufferPointer++ = tValue;
        // prepare for next
        DataBufferControl.DataBufferNextInPointer = tDataBufferPointer;
    } else {
        // stop acquisition
        // End of conversion => stop ADC in order to make it reconfigurable (change timebase)
        ADC_StopConversion(DSO_ADC_ID );
        ADC_disableEOCInterrupt(DSO_ADC_ID );
        /*
         * signal to main loop that acquisition ended
         * Main loop is responsible to start a new acquisition via call of startAcquisition();
         */
        DataBufferControl.DataBufferFull = true;
    }
}
/**
 * set attenuator to infinite and AC Pin active, read n samples and store it in MeasurementControl.DSOReadingACZero
 */
void autoACZeroCalibration(void) {
    bool tState = DSO_getACMode();
    DSO_setACMode(true);
    DSO_setAttenuator(ATTENUATOR_INFINITE_VALUE);
    ADC1_getChannelValue(ADCInputMUXChannels[START_ADC_CHANNEL_INDEX]);
    // wait to settle
    delayMillis(10);
    uint16_t tSum = 0;
    for (int i = 0; i < 16; ++i) {
        uint16_t tVal = ADC1_getChannelValue(ADCInputMUXChannels[START_ADC_CHANNEL_INDEX]);
        if (tVal > ADC_MAX_CONVERSION_VALUE) {
            // conversion impossible -> set fallback value
            MeasurementControl.RawDSOReadingACZero = ADC_MAX_CONVERSION_VALUE / 2;
            return;
        }
        tSum += ADC1_getChannelValue(ADCInputMUXChannels[START_ADC_CHANNEL_INDEX]);
    }
    MeasurementControl.RawDSOReadingACZero = tSum >> 4;
    DSO_setACMode(tState);
}
/**
 * Get period and frequency and average for display
 * Get max and min for display and automatic triggering.
 *
 * Use only post trigger area!
 */
void computeMinMaxAverageAndPeriodFrequency(void) {
    uint16_t * tDataBufferPointer = DataBufferControl.DataBufferDisplayStart
            + adjustIntWithScaleFactor(DisplayControl.DatabufferPreTriggerDisplaySize, DisplayControl.XScale);
    if (DataBufferControl.DataBufferEndPointer > tDataBufferPointer) {
        uint16_t tAcquisitionSize = DataBufferControl.DataBufferEndPointer + 1 - tDataBufferPointer;
        uint16_t tMax, tMin;
        tMax = *tDataBufferPointer;
        tMin = *tDataBufferPointer;
        uint16_t tValue;
        uint32_t tIntegrateValue = 0;
        uint32_t tIntegrateValueForTotalPeriods = 0;
        int tCount = 0;
        int tLastFoundPosition = 0;
        int tPeriodDelta = 0;
        int tPeriodMin = 1024;
        int tPeriodMax = 0;
        int tTriggerStatus = TRIGGER_START;
        bool tFalling = !MeasurementControl.TriggerSlopeRising;
        uint16_t tActualCompareValue = MeasurementControl.RawTriggerLevelHysteresis;
        bool tReliableValue = true;

        for (int i = 0; i < tAcquisitionSize; ++i) {
            tValue = *tDataBufferPointer++;
            /*
             * trigger condition and average taken only from entire periods
             */ //
            bool tValueGreaterRef = (tValue > tActualCompareValue);
            tValueGreaterRef = tValueGreaterRef ^ tFalling; // change value if tFalling == true

            if (tTriggerStatus == TRIGGER_START) {
                // rising slope - wait for value below 1. threshold
                // falling slope - wait for value above 1. threshold
                if (!tValueGreaterRef) {
                    tTriggerStatus = TRIGGER_BEFORE_THRESHOLD;
                    tActualCompareValue = MeasurementControl.RawTriggerLevel;
                }
            } else {
                // rising slope - wait for value to rise above 2. threshold
                // falling slope - wait for value to go below 2. threshold
                if (tValueGreaterRef) {
                    if ((tPeriodDelta) < MIN_SAMPLES_PER_PERIOD_FOR_RELIABLE_FREQUENCY_VALUE) {
                        // found new trigger in less than MIN_SAMPLES_PER_PERIOD_FOR_RELIABLE_FREQUENCY_VALUE samples => no reliable value
                        tReliableValue = false;
                    } else {
                        if (tPeriodDelta < tPeriodMin) {
                            tPeriodMin = tPeriodDelta;
                        } else if (tPeriodDelta > tPeriodMax) {
                            tPeriodMax = tPeriodDelta;
                        }
                        tPeriodDelta = 0;
                        // found and search for next slope
                        tIntegrateValueForTotalPeriods = tIntegrateValue;
                        tCount++;
                        tLastFoundPosition = i;
                        tTriggerStatus = TRIGGER_START;
                        tActualCompareValue = MeasurementControl.RawTriggerLevelHysteresis;
                    }
                }
            }
            tIntegrateValue += tValue;
            tPeriodDelta++;

            /*
             * Min and Max
             */
            if (tValue > tMax) {
                tMax = tValue;
            } else if (tValue < tMin) {
                tMin = tValue;
            }
        }

        MeasurementControl.RawValueMin = tMin;
        MeasurementControl.RawValueMax = tMax;

        /*
         * check for plausi of period values
         * allow delta of periods to be at least 1/8 period + 3
         */
        tPeriodDelta = tPeriodMax - tPeriodMin;
        if (((tLastFoundPosition / (8 * tCount)) + 3) < tPeriodDelta) {
            tReliableValue = false;
        }

        /*
         * compute period and frequency
         */
        float tPeriodMicros = 0.0;
        float tHertz = 0.0;
        if (tLastFoundPosition != 0 && tCount != 0 && tReliableValue) {
            MeasurementControl.RawValueAverage = (tIntegrateValueForTotalPeriods + (tLastFoundPosition / 2)) / tLastFoundPosition;

            // compute microseconds per period
            tPeriodMicros = tLastFoundPosition;
            if (MeasurementControl.TimebaseIndex < TIMEBASE_NUMBER_OF_EXCACT_ENTRIES) {
                // use exact value where needed
                tPeriodMicros = tPeriodMicros * TimebaseExactDivValuesMicros[MeasurementControl.TimebaseIndex];
                if (MeasurementControl.TimebaseIndex < TIMEBASE_NUMBER_OF_XSCALE_CORRECTION) {
                    tPeriodMicros *= xScaleForTimebase[MeasurementControl.TimebaseIndex];
                }
            } else {
                tPeriodMicros = tPeriodMicros * TimebaseDivValues[MeasurementControl.TimebaseIndex];
            }
            tPeriodMicros = tPeriodMicros / (tCount * TIMING_GRID_WIDTH);
            // nanos are handled by TimebaseExactDivValues < 1
            if (MeasurementControl.TimebaseIndex >= TIMEBASE_INDEX_MILLIS) {
                tPeriodMicros = tPeriodMicros * 1000;
            }
            // frequency
            tHertz = 1000000.0 / tPeriodMicros;
        } else {
            MeasurementControl.RawValueAverage = (tIntegrateValue + (tAcquisitionSize / 2)) / tAcquisitionSize;
        }
        MeasurementControl.FrequencyHertz = tHertz  + 0.5;
        MeasurementControl.PeriodMicros = tPeriodMicros  + 0.5;
        return;
    }
}

/**
 * compute new trigger value and hysteresis
 * If old values are reasonable don't change them to avoid jitter
 */
void computeAutoTrigger(void) {

    if (MeasurementControl.TriggerMode == TRIGGER_MODE_AUTOMATIC) {
        /*
         * Set auto trigger in middle between min and max
         */
        int tPeakToPeak = MeasurementControl.RawValueMax - MeasurementControl.RawValueMin;
        int tTriggerValue = MeasurementControl.RawValueMin + (tPeakToPeak / 2);

        /*
         * set effective hysteresis to quarter tPeakToPeak
         */
        int TriggerHysteresis = tPeakToPeak / 4;

        // keep reasonable value - avoid jitter
        int tTriggerDelta = abs(tTriggerValue - MeasurementControl.RawTriggerLevel);
        if (tTriggerDelta > TriggerHysteresis / 4) {
            setTriggerLevelAndHysteresis(tTriggerValue, TriggerHysteresis);
        }
    }
}

/**
 * Real ADC PeakToPeakValue is multiplied by RealAttenuationFactor[MeasurementControl.TotalRangeIndex]
 * to get values of a virtual 18 bit ADC.
 *
 * First check for clipping (check ADC_MAX_CONVERSION_VALUE and also 0 if AC mode)
 * If no clipping occurred, sets range index and attenuator so that max (virtual) peek to peek value fits into the new range on display.
 */
void computeAutoInputRange(void) {
    if (MeasurementControl.RangeAutomatic) {
        //First check for clipping (check ADC_MAX_CONVERSION_VALUE and also 0 if AC mode)
        if (MeasurementControl.RawValueMax == ADC_MAX_CONVERSION_VALUE
                || (MeasurementControl.isACMode && MeasurementControl.RawValueMin == 0)) {
            int tRangeChangeValue = 1;
            // Need to change attenuator so lets find next range with greater attenuation
            for (int i = MeasurementControl.InputRangeIndex; i < NUMBER_OF_RANGES - 1; ++i) {
                if (RawAttenuationFactor[i] != RawAttenuationFactor[i + 1]) {
                    break;
                }
            }
            changeInputRange(tRangeChangeValue); // also adjusts trigger level if needed
            return;
        }

        /*
         * no clipping occurred -> check if another range is more suitable
         */
        // get relevant peak2peak value
        int tPeakToPeak = MeasurementControl.RawValueMax;
        if (MeasurementControl.isACMode) {
            if (MeasurementControl.RawDSOReadingACZero - MeasurementControl.RawValueMin > MeasurementControl.RawValueMax - MeasurementControl.RawDSOReadingACZero) {
                //difference between zero and min is greater than difference between zero and max => min determines the range
                tPeakToPeak = MeasurementControl.RawDSOReadingACZero - MeasurementControl.RawValueMin;
            } else {
                tPeakToPeak -= MeasurementControl.RawDSOReadingACZero;
            }
            // since tPeakToPeak must fit in half of display
            tPeakToPeak *= 2;
        }

        // get virtual peak2peak value (simulate an virtual 18 bit ADC)
        tPeakToPeak *= RawAttenuationFactor[MeasurementControl.InputRangeIndex];

        int tOldRangeIndex = MeasurementControl.InputRangeIndex;
        /*
         * find new range index
         */
        int tNewRangeIndex;
        for (tNewRangeIndex = 0; tNewRangeIndex < NUMBER_OF_RANGES; ++tNewRangeIndex) {
            if (tPeakToPeak <= MaxPeakToPeakValue[tNewRangeIndex]) {
                //actual PeakToPeak fits into display size now
                break;
            }
        }
        uint32_t tActualMillis = getMillisSinceBoot();
        if (tOldRangeIndex != tNewRangeIndex) {
            if (tNewRangeIndex > tOldRangeIndex) {
                changeInputRange(tNewRangeIndex - tOldRangeIndex);
            } else {
                /*
                 * wait n-milliseconds before switch to higher resolution (lower index)
                 */
                if (tActualMillis - MeasurementControl.TimestampLastRangeChange > SCALE_CHANGE_DELAY_MILLIS) {
                    MeasurementControl.TimestampLastRangeChange = tActualMillis;
                    // decrease range only if value is not greater than 80% of new range max value to avoid fast switching back
                    if ((tPeakToPeak * 10) / 8 <= MaxPeakToPeakValue[tNewRangeIndex]) {
                        changeInputRange(tNewRangeIndex - tOldRangeIndex);
                    } else if ((tOldRangeIndex - tNewRangeIndex) > 1) {
                        // change range at least by one if ranges differ more than one (which really happened!)
                        changeInputRange(-1);
                    }
                }
            }
        } else {
            // reset "delay"
            MeasurementControl.TimestampLastRangeChange = tActualMillis;
        }
    }
}

/**
 * Computes a display range in order to fill screen with signal.
 * Only makes sense in conjunction with a display offset value.
 * Therefore calls computeAutoOffset() if display range changed or display clipping occurs.
 * Changing input range in this mode makes no sense.
 */
void computeAutoDisplayRange(void) {
    if (MeasurementControl.OffsetMode == OFFSET_MODE_AUTOMATIC) {
        /*
         * check if a display range less than input range is more suitable.
         */
        // get relevant peak2peak value
        int tPeakToPeak = MeasurementControl.RawValueMax - MeasurementControl.RawValueMin;
        // increase value by OFFSET_FIT_FACTOR = 10/8 (1.25) to have margin to have minimum starting at first grid line - exact factor for this would be (6/5 = 1.2)
        tPeakToPeak = (tPeakToPeak * 10) / 8;
        // get virtual value
        tPeakToPeak *= RawAttenuationFactor[MeasurementControl.InputRangeIndex];

        /*
         * find new display range index
         */
        int tNewDisplayRangeIndex;
        for (tNewDisplayRangeIndex = 0; tNewDisplayRangeIndex < NUMBER_OF_RANGES; ++tNewDisplayRangeIndex) {
            if (tPeakToPeak <= MaxPeakToPeakValue[tNewDisplayRangeIndex]) {
                //actual PeakToPeak fits into display size now
                break;
            }
        }

        // clip to reasonable value according to ADC resolution
        if (tNewDisplayRangeIndex < PossibleMinimumRange[MeasurementControl.InputRangeIndex]) {
            tNewDisplayRangeIndex = PossibleMinimumRange[MeasurementControl.InputRangeIndex];
        }

        // display range changed?
        if (tNewDisplayRangeIndex != MeasurementControl.DisplayRangeIndex) {
            // switch to higher resolution only if actual value is less than 80% of max possible value
            if (tNewDisplayRangeIndex > MeasurementControl.DisplayRangeIndex
                    || ((tPeakToPeak * 10) / 8) <= MaxPeakToPeakValue[tNewDisplayRangeIndex]) {
                MeasurementControl.DisplayRangeIndex = tNewDisplayRangeIndex;
                // IndexDisplayRange can be greater than InputRangeIndex because of OFFSET_FIT_FACTOR above
                setDisplayRange(tNewDisplayRangeIndex, true);
                computeAutoOffset();
            }
        } else if (MeasurementControl.RawValueMin < MeasurementControl.RawValueOffsetClippingLower
                || MeasurementControl.RawValueMax > MeasurementControl.RawValueOffsetClippingUpper) {
            // Change offset if display clipping occurs e.g. actual min < actual offset or actual max > (actual offset + value for full scale)
            computeAutoOffset();
        }
    }
}

/**
 * compute offset based on center value in order display values at center of screen
 */
void computeAutoOffset(void) {

    int tValueMiddleY = MeasurementControl.RawValueMin + ((MeasurementControl.RawValueMax - MeasurementControl.RawValueMin) / 2);
    if (MeasurementControl.isACMode) {
        tValueMiddleY -= MeasurementControl.RawDSOReadingACZero;
    }
    tValueMiddleY = (tValueMiddleY * FactorFromInputToDisplayRangeShift12) >> DSO_INPUT_TO_DISPLAY_SHIFT;

    // Adjust to multiple of div
    // formula is: RawPerDiv[i] = HORIZONTAL_GRID_HEIGHT / ScaleFactorRawToDisplay[i]);
    // tOffsetDivCount = tValueMiddleY / RawPerDiv[tNewDisplayRangeIndex];
    signed int tOffsetGridCount = tValueMiddleY
            * (ScaleFactorRawToDisplayShift18[MeasurementControl.DisplayRangeIndex] / HORIZONTAL_GRID_HEIGHT);
    tOffsetGridCount = tOffsetGridCount >> DSO_SCALE_FACTOR_SHIFT;

    if (MeasurementControl.isACMode || tOffsetGridCount >= (HORIZONTAL_GRID_COUNT / 2)) {
        // Adjust start OffsetDivCount in order to display values at center of screen
        tOffsetGridCount -= (HORIZONTAL_GRID_COUNT / 2);
    }
    setOffsetGridCount(tOffsetGridCount);
    drawGridLinesWithHorizLabelsAndTriggerLine(COLOR_GRID_LINES);
}

void setTriggerLevelAndHysteresis(int aRawTriggerValue, int aRawTriggerHysteresis) {
    MeasurementControl.RawTriggerLevel = aRawTriggerValue;
    if (MeasurementControl.TriggerSlopeRising) {
        MeasurementControl.RawTriggerLevelHysteresis = aRawTriggerValue - aRawTriggerHysteresis;
    } else {
        MeasurementControl.RawTriggerLevelHysteresis = aRawTriggerValue + aRawTriggerHysteresis;
    }
}

/**
 * adjusts trigger level if needed
 * sets exclusively: InputRangeIndex, actualDSOReadingACZeroForInputRange and hardware attenuator
 * @param aValue value to change range
 * @return true if range has changed false otherwise
 */ //
bool changeInputRange(int aValue) {
    bool tRetValue = true;
    int tNewRange = MeasurementControl.InputRangeIndex + aValue;

    if (aValue == 0) {
        tRetValue = false;
    } else if (tNewRange > NUMBER_OF_RANGES - 1) {
        tNewRange = NUMBER_OF_RANGES - 1;
        tRetValue = false;
    } else if (tNewRange < 0) {
        tNewRange = 0;
        tRetValue = false;
    } else if (!isAttenuatorAvailable && (tNewRange > NO_ATTENUATOR_MAX_RANGE_INDEX || tNewRange < NO_ATTENUATOR_MIN_RANGE_INDEX)) {
        // no attenuator available
        tRetValue = false;
    } else {

        float tOldDSORawToVoltFactor = actualDSORawToVoltFactor;
        MeasurementControl.InputRangeIndex = tNewRange;
        // set attenuator and conversion factor
        DSO_setAttenuator(AttenuatorHardwareValue[tNewRange]);
        actualDSORawToVoltFactor = ADCToVoltFactor * RawAttenuationFactor[MeasurementControl.InputRangeIndex];

        if (MeasurementControl.TriggerMode != TRIGGER_MODE_OFF) {
            // adjust trigger raw level if needed
            if (tOldDSORawToVoltFactor != actualDSORawToVoltFactor) {
                int tAcCompensation = 0;
                if (MeasurementControl.isACMode) {
                    tAcCompensation = MeasurementControl.RawDSOReadingACZero;
                }
                // Reuse variable here
                tOldDSORawToVoltFactor = tOldDSORawToVoltFactor / actualDSORawToVoltFactor;
                MeasurementControl.RawTriggerLevel = ((MeasurementControl.RawTriggerLevel - tAcCompensation)
                        * tOldDSORawToVoltFactor) + tAcCompensation;
                MeasurementControl.RawTriggerLevelHysteresis = ((MeasurementControl.RawTriggerLevelHysteresis - tAcCompensation)
                        * tOldDSORawToVoltFactor) + tAcCompensation;
            }
        }

        if (MeasurementControl.OffsetMode == OFFSET_MODE_0_VOLT) {
            MeasurementControl.DisplayRangeIndex = MeasurementControl.InputRangeIndex;
            // adjust raw offset value for ac
            if (MeasurementControl.isACMode) {
                MeasurementControl.RawOffsetValueForDisplayRange = getRawOffsetValueFromGridCount(
                        DISPLAY_AC_ZERO_OFFSET_GRID_COUNT);
            }
            // keep labels up to date
            drawGridLinesWithHorizLabelsAndTriggerLine(COLOR_GRID_LINES);
        }
    }
    return tRetValue;
}

/**
 * sets absolute range, for handling of direct ADC channels
 */            //
bool setInputRange(int aValue) {
    return changeInputRange(aValue - MeasurementControl.InputRangeIndex);
}

void setACMode(bool aACMode) {
    MeasurementControl.isACMode = aACMode;
    DSO_setACMode(aACMode);
    if (aACMode) {
        // AC range on
        // Zero line is at grid 3 if ACRange == true
        MeasurementControl.OffsetGridCount = DISPLAY_AC_ZERO_OFFSET_GRID_COUNT;
        MeasurementControl.RawOffsetValueForDisplayRange = getRawOffsetValueFromGridCount(DISPLAY_AC_ZERO_OFFSET_GRID_COUNT);
    } else {
        MeasurementControl.RawOffsetValueForDisplayRange = 0;
        MeasurementControl.OffsetGridCount = 0;
    }
}

/**
 *
 * @param aNewDisplayRangeIndex
 * @param aClipToInputRangeIndex (true for automatic display range)
 * @return false if new value was clipped
 */ //
bool setDisplayRange(int aNewDisplayRangeIndex, bool aClipToInputRangeIndex) {
    bool tRetValue = true;
    // check for bounds
    if (aNewDisplayRangeIndex < 0) {
        aNewDisplayRangeIndex = 0;
        tRetValue = false;
    } else if (aNewDisplayRangeIndex > NUMBER_OF_RANGES) {
        aNewDisplayRangeIndex = NUMBER_OF_RANGES;
        tRetValue = false;
    }
    MeasurementControl.DisplayRangeIndex = aNewDisplayRangeIndex;
    if ((aNewDisplayRangeIndex > MeasurementControl.InputRangeIndex && aClipToInputRangeIndex)
            || aNewDisplayRangeIndex == MeasurementControl.InputRangeIndex) {
        // Set IndexDisplayRange to InputRangeIndex
        MeasurementControl.DisplayRangeIndex = MeasurementControl.InputRangeIndex;
        FactorFromInputToDisplayRangeShift12 = 1 << DSO_INPUT_TO_DISPLAY_SHIFT;
        MeasurementControl.InputRangeIndexOtherThanDisplayRange = false;
    } else {
        // set DisplayRangeIndex to value other (higher or lower!) than InputRangeIndex
        MeasurementControl.InputRangeIndexOtherThanDisplayRange = true;
        FactorFromInputToDisplayRangeShift12 = (RawAttenuationFactor[MeasurementControl.InputRangeIndex]
                * (1 << DSO_INPUT_TO_DISPLAY_SHIFT)) / RawAttenuationFactor[MeasurementControl.DisplayRangeIndex];
    }
    return tRetValue;
}

/**
 * Handler for display range up / down
 * @param aValue to add to MeasurementControl.IndexDisplayRange
 * @return true if no clipping if index occurs
 */
int changeDisplayRange(int aValue) {
    int tFeedbackType = FEEDBACK_TONE_SHORT_ERROR;
    float tOldFactor = ScaleVoltagePerDiv[MeasurementControl.DisplayRangeIndex];
    if (setDisplayRange(MeasurementControl.DisplayRangeIndex + aValue, false)) {
        tFeedbackType = FEEDBACK_TONE_NO_ERROR;
    }
    // adjust OffsetGridCount so that bottom line will stay
    setOffsetGridCount(MeasurementControl.OffsetGridCount * tOldFactor / ScaleVoltagePerDiv[MeasurementControl.DisplayRangeIndex]);

    return tFeedbackType;
}

/**
 * Sets grid count and depending values and draws new grid and labels
 * @param aOffsetGridCount
 */
void setOffsetGridCount(int aOffsetGridCount) {
    MeasurementControl.OffsetGridCount = aOffsetGridCount;
    //Formula is: MeasurementControl.ValueOffset = tOffsetDivCount * RawPerDiv[tNewRangeIndex];
    MeasurementControl.RawOffsetValueForDisplayRange = getRawOffsetValueFromGridCount(aOffsetGridCount);
    MeasurementControl.RawValueOffsetClippingLower = getInputRawFromDisplayValue(DISPLAY_VALUE_FOR_ZERO); // value for bottom of display
    MeasurementControl.RawValueOffsetClippingUpper = getInputRawFromDisplayValue(0); // value for top of display
}

/**
 * Real timebase change is done here
 * @param aForceSetPrescaler forces setting of prescaler (needed for initializing)
 */
void changeTimeBase(bool aForceSetPrescaler) {
    int tOldIndex = MeasurementControl.TimebaseIndex;
    MeasurementControl.TimebaseIndex = MeasurementControl.TimebaseNewIndex;
    ADC_SetTimer6Period(TimebaseTimerDividerValues[MeasurementControl.TimebaseIndex],
            TimebaseTimerPrescalerDividerValues[MeasurementControl.TimebaseIndex]);
    // so set matching sampling times for channel
    ADC_SetChannelSampleTime(DSO_ADC_ID, ADCInputMUXChannels[MeasurementControl.ADCInputMUXChannelIndex],
            (MeasurementControl.TimebaseIndex < TIMEBASE_FAST_MODES));

    if (aForceSetPrescaler || ADCClockPrescalerValues[MeasurementControl.TimebaseIndex] != ADCClockPrescalerValues[tOldIndex]) {
        // stop and start is really needed :-(
        // first disable ADC otherwise sometimes interrupts just stop after the first reading
        ADC_disableAndWait(DSO_ADC_ID );
        ADC_SetClockPrescaler(ADCClockPrescalerValues[MeasurementControl.TimebaseIndex]);
        ADC_enableAndWait(DSO_ADC_ID );
    }

    if (MeasurementControl.TimebaseIndex < TIMEBASE_NUMBER_OF_XSCALE_CORRECTION) {
        DisplayControl.XScale = xScaleForTimebase[MeasurementControl.TimebaseIndex];
    } else {
        DisplayControl.XScale = 0;
    }
    DataBufferControl.DrawWhileAcquire = (MeasurementControl.TimebaseIndex >= TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE);
    MeasurementControl.TriggerTimeoutSampleOrLoopCount = computeNumberOfSamplesToTimeout(MeasurementControl.TimebaseIndex);
    printInfo();
}

/**
 * adjust (copy around) the cyclic pre trigger buffer in order to have them at linear time
 * app. 100 us
 */
void adjustPreTriggerBuffer(uint16_t* tTempBuffer) {
// align pre trigger buffer
    int tCount = 0;
    uint16_t* tDestPtr;
    uint16_t* tSrcPtr;
    if ((DataBufferControl.DataBufferPreTriggerNextPointer == &DataBufferControl.DataBuffer[0])
            || MeasurementControl.TriggerMode == TRIGGER_MODE_OFF) {
        return;
    }

    tCount = &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE] - DataBufferControl.DataBufferPreTriggerNextPointer;

    /*
     * If Modus is DrawWhileAcquire and pre trigger buffer was only written once,
     * (since trigger condition was met before buffer wrap around)
     * then the tail buffer region from last pre trigger value to end of pre trigger region is invalid.
     */ //
    bool tLastPretriggerRegionInvalid = (DataBufferControl.DrawWhileAcquire
            && MeasurementControl.TriggerSampleCount < DATABUFFER_PRE_TRIGGER_SIZE);
    if (!tLastPretriggerRegionInvalid) {
// 1. shift region from last pre trigger value to end of pre trigger region to start of buffer. Use temp buffer (DisplayLineBuffer)
        tDestPtr = tTempBuffer;
        tSrcPtr = DataBufferControl.DataBufferPreTriggerNextPointer;
        memcpy(tDestPtr, tSrcPtr, tCount * sizeof(*tDestPtr));
    }
// 2. shift region from beginning to last pre trigger value to end of pre trigger region
    tDestPtr = &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE - 1];
    tSrcPtr = &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE - tCount - 1];
//	this does not work :-(  memmove(tDestPtr, tSrcPtr, (DATABUFFER_PRE_TRIGGER_SIZE - tCount) * sizeof(*tDestPtr));
    for (int i = DATABUFFER_PRE_TRIGGER_SIZE - tCount; i > 0; --i) {
        *tDestPtr-- = *tSrcPtr--;
    }
    tDestPtr++;
// 3. copy temp buffer to start of pre trigger region
    tSrcPtr = tTempBuffer;
    tDestPtr = DataBufferControl.DataBuffer;
    if (tLastPretriggerRegionInvalid) {
// set invalid values in pretrigger area to special value
        for (int i = tCount; i > 0; --i) {
            *tDestPtr++ = DATABUFFER_INVISIBLE_RAW_VALUE;
        }
// memset can only set bytes :-(  memset(tDestPtr, DATABUFFER_INVISIBLE_RAW_VALUE, tCount * sizeof(*tDestPtr));
    } else {
        memcpy(tDestPtr, tSrcPtr, tCount * sizeof(*tDestPtr));
    }
}

/**
 * 3ms for FFT with -OS
 */
void computeFFT(uint16_t * aDataBufferPointer, float32_t *aFFTBuffer) {
    arm_cfft_radix4_instance_f32 tFFTControlStruct;
    int i;

    uint32_t tTime = getMillisSinceBoot();

    // generates FFT input array
    for (i = 0; i < FFT_SIZE; ++i) {
        *aFFTBuffer++ = getFloatFromRawValue(*aDataBufferPointer++);
        *aFFTBuffer++ = 0.0;
    }
    // Initialize the CFFT/CIFFT module 256 samples, no IFFT, bitReverse
    arm_cfft_radix4_init_f32(&tFFTControlStruct, FFT_SIZE, 0, 1);

    aFFTBuffer = (float32_t *) FourDisplayLinesBuffer;
    /* Process the data through the CFFT/CIFFT module */
    arm_cfft_radix4_f32(&tFFTControlStruct, aFFTBuffer);

    aFFTBuffer = &((float32_t *) FourDisplayLinesBuffer)[2]; // skip DC value
    float32_t *tOutBuffer = (float32_t *) FourDisplayLinesBuffer;
    float32_t tRealValue, tImaginaryValue;
    float tMaxValue = 0.0;
    int tMaxIndex = 0;

    *tOutBuffer++ = 0; // set DC to zero so it does not affect the scaling
    // use only the first half of bins
    for (i = 1; i < FFT_SIZE / 2; ++i) {
        /*
         * in place conversion
         */
        tRealValue = *aFFTBuffer++;
        tImaginaryValue = *aFFTBuffer++;
        tRealValue = sqrtf((tRealValue * tRealValue) + (tImaginaryValue * tImaginaryValue));
        // find max bin value for scaling and frequency display
        if (tRealValue > tMaxValue) {
            tMaxValue = tRealValue;
            tMaxIndex = i;
        }
        *tOutBuffer++ = tRealValue;
    }
    FFTInfo.MaxValue = tMaxValue;
    FFTInfo.MaxIndex = tMaxIndex;
    FFTInfo.TimeElapsedMillis = getMillisSinceBoot() - tTime;

}
