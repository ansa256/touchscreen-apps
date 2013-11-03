/*
 * misc.cpp
 *
 * @date 21.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 */

#include "misc.h"
#include "assert.h"

#ifdef __cplusplus
extern "C" {

#include "stm32f30xPeripherals.h"
#include "timing.h"
#include "diskio.h"
#include "ff.h"

#include "stm32f3_discovery.h"
#include <stdio.h>
#include <string.h>

}
#endif

#include "Pages.h"

/*
 * buffers for any purpose...
 */
char StringBuffer[SIZEOF_STRINGBUFFER];
uint16_t FourDisplayLinesBuffer[SIZEOF_DISPLAYLINE_BUFFER];

/**
 * Strings for any purpose...
 */
const char StringEmpty[] = "";

// Single character strings
const char StringPlus[] = "+";
const char StringMinus[] = "-";
const char StringGreaterThan[] = ">";
const char StringLessThan[] = "<";
const char StringComma[] = ",";
const char StringDot[] = ".";
const char StringSign[] = "\xF1";
const char StringEnterChar[] = "\xD6";
const char StringHomeChar[] = "\xD3";
const char StringDoubleHorizontalArrow[] = "\xD7\xD8";

// Numeric character strings
const char String0[] = "0";
const char String1[] = "1";
const char String2[] = "2";
const char String3[] = "3";
const char String4[] = "4";
const char String5[] = "5";
const char String6[] = "6";
const char String7[] = "7";
const char String8[] = "8";
const char String9[] = "9";
const char String10[] = "10";
const char String20[] = "20";
const char String50[] = "50";
const char String100[] = "100";
const char String200[] = "200";
const char String500[] = "500";
const char String1000[] = "1000";

// miscellaneous strings
const char StringOn[] = "on";
const char StringOff[] = "off";
const char StringAuto[] = "auto";
const char StringMan[] = "man";
const char StringSmall[] = "small";
const char StringLarge[] = "large";
const char StringLow[] = "low";
const char StringMid[] = "mid";
const char StringHigh[] = "high";
const char StringStart[] = "Start";
const char StringStop[] = "Stop";
const char StringClear[] = "Clear";
const char StringNext[] = "Next";
const char StringContinue[] = "Continue";
const char StringBack[] = "Back";
const char StringCancel[] = "Cancel";
const char StringHome[] = "Home";
const char StringMain[] = "Main";
const char StringSettings[] = "Settings";
const char StringCalibration[] = "Calibration";
const char StringLoad[] = "Load";
const char StringStore[] = "Store";
const char StringExport[] = "Export";
const char StringExtern[] = "Extern";
const char StringTest[] = "Test";
const char StringError[] = "Error";
const char StringOK[] = "OK";
const char StringZero[] = "Zero";

const char StringVolt[] = "Volt";
const char StringMilliOhm[] = "mOhm";

// date strings
const char StringClock[] = "clock";
const char StringYear[] = "year";
const char StringMonth[] = "month";
const char StringDay[] = "day";
const char StringHour[] = "hour";
const char StringMinute[] = "minute";
const char StringMin[] = "min";
const char StringSecond[] = "second";
const char * DateStrings[] = { StringClock, StringSecond, StringMinute, StringHour, StringDay, StringMonth, StringYear };

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  aFile: pointer to the source file name
 * @param  aLine: assert_param error line source number
 * @retval None
 */
extern "C" void assert_failed(uint8_t* aFile, uint32_t aLine) {
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	if (isInitializedHY32D) {
		char * tFile = (strrchr(const_cast<char*>((char*) aFile), '/') + 1);
		snprintf(StringBuffer, sizeof StringBuffer, "Wrong parameters value on line: %lu\nfile: %s", aLine, tFile);
		drawMLText(0, ASSERT_START_Y, DISPLAY_WIDTH, ASSERT_START_Y + (4 * FONT_HEIGHT), StringBuffer, 1, COLOR_RED, COLOR_WHITE);
		delayMillis(2000);
	} else {
		/* Infinite loop */
		while (1) {
			STM_EVAL_LEDToggle(LED6); // GREEN LEFT
			/* Insert delay */
			delayMillis(500);
		}
	}
}

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 *         usage: assertFailedMessageParam((uint8_t *) __FILE__, __LINE__, getLR14(), "Message", tValue);
 * @param aFile pointer to the source file name
 * @param aLine
 * @param aLinkRegister
 * @param aMessage
 * @param aWrongParameter
 */
extern "C" void assertFailedMessageParam(uint8_t* aFile, uint32_t aLine, uint32_t aLinkRegister, const char * aMessage,
		int aWrongParameter) {
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	if (isInitializedHY32D) {
		char * tFile = (strrchr(const_cast<char*>((char*) aFile), '/') + 1);
		snprintf(StringBuffer, sizeof StringBuffer, "%s on line: %lu\nfile: %s\nval: %#X %d LR=%#X", aMessage, aLine, tFile,
				aWrongParameter, aWrongParameter, (unsigned int) aLinkRegister);
		drawMLText(0, ASSERT_START_Y, DISPLAY_WIDTH, ASSERT_START_Y + (4 * FONT_HEIGHT), StringBuffer, 1, COLOR_RED, COLOR_WHITE);
		delayMillis(2000);
	} else {
		/* Infinite loop */
		while (1) {
			STM_EVAL_LEDToggle(LED6); // GREEN LEFT
			/* Insert delay */
			delayMillis(500);
		}
	}
}

