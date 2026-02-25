#include "stm32f4xx.h"

#define GPIOBEN (1U<<1)
#define I2C1EN (1U<<21)
#define CR1_PE (1U<<0)

//read function
#define SR2_BUSY (1U<<1)
#define CR1_START (1U<<8)
#define SR1_SB (1U<<0)
#define SR1_ADDR (1U<<1)
#define SR1_TXE (1U<<7)
#define CR1_ACK (1U<<10)
#define CR1_STOP (1U<<9)
#define SR1_RXNE (1U<<6)
#define SR1_BTF (1U<<2)

#define I2C_100KHZ 80 //0B 0101 000 decimal = 80
#define SD_MODE_MAX_RISE_TIME 17 // various tools compute this value
/* Pinout
 * PB8-----SCL
 * PB9-----SDA
 * */

void I2C1_init(void)
{

	// Enable clock access for gpiob
	RCC->AHB1ENR |= GPIOBEN;

	// Configure SCL and SDA lines
	// Datasheet p58 alternate function PB8 SCL, PB9 SDA
	GPIOB->MODER &=~(1U<<16);
	GPIOB->MODER |=(1U<<17);
	GPIOB->MODER &=~(1U<<18);
	GPIOB->MODER |=(1U<<19); // in mode register

	// Set output type PB8 and PB9 to open drain (required for i2c), ref manual p186, write 1 for open drain
	GPIOB->OTYPER |= (1U<<8); // OT 8 and OT9 for PB8 and PB9
	GPIOB->OTYPER |= (1U<<9); // proveri zasto open drain

	// Enable Pullup for PB8,PB9
	GPIOB->PUPDR |= (1U<<16); // PB8 bits 16,17
	GPIOB->PUPDR &=~ (1U<<17); // 01 pullup

	GPIOB->PUPDR |= (1U<<18);
	GPIOB->PUPDR &=~ (1U<<19);

	// set alternate function type
	GPIOB->AFR[1] &=~ (1U<<0);
	GPIOB->AFR[1] &=~ (1U<<1);
	GPIOB->AFR[1] |= (1U<<2);
	GPIOB->AFR[1] &=~ (1U<<3);

	GPIOB->AFR[1] &=~ (1U<<4);
	GPIOB->AFR[1] &=~ (1U<<5);
	GPIOB->AFR[1] |= (1U<<6);
	GPIOB->AFR[1] &=~ (1U<<7);

	RCC->APB1ENR |= I2C1EN; // Enable clock access for i2c1, ref manual p146 bit 21

	// Enter reset mode
	I2C1->CR1 |= (1U<<15); // when set i2c is in reset state

	// Exit reset mode
	I2C1->CR1 &= ~(1U<<15); // when set i2c is in reset state

	/*Configure peripheral clock*/
	I2C1->CR2 = (1U<<4);// i2c cr 2 bus frequency. Rference manual p765. 16MHz (10000 binary 5.th bit position 5)
	//If clock is 1MHz simply write 1 since its all MHz, set bit 0 to 1; if 2Mhz write to bit 1 (boolean 2)

	I2C1->CCR = I2C_100KHZ; //CCR bits 0-11 I2C clock. We want standard mode 100kHz, 16MHz/(2*80) = 100kHz

	I2C1->TRISE = SD_MODE_MAX_RISE_TIME;// rise time, reference manual p772,773. The time it takes for the I²C signal (SDA or SCL) to go from LOW to HIGH

	//Enable i2c
	I2C1->CR1 |= CR1_PE;
}

