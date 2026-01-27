#pragma once

#include <stddef.h>
#include <stdint.h>
typedef enum {
	SPI_MODE_0,
	SPI_MODE_1,
	SPI_MODE_2,
	SPI_MODE_3,
} spi_mode_t;

void spim_init(void);
void spi_device_init_cs(uint32_t cs_pin);
void spi_transfer(uint32_t cs_pin,
	spi_mode_t mode,
	uint32_t frequency_hz,
	uint8_t* tx,
	uint8_t* rx,
	size_t len);
