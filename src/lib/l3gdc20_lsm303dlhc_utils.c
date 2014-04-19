/*
 * @file l3gdc20_lsm303dlhc_utils.c
 *
 *  Created on: 22.01.2014
 * @author Armin Joachimsmeyer
 * armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#include "l3gdc20_lsm303dlhc_utils.h"
#include "stm32f3_discovery_lsm303dlhc.h"
#include "stm32f3_discovery_l3gd20.h"
#include "timing.h"

static int16_t AcceleratorZeroCompensation[3];
static int16_t GyroscopeZeroCompensation[3];
int16_t AcceleratorGyroscopeCompassRawDataBuffer[3];

#define LSM_Acc_Sensitivity_2g     (float)     1.0f            /*!< accelerometer sensitivity with 2 g full scale [LSB/mg] */
#define LSM_Acc_Sensitivity_4g     (float)     0.5f            /*!< accelerometer sensitivity with 4 g full scale [LSB/mg] */
#define LSM_Acc_Sensitivity_8g     (float)     0.25f           /*!< accelerometer sensitivity with 8 g full scale [LSB/mg] */
#define LSM_Acc_Sensitivity_16g    (float)     0.0834f         /*!< accelerometer sensitivity with 12 g full scale [LSB/mg] */

#define LSM303DLHC_CALIBRATION_OVERSAMPLING 8

/**
 * @brief Read LSM303DLHC output register, and calculate the acceleration ACC=(1/SENSITIVITY)* (out_h*256+out_l)/16 (12 bit rappresentation)
 * @param aAcceleratorRawData: pointer to float buffer where to store data
 * @retval None
 */
void readAcceleratorRaw(int16_t aAcceleratorRawData[]) {
    uint8_t ctrlx[2];
    uint8_t cDivider;
    uint8_t i = 0;

    /* Read the register content */
    LSM303DLHC_Read(ACC_I2C_ADDRESS, LSM303DLHC_CTRL_REG4_A, ctrlx, 2);
    LSM303DLHC_Read(ACC_I2C_ADDRESS, LSM303DLHC_OUT_X_L_A, (uint8_t *) aAcceleratorRawData, 6);

    if (ctrlx[1] & 0x40)
        // High resolution mode
        cDivider = 64;
    else
        cDivider = 16;

    for (i = 0; i < 3; i++) {
        aAcceleratorRawData[i] = aAcceleratorRawData[i] / cDivider;
    }
    // invert X value, it fits better :-)
    aAcceleratorRawData[0] = -aAcceleratorRawData[0];
}

void readAcceleratorZeroCompensated(int16_t * aAcceleratorRawData) {
    readAcceleratorRaw(aAcceleratorRawData);
    aAcceleratorRawData[0] -= AcceleratorZeroCompensation[0];
    aAcceleratorRawData[1] -= AcceleratorZeroCompensation[1];
    aAcceleratorRawData[2] -= AcceleratorZeroCompensation[2];
}

void readGyroscopeRaw(int16_t aGyroscopeRawData[]) {
    L3GD20_Read((uint8_t *) aGyroscopeRawData, L3GD20_OUT_X_L_ADDR, 6);
}

void readGyroscopeZeroCompensated(int16_t aGyroscopeRawData[]) {
    readGyroscopeRaw(aGyroscopeRawData);
    aGyroscopeRawData[0] -= GyroscopeZeroCompensation[0];
    aGyroscopeRawData[1] -= GyroscopeZeroCompensation[1];
    aGyroscopeRawData[2] -= GyroscopeZeroCompensation[2];
}

