#include "FreeRTOS.h"
#include "drivers/spi.h"
#include "logger.h"
#include "task.h"
#include <stddef.h>
#include <stdint.h>

#define CSN_PIN 26

static const uint8_t w5500_hdr[4] = {0x00, 0x39, 0x00, 0x00};

static void spi_task(void* arg) {
	(void)arg;

	static const spi_device_t w5500 = {.cs_pin = CSN_PIN,
		.mode = SPI_MODE_0,
		.frequency = SPI_FREQ_8M,
		.order = SPI_MSB_FIRST,
		.dummy_byte = 0xFF};

	spi_device_init(&w5500);
	// Let W5500 finish power-up before first access
	vTaskDelay(pdMS_TO_TICKS(150));

	uint8_t version[4] = {0};

	for (;;) {
		if (spi_begin(&w5500) == 0) {

			(void)spi_txrx(w5500_hdr, version, sizeof(w5500_hdr));

			logger_log_hex_len("W5500 VERSIONR:",
				(uint8_t)(sizeof("W5500 VERSIONR:") - 1),
				&version[3],
				1);

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
