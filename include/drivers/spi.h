#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum {
	SPI_MODE_0,
	SPI_MODE_1,
	SPI_MODE_2,
	SPI_MODE_3,
} spi_mode_t;

typedef enum {
	SPI_MSB_FIRST,
	SPI_LSB_FIRST,
} spi_bit_order_t;

typedef struct {
	uint32_t cs_pin;
	spi_mode_t mode;
	uint32_t frequency_hz;
	spi_bit_order_t order;
	uint8_t dummy_byte; // 0x00 or 0xFF, used for rx-only transactions
} spi_device_t;

void spim_init(void);

void spi_device_init(const spi_device_t* dev);

// Transfers only valid b/w begin/end
int spi_begin(const spi_device_t* dev); // stores active dev config (including dummy byte)
int spi_end(void);			// deasserts CS and clears active dev config
// must call spi_begin() before calling any functions below

int spi_tx(const uint8_t* tx_buf, size_t tx_len); // write only
int spi_rx(uint8_t* rx_buf,
	size_t rx_len); // read only - uses active dev’s dummy byte to clock reads
int spi_txrx(const uint8_t* tx_buf, uint8_t* rx_buf, size_t len); // full duplex

// must call spi_end() after calling any functions above
