/**
 * TouchDSODisplay.cpp
 * @brief contains the display related functions for DSO.
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
#include "misc.h"
#include "USART_DMA.h"
#include <string.h>
#include "Chart.h" // for drawChartDataFloat()
/*
 * Layout
 */
#define TRIGGER_LEVEL_INFO_LARGE_X INFO_LEFT_MARGIN + (11 * TEXT_SIZE_11_WIDTH)
#define TRIGGER_LEVEL_INFO_SMALL_X (INFO_LEFT_MARGIN + (33 * TEXT_SIZE_11_WIDTH))
#define TRIGGER_HIGH_DISPLAY_OFFSET 7 // for trigger state line
/*
 * COLORS
 */
//Line colors
#define COLOR_MAX_MIN_LINE COLOR_GREEN
#define COLOR_HOR_REF_LINE_LABEL COLOR_BLUE
#define COLOR_HOR_REF_LINE_LABEL_NEGATIVE COLOR_RED
#define COLOR_TRIGGER_LINE RGB(0xFF,0x00,0xFF)

/*****************************
 * Display stuff
 *****************************/
uint8_t DisplayBufferFFT[FFT_SIZE / 2];
uint8_t DisplayBuffer[DSO_DISPLAY_WIDTH]; // Buffer for raw display data of current chart
uint8_t DisplayBuffer2[DSO_DISPLAY_WIDTH]; // Buffer for trigger state line

/*
 * Display control
 * while running switch between upper info line on/off
 * while stopped switch between chart / t+info line and gui
 */

DisplayControlStruct DisplayControl;

#define PIXEL_AFTER_LABEL 2 // Space between end of label and display border
const char * const TriggerStatusStrings[] = { "slope", "level", "nothing" }; // waiting for slope, waiting for trigger level (slope condition is met)

/**
 * scale Values for the horizontal scales
 */
const float ScaleVoltagePerDiv[NUMBER_OF_RANGES] = { 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0, 50.0 };
const uint8_t RangePrecision[NUMBER_OF_RANGES] = { 2, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0 }; // number of digits to be printed after the decimal point

/**
 * The array with the display scale factor for raw -> display for all ranges
 * actual values are shift by 18
 */
int ScaleFactorRawToDisplayShift18[NUMBER_OF_RANGES];
int FactorFromInputToDisplayRangeShift12 = 1 << DSO_INPUT_TO_DISPLAY_SHIFT; // is (attenuation for input range / attenuation for display range)

float actualDSORawToVoltFactor;

/*
 * FFT
 */
Chart ChartFFT;
#define FFT_DISPLAY_SCALE_FACTOR_X 2 // pixel per value
/*******************************************************************************************
 * Program code starts here
 *******************************************************************************************/
void initRawToDisplayFactors(void) {
// used for getFloatFromRawValue() and for changeInputRange()
    actualDSORawToVoltFactor = (ADCToVoltFactor * RawAttenuationFactor[MeasurementControl.InputRangeIndex]);

// Reading * ScaleValueForDisplay = Display value - only used for getDisplayFrowRawValue() and getDisplayFrowRawValue()
// use value shifted by 19 to avoid floating point multiplication by retaining correct value
    ScaleFactorRawToDisplayShift18[11] = sADCScaleFactorShift18 * (RawAttenuationFactor[11] / (2 * ScaleVoltagePerDiv[11])); // 50 Volt almost same value as 0.5V/div
    ScaleFactorRawToDisplayShift18[10] = sADCScaleFactorShift18 * (RawAttenuationFactor[10] / (2 * ScaleVoltagePerDiv[10])); // 20 almost same value as 0.5V/div
    ScaleFactorRawToDisplayShift18[9] = sADCScaleFactorShift18 * (RawAttenuationFactor[9] / (2 * ScaleVoltagePerDiv[9])); // 10
    ScaleFactorRawToDisplayShift18[8] = sADCScaleFactorShift18 * (RawAttenuationFactor[8] / (2 * ScaleVoltagePerDiv[8])); // 5
    ScaleFactorRawToDisplayShift18[7] = sADCScaleFactorShift18 * (RawAttenuationFactor[7] / (2 * ScaleVoltagePerDiv[7])); // 2 Volt - almost same value as 0.5V/div, since external attenuation of 4
    ScaleFactorRawToDisplayShift18[6] = sADCScaleFactorShift18 * (RawAttenuationFactor[6] / (2 * ScaleVoltagePerDiv[6])); // 1 Volt

    ScaleFactorRawToDisplayShift18[5] = sADCScaleFactorShift18 * (RawAttenuationFactor[5] / (2 * ScaleVoltagePerDiv[5])); // full scale 0,5 Volt / div
    ScaleFactorRawToDisplayShift18[4] = sADCScaleFactorShift18 * (RawAttenuationFactor[4] / (2 * ScaleVoltagePerDiv[4])); // 0,2
    ScaleFactorRawToDisplayShift18[3] = sADCScaleFactorShift18 * (RawAttenuationFactor[3] / (2 * ScaleVoltagePerDiv[3])); // 0,1
    ScaleFactorRawToDisplayShift18[2] = sADCScaleFactorShift18 * (RawAttenuationFactor[2] / (2 * ScaleVoltagePerDiv[2])); // 0,05
    ScaleFactorRawToDisplayShift18[1] = sADCScaleFactorShift18 * (RawAttenuationFactor[1] / (2 * ScaleVoltagePerDiv[1])); // 0,02
    ScaleFactorRawToDisplayShift18[0] = sADCScaleFactorShift18 * (RawAttenuationFactor[0] / (2 * ScaleVoltagePerDiv[0])); // 0,01
}

/************************************************************************
 * Graphical output section
 ************************************************************************/

