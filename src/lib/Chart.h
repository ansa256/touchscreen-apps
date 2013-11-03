/**
 * chart.h
 *
 * Draws charts for LCD screen
 * Charts have axes and a data area
 * Data can be printed as pixels or line or area
 * Labels and grid are optional
 *
 * @date  14.02.2012
 * @author  Armin Joachimsmeyer
 * armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 *
 */

#ifndef CHART_H_
#define CHART_H_

#define CHART_DEFAULT_AXES_COLOR 		COLOR_BLACK
#define CHART_DEFAULT_GRID_COLOR 		RGB( 180, 180, 180)
#define CHART_DEFAULT_BACKGROUND_COLOR  COLOR_WHITE
#define CHART_DEFAULT_LABEL_COLOR  		COLOR_BLACK
#define CHART_MAX_AXES_SIZE 			10

// data drawing modes
#define CHART_MODE_PIXEL 				0
#define CHART_MODE_LINE 				1
#define CHART_MODE_AREA 				2

// Error codes
#define CHART_ERROR_POS_X 		-1
#define CHART_ERROR_POS_Y 		-2
#define CHART_ERROR_WIDTH 		-4
#define CHART_ERROR_HEIGHT		-8
#define CHART_ERROR_AXES_SIZE	-16
#define CHART_ERROR_GRID_X_RESOLUTION -32

// Masks for mFlags
#define CHART_HAS_GRID 0x01
#define CHART_X_LABEL_USED 0x02
#define CHART_X_LABEL_INT 0x04 // else label is float
#define CHART_Y_LABEL_USED 0x08
#define CHART_Y_LABEL_INT 0x10 // else label is float
typedef union {
	int IntValue;
	float FloatValue;
} int_float_union;

class Chart {
public:
	Chart(void);
	uint8_t initChart(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX, const uint16_t aHeightY,
			const uint8_t aAxesSize, const bool aHasGrid, const uint8_t aGridXResolution, const uint8_t aGridYResolution);

	void initChartColors(const uint16_t aDataColor, const uint16_t aAxesColor, const uint16_t aGridColor,
			const uint16_t aLabelColor, const uint16_t aBackgroundColor);
	void setDataColor(uint16_t aDataColor);

	void clear(void);

	void setWidthX(const uint16_t widthX);
	void setHeightY(const uint16_t heightY);
	void setPositionX(const uint16_t positionX);
	void setPositionY(const uint16_t positionY);

	uint16_t getWidthX(void) const;
	uint16_t getHeightY(void) const;
	uint16_t getPositionX(void) const;
	uint16_t getPositionY(void) const;

	void drawAxes(const bool aClearLabelsBefore);
	void drawAxesAndGrid(void);
	bool drawChartDataDirect(const uint8_t *aDataPointer, uint16_t aDataLength, const uint8_t aMode);
	bool drawChartData(const int16_t *aDataPointer, const int16_t * aDataEndPointer, const uint8_t aMode);
	void drawGrid(void);

	/***************
	 * X Axis
	 */
	void drawXAxis(const bool aClearLabelsBefore);

	void setXGridSpacing(uint8_t aXGridSpacing);
	void setXGridAndLabelInt(const uint8_t aGridXSpacing, const int aXLabelIncrementValue, const uint8_t aXMinStringWidth);
	uint8_t getXGridSpacing(void) const;

	/*
	 * X Label
	 */
	// Init
	void initXLabelInt(const int aXLabelStartValue, const int aXLabelIncrementValue, const uint8_t aXLabelScaleFactor,
			const uint8_t aXMinStringWidth);
	void initXLabelFloat(const float aXLabelStartValue, const float aXLabelIncrementValue, const int8_t aXScaleFactor,
			uint8_t aXMinStringWidthIncDecimalPoint, uint8_t aXNumVarsAfterDecimal);
	void disableXLabel(void);

	// Start value
	void setXLabelStartValue(int xLabelStartValue);
	void setXLabelStartValueFloat(float xLabelStartValueFloat);
	void setXLabelIntStartValueByIndex(const int aNewXStartIndex, const bool doDraw); // does not use XScaleFactor
	int_float_union getXLabelStartValue(void) const;
	bool stepXLabelStartValueInt(const bool aDoIncrement, const int aMinValue, const int aMaxValue); // does not use XScaleFactor
	float stepXLabelStartValueFloat(const bool aDoIncrement); // does not use XScaleFactor

