/*
 * PageAcceleratorCompassGyroDemo.cpp
 * @brief shows graphic output of the accelerator, gyroscope and compass of the STM32F3-Discovery board
 *
 * @date 02.01.2013
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 *      Source based on STM code samples
 * @version 1.2.0
 */

#include "Pages.h"
#include "misc.h"

#ifdef __cplusplus
extern "C" {

#include "usb_misc.h"

#include "hw_config.h" // for USB
#include "usb_lib.h"

#include "stm32f30x.h"
#include "stm32f3_discovery.h"
#include "stm32f3_discovery_lsm303dlhc.h"
#include "stm32f3_discovery_l3gd20.h"
#include "stm32f3_discovery_leds_buttons.h"
#include "math.h"

}
#endif

#define LSM_Acc_Sensitivity_2g     (float)     1.0f            /*!< accelerometer sensitivity with 2 g full scale [LSB/mg] */
#define LSM_Acc_Sensitivity_4g     (float)     0.5f            /*!< accelerometer sensitivity with 4 g full scale [LSB/mg] */
#define LSM_Acc_Sensitivity_8g     (float)     0.25f           /*!< accelerometer sensitivity with 8 g full scale [LSB/mg] */
#define LSM_Acc_Sensitivity_16g    (float)     0.0834f         /*!< accelerometer sensitivity with 12 g full scale [LSB/mg] */
#define PI                         (float)     3.14159265f

#define LSM303DLHC_CALIBRATION_OVERSAMPLING 8
#define COLOR_ACC_GYRO_BACKGROUND COLOR_CYAN

#define COMPASS_RADIUS 40
#define COMPASS_MID_X (COMPASS_RADIUS + 10)
#define COMPASS_MID_Y 140

#define GYRO_MID_X (DISPLAY_WIDTH - (2*COMPASS_RADIUS))
#define GYRO_MID_Y (COMPASS_MID_Y + COMPASS_RADIUS)

#define VERTICAL_SLIDER_NULL_VALUE 90
#define HORIZONTAL_SLIDER_NULL_VALUE 160

#define TEXT_START_Y 10
#define TEXT_START_X 10

int16_t AcceleratorZeroCompensation[3];
int16_t GyroscopeZeroCompensation[3];

static struct ThickLine AcceleratorLine;
static struct ThickLine CompassLine;
static struct ThickLine GyroYawLine;

TouchButtonAutorepeat * TouchButtonAutorepeatAccScalePlus;
TouchButtonAutorepeat * TouchButtonAutorepeatAccScaleMinus;

static TouchButton * TouchButtonClearScreen;

void doSetZero(TouchButton * const aTheTouchedButton, int16_t aValue);
static TouchButton * TouchButtonSetZero;

void doChangeAccScale(TouchButton * const aTheTouchedButton, int16_t aValue);
uint8_t AcceleratorScale;

static int16_t RawDataBuffer[3];
void readCompassRaw(int16_t* aCompassRawData);
void readGyroscopeRaw(int16_t* aGyroscopeRawData);
int16_t readTempRaw(void);

