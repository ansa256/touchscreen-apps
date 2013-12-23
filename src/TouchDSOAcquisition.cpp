/**
 * TouchDSOAcquisition.cpp
 *
 * @date 29.03.2012
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
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
#ifdef __cplusplus
extern "C" {
#include "stm32f3_discovery.h"  // For LEDx
}
#endif

/*****************************
 * Timebase stuff
 *****************************/

// first 3 entries are realized by xScale > 1 and not by higher ADC clock -> see TIMEBASE_INDEX_MILLIS
const uint8_t xScaleForTimebase[TIMEBASE_NUMBER_OF_XSCALE_CORRECTION] = { 31, 12, 6, 3, 2 }; // only for GUI
const uint16_t TimebaseDivValues[TIMEBASE_NUMBER_OF_ENTRIES] = { 200, 500, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1, 2, 5, 10, 20, 50,
		100, 200, 500, 1000 }; // only for Display - in nanoseconds/microseconds/milliseconds

// (1/72) * TimebaseTimerDividerValues * (GridSize =32) / xScaleForTimebase - for frequency
const float TimebaseExactDivValues[TIMEBASE_NUMBER_OF_EXCACT_ENTRIES] = { 0.200716845878, 0.518518518, 1.037037037, 2.074074074,
		4.8888888, 9.7777777, 20, 49.777777 };

// Timer cannot divide by 1!
// ADC needs at last 14 cycles for 12 bit conversion and fastest sample time of 1.5
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

/*
 * Data buffer
 */
struct DataBufferStruct DataBufferControl;

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
 */
float RawAttenuationFactor[NUMBER_OF_HARDWARE_RANGES] = { 0.5, 1, 4, 41.6666, 100 };
const uint8_t RangeToRawAttenuationIndexMapping[NUMBER_OF_RANGES] = { 0, 1, 1, 1, 1, 1, 2, 2, 3, 3, 3, 4 };
float getRawAttenuationFactor(int rangeIndex) {
	return RawAttenuationFactor[RangeToRawAttenuationIndexMapping[rangeIndex]];
}

// Mapping for external attenuator hardware
#define ATTENUATOR_INFINITE_VALUE 0 // value for which attenuator shortens the input
const uint8_t AttenuatorHardwareValue[NUMBER_OF_RANGES] = { 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05 };
// reasonable minimal display range until we see the native ADC resolution (12bit->8bit gives factor 16 gives 4 ranges less (which are factor20-25))
const uint8_t PossibleMinimumRange[NUMBER_OF_RANGES] = { 0, 1, 1, 1, 1, 1, 3, 3, 6, 6, 6, 7 };

uint16_t RawDSOReadingACZero;

/**
 * Virtual ADC values for full scale (DisplayHeight) - only used for SetAutoRange()
 */
int MaxPeakToPeakValue[NUMBER_OF_RANGES]; // for a virtual 18 bit ADC

void autoACZeroCalibration(void);

/*******************************************************************************************
 * Program code starts here
 *******************************************************************************************/