void setZeroAcceleratorGyroValue(void) {
    // use oversampling
    AcceleratorZeroCompensation[0] = 0;
    AcceleratorZeroCompensation[1] = 0;
    AcceleratorZeroCompensation[2] = 0;
    GyroscopeZeroCompensation[0] = 0;
    GyroscopeZeroCompensation[1] = 0;
    GyroscopeZeroCompensation[2] = 0;
    int i = 0;
    for (i = 0; i < LSM303DLHC_CALIBRATION_OVERSAMPLING; ++i) {
        // Accelerator
        readAcceleratorRaw(AcceleratorGyroscopeCompassRawDataBuffer);
        AcceleratorZeroCompensation[0] += AcceleratorGyroscopeCompassRawDataBuffer[0];
        AcceleratorZeroCompensation[1] += AcceleratorGyroscopeCompassRawDataBuffer[1];
        AcceleratorZeroCompensation[2] += AcceleratorGyroscopeCompassRawDataBuffer[2];
        // Gyroscope
        readGyroscopeRaw(AcceleratorGyroscopeCompassRawDataBuffer);
        GyroscopeZeroCompensation[0] += AcceleratorGyroscopeCompassRawDataBuffer[0];
        GyroscopeZeroCompensation[1] += AcceleratorGyroscopeCompassRawDataBuffer[1];
        GyroscopeZeroCompensation[2] += AcceleratorGyroscopeCompassRawDataBuffer[2];
        delayMillis(10);
    }
    AcceleratorZeroCompensation[0] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
    AcceleratorZeroCompensation[1] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
    AcceleratorZeroCompensation[2] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
    GyroscopeZeroCompensation[0] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
    GyroscopeZeroCompensation[1] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
    GyroscopeZeroCompensation[2] /= LSM303DLHC_CALIBRATION_OVERSAMPLING;
}

void readCompassRaw(int16_t aCompassRawData[]) {
    uint8_t ctrlx[2];
    uint8_t cDivider;
    uint8_t i = 0;

    /* Read the register content */
    LSM303DLHC_Read(ACC_I2C_ADDRESS, LSM303DLHC_CTRL_REG4_A, ctrlx, 2);
    LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_X_H_M, (uint8_t *) aCompassRawData, 6);

    if (ctrlx[1] & 0x40)
        // High resolution mode
        cDivider = 64;
    else
        cDivider = 16;

    for (i = 0; i < 3; i++) {
        aCompassRawData[i] = aCompassRawData[i] / cDivider;
    }
}

/*
 * 12 bit resolution 8 LSB/Centigrade
 */
int16_t readLSM303TempRaw(void) {
    uint8_t ctrlx[2];

    /* Read the register content */
    LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_TEMP_OUT_H_M, ctrlx, 2);
    return ctrlx[0] << 4 | ctrlx[1] >> 4;
//    return ctrlx[0] << 8 | ctrlx[1];
}

float readLSM303TempFloat(void) {
    float tTemp = readLSM303TempRaw();
    return tTemp / 2;
}

/**
 * @brief  calculate the magnetic field Magn.
 * @param  pfData: pointer to the data out
 * @retval None
 */
void Demo_CompassReadMag(float* pfData) {
    static uint8_t buffer[6] = { 0 };
    uint8_t CTRLB = 0;
    uint16_t Magn_Sensitivity_XY = 0, Magn_Sensitivity_Z = 0;
    uint8_t i = 0;
    LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_CRB_REG_M, &CTRLB, 1);

    LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_X_H_M, buffer, 1);
    LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_X_L_M, buffer + 1, 1);
    LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_Y_H_M, buffer + 2, 1);
    LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_Y_L_M, buffer + 3, 1);
    LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_Z_H_M, buffer + 4, 1);
    LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_Z_L_M, buffer + 5, 1);
    /* Switch the sensitivity set in the CRTLB*/
    switch (CTRLB & 0xE0) {
    case LSM303DLHC_FS_1_3_GA:
        Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_1_3Ga;
        Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_1_3Ga;
        break;
    case LSM303DLHC_FS_1_9_GA:
        Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_1_9Ga;
        Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_1_9Ga;
        break;
    case LSM303DLHC_FS_2_5_GA:
        Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_2_5Ga;
        Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_2_5Ga;
        break;
    case LSM303DLHC_FS_4_0_GA:
        Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_4Ga;
        Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_4Ga;
        break;
    case LSM303DLHC_FS_4_7_GA:
        Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_4_7Ga;
        Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_4_7Ga;
        break;
    case LSM303DLHC_FS_5_6_GA:
        Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_5_6Ga;
        Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_5_6Ga;
        break;
    case LSM303DLHC_FS_8_1_GA:
        Magn_Sensitivity_XY = LSM303DLHC_M_SENSITIVITY_XY_8_1Ga;
        Magn_Sensitivity_Z = LSM303DLHC_M_SENSITIVITY_Z_8_1Ga;
        break;
    }

    for (i = 0; i < 2; i++) {
        pfData[i] = (float) ((int16_t) (((uint16_t) buffer[2 * i] << 8) + buffer[2 * i + 1]) * 1000) / Magn_Sensitivity_XY;
    }
    pfData[2] = (float) ((int16_t) (((uint16_t) buffer[4] << 8) + buffer[5]) * 1000) / Magn_Sensitivity_Z;
}