	// Increment value
	void setXLabelBaseIncrementValue(int xLabelIncrementValue);
	void setXLabelBaseIncrementValueFloat(float xLabelIncrementValueFloat);
	// Increment factor
	void setXScaleFactor(int aXScaleFactor, const bool doDraw);
	int getXScaleFactor(void) const;

	// X Title
	void setXTitleText(const char * aLabelText);
	void drawXAxisTitle(void) const;

	/*******************
	 *  Y Axis
	 */
	void drawYAxis(const bool aClearLabelsBefore);

	void setYGridSpacing(uint8_t aYGridSpacing);
	uint8_t getYGridSpacing(void) const;

	// Y Label
	void initYLabelInt(const int aYLabelStartValue, const int aYLabelIncrementValue, const float aYFactor,
			const uint8_t aMaxYLabelCharacters);
	void initYLabelFloat(const float aYLabelStartValue, const float aYLabelIncrementValue, const float aYFactor,
			const uint8_t aYMinStringWidthIncDecimalPoint, const uint8_t aYNumVarsAfterDecimal);
	void disableYLabel(void);

	// Start value
	void setYLabelStartValue(int yLabelStartValue);
	void setYLabelStartValueFloat(float yLabelStartValueFloat);
	int_float_union getYLabelStartValue(void) const;
	uint16_t getYLabelStartValueRawFromFloat(void);
	uint16_t getYLabelEndValueRawFromFloat(void);
	bool stepYLabelStartValueInt(const bool aDoIncrement, const int aMinValue, const int aMaxValue);
	float stepYLabelStartValueFloat(int aSteps);

	// Increment value
	void setYLabelBaseIncrementValue(int yLabelIncrementValue);
	void setYLabelBaseIncrementValueFloat(float yLabelIncrementValueFloat);
	// Increment factor
	//void setYScaleFactor(int aYScaleFactor, const bool doDraw);

	// Y Title
	void setYTitleText(const char * aLabelText);
	void drawYAxisTitle(const int aYOffset) const;

private:

	// layout
	uint16_t mPositionX; // Position in display coordinates of x - origin is on x axis
	uint16_t mPositionY; // Position in display coordinates of y - origin is on y axis
	uint16_t mWidthX; // length of x axes in pixel
	uint16_t mHeightY; // height of y axes in pixel
	uint16_t mChartBackgroundColor;
	uint8_t mAxesSize; //thickness of x and y axes - origin is on innermost line of axes
	uint8_t mFlags;
	// grid
	uint16_t mDataColor;
	uint16_t mAxesColor;
	uint16_t mGridColor;
	uint16_t mLabelColor;

	// Scale value for data display
	float mYDataFactor; // Factor for raw to chart value - e.g. (3.0 / 4096) for adc reading of 4096 for 3 Volt or 0.2 for 1000 display at 5000 raw value

	// X axis
	uint8_t mGridXSpacing; // difference in pixel between 2 X grid lines
	int_float_union mXLabelStartValue;
	int_float_union mXLabelBaseIncrementValue; // Value difference between 2 grid labels - the effective IncrementValue is mXScaleFactor * mXLabelBaseIncrementValue
	int8_t mXScaleFactor; // Factor for X Data expansion(>1) or compression(<-1). 2->display 1 value 2 times -2->display average of 2 values etc.

	// label formatting
	uint8_t mXNumVarsAfterDecimal;
	uint8_t mXMinStringWidth;

	const char* mXTitleText; // No title text if NULL

	// Y label
	uint8_t mGridYSpacing; // difference in pixel between 2 Y grid lines
	int_float_union mYLabelStartValue;
	int_float_union mYLabelBaseIncrementValue; // Value difference between 2 grid labels

	// label formatting
	uint8_t mYNumVarsAfterDecimal;
	uint8_t mYMinStringWidth;

	const char* mYTitleText; // No title text if NULL

	uint8_t checkParameterValues();
	int adjustIntWithXScaleFactor(int Value);
	float adjustFloatWithXScaleFactor(float Value);
};

#endif /* CHART_H_ */
