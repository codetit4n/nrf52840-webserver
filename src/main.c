#include "FreeRTOS.h"
#include "board.h"
#include "drivers/spi.h"
#include "projdefs.h"
#include "task.h"
#include <stddef.h>
#include <stdint.h>

static uint8_t tx_buf[] = {0x00, 0x39, 0x00, 0x00}; // Read VERSIONR register at offset 0x0039
static uint8_t rx_buf[sizeof(tx_buf)];

static void process_rx(const uint8_t* rx_buf, size_t len) {
	for (size_t i = 0; i < len; i++) {
		// Put a breakpoint on this line to see rx_buf
		volatile uint8_t byte = rx_buf[i];
		(void)byte;
	}
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

	vTaskStartScheduler();

	/* Should never reach here */
	taskDISABLE_INTERRUPTS();
	for (;;)
		;

	return 0;
}
