#include <stdio.h>
#include "stm32f4xx.h"
#include <stdint.h>
#include "uart.h"
#include "mpu6500.h"
#include "systick.h"

int16_t x,y,z;
float xg,yg,zg;

MPU6500_Data_t imu_data;
MPU6500_Gyro_bias gyro_bias;
uint16_t gyro_calibrate_samples = 200;

volatile uint32_t systick_ms; // Elapsed time in ms
uint32_t last_imu_call = 0;

uint8_t data_rec[6];



int main(void)
{
	SysTick_Init();
	mpu6500_init();
	mpu6500_calibrate_gyro(gyro_calibrate_samples, &gyro_bias);

		while(1)

			{

			if ((systick_ms - last_imu_call) >= 1) // Schedules IMU reading every 1ms
					    {

							last_imu_call = systick_ms;

							mpu6500_sample(DATA_START_ADDR, &gyro_bias, &imu_data); // Read and store in data_rec

					    }
			}
}


// Counting time in ms to schedule task
void SysTick_Handler(void)
{
    systick_ms++;
}
