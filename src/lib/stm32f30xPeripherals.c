/**
 * @file stm32f30xPeripherals.c
 *
 * @date 05.12.2012
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 */
#include "stm32f30xPeripherals.h"
#include "stm32f3_discovery_lsm303dlhc.h"
#include "stm32f3_discovery_l3gd20.h"
#include "timing.h"
#include "stm32f30x_tim.h"
#include "stm32f3_discovery.h"
#include <stdio.h>

/** @addtogroup Peripherals_Library
 * @{
 */

/**
 *  Calibrating with value from SystemMemory:
 *
 *    ActualRefVoltageADC = ((CalValueForRefint * 3.3V) / ActualReadingRefint)
 *
 *          3V                 Reading 3V                         3.0V * 4096 * ActualReadingRefint
 *  -------------------  = --------------------- => Reading 3V = -------------------------------------
 *  ActualRefVoltageADC     Reading VRef (4096)                   CalValueForRefint * 3.3V
 *
 *  Formula for ScaleFactor (3V for 240 Pixel) is:
 *    Reading 3V * ScaleFactor = 240 (DISPLAY_HEIGHT)
 *
 *                       240  * 3.3V * CalValueForRefint
 *  => ADCScaleFactor = ---------------------------------
 *                       4096 * 3.0V * ActualReadingRefint
 *
 *                    CalValueForRefint
 *   = 0,064453125 * -------------------
 *                    ActualReadingRefint
 *
 *   0,064453125 * 2**20(=1048576) = 67584.0
 *
 */

float ADCToVoltFactor; // Factor for ADC Reading -> Volt = sVrefVoltage / 4096
uint16_t sReading3Volt; // Raw value for 3V input
unsigned int sADCScaleFactorShift18; // Factor for ADC Reading -> Display for 0,5 Volt/div(=40pixel) scale - shift 18 because we have 12 bit ADC and 12 + 18 + sign + buffer = 32 bit register
float sVrefVoltage;

/**
 * Init the HY32D CS, Control/Data, WR, RD, and port D data pins
 */
void HY32D_IO_initalize(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable the GPIO Clocks */
	RCC_AHBPeriphClockCmd(HY32D_CS_GPIO_CLK, ENABLE);
	RCC_AHBPeriphClockCmd(HY32D_DATA_GPIO_CLK, ENABLE);

	// CS pin
	GPIO_InitStructure.GPIO_Pin = HY32D_CS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(HY32D_CS_GPIO_PORT, &GPIO_InitStructure);
	// set HIGH
	GPIO_SetBits(HY32D_CS_GPIO_PORT, HY32D_CS_PIN);

	// Register/Data select pin
	GPIO_InitStructure.GPIO_Pin = HY32D_DATA_CONTROL_PIN;
	GPIO_Init(HY32D_DATA_CONTROL_GPIO_PORT, &GPIO_InitStructure);

	// WR pin
	GPIO_InitStructure.GPIO_Pin = HY32D_WR_PIN;
	GPIO_Init(HY32D_WR_GPIO_PORT, &GPIO_InitStructure);
	// set HIGH
	GPIO_SetBits(HY32D_WR_GPIO_PORT, HY32D_WR_PIN);

	// RD pin
	GPIO_InitStructure.GPIO_Pin = HY32D_RD_PIN;
	GPIO_Init(HY32D_RD_GPIO_PORT, &GPIO_InitStructure);
	// set HIGH
	GPIO_SetBits(HY32D_RD_GPIO_PORT, HY32D_RD_PIN);

	// 16 data pins
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
	GPIO_Init(HY32D_DATA_GPIO_PORT, &GPIO_InitStructure);

}

/**
 * Pins for touchpanel CS, PENINT
 */
#define ADS7846_CS_PIN                           GPIO_Pin_2
#define ADS7846_CS_GPIO_PORT                     GPIOB
#define ADS7846_CS_GPIO_CLK                      RCC_AHBPeriph_GPIOB

// Interrupt in pin
#define ADS7846_EXTI_PIN                         GPIO_Pin_1
#define ADS7846_EXTI_GPIO_PORT                   GPIOB
#define ADS7846_EXTI_GPIO_CLK                    RCC_AHBPeriph_GPIOB
#define ADS7846_EXTI_PORT_SOURCE                 EXTI_PortSourceGPIOB
#define ADS7846_EXTI_PIN_SOURCE                  EXTI_PinSource1
#define ADS7846_EXTI_LINE                        EXTI_Line1
#define ADS7846_EXTI_IRQn                        EXTI1_IRQn

