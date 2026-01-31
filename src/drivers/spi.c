#include "drivers/spi.h"
#include "FreeRTOS.h"
#include "board.h"
#include "task.h"
#include <stdint.h>

static void cs_low(uint32_t pin) {
	GPIO_OUTCLR_REG = (1u << pin);
}

static void cs_high(uint32_t pin) {
	GPIO_OUTSET_REG = (1u << pin);
}

// only one spim for now
void spim_init(void) {

	/* ---------------- SCK pin ----------------
	 * Output driven by SPIM
	 */
	SPIM_PSEL_SCK_REG = (0 << 31) | // CONNECT = 0 → Connected
			    (0 << 5) |	// PORT = 0 → P0
			    (SCK_PIN << 0);

	GPIO_CNF(SCK_PIN) = (1 << 0) | // DIR = Output
			    (1 << 1) | // INPUT = Disconnect
			    (0 << 2) | // No pull
			    (0 << 8) | // Standard drive
			    (0 << 16); // No sense

	/* ---------------- MOSI pin ----------------
	 * Output driven by SPIM
	 */
	SPIM_PSEL_MOSI_REG = (0 << 31) | (0 << 5) | (MOSI_PIN << 0);

	GPIO_CNF(MOSI_PIN) = (1 << 0) | // Output
			     (1 << 1) | // Disconnect input
			     (0 << 2) | (0 << 8) | (0 << 16);

	/* ---------------- MISO pin ----------------
	 * Input to SPIM
	 */
	SPIM_PSEL_MISO_REG = (0 << 31) | (0 << 5) | (MISO_PIN << 0);

	GPIO_CNF(MISO_PIN) = (0 << 0) | // DIR = Input
			     (0 << 1) | // INPUT = Connected
			     (0 << 2) | // No pull (depends on slave)
			     (0 << 8) | (0 << 16);

	/* ---------------- Enable SPIM0 ----------------
	 * Value 7 = Enabled
	 */
	SPIM_ENABLE_REG = 7;
}

void spi_device_init_cs(uint32_t cs_pin) {
	// CSN: Output, Input buffer, disconnected, no pull
	GPIO_CNF(cs_pin) = (1 << 0) | // DIR = 1 → Output
			   (1 << 1) | // INPUT = 1 → Disconnect input buffer
			   (0 << 2) | // PULL = 00 → Disabled
			   (0 << 8) | // DRIVE = 000 → Standard drive (S0S1)
			   (0 << 16); // SENSE = Disabled
	cs_high(cs_pin);
}

void spi_transfer(uint32_t cs_pin,
	spi_mode_t mode,
	uint32_t frequency_hz,
	uint8_t* tx,
	uint8_t* rx,
	size_t len) {

	if (len == 0)
		return;

	/* ---------------- SPIM configuration ---------------- */

	// Just MSB first for now
	uint32_t cfg = 0;
	switch (mode) {
	case SPI_MODE_1:
		cfg = (1 << 0) | // CPHA = 1
		      (0 << 1) | // CPOL = 0
		      (0 << 2);	 // ORDER = 0 → MSB first
		break;
	case SPI_MODE_2:
		cfg = (0 << 0) | // CPHA = 0
		      (1 << 1) | // CPOL = 1
		      (0 << 2);	 // ORDER = 0 → MSB first
		break;
	case SPI_MODE_3:
		cfg = (1 << 0) | // CPHA = 1
		      (1 << 1) | // CPOL = 1
		      (0 << 2);	 // ORDER = 0 → MSB first
		break;
	case SPI_MODE_0:
	default:
		cfg = (0 << 0) | // CPHA = 0
		      (0 << 1) | // CPOL = 0
		      (0 << 2);	 // ORDER = 0 → MSB first
	}

	SPIM_CONFIG_REG = cfg;

	/* SPI clock */
	SPIM_FREQUENCY_REG = frequency_hz;

	// clear end event
	SPIM_EVENTS_END_REG = 0;

	// program DMA
	SPIM_TXD_PTR_REG = (uintptr_t)tx;
	SPIM_TXD_MAXCNT_REG = len;

	SPIM_RXD_PTR_REG = (uintptr_t)rx;
	SPIM_RXD_MAXCNT_REG = len;

	// start spi txn
	cs_low(cs_pin);
	SPIM_TASKS_START_REG = 1;

	uint32_t guard = 1000000;

	// Wait for completion
	while (SPIM_EVENTS_END_REG == 0 && guard--) {
		taskYIELD();
	}

	if (guard == 0) {
		cs_high(cs_pin);
		return;
	}

	// end transaction
	SPIM_EVENTS_END_REG = 0;
	cs_high(cs_pin);
}
