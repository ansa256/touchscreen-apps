/**
 * @file Chart.cpp
 *
 * Draws charts for LCD screen.
 * Charts have axes and a data area
 * Data can be printed as pixels or line or area
 * Labels and grid are optional
 * Origin is the Parameter PositionX + PositionY and overlaps with the axes
 *
 * @date  14.02.2012
 * @author  Armin Joachimsmeyer
 * armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 *
 *  LCD interface used:
 * 		- getHeight()
 * 		- getWidth()
 * 		- fillRect()
 * 		- drawText()
 * 		- drawPixel()
 * 		- drawLine()
 * 		- FONT_WIDTH
 * 		- FONT_HEIGHT
 *
 */

#include "HY32D.h"

#include "stm32f30x.h"
#include "Chart.h"
#include <stdio.h>
#include <string.h>

/** @addtogroup Graphic_Library
 * @{
 */
/** @addtogroup Chart
 * @{
 */
Chart::Chart(void) {
	mChartBackgroundColor = CHART_DEFAULT_BACKGROUND_COLOR;
	mAxesColor = CHART_DEFAULT_AXES_COLOR;
	mGridColor = CHART_DEFAULT_GRID_COLOR;
	mLabelColor = CHART_DEFAULT_LABEL_COLOR;
	mFlags = 0;
	mXScaleFactor = 1;
}

void Chart::initChartColors(const uint16_t aDataColor, const uint16_t aAxesColor, const uint16_t aGridColor,
		const uint16_t aLabelColor, const uint16_t aBackgroundColor) {
	mDataColor = aDataColor;
	mAxesColor = aAxesColor;
	mGridColor = aGridColor;
	mLabelColor = aLabelColor;
	mChartBackgroundColor = aBackgroundColor;
}

void Chart::setDataColor(uint16_t aDataColor) {
	mDataColor = aDataColor;
}

/**
 * aPositionX and aPositionY are the 0 coordinates of the grid and part of the axes
 */
uint8_t Chart::initChart(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX, const uint16_t aHeightY,
		const uint8_t aAxesSize, const bool aHasGrid, const uint8_t aGridXResolution, const uint8_t aGridYResolution) {
	mPositionX = aPositionX;
	mPositionY = aPositionY;
	mWidthX = aWidthX;
	mHeightY = aHeightY;
	mAxesSize = aAxesSize;
	if (aHasGrid) {
		mFlags |= CHART_HAS_GRID;
	} else {
		mFlags &= ~CHART_HAS_GRID;
	}
	mGridXSpacing = aGridXResolution;
	mGridYSpacing = aGridYResolution;

	return checkParameterValues();
}

uint8_t Chart::checkParameterValues(void) {
	uint8_t tRetValue = 0;
	// also check for zero :-)
	if (mAxesSize - 1 > CHART_MAX_AXES_SIZE) {
		mAxesSize = CHART_MAX_AXES_SIZE;
		tRetValue = CHART_ERROR_AXES_SIZE;
	}
	uint16_t t2AxesSize = 2 * mAxesSize;
	if (mPositionX < t2AxesSize - 1) {
		mPositionX = t2AxesSize - 1;
		mWidthX = 100;
		tRetValue = CHART_ERROR_POS_X;
	}
	if (mPositionY > DISPLAY_HEIGHT - t2AxesSize) {
		mPositionY = DISPLAY_HEIGHT - t2AxesSize;
		tRetValue = CHART_ERROR_POS_Y;
	}
	if (mPositionX + mWidthX > DISPLAY_WIDTH) {
		mPositionX = 0;
		mWidthX = 100;
		tRetValue = CHART_ERROR_WIDTH;
	}

	if (mHeightY > mPositionY + 1) {
		mHeightY = mPositionY + 1;
		tRetValue = CHART_ERROR_HEIGHT;
	}

	if (mGridXSpacing > mWidthX) {
		mGridXSpacing = mWidthX / 2;
		tRetValue = CHART_ERROR_GRID_X_RESOLUTION;
	}
	return tRetValue;

}

