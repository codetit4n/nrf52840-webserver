#include "drivers/uarte.h"
#include "FreeRTOS.h" // IWYU pragma: keep
#include "board.h"
#include "portmacro.h"
#include "projdefs.h"
#include "semphr.h"
#include "task.h"
#include <stdint.h>

// Static - RAM allocation, no dynamic memory management needed
static uint8_t tx_buf[UART_TX_BUF_SIZE];
static SemaphoreHandle_t uarte_mutex = NULL;

static TickType_t tx_timeout_ticks(size_t bytes) {
	// UART parameters
	const uint32_t baud = 1000000;
	const uint32_t bits_per_byte = 10;

	// Convert wire time to milliseconds (ceil division)
	uint32_t wire_ms = (bytes * bits_per_byte * 1000u + baud - 1u) / baud;

	// Fixed RTOS / jitter margin
	const uint32_t margin_ms = 15;

	uint32_t timeout_ms = wire_ms + margin_ms;

	// Minimum timeout floor
	if (timeout_ms < 20)
		timeout_ms = 20;

	return pdMS_TO_TICKS(timeout_ms);
}

static uint8_t tx_send_blocking(const uint8_t* tx, size_t len) {
	if (len == 0)
		return 1; // Nothing to send
	if (len > 0xFFFF)
		len = 0xFFFF; // MAXCNT is 16-bit

	// Clear completion event
	UARTE_EVENTS_ENDTX_REG = 0;

	// Program EasyDMA
	UARTE_TXD_PTR_REG = (uint32_t)(uintptr_t)tx;
	UARTE_TXD_MAXCNT_REG = (uint32_t)len;

	// Start TX
	UARTE_TASKS_STARTTX_REG = 1;

	TickType_t start = xTaskGetTickCount();
	TickType_t timeout = tx_timeout_ticks(len);

	// Wait for completion (DMA done)
	while (UARTE_EVENTS_ENDTX_REG == 0) {
		if ((xTaskGetTickCount() - start) > timeout) {
			uarte_recover();
			return 0; // Timeout, consider it a failure
		}
		taskYIELD();
	}

	return 1; // Success
}

void uarte_init(void) {
	UARTE_ENABLE_REG = 0; // Disable UARTE while configuring

	GPIO_CNF(TX_PIN) = (1 << 0) | // DIR = Output
			   (1 << 1) | // INPUT disconnect (input buffer not needed for TX)
			   (0 << 2) | // PULL = none (field)
			   (0 << 8) | // DRIVE = standard (field)
			   (0 << 16); // SENSE = disabled

	// PSEL format: PIN[4:0] | PORT(bit5) | CONNECT(bit31: 0=connected, 1=disconnected)
	UARTE_PSEL_TXD_REG = (TX_PIN << 0) | (0 << 5) | (0 << 31);

	UARTE_PSEL_RXD_REG = (1 << 31); // RX disconnected (TX-only)

	UARTE_CONFIG_REG = (0 << 0) |	// HWFC disabled
			   (0x0 << 1) | // PARITY excluded
			   (0 << 4);	// 1 stop bit

	UARTE_BAUDRATE_REG = 0x10000000; // 1Mega baud

	UARTE_EVENTS_ENDTX_REG = 0;
	UARTE_EVENTS_TXSTOPPED_REG = 0;

	UARTE_ENABLE_REG = 8; // Enable UARTE
	uarte_mutex = xSemaphoreCreateMutex();
}

void uarte_recover(void) {

	UARTE_TASKS_STOPTX_REG = 1; // Stop any ongoing transmission

	// wait until tx is fully stopped
	for (volatile int i = 0; i < 1000; i++) {
		if (UARTE_EVENTS_TXSTOPPED_REG)
			break;
	}

	UARTE_ENABLE_REG = 0; // Disable UARTE

	// clear events and reset state
	UARTE_EVENTS_ENDTX_REG = 0;
	UARTE_EVENTS_TXSTOPPED_REG = 0;

	UARTE_ENABLE_REG = 8; // Enable UARTE
}

// To only be used by the logging lib
uint8_t uarte_write(const uint8_t* data, size_t len) {
	if (data == NULL)
		return 0;

	if (uarte_mutex)
		xSemaphoreTake(uarte_mutex, portMAX_DELAY);

	if (len > sizeof(tx_buf))
		len = sizeof(tx_buf);

	for (size_t i = 0; i < len; i++)
		tx_buf[i] = data[i];

	uint8_t ok = tx_send_blocking(tx_buf, len);

	if (uarte_mutex)
		xSemaphoreGive(uarte_mutex);

	return ok;
}