void clearTriggerLine(uint8_t aTriggerLevelDisplayValue) {
    // clear old line
    BlueDisplay1.drawLineRel(0, aTriggerLevelDisplayValue, DSO_DISPLAY_WIDTH, 0, COLOR_BACKGROUND_DSO);
    // restore grid at old y position
    for (int tXPos = TIMING_GRID_WIDTH - 1; tXPos < DSO_DISPLAY_WIDTH - 1; tXPos += TIMING_GRID_WIDTH) {
        BlueDisplay1.drawPixel(tXPos, aTriggerLevelDisplayValue, COLOR_GRID_LINES);
    }
    if (!MeasurementControl.isRunning) {
        // in analysis mode restore graph at old y position
        uint8_t* ScreenBufferPointer = &DisplayBuffer[0];
        for (int i = 0; i < DSO_DISPLAY_WIDTH; ++i) {
            int tValueByte = *ScreenBufferPointer++;
            if (tValueByte == aTriggerLevelDisplayValue) {
                // restore old pixel
                BlueDisplay1.drawPixel(i, tValueByte, COLOR_DATA_HOLD);
            }
        }
    }
}

/**
 * draws trigger line if it is visible - do not draw clipped value e.g. value was higher than display range
 */
void drawTriggerLine(void) {
    if (DisplayControl.TriggerLevelDisplayValue != 0 && MeasurementControl.TriggerMode != TRIGGER_MODE_OFF) {
        BlueDisplay1.drawLineRel(0, DisplayControl.TriggerLevelDisplayValue, DSO_DISPLAY_WIDTH, 0, COLOR_TRIGGER_LINE);
    }
}

/**
 * draws vertical timing + horizontal reference voltage lines
 */
void drawGridLinesWithHorizLabelsAndTriggerLine(uint16_t aColor) {
    //vertical lines
    for (int tXPos = TIMING_GRID_WIDTH - 1; tXPos < DSO_DISPLAY_WIDTH; tXPos += TIMING_GRID_WIDTH) {
        BlueDisplay1.drawLineRel(tXPos, 0, 0, DSO_DISPLAY_HEIGHT, aColor);
    }
    // add 0.0001 to avoid display of -0.00
    float tActualVoltage =
            (ScaleVoltagePerDiv[MeasurementControl.DisplayRangeIndex] * (MeasurementControl.OffsetGridCount) + 0.0001);
    bool tLabelChanged = false;
    if (DisplayControl.LastDisplayRangeIndex != MeasurementControl.DisplayRangeIndex
            || DisplayControl.LastOffsetGridCount != MeasurementControl.OffsetGridCount) {
        DisplayControl.LastDisplayRangeIndex = MeasurementControl.DisplayRangeIndex;
        DisplayControl.LastOffsetGridCount = MeasurementControl.OffsetGridCount;
        tLabelChanged = true;
    }
    int tCaptionOffset = 1;
    int tLabelColor;
    int tPosX;
    for (int tYPos = DISPLAY_VALUE_FOR_ZERO; tYPos > 0; tYPos -= HORIZONTAL_GRID_HEIGHT) {
        if (tLabelChanged) {
            // clear old label
            int tXpos = DSO_DISPLAY_WIDTH - PIXEL_AFTER_LABEL - (5 * TEXT_SIZE_11_WIDTH);
            int tY = tYPos - tCaptionOffset;
            BlueDisplay1.fillRect(tXpos, tY - TEXT_SIZE_11_ASCEND, DSO_DISPLAY_WIDTH - PIXEL_AFTER_LABEL + 1,
                    tY + TEXT_SIZE_11_HEIGHT - TEXT_SIZE_11_ASCEND, COLOR_BACKGROUND_DSO);
            // restore vertical line
            BlueDisplay1.drawLineRel(9 * TIMING_GRID_WIDTH - 1, tY, 0, TEXT_SIZE_11_HEIGHT, aColor);
        }
        // draw horizontal line
        BlueDisplay1.drawLineRel(0, tYPos, DSO_DISPLAY_WIDTH, 0, aColor);

        int tCount = snprintf(StringBuffer, sizeof StringBuffer, "%0.*f", RangePrecision[MeasurementControl.DisplayRangeIndex],
                tActualVoltage);
        // right align but leave 2 pixel free after label for the last horizontal line
        tPosX = DSO_DISPLAY_WIDTH - (tCount * TEXT_SIZE_11_WIDTH) - PIXEL_AFTER_LABEL;
        // draw label over the line - use different color for negative values
        if (tActualVoltage >= 0) {
            tLabelColor = COLOR_HOR_REF_LINE_LABEL;
        } else {
            tLabelColor = COLOR_HOR_REF_LINE_LABEL_NEGATIVE;
        }
        BlueDisplay1.drawText(tPosX, tYPos - tCaptionOffset, StringBuffer, TEXT_SIZE_11, tLabelColor, COLOR_BACKGROUND_DSO);
        tCaptionOffset = -(TEXT_SIZE_11_ASCEND / 2);
        tActualVoltage += ScaleVoltagePerDiv[MeasurementControl.DisplayRangeIndex];
    }
    drawTriggerLine();
}

/*
 * draws min, max lines
 */
void drawMinMaxLines(void) {
// draw max line
    int tValueDisplay = getDisplayFrowRawInputValue(MeasurementControl.RawValueMax);
    if (tValueDisplay != 0) {
        BlueDisplay1.drawLineRel(0, tValueDisplay, DSO_DISPLAY_WIDTH, 0, COLOR_MAX_MIN_LINE);
    }
// min line
    tValueDisplay = getDisplayFrowRawInputValue(MeasurementControl.RawValueMin);
    if (tValueDisplay != DISPLAY_VALUE_FOR_ZERO) {
        BlueDisplay1.drawLineRel(0, tValueDisplay, DSO_DISPLAY_WIDTH, 0, COLOR_MAX_MIN_LINE);
    }
}
/**
 * Draws data on screen
 * @param aDataBufferPointer Data is taken from DataBufferPointer.
 *             If DataBufferPointer is NULL, then Data is taken from DisplayBuffer - used for erasing display before switching mode
 * @param aLength
 * @param aColor Draw color. - used for different colors for pretrigger data or different modes
 * @param aClearBeforeColor if > 0 data from DisplayBuffer is drawn(erased) with this color
 *              just before to avoid interfering with display refresh timing. - Color is used for history modes.
 *              DataBufferPointer must not be null then!
 * @note if aClearBeforeColor >0 then DataBufferPointer must not be NULL
 * @note NOT used for drawing while acquiring
 */