void Chart::initXLabelInt(const int aXLabelStartValue, const int aXLabelIncrementValue, const uint8_t aXLabelScaleFactor,
		const uint8_t aXMinStringWidth) {
	mXLabelStartValue.IntValue = aXLabelStartValue;
	mXLabelBaseIncrementValue.IntValue = aXLabelIncrementValue;
	mXScaleFactor = aXLabelScaleFactor;
	mXMinStringWidth = aXMinStringWidth;
	mFlags |= CHART_X_LABEL_INT | CHART_X_LABEL_USED;
}

/**
 *
 * @param aGridXSpacing
 * @param aXLabelIncrementValue
 * @param aXMinStringWidth
 */
void Chart::setXGridAndLabelInt(const uint8_t aGridXSpacing, const int aXLabelIncrementValue, const uint8_t aXMinStringWidth) {
	mGridXSpacing = aGridXSpacing;
	mXLabelBaseIncrementValue.IntValue = aXLabelIncrementValue;
	mXMinStringWidth = aXMinStringWidth;
	mFlags |= CHART_X_LABEL_INT;
	if (mXMinStringWidth != 0) {
		mFlags |= CHART_X_LABEL_USED;
	}
}

/**
 *
 * @param aXLabelStartValue
 * @param aXLabelIncrementValue
 * @param aXScaleFactor
 * @param aXMinStringWidthIncDecimalPoint
 * @param aXNumVarsAfterDecimal
 */
void Chart::initXLabelFloat(const float aXLabelStartValue, const float aXLabelIncrementValue, const int8_t aXScaleFactor,
		uint8_t aXMinStringWidthIncDecimalPoint, uint8_t aXNumVarsAfterDecimal) {
	mXLabelStartValue.FloatValue = aXLabelStartValue;
	mXLabelBaseIncrementValue.FloatValue = aXLabelIncrementValue;
	mXScaleFactor = aXScaleFactor;
	mXNumVarsAfterDecimal = aXNumVarsAfterDecimal;
	mXMinStringWidth = aXMinStringWidthIncDecimalPoint;
	mFlags &= ~CHART_X_LABEL_INT;
	if (aXMinStringWidthIncDecimalPoint != 0) {
		mFlags |= CHART_X_LABEL_USED;
	}
}

/**
 *
 * @param aYLabelStartValue
 * @param aYLabelIncrementValue increment for one grid line
 * @param aYFactor factor for raw to chart value - e.g. (3.0 / 4096) for adc reading of 4096 for 3 Volt
 * @param aYMinStringWidth for y axis label
 */
void Chart::initYLabelInt(const int aYLabelStartValue, const int aYLabelIncrementValue, const float aYFactor,
		const uint8_t aYMinStringWidth) {
	mYLabelStartValue.IntValue = aYLabelStartValue;
	mYLabelBaseIncrementValue.IntValue = aYLabelIncrementValue;
	mYMinStringWidth = aYMinStringWidth;
	mFlags |= CHART_Y_LABEL_INT | CHART_Y_LABEL_USED;
	mYDataFactor = aYFactor;
}

// aYFactor is factor for raw to chart value - e.g. (3.0 / 4096) for adc reading of 4096 for 3 Volt
/**
 *
 * @param aYLabelStartValue
 * @param aYLabelIncrementValue
 * @param aYFactor factor for raw to chart value - e.g. (3.0 / 4096) for adc reading of 4096 for 3 Volt
 * @param aYMinStringWidthIncDecimalPoint for y axis label
 * @param aYNumVarsAfterDecimal for y axis label
 */
void Chart::initYLabelFloat(const float aYLabelStartValue, const float aYLabelIncrementValue, const float aYFactor,
		const uint8_t aYMinStringWidthIncDecimalPoint, const uint8_t aYNumVarsAfterDecimal) {
	mYLabelStartValue.FloatValue = aYLabelStartValue;
	mYLabelBaseIncrementValue.FloatValue = aYLabelIncrementValue;
	mYNumVarsAfterDecimal = aYNumVarsAfterDecimal;
	mYMinStringWidth = aYMinStringWidthIncDecimalPoint;
	mFlags &= ~CHART_Y_LABEL_INT;
	mFlags |= CHART_Y_LABEL_USED;
	mYDataFactor = aYFactor;
}