void ADS7846_IO_initalize(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;

	/* Enable the GPIO Clock */
	RCC_AHBPeriphClockCmd(ADS7846_CS_GPIO_CLK, ENABLE);

	// CS pin
	GPIO_InitStructure.GPIO_Pin = ADS7846_CS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(ADS7846_CS_GPIO_PORT, &GPIO_InitStructure);
	// set HIGH
	GPIO_SetBits(ADS7846_CS_GPIO_PORT, ADS7846_CS_PIN);

	/* Enable the Clock */
	RCC_AHBPeriphClockCmd(ADS7846_EXTI_GPIO_CLK, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	/* Configure pin as input */
	GPIO_InitStructure.GPIO_Pin = ADS7846_EXTI_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(ADS7846_EXTI_GPIO_PORT, &GPIO_InitStructure);

	/* Connect EXTI Line to GPIO Pin */
	SYSCFG_EXTILineConfig(ADS7846_EXTI_PORT_SOURCE, ADS7846_EXTI_PIN_SOURCE);

	/* Configure EXTI line */
	EXTI_InitStructure.EXTI_Line = ADS7846_EXTI_LINE;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	/* Enable and set EXTI Interrupt to the lowest priority */
	NVIC_InitStructure.NVIC_IRQChannel = ADS7846_EXTI_IRQn;
	// lowest possible priority
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/**
 * @return true if line is NOT active
 */
inline bool ADS7846_getInteruptLineLevel(void) {
	return GPIO_ReadInputDataBit(ADS7846_EXTI_GPIO_PORT, ADS7846_EXTI_PIN);
}

inline void ADS7846_ClearITPendingBit(void) {
	EXTI_ClearITPendingBit(ADS7846_EXTI_LINE);
}
/**
 * first clear pending interrupts
 */
void ADS7846_clearAndEnableInterrupt(void) {
//	// Enable  interrupt
//	uint32_t tmp = (uint32_t) EXTI_BASE + EXTI_Mode_Interrupt
//			+ (((STM32F3D_ADS7846_EXTI_LINE) >> 5) * 0x20);
//	/* Enable the selected external lines */
//	*(__IO uint32_t *) tmp |= (uint32_t) (1 << (STM32F3D_ADS7846_EXTI_LINE & 0x1F));

	EXTI_ClearITPendingBit(ADS7846_EXTI_LINE);
	NVIC_ClearPendingIRQ(ADS7846_EXTI_IRQn);
	NVIC_EnableIRQ(ADS7846_EXTI_IRQn);
}

void ADS7846_disableInterrupt(void) {
//	// Disable  interrupt
//	uint32_t tmp = (uint32_t) EXTI_BASE + EXTI_Mode_Interrupt
//			+ (((STM32F3D_ADS7846_EXTI_LINE) >> 5) * 0x20);
//	/* Disable the selected external lines */
//	*(__IO uint32_t *) tmp &= ~(uint32_t) (1 << (STM32F3D_ADS7846_EXTI_LINE & 0x1F));
//

	EXTI_ClearITPendingBit(ADS7846_EXTI_LINE);
	// Disable NVIC Int
	NVIC_DisableIRQ(ADS7846_EXTI_IRQn);
	NVIC_ClearPendingIRQ(ADS7846_EXTI_IRQn);
}

inline void ADS7846_CSEnable(void) {
	GPIO_ResetBits(ADS7846_CS_GPIO_PORT, ADS7846_CS_PIN);
}

inline void ADS7846_CSDisable(void) {
	GPIO_SetBits(ADS7846_CS_GPIO_PORT, ADS7846_CS_PIN);
}

/**
 *  2 pins for AccuCapacity relays
 */
#define ACCUCAP_DISCHARGE_1_PIN     GPIO_Pin_9
#define ACCUCAP_DISCHARGE_2_PIN     GPIO_Pin_10
#define ACCUCAP_CHARGE_1_PIN        GPIO_Pin_2
#define ACCUCAP_CHARGE_2_PIN        GPIO_Pin_4
#define ACCUCAP_GPIO_PORT           GPIOF
#define ACCUCAP_GPIO_CLK            RCC_AHBPeriph_GPIOF

void AccuCapacity_IO_initalize(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable the GPIO Clock */
	RCC_AHBPeriphClockCmd(ACCUCAP_GPIO_CLK, ENABLE);

	// Discharge 1 pin
	GPIO_InitStructure.GPIO_Pin = ACCUCAP_DISCHARGE_1_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(ACCUCAP_GPIO_PORT, &GPIO_InitStructure);
	// set LOW
	GPIO_ResetBits(ACCUCAP_GPIO_PORT, ACCUCAP_DISCHARGE_1_PIN);

	// Discharge 2 pin
	GPIO_InitStructure.GPIO_Pin = ACCUCAP_DISCHARGE_2_PIN;
	GPIO_Init(ACCUCAP_GPIO_PORT, &GPIO_InitStructure);
	// set LOW
	GPIO_ResetBits(ACCUCAP_GPIO_PORT, ACCUCAP_DISCHARGE_2_PIN);

	// Charge 1 pin
	GPIO_InitStructure.GPIO_Pin = ACCUCAP_CHARGE_1_PIN;
	GPIO_Init(ACCUCAP_GPIO_PORT, &GPIO_InitStructure);
	// set LOW
	GPIO_ResetBits(ACCUCAP_GPIO_PORT, ACCUCAP_CHARGE_1_PIN);

	// Charge 2 pin
	GPIO_InitStructure.GPIO_Pin = ACCUCAP_CHARGE_2_PIN;
	GPIO_Init(ACCUCAP_GPIO_PORT, &GPIO_InitStructure);
	// set LOW
	GPIO_ResetBits(ACCUCAP_GPIO_PORT, ACCUCAP_CHARGE_2_PIN);

}

void AccuCapacity_SwitchDischarge(unsigned int aChannelIndex, bool aActive) {
	uint16_t tPinNr = ACCUCAP_DISCHARGE_2_PIN;
	if (aChannelIndex != 0) {
		tPinNr = ACCUCAP_DISCHARGE_1_PIN;
	}
	if (aActive) {
		ACCUCAP_GPIO_PORT->BSRR = tPinNr;
	} else {
		// Low
		ACCUCAP_GPIO_PORT->BRR = tPinNr;
	}
}

void AccuCapacity_SwitchCharge(unsigned int aChannelIndex, bool aActive) {
	uint16_t tPinNr = ACCUCAP_CHARGE_2_PIN;
	if (aChannelIndex != 0) {
		tPinNr = ACCUCAP_CHARGE_1_PIN;
	}
	if (aActive) {
		ACCUCAP_GPIO_PORT->BSRR = tPinNr;
	} else {
		// Low
		ACCUCAP_GPIO_PORT->BRR = tPinNr;
	}
}

/**
 * ADC2 for AccuCapacity
 */
#define ADC2_PERIPH_CLOCK                   	RCC_AHBPeriph_ADC12

// Two input pins
#define ADC2_INPUT1_CHANNEL                     ADC_Channel_6
#define ADC2_INPUT1_PIN                         GPIO_Pin_0
#define ADC2_INPUT1_GPIO_PORT                   GPIOC
#define ADC2_INPUT1_GPIO_CLK                    RCC_AHBPeriph_GPIOC

#define ADC2_INPUT2_CHANNEL                     ADC_Channel_7
#define ADC2_INPUT2_PIN                         GPIO_Pin_1
#define ADC2_INPUT2_GPIO_PORT                   GPIOC
#define ADC2_INPUT2_GPIO_CLK                    RCC_AHBPeriph_GPIOC

void ADC2_init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;

	/* Enable ADC clock */
	RCC_AHBPeriphClockCmd(ADC2_PERIPH_CLOCK, ENABLE);

	/**
	 * ADC GPIO input pin init
	 */
	RCC_AHBPeriphClockCmd(ADC2_INPUT1_GPIO_CLK, ENABLE);
	/* Configure pin as analog input */
	GPIO_InitStructure.GPIO_Pin = ADC2_INPUT1_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(ADC2_INPUT1_GPIO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = ADC2_INPUT2_PIN;
	GPIO_Init(ADC2_INPUT2_GPIO_PORT, &GPIO_InitStructure);

	/**
	 * Enable special channels
	 */
	ADC_VrefintCmd(ADC2, ENABLE);

	/**
	 *  Calibration procedure
	 */
	ADC_VoltageRegulatorCmd(ADC2, ENABLE);
	/* To settle reference voltage etc. */
	delayMillis(2);
	ADC_SelectCalibrationMode(ADC2, ADC_CalibrationMode_Single);
	ADC_StartCalibration(ADC2);
	while (ADC_GetCalibrationStatus(ADC2) != RESET) {
		;
	}

	// use defaults
	ADC_CommonStructInit(&ADC_CommonInitStructure);
	ADC_CommonInit(ADC2, &ADC_CommonInitStructure);

	// use defaults
	ADC_StructInit(&ADC_InitStructure);
	ADC_Init(ADC2, &ADC_InitStructure);

	/* Enable ADC */
	ADC_Cmd(ADC2, ENABLE);

	/* wait for ADRDY */
	setTimeoutMillis(20);
	while (!ADC_GetFlagStatus(ADC2, ADC_FLAG_RDY)) {
		if (isTimeout(ADC2->ISR)) {
			break;
		}
	}
	// set actual multiplier etc. by reading the ref voltage
	ADC_setRawToVoltFactor();
}

unsigned int AccuCapacity_ADCRead(uint8_t aProbeNumber) {
	if (aProbeNumber == 0) {
		ADC_RegularChannelConfig(ADC2, ADC2_INPUT1_CHANNEL, 1, ADC_SampleTime_61Cycles5);
	} else {
		ADC_RegularChannelConfig(ADC2, ADC2_INPUT2_CHANNEL, 1, ADC_SampleTime_61Cycles5);
	}
	ADC_StartConversion(ADC2);
	/* Test EOC flag */
	setTimeoutMillis(ADC_DEFAULT_TIMEOUT);
	while (ADC_GetFlagStatus(ADC2, ADC_FLAG_EOC) == RESET) {
		if (isTimeout(ADC2->ISR)) {
			break;
		}
	}
	return ADC_GetConversionValue(ADC2);
}

void Gyroscope_IO_initialize(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Configure GPIO PIN for Lis Chip select */
	GPIO_InitStructure.GPIO_Pin = L3GD20_SPI_CS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(L3GD20_SPI_CS_GPIO_PORT, &GPIO_InitStructure);

	/* Deselect : Chip Select high */
	GPIO_SetBits(L3GD20_SPI_CS_GPIO_PORT, L3GD20_SPI_CS_PIN);

	/* Configure GPIO PINs to detect Interrupts */
	GPIO_InitStructure.GPIO_Pin = L3GD20_SPI_INT1_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(L3GD20_SPI_INT1_GPIO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = L3GD20_SPI_INT2_PIN;
	GPIO_Init(L3GD20_SPI_INT2_GPIO_PORT, &GPIO_InitStructure);
}

/**
 * @brief  Configure the Mems L3GD20.
 */
void GyroscopeAndSPIInitialize(void) {
	L3GD20_InitTypeDef L3GD20_InitStructure;
	L3GD20_FilterConfigTypeDef L3GD20_FilterStructure;

	/* Configure Mems L3GD20 */
	L3GD20_InitStructure.Power_Mode = L3GD20_MODE_ACTIVE;
	L3GD20_InitStructure.Output_DataRate = L3GD20_OUTPUT_DATARATE_1;
	L3GD20_InitStructure.Axes_Enable = L3GD20_AXES_ENABLE;
	L3GD20_InitStructure.Band_Width = L3GD20_BANDWIDTH_4;
	L3GD20_InitStructure.BlockData_Update = L3GD20_BlockDataUpdate_Continous;
	L3GD20_InitStructure.Endianness = L3GD20_BLE_LSB;
	L3GD20_InitStructure.Full_Scale = L3GD20_FULLSCALE_500;
	L3GD20_Init(&L3GD20_InitStructure);

	L3GD20_FilterStructure.HighPassFilter_Mode_Selection = L3GD20_HPM_NORMAL_MODE_RES;
	L3GD20_FilterStructure.HighPassFilter_CutOff_Frequency = L3GD20_HPFCF_0;
	L3GD20_FilterConfig(&L3GD20_FilterStructure);

	L3GD20_FilterCmd(L3GD20_HIGHPASSFILTER_ENABLE);
}

/**
 * @brief  Configure the Mems to compass application.
 */
void initAcceleratorCompassChip(void) {
	SPI1_setPrescaler(SPI_BaudRatePrescaler_8);
	LSM303DLHCMag_InitTypeDef LSM303DLHC_InitStructure;
	LSM303DLHCAcc_InitTypeDef LSM303DLHCAcc_InitStructure;
	LSM303DLHCAcc_FilterConfigTypeDef LSM303DLHCFilter_InitStructure;

	/* Configure MEMS magnetometer main parameters: temp, working mode, full Scale and Data rate */
	LSM303DLHC_InitStructure.Temperature_Sensor = LSM303DLHC_TEMPSENSOR_ENABLE;
	LSM303DLHC_InitStructure.MagOutput_DataRate = LSM303DLHC_ODR_30_HZ;
	LSM303DLHC_InitStructure.MagFull_Scale = LSM303DLHC_FS_8_1_GA;
	LSM303DLHC_InitStructure.Working_Mode = LSM303DLHC_CONTINUOS_CONVERSION;
	LSM303DLHC_MagInit(&LSM303DLHC_InitStructure);

	/* Fill the accelerometer structure */
	LSM303DLHCAcc_InitStructure.Power_Mode = LSM303DLHC_NORMAL_MODE;
	LSM303DLHCAcc_InitStructure.AccOutput_DataRate = LSM303DLHC_ODR_50_HZ;
	LSM303DLHCAcc_InitStructure.Axes_Enable = LSM303DLHC_AXES_ENABLE;
	LSM303DLHCAcc_InitStructure.AccFull_Scale = LSM303DLHC_FULLSCALE_2G;
	LSM303DLHCAcc_InitStructure.BlockData_Update = LSM303DLHC_BlockUpdate_Continous;
	LSM303DLHCAcc_InitStructure.Endianness = LSM303DLHC_BLE_LSB;
	LSM303DLHCAcc_InitStructure.High_Resolution = LSM303DLHC_HR_ENABLE;
	/* Configure the accelerometer main parameters */
	LSM303DLHC_AccInit(&LSM303DLHCAcc_InitStructure);

	/* Fill the accelerometer LPF structure */
	LSM303DLHCFilter_InitStructure.HighPassFilter_Mode_Selection = LSM303DLHC_HPM_NORMAL_MODE;
	LSM303DLHCFilter_InitStructure.HighPassFilter_CutOff_Frequency = LSM303DLHC_HPFCF_16;
	LSM303DLHCFilter_InitStructure.HighPassFilter_AOI1 = LSM303DLHC_HPF_AOI1_DISABLE;
	LSM303DLHCFilter_InitStructure.HighPassFilter_AOI2 = LSM303DLHC_HPF_AOI2_DISABLE;

	/* Configure the accelerometer LPF main parameters */
	LSM303DLHC_AccFilterConfig(&LSM303DLHCFilter_InitStructure);
}
/**
 * init  STMF32F3 SPI_1 Interface pins CLK, MISO, MOSI
 */
#define STM32F3D_SPI1_CLK                   RCC_APB2Periph_SPI1

#define STM32F3D_SPI1_SCK_PIN               GPIO_Pin_5                  /* PA.05 */
#define STM32F3D_SPI1_SCK_GPIO_PORT         GPIOA                       /* GPIOA */
#define STM32F3D_SPI1_SCK_GPIO_CLK          RCC_AHBPeriph_GPIOA
#define STM32F3D_SPI1_SCK_SOURCE            GPIO_PinSource5
#define STM32F3D_SPI1_SCK_AF                GPIO_AF_5

#define STM32F3D_SPI1_MISO_PIN              GPIO_Pin_6                  /* PA.6 */
#define STM32F3D_SPI1_MISO_GPIO_PORT        GPIOA                       /* GPIOA */
#define STM32F3D_SPI1_MISO_GPIO_CLK         RCC_AHBPeriph_GPIOA
#define STM32F3D_SPI1_MISO_SOURCE           GPIO_PinSource6
#define STM32F3D_SPI1_MISO_AF               GPIO_AF_5

#define STM32F3D_SPI1_MOSI_PIN              GPIO_Pin_7                  /* PA.7 */
#define STM32F3D_SPI1_MOSI_GPIO_PORT        GPIOA                       /* GPIOA */
#define STM32F3D_SPI1_MOSI_GPIO_CLK         RCC_AHBPeriph_GPIOA
#define STM32F3D_SPI1_MOSI_SOURCE           GPIO_PinSource7
#define STM32F3D_SPI1_MOSI_AF               GPIO_AF_5

/**
 * init with prescaler 8
 */
uint16_t actualPrescaler;
void SPI1_initialize(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef SPI_InitStructure;

	/* Enable the SPI peripheral */
	RCC_APB2PeriphClockCmd(STM32F3D_SPI1_CLK, ENABLE);

	/* Enable SCK, MOSI and MISO GPIO clocks */
	RCC_AHBPeriphClockCmd(STM32F3D_SPI1_SCK_GPIO_CLK | STM32F3D_SPI1_MISO_GPIO_CLK | STM32F3D_SPI1_MOSI_GPIO_CLK, ENABLE);

	GPIO_PinAFConfig(STM32F3D_SPI1_SCK_GPIO_PORT, STM32F3D_SPI1_SCK_SOURCE, STM32F3D_SPI1_SCK_AF);
	GPIO_PinAFConfig(STM32F3D_SPI1_MISO_GPIO_PORT, STM32F3D_SPI1_MISO_SOURCE, STM32F3D_SPI1_MISO_AF);
	GPIO_PinAFConfig(STM32F3D_SPI1_MOSI_GPIO_PORT, STM32F3D_SPI1_MOSI_SOURCE, STM32F3D_SPI1_MOSI_AF);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; //GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	/* SPI SCK pin configuration */
	GPIO_InitStructure.GPIO_Pin = STM32F3D_SPI1_SCK_PIN;
	GPIO_Init(STM32F3D_SPI1_SCK_GPIO_PORT, &GPIO_InitStructure);

	/* SPI  MOSI pin configuration */
	GPIO_InitStructure.GPIO_Pin = STM32F3D_SPI1_MOSI_PIN;
	GPIO_Init(STM32F3D_SPI1_MOSI_GPIO_PORT, &GPIO_InitStructure);

	/* SPI MISO pin configuration */
	GPIO_InitStructure.GPIO_Pin = STM32F3D_SPI1_MISO_PIN;
	GPIO_Init(STM32F3D_SPI1_MISO_GPIO_PORT, &GPIO_InitStructure);

	/* SPI configuration -------------------------------------------------------*/
	SPI_I2S_DeInit(SPI1);
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	// SPI maximum speed
	actualPrescaler = SPI_BaudRatePrescaler_8;
	SPI_InitStructure.SPI_BaudRatePrescaler = actualPrescaler;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_Init(SPI1, &SPI_InitStructure);

	/* Configure the RX FIFO Threshold */
	SPI_RxFIFOThresholdConfig(SPI1, SPI_RxFIFOThreshold_QF);
	/* Enable SPI1  */
	SPI_Cmd(SPI1, ENABLE);
}

void SPI1_setPrescaler(uint16_t aPrescaler) {
	// SPI_BaudRatePrescaler_256 is also prescaler mask
	if (aPrescaler == (aPrescaler & SPI_BaudRatePrescaler_256) || aPrescaler == 0) {
		uint16_t tmpreg = SPI1->CR1;
		/* Clear BR[2:0] bits (Baud Rate Control)*/
		tmpreg &= ~SPI_CR1_BR;
		/* Configure SPIx: Data Size */
		tmpreg |= aPrescaler;
		SPI1->CR1 = tmpreg;
		actualPrescaler = aPrescaler;
	} else {
		assert_failed((uint8_t *) __FILE__, __LINE__);
	}
}

inline uint16_t SPI1_getPrescaler(void) {
	return actualPrescaler;
}

/**
 * @brief  Sends a Byte through the SPI interface and return the Byte received
 *         from the SPI bus.
 * @param  Byte : Byte send.
 * @retval The received byte value
 */
#define SPI_TIMEOUT             0x04 // 4 ms
uint8_t SPI1_sendReceiveOriginal(uint8_t byte) {
	uint32_t tLR14 = getLR14();

// discard RX FIFO content
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != RESET) {
		SPI_ReceiveData8(SPI1);
	}
	setTimeoutMillis(SPI_TIMEOUT);
// Wait if transmit FIFO buffer full
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) {
		if (isTimeout(tLR14)) {
			return 0;
		}
	}
	/* Send a Byte through the SPI peripheral */
	SPI_SendData8(SPI1, byte);

	setTimeoutMillis(SPI_TIMEOUT);
	/* Wait to receive a Byte */
	setTimeoutMillis(SPI_TIMEOUT);
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) {
		if (isTimeout(tLR14)) {
			return 0;
		}
	}
	/* Return the Byte read from the SPI bus */
	return (uint8_t) SPI_ReceiveData8(SPI1);
}
uint8_t SPI1_sendReceive(uint8_t byte) {
	uint32_t tLR14 = getLR14();
	uint32_t spixbase = (uint32_t) SPI1;
	spixbase += 0x0C;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	uint8_t dummy;
#pragma GCC diagnostic pop
// wait for ongoing transfer to end
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) != RESET) {
		;
	}
	while ((SPI1->SR & SPI_I2S_FLAG_RXNE) != RESET) {
		/* fetch the Byte read from the SPI bus in order to empty RX fifo */
		dummy = *(__IO uint8_t *) spixbase;
	}

	/* Send a Byte through the SPI peripheral */
	SPI_SendData8(SPI1, byte);

	setTimeoutMillis(SPI_TIMEOUT);
	/* Wait to receive a Byte */
	setTimeoutMillis(SPI_TIMEOUT);
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) {
		if (isTimeout(tLR14)) {
			return 0;
		}
	}
	/* Return the Byte read from the SPI bus */
	return (uint8_t) SPI_ReceiveData8(SPI1);
}