void drawDataBuffer(uint16_t *aDataBufferPointer, int aLength, uint16_t aColor, uint16_t aClearBeforeColor) {
    int i;
    int j = 0;
    int tValue;

    int tLastValue;
    int tLastValueClear = DisplayBuffer[0];

    uint8_t *ScreenBufferReadPointer = &DisplayBuffer[0];
    uint8_t *ScreenBufferWritePointer1 = &DisplayBuffer[0];
    uint8_t *ScreenBufferWritePointer2 = &DisplayBuffer2[0]; // for trigger state line
    int tXScale = DisplayControl.XScale;
    int tXScaleCounter = tXScale;
    int tTriggerValue = getDisplayFrowRawInputValue(MeasurementControl.RawTriggerLevel);
    if (tXScale <= 0) {
        tXScaleCounter = -tXScale;
    }

    for (i = 0; i < aLength; ++i) {
        if (aDataBufferPointer == NULL) {
            // get data from screen buffer in order to erase it
            tValue = *ScreenBufferReadPointer++;
        } else {
            tValue = getDisplayFrowRawInputValue(*aDataBufferPointer);
            /*
             * get data from data buffer and perform X scaling
             */
            if (tXScale == 0) {
                aDataBufferPointer++;
            } else if (tXScale < -1) {
                // compress - get average of multiple values
                tValue = getDisplayFrowMultipleRawValues(aDataBufferPointer, tXScaleCounter);
                aDataBufferPointer += tXScaleCounter;
            } else if (tXScale == -1) {
                // compress by factor 1.5 - every second value is the average of the next two values
                aDataBufferPointer++;
                tXScaleCounter--;
                if (tXScaleCounter < 0) {
                    if (tValue != DISPLAYBUFFER_INVISIBLE_VALUE) {
                        // get average of actual and next value
                        tValue += getDisplayFrowRawInputValue(*aDataBufferPointer++);
                        tValue /= 2;
                    }
                    tXScaleCounter = 1;
                }
            } else if (tXScale == 1) {
                aDataBufferPointer++;
                // expand by factor 1.5 - every second value will be shown 2 times
                tXScaleCounter--; // starts with 1
                if (tXScaleCounter < 0) {
                    aDataBufferPointer--;
                    tXScaleCounter = 2;
                }
            } else {
                // expand - show value several times
                if (tXScaleCounter == 0) {
                    aDataBufferPointer++;
                    tXScaleCounter = tXScale;
                }
                tXScaleCounter--;
            }
        }

        // draw trigger state line (aka Digital mode)
        if (DisplayControl.DisplayBufferDrawMode & DRAW_MODE_TRIGGER) {
#ifdef LOCAL_DISPLAY_EXISTS
            if (aClearBeforeColor > 0) {
                LocalDisplay.drawPixel(i, *ScreenBufferWritePointer2, aClearBeforeColor);
            }
#endif
            if (tValue > tTriggerValue) {
#ifdef LOCAL_DISPLAY_EXISTS
                LocalDisplay.drawPixel(i, tTriggerValue - TRIGGER_HIGH_DISPLAY_OFFSET, COLOR_DATA_TRIGGER);
#endif
                *ScreenBufferWritePointer2++ = tTriggerValue - TRIGGER_HIGH_DISPLAY_OFFSET;
            }
        }

        if (!(DisplayControl.DisplayBufferDrawMode & DRAW_MODE_LINE) || i == 0) {
            /*
             * Pixel Mode or first value of chart
             */
            if (aDataBufferPointer == NULL && j == TIMING_GRID_WIDTH) {
                // Restore grid pixel instead of clearing it
                j = 0;
#ifdef LOCAL_DISPLAY_EXISTS
                if (tValue != DISPLAYBUFFER_INVISIBLE_VALUE) {
                    LocalDisplay.drawPixel(i, tValue, COLOR_GRID_LINES);
                }
#endif
            } else {
                j++;
                if (aClearBeforeColor > 0) {
                    int tValueClear = *ScreenBufferReadPointer++;
#ifdef LOCAL_DISPLAY_EXISTS
                    if (tValueClear != DISPLAYBUFFER_INVISIBLE_VALUE) {
                        LocalDisplay.drawPixel(i, tValueClear, aClearBeforeColor);
                    }
#endif
                    if (DisplayControl.DisplayBufferDrawMode & DRAW_MODE_LINE) {
                        // erase first line in advance and set pointer
                        tLastValueClear = tValueClear;
                        tValueClear = *ScreenBufferReadPointer++;
#ifdef LOCAL_DISPLAY_EXISTS
                        if (tValueClear != DISPLAYBUFFER_INVISIBLE_VALUE) {
                            if (tLastValueClear != DISPLAYBUFFER_INVISIBLE_VALUE) {
                                // Normal mode - clear line
                                LocalDisplay.drawLineFastOneX(i, tLastValueClear, tValueClear, aClearBeforeColor);
                            } else {
                                // first visible value just clear start pixel
                                LocalDisplay.drawPixel(i, tValueClear, aClearBeforeColor);
                            }
                        }
#endif
                        tLastValueClear = tValueClear;
                    }
                }
#ifdef LOCAL_DISPLAY_EXISTS
                if (tValue != DISPLAYBUFFER_INVISIBLE_VALUE) {
                    LocalDisplay.drawPixel(i, tValue, aColor);
                }
#endif
            }
        } else {
            /*
             * Line mode here
             */
            if (aClearBeforeColor > 0 && i != DSO_DISPLAY_WIDTH - 1) {
                // erase one x value in advance in order not to overwrite the x+1 part of line just drawn before
                int tValueClear = *ScreenBufferReadPointer++;
#ifdef LOCAL_DISPLAY_EXISTS
                if (tLastValueClear != DISPLAYBUFFER_INVISIBLE_VALUE && tValueClear != DISPLAYBUFFER_INVISIBLE_VALUE) {
                    LocalDisplay.drawLineFastOneX(i, tLastValueClear, tValueClear, aClearBeforeColor);
                }
#endif
                tLastValueClear = tValueClear;
            }
#ifdef LOCAL_DISPLAY_EXISTS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
            // is initialized on i == 0
            if (tValue != DISPLAYBUFFER_INVISIBLE_VALUE) {
                if (tLastValue != DISPLAYBUFFER_INVISIBLE_VALUE) {
                    // Normal mode - draw line
                    if (tLastValue == tValue && (tValue == DISPLAY_VALUE_FOR_ZERO || tValue == 0)) {
                        // clipping occurs draw red line
                        LocalDisplay.drawLineFastOneX(i - 1, tLastValue, tValue, COLOR_DATA_RUN_CLIPPING);
                    } else {
                        LocalDisplay.drawLineFastOneX(i - 1, tLastValue, tValue, aColor);
                    }
                } else {
                    // first visible value just draw start pixel
                    LocalDisplay.drawPixel(i, tValue, aColor);
                }
            }
#pragma GCC diagnostic pop
#endif
        }
        tLastValue = tValue;
        // store data in screen buffer
        *ScreenBufferWritePointer1++ = tValue;
    }
    // draw on external screen
    if (USART_isBluetoothPaired()) {
        sendUSART5ArgsAndByteBuffer(FUNCTION_TAG_DRAW_CHART, 0, 0, aColor, aClearBeforeColor, 0, &DisplayBuffer[0], aLength);
    }
}

