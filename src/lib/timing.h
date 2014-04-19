/*
 * timing.h
 *
 * @date 05.12.2012
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#ifndef TIMING_H_
#define TIMING_H_

#include "stm32f30x.h"
#include "misc.h"
#include <stdbool.h>

// some microsecond values for timing
#define ONE_SECOND 1000
#define TWO_SECONDS 2000
#define FIVE_SECONDS 5000
#define TEN_SECONDS 10000
#define THIRTY_SECONDS 30000
#define ONE_MINUTE 60000
#define TWO_MINUTES 120000
#define TEN_MINUTES 600000

#define SYSCLK_VALUE (HSE_VALUE*9) // compile time value is 72 MHZ / runtime value is RCC_Clocks.SYSCLK_Frequency
#define NANOS_PER_CLOCK (1000000000/SYSCLK_VALUE)
#define PICOS_PER_CLOCK (1000000000000/SYSCLK_VALUE)

#define OFFSET_INTERRUPT_TYPE_TO_ISR_INT_NUMBER 0x10
// Value for set...Milliseconds in order to disable Timer
#define DISABLE_TIMER_DELAY_VALUE (0)

#define SYS_TICK_INTERRUPT_PRIO 0x04 //0x04 -> preemptive prio: 1 of 0-3 / subprio: 0 of 0-3
#define PREEMPTIVE_PRIO_MASK 0x0C // mask preemptive prio bits
#define PREEMPTIVE_PRIO_SHIFT 2 // shift preemptive prio bits
#define NUMBER_OF_PREEMPTIVE_PRIOS 4

extern volatile uint32_t TimeoutCounterForThread;

uint32_t LSM303DLHC_TIMEOUT_UserCallback(void);
uint32_t L3GD20_TIMEOUT_UserCallback(void);

void setTimeoutMillis(int32_t aTimeMillis);
#define isTimeout(aLinkRegister) (isTimeoutVerbose((uint8_t *)__FILE__, __LINE__,aLinkRegister,2000))
#define isTimeoutShowDelay(aLinkRegister, aMessageDisplayTimeMillis) (isTimeoutVerbose((uint8_t *)__FILE__, __LINE__,aLinkRegister,aMessageDisplayTimeMillis))
bool isTimeoutSimple(void);
bool isTimeoutVerbose(uint8_t* file, uint32_t line, uint32_t aLinkRegister, int32_t aMessageDisplayTimeMillis);

void delayNanos(int32_t aTimeNanos);

void delayMillis(int32_t aTimeMillis);
void registerDelayCallback(void (*aGenericCallback)(void), int32_t aTimeMillis);
void changeDelayCallback(void (*aGenericCallback)(void), int32_t aTimeMillis);

void doOneSystic(void);

uint32_t getMillisSinceBoot(void);


#define NANOS_ONE_LOOP 139
void displayTimings(uint16_t aYDisplayPos);
void testTimingsLoop(int aCount);

#endif /* TIMING_H_ */
