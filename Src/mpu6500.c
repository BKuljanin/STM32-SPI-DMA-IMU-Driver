#include "mpu6500.h"



void mpu6500_read(uint8_t address, uint8_t *rxdata)
{
	// Set read operation
	address |= READ_OPERATION;

	// Pull cs line low to enable slave
	cs_enable();

	// Send address
	spi1_transmit(&address, 1); // Start a read operation from this register, and auto increment

	// Read 14 bytes
	spi1_receive(rxdata, 14); // store received data in rxdata, 14 bytes - 7 variables, 2 bytes each (x,y,z acceleration, x,y,z angular velocity, temperature)

	// Pull cs line high to disable slave
	cs_disable();

}


void mpu6500_write (uint8_t address, uint32_t value)
{

	 uint8_t reg = address & 0x7F; // 0 on MSB for writing, for MPU6500
	 uint8_t val = value;

	// Pull cs line low to enable slave
	cs_enable();

	// Transmit address
	spi1_transmit(&reg, 1);

	// Transmit data
	spi1_transmit(&val, 1);

	// Pull cs line high to disable slave
	cs_disable();

}


void mpu6500_init(void)
{
	// enable SPI gpio
	spi_gpio_init();

	// SPI config
	spi1_config();

	// set data format range +-4g
	mpu6500_write(DATA_FORMAT_R, FOUR_G);

	// reset all bits
	mpu6500_write(POWER_CTL_R, RESET);

	// configure power control measure bit
	mpu6500_write(POWER_CTL_R, SET_MEASURE_B);
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
	 accel_data = ((int16_t)data_rec[0]<<8) + data_rec[1]; // Shifting data bytes to get data (data stored in 2 bytes for each component)
	 imu_data->a_x = accel_data * 0.000122070;

	 // Reading y component of the acceleration
	 accel_data = ((int16_t)data_rec[2]<<8) + data_rec[3];
	 imu_data->a_y = accel_data * 0.000122070;

	 // Reading z component of the acceleration
	 accel_data = ((int16_t)data_rec[4]<<8) + data_rec[5];
	 imu_data->a_z = accel_data * 0.000122070;

	 // Reading temperature
	 temp_data = ((int16_t)data_rec[6]<<8) + data_rec[7];
	 imu_data->temp = (temp_data / 333.87) + 21.0;

	 // Reading x component of the angular velocity
	 gyro_data = ((int16_t)data_rec[8]<<8) + data_rec[9];
	 imu_data->omega_x = (gyro_data - gyro_bias->omega_x_bias) / 65.6;

	 // Reading y component of the angular velocity
	 gyro_data = ((int16_t)data_rec[10]<<8) + data_rec[11];
	 imu_data->omega_y = (gyro_data - gyro_bias->omega_y_bias) / 65.6;

	 // Reading z component of the angular velocity
	 gyro_data = ((int16_t)data_rec[12]<<8) + data_rec[13];
	 imu_data->omega_z = (gyro_data - gyro_bias->omega_z_bias) / 65.6;

}


void mpu6500_calibrate_gyro(uint16_t gyro_samples, MPU6500_Gyro_bias *gyro_bias)
{
	int32_t sum_x = 0, sum_y = 0, sum_z = 0;
	uint8_t imu_data[14];

    for (int i = 0; i < gyro_samples; i++)	// Reading defined number of gyroscope samples, while drone is steady
    {

		mpu6500_read(DATA_START_ADDR,  imu_data); // Reading data to array

		for(int i = 0; i < 100; i++){} // Short delay after reading

		// Reading x component of the angular velocity
		sum_x += (imu_data[8]<<8) + imu_data[9];	// Calculating sum in each iteration

		// Reading y component of the angular velocity
		sum_y += (imu_data[10]<<8) + imu_data[11];

		// Reading z component of the angular velocity
		sum_z += (imu_data[12]<<8) + imu_data[13];

    }

    gyro_bias->omega_x_bias = sum_x / gyro_samples;	// Divide the sum with samples number to obtain bias
    gyro_bias->omega_y_bias = sum_y / gyro_samples;
    gyro_bias->omega_z_bias = sum_z / gyro_samples;

}
