#include "drivers/spi.h"
#include "FreeRTOS.h" // IWYU pragma: keep
#include "board.h"
#include "logger.h"
#include "memutils.h"
#include "portmacro.h"
#include "semphr.h"
#include "task.h"
#include <stdint.h>

static SemaphoreHandle_t spi_bus_mutex = NULL; // mutex for exclusive access to the SPI bus
StaticSemaphore_t spi_bus_mutex_buf;
static const spi_device_t* active_dev = NULL; // device currently active on the bus

static inline uint8_t in_txn() {
	return (active_dev != NULL);
}

static void cs_low(uint32_t pin) {
	GPIO_OUTCLR_REG = (1u << pin);
}

static void cs_high(uint32_t pin) {
	GPIO_OUTSET_REG = (1u << pin);
}

// only one spi master for now
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

	/* --------Set registers to known state -----
	 */

	SPIM_CONFIG_REG = (0 << 0) |	 // CPHA = 0
			  (0 << 1) |	 // CPOL = 0
			  (0 << 2);	 // ORDER = 0 → MSB first
	SPIM_FREQUENCY_REG = 0x10000000; // 1 MHz
	SPIM_ORC_REG = 0xFF;		 // Clear ORC
	// Clear events
	SPIM_EVENTS_END_REG = 0;
	SPIM_EVENTS_STARTED_REG = 0;
	SPIM_EVENTS_STOPPED_REG = 0;

	SPIM_TXD_PTR_REG = 0;		// Clear TXD pointer
	SPIM_TXD_MAXCNT_REG = 0;	// Clear TXD max count
	SPIM_RXD_PTR_REG = 0;		// Clear RXD pointer
	SPIM_RXD_MAXCNT_REG = 0;	// Clear RXD max count
	SPIM_SHORTS_REG = 0;		// Clear shorts
	SPIM_INTENCLR_REG = 0xFFFFFFFF; // Disable all interrupts

	// Create mutex for SPI bus access
	spi_bus_mutex =
		xSemaphoreCreateMutexStatic(&spi_bus_mutex_buf); // Create mutex for SPI bus access
	if (spi_bus_mutex == NULL) {
		configASSERT(0);
		for (;;)
			;
	}

	/* ---------------- Enable SPIM0 ----------------
	 * Value 7 = Enabled
	 */
	SPIM_ENABLE_REG = 7;
}

void spi_device_init(const spi_device_t* dev) {
	// CSN: Output, Input buffer, disconnected, no pull
	GPIO_CNF(dev->cs_pin) = (1 << 0) | // DIR = 1 → Output
				(1 << 1) | // INPUT = 1 → Disconnect input buffer
				(0 << 2) | // PULL = 00 → Disabled
				(0 << 8) | // DRIVE = 000 → Standard drive (S0S1)
				(0 << 16); // SENSE = Disabled
	cs_high(dev->cs_pin);
}

