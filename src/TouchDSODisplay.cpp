/**
 * TouchDSODisplay.cpp
 * @brief contains the display related functions for DSO.
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
#include <string.h>

/*
 * Layout
 */
#define TRIGGER_LEVEL_INFO_LARGE_X INFO_LEFT_MARGIN + (11 * FONT_WIDTH)
#define TRIGGER_LEVEL_INFO_SMALL_X (INFO_LEFT_MARGIN + (33 * FONT_WIDTH))
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

/*******************************************************************************************
 * Program code starts here
 *******************************************************************************************/
void initRawToDisplayFactors(void) {
// used for getFloatFromRawValue() and for changeInputRange()
    actualDSORawToVoltFactor = (ADCToVoltFactor * getRawAttenuationFactor(MeasurementControl.InputRangeIndex));

// Reading * ScaleValueForDisplay = Display value - only used for getDisplayFrowRawValue() and getDisplayFrowRawValue()
// use value shifted by 19 to avoid floating point multiplication by retaining correct value
    ScaleFactorRawToDisplayShift18[11] = sADCScaleFactorShift18 * (getRawAttenuationFactor(11) / (2 * ScaleVoltagePerDiv[11])); // 50 Volt almost same value as 0.5V/div
    ScaleFactorRawToDisplayShift18[10] = sADCScaleFactorShift18 * (getRawAttenuationFactor(10) / (2 * ScaleVoltagePerDiv[10])); // 20 almost same value as 0.5V/div
    ScaleFactorRawToDisplayShift18[9] = sADCScaleFactorShift18 * (getRawAttenuationFactor(9) / (2 * ScaleVoltagePerDiv[9])); // 10
    ScaleFactorRawToDisplayShift18[8] = sADCScaleFactorShift18 * (getRawAttenuationFactor(8) / (2 * ScaleVoltagePerDiv[8])); // 5
    ScaleFactorRawToDisplayShift18[7] = sADCScaleFactorShift18 * (getRawAttenuationFactor(7) / (2 * ScaleVoltagePerDiv[7])); // 2 Volt - almost same value as 0.5V/div, since external attenuation of 4
    ScaleFactorRawToDisplayShift18[6] = sADCScaleFactorShift18 * (getRawAttenuationFactor(6) / (2 * ScaleVoltagePerDiv[6])); // 1 Volt

    ScaleFactorRawToDisplayShift18[5] = sADCScaleFactorShift18 * (getRawAttenuationFactor(5) / (2 * ScaleVoltagePerDiv[5])); // full scale 0,5 Volt / div
    ScaleFactorRawToDisplayShift18[4] = sADCScaleFactorShift18 * (getRawAttenuationFactor(4) / (2 * ScaleVoltagePerDiv[4])); // 0,2
    ScaleFactorRawToDisplayShift18[3] = sADCScaleFactorShift18 * (getRawAttenuationFactor(3) / (2 * ScaleVoltagePerDiv[3])); // 0,1
    ScaleFactorRawToDisplayShift18[2] = sADCScaleFactorShift18 * (getRawAttenuationFactor(2) / (2 * ScaleVoltagePerDiv[2])); // 0,05
    ScaleFactorRawToDisplayShift18[1] = sADCScaleFactorShift18 * (getRawAttenuationFactor(1) / (2 * ScaleVoltagePerDiv[1])); // 0,02
    ScaleFactorRawToDisplayShift18[0] = sADCScaleFactorShift18 * (getRawAttenuationFactor(0) / (2 * ScaleVoltagePerDiv[0])); // 0,01
}

/************************************************************************
 * Graphical output section
 ************************************************************************/

void clearTriggerLine(uint8_t aTriggerLevelDisplayValue) {
    // clear old line
    drawLine(0, aTriggerLevelDisplayValue, DSO_DISPLAY_WIDTH - 1, aTriggerLevelDisplayValue, COLOR_BACKGROUND_DSO);
    // restore grid at old y position
    for (int tXPos = TIMING_GRID_WIDTH - 1; tXPos < DSO_DISPLAY_WIDTH - 1; tXPos += TIMING_GRID_WIDTH) {
        drawPixel(tXPos, aTriggerLevelDisplayValue, COLOR_GRID_LINES);
    }
    if (!MeasurementControl.isRunning) {
        // in analysis mode restore graph at old y position
        uint8_t* ScreenBufferPointer = &DisplayBuffer1[0];
        for (int i = 0; i < DSO_DISPLAY_WIDTH; ++i) {
            int tValueByte = *ScreenBufferPointer++;
            if (tValueByte == aTriggerLevelDisplayValue) {
                // restore old pixel
                drawPixel(i, tValueByte, COLOR_DATA_HOLD);
            }
        }
    }
}

