/*
 * stm32f3_discovery_leds_buttons.c
 *
 * @date 02.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

/**
 * LED   | COLOR / POS  | SOURCE 				| INFO
 * ------|--------------|-----------------------|--------
 * LED6  | GREEN LEFT   | Assertion but display not available |
 * LED7  | GREEN FRONT  | TIM6_DAC_IRQHandle 	| not used yet
 * LED8  | ORANGE FRONT |  						| Timeout
 * LED9  | BLUE FRONT   | EXTI1 				| Touch interrupt
 * LED10 | RED FRONT    | EXTI4 				| MMC Card inserted
 */
#include "stm32f3_discovery.h"
#include "timing.h"
#include "stm32f30xPeripherals.h"
#include "misc.h"

void initializeLEDs(void) {
	/* Initialize LEDs and User Button available on STM32F3-Discovery board */
	STM_EVAL_LEDInit(LED3);
	STM_EVAL_LEDInit(LED4);
	STM_EVAL_LEDInit(LED5);
	STM_EVAL_LEDInit(LED6);
	STM_EVAL_LEDInit(LED7);
	STM_EVAL_LEDInit(LED8);
	STM_EVAL_LEDInit(LED9);
	STM_EVAL_LEDInit(LED10);
}

void allLedsOff(void) {
	/* LEDs Off */
	STM_EVAL_LEDOff(LED3);
	STM_EVAL_LEDOff(LED6);
	STM_EVAL_LEDOff(LED7);
	STM_EVAL_LEDOff(LED4);
	STM_EVAL_LEDOff(LED10);
	STM_EVAL_LEDOff(LED8);
	STM_EVAL_LEDOff(LED9);
	STM_EVAL_LEDOff(LED5);
}
void toggleAllLeds(void) {
	/* LEDs Off */
	STM_EVAL_LEDOff(LED3);
	STM_EVAL_LEDOff(LED6);
	STM_EVAL_LEDOff(LED7);
	STM_EVAL_LEDOff(LED4);
	STM_EVAL_LEDOff(LED10);
	STM_EVAL_LEDOff(LED8);
	STM_EVAL_LEDOff(LED9);
	STM_EVAL_LEDOff(LED5);
}

void CycleLEDs(void) {
	/* Toggle LD3 */
	STM_EVAL_LEDToggle(LED3); // RED BACK
	/* Insert 50 ms delay */
	delayMillis(50);
	/* Toggle LD5 */
	STM_EVAL_LEDToggle(LED5); // ORANGE BACK
	/* Insert 50 ms delay */
	delayMillis(50);
	/* Toggle LD7 */
	STM_EVAL_LEDToggle(LED7); // GREEN RIGHT
	/* Insert 50 ms delay */
	delayMillis(50);
	/* Toggle LD9 */
	STM_EVAL_LEDToggle(LED9); // BLUE FRONT
	/* Insert 50 ms delay */
	delayMillis(50);
	/* Toggle LD10 */
	STM_EVAL_LEDToggle(LED10); // RED FRONT
	/* Insert 50 ms delay */
	delayMillis(50);
	/* Toggle LD8 */
	STM_EVAL_LEDToggle(LED8); // ORANGE FRONT
	/* Insert 50 ms delay */
	delayMillis(50);
	/* Toggle LD6 */
	STM_EVAL_LEDToggle(LED6); // GREEN LEFT
	/* Insert 50 ms delay */
	delayMillis(50);
	/* Toggle LD4 */
	STM_EVAL_LEDToggle(LED4); // BLUE BACK
	/* Insert 50 ms delay */
	delayMillis(50);
}

/**
 * @brief  This function handles EXTI0_IRQ Handler.
 * Input is active high, and will be discharged by a resistor.
 * @retval None
 */
void EXTI0_IRQHandler(void) {
//	STM_EVAL_LEDToggle(LED3); // RED BACK
	if ((EXTI_GetITStatus(USER_BUTTON_EXTI_LINE ) == SET) && (STM_EVAL_PBGetState(BUTTON_USER) != RESET)) {
		/* Delay */
		delayMillis(5);

		// Wait for SEL button to be pressed (high)
		setTimeoutMillis(10);
		while (STM_EVAL_PBGetState(BUTTON_USER) == RESET) {
			if (isTimeout(0)) {
				break;
			}
		}
		storeScreenshot();

	}
	/* Clear the EXTI line pending bit */
	EXTI_ClearITPendingBit(USER_BUTTON_EXTI_LINE );
}
