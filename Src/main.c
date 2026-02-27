#include <stdio.h>
#include "stm32f4xx.h"
#include <stdint.h>
#include "mpu6500.h"
#include "systick.h"
#include "exti.h"

int16_t x,y,z;
float xg,yg,zg;

MPU6500_Data_t imu_data;
MPU6500_Gyro_bias gyro_bias;
uint16_t gyro_calibrate_samples = 200;

volatile uint32_t systick_ms; // Elapsed time in ms
uint32_t last_imu_call = 0;

uint8_t data_rec[15];
// 14 bytes for reading (2 per variable, acceleration (x,y,z), angular velocity (x,y,z) and temperature plus one dummy for clocking in write

static volatile uint8_t imu_dma_busy = 0;

volatile uint8_t debug=0;

int main(void)
{
	SysTick_Init();
	dma2_stream_2_3_init();
	mpu6500_init();
	dma2_transfer_completeted_interrupt_enable();
	pc13_exti_init();
	//mpu6500_calibrate_gyro(gyro_calibrate_samples, &gyro_bias);

	while(1)
		{
		/*
			if ((systick_ms - last_imu_call) >= 1) // Schedules IMU reading every 1ms (1kHz)
				{
					last_imu_call = systick_ms;
					mpu6500_sample(DATA_START_ADDR, &gyro_bias, &imu_data); // Read and store in data_rec
				}
		*/
		}
}


// Counting time in ms to schedule task
void SysTick_Handler(void)
{
    systick_ms++;
}


// We have to implement DMA interrupt request handler. Only for receive stream
void DMA2_Stream2_IRQHandler(void)
{
	// Checking if its transfer complete interrupt that has occurred. If that is the case run the code
	if(DMA2->LISR & HISR_TCIF2) // Reference manual p223 DMA low interrupt status register, bit 21 transfer completed stream 2
	{
		// Clear flag
		DMA2->LIFCR |= HIFCR_CTCIF2;// Reference manual p225
		//debug = 1;
		// Callback function to disable SPI DMA transfer and process the transfered data
		dma_callback(&imu_data, &gyro_bias, data_rec);

		imu_dma_busy = 0; // Clearing busy flag when data is processed
		//debug = 2;
	}
}


// Interrupt handler for this particular interrupt is EXTI15_10_...
void EXTI15_10_IRQHandler(void){

	// When interrupt occurs we enter this interrupt request handler function
	if ((EXTI->PR & LINE13) != 0)// We check if its from line 13
	{
		// Clear PR flag
		EXTI->PR |= LINE13;
		debug=3;
		if (!imu_dma_busy)
		        {
		            imu_dma_busy = 1;
		            mpu6500_read(DATA_START_ADDR, data_rec, sizeof(data_rec));
		            debug = 4;
		        }
	}
}