/**
 * draws trigger line if it is visible - do not draw clipped value e.g. value was higher than display range
 */
void drawTriggerLine(void) {
    if (DisplayControl.TriggerLevelDisplayValue != 0 && MeasurementControl.TriggerMode != TRIGGER_MODE_OFF) {
        drawLine(0, DisplayControl.TriggerLevelDisplayValue, DSO_DISPLAY_WIDTH - 1, DisplayControl.TriggerLevelDisplayValue,
                COLOR_TRIGGER_LINE);
    }
}

/**
 * draws vertical timing + horizontal reference voltage lines
 */
void drawGridLinesWithHorizLabelsAndTriggerLine(uint16_t aColor) {
    //vertical lines
    for (int tXPos = TIMING_GRID_WIDTH - 1; tXPos < DSO_DISPLAY_WIDTH; tXPos += TIMING_GRID_WIDTH) {
        drawLine(tXPos, 0, tXPos, DSO_DISPLAY_HEIGHT - 1, aColor);
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
    int tCaptionOffset = FONT_HEIGHT;
    int tLabelColor;
    int tPosX;
    for (int tYPos = DISPLAY_VALUE_FOR_ZERO; tYPos > 0; tYPos -= HORIZONTAL_GRID_HEIGHT) {
        if (tLabelChanged) {
            // clear old label
            int tXpos = DSO_DISPLAY_WIDTH - PIXEL_AFTER_LABEL - (5 * FONT_WIDTH);
            int tY = tYPos - tCaptionOffset;
            fillRect(tXpos, tY, DSO_DISPLAY_WIDTH - PIXEL_AFTER_LABEL - 1, tY + (FONT_HEIGHT - 1), COLOR_BACKGROUND_DSO);
            // restore vertical line
            drawLine(9 * TIMING_GRID_WIDTH - 1, tY, 9 * TIMING_GRID_WIDTH - 1, tY + (FONT_HEIGHT - 1), aColor);
        }
        // draw horizontal line
        drawLine(0, tYPos, DSO_DISPLAY_WIDTH - 1, tYPos, aColor);

        int tCount = snprintf(StringBuffer, sizeof StringBuffer, "%0.*f", RangePrecision[MeasurementControl.DisplayRangeIndex],
                tActualVoltage);
        // right align but leave 2 pixel free after label for the last horizontal line
        tPosX = DSO_DISPLAY_WIDTH - (tCount * FONT_WIDTH) - PIXEL_AFTER_LABEL;
        // draw label over the line - use different color for negative values
        if (tActualVoltage >= 0) {
            tLabelColor = COLOR_HOR_REF_LINE_LABEL;
        } else {
            tLabelColor = COLOR_HOR_REF_LINE_LABEL_NEGATIVE;
        }
        drawText(tPosX, tYPos - tCaptionOffset, StringBuffer, 1, tLabelColor, COLOR_BACKGROUND_DSO);
        tCaptionOffset = (FONT_HEIGHT / 2);
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
        drawLine(0, tValueDisplay, DSO_DISPLAY_WIDTH - 1, tValueDisplay, COLOR_MAX_MIN_LINE);
    }
// min line
    tValueDisplay = getDisplayFrowRawInputValue(MeasurementControl.RawValueMin);
    if (tValueDisplay != DISPLAY_VALUE_FOR_ZERO) {
        drawLine(0, tValueDisplay, DSO_DISPLAY_WIDTH - 1, tValueDisplay, COLOR_MAX_MIN_LINE);
    }
}
/**
 * Draws data on screen
 * @param aDataBufferPointer Data is taken from DataBufferPointer.
 *             If DataBufferPointer is NULL, then Data is taken from DisplayBuffer - used for erasing display before switching mode
 * @param aLength
 * @param aColor Draw color. - used for different colors for pretrigger data or different modes
 * @param aClearBeforeColor if > 0 data from DisplayBuffer is drawn(erased) with this color
 *              just before to avoid interfering with display refresh timing. - used for history modes.
 *              DataBufferPointer must not be null then!
 * @note if aClearBeforeColor >0 then DataBufferPointer must not be NULL
 * @note NOT used for drawing while acquiring
 */
void drawDataBuffer(uint16_t *aDataBufferPointer, int aLength, uint16_t aColor, uint16_t aClearBeforeColor) {
    int i;
    int j = 0;
    int tValue;
    int tLastValue;
    uint8_t *ScreenBufferReadPointer = &DisplayBuffer1[0];
    uint8_t *ScreenBufferWritePointer1 = &DisplayBuffer1[0];
    uint8_t *ScreenBufferWritePointer2 = &DisplayBuffer2[0]; // for trigger state line
    int tLastValueClear = DisplayBuffer1[0];
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
            /*
             * get data from data buffer and perform X scaling
             */
            if (tXScale == 0) {
                tValue = getDisplayFrowRawInputValue(*aDataBufferPointer);
                aDataBufferPointer++;
            } else if (tXScale < -1) {
                // compress - get average of multiple values
                tValue = getDisplayFrowMultipleRawValues(aDataBufferPointer, tXScaleCounter);
                aDataBufferPointer += tXScaleCounter;
            } else if (tXScale == -1) {
                // compress by factor 1.5 - every second value is the average of the next two values
                tValue = getDisplayFrowRawInputValue(*aDataBufferPointer++);
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
                // expand by factor 1.5 - every second value will be shown 2 times
                tValue = getDisplayFrowRawInputValue(*aDataBufferPointer++);
                tXScaleCounter--; // starts with 1
                if (tXScaleCounter < 0) {
                    aDataBufferPointer--;
                    tXScaleCounter = 2;
                }
            } else {
                // expand - show value several times
                tValue = getDisplayFrowRawInputValue(*aDataBufferPointer);
                tXScaleCounter--;
                if (tXScaleCounter == 0) {
                    aDataBufferPointer++;
                    tXScaleCounter = tXScale;
                }
            }
        }

        // draw trigger state line (aka Digital mode)
        if (DisplayControl.DisplayBufferDrawMode & DRAW_MODE_TRIGGER) {
            if (aClearBeforeColor > 0) {
                drawPixel(i, *ScreenBufferWritePointer2, aClearBeforeColor);
            }
            if (tValue > tTriggerValue) {
                drawPixel(i, tTriggerValue - TRIGGER_HIGH_DISPLAY_OFFSET, COLOR_DATA_TRIGGER);
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
                if (tValue != DISPLAYBUFFER_INVISIBLE_VALUE) {
                    drawPixel(i, tValue, COLOR_GRID_LINES);
                }
            } else {
                j++;
                if (aClearBeforeColor > 0) {
                    int tValueClear = *ScreenBufferReadPointer++;
                    if (tValueClear != DISPLAYBUFFER_INVISIBLE_VALUE) {
                        drawPixel(i, tValueClear, aClearBeforeColor);
                    }
                    if (DisplayControl.DisplayBufferDrawMode & DRAW_MODE_LINE) {
                        // erase first line in advance and set pointer
                        tLastValueClear = tValueClear;
                        tValueClear = *ScreenBufferReadPointer++;
                        if (tValueClear != DISPLAYBUFFER_INVISIBLE_VALUE) {
                            if (tLastValueClear != DISPLAYBUFFER_INVISIBLE_VALUE) {
                                // Normal mode - clear line
                                drawLineFastOneX(i, tLastValueClear, tValueClear, aClearBeforeColor);
                            } else {
                                // first visible value just clear start pixel
                                drawPixel(i, tValueClear, aClearBeforeColor);
                            }
                        }
                        tLastValueClear = tValueClear;
                    }
                }
                if (tValue != DISPLAYBUFFER_INVISIBLE_VALUE) {
                    drawPixel(i, tValue, aColor);
                }
            }
        } else {
            /*
             * Line mode here
             */
            if (aClearBeforeColor > 0 && i != DSO_DISPLAY_WIDTH - 1) {
                // erase one x value in advance in order not to overwrite the x+1 part of line just drawn before
                int tValueClear = *ScreenBufferReadPointer++;
                if (tLastValueClear != DISPLAYBUFFER_INVISIBLE_VALUE && tValueClear != DISPLAYBUFFER_INVISIBLE_VALUE) {
                    drawLineFastOneX(i, tLastValueClear, tValueClear, aClearBeforeColor);
                }
                tLastValueClear = tValueClear;
            }
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
                        drawLineFastOneX(i - 1, tLastValue, tValue, COLOR_DATA_RUN_CLIPPING);
                    } else {
                        drawLineFastOneX(i - 1, tLastValue, tValue, aColor);
                    }
                } else {
                    // first visible value just draw start pixel
                    drawPixel(i, tValue, aColor);
                }
            }
