#include "mpu6500.h"



void adxl_read(uint8_t address, uint8_t *rxdata)
{
	// Set read operation
	address |= READ_OPERATION;

	// Pull cs line low to enable slave
	cs_enable();

	// Send address
	spi1_transmit(&address,1); // Start a read operation from this register, and auto increment

	// Read 6 bytes
	spi1_receive(rxdata,6); // store received data in rxdata

	// Pull cs line high to disable slave
	cs_disable();

}


void adxl_write (uint8_t address, char value)
{
	uint8_t data;

	// Enable multibyte, place address into buffer
	data = address & 0x7F; // address will be passed as an argument, then we perform OR with multibyte en. and place that to index 0

	// Place data into buffer
	data = value;

	// Pull cs line low to enable slave
	cs_enable();

	// Transmit address
	spi1_transmit(address,1);

	// Transmit data
	spi1_transmit(value,1);

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

