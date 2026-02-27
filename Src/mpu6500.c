#include "mpu6500.h"


void mpu6500_read(uint8_t address, uint8_t *rxdata)
{	/* rxdata contains additional byte at [0] position. That byte is dummy used to store read values during write
	operation where we clock out address byte, during that time we receive data (because it's full duplex) from the slave that we don't need.
	Later that byte is skipped in the processing of the data. */

	// Set read operation
	address |= READ_OPERATION;

	txdata[0] = address;

	for (int i = 1; i < sizeof(rxdata)-1; i++) // Generating (size+1)-1 additional dummy bytes, so in total there is the same amount to write and read
	{
	    txdata[i] = 0xFF;         // Dummy bytes that are sent during read operation
	}

	// Pull cs line low to enable slave
	cs_enable();

	// Set DMA transfer length
	set_dma_transfer_length(sizeof(rxdata));	// Setting size to read bytes length +1. That +1 is for write operation that goes before reading

	// Set DMA memory source for transmit and receive
	set_dma_source(rxdata, txdata);

	// Enable DMA
	dma2_enable();

}


void mpu6500_write (uint8_t address, uint8_t value)
{

	 uint8_t data[2];
	 uint8_t dummy_rx; // In writing there is no need to read received data, it's just configuring registers of the slave

	 data[0] = address & 0x7F; // 0 on MSB for writing, for MPU6500
	 data[1] = value;			// Data we are writing

	// Pull cs line low to enable slave
	cs_enable();

	// Set DMA transfer length
	set_dma_transfer_length(sizeof(data));

	// Set DMA memory source for transmit and receive
	set_dma_source(dummy_rx, data);	// Receive to dummy data and send data which we constructed

	// Enable DMA
	dma2_enable();

}


void mpu6500_init(void)
{
	// enable SPI gpio
	spi_gpio_init();

	// SPI config
	spi1_config();

	// Wake up sensor
	mpu6500_write(POWER_CTL_R, RESET);

	// Set data format of accelerometer to range +-4g
	mpu6500_write(ACC_DATA_FORMAT_R, ACC_FOUR_G);

	// Set data format of gyroscope to range +/-500dps
	mpu6500_write(GYRO_DATA_FORMAT_R, GYRO_500_DPS);

}

// Reads and processes the reading of IMU
void mpu6500_sample(uint8_t address, MPU6500_Gyro_bias *gyro_bias, MPU6500_Data_t *imu_data)
{
	 int16_t accel_data;
	 int16_t gyro_data;
	 int16_t temp_data;
	 uint8_t data_rec[14];

	 mpu6500_read(address, data_rec);

	 // Reading x component of the acceleration
	 accel_data = ((int16_t)data_rec[0]<<8) | data_rec[1]; // Shifting data bytes to get data (data stored in 2 bytes for each component)
	 imu_data->a_x = accel_data * 0.000122070; // This is scaling for +/-4g range as per datasheet

	 // Reading y component of the acceleration
	 accel_data = ((int16_t)data_rec[2]<<8) | data_rec[3];
	 imu_data->a_y = accel_data * 0.000122070;

	 // Reading z component of the acceleration
	 accel_data = ((int16_t)data_rec[4]<<8) | data_rec[5];
	 imu_data->a_z = accel_data * 0.000122070;

	 // Reading temperature
	 temp_data = ((int16_t)data_rec[6]<<8) | data_rec[7];
	 imu_data->temp = (temp_data / 333.87) + 21.0;

	 // Reading x component of the angular velocity
	 gyro_data = ((int16_t)data_rec[8]<<8) | data_rec[9];
	 imu_data->omega_x = (gyro_data - gyro_bias->omega_x_bias) / 65.6;	// This is scaling for +/-500 dps as per datasheet

	 // Reading y component of the angular velocity
	 gyro_data = ((int16_t)data_rec[10]<<8) | data_rec[11];
	 imu_data->omega_y = (gyro_data - gyro_bias->omega_y_bias) / 65.6;

	 // Reading z component of the angular velocity
	 gyro_data = ((int16_t)data_rec[12]<<8) | data_rec[13];
	 imu_data->omega_z = (gyro_data - gyro_bias->omega_z_bias) / 65.6;

}


void mpu6500_calibrate_gyro(uint16_t gyro_samples, MPU6500_Gyro_bias *gyro_bias)
{
	int32_t sum_x = 0, sum_y = 0, sum_z = 0;
	uint8_t imu_data[14];

    for (int i = 0; i < gyro_samples; i++)	// Reading defined number of gyroscope samples, while drone is steady
    {

		mpu6500_read(DATA_START_ADDR,  imu_data); // Reading data to array

		for (volatile int d = 0; d < 1000; d++) {} // Short delay after reading

		int16_t gx = (int16_t)((imu_data[8]  << 8) | imu_data[9]);  // Reading x component of the angular velocity
		int16_t gy = (int16_t)((imu_data[10] << 8) | imu_data[11]); // Reading y component of the angular velocity
		int16_t gz = (int16_t)((imu_data[12] << 8) | imu_data[13]); // Reading z component of the angular velocity

		sum_x += gx;	// Calculating sum in each iteration
		sum_y += gy;
		sum_z += gz;

    }

    gyro_bias->omega_x_bias = sum_x / gyro_samples;	// Divide the sum with samples number to obtain bias
    gyro_bias->omega_y_bias = sum_y / gyro_samples;
    gyro_bias->omega_z_bias = sum_z / gyro_samples;

}