void drawAccDemoGui(void) {
	TouchButtonMainHome->drawButton();
	TouchButtonClearScreen->drawButton();
	TouchButtonAutorepeatAccScalePlus->drawButton();
	TouchButtonAutorepeatAccScaleMinus->drawButton();
	drawCircle(COMPASS_MID_X, COMPASS_MID_Y, COMPASS_RADIUS, COLOR_BLACK);

	TouchButtonSetZero->drawButton();

	/*
	 * Vertical slider
	 */
	TouchSliderVertical.initSlider(1, 10, 2, 2 * VERTICAL_SLIDER_NULL_VALUE, VERTICAL_SLIDER_NULL_VALUE, VERTICAL_SLIDER_NULL_VALUE,
			NULL, TOUCHSLIDER_DEFAULT_TOUCH_BORDER, TOUCHSLIDER_SHOW_VALUE, NULL, NULL);
	TouchSliderVertical.setBarColor(COLOR_RED);
	TouchSliderVertical.setBarThresholdColor(COLOR_GREEN);

//	TouchSliderVertical2.initSlider(50, 40, 2, 2 * VERTICAL_SLIDER_NULL_VALUE, VERTICAL_SLIDER_NULL_VALUE,
//			VERTICAL_SLIDER_NULL_VALUE, NULL, TOUCHSLIDER_DEFAULT_TOUCH_BORDER, TOUCHSLIDER_SHOW_VALUE, NULL, NULL);
//	TouchSliderVertical2.setBarColor(COLOR_RED);
//	TouchSliderVertical2.setBarThresholdColor(COLOR_GREEN);

// Horizontal slider
	TouchSliderHorizontal.initSlider(0, 1, 2, 2 * HORIZONTAL_SLIDER_NULL_VALUE, HORIZONTAL_SLIDER_NULL_VALUE,
			HORIZONTAL_SLIDER_NULL_VALUE, NULL, TOUCHSLIDER_DEFAULT_TOUCH_BORDER,
			TOUCHSLIDER_SHOW_VALUE | TOUCHSLIDER_IS_HORIZONTAL | TOUCHSLIDER_HORIZONTAL_VALUE_LEFT, NULL, NULL);
	TouchSliderHorizontal.setBarColor(COLOR_RED);
	TouchSliderHorizontal.setBarThresholdColor(COLOR_GREEN);

	TouchSliderVertical.drawBar();
	TouchSliderHorizontal.drawBar();

}

/**
 * init hardware and thick lines
 */
void initAcceleratorCompass(void) {
	initAcceleratorCompassChip();
	GyroscopeAndSPIInitialize();
	Gyroscope_IO_initialize();

	AcceleratorLine.StartX = DISPLAY_WIDTH / 2;
	AcceleratorLine.StartY = DISPLAY_HEIGHT / 2;
	AcceleratorLine.EndX = DISPLAY_WIDTH / 2;
	AcceleratorLine.EndY = DISPLAY_HEIGHT / 2;
	AcceleratorLine.Thickness = 2;
	AcceleratorLine.ThicknessMode = LINE_THICKNESS_DRAW_CLOCKWISE;
	AcceleratorLine.Color = COLOR_RED;
	AcceleratorLine.BackgroundColor = COLOR_ACC_GYRO_BACKGROUND;

	CompassLine.StartX = COMPASS_MID_X;
	CompassLine.StartY = COMPASS_MID_Y;
	CompassLine.EndX = COMPASS_MID_X;
	CompassLine.EndY = COMPASS_MID_Y;
	CompassLine.Thickness = 5;
	CompassLine.ThicknessMode = LINE_THICKNESS_MIDDLE;
	CompassLine.Color = COLOR_BLACK;
	CompassLine.BackgroundColor = COLOR_ACC_GYRO_BACKGROUND;

	GyroYawLine.StartX = GYRO_MID_X;
	GyroYawLine.StartY = GYRO_MID_Y;
	GyroYawLine.EndX = GYRO_MID_X;
	GyroYawLine.EndY = GYRO_MID_Y;
	GyroYawLine.Thickness = 3;
	GyroYawLine.ThicknessMode = LINE_THICKNESS_MIDDLE;
	GyroYawLine.Color = COLOR_BLUE;
	GyroYawLine.BackgroundColor = COLOR_ACC_GYRO_BACKGROUND;

}

void doClearAcceleratorCompassScreen(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	clearDisplay(aValue);
	drawAccDemoGui();
}