extern "C" void FaultHandler(unsigned int * aFaultArgs) {
//From http://blog.frankvh.com/2011/12/07/cortex-m3-m4-hard-fault-handler/
//****************************************************
//To test this application, you can use this snippet anywhere:
// //Let's crash the MCU!
// asm (" MOVS r0, #1 \n"
// " LDM r0,{r1-r2} \n"
// " BX LR; \n");
// Note that this works as-is in IAR 5.20, but could have a different syntax
// in other compilers.
//****************************************************
// aFaultArgs is a pointer to your stack. you should change the adress
// assigned if your compiler places your stack anywhere else!

	uint32_t stacked_r0;
	uint32_t stacked_r1;
	uint32_t stacked_r2;
	uint32_t stacked_r3;
	uint32_t stacked_r12;
	uint32_t stacked_lr;
	uint32_t stacked_pc;
	uint32_t stacked_psr;

	stacked_r0 = ((uint32_t) aFaultArgs[0]);
	stacked_r1 = ((uint32_t) aFaultArgs[1]);
	stacked_r2 = ((uint32_t) aFaultArgs[2]);
	stacked_r3 = ((uint32_t) aFaultArgs[3]);

	stacked_r12 = ((uint32_t) aFaultArgs[4]);
	stacked_lr = ((uint32_t) aFaultArgs[5]);
	stacked_pc = ((uint32_t) aFaultArgs[6]);
	stacked_psr = ((uint32_t) aFaultArgs[7]);

	uint tIndex = 0;
	tIndex += snprintf(StringBuffer, sizeof StringBuffer, "R0=%lX R1=%lX\n", stacked_r0, stacked_r1);
	tIndex += snprintf(&StringBuffer[tIndex], sizeof StringBuffer - tIndex, "R2=%lX R3=%lX R12=%lX\n", stacked_r2, stacked_r3,
			stacked_r12);
	tIndex += snprintf(&StringBuffer[tIndex], sizeof StringBuffer - tIndex, "LR [R14]=%lX PC=%lX\nMSP=%lX PSP=%lX\n", stacked_lr,
			stacked_pc, __get_MSP(), __get_PSP());

	// Vector numbers
//	0 = Thread mode
//	1 = Reserved
//	2 = NMI
//	3 = HardFault
//	4 = MemManage
//	5 = BusFault
//	6 = UsageFault
//	7-10 = Reserved
//	11 = SVCall
//	12 = Reserved for Debug
//	13 = Reserved
//	14 = PendSV
//	15 = SysTick
//	16 = IRQ0.

	tIndex += snprintf(&StringBuffer[tIndex], sizeof StringBuffer - tIndex, "PSR=%lX VectNr.=%X ICSR=%lX\nPendVectNr.=%X\n",
			stacked_psr, (char) ((*((volatile uint32_t *) (0xE000ED04))) & 0xFF), (*((volatile uint32_t *) (0xE000ED04))),
			(unsigned short) (((*((volatile uint32_t *) (0xE000ED04))) >> 12) & 0x3F));

	IRQn_Type tIRQNr = (IRQn_Type) (((*((volatile long *) (0xE000ED04))) & 0xFF) - 16);
	if (tIRQNr == UsageFault_IRQn) {
		tIndex += snprintf(&StringBuffer[tIndex], sizeof StringBuffer - tIndex, "UsageFSR=%Xn",
				(*((volatile unsigned short *) (0xE000ED2A))));
	} else if (tIRQNr == BusFault_IRQn) {
		tIndex += snprintf(&StringBuffer[tIndex], sizeof StringBuffer - tIndex, "BusFAR=%lX BusFSR=%X\n",
				(*((volatile uint32_t *) (0xE000ED38))), (*((volatile char *) (0xE000ED29))));
	} else if (tIRQNr == MemoryManagement_IRQn) {
		tIndex += snprintf(&StringBuffer[tIndex], sizeof StringBuffer - tIndex, "MemManFSR=%X\n",
				(*((volatile char *) (0xE000ED28))));
	}
	// Hard Fault is not defined in stmf30x.h :-(
	else if (tIRQNr == MemoryManagement_IRQn - 1) {
		tIndex += snprintf(&StringBuffer[tIndex], sizeof StringBuffer - tIndex, "HardFSR=%lX\n",
				(*((volatile uint32_t *) (0xE000ED2C))));
	}

	uint32_t tAuxFaultStatusRegister = (*((volatile uint32_t *) (0xE000ED3C)));
	if ((*((volatile uint32_t *) (0xE000ED3C))) != 0x00000000) {
		// makes only sense if != 0
		tIndex += snprintf(&StringBuffer[tIndex], sizeof StringBuffer - tIndex, "AuxFSR=%lX\n", tAuxFaultStatusRegister);
	}
	drawMLText(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, StringBuffer, 1, COLOR_RED, COLOR_WHITE);
	while (1) {
	}

}