/**
 * Draws all chart values till DataBufferNextInPointer is reached - used for drawing while acquiring
 * @param aDrawColor
 */
void drawRemainingDataBufferValues(uint16_t aDrawColor) {
    // needed for last acquisition which uses the whole data buffer
    while (DataBufferControl.DataBufferNextDrawPointer < DataBufferControl.DataBufferNextInPointer
            && DataBufferControl.DataBufferNextDrawPointer <= &DataBufferControl.DataBuffer[DATABUFFER_DISPLAY_END]
            && !MeasurementControl.TriggerPhaseJustEnded && !DataBufferControl.DataBufferPreTriggerAreaWrapAround) {

        int tDisplayX = DataBufferControl.NextDrawXValue;
        if (DataBufferControl.DataBufferNextDrawPointer
                == &DataBufferControl.DataBuffer[DATABUFFER_PRE_TRIGGER_SIZE + FFT_SIZE - 1]) {
            // now data buffer is filled with more than 256 samples -> show fft
            draw128FFTValuesFast(COLOR_FFT_DATA, &DataBufferControl.DataBuffer[DATABUFFER_DISPLAY_START]);
        }

        // wrap around in display buffer
        if (tDisplayX >= DSO_DISPLAY_WIDTH) {
            tDisplayX = 0;
        }
        DataBufferControl.NextDrawXValue = tDisplayX + 1;

        uint8_t * tDisplayBufferPointer = &DisplayBuffer[tDisplayX];
        // unsigned is faster
        unsigned int tValue = *tDisplayBufferPointer;
        int tLastValue;
        int tNextValue;

        /*
         * clear old pixel / line
         */
        if (!(DisplayControl.DisplayBufferDrawMode & DRAW_MODE_LINE)) {
            // new values in data buffer => draw one pixel
            // clear pixel or restore grid
            int tColor = DisplayControl.EraseColor;
            if (tDisplayX % TIMING_GRID_WIDTH == TIMING_GRID_WIDTH - 1) {
                tColor = COLOR_GRID_LINES;
            }
            if (tValue != DISPLAYBUFFER_INVISIBLE_VALUE) {
                BlueDisplay1.drawPixel(tDisplayX, tValue, tColor);
            }
        } else {
            if (tDisplayX < DSO_DISPLAY_WIDTH - 1) {
                // fetch next value and clear line in advance
                tNextValue = DisplayBuffer[tDisplayX + 1];
                if (tNextValue != DISPLAYBUFFER_INVISIBLE_VALUE) {
                    if (tValue != DISPLAYBUFFER_INVISIBLE_VALUE) {
                        // normal mode
                        BlueDisplay1.drawLineFastOneX(tDisplayX, tValue, tNextValue, DisplayControl.EraseColor);
                    } else {
                        // first visible value, draw only start pixel
                        BlueDisplay1.drawPixel(tDisplayX, tNextValue, DisplayControl.EraseColor);
                    }
                }
            }
        }

        /*
         * get new value
         */
        tValue = getDisplayFrowRawInputValue(*DataBufferControl.DataBufferNextDrawPointer);
        *tDisplayBufferPointer = tValue;

        if (!(DisplayControl.DisplayBufferDrawMode & DRAW_MODE_LINE)) {
            if (tValue != DISPLAYBUFFER_INVISIBLE_VALUE) {
                //draw new pixel
                BlueDisplay1.drawPixel(tDisplayX, tValue, aDrawColor);
            }
        } else {
            if (tDisplayX != 0 && tDisplayX <= DSO_DISPLAY_WIDTH - 1) {
                // get lastValue and draw line
                if (tValue != DISPLAYBUFFER_INVISIBLE_VALUE) {
                    tLastValue = DisplayBuffer[tDisplayX - 1];
                    if (tLastValue != DISPLAYBUFFER_INVISIBLE_VALUE) {
                        // normal mode
                        BlueDisplay1.drawLineFastOneX(tDisplayX - 1, tLastValue, tValue, aDrawColor);
                    } else {
                        // first visible value, draw only start pixel
                        BlueDisplay1.drawPixel(tDisplayX, tValue, aDrawColor);
                    }
                }
            }
        }
        DataBufferControl.DataBufferNextDrawPointer++;
    }
}

void clearDiplayedChart(void) {
    BlueDisplay1.drawChartByteBuffer(0, 0, COLOR_BACKGROUND_DSO, COLOR_NO_BACKGROUND, &DisplayBuffer[0], sizeof(DisplayBuffer));
}