/**
 * Render the chart on the lcd
 */
void Chart::drawAxesAndGrid(void) {
	drawAxes(false);
	drawGrid();
}

void Chart::drawGrid(void) {
	if (!(mFlags & CHART_HAS_GRID)) {
		return;
	}
	uint16_t tOffset;
// draw vertical lines
	for (tOffset = mGridXSpacing; tOffset <= mWidthX; tOffset += mGridXSpacing) {
		fillRect(mPositionX + tOffset, mPositionY - 1, mPositionX + tOffset, mPositionY - mHeightY + 1, mGridColor);
	}
// draw horizontal lines
	for (tOffset = mGridYSpacing; tOffset <= mHeightY; tOffset += mGridYSpacing) {
		fillRect(mPositionX + 1, mPositionY - tOffset, mPositionX + mWidthX - 1, mPositionY - tOffset, mGridColor);
	}

}

/**
 * render axes
 * renders indicators if labels but no grid are specified
 * render labels only if at least one increment value != 0
 * @retval -11 in no space for X labels or - 12 if no space for Y labels on lcd
 */
void Chart::drawAxes(bool aClearLabelsBefore) {
	drawXAxis(aClearLabelsBefore);
	drawYAxis(aClearLabelsBefore);
}

/**
 * draw x title
 */
void Chart::drawXAxisTitle(void) const {
	if (mXTitleText != NULL) {
		/**
		 * draw axis title
		 */
		uint16_t tTextLenPixel = strlen(mXTitleText) * FONT_WIDTH;
		drawText(mPositionX + mWidthX - tTextLenPixel - 1, mPositionY - FONT_HEIGHT, mXTitleText, 1, mLabelColor,
				mChartBackgroundColor);
	}
}

/**
 * multiplies value with XScaleFactor if XScaleFactor is < -1 or divide if XScaleFactor is > 1
 * Used only by drawAxis()
 */
int Chart::adjustIntWithXScaleFactor(int Value) {
	if (mXScaleFactor < -1) {
		return Value * -mXScaleFactor;
	}
	if (mXScaleFactor > 1) {
		return Value / mXScaleFactor;
	}
	return Value;
}
/**
 * multiplies value with XScaleFactor if XScaleFactor is < -1 or divide if XScaleFactor is > 1
 */
float Chart::adjustFloatWithXScaleFactor(float Value) {
	if (mXScaleFactor < -1) {
		return Value * -mXScaleFactor;
	}
	if (mXScaleFactor > 1) {
		return Value / mXScaleFactor;
	}
	return Value;
}

/**
 * draw x line with indicators and labels
 */
void Chart::drawXAxis(bool aClearLabelsBefore) {

	char tLabelStringBuffer[32];

// draw X line
	fillRect(mPositionX - mAxesSize + 1, mPositionY, mPositionX + mWidthX - 1, mPositionY + mAxesSize - 1, mAxesColor);

	if (mFlags & CHART_X_LABEL_USED) {
		uint16_t tOffset;
		if (!(mFlags & CHART_HAS_GRID)) {
			/*
			 * draw indicators with the same size the axis has
			 */
			uint16_t tIndicatorYBottom = mPositionY + (2 * mAxesSize) - 1;
			// draw indicators with the same size the axis has
			for (tOffset = 0; tOffset <= mWidthX; tOffset += mGridXSpacing) {
				fillRect(mPositionX + tOffset, mPositionY + mAxesSize, mPositionX + tOffset, tIndicatorYBottom, mGridColor);
			}
		}

		/*
		 * draw labels (numbers)
		 */
		uint16_t tNumberYTop = mPositionY + 2 * mAxesSize;
		assertParamMessage((tNumberYTop <= (DISPLAY_HEIGHT - FONT_HEIGHT)), "no space for x labels", tNumberYTop);

		// first offset is negative
		tOffset = 1 - ((FONT_WIDTH * mXMinStringWidth) / 2);
		if (aClearLabelsBefore) {
			// clear label space before
			fillRect(mPositionX + tOffset, tNumberYTop, mPositionX + mWidthX - 1, tNumberYTop + FONT_HEIGHT - 1,
					mChartBackgroundColor);
		}

		// initialize both variables to avoid compiler warnings
		int tValue = mXLabelStartValue.IntValue;
		int tIncrementValue = adjustIntWithXScaleFactor(mXLabelBaseIncrementValue.IntValue);
		float tValueFloat = mXLabelStartValue.FloatValue;
		float tIncrementValueFloat = adjustIntWithXScaleFactor(mXLabelBaseIncrementValue.FloatValue);

		/*
		 * draw loop
		 */
		do {
			if (mFlags & CHART_X_LABEL_INT) {
				snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%d", tValue);
				tValue += tIncrementValue;
			} else {
				snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%*.*f", mXMinStringWidth, mXNumVarsAfterDecimal,
						tValueFloat);
				tValueFloat += tIncrementValueFloat;
			}
			drawText(mPositionX + tOffset, tNumberYTop, tLabelStringBuffer, 1, mLabelColor, mChartBackgroundColor);
			tOffset += mGridXSpacing;
		} while (tOffset <= mWidthX);
	}
	drawXAxisTitle();
}

