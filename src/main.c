#include "FreeRTOS.h"
#include "drivers/spi.h"
#include "logger.h"
#include "task.h"
#include <stddef.h>
#include <stdint.h>

#define CSN_PIN 26

/* W5500 VERSIONR register address is 0x0039, expected value: 0x04 */
static uint8_t w5500_hdr[3] = {0x00, 0x39, 0x00}; /* addr_hi, addr_lo, control */
static uint8_t version_byte = 0;

static void spi_task(void* arg) {
	(void)arg;

	/* Define W5500 device settings */
	static const spi_device_t w5500 = {
		.cs_pin = CSN_PIN,
		.mode = SPI_MODE_0,
		.frequency = SPI_FREQ_8M, /* or slower (1M/4M) if bring-up */
		.order = SPI_MSB_FIRST,	  /* whatever your enum/defines are */
		.dummy_byte = 0xFF	  /* for RX clocks; 0x00 is fine for W5500 */
	};

	/* IMPORTANT: no spi_end() yet, so begin ONCE */
	spi_begin(&w5500);

	for (;;) {
		/* Send 3-byte read header, then clock out 1 byte via spi_rx() */
		(void)spi_tx(w5500_hdr, sizeof(w5500_hdr));
		(void)spi_rx(&version_byte, 1);

		/* Log the single received byte as HEX */
		logger_log_hex_len("W5500 VERSIONR:",
			(uint8_t)(sizeof("W5500 VERSIONR:") - 1),
			&version_byte,
			1);

		/* Optional: also log as UINT (still raw bytes, your logger prints it as decimal) */
		/* logger_log_uint_len("W5500 VERSIONR(U):", (uint8_t)(sizeof("W5500 VERSIONR(U):")
		   - 1), &version_byte, (uint8_t)sizeof(version_byte)); */

		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

int main(void) {
	spim_init();
	logger_init();

	BaseType_t ok = xTaskCreate(spi_task, "spi_task", 256, NULL, 2, NULL);

	if (ok != pdPASS) {
		taskDISABLE_INTERRUPTS();
		for (;;)
			;
	}

	vTaskStartScheduler();

	taskDISABLE_INTERRUPTS();
	for (;;)
		;

	return 0;
}
