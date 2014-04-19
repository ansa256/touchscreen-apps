/*
 * @file l3gdc20_lsm303dlhc_utils.h
 *
 *  Created on: 22.01.2014
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#ifndef L3GDC20_LSM303DLHC_UTILS_H_
#define L3GDC20_LSM303DLHC_UTILS_H_

#include <stdint.h>

#define PI                         (float)     3.14159265f

extern int16_t AcceleratorGyroscopeCompassRawDataBuffer[3];
void readAcceleratorRaw(int16_t * aAcceleratorRawData);
void readAcceleratorZeroCompensated(int16_t* aAcceleratorRawData);
void readGyroscopeRaw(int16_t* aGyroscopeRawData);
void readGyroscopeZeroCompensated(int16_t* aGyroscopeRawData);
void setZeroAcceleratorGyroValue(void);
void readCompassRaw(int16_t * aCompassRawData);
int16_t readLSM303TempRaw(void);
float readLSM303TempFloat(void);

// Demo functions
void Demo_CompassReadAcc(float* pfData);

#endif /* L3GDC20_LSM303DLHC_UTILS_H_ */