/**
 * Version of sendReceive without function calling
 */
uint8_t SPI1_sendReceiveFast(uint8_t byte) {
	uint32_t tLR14 = getLR14();
// wait for ongoing transfer to end
	while ((SPI1->SR & SPI_I2S_FLAG_BSY) != RESET) {
		;
	}
// discard RX FIFO content
	uint32_t spixbase = (uint32_t) SPI1;
	spixbase += 0x0C;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	uint8_t dummy;
#pragma GCC diagnostic pop
	while ((SPI1->SR & SPI_I2S_FLAG_RXNE) != RESET) {
		/* fetch the Byte read from the SPI bus in order to empty RX fifo */
		dummy = *(__IO uint8_t *) spixbase;
	}

	/* Send a Byte through the SPI peripheral */
	*(__IO uint8_t *) spixbase = byte;

	/* Wait to receive a Byte */
	setTimeoutMillis(SPI_TIMEOUT);
	while ((SPI1->SR & SPI_I2S_FLAG_RXNE) == RESET) {
		if (isTimeout(tLR14)) {
			break;
		}
	}

	/* Return the Byte read from the SPI bus */
	spixbase = (uint32_t) SPI1;
	spixbase += 0x0C;
	return *(__IO uint8_t *) spixbase;
}

inline void setTimeoutLED(void) {
	STM_EVAL_LEDOn(LED8); // ORANGE FRONT
}

