/*
 * misc.cpp
 *
 * @date 21.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#include "misc.h"
#include "assert.h"

extern "C" {
#include "stm32f30xPeripherals.h"
#include "timing.h"
#include "diskio.h"
#include "ff.h"

#include "stm32f3_discovery.h"
#include <stdio.h>
#include <string.h>
}

#include "Pages.h"

int sLockCount = 0; // counts skipped drawChars because of display locks

int DebugValue1;
int DebugValue2;
int DebugValue3;
int DebugValue4;
int DebugValue5;

/*
 * buffers for any purpose...
 */
char StringBuffer[SIZEOF_STRINGBUFFER];
uint16_t FourDisplayLinesBuffer[SIZEOF_DISPLAYLINE_BUFFER];

/**
 * Strings for any purpose...
 */
const char StringEmpty[] = "";
const char StringSpace[] = " ";

// Single character strings
const char StringPlus[] = "+";
const char StringMinus[] = "-";
const char StringPlusMinus[] = "\xB1"; // +- character
const char StringGreaterThan[] = ">";
const char StringLessThan[] = "<";
const char StringComma[] = ",";
const char StringDot[] = ".";
const char StringHomeChar[] = "\xD3";
const char StringDoubleHorizontalArrow[] = "\xAB\xBB";

// Numeric character strings

// miscellaneous strings

const char StringStart[] = "Start";
const char StringStop[] = "Stop";
const char StringClear[] = "Clear";
const char StringBack[] = "Back";

const char StringHome[] = "Home";
const char StringSettings[] = "Settings";
const char StringMore[] = "More";
const char StringCalibration[] = "Calibration";
const char StringLoad[] = "Load";
const char StringStore[] = "Store";
const char StringTest[] = "Test";
const char StringInfo[] = "Info";
const char StringError[] = "Error";
const char StringOK[] = "OK";
const char StringZero[] = "Zero";

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  aFile: pointer to the source file name
 * @param  aLine: assert_param error line source number
 * @retval false
 */
extern "C" bool assert_failed(uint8_t* aFile, uint32_t aLine) {
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    if (isDisplayAvailable) {
        char * tFile = (strrchr(const_cast<char*>((char*) aFile), '/') + 1);
        snprintf(StringBuffer, sizeof StringBuffer, "Wrong parameters value on line: %lu\nfile: %s", aLine, tFile);
        drawMLText(0, ASSERT_START_Y, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_WHITE);
        delayMillis(2000);
    } else {
        /* Infinite loop */
        while (1) {
            STM_EVAL_LEDToggle(LED6); // GREEN LEFT
            /* Insert delay */
            delayMillis(500);
        }
    }
    return false;
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
 * @retval false
 *
 */
extern "C" bool assertFailedParamMessage(uint8_t* aFile, uint32_t aLine, uint32_t aLinkRegister, int aWrongParameter,
        const char * aMessage) {
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    if (isDisplayAvailable) {
        char * tFile = (strrchr(const_cast<char*>((char*) aFile), '/') + 1);
        snprintf(StringBuffer, sizeof StringBuffer, "%s on line: %lu\nfile: %s\nval: %#X %d LR=%#X", aMessage, aLine, tFile,
                aWrongParameter, aWrongParameter, (unsigned int) aLinkRegister);
#ifdef LOCAL_DISPLAY_EXISTS
        // reset lock (just in case...)
        sDrawLock = 0;
#endif
        drawMLText(0, ASSERT_START_Y, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_WHITE);
        delayMillis(2000);
    } else {
        /* Infinite loop */
        while (1) {
            STM_EVAL_LEDToggle(LED6); // GREEN LEFT
            /* Insert delay */
            delayMillis(500);
        }
    }
    return false;
}
extern "C" void errorMessage(const char * aMessage) {
    if (isDisplayAvailable) {
        drawMLText(0, ASSERT_START_Y, aMessage, TEXT_SIZE_11, COLOR_RED, COLOR_WHITE);
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
#ifdef LOCAL_DISPLAY_EXISTS
    // reset lock (just in case...)
    sDrawLock = 0;
#endif
    drawMLText(0, TEXT_SIZE_11_ASCEND, StringBuffer, TEXT_SIZE_11, COLOR_RED, COLOR_WHITE);
    while (1) {
    }
}
