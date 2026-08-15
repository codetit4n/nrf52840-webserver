// Port to make SPI Driver work with the Wiznet ioLibrary for W5500
#include "FreeRTOS.h" // IWYU pragma: keep
#include "board.h"
#include "drivers/spi.h"
#include "modules/logger.h"
#include "modules/net.h"
#include "semphr.h"
#include "wizchip_conf.h"

#define W5500_RST_PIN 31

static SemaphoreHandle_t w5500_mutex;
static StaticSemaphore_t w5500_mutex_buf;

static const spi_device_t w5500_dev = {.cs_pin = W5500_CSN_PIN,
	.mode = SPI_MODE_0,
	.frequency = SPI_FREQ_8M,
	.order = SPI_MSB_FIRST,
	.dummy_byte = 0xFF};

void cs_select(void) {
	spi_begin(&w5500_dev);
}

void cs_deselect(void) {
	spi_end();
}

uint8_t w5500_spi_readbyte(void) {
	uint8_t b;
	(void)spi_rx(&b, 1);
	return b;
}

void w5500_spi_writebyte(uint8_t wb) {
	(void)spi_tx(&wb, 1);
}

void w5500_spi_readburst(uint8_t* pBuf, uint16_t len) {
	(void)spi_rx(pBuf, len);
}

void w5500_spi_writeburst(uint8_t* pBuf, uint16_t len) {
	(void)spi_tx(pBuf, len);
}

void w5500_cris_enter(void) {
	xSemaphoreTake(w5500_mutex, portMAX_DELAY);
}

void w5500_cris_exit(void) {
	xSemaphoreGive(w5500_mutex);
}

static void busy_wait_ms(uint32_t ms) {
	volatile uint32_t count;

	while (ms--) {
		count = 64000;

		while (count--) {
			__asm volatile("nop");
		}
	}
}

int8_t w5500_init(void) {
	logger_log_literal_len("W5500 INIT:",
		(uint8_t)(sizeof("W5500 INIT:") - 1),
		"MUTEX",
		(uint8_t)(sizeof("MUTEX") - 1));

	w5500_mutex = xSemaphoreCreateMutexStatic(&w5500_mutex_buf);
	configASSERT(w5500_mutex);

	logger_log_literal_len("W5500 INIT:",
		(uint8_t)(sizeof("W5500 INIT:") - 1),
		"SPI DEVICE",
		(uint8_t)(sizeof("SPI DEVICE") - 1));

	spi_device_init(&w5500_dev);

	logger_log_literal_len("W5500 INIT:",
		(uint8_t)(sizeof("W5500 INIT:") - 1),
		"CALLBACKS",
		(uint8_t)(sizeof("CALLBACKS") - 1));

	reg_wizchip_cs_cbfunc(cs_select, cs_deselect);
	reg_wizchip_spi_cbfunc(w5500_spi_readbyte, w5500_spi_writebyte);
	reg_wizchip_spiburst_cbfunc(w5500_spi_readburst, w5500_spi_writeburst);
	reg_wizchip_cris_cbfunc(w5500_cris_enter, w5500_cris_exit);

	logger_log_literal_len("W5500 INIT:",
		(uint8_t)(sizeof("W5500 INIT:") - 1),
		"RESET LOW",
		(uint8_t)(sizeof("RESET LOW") - 1));

	// RESET pin setup
	GPIO_CNF(W5500_RST_PIN) = (1 << 0) | // DIR = Output
				  (1 << 1) | // INPUT = Disconnect
				  (0 << 2) | // No pull
				  (0 << 8) | // Standard drive
				  (0 << 16); // No sense

	pin_low(W5500_RST_PIN);
	vTaskDelay(pdMS_TO_TICKS(10));

	logger_log_literal_len("W5500 INIT:",
		(uint8_t)(sizeof("W5500 INIT:") - 1),
		"RESET HIGH",
		(uint8_t)(sizeof("RESET HIGH") - 1));

	pin_high(W5500_RST_PIN);

	TickType_t t_before = xTaskGetTickCount();

	logger_log_uint_len("W5500 TICK BEFORE:",
		(uint8_t)(sizeof("W5500 TICK BEFORE:") - 1),
		&t_before,
		sizeof(t_before));

	logger_log_literal_len("W5500 INIT:",
		(uint8_t)(sizeof("W5500 INIT:") - 1),
		"POWERUP WAIT",
		(uint8_t)(sizeof("POWERUP WAIT") - 1));

	vTaskDelay(pdMS_TO_TICKS(150));
	busy_wait_ms(150);

	TickType_t t_after = xTaskGetTickCount();

	logger_log_uint_len("W5500 TICK AFTER:",
		(uint8_t)(sizeof("W5500 TICK AFTER:") - 1),
		&t_after,
		sizeof(t_after));

	logger_log_literal_len("W5500 INIT:",
		(uint8_t)(sizeof("W5500 INIT:") - 1),
		"WIZCHIP START",
		(uint8_t)(sizeof("WIZCHIP START") - 1));

	uint8_t txsize[8] = {2, 2, 2, 2, 2, 2, 2, 2};
	uint8_t rxsize[8] = {2, 2, 2, 2, 2, 2, 2, 2};

	int8_t ret = wizchip_init(txsize, rxsize);

	logger_log_literal_len("W5500 INIT:",
		(uint8_t)(sizeof("W5500 INIT:") - 1),
		"WIZCHIP DONE",
		(uint8_t)(sizeof("WIZCHIP DONE") - 1));

	return ret;
}
