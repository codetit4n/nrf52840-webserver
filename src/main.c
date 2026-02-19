#include "FreeRTOS.h"
#include "drivers/spi.h"
#include "logger.h"
#include "task.h"
#include <stdint.h>

#define CSN_PIN 26

// W5500 VERSIONR @ 0x0039
// 3-byte header + 1 dummy byte to clock out 1 data byte
//
// Header format: [ADDR_HI, ADDR_LO, CTRL]
// CTRL for common reg, READ, VDM = 0x04
static const uint8_t w5500_cmd[4] = {0x00, 0x39, 0x04, 0x00};

static void spi_task(void* arg) {
	(void)arg;

	static const spi_device_t w5500 = {
		.cs_pin = CSN_PIN,
		.mode = SPI_MODE_0,
		.frequency = SPI_FREQ_8M, // if unstable try 1M first
		.order = SPI_MSB_FIRST,
		.dummy_byte =
			0xFF, // not used in this tx-only path (you provide dummy in w5500_cmd[3])
	};

	spi_device_init(&w5500);

	for (;;) {
		// One framed transaction per loop
		if (spi_begin(&w5500) == 0) {
			(void)spi_tx(w5500_cmd, sizeof(w5500_cmd));

			const uint8_t* rx = spi_last_xfer_rx();
			uint8_t version = rx[3]; // byte returned during dummy clocks

			logger_log_hex_len("RX4:", (uint8_t)(sizeof("RX4:") - 1), rx, 4);
			logger_log_hex_len("W5500 VERSIONR:",
				(uint8_t)(sizeof("W5500 VERSIONR:") - 1),
				&version,
				1);

			spi_end();
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

	// unreachable
	// return 0;
}
