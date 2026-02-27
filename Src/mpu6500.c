#include "mpu6500.h"
#include "spi.h"

static inline void dma2_clear_spi1_flags(void)
{
    // Stream2 flags (TCIF2 etc.) are in LIFCR bits around 16..21
    DMA2->LIFCR = (1U<<16)|(1U<<18)|(1U<<19)|(1U<<20)|(1U<<21); // Stream2 clear all

    // Stream3 flags are in LIFCR bits around 22..27
    DMA2->LIFCR = (1U<<22)|(1U<<24)|(1U<<25)|(1U<<26)|(1U<<27); // Stream3 clear all
}

void mpu6500_read(uint8_t address, uint8_t *rxdata, uint16_t len)
{	/* rxdata contains additional byte at [0] position. That byte is dummy used to store read values during write
	operation where we clock out address byte, during that time we receive data (because it's full duplex) from the slave that we don't need.
	Later that byte is skipped in the processing of the data. */

	// Set read operation
	address |= READ_OPERATION;

	static uint8_t txdata[15];
	// IMPORTANT: This buffer must be static (or global) because DMA continues to read from this memory after the function returns; if it were a local
	// stack variable, its memory could be reused/overwritten, causing corrupted transfers or hard faults

	txdata[0] = address;

	for (int i = 1; i < len; i++) // Generating 14 additional dummy bytes, so in total there is the same amount to write and read
	{
	    txdata[i] = 0xFF;         // Dummy bytes that are sent during read operation
	}

	// Pull cs line low to enable slave
	cs_enable();

	// Disable DMA
	dma2_disable();

	dma2_clear_spi1_flags(); // test line

	// Set DMA transfer length
	set_dma_transfer_length(len);	// Setting size to read bytes length +1. That +1 is for write operation that goes before reading

	// Set DMA memory source for transmit and receive
	set_dma_source(rxdata, txdata);

	// Enable DMA
	dma2_enable();

}


void mpu6500_write (uint8_t address, uint8_t value)
{

	 static uint8_t data[2];
	 static uint8_t dummy_rx; // In writing there is no need to read received data, it's just configuring registers of the slave
	 // IMPORTANT: This buffer must be static (or global) because DMA continues to read from this memory after the function returns; if it were a local
	 // stack variable, its memory could be reused/overwritten, causing corrupted transfers or hard faults

	 data[0] = address & 0x7F; // 0 on MSB for writing, for MPU6500
	 data[1] = value;			// Data we are writing

	// Pull cs line low to enable slave
	cs_enable();

	// Disable DMA
	dma2_disable();

	// Set DMA transfer length
	set_dma_transfer_length(sizeof(data));

	// Set DMA memory source for transmit and receive
	set_dma_source(&dummy_rx, data);	// Receive to dummy data and send data which we constructed

	// Enable DMA
	dma2_enable();

}

void mpu6500_read_blocking(uint8_t address, uint8_t *rxdata)
{
	// Set read operation
	address |= READ_OPERATION;

	// Pull cs line low to enable slave
	cs_enable();

	// Send address
	spi1_transmit_blocking(&address, 1); // Start a read operation from this register, and auto increment

	// Read 14 bytes
	spi1_receive_blocking(rxdata, 14); // store received data in rxdata, 14 bytes - 7 variables, 2 bytes each (x,y,z acceleration, x,y,z angular velocity, temperature)

	// Pull cs line high to disable slave
	cs_disable();

}


void mpu6500_write_blocking(uint8_t address, uint8_t value)
{

	 uint8_t reg = address & 0x7F; // 0 on MSB for writing, for MPU6500
	 uint8_t val = value;

	// Pull cs line low to enable slave
	cs_enable();

	// Transmit address
	spi1_transmit_blocking(&reg, 1);

	// Transmit data
	spi1_transmit_blocking(&val, 1);

	// Pull cs line high to disable slave
	cs_disable();

}


void mpu6500_init(void)
{
	// enable SPI gpio
	spi_gpio_init();

	// SPI config
	spi1_config();

	// Wake up sensor
	mpu6500_write_blocking(POWER_CTL_R, RESET);

	// Set data format of accelerometer to range +-4g
	mpu6500_write_blocking(ACC_DATA_FORMAT_R, ACC_FOUR_G);

	// Set data format of gyroscope to range +/-500dps
	mpu6500_write_blocking(GYRO_DATA_FORMAT_R, GYRO_500_DPS);

	// Enable interrupt when data ready
	mpu6500_write_blocking(INTERRUPT_EN_R, INTERRUPT_EN);

	mpu6500_write_blocking(0x37, 0x00);   // INT_PIN_CFG test line

}

