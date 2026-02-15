#include "FreeRTOS.h"
#include "board.h"
#include "drivers/spi.h"
#include "drivers/uarte.h"
#include "logger.h"
#include "memutils.h"
#include "projdefs.h"
#include "task.h"
#include <stddef.h>
#include <stdint.h>

static uint8_t tx_buf[] = {0x00, 0x2E, 0x00, 0x00}; // PHYCFGR register read
static uint8_t rx_buf[sizeof(tx_buf)];

static void process_rx(uint8_t* rx_buf, uint8_t len) {
	if (rx_buf == NULL || len == 0) {
		return;
	}

	uint32_t value = len;

	log_t l1 = {.type = LOG_UINT, .label = "RX BUF LEN:", .len = sizeof(uint32_t)};
	memcpy_u8(l1.payload, (const uint8_t*)&value, sizeof(value));
	logger_log(l1);

	log_t l2 = {.type = LOG_HEX,
		.label = "RX DATA:",
		.len = len - 3}; // first 3 bytes are header, skip them
	memcpy_u8((uint8_t*)l2.payload, (const uint8_t*)&rx_buf[3], len - 3);
	logger_log(l2);
}

#define CSN_PIN 26

static void spi_task(void* arg) {
	(void)arg;

	for (;;) {
		spi_transfer(CSN_PIN, SPI_MODE_0, 0x02000000UL, tx_buf, rx_buf, sizeof(tx_buf));
		process_rx(rx_buf, sizeof(rx_buf));

		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

int main(void) {

	spim_init();
	spi_device_init_cs(CSN_PIN);
	logger_init();

	BaseType_t ok = xTaskCreate(spi_task, /* Task function */
		"SPI",			      /* Name (for debug) */
		256,			      /* Stack size (words, not bytes) */
		NULL,			      /* Parameters */
		2,			      /* Priority */
		NULL			      /* Task handle */
	);

	if (ok != pdPASS) {
		taskDISABLE_INTERRUPTS();
		for (;;)
			;
	}

	ok = xTaskCreate(logger_task, /* Task function */
		"LOGGER",	      /* Name (for debug) */
		256,		      /* Stack size (words, not bytes) */
		NULL,		      /* Parameters */
		1,		      /* Priority */
		NULL		      /* Task handle */
	);

	if (ok != pdPASS) {
		taskDISABLE_INTERRUPTS();
		for (;;)
			;
	}

	vTaskStartScheduler();

	/* Should never reach here */
	taskDISABLE_INTERRUPTS();
	for (;;)
		;

	return 0;
}
