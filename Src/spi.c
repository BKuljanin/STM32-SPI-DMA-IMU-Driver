#include "spi.h"

// SPI related configuration
#define SPI1EN (1U<<12)
#define GPIOAEN (1U<<0)

#define SR_TXE (1U<<1)
#define SR_BSY (1U<<7)
#define SR_RXNE (1U<<0)

// DMA related configuration
#define DMA2EN (1U<<22)
#define CHSEL3 ((1U<<25) | (1U<<26))
#define DMA_MEM_INC (1U<<10)
#define DMA_DIR_MEM_TO_PERIPH (1U<<6)
#define DMA_DIR_PERIPH_TO_MEM6 (1U<<6)
#define DMA_DIR_PERIPH_TO_MEM7 (1U<<7)
#define DMA_CR_TCIE (1U<<4)
#define DMA_CR_EN (1U<<0)
#define SPI_CR2_RXDMAEN (1U<<0)
#define SPI_CR2_TXDMAEN (1U<<1)


void dma2_stream_2_3_init(uint8_t src_rx, uint8_t src_tx, uint32_t dst) // Arguments: sources (for tx and rx), destination and length of the data
{	// Reference manual p205 table 29 shows DMA request mapping: SPI1_RX and SPI1_TX
	// DMA2 Stream 2, Channel 3: SPI RX
	// DMA2 Stream 3, Channel 3: SPI_TX

	// Enable clock access to DMA
	RCC->AHB1ENR |= DMA2EN;// Datasheet p16 shows AHB1 bus connected to DMA1/2. Reference manual p144 bit 22

	// Disable DMA2 streams 2 and 3
	DMA2_Stream2->CR &=~ DMA_CR_EN; // Reference manual p226 DMA stream configuration register, we need to disable bit 0 (for both streams)
	DMA2_Stream3->CR &=~ DMA_CR_EN;

	// Wait until DMA2 streams 2 and 3 are disabled
	while(DMA2_Stream2->CR & DMA_CR_EN){} // If returns true we are stuck here until its disabled
	while(DMA2_Stream3->CR & DMA_CR_EN){}

	// Clear interrupt flags for streams 2 and 3
	// Stream 2
	DMA2->LIFCR |= (1U<<16); // Reference manual p225, low interrupt flag clear register. Register starts with stream 0, ends with 3 (thats why its low reg, low starts with 0)
	DMA2->LIFCR |= (1U<<18); // This is all flag clear register, these bits are for stream 2 (clears bits by putting 1 in them)
	DMA2->LIFCR |= (1U<<19);
	DMA2->LIFCR |= (1U<<20);
	DMA2->LIFCR |= (1U<<21);
	// Stream 3
	DMA2->LIFCR |= (1U<<22);
	DMA2->LIFCR |= (1U<<24);
	DMA2->LIFCR |= (1U<<25);
	DMA2->LIFCR |= (1U<<26);
	DMA2->LIFCR |= (1U<<27);

	// Set the destination buffer (peripheral address register). Destination that the user will pass, reference manual p229
	DMA2_Stream2->PAR = dst; // RX
	DMA2_Stream3->PAR = dst; // TX
	// Same peripheral is used by both streams, just different directions. Both streams access SPI1->DR

	// Set the source buffers
	DMA2_Stream2->M0AR = src_rx; // Memory source. Stream peripheral address register. Reference manual p229. Source is memory. Memory 0 AR
	DMA2_Stream3->M0AR = src_tx;

	/*
	// Set length of transfer, same for TX and RX in full duplex
	DMA2_Stream2-> NDTR = len; // Register hold length of the transfer
	DMA2_Stream3-> NDTR = len;
	// This goes in other function, for now leave it here but this will move
	 * , uint32_t len
	 */

	// Select stream 2 channel 3 and stream 3 channel 3
	DMA2_Stream2->CR |= CHSEL3; // Reference manual p226 stream configuration register. CHSEL bit is for channel select. Bits 25,26,27. 011 is channel 3
	DMA2_Stream3->CR |= CHSEL3;
	DMA2_Stream2->CR &= ~(1U<<27); // To be safe if this contains value from before
	DMA2_Stream3->CR &= ~(1U<<27);

	// Enable memory increment. Configure memory to increment because the data is stored in buffer, when first index of the buffer is accessed, next index increments
	DMA2_Stream2->CR |= DMA_MEM_INC; // Same register as above, MINC (memory increment)
	DMA2_Stream3->CR |= DMA_MEM_INC; // On both TX and RX use MINC

	// Configure transfer direction
	DMA2_Stream2->CR &= ~DMA_DIR_PERIPH_TO_MEM6;// Same register as above, bits 6,7 DIR. 00 periph to memory, 01 mem to periph, 10 mem-to-mem. We want 01 memory to peripheral, so we just set one bit
	DMA2_Stream2->CR &= ~DMA_DIR_PERIPH_TO_MEM7;

	DMA2_Stream3->CR |= DMA_DIR_MEM_TO_PERIPH; // Configure peripheral to memory direction for TX

	// Enable DMA transfer complete interrupt for receive only
	DMA2_Stream2->CR |= DMA_CR_TCIE; // We will use only transfer complete interrupt. Reference manual p226-228 DMA stream configuration register. Transfer complete interrupt enable, TCIE bit 4

	// Enable direct mode and disable FIFO
	DMA2_Stream2->FCR = 0; // Fifo control register, reference manual p231. 0 in the register is interrupt disabled, also 0 is direct mode enable
	DMA2_Stream3->FCR = 0;

	// Enable SPI DMA RX and TX
	SPI1->CR2 |= SPI_CR2_RXDMAEN;
	SPI1->CR2 |= SPI_CR2_TXDMAEN;

	// Enable DMA interrupt in NVIC. After transfer is completed interrupt occurs
	NVIC_EnableIRQ(DMA2_Stream2_IRQn); // Transfer completed interrupt only for RX (stream 2)
}

void dma2_enable(void)
{
	// Enable DMA2 stream 2 and 3
	DMA2_Stream2->CR |= DMA_CR_EN; // First enable RX stream because if TX is enabled first SPI can start clocking before enabling RX
	DMA2_Stream3->CR |= DMA_CR_EN; // DMA stream configuration register p226 reference manual
	while(DMA2_Stream2->CR & DMA_CR_EN){}
	while(DMA2_Stream3->CR & DMA_CR_EN){}
}

void dma2_disable(void)
{
	// Enable DMA2 stream 2 and 3
	DMA2_Stream2->CR &= ~DMA_CR_EN; // DMA stream configuration register p226 reference manual
	DMA2_Stream3->CR &= ~DMA_CR_EN;
	while(DMA2_Stream2->CR & DMA_CR_EN){}
	while(DMA2_Stream3->CR & DMA_CR_EN){}
}

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
