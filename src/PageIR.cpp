/*
 * PageIR.cpp
 *
 * @date 16.12.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#include "Pages.h"
#include "main.h"  /* For RCC_Clocks */

#ifdef __cplusplus
extern "C" {
#include "irmp.h"
#include "irsnd.h"
#include "stm32f30x_syscfg.h"
#include "stm32f3_discovery.h"  /* For LEDx */
}
#endif

#define NUMBER_OF_IR_BUTTONS 4
#define IN_STOP_CODE 250 /* forbidden code - resets TV :-( */

void displayIRPage(void);
IRMP_DATA sIRReceiveData;
IRMP_DATA sIRSendData;

const char StringEZ[] = "EZ";
const char StringInStart[] = "In Start";
const char StringResend[] = "Resend";
const char StringCode[] = "Code";

const char * const IRButtonStrings[] = { StringEZ, StringInStart, StringCode, StringResend };

/**
 * IR send buttons
 */
static TouchButton * TouchButtonsIRSend[NUMBER_OF_IR_BUTTONS];

int sCode = 0;

void doIRButtons(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	if (aTheTouchedButton == TouchButtonsIRSend[0]) {
		sIRSendData.protocol = IRMP_NEC_PROTOCOL; // use NEC protocol
		sIRSendData.address = 64260; // set address LG 0XFB04
		sIRSendData.command = 0x00FF; // set command to EZ Adjust
		sIRSendData.flags = 0; // don't repeat frame
		//irsnd_send_data(&sIRSendData, TRUE); // send frame, wait for completion
		irsnd_send_data(&sIRSendData, FALSE); // send frame, do not wait for completion
	} else if (aTheTouchedButton == TouchButtonsIRSend[1]) {
		sIRSendData.protocol = IRMP_NEC_PROTOCOL; // use NEC protocol
		sIRSendData.address = 64260; // set address
		sIRSendData.command = 251; // set command to In  Start
		sIRSendData.flags = 3;
		irsnd_send_data(&sIRSendData, TRUE); // send frame, wait for completion
	} else if (aTheTouchedButton == TouchButtonsIRSend[2]) {
		float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, COLOR_GREEN);
		if (!isnan(tNumber) && tNumber != IN_STOP_CODE) {
			sCode = tNumber;
		}
		displayIRPage();
		assertParamMessage((sCode != IN_STOP_CODE), sCode, "");
		if (sCode != IN_STOP_CODE) {
			sIRSendData.protocol = IRMP_NEC_PROTOCOL; // use NEC protocol
			sIRSendData.address = 64260; // set address
			sIRSendData.command = sCode; // set command to
			sIRSendData.flags = 0; //  repeat frame
			irsnd_send_data(&sIRSendData, TRUE); // send frame, wait for completion
		}
	} else if (aTheTouchedButton == TouchButtonsIRSend[3]) {
		sIRSendData.protocol = IRMP_NEC_PROTOCOL; // use NEC protocol
		sIRSendData.address = 64260; // set address
		sIRSendData.command = sCode; // set command to Volume -
		sIRSendData.flags = 0; // repeat frame forever
		irsnd_send_data(&sIRSendData, TRUE); // send frame, wait for completion
	}

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * timer 15 update handler, called every 1/15000 sec
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
extern "C" void TIM1_BRK_TIM15_IRQHandler(void) {
// if irsnd_ISR not busy call irmp ISR
	if (!irsnd_ISR()) {
		irmp_ISR();
	}

	STM_EVAL_LEDToggle(LED7); // GREEN RIGHT
	TIM_ClearITPendingBit(TIM15, TIM_IT_Update);
}

void displayIRPage(void) {
	clearDisplay(COLOR_BACKGROUND_DEFAULT);
	initMainHomeButton(true);
	// draw all buttons
	for (unsigned int i = 0; i < NUMBER_OF_IR_BUTTONS; ++i) {
		TouchButtonsIRSend[i]->drawButton();
	}
}

/**
 * use TIM15 as timer and TIM17 as PWM generator
 */
void startIRPage(void) {

	TouchButton::setDefaultButtonColor(COLOR_GREEN);
	int tXPos = 0;
	for (unsigned int i = 0; i < NUMBER_OF_IR_BUTTONS; ++i) {
		TouchButtonsIRSend[i] = TouchButton::allocAndInitSimpleButton(tXPos, 0, BUTTON_WIDTH_4, BUTTON_HEIGHT_5, 0,
				IRButtonStrings[i], 1, i, &doIRButtons);
		tXPos += BUTTON_WIDTH_4 + BUTTON_DEFAULT_SPACING;
	}

// enable high sink capability
//	SYSCFG_I2CFastModePlusConfig(SYSCFG_I2CFastModePlus_PB9, ENABLE);
	irsnd_init();

//compute autoreload value for 16 bit timer without prescaler
	RCC_GetClocksFreq(&RCC_Clocks);
	IR_Timer_initialize((RCC_Clocks.PCLK2_Frequency / F_INTERRUPTS) - 1);
	IR_Timer_Start();
	displayIRPage();
}

//const char StringNEC[] = "NEC";

void loopIRPage(void) {
	if (irmp_get_data(&sIRReceiveData)) {
//		snprintf(StringBuffer, sizeof StringBuffer, "Addr=%6d|%4X Cmd=%4d|%4X %s F=%d", sIRReceiveData.address,
//				sIRReceiveData.address, sIRReceiveData.command, sIRReceiveData.command, sIRReceiveData.protocol,
//				irmp_protocol_names[sIRReceiveData.protocol], sIRReceiveData.flags);
		// for IHelicopter
		snprintf(StringBuffer, sizeof StringBuffer, "%3d|%2X %3d|%2X %1X|%1X %2X  %s F=%d", sIRReceiveData.address >> 8,
				sIRReceiveData.address >> 8, sIRReceiveData.address & 0xFF, sIRReceiveData.address & 0xFF,
				(sIRReceiveData.command >> 8) & 0xF0, (sIRReceiveData.command >> 8) & 0x0F, sIRReceiveData.command & 0xFF,
				irmp_protocol_names[sIRReceiveData.protocol], sIRReceiveData.flags);
		drawText(2, BUTTON_HEIGHT_5_LINE_2, StringBuffer, 1, COLOR_PAGE_INFO, COLOR_WHITE);
	}

	snprintf(StringBuffer, sizeof StringBuffer, "Cmd=%4d|%3X", sCode, sCode);
	drawText(2, BUTTON_HEIGHT_5_LINE_2 + 2 * FONT_HEIGHT, StringBuffer, 2, COLOR_PAGE_INFO, COLOR_WHITE);
}

/**
 * cleanup on leaving this page
 */
void stopIRPage(void) {
// free buttons
	for (unsigned int i = 0; i < NUMBER_OF_IR_BUTTONS; ++i) {
		TouchButtonsIRSend[i]->setFree();
	}
	IR_Timer_Stop();
//	SYSCFG_I2CFastModePlusConfig(SYSCFG_I2CFastModePlus_PB9, DISABLE);
	STM_EVAL_LEDOff(LED7);
}
