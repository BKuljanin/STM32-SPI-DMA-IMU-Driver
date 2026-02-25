// now we want to receive the data on the microcontroller. We will shot that with the LED
// Run in debug mode and send data from putty/RealTerm

// The course originally uses adxl which is saved by the other name. this is adjusted to mpu6500
#include <stdio.h>
#include "stm32f4xx.h"
#include <stdint.h>
#include "uart.h"
#include "mpu6500.h"

int16_t x,y,z;
float xg,yg,zg;
//const float FOUR_G_SCALE_FACT = 0.000122070; // scale factor for 4g value

//void Clock_Init_16MHz(void);//test line

uint8_t data_rec[6]; //prveor extern sta je ovo je isto def u mpu6500.c
//extern char data;// TEST LINE DELETE


int main(void)
{
//	Clock_Init_16MHz(); // delete test line
	adxl_init();

		while(1)
	{
			adxl_read(DATA_START_ADDR, data_rec); // Read and store in data_rec
			x = ((data_rec[0]<<8) | data_rec[1]); // shiting to obtain x, mislim da je u 2 bita smesteno
			y = ((data_rec[2]<<8) | data_rec[3]); // multiplied to get unit in g
			z = ((data_rec[4]<<8) | data_rec[5]); // high byte first

			xg = x * 0.000122070;
			yg = y * 0.000122070;
			zg = z * 0.000122070;
	}
}


