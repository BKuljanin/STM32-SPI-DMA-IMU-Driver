#include <stdio.h>
#include "stm32f4xx.h"
#include <stdint.h>
#include "mpu6500.h"
#include "exti.h"

MPU6500_Data_t imu_data; 	// Structure where IMU readings are stored
MPU6500_IMU_bias imu_bias;	// Structure where IMU bias are stored
uint16_t imu_calibrate_samples = 200;	// Number of IMU samples for calibration
uint8_t data_rec[15]; // 14 bytes for reading (2 per variable, acceleration (x,y,z), angular velocity (x,y,z) and temperature plus one dummy for clocking in write

static volatile uint8_t imu_dma_busy = 0; // DMA busy flag


int main(void)
{
	mpu6500_init();	// Initialize the IMU by configuring its registers
	dma2_transfer_completeted_interrupt_enable();	// Enabling DMA transfer completed interrupt
	mpu6500_calibrate_imu(imu_calibrate_samples, &imu_bias); // Calibration of the IMU
	pc13_exti_init(); // PC13 for data ready interrupt
	dma2_stream_2_3_init(); // Initialize DMA

	while(1)
		{

		}
}


// Implementing DMA interrupt request handler, only for receive stream
void DMA2_Stream2_IRQHandler(void)
{
	// Checking if its transfer complete interrupt that has occurred. If that is the case run the code
	if(DMA2->LISR & HISR_TCIF2) // Reference manual p223 DMA low interrupt status register, bit 21 transfer completed stream 2
	{
		// Clear flag
		DMA2->LIFCR |= HIFCR_CTCIF2;// Reference manual p225

		// Callback function to disable SPI DMA transfer and process the transfered data
		dma_callback(&imu_data, &imu_bias, data_rec);

		imu_dma_busy = 0; // Clearing busy flag when data is processed

	}
}


// Interrupt handler for INT pin (data ready) of the IMU
void EXTI15_10_IRQHandler(void){

	// When interrupt occurs we enter this interrupt request handler function
	if ((EXTI->PR & LINE13) != 0)	// We check if the interrupt is from line 13
	{
		// Clear PR flag
		EXTI->PR |= LINE13;

		if (!imu_dma_busy)
		        {
		            imu_dma_busy = 1;
		            mpu6500_read(DATA_START_ADDR, data_rec, sizeof(data_rec)); // Start sensor reading via SPI DMA
		        }
	}
}