void drawFFT(void) {
    // compute and draw FFT
    BlueDisplay1.clearDisplay(COLOR_BACKGROUND_DSO);
    computeFFT(DataBufferControl.DataBufferDisplayStart, (float32_t*) (FourDisplayLinesBuffer)); // enough space for 640 floats);
    // init and draw chart 12 milliseconds with -O0
    // display with Xscale = 2
    ChartFFT.initChart(4 * TEXT_SIZE_11_WIDTH, DSO_DISPLAY_HEIGHT - 2 * TEXT_SIZE_11_HEIGHT, FFT_SIZE, 32 * 5, 2, true, 64, 32);
    ChartFFT.initChartColors(COLOR_FFT_DATA, COLOR_RED, RGB(0xC0,0xC0,0xC0), COLOR_RED, COLOR_BACKGROUND_DSO);
    // compute Label for x Frequency axis
    char tFreqUnitString[4] = { };
    float tTimebaseExactValue = getTimebaseExactValueMicros(MeasurementControl.TimebaseIndex);
    // compute frequency for 32.th bin (1/4 of nyquist frequency at 256 samples)
    int tFreqAtBin32 = 4000 / tTimebaseExactValue;
    // draw x axis
    if (tFreqAtBin32 >= 1000) {
        tFreqAtBin32 /= 1000;
        tFreqUnitString[0] = 'k'; // kHz
    }
    ChartFFT.initXLabelInt(0, tFreqAtBin32 * FFT_DISPLAY_SCALE_FACTOR_X, FFT_DISPLAY_SCALE_FACTOR_X, 5);
    ChartFFT.setXTitleText(tFreqUnitString);
    // display 1.0 for input value of tMaxValue
    ChartFFT.initYLabelFloat(0, 0.2, 1.0 / FFTInfo.MaxValue, 3, 1);
    ChartFFT.drawAxesAndGrid();
    ChartFFT.drawXAxisTitle();
    // show chart
    ChartFFT.drawChartDataFloat((float *) FourDisplayLinesBuffer, (float *) &FourDisplayLinesBuffer[2 * FFT_SIZE], CHART_MODE_AREA);
    // print max bin frequency information
    // compute frequency of max bin
    tFreqUnitString[0] = ' '; // Hz
    float tFreqAtMaxBin = FFTInfo.MaxIndex * 125000 / tTimebaseExactValue;
    if (tFreqAtMaxBin >= 10000) {
        tFreqAtMaxBin /= 1000;
        tFreqUnitString[0] = 'k'; // Hz
    }
    snprintf(StringBuffer, sizeof(StringBuffer), "%0.2f%s", tFreqAtMaxBin, tFreqUnitString);
    BlueDisplay1.drawText(140, 4 * TEXT_SIZE_11_HEIGHT + TEXT_SIZE_22_ASCEND, StringBuffer, TEXT_SIZE_22, COLOR_RED,
            COLOR_BACKGROUND_DSO);
    tFreqUnitString[0] = ' '; // Hz
    float tFreqDeltaHalf = 62.5 / tTimebaseExactValue; // = tFreqAtBin32 / 64;
    snprintf(StringBuffer, sizeof(StringBuffer), "[\xF1%0.1f%s]", tFreqDeltaHalf, tFreqUnitString);
    BlueDisplay1.drawText(140, 6 * TEXT_SIZE_11_HEIGHT + TEXT_SIZE_11_ASCEND, StringBuffer, TEXT_SIZE_11, COLOR_RED,
            COLOR_BACKGROUND_DSO);
}

/**
 * Draws area chart for fft values. 3 pixel for each value (for a 320 pixel screen)
 * Data is scaled to max = 32 pixel high and drawn at bottom of screen
 */
void draw128FFTValuesFast(uint16_t aColor, uint16_t * aDataBufferPointer) {
    if (DisplayControl.ShowFFT) {
        float *tFFTDataPointer = (float32_t *) FourDisplayLinesBuffer; // enough space for 640 floats
        computeFFT(DataBufferControl.DataBufferDisplayStart, tFFTDataPointer);

        uint8_t *tDisplayBufferPtr = &DisplayBufferFFT[0];
        float tInputValue;
        uint16_t tDisplayY, DisplayYOld;

        MeasurementControl.MaxFFTValue = FFTInfo.MaxValue;
        // compute frequency of max bin
        MeasurementControl.FrequencyHertzAtMaxFFTBin = FFTInfo.MaxIndex * 125000
                / getTimebaseExactValueMicros(MeasurementControl.TimebaseIndex);

        //compute scale factor
        float tYDisplayScaleFactor = HORIZONTAL_GRID_HEIGHT / FFTInfo.MaxValue;
        for (int tDisplayX = 0; tDisplayX < DSO_DISPLAY_WIDTH; tDisplayX += 3) {
            /*
             *  get data and perform scaling
             */
            tInputValue = *tFFTDataPointer++;
            tDisplayY = tYDisplayScaleFactor * tInputValue;
            // tDisplayY now from 0 to 32
            tDisplayY = DSO_DISPLAY_HEIGHT - tDisplayY;
            DisplayYOld = *tDisplayBufferPtr;
            if (tDisplayY < DisplayYOld) {
                // increase old bar
                BlueDisplay1.fillRect(tDisplayX, tDisplayY, tDisplayX + 2, DisplayYOld, aColor);
            } else if (tDisplayY > DisplayYOld) {
                // Remove part of old bar
                BlueDisplay1.fillRect(tDisplayX, DisplayYOld, tDisplayX + 2, tDisplayY, COLOR_BACKGROUND_DSO);
            }
            *tDisplayBufferPtr++ = tDisplayY;
        }
    }
}

void clearFFTValuesOnDisplay(void) {
    uint8_t *tDisplayBufferPtr = &DisplayBufferFFT[0];
    uint16_t DisplayYOld;
    for (int tDisplayX = 0; tDisplayX < DSO_DISPLAY_WIDTH; tDisplayX += 3) {
        DisplayYOld = *tDisplayBufferPtr;
        // Remove old bar
        BlueDisplay1.fillRect(tDisplayX, DisplayYOld, tDisplayX + 2, DSO_DISPLAY_HEIGHT - 1, COLOR_BACKGROUND_DSO);
    }
}

/************************************************************************
 * Text output section
 ************************************************************************/