/**
 * @brief Read LSM303DLHC output register, and calculate the acceleration ACC=(1/SENSITIVITY)* (out_h*256+out_l)/16 (12 bit rappresentation)
 * @param pfData: pointer to float buffer where to store data
 * @retval None
 */
void Demo_CompassReadAcc(float* pfData) {
    int16_t pnRawData[3];
    uint8_t ctrlx[2];
    uint8_t buffer[6], cDivider;
    uint8_t i = 0;
    float LSM_Acc_Sensitivity = LSM_Acc_Sensitivity_2g;

    /* Read the register content */
    LSM303DLHC_Read(ACC_I2C_ADDRESS, LSM303DLHC_CTRL_REG4_A, ctrlx, 2);
    LSM303DLHC_Read(ACC_I2C_ADDRESS, LSM303DLHC_OUT_X_L_A, buffer, 6);

    if (ctrlx[1] & 0x40)
        cDivider = 64;
    else
        cDivider = 16;

    /* check in the control register4 the data alignment*/
    if (!(ctrlx[0] & 0x40) || (ctrlx[1] & 0x40)) /* Little Endian Mode or FIFO mode */
    {
        for (i = 0; i < 3; i++) {
            pnRawData[i] = ((int16_t) ((uint16_t) buffer[2 * i + 1] << 8) + buffer[2 * i]) / cDivider;
        }
    } else /* Big Endian Mode */
    {
        for (i = 0; i < 3; i++)
            pnRawData[i] = ((int16_t) ((uint16_t) buffer[2 * i] << 8) + buffer[2 * i + 1]) / cDivider;
    }
    /* Read the register content */
    LSM303DLHC_Read(ACC_I2C_ADDRESS, LSM303DLHC_CTRL_REG4_A, ctrlx, 2);

    if (ctrlx[1] & 0x40) {
        /* FIFO mode */
        LSM_Acc_Sensitivity = 0.25;
    } else {
        /* normal mode */
        /* switch the sensitivity value set in the CRTL4*/
        switch (ctrlx[0] & 0x30) {
        case LSM303DLHC_FULLSCALE_2G:
            LSM_Acc_Sensitivity = LSM_Acc_Sensitivity_2g;
            break;
        case LSM303DLHC_FULLSCALE_4G:
            LSM_Acc_Sensitivity = LSM_Acc_Sensitivity_4g;
            break;
        case LSM303DLHC_FULLSCALE_8G:
            LSM_Acc_Sensitivity = LSM_Acc_Sensitivity_8g;
            break;
        case LSM303DLHC_FULLSCALE_16G:
            LSM_Acc_Sensitivity = LSM_Acc_Sensitivity_16g;
            break;
        }
    }

    /* Obtain the mg value for the three axis */
    for (i = 0; i < 3; i++) {
        pfData[i] = (float) pnRawData[i] / LSM_Acc_Sensitivity;
    }
}
