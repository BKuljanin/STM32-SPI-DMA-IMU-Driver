/*
 * mpu6500.c
 *
 *  Created on: Jan 26, 2026
 *      Author: Bogdan Kuljanin
 */


#include "mpu6500.h"

#define MULTI_BYTE_EN 0x40 // for adxl
#define READ_OPERATION 0x80 // for adxl

void adxl_read(uint8_t address, uint8_t *rxdata)
{
	// Set read operation
	address |= READ_OPERATION; // applies both for mpu and adxl

	// Enable multibyte
	//address |= MULTI_BYTE_EN;// Uncomment for adxl
	// Now final address has values of read operation and multi byte operation, only for adxl

	// Pull cs line low to enable slave
	cs_enable();

	// Send address
	spi1_transmit(&address,1); // Start a read operation from this register, and auto increment

	// Read 6 bytes
	spi1_receive(rxdata,6); // store received data in rxdata

	// Pull cs line high to disable slave
	cs_disable();

}
/* ADXL
void adxl_write (uint8_t address, char value)
{
	uint8_t data[2];

	// Enable multibyte, place address into buffer
	data[0] = address | MULTI_BYTE_EN; // address will be passed as an argument, then we perform OR with multibyte en. and place that to index 0
	// NOTE:
	// ADXL346: Bit 6 (0x40) in the SPI command enables multi-byte transfer (auto-increment of register address).
	// MPU6500: No multi-byte bit required — keeping CS low automatically allows consecutive register reads.

	// Place data into buffer
	data[1] = value;

	// Pull cs line low to enable slave
	cs_enable();

	// Transmit data and address
	spi1_transmit(data,2);

	// Pull cs line high to disable slave
	cs_disable();

}
*/

// MPU6500
void adxl_write (uint8_t address, char value)
{
	uint8_t data[2];

	// Enable multibyte, place address into buffer
	data[0] = address & 0x7F; // address will be passed as an argument, then we perform OR with multibyte en. and place that to index 0
	// NOTE:
	// ADXL346: Bit 6 (0x40) in the SPI command enables multi-byte transfer (auto-increment of register address).
	// MPU6500: No multi-byte bit required — keeping CS low automatically allows consecutive register reads.

	// Place data into buffer
	data[1] = value;

	// Pull cs line low to enable slave
	cs_enable();

	// Transmit data and address
	spi1_transmit(data,2);

	// Pull cs line high to disable slave
	cs_disable();

}


void adxl_init(void)
{
	// enable SPI gpio
	spi_gpio_init();

	// SPI config
	spi1_config();

	// set data format range +-4g
	adxl_write(DATA_FORMAT_R, FOUR_G);

	// reset all bits
	adxl_write(POWER_CTL_R, RESET);

	// configure power control measure bit
	adxl_write(POWER_CTL_R, SET_MEASURE_B);
}

