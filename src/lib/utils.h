/*
 * utils.h
 *
 *  Created on: 15.12.2013
 *      Author: Armin
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>

#define THOUSANDS_SEPARATOR '.'

void showRTCTime(uint16_t x, uint16_t y, uint16_t aColor, uint16_t aBackColor, bool aForceDisplay);
void showRTCTimeEverySecond(uint16_t x, uint16_t y, uint16_t aColor, uint16_t aBackColor);
void formatThousandSeparator(char * aDestPointer, char * aSeparatorAddress);

#endif /* UTILS_H_ */