/**
 * Set x label start to index.th value / start not with first but with startIndex label
 * redraw Axis
 */
void Chart::setXLabelIntStartValueByIndex(const int aNewXStartIndex, const bool doDraw) {
	mXLabelStartValue.IntValue = mXLabelBaseIncrementValue.IntValue * aNewXStartIndex;
	if (doDraw) {
		drawXAxis(true);
	}
}

/**
 * If aDoIncrement = true increment XLabelStartValue , else decrement
 * redraw Axis
 * @retval true if X value was not clipped
 */bool Chart::stepXLabelStartValueInt(const bool aDoIncrement, const int aMinValue, const int aMaxValue) {
	bool tRetval = true;
	if (aDoIncrement) {
		mXLabelStartValue.IntValue += mXLabelBaseIncrementValue.IntValue;
		if (mXLabelStartValue.IntValue > aMaxValue) {
			mXLabelStartValue.IntValue = aMaxValue;
			tRetval = false;
		}
	} else {
		mXLabelStartValue.IntValue -= mXLabelBaseIncrementValue.IntValue;
		if (mXLabelStartValue.IntValue < aMinValue) {
			mXLabelStartValue.IntValue = aMinValue;
			tRetval = false;
		}
	}
	drawXAxis(true);
	return tRetval;
}

/**
 * Increments or decrements the start value by one increment value (one grid line)
 * and redraws Axis
 * does not decrement below 0
 */
float Chart::stepXLabelStartValueFloat(const bool aDoIncrement) {
	if (aDoIncrement) {
		mXLabelStartValue.FloatValue += mXLabelBaseIncrementValue.FloatValue;
	} else {
		mXLabelStartValue.FloatValue -= mXLabelBaseIncrementValue.FloatValue;
	}
	if (mXLabelStartValue.FloatValue < 0) {
		mXLabelStartValue.FloatValue = 0;
	}
	drawXAxis(true);
	return mXLabelStartValue.FloatValue;
}

/**
 * draw y title
 */
void Chart::drawYAxisTitle(const int aYOffset) const {
	if (mYTitleText != NULL) {
		/**
		 * draw axis title - use data color
		 */
		drawText(mPositionX + mAxesSize + 1, mPositionY - mHeightY + aYOffset, mYTitleText, 1, mDataColor, mChartBackgroundColor);
	}
}

/**
 * draw y line with indicators and labels
 */