#pragma GCC diagnostic pop
        }
        tLastValue = tValue;
        // store data in screen buffer
        *ScreenBufferWritePointer1++ = tValue;
    }
}

/**
 * Draws all chart values till DataBufferNextPointer is reached - used for drawing while acquiring
 * @param aDrawColor
 */
void drawRemainingDataBufferValues(uint16_t aDrawColor) {
    // needed for last acquisition which uses the whole data buffer
    while (DataBufferControl.DataBufferNextDrawPointer < DataBufferControl.DataBufferWritePointer
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

        uint8_t * tDisplayBufferPointer = &DisplayBuffer1[tDisplayX];
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
                drawPixel(tDisplayX, tValue, tColor);
            }
        } else {
            if (tDisplayX < DSO_DISPLAY_WIDTH - 1) {
                // fetch next value and clear line in advance
                tNextValue = DisplayBuffer1[tDisplayX + 1];
                if (tNextValue != DISPLAYBUFFER_INVISIBLE_VALUE) {
                    if (tValue != DISPLAYBUFFER_INVISIBLE_VALUE) {
                        // normal mode
                        drawLineFastOneX(tDisplayX, tValue, tNextValue, DisplayControl.EraseColor);
                    } else {
                        // first visible value, draw only start pixel
                        drawPixel(tDisplayX, tNextValue, DisplayControl.EraseColor);
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
                drawPixel(tDisplayX, tValue, aDrawColor);
            }
        } else {
            if (tDisplayX != 0 && tDisplayX <= DSO_DISPLAY_WIDTH - 1) {
                // get lastValue and draw line
                if (tValue != DISPLAYBUFFER_INVISIBLE_VALUE) {
                    tLastValue = DisplayBuffer1[tDisplayX - 1];
                    if (tLastValue != DISPLAYBUFFER_INVISIBLE_VALUE) {
                        // normal mode
                        drawLineFastOneX(tDisplayX - 1, tLastValue, tValue, aDrawColor);
                    } else {
                        // first visible value, draw only start pixel
                        drawPixel(tDisplayX, tValue, aDrawColor);
                    }
                }
            }
        }
        DataBufferControl.DataBufferNextDrawPointer++;
    }
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
        for (int tDisplayX = 0; tDisplayX < DISPLAY_WIDTH; tDisplayX += 3) {
            /*
             *  get data and perform scaling
             */
            tInputValue = *tFFTDataPointer++;
            tDisplayY = tYDisplayScaleFactor * tInputValue;
            // tDisplayY now from 0 to 32
            tDisplayY = DISPLAY_HEIGHT - tDisplayY;
            DisplayYOld = *tDisplayBufferPtr;
            if (tDisplayY < DisplayYOld) {
                // increase old bar
                fillRect(tDisplayX, tDisplayY, tDisplayX + 2, DisplayYOld, aColor);
            } else if (tDisplayY > DisplayYOld) {
                // Remove part of old bar
                fillRect(tDisplayX, DisplayYOld, tDisplayX + 2, tDisplayY, COLOR_BACKGROUND_DSO);
            }
            *tDisplayBufferPtr++ = tDisplayY;
        }
    }
}