inline void resetTimeoutLED(void) {
	STM_EVAL_LEDOff(LED8); // ORANGE FRONT
}

void RTC_initialize_LSE(void) {

	RTC_InitTypeDef RTC_InitStructure;

	/* Enable PWR APB1 Clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	/* Allow access to Backup */
	PWR_BackupAccessCmd(ENABLE);
//
//	/* Reset RTC Domain */
//	RCC_BackupResetCmd(ENABLE);
//	RCC_BackupResetCmd(DISABLE);

// Enable the LSE OSC
	RCC_LSEConfig(RCC_LSE_ON);

// 40mVPP at RCC_LSEDrive_MediumLow
	RCC_LSEDriveConfig(RCC_LSEDrive_MediumLow);

	// Wait till LSE is ready - it gives timeout if no hardware is attached
	setTimeoutMillis(10);
	while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {
		if (isTimeout(RCC->CSR)) {
			break;
		}
	}

	/* Select the RTC Clock Source */
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

	/* Enable the RTC Clock */
	RCC_RTCCLKCmd(ENABLE);

	/* Wait for RTC APB registers synchronisation */
	RTC_WaitForSynchro();

	/* RTC prescaler configuration */
	RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
	RTC_InitStructure.RTC_AsynchPrediv = 127; // 127 is the default value for 32kHz
	RTC_InitStructure.RTC_SynchPrediv = 255; // 255 is the default value for 32kHz
	RTC_Init(&RTC_InitStructure);

//RTC_setTime(0, 8, 0, 5, 21, 12, 2012);
	/* Deny access to RTC */
	PWR_BackupAccessCmd(DISABLE);
}

DWORD get_fattime(void) {
	RTC_DateTypeDef RTC_DateStructure;
	RTC_TimeTypeDef RTC_TimeStructure;

	/* Get the RTC current Time */
	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
	/* Get the RTC current Date */
	RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
// year since 1980
	DWORD res = (((DWORD) RTC_DateStructure.RTC_Year + 20) << 25) | ((DWORD) RTC_DateStructure.RTC_Month << 21)
			| ((DWORD) RTC_DateStructure.RTC_Date << 16) | (WORD) (RTC_TimeStructure.RTC_Hours << 11)
			| (WORD) (RTC_TimeStructure.RTC_Minutes << 5) | (WORD) (RTC_TimeStructure.RTC_Seconds >> 1);
	return res;
}

void fillTimeStructure(struct tm *aTimeStructurePtr) {
	RTC_DateTypeDef RTC_DateStructure;
	RTC_TimeTypeDef RTC_TimeStructure;

	/* Get the RTC current Time */
	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
	/* Get the RTC current Date */
	RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
	aTimeStructurePtr->tm_year = RTC_DateStructure.RTC_Year + 100;
	aTimeStructurePtr->tm_mon = RTC_DateStructure.RTC_Month - 1;
	aTimeStructurePtr->tm_mday = RTC_DateStructure.RTC_Date;
	aTimeStructurePtr->tm_hour = RTC_TimeStructure.RTC_Hours;
	aTimeStructurePtr->tm_min = RTC_TimeStructure.RTC_Minutes;
	aTimeStructurePtr->tm_sec = RTC_TimeStructure.RTC_Seconds;
}

long RTC_getTimeAsLong(void) {
	struct tm tTm;
	fillTimeStructure(&tTm);
	return mktime(&tTm);
}

int RTC_getTimeStringForFile(char * aStringBuffer) {
	RTC_TimeTypeDef RTC_TimeStructure;

	/* Get the RTC current Time */
	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
	/* Display  Format : hh-mm-ss */
// do not have a buffer size :-(
	return sprintf(aStringBuffer, "%02i-%02i-%02i", RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes,
			RTC_TimeStructure.RTC_Seconds);
}

int RTC_getDateStringForFile(char * aStringBuffer) {
	RTC_DateTypeDef RTC_DateStructure;
	RTC_TimeTypeDef RTC_TimeStructure;

	/* Get the RTC current Time */
	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
	/* Get the RTC current Date */
	RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
	/* Display  Format : yyyy-mm-dd hh-mm-ss */
// do not have a buffer size :-(
	return sprintf(aStringBuffer, "%04i-%02i-%02i %02i-%02i-%02i", 2000 + RTC_DateStructure.RTC_Year, RTC_DateStructure.RTC_Month,
			RTC_DateStructure.RTC_Date, RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds);
}

int RTC_getTimeString(char * aStringBuffer) {
	RTC_DateTypeDef RTC_DateStructure;
	RTC_TimeTypeDef RTC_TimeStructure;

	/* Get the RTC current Time */
	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
	/* Get the RTC current Date */
	RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
	/* Display  Format : dd.mm.yyyy hh:mm:ss */
// do not have a buffer size :-(
	return sprintf(aStringBuffer, "%02i.%02i.%04i %02i:%02i:%02i", RTC_DateStructure.RTC_Date, RTC_DateStructure.RTC_Month,
			2000 + RTC_DateStructure.RTC_Year, RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes,
			RTC_TimeStructure.RTC_Seconds);
}

void RTC_setTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t dayOfWeek, uint8_t day, uint8_t month, uint16_t year) {
	RTC_DateTypeDef RTC_DateStructure;
	RTC_TimeTypeDef RTC_TimeStructure;

	RTC_DateStructure.RTC_Year = year - 2000;
	RTC_DateStructure.RTC_Month = month;
	RTC_DateStructure.RTC_Date = day;
	RTC_DateStructure.RTC_WeekDay = dayOfWeek;
	RTC_SetDate(RTC_Format_BIN, &RTC_DateStructure);

	RTC_TimeStructure.RTC_H12 = RTC_H12_AM;
	RTC_TimeStructure.RTC_Hours = hour;
	RTC_TimeStructure.RTC_Minutes = min;
	RTC_TimeStructure.RTC_Seconds = sec;
	RTC_SetTime(RTC_Format_BIN, &RTC_TimeStructure);
}

/**
 * PWM for tone generation with timer 3
 */
#define STM32F3D_TONE1_TIMER                       TIM3
#define STM32F3D_TONE1_TIMER_CLOCK                 RCC_APB1Periph_TIM3
#define STM32F3D_TONE1_CHANNEL                     TIM_Channel_1
#define STM32F3D_TONE1_GPIO_PIN                    GPIO_Pin_6
#define STM32F3D_TONE1_GPIO_PORT                   GPIOC
#define STM32F3D_TONE1_GPIO_CLK                    RCC_AHBPeriph_GPIOC
#define STM32F3D_TONE1_SOURCE           		   GPIO_PinSource6
#define STM32F3D_TONE1_AF                          GPIO_AF_2

