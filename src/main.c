#include "FreeRTOS.h"
#include "drivers/spi.h"
#include "logger.h"
#include "task.h"
#include <stddef.h>
#include <stdint.h>

#define CSN_PIN 26

static const uint8_t w5500_hdr[3] = {0x00, 0x2E, 0x00};

static void spi_task(void* arg) {
	(void)arg;

	static const spi_device_t w5500 = {
		.cs_pin = CSN_PIN,
		.mode = SPI_MODE_0,
		.frequency = SPI_FREQ_8M,
		.order = SPI_MSB_FIRST,
		.dummy_byte = 0xFF,
	};

	spi_device_init(&w5500);

	// Let W5500 finish power-up before first access
	vTaskDelay(pdMS_TO_TICKS(150));

	// TXRX buffer: 3-byte header + 1 dummy byte to clock 1 data byte back
	uint8_t txrx_buf[4] = {0};
	uint8_t rx_buf[4] = {0};

	for (;;) {
		if (spi_begin(&w5500) == 0) {

			// Build TX: header + one dummy byte
			txrx_buf[0] = w5500_hdr[0];
			txrx_buf[1] = w5500_hdr[1];
			txrx_buf[2] = w5500_hdr[2];
			txrx_buf[3] = w5500.dummy_byte; // clock 1 byte

			(void)spi_txrx(txrx_buf, rx_buf, sizeof(txrx_buf));

			// W5500 returns payload after the 3-byte header phase
			uint8_t version = rx_buf[3];

			logger_log_hex_len("W5500 VERSIONR:",
				(uint8_t)(sizeof("W5500 VERSIONR:") - 1),
				&version,
				1);

			// Optional: log full RX to verify the 3 header bytes are "junk/echo"
			// logger_log_hex_len("W5500 RXRAW:",
			// 	(uint8_t)(sizeof("W5500 RXRAW:") - 1),
			// 	rx_buf,
			// 	(uint8_t)sizeof(rx_buf));

			(void)spi_end();
		} else {
			logger_log_literal_len("SPI:",
				(uint8_t)(sizeof("SPI:") - 1),
				"BEGIN FAIL",
				(uint8_t)(sizeof("BEGIN FAIL") - 1));
		}

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
}
