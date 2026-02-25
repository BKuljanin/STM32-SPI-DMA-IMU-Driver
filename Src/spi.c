#include "spi.h"

#define SPI1EN (1U<<12)
#define GPIOAEN (1U<<0)

#define SR_TXE (1U<<1)
#define SR_BSY (1U<<7)
#define SR_RXNE (1U<<0)

void spi_gpio_init(void)
{
	// Datasheet p16 block diagram, SPI modules. We use SPI 1, connected to APB2
	// Datasheet p57 AF mapping, CLOCK PA5, MISO PA 6, MOSI PA7, any pin for SS for example PA9

	// Enable clock for GPIOA
	RCC->AHB1ENR |= GPIOAEN;

	// Configuring pin 5 in AF mode
	GPIOA->MODER &= ~ (1U<<10); // Reference manual p186
	GPIOA->MODER |= (1U<<11);

	// Configuring pin 6 in AF mode
	GPIOA->MODER &= ~ (1U<<12); // Reference manual p186
	GPIOA->MODER |= (1U<<13);

	// Configuring pin 7 in AF mode
	GPIOA->MODER &= ~ (1U<<14); // Reference manual p186
	GPIOA->MODER |= (1U<<15);

	// Configuring pin 9 as output pin
	GPIOA->MODER |= (1U<<18); // Reference manual p186
	GPIOA->MODER &= ~ (1U<<19);

	// Set PA5,PA6,PA7 alternate function type to SPI1 (AF5)
	// PA5
	GPIOA->AFR[0] |= (1U<<20); // Reference manual p190
	GPIOA->AFR[0] &= ~ (1U<<21);
	GPIOA->AFR[0] |= (1U<<22);
	GPIOA->AFR[0] &= ~ (1U<<23);
	// PA6
	GPIOA->AFR[0] |= (1U<<24);
	GPIOA->AFR[0] &= ~ (1U<<25);
	GPIOA->AFR[0] |= (1U<<26);
	GPIOA->AFR[0] &= ~ (1U<<27);
	// PA7
	GPIOA->AFR[0] |= (1U<<28);
	GPIOA->AFR[0] &= ~ (1U<<29);
	GPIOA->AFR[0] |= (1U<<30);
	GPIOA->AFR[0] &= ~ (1U<<31);
}

void spi1_config(void)
{
	// Enable clock access to SPI1 module
	RCC->APB2ENR |= SPI1EN; // Reference manual p149

	// Set clock to fPCLK/4
	// Values that divides SPI Clock, default frequency of MCU is 16MHz, so frequency of APB2 is also by default 16MHz. Divide clock by 4. Reference manual p867
	SPI1->CR1 |= (1U<<3);
	SPI1->CR1 &= ~ (1U<<4);
	SPI1->CR1 &= ~ (1U<<5);

	// Set clock polarity and clock phase to 1
	SPI1->CR1 |= (1U<<0); // Reference manual p868, set CPOL and CHPA to 1
	SPI1->CR1 |= (1U<<1);

	// Enable full duplex
	SPI1->CR1 &= ~ (1U<<10); // Reference manual p867 bit 10, 0 for full duplex, 1 for receive only mode

	// Set MSB first
	SPI1->CR1 &= ~ (1U<<7); // Reference manual p867 bit 7

	// Set mode to master
	SPI1->CR1 |= (1U<<2); // Reference manual p867, 0 slave configuration, 1 master configuration. We choose MCU master, since accelerometer is the slave

	// Set 8 bit data mode
	SPI1->CR1 &= ~ (1U<<11); // Data frame format, reference manual p866. 0 8 bit, 1 16 bit data format

	// Software slave management
	SPI1->CR1 |= (1U<<8); // Reference manual p867
	SPI1->CR1 |= (1U<<9); // Enable software slave management (NSS signal is taken from SSI bit instead of NSS pin)

	// Enable the SPI module
	SPI1->CR1 |= (1U<<6); // Reference manual p867

	GPIOA->ODR |= (1U<<9); // CS high idle to ensure correct start
	// NOTE: Without this line the code didn't work. Writing in initialization wasn't  working because the CS line was initially set low.
	// It must be low only during the SPI transmit/receive
}

void spi1_transmit(uint8_t *data, uint32_t size)
{
	uint32_t i = 0;
	uint8_t temp;

	// Multiple data items, we need a loop
	while(i<size)
	{
		// Wait until TXE is set, flag from status register. Reference manual p870
		while(!(SPI1->SR & (SR_TXE))){}

		// Write the data to the data register
		SPI1->DR = data[i];
		i++; // Increment the index, this is a pointer to the buffer
	}

	// Wait until TXE is set
	while(!(SPI1->SR & (SR_TXE))){} // Reference manual p869

	// Wait for busy flag to reset
	while((SPI1->SR & (SR_BSY))){} // 0 not busy (we wait while its busy), refrence manual p869

	// Clear OVR flag
	temp = SPI1->DR; // Reference manual p869, description of overrun error and reset p844
	temp = SPI1->SR;
	// OVR occurs if a new SPI frame is received before the previous data was read from SPI_DR (RXNE still set)
	// The new data is lost, OVR is set, and must be cleared by reading SPI_DR then SPI_SR

}


void spi1_receive(uint8_t *data, uint32_t size)
{	// Pointer to buffer to store received data
	while(size)// while there is data to be received, size not zero
	{
		// Send dummy data
		SPI1->DR = 0;

		// Waiting for RXNE flag
		while(!(SPI1->SR & (SR_RXNE))){} // Once set it implies that there is data in receive buffer that we can consume

		// Read data from data register
		*data++ = (SPI1->DR); // pointer to storage data, increment
		size--;

	}
}


void cs_enable(void)
{
	// Enable CS line (PA9 is CS line)
	GPIOA->ODR &= ~(1U<<9); // Set to 0 to enable it

}


void cs_disable(void)
{
	// Disable CS line (PA9 is CS line)
	GPIOA->ODR |= (1U<<9); // Set to 1 to disable it

}