// read funciton
// 3 arguments, 1st address of slave, 2nd , 3rd pointer where to store
void I2C1_byteRead(char saddr, char maddr, char* data){

	volatile int tmp;

	while(I2C1->SR2 & (SR2_BUSY)){} // make sure bus is not busy, check busy flag. sr2 reg bit 1 busy

	I2C1->CR1 |= CR1_START; // once its not busy, generate start condition, set start bit in cr1. bit 8 start bit p763

	// wait until start bit set
	while(!(I2C1->SR1 & (SR1_SB))){}// ref man p767-770 sb start bit 0; 1: start condition generated

	// transmit slave address + write
	I2C1->DR = saddr <<1; // ref man p767. This moves it into bits [7:1] and clears bit 0. R/W 0 = WRITE. So we type slave address, shift one left and that way bit 0 is 0 which is write

	// wait until address flag is set
	while(!(I2C1->SR1 & (SR1_ADDR))){}

	// clear addr flag, clear it after the loop. clear by reading SR
	tmp = I2C1->SR2;

	// now we can send memory address, send to data register
	I2C1->DR = maddr;

	// wait until data reg empty, check txe flag
	while(!(I2C1->SR1 & SR1_TXE)){} // uncomment line
	//while(!(I2C1->SR1 & SR1_BTF)){}

	// generate another start condition - restart condition, same line as above
	I2C1->CR1 |= CR1_START;

	// wait until start bit set
	while(!(I2C1->SR1 & (SR1_SB))){}// ref man p767-770 sb start bit 0; 1: start condition generated

	// transmit slave address + read
	I2C1->DR = saddr <<1 | 1; // shift slave address and sets bit 0 which is read

	// wait until address flag is set, same as above
	while(!(I2C1->SR1 & (SR1_ADDR))){}

	// disable acknowledge
	I2C1->CR1 &=~ CR1_ACK;

	// clear addr flag, clear it after the loop. clear by reading SR
	tmp = I2C1->SR2;

	I2C1->CR1 |= CR1_STOP; // generate stop after data received

	// data register not empty
	while(!(I2C1->SR1 & SR1_RXNE)){}

	*data++ = I2C1->DR; // Store received I2C byte in buffer and increment pointer. Will be useful for burst read

}


void I2C1_burstRead(char saddr, char maddr, int n, char* data)
{
	volatile int tmp;

	while(I2C1->SR2 & (SR2_BUSY)){} // make sure bus is not busy, check busy flag. sr2 reg bit 1 busy

	I2C1->CR1 |= CR1_START; // once its not busy, generate start condition, set start bit in cr1. bit 8 start bit p763

	// wait until start bit set
	while(!(I2C1->SR1 & (SR1_SB))){}// ref man p767-770 sb start bit 0; 1: start condition generated

	// transmit slave address + write
	I2C1->DR = saddr <<1; // ref man p767. This moves it into bits [7:1] and clears bit 0. R/W 0 = WRITE. So we type slave address, shift one left and that way bit 0 is 0 which is write

	while(!(I2C1->SR1 & (SR1_ADDR))){}

	// clear addr flag
	tmp = I2C1->SR2;

	while(!(I2C1->SR1 & SR1_TXE)){} // wait until transmitter empty

	I2C1->DR = maddr;

	while(!(I2C1->SR1 & SR1_TXE)){}

	I2C1->CR1 |= CR1_START;

	// wait until start bit set
	while(!(I2C1->SR1 & (SR1_SB))){}

	// transmit slave address + read
	I2C1->DR = saddr <<1 | 1;

	while(!(I2C1->SR1 & (SR1_ADDR))){}

	// clear addr flag, clear it after the loop. clear by reading SR
	tmp = I2C1->SR2;

	// enable acknowledge
	I2C1->CR1 |= CR1_ACK;

	while(n>0U)
	{
		// if one byte
		if(n==1U)
		{
			// dis acknolwedege
			I2C1->CR1 &=~ CR1_ACK;

			// generate stop
			I2C1->CR1 |= CR1_STOP;

			while(!(I2C1->SR1 & SR1_RXNE)){}

			*data++ = I2C1->DR;
			break;
		}
		else
		{
			while(!(I2C1->SR1 & SR1_RXNE)){}

			//read data from dr
			(*data++) = I2C1->DR;

			n--;
		}
	}
}

void I2C1_burstWrite(char saddr, char maddr, int n, char* data){
// Slave address, memory address, number of bytes, pointer data
	volatile int tmp;

	//Wait until bus not busy
	while(I2C1->SR2 & (SR2_BUSY)){}

	// Generate start condition
	I2C1->CR1 |= CR1_START;

	while(!(I2C1->SR1 & (SR1_SB))){} // waiting start condition

	// transmit slave address + write
	I2C1->DR = saddr <<1;

	// wait until address flag is set
	while(!(I2C1->SR1 & (SR1_ADDR))){}

	// clear addr flag, clear it after the loop. clear by reading SR
	tmp = I2C1->SR2;

	// wait until data reg empty, check txe flag
	while(!(I2C1->SR1 & SR1_TXE)){}

	// now we can send memory address, send to data register
	I2C1->DR = maddr;

	// now we can write in for loop
	for (int i =0;i<n;i++){
		// wait until data reg i ready
		while(!(I2C1->SR1 & SR1_TXE)){}

		I2C1->DR = *data++;

	}

	// Reference manual p770 but 2 byte trasnfer finished, waiting. 1: data byte transfer succeeded
	while(!(I2C1->SR1 &(SR1_BTF))){}

	I2C1->CR1 |= CR1_STOP; // generate stop after data received
}


