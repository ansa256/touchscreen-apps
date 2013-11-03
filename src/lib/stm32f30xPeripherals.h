/**
 * @file stm32f30xPeripherals.h
 *
 * @date 05.12.2012
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 *
 * Port pin assignment
 * -------------------
 *
 * Port	|Pin|Device|Function
 * -----|---|------|--------
 * A	|0	|Button	|INT input
 * A	|1	|ADC1	|Channel 2 / DSO-Input 1
 * A	|2	|ADC1	|Channel 3 / DSO-Input 2
 * A	|3	|ADC1	|Channel 4 / DSO-Input 3
 * A	|4	|DAC	|Output 1
 * A	|5	|SPI1	|CLK
 * A	|6	|SPI1	|MISO
 * A	|7	|SPI1	|CLK
 * A	|8	|	|
 * A	|9	|	|
 * A	|10	|Freq Synth	|TIM2_CH4
 * A	|11	|USB	|DM
 * A	|12 |USB	|DP
 * A	|13 |SWD	|IO
 * A	|14 |SWD	|CLK
 * A	|15	|	|
 * &nbsp;|&nbsp;|&nbsp;|&nbsp;
 * B	|0	|HY32D		|CS
 * B	|1	|ADS7846	|INT input
 * B	|2	|ADS7846	|CS
 * B	|4	|HY32D		|DATA
 * B	|5	|HY32D		|WR
 * B	|6	|I2C		|CLK
 * B	|7	|I2C		|SDA
 * B	|8	|CAN		|RX - not used yet
 * B	|9	|CAN		|TX - not used yet
 * B	|10	|HY32D		|RD
 * B	|13-15	|SPI2	|not used yet
 * &nbsp;|&nbsp;|&nbsp;|&nbsp;
 * C	|0	|ADC2		|Channel 6 / AccuCap-Input 1
 * C	|1	|ADC2		|Channel 7 / AccuCap-Input 2
 * C	|4	|SD CARD	|INT input / CardDetect
 * C	|5	|SD CARD	|CS
 * C	|6	|TIM3		|Tone signal output
 * C	|7,8,9	|DSO	|Attenuator control
 * C	|10	|USART		|TX - not used yet
 * C	|11	|USART		|RX - not used yet
 * C	|12	|DSO		|AC mode of preamplifier
 * C	|13	|DSO		|
 * C	|14	|Intern		|Clock
 * C	|15	|Intern		|Clock
 * &nbsp;|&nbsp;|&nbsp;|&nbsp;
 * D 	|0-15 |HY32D  	|16 bit data
 * &nbsp;|&nbsp;|&nbsp;|&nbsp;
 * E	|0	|L3GD20		|INT input 1
 * E	|1	|L3GD20		|INT input 2
 * E	|2	|LSM303		|DRDY
 * E	|3	|L3GD20		|CS
 * E	|4	|LSM303		|INT input 1
 * E	|5	|LSM303		|INT input 2
 * E	|6	|		 	|
 * E	|7	|DEBUG		|
 * E	|8-15|LED		|Onboard LEDs
 * &nbsp;|&nbsp;|&nbsp;|&nbsp;
 * F full	|0,1|Clock		|Clock generator
 * F	|2	|RELAY		|Mode data acquisition Channel 1
 * F	|4	|RELAY		|Mode data acquisition Channel 2
 * F	|6	|HY32D		|TIM4 PWM output
 * F	|9	|RELAY		|Start/Stop data acquisition Channel 1
 * F	|10	|RELAY		|Start/Stop data acquisition Channel 2
 *
 *
 *
 *  Timer usage
 * -------------------
 *
 * Timer	|Function
 * ---------|--------
 * 2	    | Frequency synthesizer
 * 3	    | PWM tone generation -> Pin C6
 * 4	    | PWM backlight led -> Pin F6
 * 6	    | ADC Timebase
 * 7	    | DAC Timebase
 *
 *
 *  Interrupt priority (lower value is higher priority)
 * ---------------------
 * 2 bits for pre-emption priority + 2 bits for subpriority
 *
 * Prio	| ISR Nr| Name  			| Usage
 * -----|---------------------------|-------------
 * 0 1	| 0x22 | ADC1_2_IRQn		| ADC EOC
 * 0 1	| 0x1B | DMA1_Channel1_IRQn	| ADC DMA
 * 1 0	| 0x0F | SysTick_IRQn		| SysTick
 * 1 0	| 0x2A | USBWakeUp_IRQn		|
 * 2 0	| 0x24 | USB_LP_CAN1_RX0_IRQn|
 * 3 3	| 0x16 | EXTI0				| User button
 * 3 3	| 0x46 | TIM6_DAC_IRQn		| ADC Timer - not used yet
 * 3 3	| 0x17 | EXTI1				| Touch
 * 3 3	| 0x1A | EXTI4				| MMC card detect
 *
 */