#define TONE_TIMER_TICK_FREQ 1000000
void initalizeTone(void) {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	/* TIM3 clock enable */
	RCC_APB1PeriphClockCmd(STM32F3D_TONE1_TIMER_CLOCK, ENABLE);
	uint16_t PrescalerValue = (uint16_t) ((SystemCoreClock / 2) / (TONE_TIMER_TICK_FREQ)) - 1; // 1 MHZ Timer Frequency

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 1000; // 1 kHz initial
	TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(STM32F3D_TONE1_TIMER, &TIM_TimeBaseStructure);

	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_AHBPeriphClockCmd(STM32F3D_TONE1_GPIO_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; //GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Pin = STM32F3D_TONE1_GPIO_PIN;
	GPIO_Init(STM32F3D_TONE1_GPIO_PORT, &GPIO_InitStructure);

	GPIO_PinAFConfig(STM32F3D_TONE1_GPIO_PORT, STM32F3D_TONE1_SOURCE, STM32F3D_TONE1_AF);

// Channel 1
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Inactive;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = TIM_OPMode_Repetitive;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(STM32F3D_TONE1_TIMER, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(STM32F3D_TONE1_TIMER, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(STM32F3D_TONE1_TIMER, ENABLE);
	/* TIM3 enable counter */
	TIM_Cmd(STM32F3D_TONE1_TIMER, ENABLE);

	TIM_SetCompare1(STM32F3D_TONE1_TIMER, 500); // start with 50% duty cycle
}

int32_t ToneDuration = 0; //! signed because >0 means wait, == 0 means action and < 0 means do nothing

void tone(uint16_t aFreqHertz, uint16_t aDurationMillis) {
// enable timer PWM output
	TIM_SelectOCxM(STM32F3D_TONE1_TIMER, STM32F3D_TONE1_CHANNEL, TIM_OCMode_PWM1); // disables channel

// compute reload value
	uint32_t tPeriod = TONE_TIMER_TICK_FREQ / aFreqHertz;
	if (tPeriod >= 0x10000) {
		tPeriod = 0x10000 - 1;
	}
	TIM_SetAutoreload(STM32F3D_TONE1_TIMER, tPeriod - 1);
	TIM_SetCompare1(STM32F3D_TONE1_TIMER, tPeriod / 2); // 50 % duty cycle
	TIM_CCxCmd(STM32F3D_TONE1_TIMER, STM32F3D_TONE1_CHANNEL, TIM_CCx_Enable); // enable channel
	ToneDuration = aDurationMillis;
	changeDelayCallback(&noTone, aDurationMillis);
}

void noTone(void) {
// disable timer PWM output
	TIM_SelectOCxM(STM32F3D_TONE1_TIMER, STM32F3D_TONE1_CHANNEL, TIM_OCMode_Inactive); // disables also channel
}

void FeedbackToneOK(void) {
	FeedbackTone(FEEDBACK_TONE_NO_ERROR);
}

void FeedbackTone(unsigned int aFeedbackType) {
	if (aFeedbackType == FEEDBACK_TONE_NO_TONE) {
		return;
	}
	if (aFeedbackType == FEEDBACK_TONE_NO_ERROR) {
		tone(3000, 50);
	} else if (aFeedbackType == FEEDBACK_TONE_SHORT_ERROR) {
		// two short beeps
		tone(4000, 30);
		delayMillis(60);
		tone(2000, 30);
	} else {
		// long tone
		tone(4500, 500);
	}
}

void EndTone(void) {
	tone(880, 1000);
	delayMillis(1000);
	tone(660, 1000);
	delayMillis(1000);
	tone(440, 1000);
}

/**
 * PWM for display backlight with timer 4
 */

#define STM32F3D_PWM1_TIMER                       TIM4
#define STM32F3D_PWM1_TIMER_CLOCK                 RCC_APB1Periph_TIM4

// for pin F6
#define STM32F3D_PWM1_GPIO_PIN                    GPIO_Pin_6
#define STM32F3D_PWM1_GPIO_PORT                   GPIOF
#define STM32F3D_PWM1_GPIO_CLK                    RCC_AHBPeriph_GPIOF
#define STM32F3D_PWM1_SOURCE           			  GPIO_PinSource6
#define STM32F3D_PWM1_AF                          GPIO_AF_2
#define STM32F3D_PWM1_CHANNEL                     TIM_Channel_4
#define STM32F3D_PWM1_CHANNEL_INIT_COMMAND        TIM_OC4Init
#define STM32F3D_PWM1_CHANNEL_PRELOAD_COMMAND     TIM_OC4PreloadConfig
#define STM32F3D_PWM1_CHANNEL_SET_COMMAND         TIM_SetCompare4

#define PWM_RESOLUTION 0x100 // 0-255
void PWM_initalize(void) {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	/* TIM4 clock enable */
	RCC_APB1PeriphClockCmd(STM32F3D_PWM1_TIMER_CLOCK, ENABLE);

	/* Compute the prescaler value for 1 kHz period with 256 resolution*/
	uint16_t PrescalerValue = (uint16_t) ((SystemCoreClock / 2) / (1000 * PWM_RESOLUTION)) - 1;

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = PWM_RESOLUTION - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(STM32F3D_PWM1_TIMER, &TIM_TimeBaseStructure);

	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHBPeriphClockCmd(STM32F3D_PWM1_GPIO_CLK, ENABLE);

// Configure pin as alternate function for timer
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; //GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = STM32F3D_PWM1_GPIO_PIN;
	GPIO_Init(STM32F3D_PWM1_GPIO_PORT, &GPIO_InitStructure);
	GPIO_PinAFConfig(STM32F3D_PWM1_GPIO_PORT, STM32F3D_PWM1_SOURCE, STM32F3D_PWM1_AF);

// Channel
	/* PWM1 Mode configuration */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = TIM_OPMode_Repetitive;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	STM32F3D_PWM1_CHANNEL_INIT_COMMAND(STM32F3D_PWM1_TIMER, &TIM_OCInitStructure);

	STM32F3D_PWM1_CHANNEL_PRELOAD_COMMAND(STM32F3D_PWM1_TIMER, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(STM32F3D_PWM1_TIMER, ENABLE);
	/* TIM4 enable counter */
	TIM_Cmd(STM32F3D_PWM1_TIMER, ENABLE);

// Set channel 4 value
	STM32F3D_PWM1_CHANNEL_SET_COMMAND(STM32F3D_PWM1_TIMER, PWM_RESOLUTION / 2); // start with 50% duty cycle
	TIM_CCxCmd(STM32F3D_PWM1_TIMER, STM32F3D_PWM1_CHANNEL, TIM_CCx_Enable); // enable channel
}

void PWM_setOnRatio(uint32_t power) {
	if (power >= 100) {
		power = 100;
	}
	STM32F3D_PWM1_CHANNEL_SET_COMMAND(STM32F3D_PWM1_TIMER, (uint32_t) power * 255 / 100);
}

/**
 * ADC must be ready and continuous mode disabled
 * read ref channel and use Value from Address 0x1FFFF7BA
 * USE ADC 2
 */
void ADC_setRawToVoltFactor(void) {
	uint32_t tRefActualValue;

// TODO test if need oversampling
// use conservative sample time
	ADC_RegularChannelConfig(ADC2, ADC_Channel_Vrefint, 1, ADC_SampleTime_61Cycles5);

	ADC_StartConversion(ADC2);
// get VREFINT_CAL (value of reading calibration input acquired at temperature of 30 °C and VDDA=3.3 V) - see chapter 3.12.2 of datasheet
	uint16_t tREFCalibrationValue = *((uint16_t*) 0x1FFFF7BA);
	/* Test EOC flag */
	setTimeoutMillis(10);
	bool tTimeout = false;
	while (ADC_GetFlagStatus(ADC2, ADC_FLAG_EOC) == RESET) {
		if (isTimeout(ADC2->ISR)) {
			tTimeout = true;
			break;
		}
	}
	tRefActualValue = ADC_GetConversionValue(ADC2);
	if (tTimeout) {
		tRefActualValue = tREFCalibrationValue;
	}
// use stored calibration value from system area
	sADCScaleFactorShift18 = tREFCalibrationValue * 16896 / tRefActualValue;
	sReading3Volt = (3.0 * tRefActualValue * 4096) / (3.3 * tREFCalibrationValue);
	sVrefVoltage = 3.3 * tREFCalibrationValue / tRefActualValue;
	ADCToVoltFactor = sVrefVoltage / 4096;
}

void ADC12_init(void) {
	/**
	 * Configure the ADC clock
	 */
	RCC_ADCCLKConfig(RCC_ADC12PLLCLK_Div8); // 9Mhz
}

/**
 * ADC1
 */
#define ADC1_PERIPH_CLOCK                   	RCC_AHBPeriph_ADC12

#define ADC1_INPUT1_PIN                         GPIO_Pin_1
#define ADC1_INPUT1_GPIO_PORT                   GPIOA
#define ADC1_INPUT1_GPIO_CLK                    RCC_AHBPeriph_GPIOA

#define ADC1_INPUT2_PIN                         GPIO_Pin_2
#define ADC1_INPUT2_GPIO_PORT                   GPIOA
#define ADC1_INPUT2_GPIO_CLK                    RCC_AHBPeriph_GPIOA

#define ADC1_INPUT3_PIN                         GPIO_Pin_3
#define ADC1_INPUT3_GPIO_PORT                   GPIOA
#define ADC1_INPUT3_GPIO_CLK                    RCC_AHBPeriph_GPIOA

#define ADC1_ATTENUATOR_0_PIN                   GPIO_Pin_7
#define ADC1_ATTENUATOR_1_PIN                   GPIO_Pin_8
#define ADC1_ATTENUATOR_2_PIN                   GPIO_Pin_9
#define ADC1_AC_RANGE_PIN                       GPIO_Pin_12
#define ADC1_ATTENUATOR_GPIO_PORT               GPIOC
#define ADC1_ATTENUATOR_GPIO_CLK                RCC_AHBPeriph_GPIOC

static bool isADCInitializedForTimerEvents;

void ADC_init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;

	ADC_DMA_initialize();

	/* Enable ADC1 clock */
	RCC_AHBPeriphClockCmd(ADC1_PERIPH_CLOCK, ENABLE);

	/**
	 * ADC GPIO input pin init
	 */
	RCC_AHBPeriphClockCmd(ADC1_INPUT1_GPIO_CLK, ENABLE);
// Configure pin A1 as analog input
	GPIO_InitStructure.GPIO_Pin = ADC1_INPUT1_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(ADC1_INPUT1_GPIO_PORT, &GPIO_InitStructure);

// Configure pin A2 as analog input
	GPIO_InitStructure.GPIO_Pin = ADC1_INPUT2_PIN;
	GPIO_Init(ADC1_INPUT2_GPIO_PORT, &GPIO_InitStructure);

// Configure pin A3 as analog input
	GPIO_InitStructure.GPIO_Pin = ADC1_INPUT3_PIN;
	GPIO_Init(ADC1_INPUT3_GPIO_PORT, &GPIO_InitStructure);

	/**
	 * Enable special channels
	 */
	ADC_TempSensorCmd(DSO_ADC_ID, ENABLE);
	ADC_VrefintCmd(DSO_ADC_ID, ENABLE);
	ADC_VbatCmd(DSO_ADC_ID, ENABLE);

	/**
	 *  Calibration procedure
	 */
	ADC_VoltageRegulatorCmd(DSO_ADC_ID, ENABLE);
	/* To settle reference voltage etc. */
	delayMillis(2);
	ADC_SelectCalibrationMode(DSO_ADC_ID, ADC_CalibrationMode_Single);
	ADC_StartCalibration(DSO_ADC_ID);
	while (ADC_GetCalibrationStatus(DSO_ADC_ID) != RESET) {
		;
	}

// use defaults
	ADC_CommonStructInit(&ADC_CommonInitStructure);
//	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
//	ADC_CommonInitStructure.ADC_Clock = ADC_Clock_AsynClkMode; // allows higher clock prescaler
//	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled; // multiple DMA mode disabled
//	ADC_CommonInitStructure.ADC_DMAMode = ADC_DMAMode_OneShot;
//	ADC_CommonInitStructure.ADC_TwoSamplingDelay = 0;
	ADC_CommonInit(DSO_ADC_ID, &ADC_CommonInitStructure);

	ADC_StructInit(&ADC_InitStructure);
//	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
//	ADC_InitStructure.ADC_ContinuousConvMode = ADC_ContinuousConvMode_Disable;
	ADC_InitStructure.ADC_ExternalTrigConvEvent = ADC_ExternalTrigConvEvent_13; //TIM6_TRGO event
	ADC_InitStructure.ADC_ExternalTrigEventEdge = ADC_ExternalTrigEventEdge_RisingEdge;
//	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_OverrunMode = ADC_OverrunMode_Enable;
//	ADC_InitStructure.ADC_AutoInjMode = ADC_AutoInjec_Disable;
//	ADC_InitStructure.ADC_NbrOfRegChannel = 1;
	ADC_Init(DSO_ADC_ID, &ADC_InitStructure);
	isADCInitializedForTimerEvents = true;

	ADC_DMACmd(DSO_ADC_ID, ENABLE);

	ADC_DMAConfig(DSO_ADC_ID, ADC_DMAMode_OneShot);

	/* Enable ADC */
	ADC_enableAndWait(DSO_ADC_ID);

	NVIC_InitTypeDef NVIC_InitStructure;
	/* Enable the ADC1 global Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
// highest prio
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
// lower2 bits of priority are discarded by library
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 4;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	DSO_initializeAttenuatorAndAC();
	ADC_initalizeTimer6();
}

void ADC_enableAndWait(ADC_TypeDef* aADCId) {
	ADC_Cmd(DSO_ADC_ID, ENABLE);
	/* wait for ADRDY */
	setTimeoutMillis(ADC_DEFAULT_TIMEOUT);
	while (!ADC_GetFlagStatus(DSO_ADC_ID, ADC_FLAG_RDY)) {
		if (isTimeout(DSO_ADC_ID->ISR)) {
			break;
		}
	}
}

void ADC_disableAndWait(ADC_TypeDef* aADCId) {
	assert_param(ADC_GetStartConversionStatus(aADCId) == RESET);
// first disable ADC otherwise sometimes interrupts just stops after the first reading
	ADC_DisableCmd(aADCId);
// wait for disable to complete
	setTimeoutMillis(ADC_DEFAULT_TIMEOUT);
	while (ADC_GetDisableCmdStatus(aADCId) != RESET) {
		if (isTimeout(aADCId->CR)) {
			break;
		}
	}
}

void DSO_initializeAttenuatorAndAC(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable the GPIO Clock */
	RCC_AHBPeriphClockCmd(ADC1_ATTENUATOR_GPIO_CLK, ENABLE);

// Bit 0 pin
	GPIO_InitStructure.GPIO_Pin = ADC1_ATTENUATOR_0_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(ADC1_ATTENUATOR_GPIO_PORT, &GPIO_InitStructure);
// set LOW
	GPIO_ResetBits(ADC1_ATTENUATOR_GPIO_PORT, ADC1_ATTENUATOR_0_PIN);

// Bit 1 pin
	GPIO_InitStructure.GPIO_Pin = ADC1_ATTENUATOR_1_PIN;
	GPIO_Init(ADC1_ATTENUATOR_GPIO_PORT, &GPIO_InitStructure);
// set LOW
	GPIO_ResetBits(ADC1_ATTENUATOR_GPIO_PORT, ADC1_ATTENUATOR_1_PIN);

// Bit 2 pin
	GPIO_InitStructure.GPIO_Pin = ADC1_ATTENUATOR_2_PIN;
	GPIO_Init(ADC1_ATTENUATOR_GPIO_PORT, &GPIO_InitStructure);
// set LOW
	GPIO_ResetBits(ADC1_ATTENUATOR_GPIO_PORT, ADC1_ATTENUATOR_2_PIN);

// AC range pin
	GPIO_InitStructure.GPIO_Pin = ADC1_AC_RANGE_PIN;
	GPIO_Init(ADC1_ATTENUATOR_GPIO_PORT, &GPIO_InitStructure);
// set LOW
	GPIO_ResetBits(ADC1_ATTENUATOR_GPIO_PORT, ADC1_AC_RANGE_PIN);
}

void DSO_setAttenuator(uint8_t aValue) {
// shift to right bit position
	uint16_t tValue = aValue << 7;

	tValue = tValue & 0x380;
	uint16_t tPort = GPIO_ReadOutputData(ADC1_ATTENUATOR_GPIO_PORT);
	tPort &= ~0x380;
	tPort |= tValue;
	GPIO_Write(ADC1_ATTENUATOR_GPIO_PORT, tPort);
}

void DSO_setACRange(bool aValue) {
	GPIO_WriteBit(ADC1_ATTENUATOR_GPIO_PORT, ADC1_AC_RANGE_PIN, aValue);
}

/**
 * ADC must be ready and continuous mode disabled
 * returns 2 * ADC_MAX_CONVERSION_VALUE on timeout
 */
uint16_t ADC1_getChannelValue(uint8_t aChannel) {
	if (isADCInitializedForTimerEvents) {
		ADC_InitTypeDef ADC_InitStructure;

		// use defaults
		ADC_StructInit(&ADC_InitStructure);
		ADC_Init(DSO_ADC_ID, &ADC_InitStructure);
		isADCInitializedForTimerEvents = false;
		// not needed ADC_DMACmd(DSO_ADC_ID, DISABLE);
	}
// use conservative sample time (ADC Clocks!)
// temp needs 2.2 micro seconds
	ADC_RegularChannelConfig(ADC1, aChannel, 1, ADC_SampleTime_4Cycles5); //	has no effect

// use slow clock
	RCC_ADCCLKConfig(RCC_ADC12PLLCLK_Div64); // has no effect

	ADC_StartConversion(ADC1);
	/* Test EOC flag */
	setTimeoutMillis(ADC_DEFAULT_TIMEOUT);
	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET) {
		if (isTimeoutShowDelay(ADC1->CR,300)) {
			return 2 * ADC_MAX_CONVERSION_VALUE;
		}
	}
	return ADC_GetConversionValue(ADC1);
}

void ADC_SetChannelSampleTime(ADC_TypeDef* aADCId, uint8_t aChannelNumber, bool aFastMode) {
	uint8_t tSampleTime = ADC_SampleTime_1Cycles5;
	switch (aChannelNumber) {
	case ADC_Channel_2: // fast channels
	case ADC_Channel_3:
	case ADC_Channel_4:
		if (!aFastMode) {
			tSampleTime = ADC_SampleTime_4Cycles5;
		}
		break;
	case ADC_Channel_7: //slow channel
		if (!aFastMode) {
			tSampleTime = ADC_SampleTime_7Cycles5;
		}
		break;
	case ADC_Channel_TempSensor:
	case ADC_Channel_Vbat:
	case ADC_Channel_Vrefint:
		// TEMP + VBAT channel need 2.2 micro sec. sampling
		if (!aFastMode) {
			tSampleTime = ADC_SampleTime_61Cycles5;
		}
		break;
	default:
		break;
	}
	ADC_RegularChannelConfig(aADCId, aChannelNumber, 1, tSampleTime);
}

inline void ADC_enableEOCInterrupt(ADC_TypeDef* aADCId) {
	ADC_ITConfig(aADCId, ADC_IT_EOC, ENABLE);
}

inline void ADC_disableEOCInterrupt(ADC_TypeDef* aADCId) {
	ADC_ITConfig(aADCId, ADC_IT_EOC, DISABLE);
}

/**
 *
 * @return ADC configuration register
 */
uint32_t ADC_getCFGR(void) {
	return (DSO_ADC_ID->CFGR);
}

/**
 * Stub to pass right parameters
 */
inline void ADC_clearITPendingBit(ADC_TypeDef* aADCId) {
	ADC_ClearITPendingBit(DSO_ADC_ID, ADC_IT_EOC);
}

/**
 * from 0x00 to 0x0B which results in prescaler from 1 to 256
 */
void ADC_SetClockPrescaler(uint32_t aValue) {
	aValue &= 0x0F;
	if (aValue > 0x0B) {
		aValue = 0x0B;
	}
	aValue = (aValue << 4) + RCC_ADC12PLLCLK_Div1;
	RCC_ADCCLKConfig(aValue);
}

#define STM32F3D_ADC_TIMER                       TIM6
#define STM32F3D_ADC_TIMER_CLOCK                 RCC_APB1Periph_TIM6

#define STM32F3D_ADC_TIMER_PRESCALER_START (9 - 1)

void ADC_initalizeTimer6(void) {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	/* TIM6 clock enable */
	RCC_APB1PeriphClockCmd(STM32F3D_ADC_TIMER_CLOCK, ENABLE);

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Prescaler = STM32F3D_ADC_TIMER_PRESCALER_START; // 1/8 us resolution (72/9)
	TIM_TimeBaseStructure.TIM_Period = 25 - 1; // 10 us
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0x00;
	TIM_TimeBaseInit(STM32F3D_ADC_TIMER, &TIM_TimeBaseStructure);

	TIM_SelectOutputTrigger(STM32F3D_ADC_TIMER, TIM_TRGOSource_Update);

// enable Interrupt for testing purposes
//	ADC_Timer6EnableInterrupt();
}

/**
 * not used yet
 */
void ADC_Timer6EnableInterrupt(void) {
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM6_DAC_IRQn;
// lowest possible priority
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_ITConfig(STM32F3D_ADC_TIMER, TIM_IT_Update, ENABLE);
}

/**
 * ISR for ADC timer 6 and for DAC1&2 underrun error interrupts
 * not used yet
 */
void TIM6_DAC_IRQHandler(void) {
	STM_EVAL_LEDToggle(LED7); // GREEN RIGHT
	TIM_ClearITPendingBit(STM32F3D_ADC_TIMER, TIM_IT_Update);
}

void ADC_SetTimer6Period(uint16_t aDivider, uint16_t aPrescalerDivider) {
	if (!isADCInitializedForTimerEvents) {
		ADC_InitTypeDef ADC_InitStructure;
		ADC_StructInit(&ADC_InitStructure);
		ADC_InitStructure.ADC_ExternalTrigConvEvent = ADC_ExternalTrigConvEvent_13; //TIM6_TRGO event
		ADC_InitStructure.ADC_ExternalTrigEventEdge = ADC_ExternalTrigEventEdge_RisingEdge;
		ADC_InitStructure.ADC_OverrunMode = ADC_OverrunMode_Enable;
		ADC_Init(DSO_ADC_ID, &ADC_InitStructure);
		isADCInitializedForTimerEvents = true;
	}
	TIM_PrescalerConfig(STM32F3D_ADC_TIMER, aPrescalerDivider - 1, TIM_PSCReloadMode_Immediate);
	TIM_SetAutoreload(STM32F3D_ADC_TIMER, aDivider - 1);
}

inline void ADC_startTimer6(void) {
//TIM_Cmd(STM32F3D_ADC_TIMER, ENABLE);
// Enable the TIM Counter
	STM32F3D_ADC_TIMER->CR1 |= TIM_CR1_CEN;
}

inline void ADC_stopTimer6(void) {
	TIM_Cmd(STM32F3D_ADC_TIMER, DISABLE);
}

inline void Default_IRQHandler(void) {
	STM_EVAL_LEDToggle(LED10); // RED FRONT
}

void ADC_DMA_initialize(void) {
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	DMA_InitTypeDef DMA_InitStructure;
// DMA1 channel1 configuration
	DMA_DeInit(DSO_DMA_CHANNEL);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(DSO_ADC_ID->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = 0;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = 1;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
//	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DSO_DMA_CHANNEL, &DMA_InitStructure);

	NVIC_InitTypeDef NVIC_InitStructure;
	/* Enable DMA1 channel1 IRQ Channel */
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
// highest prio
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
// lower2 bits of priority are discarded by library
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 4;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

//TC -> transfer complete interrupt
// Enable DMA1 channel1
//	DMA_ITConfig(DSO_DMA_CHANNEL, DMA_IT_TC | DMA_IT_HT | DMA_IT_TE, ENABLE);
	DMA_ITConfig(DSO_DMA_CHANNEL, DMA_IT_TC | DMA_IT_TE, ENABLE);
}

void ADC_DMA_start(uint32_t aMemoryBaseAddr, uint16_t aBufferSize) {
// Disable DMA1 channel1 - is really needed here!
	DSO_DMA_CHANNEL->CCR &= ~DMA_CCR_EN;

// Write to DMA Channel1 CMAR
	DSO_DMA_CHANNEL->CMAR = aMemoryBaseAddr;
// Write to DMA1 Channel1 CNDTR
	DSO_DMA_CHANNEL->CNDTR = aBufferSize;

//	DMA_Cmd(DSO_DMA_CHANNEL, ENABLE);
	DSO_DMA_CHANNEL->CCR |= DMA_CCR_EN;
// starts conversion at next timer edge - is definitely needed to start dma transfer!
	DSO_ADC_ID->CR |= ADC_CR_ADSTART;

}

uint16_t ADC_DMA_GetCurrDataCounter(void) {
	return ((uint16_t) (DSO_DMA_CHANNEL->CNDTR));
}

void ADC_DMA_stop(void) {
// Disable DMA1 channel1
	DSO_DMA_CHANNEL->CCR &= (uint16_t) (~DMA_CCR_EN);
// Disable ADC DMA
	ADC_DMACmd(DSO_ADC_ID, DISABLE);
// reset all bits
	DMA_ClearITPendingBit(DMA1_IT_GL1);
//TC -> transfer complete interrupt
	DMA_ITConfig(DSO_DMA_CHANNEL, DMA1_IT_TC1, DISABLE);
}

#define MICROSD_CS_PIN                         GPIO_Pin_5
#define MICROSD_CS_GPIO_PORT                   GPIOC
#define MICROSD_CS_GPIO_CLK                    RCC_AHBPeriph_GPIOC

#define MICROSD_CARD_DETECT_PIN                GPIO_Pin_4
#define MICROSD_CARD_DETECT_GPIO_PORT          GPIOC
#define MICROSD_CARD_DETECT_GPIO_CLK           RCC_AHBPeriph_GPIOC
#define MICROSD_CARD_DETECT_EXTI_PORT_SOURCE   EXTI_PortSourceGPIOC
#define MICROSD_CARD_DETECT_EXTI_PIN_SOURCE    EXTI_PinSource4
#define MICROSD_CARD_DETECT_EXTI_LINE          EXTI_Line4
#define MICROSD_CARD_DETECT_EXTI_IRQn          EXTI4_IRQn
/**
 * Init the MicroSD card pins CS + CardDetect
 */
void MICROSD_IO_initalize(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;

// Enable the GPIO Clock
	RCC_AHBPeriphClockCmd(MICROSD_CS_GPIO_CLK, ENABLE);

// CS pin
	GPIO_InitStructure.GPIO_Pin = MICROSD_CS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(MICROSD_CS_GPIO_PORT, &GPIO_InitStructure);
// set High
	GPIO_SetBits(MICROSD_CS_GPIO_PORT, MICROSD_CS_PIN);

// CardDetect pin
	GPIO_InitStructure.GPIO_Pin = MICROSD_CARD_DETECT_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(MICROSD_CARD_DETECT_GPIO_PORT, &GPIO_InitStructure);

// Connect EXTI Line to GPIO Pin
// Enable the Sysycfg Clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	SYSCFG_EXTILineConfig(MICROSD_CARD_DETECT_EXTI_PORT_SOURCE, MICROSD_CARD_DETECT_EXTI_PIN_SOURCE);

	/* Configure EXTI line */
	EXTI_InitStructure.EXTI_Line = MICROSD_CARD_DETECT_EXTI_LINE;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	/* Enable and set EXTI Interrupt to the lowest priority */
	NVIC_InitStructure.NVIC_IRQChannel = MICROSD_CARD_DETECT_EXTI_IRQn;
// lowest possible priority
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

inline bool MICROSD_isCardInserted(void) {
	return GPIO_ReadInputDataBit(MICROSD_CARD_DETECT_GPIO_PORT, MICROSD_CARD_DETECT_PIN);
}

inline void MICROSD_CSEnable(void) {
// set to LOW
	MICROSD_CARD_DETECT_GPIO_PORT->BRR = MICROSD_CS_PIN;
}

inline void MICROSD_CSDisable(void) {
// set to HIGH
	MICROSD_CARD_DETECT_GPIO_PORT->BSRR = MICROSD_CS_PIN;
}

inline void MICROSD_ClearITPendingBit(void) {
	EXTI_ClearITPendingBit(MICROSD_CARD_DETECT_EXTI_LINE);
}

#define DAC_OUTPUT_PIN                       GPIO_Pin_4
#define DAC_OUTPUT_GPIO_PORT                 GPIOA
#define DAC_OUTPUT_GPIO_CLK                  RCC_AHBPeriph_GPIOA

/**
 * @brief  DAC channels configurations (PA4 in analog,
 *                           enable DAC clock, enable DMA2 clock)
 */
void DAC_init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	/* DMA2 clock enable (to be used with DAC) */
//	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);
	/* DAC Periph clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

	/* GPIOA clock enable */
	RCC_AHBPeriphClockCmd(DAC_OUTPUT_GPIO_CLK, ENABLE);

	/* Configure PA.04 (DAC_OUT1) as analog in to avoid parasitic consumption */
	GPIO_InitStructure.GPIO_Pin = DAC_OUTPUT_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(DAC_OUTPUT_GPIO_PORT, &GPIO_InitStructure);

// set triangle Mode
	DAC_ModeTriangle();

// set output data register
	DAC_SetChannel1Data(DAC_Align_12b_R, 0);

}

/* TIM7 configuration to trigger DAC */
void DAC_Timer_initialize(uint32_t aAutoreload) {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	/* TIM7 Peripheral clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);

	/* Time base configuration */
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Prescaler = 0x0; // no prescaler
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down; // to easily change reload value
	TIM_TimeBaseStructure.TIM_Period = aAutoreload;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0x0;
	TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure);

	// TIM7 TRGO selection - trigger on each update
	TIM_SelectOutputTrigger(TIM7, TIM_TRGOSource_Update);

	/* TIM7 enable counter */
	TIM_Cmd(TIM7, ENABLE);
}

/**
 *
 * @param aReloadValue value less than 4 makes no sense since reload operation takes 3 ticks
 */
inline void DAC_Timer_SetReloadValue(uint32_t aReloadValue) {
	if (aReloadValue < 4) {
		assertFailedMessageParam((uint8_t *) __FILE__, __LINE__, getLR14(), "DAC reload value < 4", aReloadValue);
	}
	uint32_t tReloadValue = aReloadValue;
	uint16_t tPrescalerValue = 1;
	// convert 32 bit to 16 bit prescaler and 16 bit counter
	if (tReloadValue > 0xFFFF) {
		//Count Leading Zeros
		int tShiftCount = 16 - __CLZ(aReloadValue);
		tPrescalerValue <<= tShiftCount;
		tReloadValue = aReloadValue >> tShiftCount;
	}
	TIM_PrescalerConfig(TIM7, tPrescalerValue - 1, TIM_PSCReloadMode_Update);
	TIM_SetAutoreload(TIM7, tReloadValue - 1);
}

/**
 * Enable DAC Channel1: Once the DAC channel1 is enabled, Pin A.05 is automatically connected to the DAC converter.
 */
void DAC_Start(void) {
	DAC_Cmd(DAC_Channel_1, ENABLE);
	TIM_Cmd(TIM7, ENABLE);
}

void DAC_Stop(void) {
	TIM_Cmd(TIM7, DISABLE);
	DAC_Cmd(DAC_Channel_1, DISABLE);

}

/* Noise Wave generator */
void DAC_ModeNoise(void) {

	DAC_InitTypeDef DAC_InitStructure;

	/* DAC channel1 Configuration */
	DAC_DeInit();
	DAC_InitStructure.DAC_Trigger = DAC_Trigger_T7_TRGO;
	DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_Noise;
	DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bits11_0;
	DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
	DAC_Init(DAC_Channel_1, &DAC_InitStructure);

	/* Set DAC Channel1 DHR12L register */
	DAC_SetChannel1Data(DAC_Align_12b_L, 0x7FF0);

	/* Enable DAC channel1 wave generator */
	DAC_WaveGenerationCmd(DAC_Channel_1, DAC_Wave_Noise, ENABLE);
}

/* Triangle Wave generator */
void DAC_ModeTriangle(void) {

	DAC_InitTypeDef DAC_InitStructure;

	/* DAC channel1 Configuration */
	DAC_DeInit();
	DAC_InitStructure.DAC_Trigger = DAC_Trigger_T7_TRGO;
	DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_Triangle;
	DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_TriangleAmplitude_2047;
	DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
	DAC_Init(DAC_Channel_1, &DAC_InitStructure);

	/* Enable DAC channel1 wave generator */
	DAC_WaveGenerationCmd(DAC_Channel_1, DAC_Wave_Triangle, ENABLE);
}

inline void DAC_SetOutputValue(uint16_t aOutputValue) {
	DAC_SetChannel1Data(DAC_Align_12b_R, aOutputValue);
}

// amplitude from 0 to 0x0B
void DAC_TriangleAmplitude(unsigned int aAmplitude) {
	aAmplitude &= 0x0F;
	if (aAmplitude > 0x0B) {
		aAmplitude = 0x0B;
	}
	uint32_t tTemp = DAC->CR;
	tTemp &= ~0xF00;
	tTemp |= aAmplitude << 8;
	DAC->CR = tTemp;
}

#define FREQ_SYNTH_OUTPUT_PIN                       GPIO_Pin_10
#define FREQ_SYNTH_OUTPUT_GPIO_PORT                 GPIOA
#define FREQ_SYNTH_OUTPUT_GPIO_CLK                  RCC_AHBPeriph_GPIOA
#define FREQ_SYNTH_OUTPUT_SOURCE           			GPIO_PinSource10
#define FREQ_SYNTH_OUTPUT_AF                        GPIO_AF_10

/* TIM2 configuration for frequency synthesizer */
void HIRES_Timer_initialize(uint32_t aAutoreload) {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	/* TIM2 Peripheral clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	/* Time base configuration */
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Prescaler = 0x0; // no prescaler
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down; // to easily change reload value
	TIM_TimeBaseStructure.TIM_Period = aAutoreload;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0x0;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	TIM_OCInitTypeDef TIM_OCInitStructure;

	// Channel 4
	/* Toggle Mode configuration */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Toggle;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = TIM_OPMode_Repetitive;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC4Init(TIM2, &TIM_OCInitStructure);

	// init output pin
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable the GPIO Clocks */
	RCC_AHBPeriphClockCmd(FREQ_SYNTH_OUTPUT_GPIO_CLK, ENABLE);

// Synthesizer output pin
	GPIO_InitStructure.GPIO_Pin = FREQ_SYNTH_OUTPUT_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(FREQ_SYNTH_OUTPUT_GPIO_PORT, &GPIO_InitStructure);
	GPIO_PinAFConfig(FREQ_SYNTH_OUTPUT_GPIO_PORT, FREQ_SYNTH_OUTPUT_SOURCE, FREQ_SYNTH_OUTPUT_AF);

// set Low
//	GPIO_ResetBits(FREQ_SYNTH_OUTPUT_GPIO_PORT, FREQ_SYNTH_OUTPUT_PIN);
}

/**
 *
 * @param reload value
 */
inline void HIRES_Timer_SetReloadValue(uint32_t aReloadValue) {
	// this causes the compiler to crash here
	//assertParamMessage((aReloadValue > 1), "HIRES reload value < 2", aReloadValue);
	if (aReloadValue < 2) {
		assertFailedMessageParam((uint8_t *) __FILE__, __LINE__, getLR14(), "HIRES reload value < 2", aReloadValue);
	}
	TIM_SetAutoreload(TIM2, aReloadValue - 1);
}

void HIRES_Timer_Start(void) {
	TIM_Cmd(TIM2, ENABLE);
}

/**
 *
 * @return actual counter before stop
 */
uint32_t HIRES_Timer_Stop(void) {
	uint32_t tCount = TIM_GetCounter(TIM2);
	TIM_Cmd(TIM2, DISABLE);
	return tCount;
}

uint32_t HIRES_Timer_GetCounter(void) {
	return TIM_GetCounter(TIM2);
}


/**
 * Init the Debug pin E7
 */
void Debug_IO_initalize(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable the GPIO Clocks */
	RCC_AHBPeriphClockCmd(DEBUG_GPIO_CLK, ENABLE);

// Debug pin
	GPIO_InitStructure.GPIO_Pin = DEBUG_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(DEBUG_GPIO_PORT, &GPIO_InitStructure);
// set Low
	GPIO_ResetBits(DEBUG_GPIO_PORT, DEBUG_PIN);
}
/** @} */