// Reads and processes the reading of IMU
void mpu6500_process(MPU6500_IMU_bias *imu_bias, MPU6500_Data_t *imu_data, uint8_t *data_rec)
{
	 // Processes received data from the array data_rec and writes IMU reading to imu_data variable

	 // Reading x component of the acceleration
	 int16_t ax_raw = (int16_t)((data_rec[1] << 8) | data_rec[2]); // Shifting data bytes to get data (data stored in 2 bytes for each component)
	 imu_data->a_x = (ax_raw - imu_bias->a_x_bias) * 0.000122070; // This is scaling for +/-4g range as per datasheet*/

	 // Reading y component of the acceleration
	 int16_t ay_raw = (int16_t)((data_rec[3] << 8) | data_rec[4]);
	 imu_data->a_y = (ay_raw - imu_bias->a_y_bias) * 0.000122070;

	 // Reading z component of the acceleration
	 int16_t az_raw = (int16_t)((data_rec[5] << 8) | data_rec[6]);
	 imu_data->a_z = (az_raw - imu_bias->a_z_bias) * 0.000122070;

	 // Reading temperature
	 int16_t temp_raw = (int16_t)((data_rec[7] << 8) | data_rec[8]);
	 imu_data->temp = (temp_raw / 333.87) + 21.0;

	 // Reading x component of the angular velocity
	 int16_t omegax_raw = (int16_t)((data_rec[9] << 8) | data_rec[10]);
	 imu_data->omega_x = (omegax_raw - imu_bias->omega_x_bias) / 65.6;
	 // This is scaling for +/-500 dps as per datasheet

	 // Reading y component of the angular velocity
	 int16_t omegay_raw = (int16_t)((data_rec[11] << 8) | data_rec[12]);
	 imu_data->omega_y = (omegay_raw - imu_bias->omega_y_bias) / 65.6;

	 // Reading z component of the angular velocity
	 int16_t omegaz_raw = (int16_t)((data_rec[13] << 8) | data_rec[14]);
	 imu_data->omega_z = (omegaz_raw - imu_bias->omega_z_bias) / 65.6;

	 // Each data measurement is represented with 2 bytes. The measurement is reconstructed by moving MSB byte left and appending LSB on right.
	 // data_rec[0] is skipped since that byte is filled during slave internal register configuration write. That is a dummy byte. The rest are data bytes
}


void mpu6500_calibrate_imu(uint16_t calibration_samples, MPU6500_IMU_bias *imu_bias)
{
	int32_t sum_ax = 0, sum_ay = 0, sum_az = 0, sum_omegax = 0, sum_omegay = 0, sum_omegaz = 0;
	uint8_t imu_data[14];

    for (int i = 0; i < calibration_samples; i++)	// Reading defined number of gyroscope samples, while drone is steady
    {

		mpu6500_read_blocking(DATA_START_ADDR,  imu_data); // Reading data to array

		for (volatile int d = 0; d < 1000; d++) {} // Short delay after reading

		int16_t ax = (int16_t)((imu_data[0]  << 8) | imu_data[1]);  // Reading x component of the angular velocity
		int16_t ay = (int16_t)((imu_data[2] << 8) | imu_data[3]); // Reading y component of the angular velocity
		int16_t az = (int16_t)((imu_data[4] << 8) | imu_data[5]); // Reading z component of the angular velocity
		int16_t omegax = (int16_t)((imu_data[8]  << 8) | imu_data[9]);  // Reading x component of the angular velocity
		int16_t omegay = (int16_t)((imu_data[10] << 8) | imu_data[11]); // Reading y component of the angular velocity
		int16_t omegaz = (int16_t)((imu_data[12] << 8) | imu_data[13]); // Reading z component of the angular velocity

		sum_omegax += omegax;	// Calculating sum in each iteration
		sum_omegay += omegay;
		sum_omegaz += omegaz;
		sum_ax += ax;
		sum_ay += ay;
		sum_az += az;

    }

    imu_bias->omega_x_bias = sum_omegax / calibration_samples;	// Divide the sum with samples number to obtain bias
    imu_bias->omega_y_bias = sum_omegay / calibration_samples;
    imu_bias->omega_z_bias = sum_omegaz / calibration_samples;
    imu_bias->a_x_bias = sum_ax / calibration_samples;
    imu_bias->a_y_bias = sum_ay / calibration_samples;
    imu_bias->a_z_bias = (sum_az / calibration_samples) - 8192;	// For z calibration is done for g, this is g before scaling (in +/-4g range)

}


// DMA completed interrupt
void dma_callback(MPU6500_Data_t *imu_data, MPU6500_IMU_bias *imu_bias, uint8_t *data_rec)
{
    // 1) Wait until SPI finished shifting out the last bit
    while (SPI1->SR & SR_BSY) {}

    // 2) Clear possible OVR (safe even if not set)
    volatile uint8_t tmp;
    tmp = SPI1->DR;
    tmp = SPI1->SR;
    (void)tmp;

	// Disable DMA
	dma2_disable();

	// Pull CS line high to disable slave
	cs_disable();

	// Handles disabling SPI slave and DMA transfer. Takes received data, processes it and writes IMU data to the variable of type MPU6500_Data_t
	mpu6500_process(imu_bias, imu_data, data_rec);

}