#ifndef STM32F30XPERIPHERALS_H_
#define STM32F30XPERIPHERALS_H_

#include "stm32f30x.h"
#include <stdbool.h>

#include "integer.h" // for get_fattime
#include <time.h>

/** @addtogroup Peripherals_Library
 * @{
 */

typedef enum {
	LOW = 0, HIGH
} IOLevel;

/* DEBUG */
#define DEBUG_PIN                             GPIO_Pin_7
#define DEBUG_GPIO_PORT                       GPIOE
#define DEBUG_GPIO_CLK                        RCC_AHBPeriph_GPIOE

#define Set_DebugPin() (DEBUG_GPIO_PORT->BSRR = DEBUG_PIN)
#define Reset_DebugPin() (DEBUG_GPIO_PORT->BRR = DEBUG_PIN)
void Debug_IO_initalize(void);

#define HY32D_CS_PIN                          GPIO_Pin_0
#define HY32D_CS_GPIO_PORT                    GPIOB
#define HY32D_CS_GPIO_CLK                     RCC_AHBPeriph_GPIOB

#define HY32D_DATA_CONTROL_PIN                GPIO_Pin_4
#define HY32D_DATA_CONTROL_GPIO_PORT          GPIOB

#define HY32D_RD_PIN                          GPIO_Pin_10
#define HY32D_RD_GPIO_PORT                    GPIOB

#define HY32D_WR_PIN                          GPIO_Pin_5
#define HY32D_WR_GPIO_PORT                    GPIOB

#define HY32D_DATA_GPIO_PORT          		  GPIOD
#define HY32D_DATA_GPIO_CLK                   RCC_AHBPeriph_GPIOD

void HY32D_IO_initalize(void);

void ADS7846_IO_initalize(void);
bool ADS7846_getInteruptLineLevel(void);
void ADS7846_ClearITPendingBit(void);
void ADS7846_clearAndEnableInterrupt(void);
void ADS7846_disableInterrupt(void);
void ADS7846_CSEnable(void);
void ADS7846_CSDisable(void);

void AccuCapacity_IO_initalize(void);
void AccuCapacity_SwitchDischarge(unsigned int aChannelIndex, bool aActive);
void AccuCapacity_SwitchCharge(unsigned int aChannelIndex, bool aActive);

unsigned int AccuCapacity_ADCRead(uint8_t aProbeNumber);

void Gyroscope_IO_initialize(void);
void GyroscopeAndSPIInitialize(void);
void initAcceleratorCompassChip(void);

// ADC
#define DSO_ADC_ID   		   	ADC1
#define ADC1_INPUT1_CHANNEL     ADC_Channel_2
#define ADC1_INPUT2_CHANNEL     ADC_Channel_3
#define ADC1_INPUT3_CHANNEL     ADC_Channel_4

#define DSO_DMA_CHANNEL   		DMA1_Channel1
#define ADC_DEFAULT_TIMEOUT    2 // in microseconds
#define ADC_MAX_CONVERSION_VALUE (4096 -1)

void ADC12_init(void);
void ADC_init(void);
void ADC_enableAndWait(ADC_TypeDef* aADCId);
void ADC_disableAndWait(ADC_TypeDef* aADCId);
void ADC2_init(void);
void ADC_SetChannelSampleTime(ADC_TypeDef* aADCId, uint8_t aChannelNumber, bool aFastMode);
void ADC_enableEOCInterrupt(ADC_TypeDef* aADCId);
void ADC_disableEOCInterrupt(ADC_TypeDef* aADCId);
void ADC_clearITPendingBit(ADC_TypeDef* aADCId);
uint32_t ADC_getCFGR(void);

// ADC attenuator and range
void DSO_initializeAttenuatorAndAC(void);
void DSO_setAttenuator(uint8_t aValue);
void DSO_setACRange(bool aValue);

