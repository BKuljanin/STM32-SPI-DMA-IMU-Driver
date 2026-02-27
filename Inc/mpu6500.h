/*
 * mpu6500.h
 *
 *  Created on: Jan 26, 2026
 *      Author: Bogdan Kuljanin
 */

#ifndef MPU6500_H_
#define MPU6500_H_

#include "spi.h"

#include <stdint.h>

// IMU data structure
typedef struct
{
    float a_x;
    float a_y;
    float a_z;

    float omega_x;
    float omega_y;
    float omega_z;

    float temp;

} MPU6500_Data_t;

// Gyroscope bias calibration structure
typedef struct
{
    int16_t omega_x_bias;
    int16_t omega_y_bias;
    int16_t omega_z_bias;

} MPU6500_Gyro_bias;


// The document MPU6500 will be refered
#define DEVID_R (0x75) // Device ID register WHO_AM_I (name)
#define ACC_DATA_FORMAT_R (0x1C) // Accelerometer measurement range
#define POWER_CTL_R (0x6B) // p8 register used to put the sensor into "Measurement Mode"
#define DATA_START_ADDR (0x3B) // Start address of the data register
#define GYRO_DATA_FORMAT_R (0x1B) // Gyroscope configuration register

#define ACC_FOUR_G (0x08) // +/-4g accelerometer measurement range
#define GYRO_500_DPS (0x08) // +/-500dps gyroscope measurement range
#define RESET (0x00)
#define SET_MEASURE_B (0x00)

#define READ_OPERATION 0x80

void mpu6500_init(void);
void mpu6500_read(uint8_t address, uint8_t *rxdata);
void mpu6500_calibrate_gyro(uint16_t gyro_samples, MPU6500_Gyro_bias *gyro_bias);
void mpu6500_sample(uint8_t address, MPU6500_Gyro_bias *gyro_bias, MPU6500_Data_t *imu_data);


#endif /* MPU6500_H_ */
