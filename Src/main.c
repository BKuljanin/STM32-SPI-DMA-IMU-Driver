#include <stdio.h>
#include "stm32f4xx.h"
#include <stdint.h>
#include "mpu6500.h"
#include "systick.h"
#include "exti.h"

int16_t x,y,z;
float xg,yg,zg;

MPU6500_Data_t imu_data;
MPU6500_IMU_bias imu_bias;
uint16_t imu_calibrate_samples = 200;

volatile uint32_t systick_ms; // Elapsed time in ms
uint32_t last_imu_call = 0;

uint8_t data_rec[15];
// 14 bytes for reading (2 per variable, acceleration (x,y,z), angular velocity (x,y,z) and temperature plus one dummy for clocking in write

static volatile uint8_t imu_dma_busy = 0;


int main(void)
{
	SysTick_Init();
	mpu6500_init();
	dma2_transfer_completeted_interrupt_enable();
	mpu6500_calibrate_imu(imu_calibrate_samples, &imu_bias);
	pc13_exti_init(); // PC13 for data ready interrupt
	dma2_stream_2_3_init(); // Initialize DMA

	while(1)
		{

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

		// Callback function to disable SPI DMA transfer and process the transfered data
		dma_callback(&imu_data, &imu_bias, data_rec);

		imu_dma_busy = 0; // Clearing busy flag when data is processed

	}
}


// Interrupt handler for this particular interrupt is EXTI15_10_...
void EXTI15_10_IRQHandler(void){

	// When interrupt occurs we enter this interrupt request handler function
	if ((EXTI->PR & LINE13) != 0)// We check if its from line 13
	{
		// Clear PR flag
		EXTI->PR |= LINE13;

		if (!imu_dma_busy)
		        {
		            imu_dma_busy = 1;
		            mpu6500_read(DATA_START_ADDR, data_rec, sizeof(data_rec));
		        }
	}
}