void initMaxPeakToPeakValues(void) {
	// maximal virtual reading for 6 div's - only used for setAutoRange()
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
 * called at start of application (startDSO)
 */
void initAcquisition(void) {

	autoACZeroCalibration(); // sets MeasurementControl.DSOReadingACZero

	MeasurementControl.IsRunning = false;
	MeasurementControl.TriggerSlopeRising = true;
	MeasurementControl.ADS7846ChannelsActive = false;
	MeasurementControl.IndexADCInputMUXChannel = START_ADC_CHANNEL_INDEX;

	// Set attenuator / range
	MeasurementControl.IndexInputRange = NUMBER_OF_RANGES - 1; // max attenuation
	MeasurementControl.IndexDisplayRange = NUMBER_OF_RANGES - 1;
	MeasurementControl.InputRangeIndexOtherThanDisplayRange = false;
	DSO_setAttenuator(AttenuatorHardwareValue[NUMBER_OF_RANGES - 1]);
	MeasurementControl.RangeAutomatic = true;
	MeasurementControl.OffsetAutomatic = false;

	// AC range on
	MeasurementControl.ACRange = true;
	DSO_setACRange(MeasurementControl.ACRange);
	// Zero line is at grid 3 if ACRange == true
	MeasurementControl.OffsetGridCount = DISPLAY_AC_ZERO_OFFSET_GRID_COUNT;
	MeasurementControl.RawOffsetValueForDisplayRange = getRawOffsetValueFromGridCount(DISPLAY_AC_ZERO_OFFSET_GRID_COUNT);

	// Trigger
	MeasurementControl.TriggerMode = TRIGGER_MODE_AUTOMATIC;
	MeasurementControl.isSingleShotMode = false;

	// Timebase
	MeasurementControl.TimebaseFastDMAMode = false;
	MeasurementControl.doPretriggerCopyForDisplay = false;
	MeasurementControl.TimestampLastRangeChange = 0; // enable direct range change at start
	MeasurementControl.IndexTimebase = TIMEBASE_INDEX_START_VALUE;
	ADC_SetClockPrescaler(ADCClockPrescalerValues[TIMEBASE_INDEX_START_VALUE]);
	ADC_SetTimer6Period(TimebaseTimerDividerValues[TIMEBASE_INDEX_START_VALUE],
			TimebaseTimerPrescalerDividerValues[TIMEBASE_INDEX_START_VALUE]);
	DataBufferControl.DrawWhileAcquire = (TIMEBASE_INDEX_START_VALUE >= TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE); // false here
	MeasurementControl.TriggerTimeoutSampleOrLoopCount = computeNumberOfSamplesToTimeout(TIMEBASE_INDEX_START_VALUE);

	MeasurementControl.InfoSizeSmall = false;

	DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[DATABUFFER_DISPLAY_START];
	ADC_SetChannelSampleTime(DSO_ADC_ID, ADCInputMUXChannels[MeasurementControl.IndexADCInputMUXChannel],
			MeasurementControl.TimebaseFastDMAMode);

	initMaxPeakToPeakValues();
	initRawToDisplayFactors();
}

/*
 * prepares all variables for new acquisition
 * switches between fast an interrupt mode depending on TIMEBASE_FAST_MODES
 * sets ADC status register including prescaler
 * setup new interrupt cycle only if not to be stopped
 */
void startAcquisition(void) {

	if (MeasurementControl.ADS7846ChannelsActive) {
		// to avoid pre trigger buffer adjustment
		DataBufferControl.DataBufferPreTriggerNextPointer = &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE];
		// because of no interrupt handling for ADS7846Channels
		return;
	}
	if (MeasurementControl.isSingleShotMode) {
		// Start and request immediate stop
		MeasurementControl.StopRequested = true;
	} else {
		DataBufferControl.DataBufferEndPointer = &DataBufferControl.DataBuffer[DATABUFFER_DISPLAY_END];
	}
	if (MeasurementControl.StopRequested) {
		// last acquisition or single shot mode -> use all data buffer
		DataBufferControl.DataBufferEndPointer = &DataBufferControl.DataBuffer[DATABUFFER_SIZE - 1];
	}

	MeasurementControl.StopAcknowledged = false;
	DataBufferControl.IndexInputRangeUsed = MeasurementControl.IndexInputRange;
	DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[DATABUFFER_DISPLAY_START];
	DataBufferControl.DataBufferNextPointer = &DataBufferControl.DataBuffer[0];
	DataBufferControl.DataBufferNextDrawPointer = &DataBufferControl.DataBuffer[0];
	DataBufferControl.NextDrawXValue = 0;
	DataBufferControl.DataBufferFull = false;
// start with waiting for triggering condition
	MeasurementControl.TriggerSampleCount = 0;
	MeasurementControl.TriggerStatus = TRIGGER_START;
	MeasurementControl.ActualPhase = PHASE_PRE_TRIGGER;
	MeasurementControl.TriggerPhaseJustEnded = false;
	MeasurementControl.doPretriggerCopyForDisplay = false;

	MeasurementControl.TimebaseFastDMAMode = false;
	if (MeasurementControl.IndexTimebase < TIMEBASE_FAST_MODES) {
		// TimebaseFastFreerunningMode must be set only here at beginning of acquisition
		MeasurementControl.TimebaseFastDMAMode = true;
	} else if (MeasurementControl.IndexTimebase >= TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE) {
		MeasurementControl.ActualPhase = PHASE_SEARCH_TRIGGER;
	}

	/*
	 * Start ADC + timer
	 */
	ADC_startTimer6();
	if (!MeasurementControl.TimebaseFastDMAMode) {
		ADC_enableEOCInterrupt(DSO_ADC_ID);
		// starts conversion at next timer edge
		ADC_StartConversion(DSO_ADC_ID);
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
uint16_t computeNumberOfSamplesToTimeout(uint16_t aTimebaseIndex) {
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
/*
 * read ADS7846 channels
 */
void readADS7846Channels(void) {

	uint16_t tValue;
	uint16_t *DataPointer = &DataBufferControl.DataBuffer[0];

	tValue = TouchPanel.readChannel(ADS7846ChannelMapping[MeasurementControl.IndexADCInputMUXChannel], true, false, 0x1);
	while (DataPointer <= DataBufferControl.DataBufferEndPointer) {
		tValue = TouchPanel.readChannel(ADS7846ChannelMapping[MeasurementControl.IndexADCInputMUXChannel], true, false, 0x1);
		*DataPointer++ = tValue;
	}

	DataBufferControl.DataBufferFull = true;
}

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
			do {
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
			} while (tDMAMemoryAddress < tEndMemoryAddress);
			if (tTriggerStatus == TRIGGER_OK) {
				// success exit from loop
				break;
			}
			// check if new DMA Transfer must be started - see setting below "if (tCount <..."
			if (tEndMemoryAddress
					>= &DataBufferControl.DataBuffer[DATABUFFER_SIZE - (DSO_DISPLAY_WIDTH - DATABUFFER_PRE_TRIGGER_DISPLAY_SIZE) - 1]
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
				// reset transfer complete status before return
				DMA_ClearITPendingBit(DMA1_IT_TC1);
				DataBufferControl.DataBufferFull = false;
				return;
			} else {
				// set new tEndMemoryAddress and check if it is before last possible trigger position
				uint32_t tCount = DSO_DMA_CHANNEL->CNDTR;
				if (tCount < DSO_DISPLAY_WIDTH - DATABUFFER_PRE_TRIGGER_DISPLAY_SIZE) {
					tCount = DSO_DISPLAY_WIDTH - DATABUFFER_PRE_TRIGGER_DISPLAY_SIZE;
				}
				tEndMemoryAddress = &DataBufferControl.DataBuffer[DATABUFFER_SIZE - tCount - 1];
			}
		} while (true); // end with break above

		/*
		 * set pointer for display of data
		 */
		if (tTriggerStatus != TRIGGER_OK) {
			// timeout here
			tDMAMemoryAddress = &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_DISPLAY_SIZE];
		}
		// correct start by DisplayControl.XScale - XScale is known to be >=0 here
		DataBufferControl.DataBufferDisplayStart = tDMAMemoryAddress - 1
				- adjustIntWithScaleFactor(DATABUFFER_PRE_TRIGGER_DISPLAY_SIZE, DisplayControl.XScale);
		if (MeasurementControl.StopRequested) {
			MeasurementControl.StopAcknowledged = true;
		} else {
			// set end pointer to end of display for reproducible min max + average findings - XScale is known to be >=0 here
			int tAdjust = adjustIntWithScaleFactor(DSO_DISPLAY_WIDTH - DATABUFFER_PRE_TRIGGER_DISPLAY_SIZE - 1,
					DisplayControl.XScale);
			DataBufferControl.DataBufferEndPointer = tDMAMemoryAddress + tAdjust;
		}
	}
}

