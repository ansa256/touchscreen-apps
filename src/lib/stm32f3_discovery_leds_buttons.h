/*
 * @file stm32f3_discovery_leds_buttons.h
 *
* @date 02.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#ifndef STM32F3_DISCOVERY_LEDS_H_
#define STM32F3_DISCOVERY_LEDS_H_

extern volatile uint32_t UserButtonPressed;


void initializeLEDs(void);
void allLedsOff(void);
void CycleLEDs(void);
void toggleAllLeds(void);

#endif /* STM32F3_DISCOVERY_LEDS_H_ */