void clearFFTValuesOnDisplay(void) {
    uint8_t *tDisplayBufferPtr = &DisplayBufferFFT[0];
    uint16_t DisplayYOld;
    for (int tDisplayX = 0; tDisplayX < DISPLAY_WIDTH; tDisplayX += 3) {
        DisplayYOld = *tDisplayBufferPtr;
        // Remove old bar
        fillRect(tDisplayX, DisplayYOld, tDisplayX + 2, DISPLAY_HEIGHT - 1, COLOR_BACKGROUND_DSO);
    }
}

/************************************************************************
 * Text output section
 ************************************************************************/

void clearInfo(const uint8_t aInfoLineNumber) {
    int tYPos = (aInfoLineNumber - 1) * FONT_HEIGHT;
    fillRect(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + tYPos, INFO_LEFT_MARGIN + (39 * FONT_WIDTH)- 1,
    INFO_UPPER_MARGIN + tYPos + FONT_HEIGHT - 1, COLOR_BACKGROUND_DSO);

    }

    /*
     * Output info line
     */
void printInfo(void) {
    if (DisplayControl.DisplayPage != DATA || DisplayControl.DisplayMode != PLUS_INFO) {
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
    if (MeasurementControl.ACRange) {
        tValueDiff += RawDSOReadingACZero;
    }
    float tMicrosPeriod = MeasurementControl.PeriodMicros;
    char tPeriodUnitChar = 0xE6; // micro
    if (tMicrosPeriod >= 50000) {
        tMicrosPeriod = tMicrosPeriod / 1000;
        tPeriodUnitChar = 'm'; // milli
    }
    int tUnitsPerGrid;
    if (MeasurementControl.TimebaseIndex >= TIMEBASE_INDEX_MILLIS) {
        tTimebaseUnitChar = 'm';
    } else if (MeasurementControl.TimebaseIndex >= TIMEBASE_INDEX_MICROS) {
        tTimebaseUnitChar = 0xE6; // micro
    } else {
        tTimebaseUnitChar = 'n'; // nano
    }
    tUnitsPerGrid = TimebaseDivValues[MeasurementControl.TimebaseIndex];
// number of digits to be printed after the decimal point
    int tPrecision = 3;
    if ((MeasurementControl.ACRange && MeasurementControl.InputRangeIndex >= 8) || MeasurementControl.InputRangeIndex >= 10) {
        tPrecision = 2;
    }
    if (MeasurementControl.ACRange && MeasurementControl.InputRangeIndex >= 11) {
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
        if (MeasurementControl.ADS7846ChannelsAsDatasource) {
            tChannelChar = ADS7846ChannelChars[MeasurementControl.ADCInputMUXChannelIndex];
        }
        // first line
        snprintf(StringBuffer, sizeof StringBuffer, "%6.*f %6.*f %6.*f %6.*fV%2u %4u%cs %c", tPrecision,
                getFloatFromRawValue(MeasurementControl.RawValueAverage), tPrecision,
                getFloatFromRawValue(MeasurementControl.RawValueMin), tPrecision,
                getFloatFromRawValue(MeasurementControl.RawValueMax), tPrecision, getFloatFromRawValue(tValueDiff),
                DisplayControl.XScale, tUnitsPerGrid, tTimebaseUnitChar, tChannelChar);
        drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN, StringBuffer, 1, COLOR_BLACK, COLOR_INFO_BACKGROUND);

        // second line first part
        drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + FONT_HEIGHT, tBufferForPeriodAndFrequency, 1, COLOR_BLACK,
                COLOR_INFO_BACKGROUND);

        // second line second part
        snprintf(StringBuffer, sizeof StringBuffer, "%c%c %5.*fV", tSlopeChar, tTriggerAutoChar, tPrecision - 1,
                getFloatFromRawValue(MeasurementControl.RawTriggerLevel));
        drawText(TRIGGER_LEVEL_INFO_SMALL_X - (3 * FONT_WIDTH), INFO_UPPER_MARGIN + FONT_HEIGHT, StringBuffer, 1, COLOR_BLACK,
        COLOR_INFO_BACKGROUND);

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
        drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN, StringBuffer, 1, COLOR_BLACK, COLOR_INFO_BACKGROUND);

        const char * tChannelString = ADCInputMUXChannelStrings[MeasurementControl.ADCInputMUXChannelIndex];
        if (MeasurementControl.ADS7846ChannelsAsDatasource) {
            tChannelString = ADS7846ChannelStrings[MeasurementControl.ADCInputMUXChannelIndex];
        }

        // Second line
        // XScale + Timebase + MicrosPerPeriod + Hertz + Channel
        snprintf(StringBuffer, sizeof StringBuffer, "%2d %4u%cs %s %s", DisplayControl.XScale, tUnitsPerGrid, tTimebaseUnitChar,
                tBufferForPeriodAndFrequency, tChannelString);
        drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + FONT_HEIGHT, StringBuffer, 1, COLOR_BLACK, COLOR_INFO_BACKGROUND);

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
        drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + (2 * FONT_HEIGHT), StringBuffer, 1, COLOR_BLACK, COLOR_INFO_BACKGROUND);

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
//		drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + (3 * FONT_HEIGHT), StringBuffer, 1, COLOR_BLUE, COLOR_INFO_BACKGROUND);
        // Debug end
    }
}

