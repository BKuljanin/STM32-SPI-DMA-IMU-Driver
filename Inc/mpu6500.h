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


// The document MPU6500 will be refered
#define DEVID_R (0x75) //device ID register WHO_AM_I (name)
#define DEVICE_ADDR (0x68) // p44 0x68 or 0x69 depending on AD0 pin. address of the chip
#define DATA_FORMAT_R (0x1C) //// data format p6 how sensitive should the sensor be (range)
#define POWER_CTL_R (0x6B) // p8 register used to put the sensor into "Measurement Mode." Without writing to it, that sensor stays in standby.
#define DATA_START_ADDR (0x3B) // start address of the data register

#define FOUR_G (0x08) // 4g check this just in case
#define RESET (0x00)
#define SET_MEASURE_B (0x00) // wriging in power control // test, the original value is 0x00



void adxl_init(void);
void adxl_read(uint8_t address, uint8_t *rxdata);
//void adxl_write (uint8_t address, char value);

#endif /* MPU6500_H_ */