void startAcceleratorCompass(void) {
	clearDisplay(COLOR_ACC_GYRO_BACKGROUND);
	TouchButtonClearScreen->setValue(COLOR_ACC_GYRO_BACKGROUND);

	TouchButtonAutorepeatAccScalePlus = TouchButtonAutorepeat::allocAndInitSimpleButton(
			3 * (BUTTON_WIDTH_6 + BUTTON_DEFAULT_SPACING), DISPLAY_HEIGHT - +BUTTON_DEFAULT_SPACING - 2 * BUTTON_HEIGHT_6,
			BUTTON_WIDTH_6, BUTTON_HEIGHT_6, COLOR_RED, StringPlus, 1, 1, &doChangeAccScale);
	TouchButtonAutorepeatAccScaleMinus = TouchButtonAutorepeat::allocAndInitSimpleButton(
			2 * (BUTTON_WIDTH_6 + BUTTON_DEFAULT_SPACING), DISPLAY_HEIGHT - +BUTTON_DEFAULT_SPACING - 2 * BUTTON_HEIGHT_6,
			BUTTON_WIDTH_6, BUTTON_HEIGHT_6, COLOR_RED, StringMinus, 1, -1, &doChangeAccScale);

	TouchButtonSetZero = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, DISPLAY_HEIGHT - BUTTON_HEIGHT_6,
			BUTTON_WIDTH_3, BUTTON_HEIGHT_6, COLOR_RED, StringZero, 2, 0, &doSetZero);

	// Clear button - lower left corner
	TouchButtonClearScreen = TouchButton::allocAndInitSimpleButton(0, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
			COLOR_RED, StringClear, 2, COLOR_BACKGROUND_DEFAULT, &doClearAcceleratorCompassScreen);

	drawAccDemoGui();
	AcceleratorScale = 9;
	MillisSinceLastAction = 0;

	SPI1_setPrescaler(SPI_BaudRatePrescaler_8 );
}

