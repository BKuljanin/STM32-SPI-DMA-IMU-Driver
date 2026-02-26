/*
 * spi.h
 *
 *  Created on: Feb 21, 2026
 *      Author: Bogdan Kuljanin
 */

#ifndef SPI_H_
#define SPI_H_

#include "stm32f4xx.h"
#include <stdint.h>

void spi_gpio_init(void);
void spi1_config(void);
void spi1_transmit(uint8_t *data, uint32_t size);
void spi1_receive(uint8_t *data, uint32_t size);
void cs_enable(void);
void cs_disable(void);
void dma2_stream_2_3_init(uint32_t src_rx, uint32_t src_tx, uint32_t dst);
void dma2_enable(void);
void dma2_disable(void);

#define HISR_TCIF2 (1U<<21)
#define HIFCR_CTCIF2 (1U<<21)

#endif /* SPI_H_ */