void Chart::drawYAxis(const bool aClearLabelsBefore) {

	char tLabelStringBuffer[32];

	fillRect(mPositionX - mAxesSize + 1, mPositionY - mHeightY + 1, mPositionX, mPositionY - 1, mAxesColor);

	if (mFlags & CHART_Y_LABEL_USED) {
		uint16_t tOffset;
		if (!(mFlags & CHART_HAS_GRID)) {
			/*
			 * draw indicators with the same size the axis has
			 */
			uint16_t tIndicatorXLeft = mPositionX - (2 * mAxesSize) + 1;

			for (tOffset = 0; tOffset <= mHeightY; tOffset += mGridYSpacing) {
				fillRect(tIndicatorXLeft, mPositionY - tOffset, mPositionX - mAxesSize, mPositionY - tOffset, mGridColor);
			}
		}

		/*
		 * draw labels (numbers)
		 */
		int16_t tNumberXLeft = mPositionX - 2 * mAxesSize - 1 - (mYMinStringWidth * FONT_WIDTH);
		assertParamMessage((tNumberXLeft >= 0), "no space for y labels", tNumberXLeft);

		// first offset is negative
		tOffset = FONT_HEIGHT / 2;
		if (aClearLabelsBefore) {
			// clear label space before
			fillRect(tNumberXLeft, mPositionY - mHeightY + 1, mPositionX - mAxesSize - 1, mPositionY - tOffset + FONT_HEIGHT,
					mChartBackgroundColor);
		}

		// convert to string
		// initialize both variables to avoid compiler warnings
		long tValue = mYLabelStartValue.IntValue;
		float tValueFloat = mYLabelStartValue.FloatValue;
		/*
		 * draw loop
		 */
		do {
			if (mFlags & CHART_Y_LABEL_INT) {
				snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%ld", tValue);
				tValue += mYLabelBaseIncrementValue.IntValue;
			} else {
				snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%*.*f", mYMinStringWidth, mYNumVarsAfterDecimal,
						tValueFloat);
				tValueFloat += mYLabelBaseIncrementValue.FloatValue;
			}
			drawText(tNumberXLeft, mPositionY - tOffset, tLabelStringBuffer, 1, mLabelColor, mChartBackgroundColor);
			tOffset += mGridYSpacing;
		} while (tOffset <= mHeightY);
	}
}

bool Chart::stepYLabelStartValueInt(const bool aDoIncrement, const int aMinValue, const int aMaxValue) {
	bool tRetval = true;
	if (aDoIncrement) {
		mYLabelStartValue.IntValue += mYLabelBaseIncrementValue.IntValue;
		if (mYLabelStartValue.IntValue > aMaxValue) {
			mYLabelStartValue.IntValue = aMaxValue;
			tRetval = false;
		}
	} else {
		mYLabelStartValue.IntValue -= mYLabelBaseIncrementValue.IntValue;
		if (mYLabelStartValue.IntValue < aMinValue) {
			mYLabelStartValue.IntValue = aMinValue;
			tRetval = false;
		}
	}
	drawYAxis(true);
	return tRetval;
}

/**
 * increments or decrements the start value by one increment value (one grid line)
 * and redraws Axis
 * does not decrement below 0
 */
float Chart::stepYLabelStartValueFloat(const int aSteps) {
	mYLabelStartValue.FloatValue += mYLabelBaseIncrementValue.FloatValue * aSteps;
	if (mYLabelStartValue.FloatValue < 0) {
		mYLabelStartValue.FloatValue = 0;
	}
	drawYAxis(false);
	return mYLabelStartValue.FloatValue;
}

/**
 * Clears chart area and redraws axes lines
 */
void Chart::clear(void) {
	fillRect(mPositionX + 1, mPositionY - 1, mPositionX + mWidthX - 1, mPositionY - mHeightY + 1, mChartBackgroundColor);
	// draw X line
	fillRect(mPositionX - mAxesSize + 1, mPositionY, mPositionX + mWidthX - 1, mPositionY + mAxesSize - 1, mAxesColor);
	//draw y line
	fillRect(mPositionX - mAxesSize + 1, mPositionY - mHeightY + 1, mPositionX, mPositionY - 1, mAxesColor);
}