void loopAcceleratorGyroCompassDemo(void) {

	/* Wait 50 ms for data ready */
	uint32_t tMillis = getMillisSinceBoot();
	MillisSinceLastAction += tMillis - MillisLastLoop;
	MillisLastLoop = tMillis;
	if (MillisSinceLastAction > 50) {
		MillisSinceLastAction = 0;
		/**
		 * Accelerator data
		 */
		readAcceleratorZeroCompensated(RawDataBuffer);
		snprintf(StringBuffer, sizeof StringBuffer, "Accelerator X=%5d Y=%5d Z=%5d", RawDataBuffer[0], RawDataBuffer[1],
				RawDataBuffer[2]);
		drawText(TEXT_START_X, TEXT_START_Y, StringBuffer, 1, COLOR_BLACK, COLOR_GREEN);
		refreshVector(&AcceleratorLine, (RawDataBuffer[0] / AcceleratorScale), -(RawDataBuffer[1] / AcceleratorScale));
		drawPixel(AcceleratorLine.StartX, AcceleratorLine.StartY, COLOR_BLACK);

		/**
		 * Send over USB
		 */
		if (isUSBReady()) {
			HID_Buffer[1] = 0;
			HID_Buffer[2] = 0;
			/* LEFT Direction */
			if ((RawDataBuffer[0] / AcceleratorScale) < -2) {
				HID_Buffer[1] -= CURSOR_STEP;
			}
			/* RIGHT Direction */
			if ((RawDataBuffer[0] / AcceleratorScale) > 2) {
				HID_Buffer[1] += CURSOR_STEP;
			}
			/* UP Direction */
			if ((RawDataBuffer[1] / AcceleratorScale) < -2) {
				HID_Buffer[2] += CURSOR_STEP;
			}
			/* DOWN Direction */
			if ((RawDataBuffer[1] / AcceleratorScale) > 2) {
				HID_Buffer[2] -= CURSOR_STEP;
			}
			/* Update the cursor position */
			if ((HID_Buffer[1] != 0) || (HID_Buffer[2] != 0)) {
				/* Reset the control token to inform upper layer that a transfer is ongoing */
				PrevXferComplete = 0;

				/* Copy mouse position info in ENDP1 Tx Packet Memory Area*/
				USB_SIL_Write(EP1_IN, HID_Buffer, 4);

				/* Enable endpoint for transmission */
				SetEPTxValid(ENDP1 );
			}
		}

		/**
		 * Compass data
		 */
		readCompassRaw(RawDataBuffer);
		snprintf(StringBuffer, sizeof StringBuffer, "Compass X=%5d Y=%5d Z=%5d", RawDataBuffer[0], RawDataBuffer[1],
				RawDataBuffer[2]);
		drawText(TEXT_START_X, FONT_HEIGHT + TEXT_START_Y, StringBuffer, 1, COLOR_BLACK, COLOR_GREEN);
		// values can reach 800
		refreshVector(&CompassLine, (RawDataBuffer[0] >> 5), -(RawDataBuffer[2] >> 5));
		drawPixel(CompassLine.StartX, CompassLine.StartY, COLOR_RED);

		int16_t tTemp = readTempRaw();
		uint16_t tADCTemp = ADC1_getChannelValue(ADC_Channel_TempSensor );
		uint32_t tADCState = ADC_getCFGR();
		// 1.43V at 25C = 0x6EE  4.3mV/C => 344mV/80C
		snprintf(StringBuffer, sizeof StringBuffer, "%#x\xF8" "C %#x\xF8" "C 30\xF8=%#x 110\xF8=%#x", tTemp, tADCTemp,
				*((uint16_t*) 0x1FFFF7B8), *((uint16_t*) 0x1FFFF7C2));
		drawText(TEXT_START_X, 3 * FONT_HEIGHT + TEXT_START_Y, StringBuffer, 1, COLOR_BLACK, COLOR_GREEN);
		snprintf(StringBuffer, sizeof StringBuffer, "0x%4lX ", tADCState);
		drawText(TEXT_START_X, 4 * FONT_HEIGHT + TEXT_START_Y, StringBuffer, 1, COLOR_BLACK, COLOR_GREEN);

		/**
		 * Gyroscope data
		 */
		readGyroscopeZeroCompensated(RawDataBuffer);
		snprintf(StringBuffer, sizeof StringBuffer, "Gyroscope R=%5d P=%5d Y=%5d", RawDataBuffer[0], RawDataBuffer[1],
				RawDataBuffer[2]);
		drawText(TEXT_START_X, 2 * FONT_HEIGHT + TEXT_START_Y, StringBuffer, 1, COLOR_BLACK, COLOR_GREEN);

		TouchSliderHorizontal.setActualValueAndDrawBar((RawDataBuffer[0] / 4) + HORIZONTAL_SLIDER_NULL_VALUE);
		TouchSliderVertical.setActualValueAndDrawBar((RawDataBuffer[1] / 4) + VERTICAL_SLIDER_NULL_VALUE);

		refreshVector(&GyroYawLine, -RawDataBuffer[2] / 8, -(2 * COMPASS_RADIUS));
		drawPixel(GyroYawLine.StartX, GyroYawLine.StartY, COLOR_RED);
//	TouchSliderHorizontal.setActualValue(RawDataBuffer[0]);
//	TouchSliderHorizontal.printValue();
//	TouchSliderVertical.setActualValue(RawDataBuffer[1]);
//	TouchSliderVertical.printValue();
//	TouchSliderVertical2.setActualValue(RawDataBuffer[2]);
//	TouchSliderVertical2.printValue();
	}
}

void stopAcceleratorCompass(void) {
	TouchButtonSetZero->setFree();
	TouchButtonClearScreen->setFree();
	TouchButtonAutorepeatAccScalePlus->setFree();
	TouchButtonAutorepeatAccScaleMinus->setFree();
}

void doChangeAccScale(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	AcceleratorScale += aValue;
	snprintf(StringBuffer, sizeof StringBuffer, "Scale=%2u", AcceleratorScale);
	drawText(TouchButtonAutorepeatAccScaleMinus->getPositionX(), DISPLAY_HEIGHT - (BUTTON_HEIGHT_6 + FONT_HEIGHT), StringBuffer, 1,
	COLOR_BLUE, COLOR_GREEN);
}

