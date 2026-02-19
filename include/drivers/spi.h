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

// as per datasheet
typedef uint32_t spi_frequency_t;

#define SPI_FREQ_125K 0x02000000u
#define SPI_FREQ_250K 0x04000000u
#define SPI_FREQ_500K 0x08000000u
#define SPI_FREQ_1M 0x10000000u
#define SPI_FREQ_2M 0x20000000u
#define SPI_FREQ_4M 0x40000000u
#define SPI_FREQ_8M 0x80000000u
#define SPI_FREQ_16M 0x0A000000u
#define SPI_FREQ_32M 0x14000000u

typedef struct {
	uint32_t cs_pin;
	spi_mode_t mode;
	spi_frequency_t frequency;
	spi_bit_order_t order;
	uint8_t dummy_byte; // 0x00 or 0xFF, used for rx-only transactions
} spi_device_t;

#define SPI_MAX_XFER 512
#define SPI_TIMEOUT_TICKS pdMS_TO_TICKS(50) // for v1 only

void spim_init(void);

// Static Hardware Setup only
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