/**
 * Draws a chart  - Factor for raw to chart value (mYFactor) is used to compute display values
 * @param aDataPointer pointer to raw data array
 * @param aDataEndPointer pointer to first element after data
 * @param aMode CHART_MODE_PIXEL, CHART_MODE_LINE or CHART_MODE_AREA
 * @return false if clipping occurs
 */bool Chart::drawChartData(const int16_t * aDataPointer, const int16_t * aDataEndPointer, const uint8_t aMode) {

	bool tRetValue = true;
	int tDisplayValue;

// used only in line mode
	int tLastValue = 0;

	// Factor for Raw -> Display value
	float tYDisplayFactor = 1;
	int tYOffset = 0;

	if (mFlags & CHART_Y_LABEL_INT) {
		// mGridYSpacing / mYLabelIncrementValue.IntValue is factor raw -> pixel e.g. 40 pixel for 200 value
		tYDisplayFactor = (mYDataFactor * mGridYSpacing) / mYLabelBaseIncrementValue.IntValue;
		tYOffset = mYLabelStartValue.IntValue / mYDataFactor;
	} else {
		tYDisplayFactor = (mYDataFactor * mGridYSpacing) / mYLabelBaseIncrementValue.FloatValue;
		tYOffset = mYLabelStartValue.FloatValue / mYDataFactor;
	}

	uint16_t tXpos = mPositionX;
	bool tFirstValue = true;

	uint8_t tXScaleCounter = mXScaleFactor;
	if (mXScaleFactor < 0) {
		tXScaleCounter = -mXScaleFactor;
	}

	for (int i = mWidthX; i > 0; i--) {
		if (mXScaleFactor == 1) {
			tDisplayValue = *aDataPointer++;
		} else if (mXScaleFactor < 0) {
			tDisplayValue = 0;
			for (int i = 0; i < tXScaleCounter; ++i) {
				tDisplayValue += *aDataPointer++;
			}
			tDisplayValue /= tXScaleCounter;
		} else {
			tDisplayValue = *aDataPointer;
			tXScaleCounter--;
			if (tXScaleCounter == 0) {
				aDataPointer++;
				tXScaleCounter = mXScaleFactor;
			}
		}
		// check for data still in data buffer
		if (aDataPointer >= aDataEndPointer) {
			break;
		}

		tDisplayValue = tYDisplayFactor * (tDisplayValue - tYOffset);
		// clip to bottom line
		if (tDisplayValue < 0) {
			tDisplayValue = 0;
			tRetValue = false;
		}
		// clip to top value
		if (tDisplayValue > mHeightY - 1) {
			tDisplayValue = mHeightY - 1;
			tRetValue = false;
		}
		// draw first value as pixel only
		if (aMode == CHART_MODE_PIXEL || tFirstValue) {
			tFirstValue = false;
			drawPixel(tXpos, mPositionY - tDisplayValue, mDataColor);
		} else if (aMode == CHART_MODE_LINE) {
			drawLineFastOneX(tXpos - 1, mPositionY - tLastValue, mPositionY - tDisplayValue, mDataColor);
		} else if (aMode == CHART_MODE_AREA) {
			drawLine(tXpos, mPositionY, tXpos, mPositionY - tDisplayValue, mDataColor);
		}
		tLastValue = tDisplayValue;
		tXpos++;
	}
	return tRetValue;
}

/**
 *
 * @param aDataPointer
 * @param aDataLength
 * @param aMode
 * @return false if clipping occurs
 */bool Chart::drawChartDataDirect(const uint8_t * aDataPointer, uint16_t aDataLength, const uint8_t aMode) {

	bool tRetValue = true;
	uint8_t tValue;

	if (aDataLength > mWidthX) {
		aDataLength = mWidthX;
		tRetValue = false;
	}

// used only in line mode
	uint8_t tLastValue = *aDataPointer;
	if (tLastValue > mHeightY - 1) {
		tLastValue = mHeightY - 1;
		tRetValue = false;
	}

	uint16_t tXpos = mPositionX;

	for (; aDataLength > 0; aDataLength--) {
		tValue = *aDataPointer++;
		if (tValue > mHeightY - 1) {
			tValue = mHeightY - 1;
			tRetValue = false;
		}
		if (aMode == CHART_MODE_PIXEL) {
			tXpos++;
			drawPixel(tXpos, mPositionY - tValue, mDataColor);
		} else if (aMode == CHART_MODE_LINE) {
			drawLineFastOneX(tXpos, mPositionY - tLastValue, mPositionY - tValue, mDataColor);
//			drawLine(tXpos, mPositionY - tLastValue, tXpos + 1, mPositionY - tValue,
//					aDataColor);
			tXpos++;
			tLastValue = tValue;
		} else if (aMode == CHART_MODE_AREA) {
			tXpos++;
			drawLine(tXpos, mPositionY, tXpos, mPositionY - tValue, mDataColor);
		}
	}
	return tRetValue;
}