void readCompassRaw(int16_t* aCompassRawData) {
	uint8_t ctrlx[2];
	uint8_t cDivider;
	uint8_t i = 0;

	/* Read the register content */
	LSM303DLHC_Read(ACC_I2C_ADDRESS, LSM303DLHC_CTRL_REG4_A, ctrlx, 2);
	LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_X_H_M, (uint8_t *) aCompassRawData, 6);

	if (ctrlx[1] & 0x40)
		// High resolution mode
		cDivider = 64;
	else
		cDivider = 16;

	for (i = 0; i < 3; i++) {
		aCompassRawData[i] = aCompassRawData[i] / cDivider;
	}
}

/**
 * @brief Read LSM303DLHC output register, and calculate the acceleration ACC=(1/SENSITIVITY)* (out_h*256+out_l)/16 (12 bit rappresentation)
 * @param aAcceleratorRawData: pointer to float buffer where to store data
 * @retval None
 */
void readAcceleratorRaw(int16_t* aAcceleratorRawData) {
	uint8_t ctrlx[2];
	uint8_t cDivider;
	uint8_t i = 0;

	/* Read the register content */
	LSM303DLHC_Read(ACC_I2C_ADDRESS, LSM303DLHC_CTRL_REG4_A, ctrlx, 2);
	LSM303DLHC_Read(ACC_I2C_ADDRESS, LSM303DLHC_OUT_X_L_A, (uint8_t *) aAcceleratorRawData, 6);

	if (ctrlx[1] & 0x40)
		// High resolution mode
		cDivider = 64;
	else
		cDivider = 16;

	for (i = 0; i < 3; i++) {
		aAcceleratorRawData[i] = aAcceleratorRawData[i] / cDivider;
	}
	// invert X value, it fits better :-)
	aAcceleratorRawData[0] = -aAcceleratorRawData[0];
}

void readAcceleratorZeroCompensated(int16_t* aAcceleratorRawData) {
	readAcceleratorRaw(aAcceleratorRawData);
	aAcceleratorRawData[0] -= AcceleratorZeroCompensation[0];
	aAcceleratorRawData[1] -= AcceleratorZeroCompensation[1];
	aAcceleratorRawData[2] -= AcceleratorZeroCompensation[2];
}

void setZeroAcceleratorGyroValue(void) {
	// use oversampling
	AcceleratorZeroCompensation[0] = 0;
	AcceleratorZeroCompensation[1] = 0;
	AcceleratorZeroCompensation[2] = 0;
	GyroscopeZeroCompensation[0] = 0;
	GyroscopeZeroCompensation[1] = 0;
	GyroscopeZeroCompensation[2] = 0;
	for (int i = 0; i < LSM303DLHC_CALIBRATION_OVERSAMPLING; ++i) {
		readAcceleratorRaw(RawDataBuffer);
		AcceleratorZeroCompensation[0] += RawDataBuffer[0];
		AcceleratorZeroCompensation[1] += RawDataBuffer[1];
		AcceleratorZeroCompensation[2] += RawDataBuffer[2];
		readGyroscopeRaw(RawDataBuffer);
		GyroscopeZeroCompensation[0] += RawDataBuffer[0];
		GyroscopeZeroCompensation[1] += RawDataBuffer[1];
		GyroscopeZeroCompensation[2] += RawDataBuffer[2];
		delayMillis(10);
	}
	AcceleratorZeroCompensation[0] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
	AcceleratorZeroCompensation[1] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
	AcceleratorZeroCompensation[2] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
	GyroscopeZeroCompensation[0] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
	GyroscopeZeroCompensation[1] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
	GyroscopeZeroCompensation[2] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
}

void readGyroscopeRaw(int16_t* aGyroscopeRawData) {
	L3GD20_Read((uint8_t *) aGyroscopeRawData, L3GD20_OUT_X_L_ADDR, 6);
}

void readGyroscopeZeroCompensated(int16_t* aGyroscopeRawData) {
	readGyroscopeRaw(aGyroscopeRawData);
	aGyroscopeRawData[0] -= GyroscopeZeroCompensation[0];
	aGyroscopeRawData[1] -= GyroscopeZeroCompensation[1];
	aGyroscopeRawData[2] -= GyroscopeZeroCompensation[2];
}