int spi_begin(const spi_device_t* dev) {

	BaseType_t ok = xSemaphoreTake(spi_bus_mutex, portMAX_DELAY);

	if (ok != pdTRUE) {
		log_t l1 = {.label = "SPI CSN:", .type = LOG_UINT, .len = sizeof(uint32_t)};
		mem_cpy(l1.payload, &dev->cs_pin, sizeof(dev->cs_pin));
		logger_log(l1);

		log_t l2 = {.label = "SPI BEGIN:",
			.type = LOG_STRING,
			.payload = "MUTEX TAKE FAILED",
			.len = 17};
		logger_log(l2);

		return -1; // failed to take mutex
	}

	configASSERT(active_dev == NULL); // should never happen, but just in case

	uint32_t order = 0;
	switch (dev->order) {
	case SPI_MSB_FIRST:
		order = 0; // MSB first
		break;

	case SPI_LSB_FIRST:
		order = 1; // LSB first
		break;
	}

	uint32_t cfg = 0;
	switch (dev->mode) {
	case SPI_MODE_1:

		cfg = (1 << 0) |    // CPHA = 1
		      (0 << 1) |    // CPOL = 0
		      (order << 2); // ORDER = 0 → MSB first, 1 → LSB first
		break;
	case SPI_MODE_2:
		cfg = (0 << 0) |    // CPHA = 0
		      (1 << 1) |    // CPOL = 1
		      (order << 2); // ORDER = 0 → MSB first, 1 → LSB first
		break;
	case SPI_MODE_3:
		cfg = (1 << 0) |    // CPHA = 1
		      (1 << 1) |    // CPOL = 1
		      (order << 2); // ORDER = 0 → MSB first, 1 → LSB first
		break;
	case SPI_MODE_0:
	default:
		cfg = (0 << 0) |    // CPHA = 0
		      (0 << 1) |    // CPOL = 0
		      (order << 2); // ORDER = 0 → MSB first, 1 → LSB first
	}

	SPIM_CONFIG_REG = cfg;

	/* SPI clock */
	SPIM_FREQUENCY_REG = dev->frequency;

	// ORC
	SPIM_ORC_REG = dev->dummy_byte;

	// clear end event
	SPIM_EVENTS_END_REG = 0;

	cs_low(dev->cs_pin);
	active_dev = dev;

	log_t l1 = {.label = "SPI CSN:", .type = LOG_UINT, .len = sizeof(uint32_t)};
	mem_cpy(l1.payload, &dev->cs_pin, sizeof(dev->cs_pin));
	logger_log(l1);

	log_t l2 = {.label = "SPI BEGIN:", .type = LOG_STRING, .payload = "OK", .len = 2};
	logger_log(l2);

	return 0;
}

// void spi_transfer(uint32_t cs_pin,
//	spi_mode_t mode,
//	uint32_t frequency_hz,
//	uint8_t* tx,
//	uint8_t* rx,
//
//	size_t len) {
//
//	if (len == 0)
//		return;
//
//	/* ---------------- SPIM configuration ---------------- */
//
//	// Just MSB first for now
//	uint32_t cfg = 0;
//	switch (mode) {
//	case SPI_MODE_1:
//
//		cfg = (1 << 0) | // CPHA = 1
//		      (0 << 1) | // CPOL = 0
//		      (0 << 2);	 // ORDER = 0 → MSB first
//		break;
//	case SPI_MODE_2:
//		cfg = (0 << 0) | // CPHA = 0
//		      (1 << 1) | // CPOL = 1
//		      (0 << 2);	 // ORDER = 0 → MSB first
//		break;
//	case SPI_MODE_3:
//		cfg = (1 << 0) | // CPHA = 1
//		      (1 << 1) | // CPOL = 1
//		      (0 << 2);	 // ORDER = 0 → MSB first
//		break;
//	case SPI_MODE_0:
//	default:
//		cfg = (0 << 0) | // CPHA = 0
//		      (0 << 1) | // CPOL = 0
//		      (0 << 2);	 // ORDER = 0 → MSB first
//	}
//
//	SPIM_CONFIG_REG = cfg;
//
//	/* SPI clock */
//	SPIM_FREQUENCY_REG = frequency_hz;
//
//	// clear end event
//	SPIM_EVENTS_END_REG = 0;
//
//	// program DMA
//	SPIM_TXD_PTR_REG = (uintptr_t)tx;
//	SPIM_TXD_MAXCNT_REG = len;
//
//	SPIM_RXD_PTR_REG = (uintptr_t)rx;
//	SPIM_RXD_MAXCNT_REG = len;
//
//	// start spi txn
//	cs_low(cs_pin);
//	SPIM_TASKS_START_REG = 1;
//
//	uint32_t guard = 1000000;
//
//	// Wait for completion
//	while (SPIM_EVENTS_END_REG == 0 && guard--) {
//		taskYIELD();
//	}
//
//	if (guard == 0) {
//		cs_high(cs_pin);
//		return;
//	}
//
//	// end transaction
//	SPIM_EVENTS_END_REG = 0;
//	cs_high(cs_pin);
// }