void clearInfo(void) {
    BlueDisplay1.fillRectRel(INFO_LEFT_MARGIN, 0, DSO_DISPLAY_WIDTH,
            3 * TEXT_SIZE_11_HEIGHT + (INFO_UPPER_MARGIN - TEXT_SIZE_11_ASCEND), COLOR_BACKGROUND_DSO);
}

/*
 * Output info line
 */
void printInfo(void) {
    if (DisplayControl.DisplayPage != CHART || DisplayControl.showInfoMode != LONG_INFO) {
        return;
    }

    char tSlopeChar;
    char tTriggerAutoChar;
    char tTimebaseUnitChar;

    switch (MeasurementControl.TriggerMode) {
    case TRIGGER_MODE_AUTOMATIC:
        tTriggerAutoChar = 'A';
        break;
    case TRIGGER_MODE_MANUAL:
        tTriggerAutoChar = 'M';
        break;
    default:
        tTriggerAutoChar = 'O';
        break;
    }

    if (MeasurementControl.TriggerSlopeRising) {
        tSlopeChar = 0xD1; //ascending
    } else {
        tSlopeChar = 0xD2; //Descending
    }

// compute value here, because min and max can have changed by completing another measurement,
// while printing first line to screen
    int tValueDiff = MeasurementControl.RawValueMax - MeasurementControl.RawValueMin;
// compensate for aValue -= RawDSOReadingACZero; in getFloatFromRawValue()
    if (MeasurementControl.isACMode) {
        tValueDiff += MeasurementControl.RawDSOReadingACZero;
    }
    float tMicrosPeriod = MeasurementControl.PeriodMicros;
    char tPeriodUnitChar = 0xB5; // micro
    if (tMicrosPeriod >= 50000) {
        tMicrosPeriod = tMicrosPeriod / 1000;
        tPeriodUnitChar = 'm'; // milli
    }
    int tUnitsPerGrid;
    if (MeasurementControl.TimebaseIndex >= TIMEBASE_INDEX_MILLIS) {
        tTimebaseUnitChar = 'm';
    } else if (MeasurementControl.TimebaseIndex >= TIMEBASE_INDEX_MICROS) {
        tTimebaseUnitChar = 0xB5; // micro
    } else {
        tTimebaseUnitChar = 'n'; // nano
    }
    tUnitsPerGrid = TimebaseDivValues[MeasurementControl.TimebaseIndex];
// number of digits to be printed after the decimal point
    int tPrecision = 3;
    if ((MeasurementControl.isACMode && MeasurementControl.InputRangeIndex >= 8) || MeasurementControl.InputRangeIndex >= 10) {
        tPrecision = 2;
    }
    if (MeasurementControl.isACMode && MeasurementControl.InputRangeIndex >= 11) {
        tPrecision = 1;
    }

// render period + frequency
    char tBufferForPeriodAndFrequency[20];

    int tPeriodStringLength = 7;
    int tFreqStringSize = 6;
    int tPeriodStringPrecision = 1;

    if (tMicrosPeriod < 100) {
        // move period value 1 character left and increase precision
        tPeriodStringLength = 6;
        tPeriodStringPrecision = 2;
        tFreqStringSize = 7;
    }
    if (tMicrosPeriod > 10000) {
        // move period value 1 character right and decrease precision
        tPeriodStringLength = 8;
        tPeriodStringPrecision = 0;
        tFreqStringSize = 5;
    }

    snprintf(tBufferForPeriodAndFrequency, sizeof tBufferForPeriodAndFrequency, "%*.*f%cs  %*luHz", tPeriodStringLength,
            tPeriodStringPrecision, tMicrosPeriod, tPeriodUnitChar, tFreqStringSize, MeasurementControl.FrequencyHertz);
// set separator for thousands
    char * tBufferDestPtr = &tBufferForPeriodAndFrequency[0];
    char * tBufferSrcPtr = tBufferDestPtr + 1;
    if (tMicrosPeriod > 10000) {
        *tBufferDestPtr++ = *tBufferSrcPtr++;
        *tBufferDestPtr++ = *tBufferSrcPtr++;
        *tBufferDestPtr++ = *tBufferSrcPtr++;
    }
    if (tMicrosPeriod >= 1000) {
        *tBufferDestPtr++ = *tBufferSrcPtr++;
        *tBufferDestPtr = THOUSANDS_SEPARATOR;
    }

    if (MeasurementControl.FrequencyHertz >= 1000) {
        // set separator for thousands
        formatThousandSeparator(&tBufferForPeriodAndFrequency[9], &tBufferForPeriodAndFrequency[13]);
    }

    if (MeasurementControl.InfoSizeSmall) {
        /**
         * small mode
         */
        char tChannelChar = ADCInputMUXChannelChars[MeasurementControl.ADCInputMUXChannelIndex];
#ifdef LOCAL_DISPLAY_EXISTS
        if (MeasurementControl.ADS7846ChannelsAsDatasource) {
            tChannelChar = ADS7846ChannelChars[MeasurementControl.ADCInputMUXChannelIndex];
        }
#endif
        // first line
        snprintf(StringBuffer, sizeof StringBuffer, "%6.*f %6.*f %6.*f %6.*fV%2u %4u%cs %c", tPrecision,
                getFloatFromRawValue(MeasurementControl.RawValueAverage), tPrecision,
                getFloatFromRawValue(MeasurementControl.RawValueMin), tPrecision,
                getFloatFromRawValue(MeasurementControl.RawValueMax), tPrecision, getFloatFromRawValue(tValueDiff),
                DisplayControl.XScale, tUnitsPerGrid, tTimebaseUnitChar, tChannelChar);
        BlueDisplay1.drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN, StringBuffer, TEXT_SIZE_11, COLOR_BLACK, COLOR_INFO_BACKGROUND);

        // second line first part
        BlueDisplay1.drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + TEXT_SIZE_11_HEIGHT, tBufferForPeriodAndFrequency, TEXT_SIZE_11,
                COLOR_BLACK, COLOR_INFO_BACKGROUND);

        // second line second part
        snprintf(StringBuffer, sizeof StringBuffer, "%c%c %5.*fV", tSlopeChar, tTriggerAutoChar, tPrecision - 1,
                getFloatFromRawValue(MeasurementControl.RawTriggerLevel));
        BlueDisplay1.drawText(TRIGGER_LEVEL_INFO_SMALL_X - (3 * TEXT_SIZE_11_WIDTH), INFO_UPPER_MARGIN + TEXT_SIZE_11_HEIGHT,
                StringBuffer, TEXT_SIZE_11, COLOR_BLACK, COLOR_INFO_BACKGROUND);

    } else {
        /**
         * Large mode
         */
        if (MeasurementControl.isSingleShotMode) {
            // current value
            snprintf(StringBuffer, sizeof StringBuffer, "Current=%4.3fV waiting for %s",
                    getFloatFromRawValue(MeasurementControl.RawValueBeforeTrigger),
                    TriggerStatusStrings[MeasurementControl.TriggerStatus]);
        } else {

            // First line
            // Min + Average + Max + Peak-to-peak
            snprintf(StringBuffer, sizeof StringBuffer, "Av%6.*fV Min%6.*f Max%6.*f P2P%6.*f", tPrecision,
                    getFloatFromRawValue(MeasurementControl.RawValueAverage), tPrecision,
                    getFloatFromRawValue(MeasurementControl.RawValueMin), tPrecision,
                    getFloatFromRawValue(MeasurementControl.RawValueMax), tPrecision, getFloatFromRawValue(tValueDiff));
        }
        BlueDisplay1.drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN, StringBuffer, TEXT_SIZE_11, COLOR_BLACK, COLOR_INFO_BACKGROUND);

        const char * tChannelString = ADCInputMUXChannelStrings[MeasurementControl.ADCInputMUXChannelIndex];
