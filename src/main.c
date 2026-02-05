#include "FreeRTOS.h"
#include "board.h"
#include "drivers/spi.h"
#include "drivers/uarte.h"
#include "projdefs.h"
#include "task.h"
#include <stddef.h>
#include <stdint.h>

static uint8_t tx_buf[] = {0x00, 0x2E, 0x00, 0x00};
static uint8_t rx_buf[sizeof(tx_buf)];

static void process_rx(const uint8_t* rx_buf, size_t len) {
	// const char* text = (const char*)rx_buf;
	// uarte_write(text, len);
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

static void uart_task(void* arg) {
	(void)arg;

	for (;;) {
		uarte_write("hey!\r\n", 6);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

int main(void) {

	spim_init();
	spi_device_init_cs(CSN_PIN);
	uarte_init();

	BaseType_t ok = xTaskCreate(spi_task, /* Task function */
		"SPI",			      /* Name (for debug) */
		256,			      /* Stack size (words, not bytes) */
		NULL,			      /* Parameters */
		1,			      /* Priority */
		NULL			      /* Task handle */
	);

	if (ok != pdPASS) {
		taskDISABLE_INTERRUPTS();
		for (;;)
			;
	}

	ok = xTaskCreate(uart_task, /* Task function */
		"UART",		    /* Name (for debug) */
		256,		    /* Stack size (words, not bytes) */
		NULL,		    /* Parameters */
		2,		    /* Priority */
		NULL		    /* Task handle */
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