uint16_t Chart::getHeightY(void) const {
	return mHeightY;
}

uint16_t Chart::getPositionX(void) const {
	return mPositionX;
}

uint16_t Chart::getPositionY(void) const {
	return mPositionY;
}

uint16_t Chart::getWidthX(void) const {
	return mWidthX;
}

void Chart::setHeightY(uint16_t heightY) {
	mHeightY = heightY;
}

void Chart::setPositionX(uint16_t positionX) {
	mPositionX = positionX;
}

void Chart::setPositionY(uint16_t positionY) {
	mPositionY = positionY;
}

void Chart::setWidthX(uint16_t widthX) {
	mWidthX = widthX;
}

void Chart::setXGridSpacing(uint8_t aXGridSpacing) {
	mGridXSpacing = aXGridSpacing;
}

void Chart::setYGridSpacing(uint8_t aYGridSpacing) {
	mGridYSpacing = aYGridSpacing;
}

uint8_t Chart::getXGridSpacing(void) const {
	return mGridXSpacing;
}

uint8_t Chart::getYGridSpacing(void) const {
	return mGridYSpacing;
}

void Chart::setXScaleFactor(int aXScaleFactor, const bool doDraw) {
	mXScaleFactor = aXScaleFactor;
	if (doDraw) {
		drawXAxis(true);
	}
}

int Chart::getXScaleFactor(void) const {
	return mXScaleFactor;
}

/*
 * Label
 */
void Chart::setXLabelStartValue(int xLabelStartValue) {
	mXLabelStartValue.IntValue = xLabelStartValue;
}

void Chart::setXLabelStartValueFloat(float xLabelStartValueFloat) {
	mXLabelStartValue.FloatValue = xLabelStartValueFloat;
}

void Chart::setYLabelStartValue(int yLabelStartValue) {
	mYLabelStartValue.IntValue = yLabelStartValue;
}

void Chart::setYLabelStartValueFloat(float yLabelStartValueFloat) {
	mYLabelStartValue.FloatValue = yLabelStartValueFloat;
}

/**
 * not tested
 * @retval (YStartValue / mYFactor)
 */
uint16_t Chart::getYLabelStartValueRawFromFloat(void) {
	return (mYLabelStartValue.FloatValue / mYDataFactor);
}

/**
 * not tested
 * @retval (YEndValue = YStartValue + (scale * (mHeightY / GridYSpacing))  / mYFactor
 */
uint16_t Chart::getYLabelEndValueRawFromFloat(void) {
	return ((mYLabelStartValue.FloatValue + mYLabelBaseIncrementValue.FloatValue * (mHeightY / mGridYSpacing)) / mYDataFactor);
}

void Chart::setXLabelBaseIncrementValue(int xLabelBaseIncrementValue) {
	mXLabelBaseIncrementValue.IntValue = xLabelBaseIncrementValue;
}

void Chart::setXLabelBaseIncrementValueFloat(float xLabelBaseIncrementValueFloat) {
	mXLabelBaseIncrementValue.FloatValue = xLabelBaseIncrementValueFloat;
}

void Chart::setYLabelBaseIncrementValue(int yLabelBaseIncrementValue) {
	mYLabelBaseIncrementValue.IntValue = yLabelBaseIncrementValue;
}

void Chart::setYLabelBaseIncrementValueFloat(float yLabelBaseIncrementValueFloat) {
	mYLabelBaseIncrementValue.FloatValue = yLabelBaseIncrementValueFloat;
}

int_float_union Chart::getXLabelStartValue(void) const {
	return mXLabelStartValue;
}

int_float_union Chart::getYLabelStartValue(void) const {
	return mYLabelStartValue;
}

void Chart::disableXLabel(void) {
	mFlags &= ~CHART_X_LABEL_USED;
}

void Chart::disableYLabel(void) {
	mFlags &= ~CHART_Y_LABEL_USED;
}

void Chart::setXTitleText(const char * aTitleText) {
	mXTitleText = aTitleText;
}

void Chart::setYTitleText(const char * aTitleText) {
	mYTitleText = aTitleText;
}
/** @} */
/** @} */
