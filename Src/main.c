
#include <stdio.h>
#include "stm32f4xx.h"
#include <stdint.h>
#include "uart.h"
#include "mpu6500.h"

int16_t x,y,z;
float xg,yg,zg;


uint8_t data_rec[6];


int main(void)
{

	adxl_init();

		while(1)
	{
			adxl_read(DATA_START_ADDR, data_rec); // Read and store in data_rec
			x = ((data_rec[0]<<8) | data_rec[1]); // shiting to obtain x
			y = ((data_rec[2]<<8) | data_rec[3]); // multiplied to get unit in g
			z = ((data_rec[4]<<8) | data_rec[5]); // high byte first

			xg = x * 0.000122070;
			yg = y * 0.000122070;
			zg = z * 0.000122070;
	}
}