/**
 * prints only trigger value
 */
void printTriggerInfo(void) {
    if (DisplayControl.DisplayMode == PLUS_INFO) {
        int tPrecision = 2;
        if ((MeasurementControl.ACRange && MeasurementControl.InputRangeIndex >= 8) || MeasurementControl.InputRangeIndex >= 10) {
            tPrecision = 1;
        }
        if (MeasurementControl.ACRange && MeasurementControl.InputRangeIndex >= 11) {
            tPrecision = 0;
        }
        snprintf(StringBuffer, sizeof StringBuffer, "%5.*fV", tPrecision, getFloatFromRawValue(MeasurementControl.RawTriggerLevel));
        if (MeasurementControl.InfoSizeSmall) {
            drawText(TRIGGER_LEVEL_INFO_SMALL_X, INFO_UPPER_MARGIN + 1 * FONT_HEIGHT, StringBuffer, 1, COLOR_BLACK,
                    COLOR_INFO_BACKGROUND);
        } else {
            drawText(TRIGGER_LEVEL_INFO_LARGE_X, INFO_UPPER_MARGIN + 2 * FONT_HEIGHT, StringBuffer, 1, COLOR_BLACK,
                    COLOR_INFO_BACKGROUND);
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
    if (MeasurementControl.ACRange) {
        aAdcValue -= RawDSOReadingACZero;
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
    if (MeasurementControl.ACRange) {
        aValue += RawDSOReadingACZero;
    }
    return aValue;
}

/*
 * computes corresponding voltage from raw value
 */
float getFloatFromRawValue(int aValue) {
    if (MeasurementControl.ACRange) {
        aValue -= RawDSOReadingACZero;
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
    MeasurementControl.ACRange = false;
    MeasurementControl.InputRangeIndex = 6;    // 1 Volt / div  Raw-1360 / div
    MeasurementControl.DisplayRangeIndex = 3;    // 0,1 Volt / div | Raw-136 / div | 827 max
    MeasurementControl.RawOffsetValueForDisplayRange = 100;
    autoACZeroCalibration();
    initRawToDisplayFactors();
    FactorFromInputToDisplayRangeShift12 = (getRawAttenuationFactor(MeasurementControl.InputRangeIndex)
            * (1 << DSO_INPUT_TO_DISPLAY_SHIFT)) / getRawAttenuationFactor(MeasurementControl.DisplayRangeIndex);

// Tests of raw <-> display conversion routines
    int tValue;
    tValue = getRawOffsetValueFromGridCount(3);    // appr. 416

    tValue = getDisplayFrowRawInputValue(400);
    tValue = getInputRawFromDisplayValue(tValue);

    MeasurementControl.InputRangeIndexOtherThanDisplayRange = true;
    MeasurementControl.ACRange = true;

    tValue = getDisplayFrowRawInputValue(2200);    // since it is ac range
    tValue = getInputRawFromDisplayValue(tValue);

    float tFloatValue;
    tFloatValue = getFloatFromRawValue(400 + RawDSOReadingACZero);
    tFloatValue = getFloatFromDisplayValue(getDisplayFrowRawInputValue(400));
    tFloatValue = ADCToVoltFactor * 400;
    tFloatValue = tFloatValue / ADCToVoltFactor;
// to see above result in debugger
    tFloatValue = tFloatValue * 2;
}
