/*
 * assert.h
 *
* @date 19.02.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 */
#include <stddef.h>
#include "misc.h"


#ifndef ASSERT_H_
#define ASSERT_H_

#define USE_FULL_ASSERT    1

/* Exported macro ------------------------------------------------------------*/
#ifdef  USE_FULL_ASSERT

/**
 * @brief  The assert_param macro is used for function's parameters check.
 * @param  expr: If expr is false, it calls assert_failed function which reports
 *         the name of the source file and the source line number of the call
 *         that failed. If expr is true, it returns no value.
 * @retval None
 */
#define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))

#define failParamMessage(message,wrongParam) (assertFailedMessageParam((uint8_t *)__FILE__, __LINE__,getLR14(),message,(int)wrongParam))

#define assertParamMessage(expr,message,wrongParam) ((expr) ? (void)0 : assertFailedMessageParam((uint8_t *)__FILE__, __LINE__,getLR14(),message,(int)wrongParam))

/* Exported functions ------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif
// from misc.cpp
void assert_failed(uint8_t* file, uint32_t line);
void assertFailedMessageParam(uint8_t* aFile, uint32_t aLine, uint32_t aLinkRegister, const char * aMessage, int aWrongParameter);
#ifdef __cplusplus
}
#endif

#else
#define assert_param(expr) ((void)0)
#define assert_param_message(expr) ((void)0)
#endif /* USE_FULL_ASSERT */

#define ASSERT_START_Y 120

#endif /* ASSERT_H_ */
