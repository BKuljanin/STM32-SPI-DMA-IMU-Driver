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
void cs_enable(void);
void cs_disable(void);
void dma2_stream_2_3_init();
void dma2_enable(void);
void dma2_disable(void);
void dma2_transfer_completeted_interrupt_enable(void);
void set_dma_transfer_length(uint32_t len);
void set_dma_source(uint8_t *src_rx, uint8_t *src_tx);

#define HISR_TCIF2 (1U<<21)
#define HIFCR_CTCIF2 (1U<<21)

#endif /* SPI_H_ */
