#include "drivers/uarte.h"
#include "FreeRTOS.h"
#include "board.h"
#include "task.h"
#include <stdint.h>

#define UART_TX_BUF_SIZE 256

// Static - RAM allocation, no dynamic memory management needed
static uint8_t tx_buf[UART_TX_BUF_SIZE];

static void tx_send(const uint8_t* tx, size_t len) {
	if (len == 0)
		return;
	if (len > 0xFFFF)
		len = 0xFFFF; // MAXCNT is 16-bit

	UARTE_EVENTS_ENDTX_REG = 0;
	// Set up TX buffer
	UARTE_TXD_PTR_REG = (uint32_t)(uintptr_t)tx;
	UARTE_TXD_MAXCNT_REG = len;

	// Start TX
	UARTE_TASKS_STARTTX_REG = 1;

	uint32_t guard = 1000000;

	// Wait for completion
	while (UARTE_EVENTS_ENDTX_REG == 0 && guard--) {
		taskYIELD();
	}
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

	UARTE_BAUDRATE_REG = 0x01D60000; // 115200 (per datasheet table)

	UARTE_EVENTS_ENDTX_REG = 0;
	UARTE_EVENTS_TXSTOPPED_REG = 0;

	UARTE_ENABLE_REG = 8; // Enable UARTE
}

void uarte_write(const char* text, size_t len) {
	if (len > sizeof(tx_buf))
		len = sizeof(tx_buf);

	for (size_t i = 0; i < len; i++)
		tx_buf[i] = (uint8_t)text[i];

	tx_send(tx_buf, len);
}