#ifdef LOCAL_DISPLAY_EXISTS
        if (MeasurementControl.ADS7846ChannelsAsDatasource) {
            tChannelString = ADS7846ChannelStrings[MeasurementControl.ADCInputMUXChannelIndex];
        }
#endif
        // Second line
        // XScale + Timebase + MicrosPerPeriod + Hertz + Channel
        snprintf(StringBuffer, sizeof StringBuffer, "%2d %4u%cs %s %s", DisplayControl.XScale, tUnitsPerGrid, tTimebaseUnitChar,
                tBufferForPeriodAndFrequency, tChannelString);
        BlueDisplay1.drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + TEXT_SIZE_11_HEIGHT, StringBuffer, TEXT_SIZE_11, COLOR_BLACK,
                COLOR_INFO_BACKGROUND);

        // Third line
        // Trigger: Slope + Mode + Level + FFT max frequency - Empty space after string is needed for voltage picker value
        if (DisplayControl.ShowFFT) {
            snprintf(tBufferForPeriodAndFrequency, sizeof tBufferForPeriodAndFrequency, " %6.0fHz %4.1f",
                    MeasurementControl.FrequencyHertzAtMaxFFTBin, MeasurementControl.MaxFFTValue);
            if (MeasurementControl.FrequencyHertzAtMaxFFTBin >= 1000) {
                formatThousandSeparator(&tBufferForPeriodAndFrequency[0], &tBufferForPeriodAndFrequency[3]);
            }
        } else {
            memset(tBufferForPeriodAndFrequency, ' ', 9);
            tBufferForPeriodAndFrequency[9] = '\0';
        }
        snprintf(StringBuffer, sizeof StringBuffer, "Trigg: %c %c %5.*fV %s", tSlopeChar, tTriggerAutoChar, tPrecision - 1,
                getFloatFromRawValue(MeasurementControl.RawTriggerLevel), tBufferForPeriodAndFrequency);
        BlueDisplay1.drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + (2 * TEXT_SIZE_11_HEIGHT), StringBuffer, TEXT_SIZE_11,
                COLOR_BLACK, COLOR_INFO_BACKGROUND);

        // Debug infos
//		char tTriggerTimeoutChar = 0x20; // space
//		if (MeasurementControl.TriggerStatus < 2) {
//			tTriggerTimeoutChar = 0x21;
//		}
//		snprintf(StringBuffer, sizeof StringBuffer, "%c TO=%6d DB=%x S=%x E=%x", tTriggerTimeoutChar,
//				MeasurementControl.TriggerTimeoutSampleOrLoopCount,
//				(uint16_t) ((uint32_t) &DataBufferControl.DataBuffer[0]) & 0xFFFF,
//				(uint16_t) ((uint32_t) DataBufferControl.DataBufferDisplayStart) & 0xFFFF,
//				(uint16_t) ((uint32_t) DataBufferControl.DataBufferEndPointer) & 0xFFFF);
        // Print RAW min max average
//		snprintf(StringBuffer, sizeof StringBuffer, "Min%#X Average%#X Max%#X", MeasurementControl.ValueMin,
//				MeasurementControl.ValueAverage, MeasurementControl.ValueMax);
//
//		BlueDisplay1.drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + (3 * TEXT_SIZE_11_HEIGHT), StringBuffer, 1, COLOR_BLUE, COLOR_INFO_BACKGROUND);
        // Debug end
    }
}

/**
 * prints only trigger value
 */
void printTriggerInfo(void) {
    if (DisplayControl.showInfoMode == LONG_INFO) {
        int tPrecision = 2;
        if ((MeasurementControl.isACMode && MeasurementControl.InputRangeIndex >= 8) || MeasurementControl.InputRangeIndex >= 10) {
            tPrecision = 1;
        }
        if (MeasurementControl.isACMode && MeasurementControl.InputRangeIndex >= 11) {
            tPrecision = 0;
        }
        snprintf(StringBuffer, sizeof StringBuffer, "%5.*fV", tPrecision, getFloatFromRawValue(MeasurementControl.RawTriggerLevel));
        if (MeasurementControl.InfoSizeSmall) {
            BlueDisplay1.drawText(TRIGGER_LEVEL_INFO_SMALL_X, INFO_UPPER_MARGIN + TEXT_SIZE_11_HEIGHT, StringBuffer, TEXT_SIZE_11,
                    COLOR_BLACK, COLOR_INFO_BACKGROUND);
        } else {
            BlueDisplay1.drawText(TRIGGER_LEVEL_INFO_LARGE_X, INFO_UPPER_MARGIN + 2 * TEXT_SIZE_11_HEIGHT, StringBuffer,
                    TEXT_SIZE_11, COLOR_BLACK, COLOR_INFO_BACKGROUND);
        }
    }
}