// ADC timer
void ADC_initalizeTimer6();
void ADC_Timer6EnableInterrupt(void);
void ADC_SetTimer6Period(uint16_t Autoreload, uint16_t aPrescaler);
void ADC_startTimer6();
void ADC_stopTimer6();
void ADC_SetClockPrescaler(uint32_t aValue);

// ADC DMA
void ADC_DMA_initialize(void);
void ADC_DMA_start(uint32_t aMemoryBaseAddr, uint16_t aBufferSize);
void ADC_DMA_stop(void);
uint16_t ADC_DMA_GetCurrDataCounter(void);

uint16_t ADC1_getChannelValue(uint8_t aChannel);
void ADC_setRawToVoltFactor(void);
extern float sVrefVoltage;
extern float ADCToVoltFactor;  // Factor for ADC Reading -> Volt
#define ADC_SCALE_FACTOR_SHIFT 18
extern unsigned int sADCScaleFactorShift18; // Factor for ADC Reading -> Display calibrated for sReading3Volt -> 240,  shift 18 because we have 12 bit ADC and 12 + 18 + 2 = 32 bit register
extern uint16_t sReading3Volt; // Approximately 4096

//SPI
void SPI1_initialize(void);
void SPI1_setPrescaler(uint16_t aPrescaler);
uint16_t SPI1_getPrescaler(void);
uint8_t SPI1_sendReceive(uint8_t byte);
uint8_t SPI1_sendReceiveFast(uint8_t byte);

//RTC
void RTC_initialize_LSE(void);
DWORD get_fattime(void);
void fillTimeStructure(struct tm *aTimeStructurePtr);
int RTC_getTimeStringForFile(char * aStringBuffer);
int RTC_getDateStringForFile(char * aStringBuffer);
int RTC_getTimeString(char * StringBuffer);
void RTC_setTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t dayOfWeek, uint8_t day, uint8_t month, uint16_t year);
#define RTC_BKP_DR_NUMBER   16  /* RTC Backup Data Register Number */
void RTC_WriteBackupDataRegister(uint8_t aIndex, uint32_t aRTCBackupData);
uint32_t RTC_ReadBackupDataRegister(uint8_t aIndex);

// Misc
__STATIC_INLINE uint32_t getSysticValue(void) {
	return SysTick ->VAL;
}
__STATIC_INLINE uint32_t getSysticReloadValue(void) {
	return SysTick ->LOAD;
}
__STATIC_INLINE void clearSystic(void) {
	SysTick ->VAL = 0;
}
__STATIC_INLINE bool hasSysticCounted(void) {
	return (SysTick ->CTRL & SysTick_CTRL_COUNTFLAG_Msk);
}

// Tone
#define FEEDBACK_TONE_NO_ERROR 0
#define FEEDBACK_TONE_SHORT_ERROR 1
#define FEEDBACK_TONE_LONG_ERROR 2
#define FEEDBACK_TONE_NO_TONE 7

void initalizeTone(void);
void tone(uint16_t aFreqHertz, uint16_t aDurationMillis);
void FeedbackToneOK(void);
void FeedbackTone(unsigned int aFeedbackType);
void EndTone(void);
void noTone(void);

// PWM
void PWM_initalize(void);
void PWM_setOnRatio(uint32_t power);

void setTimeoutLED(void);
void resetTimeoutLED(void);

void DAC_init(void);
void DAC_Timer_initialize(uint32_t aPeriod);
void DAC_Timer_SetReloadValue(uint32_t aReloadValue);
void DAC_Start(void);
void DAC_Stop(void);
void DAC_ModeTriangle(void);
void DAC_ModeNoise(void);
void DAC_TriangleAmplitude(unsigned int aAmplitude);
void DAC_SetOutputValue(uint16_t aOutputValue);

void HIRES_Timer_initialize(uint32_t aPeriod);
void HIRES_Timer_SetReloadValue(uint32_t aReloadValue);
void HIRES_Timer_Start(void);
uint32_t HIRES_Timer_Stop(void);
uint32_t HIRES_Timer_GetCounter(void);

void MICROSD_IO_initalize(void);
bool MICROSD_isCardInserted(void);
void MICROSD_CSEnable(void);
void MICROSD_CSDisable(void);
void MICROSD_ClearITPendingBit(void);

/** @} */

#endif /* STM32F30XPERIPHERALS_H_ */