extern "C" void DMA1_Channel1_IRQHandler(void) {

	// Test on DMA Transfer Complete interrupt
	if (DMA_GetITStatus(DMA1_IT_TC1)) {
		/* Clear DMA  Transfer Complete interrupt pending bit */
		DMA_ClearITPendingBit(DMA1_IT_TC1);
		DataBufferControl.DataBufferFull = true;
		ADC_StopConversion(DSO_ADC_ID);
	}
	// Test on DMA Transfer Error interrupt
	if (DMA_GetITStatus(DMA1_IT_TE1)) {
		failParamMessage( DSO_DMA_CHANNEL ->CPAR, "DMA Error");
		DMA_ClearITPendingBit(DMA1_IT_TE1);
	}
	// Test on DMA Transfer Half interrupt
	if (DMA_GetITStatus(DMA1_IT_HT1)) {
		DMA_ClearITPendingBit(DMA1_IT_HT1);
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

//	uint16_t tValue = ADC_GetConversionValue(DSO_ADC_ID );
	uint16_t tValue = DSO_ADC_ID->DR;

	/*
	 * read at least DATABUFFER_PRE_TRIGGER_SIZE values in pre trigger phase
	 */
	if (MeasurementControl.ActualPhase == PHASE_PRE_TRIGGER) {
		// store value
		*DataBufferControl.DataBufferNextPointer++ = tValue;
		MeasurementControl.TriggerSampleCount++;
		if (MeasurementControl.TriggerSampleCount >= DATABUFFER_PRE_TRIGGER_SIZE) {
			// now we have read at least DATABUFFER_PRE_TRIGGER_SIZE values => start search for trigger
			if (MeasurementControl.TriggerMode == TRIGGER_MODE_OFF) {
				MeasurementControl.ActualPhase = PHASE_POST_TRIGGER;
				return;
			} else {
				MeasurementControl.ActualPhase = PHASE_SEARCH_TRIGGER;
			}
			MeasurementControl.TriggerSampleCount = 0;
			DataBufferControl.DataBufferNextPointer = DataBufferControl.DataBuffer;
		}
		return;
	}
	if (MeasurementControl.ActualPhase == PHASE_SEARCH_TRIGGER) {
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

		uint16_t * tDataBufferPointer = DataBufferControl.DataBufferNextPointer;
		if (!tTriggerFound) {
			/*
			 * store value - can wrap around in pre trigger area
			 */
			// store value
			*tDataBufferPointer++ = tValue;
			MeasurementControl.TriggerSampleCount++;
			// detect end of pre trigger buffer
			if (tDataBufferPointer >= &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE]) {
				// wrap around
				DataBufferControl.DataBufferWrapAround = true;
				tDataBufferPointer = &DataBufferControl.DataBuffer[0];
			}
			// prepare for next
			DataBufferControl.DataBufferNextPointer = tDataBufferPointer;

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
		MeasurementControl.ActualPhase = PHASE_POST_TRIGGER;
		DataBufferControl.DataBufferPreTriggerNextPointer = tDataBufferPointer;
		/*
		 * only needed for draw while acquire
		 * set flag for main loop to detect end of trigger phase
		 */
		MeasurementControl.TriggerPhaseJustEnded = true;
		tDataBufferPointer = &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE];
		// store value
		*tDataBufferPointer++ = tValue;
		DataBufferControl.DataBufferNextPointer = tDataBufferPointer;
		return;
	}

	/*
	 * detect end of buffer
	 */
	uint16_t * tDataBufferPointer = DataBufferControl.DataBufferNextPointer;
	if (tDataBufferPointer <= DataBufferControl.DataBufferEndPointer) {
		// store display value
		*tDataBufferPointer++ = tValue;
		// prepare for next
		DataBufferControl.DataBufferNextPointer = tDataBufferPointer;
	} else {
		// stop acquisition
		// End of conversion => stop ADC in order to make it reconfigurable (change timebase)
		ADC_StopConversion(DSO_ADC_ID);
		ADC_disableEOCInterrupt(DSO_ADC_ID);
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
	DSO_setACRange(true);
	DSO_setAttenuator(ATTENUATOR_INFINITE_VALUE);
	ADC1_getChannelValue(ADCInputMUXChannels[START_ADC_CHANNEL_INDEX]);
	// wait to settle
	delayMillis(10);
	uint16_t tSum = 0;
	for (int i = 0; i < 16; ++i) {
		uint16_t tVal = ADC1_getChannelValue(ADCInputMUXChannels[START_ADC_CHANNEL_INDEX]);
		if (tVal > ADC_MAX_CONVERSION_VALUE) {
			// conversion impossible -> set fallback value
			RawDSOReadingACZero = ADC_MAX_CONVERSION_VALUE / 2;
			return;
		}
		tSum += ADC1_getChannelValue(ADCInputMUXChannels[START_ADC_CHANNEL_INDEX]);
	}
	RawDSOReadingACZero = tSum >> 4;
}

/**
 * Get max and min and average for display and automatic triggering.
 * Use only post trigger area!
 */
void findMinMaxAverage(void) {
	uint16_t * tDataBufferPointer = DataBufferControl.DataBufferDisplayStart + DATABUFFER_PRE_TRIGGER_DISPLAY_SIZE;
	if (DataBufferControl.DataBufferEndPointer > tDataBufferPointer) {
		uint16_t tAcquisitionSize = DataBufferControl.DataBufferEndPointer + 1 - tDataBufferPointer;
		uint16_t tMax, tMin;
		tMax = *tDataBufferPointer;
		tMin = *tDataBufferPointer;
		uint16_t tValue;
		uint32_t tIntegrateValue = 0;
		for (int i = 0; i < tAcquisitionSize; ++i) {
			tValue = *tDataBufferPointer++;
			if (tValue > tMax) {
				tMax = tValue;
			} else if (tValue < tMin) {
				tMin = tValue;
			}
			tIntegrateValue += tValue;
		}

		MeasurementControl.RawValueMin = tMin;
		MeasurementControl.RawValueMax = tMax;
		MeasurementControl.RawValueAverage = (tIntegrateValue + (tAcquisitionSize / 2)) / tAcquisitionSize;
	}
}

/**
 * compute hysteresis, shift and offset such that graph are from bottom to DISPLAY_USAGE
 * If old values are reasonable don't change them to avoid jitter
 */
void computeAutoTrigger(void) {

	int tPeakToPeak = MeasurementControl.RawValueMax;
	if (MeasurementControl.TriggerMode == TRIGGER_MODE_AUTOMATIC) {
		/*
		 * Set auto trigger in middle between min and max
		 */
		tPeakToPeak -= MeasurementControl.RawValueMin;
		int tTriggerValue = MeasurementControl.RawValueMin + (tPeakToPeak / 2);

		/*
		 * set effective hysteresis to quarter delta
		 */
		int TriggerHysteresis = tPeakToPeak / 4;
		int tTriggerDelta = abs(tTriggerValue - MeasurementControl.RawTriggerLevel);

		// keep reasonable value - avoid jitter
		if (tTriggerDelta > TriggerHysteresis >> 2) {
			setLevelAndHysteresis(tTriggerValue, TriggerHysteresis);
		}
	}
}

/**
 * Real ADC PeakToPeakValue is multiplied by RealAttenuationFactor[MeasurementControl.TotalRangeIndex]
 * to get values of a virtual 18 bit ADC
 */
void computeAutoRange(void) {
	if (MeasurementControl.RangeAutomatic) {
		/*
		 * set new auto range
		 * first look for clipping (check 0 and ADC_MAX_CONVERSION_VALUE)
		 */
		// check clipping for autoRange
		if (MeasurementControl.RawValueMax == ADC_MAX_CONVERSION_VALUE
				|| (MeasurementControl.ACRange && MeasurementControl.RawValueMin == 0)) {
			int tRangeChangeValue = 1;
			// find next hardware range
			for (int i = MeasurementControl.IndexInputRange; i < NUMBER_OF_RANGES - 1; ++i) {
				if (RangeToRawAttenuationIndexMapping[i] != RangeToRawAttenuationIndexMapping[i + 1]) {
					break;
				}
			}
			changeInputRange(tRangeChangeValue); // adjusts trigger level if needed
		}

		/*
		 * no clipping occurred -> check if another range is more suitable
		 */
		// get relevant peak2peak value
		int tPeakToPeak = MeasurementControl.RawValueMax;
		if (MeasurementControl.ACRange) {
			if (RawDSOReadingACZero - MeasurementControl.RawValueMin > MeasurementControl.RawValueMax - RawDSOReadingACZero) {
				//difference between zero and min is greater than difference between zero and max => min determines the range
				tPeakToPeak = RawDSOReadingACZero - MeasurementControl.RawValueMin;
			} else {
				tPeakToPeak -= RawDSOReadingACZero;
			}
			// since tPeakToPeak must fit in half of display
			tPeakToPeak *= 2;
		}

		// get virtual value (simulate an virtual 18 bit ADC)
		tPeakToPeak *= getRawAttenuationFactor(MeasurementControl.IndexInputRange);

		int tOldRangeIndex = MeasurementControl.IndexInputRange;
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
		if (tOldRangeIndex != tNewRangeIndex) {
			/*
			 * delay for changing range to higher resolution
			 */
			if (tNewRangeIndex > tOldRangeIndex) {
				changeInputRange(tNewRangeIndex - tOldRangeIndex);
			} else {

				// wait n-milliseconds before switch to higher resolution (lower index)
				if (getMillisSinceBoot() - MeasurementControl.TimestampLastRangeChange > SCALE_CHANGE_DELAY_MILLIS) {
					MeasurementControl.TimestampLastRangeChange = getMillisSinceBoot();
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
			MeasurementControl.TimestampLastRangeChange = getMillisSinceBoot();
		}
	}
}

/**
 * Sets grid count and depending values
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
 * Real ADC PeakToPeakValue is multiplied by RealAttenuationFactor[MeasurementControl.TotalRangeIndex]
 * to get values of a virtual 18 bit ADC
 */
void computeAutoOffset(void) {
	if (MeasurementControl.OffsetAutomatic) {
		/*
		 * check if another range is more suitable
		 */
		// get relevant peak2peak value
		int tPeakToPeak = MeasurementControl.RawValueMax - MeasurementControl.RawValueMin;
		// increase value by 10/8 (1.25) to have margin to have minimum starting at first grid line - exact factor for this would be (6/5 = 1.2)
		tPeakToPeak = (tPeakToPeak * 10) / 8;
		// get virtual value
		tPeakToPeak *= getRawAttenuationFactor(MeasurementControl.IndexInputRange);

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

		if (MeasurementControl.IndexDisplayRange > MeasurementControl.IndexInputRange) {
			MeasurementControl.IndexDisplayRange = MeasurementControl.IndexInputRange;
			MeasurementControl.InputRangeIndexOtherThanDisplayRange = false;
			FactorFromInputToDisplayRangeShift12 = 1 << DSO_INPUT_TO_DISPLAY_SHIFT;
		}
		if (tNewRangeIndex < PossibleMinimumRange[MeasurementControl.IndexInputRange]) {
			// clip to reasonable value according to ADC resolution
			tNewRangeIndex = PossibleMinimumRange[MeasurementControl.IndexInputRange];
		}
		/*
		 * Change offset only if range changed
		 * and is lower than input range (must be checked because of OFFSET_FIT_FACTOR)
		 * or actual min < actual offset
		 * or actual max > (actual offset + value for full scale)
		 */
		if ((tNewRangeIndex <= MeasurementControl.IndexInputRange && tNewRangeIndex != MeasurementControl.IndexDisplayRange)
				|| MeasurementControl.RawValueMin < MeasurementControl.RawValueOffsetClippingLower
				|| MeasurementControl.RawValueMax > MeasurementControl.RawValueOffsetClippingUpper) {
			/*
			 * magnification by using offset and a different display range is possible here
			 */
			// Avoid jitter
			if (tNewRangeIndex != MeasurementControl.IndexDisplayRange) {
				if (tNewRangeIndex > MeasurementControl.IndexDisplayRange
						|| ((tPeakToPeak * 10) / 8) <= MaxPeakToPeakValue[tNewRangeIndex]) {
					// switch to higher resolution only if actual value is less than 80% of max possible value
					MeasurementControl.IndexDisplayRange = tNewRangeIndex;
					MeasurementControl.InputRangeIndexOtherThanDisplayRange = true;
					FactorFromInputToDisplayRangeShift12 = (getRawAttenuationFactor(MeasurementControl.IndexInputRange)
							* (1 << DSO_INPUT_TO_DISPLAY_SHIFT)) / getRawAttenuationFactor(MeasurementControl.IndexDisplayRange);
				}
			} // else range is same but clipping occurs

			/*
			 * compute offset based on center value in order display values at center of screen
			 */
			int tValueMiddleX = MeasurementControl.RawValueMin
					+ ((MeasurementControl.RawValueMax - MeasurementControl.RawValueMin) / 2);
			if (MeasurementControl.ACRange) {
				tValueMiddleX -= RawDSOReadingACZero;
			}
			tValueMiddleX = (tValueMiddleX * FactorFromInputToDisplayRangeShift12) >> DSO_INPUT_TO_DISPLAY_SHIFT;

			// Adjust to multiple of div
			// formula is: RawPerDiv[i] = HORIZONTAL_GRID_HEIGHT / ScaleFactorRawToDisplay[i]);
			// tOffsetDivCount = tValueMiddleX / RawPerDiv[tNewRangeIndex];
			signed int tOffsetGridCount = tValueMiddleX * (ScaleFactorRawToDisplayShift18[tNewRangeIndex] / HORIZONTAL_GRID_HEIGHT);
			tOffsetGridCount = tOffsetGridCount >> DSO_SCALE_FACTOR_SHIFT;

			// Adjust start OffsetDivCount in order to display values at center of screen
			if (MeasurementControl.ACRange || tOffsetGridCount >= (HORIZONTAL_GRID_COUNT / 2)) {
				tOffsetGridCount -= (HORIZONTAL_GRID_COUNT / 2);
			}
			setOffsetGridCount(tOffsetGridCount);

			drawGridLinesWithHorizLabelsAndTriggerLine(COLOR_GRID_LINES);
		}
	}
}

void setLevelAndHysteresis(int aRawTriggerValue, int aRawTriggerHysteresis) {
	MeasurementControl.RawTriggerLevel = aRawTriggerValue;
	if (MeasurementControl.TriggerSlopeRising) {
		MeasurementControl.RawTriggerLevelHysteresis = aRawTriggerValue - aRawTriggerHysteresis;
	} else {
		MeasurementControl.RawTriggerLevelHysteresis = aRawTriggerValue + aRawTriggerHysteresis;
	}
}

/**
 * adjusts trigger level if needed
 * sets exclusively IndexInputRange, actualDSOReadingACZeroForInputRange
 * @param aValue value to change range
 * @return true if range has changed false otherwise
 */

bool changeInputRange(int aValue) {
	bool tRetValue = true;
	int tNewRange = MeasurementControl.IndexInputRange + aValue;
	if (tNewRange > NUMBER_OF_RANGES - 1) {
		tNewRange = NUMBER_OF_RANGES - 1;
		tRetValue = false;
	} else if (tNewRange < 0) {
		tNewRange = 0;
		tRetValue = false;
	} else if (MeasurementControl.IndexADCInputMUXChannel >= ADC_CHANNEL_NO_ATTENUATOR_INDEX
			&& (tNewRange > ADC_CHANNEL_NO_ATTENUATOR_MAX_RANGE_INDEX || tNewRange < ADC_CHANNEL_NO_ATTENUATOR_MIN_RANGE_INDEX)) {
		// direct ADC channel with no attenuator
		tRetValue = false;
	} else {
		float tOldDSORawToVoltFactor = actualDSORawToVoltFactor;
		MeasurementControl.IndexInputRange = tNewRange;
		if (MeasurementControl.IndexADCInputMUXChannel < ADC_CHANNEL_NO_ATTENUATOR_INDEX) {
			// set attenuator and conversion factor
			DSO_setAttenuator(AttenuatorHardwareValue[tNewRange]);
		}
		actualDSORawToVoltFactor = ADCToVoltFactor * getRawAttenuationFactor(MeasurementControl.IndexInputRange);

		if (MeasurementControl.TriggerMode != TRIGGER_MODE_OFF) {
			// adjust trigger raw level if needed
			if (tOldDSORawToVoltFactor != actualDSORawToVoltFactor) {
				int tAcCompensation = 0;
				if (MeasurementControl.ACRange) {
					tAcCompensation = RawDSOReadingACZero;
				}
				MeasurementControl.RawTriggerLevel = (((MeasurementControl.RawTriggerLevel - tAcCompensation)
						* tOldDSORawToVoltFactor) / actualDSORawToVoltFactor) + tAcCompensation;
				MeasurementControl.RawTriggerLevelHysteresis = (((MeasurementControl.RawTriggerLevelHysteresis - tAcCompensation)
						* tOldDSORawToVoltFactor) / actualDSORawToVoltFactor) + tAcCompensation;
			}
		}

		// keep labels up to date
		if (!MeasurementControl.OffsetAutomatic) {
			MeasurementControl.IndexDisplayRange = MeasurementControl.IndexInputRange;
			drawGridLinesWithHorizLabelsAndTriggerLine(COLOR_GRID_LINES);
			if (MeasurementControl.ACRange) {
				// adjust new offset for ac
				MeasurementControl.RawOffsetValueForDisplayRange = getRawOffsetValueFromGridCount(
						DISPLAY_AC_ZERO_OFFSET_GRID_COUNT);
			}
		}
	}
	return tRetValue;
}

/**
 * sets absolute range, for handling of direct ADC channels
 */bool setInputRange(int aValue) {
	return changeInputRange(aValue - MeasurementControl.IndexInputRange);
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

	/*
	 * If Modus is DrawWhileAcquire and pre trigger buffer was only written once,
	 * (since trigger condition was met before buffer wrap around)
	 * then the tail buffer region from last pre trigger value to end of pre trigger region is invalid.
	 */

	bool tLastRegionInvalid = (DataBufferControl.DrawWhileAcquire
			&& MeasurementControl.TriggerSampleCount < DATABUFFER_PRE_TRIGGER_SIZE);
	tCount = &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE] - DataBufferControl.DataBufferPreTriggerNextPointer;
	if (!tLastRegionInvalid) {
// 1. shift region from last pre trigger value to end of pre trigger region to start of buffer. Use temp buffer (DisplayLineBuffer)
		tDestPtr = tTempBuffer;
		tSrcPtr = DataBufferControl.DataBufferPreTriggerNextPointer;
		memcpy(tDestPtr, tSrcPtr, tCount * sizeof(*tDestPtr));
	}
// 2. shift region from beginning to last pre trigger value to end of pre trigger region
	tDestPtr = &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE - 1];
	tSrcPtr = &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE - tCount - 1];
//	this does not work :-( memmove(tDestPtr, tSrcPtr, (DATABUFFER_PRE_TRIGGER_SIZE - tCount) * sizeof(*tDestPtr));
	for (int i = DATABUFFER_PRE_TRIGGER_SIZE - tCount; i > 0; --i) {
		*tDestPtr-- = *tSrcPtr--;
	}
	tDestPtr++;
	uint16_t tFirstPreTriggerValue = *tDestPtr;
// 3. copy temp buffer to start of pre trigger region
	tSrcPtr = tTempBuffer;
	tDestPtr = DataBufferControl.DataBuffer;
	if (tLastRegionInvalid) {
		memset(tDestPtr, tFirstPreTriggerValue, tCount * sizeof(*tDestPtr));
	} else {
		memcpy(tDestPtr, tSrcPtr, tCount * sizeof(*tDestPtr));
	}
}