void doSetZero(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	int16_t tBufferRaw[3];
	// wait for end of touch vibration
	delayMillis(300);
	setZeroAcceleratorGyroValue();
	readGyroscopeRaw(tBufferRaw);
}

/**
 * @brief  calculate the magnetic field Magn.
 * @param  pfData: pointer to the data out
 * @retval None
 */
void Demo_CompassReadMag(float* pfData) {
	static uint8_t buffer[6] = { 0 };
	uint8_t CTRLB = 0;
	uint16_t Magn_Sensitivity_XY = 0, Magn_Sensitivity_Z = 0;
	uint8_t i = 0;
	LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_CRB_REG_M, &CTRLB, 1);

	LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_X_H_M, buffer, 1);
	LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_X_L_M, buffer + 1, 1);
	LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_Y_H_M, buffer + 2, 1);
	LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_Y_L_M, buffer + 3, 1);
	LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_Z_H_M, buffer + 4, 1);
	LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_Z_L_M, buffer + 5, 1);
	/* Switch the sensitivity set in the CRTLB*/
	switch (CTRLB & 0xE0) {
	case LSM303DLHC_FS_1_3_GA :
		Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_1_3Ga;
		Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_1_3Ga;
		break;
	case LSM303DLHC_FS_1_9_GA :
		Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_1_9Ga;
		Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_1_9Ga;
		break;
	case LSM303DLHC_FS_2_5_GA :
		Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_2_5Ga;
		Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_2_5Ga;
		break;
	case LSM303DLHC_FS_4_0_GA :
		Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_4Ga;
		Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_4Ga;
		break;
	case LSM303DLHC_FS_4_7_GA :
		Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_4_7Ga;
		Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_4_7Ga;
		break;
	case LSM303DLHC_FS_5_6_GA :
		Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_5_6Ga;
		Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_5_6Ga;
		break;
	case LSM303DLHC_FS_8_1_GA :
		Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_8_1Ga;
		Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_8_1Ga;
		break;
	}

	for (i = 0; i < 2; i++) {
		pfData[i] = (float) ((int16_t) (((uint16_t) buffer[2 * i] << 8) + buffer[2 * i + 1]) * 1000) / Magn_Sensitivity_XY;
	}
	pfData[2] = (float) ((int16_t) (((uint16_t) buffer[4] << 8) + buffer[5]) * 1000) / Magn_Sensitivity_Z;
}

int16_t readTempRaw(void) {
	uint8_t ctrlx[2];

	/* Read the register content */
	LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_TEMP_OUT_H_M, ctrlx, 2);

	return ctrlx[0] << 4 | ctrlx[1] >> 4;
//	return ctrlx[0] << 8 | ctrlx[1];
}

/**
 * @brief Read LSM303DLHC output register, and calculate the acceleration ACC=(1/SENSITIVITY)* (out_h*256+out_l)/16 (12 bit rappresentation)
 * @param pfData: pointer to float buffer where to store data
 * @retval None
 */