/*******************************
 * RAW to display value section
 *******************************/

/**
 * @param aAdcValue raw ADC value
 * @return Display value (0 to 240-DISPLAY_VALUE_FOR_ZERO) or 0 if raw value to high
 */
int getDisplayFrowRawInputValue(int aAdcValue) {
    if (aAdcValue == DATABUFFER_INVISIBLE_RAW_VALUE) {
        return DISPLAYBUFFER_INVISIBLE_VALUE;
    }
// first: convert raw to signed values if ac range is selected
    if (MeasurementControl.isACMode) {
        aAdcValue -= MeasurementControl.RawDSOReadingACZero;
    }

// second: convert raw signed value from input range to display range
    if (MeasurementControl.InputRangeIndexOtherThanDisplayRange) {
        aAdcValue = aAdcValue * FactorFromInputToDisplayRangeShift12;
        aAdcValue >>= DSO_INPUT_TO_DISPLAY_SHIFT;
    }

// third: adjust with display range offset
    aAdcValue = aAdcValue - MeasurementControl.RawOffsetValueForDisplayRange;
    if (aAdcValue < 0) {
        return DISPLAY_VALUE_FOR_ZERO;
    }

// fourth: convert raw to display value
    aAdcValue *= ScaleFactorRawToDisplayShift18[MeasurementControl.DisplayRangeIndex];
    aAdcValue >>= DSO_SCALE_FACTOR_SHIFT;

//fifth: invert and clip value
    if (aAdcValue > DISPLAY_VALUE_FOR_ZERO) {
        aAdcValue = 0;
    } else {
        aAdcValue = (DISPLAY_VALUE_FOR_ZERO) - aAdcValue;
    }
    return aAdcValue;
}

/**
 *
 * @param aAdcValuePtr Data pointer
 * @param aCount number of samples for oversampling
 * @return average of count values from data pointer
 */
int getDisplayFrowMultipleRawValues(uint16_t * aAdcValuePtr, int aCount) {
//
    int tAdcValue = 0;
    for (int i = 0; i < aCount; ++i) {
        tAdcValue += *aAdcValuePtr++;
    }
    return getDisplayFrowRawInputValue(tAdcValue / aCount);
}

int getRawOffsetValueFromGridCount(int aCount) {
    aCount = (aCount * HORIZONTAL_GRID_HEIGHT) << DSO_SCALE_FACTOR_SHIFT;
    aCount = aCount / ScaleFactorRawToDisplayShift18[MeasurementControl.DisplayRangeIndex];
    return aCount;
}

/*
 * get raw value for display value - assume InputRangeIndex
 * aValue raw display value 0 is top
 */
int getInputRawFromDisplayValue(int aValue) {
// invert value
    aValue = (DISPLAY_VALUE_FOR_ZERO) - aValue;

// convert to raw
    aValue = aValue << DSO_SCALE_FACTOR_SHIFT;
    aValue /= ScaleFactorRawToDisplayShift18[MeasurementControl.DisplayRangeIndex];

//adjust with offset
    aValue += MeasurementControl.RawOffsetValueForDisplayRange;

// convert from raw to input range
    if (MeasurementControl.InputRangeIndexOtherThanDisplayRange) {
        aValue = (aValue << DSO_INPUT_TO_DISPLAY_SHIFT) / FactorFromInputToDisplayRangeShift12;
    }

// adjust for zero offset
    if (MeasurementControl.isACMode) {
        aValue += MeasurementControl.RawDSOReadingACZero;
    }
    return aValue;
}

/*
 * computes corresponding voltage from raw value
 */
float getFloatFromRawValue(int aValue) {
    if (MeasurementControl.isACMode) {
        aValue -= MeasurementControl.RawDSOReadingACZero;
    }
    return (actualDSORawToVoltFactor * aValue);
}

/*
 * computes corresponding voltage from display y position
 */
float getFloatFromDisplayValue(uint8_t aDisplayValue) {
    int tRawValue = getInputRawFromDisplayValue(aDisplayValue);
    return getFloatFromRawValue(tRawValue);
}

/*
 * for internal test of the conversion routines
 */
void testDSOConversions(void) {
// Prerequisites
    MeasurementControl.InputRangeIndexOtherThanDisplayRange = false;
    MeasurementControl.isACMode = false;
    MeasurementControl.InputRangeIndex = 6;    // 1 Volt / div  Raw-1360 / div
    MeasurementControl.DisplayRangeIndex = 3;    // 0,1 Volt / div | Raw-136 / div | 827 max
    MeasurementControl.RawOffsetValueForDisplayRange = 100;
    autoACZeroCalibration();
    initRawToDisplayFactors();
    FactorFromInputToDisplayRangeShift12 = (RawAttenuationFactor[MeasurementControl.InputRangeIndex]
            * (1 << DSO_INPUT_TO_DISPLAY_SHIFT)) / RawAttenuationFactor[MeasurementControl.DisplayRangeIndex];

// Tests of raw <-> display conversion routines
    int tValue;
    tValue = getRawOffsetValueFromGridCount(3);    // appr. 416

    tValue = getDisplayFrowRawInputValue(400);
    tValue = getInputRawFromDisplayValue(tValue);

    MeasurementControl.InputRangeIndexOtherThanDisplayRange = true;
    MeasurementControl.isACMode = true;

    tValue = getDisplayFrowRawInputValue(2200);    // since it is AC range
    tValue = getInputRawFromDisplayValue(tValue);

    float tFloatValue;
    tFloatValue = getFloatFromRawValue(400 + MeasurementControl.RawDSOReadingACZero);
    tFloatValue = getFloatFromDisplayValue(getDisplayFrowRawInputValue(400));
    tFloatValue = ADCToVoltFactor * 400;
    tFloatValue = tFloatValue / ADCToVoltFactor;
// to see above result in debugger
    tFloatValue = tFloatValue * 2;
}