void Demo_CompassReadAcc(float* pfData) {
	int16_t pnRawData[3];
	uint8_t ctrlx[2];
	uint8_t buffer[6], cDivider;
	uint8_t i = 0;
	float LSM_Acc_Sensitivity = LSM_Acc_Sensitivity_2g;

	/* Read the register content */
	LSM303DLHC_Read(ACC_I2C_ADDRESS, LSM303DLHC_CTRL_REG4_A, ctrlx, 2);
	LSM303DLHC_Read(ACC_I2C_ADDRESS, LSM303DLHC_OUT_X_L_A, buffer, 6);

	if (ctrlx[1] & 0x40)
		cDivider = 64;
	else
		cDivider = 16;

	/* check in the control register4 the data alignment*/
	if (!(ctrlx[0] & 0x40) || (ctrlx[1] & 0x40)) /* Little Endian Mode or FIFO mode */
	{
		for (i = 0; i < 3; i++) {
			pnRawData[i] = ((int16_t) ((uint16_t) buffer[2 * i + 1] << 8) + buffer[2 * i]) / cDivider;
		}
	} else /* Big Endian Mode */
	{
		for (i = 0; i < 3; i++)
			pnRawData[i] = ((int16_t) ((uint16_t) buffer[2 * i] << 8) + buffer[2 * i + 1]) / cDivider;
	}
	/* Read the register content */
	LSM303DLHC_Read(ACC_I2C_ADDRESS, LSM303DLHC_CTRL_REG4_A, ctrlx, 2);

	if (ctrlx[1] & 0x40) {
		/* FIFO mode */
		LSM_Acc_Sensitivity = 0.25;
	} else {
		/* normal mode */
		/* switch the sensitivity value set in the CRTL4*/
		switch (ctrlx[0] & 0x30) {
		case LSM303DLHC_FULLSCALE_2G :
			LSM_Acc_Sensitivity = LSM_Acc_Sensitivity_2g;
			break;
		case LSM303DLHC_FULLSCALE_4G :
			LSM_Acc_Sensitivity = LSM_Acc_Sensitivity_4g;
			break;
		case LSM303DLHC_FULLSCALE_8G :
			LSM_Acc_Sensitivity = LSM_Acc_Sensitivity_8g;
			break;
		case LSM303DLHC_FULLSCALE_16G :
			LSM_Acc_Sensitivity = LSM_Acc_Sensitivity_16g;
			break;
		}
	}

	/* Obtain the mg value for the three axis */
	for (i = 0; i < 3; i++) {
		pfData[i] = (float) pnRawData[i] / LSM_Acc_Sensitivity;
	}
}

void DemoPitchRoll(void) {
	uint8_t i = 0;
	float AccBuffer[3] = { 0.0f };
	float fNormAcc, fSinRoll, fCosRoll, fSinPitch, fCosPitch = 0.0f, RollAng = 0.0f, PitchAng = 0.0f;

	/* Wait for data ready */
	delayMillis(50);

	/* Read Accelerometer data */
	Demo_CompassReadAcc(AccBuffer);

	for (i = 0; i < 3; i++)
		AccBuffer[i] /= 100.0f;

	fNormAcc = sqrt((AccBuffer[0] * AccBuffer[0]) + (AccBuffer[1] * AccBuffer[1]) + (AccBuffer[2] * AccBuffer[2]));

	fSinRoll = -AccBuffer[1] / fNormAcc;
	fCosRoll = sqrt(1.0 - (fSinRoll * fSinRoll));
	fSinPitch = AccBuffer[0] / fNormAcc;
	fCosPitch = sqrt(1.0 - (fSinPitch * fSinPitch));
	if (fSinRoll > 0) {
		if (fCosRoll > 0) {
			RollAng = acos(fCosRoll) * 180 / PI;
		} else {
			RollAng = acos(fCosRoll) * 180 / PI + 180;
		}
	} else {
		if (fCosRoll > 0) {
			RollAng = acos(fCosRoll) * 180 / PI + 360;
		} else {
			RollAng = acos(fCosRoll) * 180 / PI + 180;
		}
	}

	if (fSinPitch > 0) {
		if (fCosPitch > 0) {
			PitchAng = acos(fCosPitch) * 180 / PI;
		} else {
			PitchAng = acos(fCosPitch) * 180 / PI + 180;
		}
	} else {
		if (fCosPitch > 0) {
			PitchAng = acos(fCosPitch) * 180 / PI + 360;
		} else {
			PitchAng = acos(fCosPitch) * 180 / PI + 180;
		}
	}

	if (RollAng >= 360) {
		RollAng = RollAng - 360;
	}

	if (PitchAng >= 360) {
		PitchAng = PitchAng - 360;
	}

	allLedsOff();
	if (fSinRoll > 0.2f) {
		STM_EVAL_LEDOn(LED7);
	} else if (fSinRoll < -0.2f) {
		STM_EVAL_LEDOn(LED6);
	}
	if (fSinPitch > 0.2f) {
		STM_EVAL_LEDOn(LED3);
	} else if (fSinPitch < -0.2f) {
		STM_EVAL_LEDOn(LED10);
	}
	CheckTouchGeneric(true);
}

