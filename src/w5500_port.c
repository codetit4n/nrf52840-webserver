// Port to make SPI Driver work with the Wiznet ioLibrary for W5500

#include "FreeRTOS.h" // IWYU pragma: keep
#include "drivers/spi.h"
#include "semphr.h"
#include "wizchip_conf.h"

#define CSN_PIN 26

static SemaphoreHandle_t w5500_mutex;
static StaticSemaphore_t w5500_mutex_buf;

static const spi_device_t w5500_dev = {.cs_pin = CSN_PIN,
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

void w5500_init(void) {
	w5500_mutex = xSemaphoreCreateMutexStatic(&w5500_mutex_buf);
	configASSERT(w5500_mutex);

	spi_device_init(&w5500_dev);

	reg_wizchip_cs_cbfunc(cs_select, cs_deselect);
	reg_wizchip_spi_cbfunc(w5500_spi_readbyte, w5500_spi_writebyte);
	reg_wizchip_spiburst_cbfunc(w5500_spi_readburst, w5500_spi_writeburst);
	reg_wizchip_cris_cbfunc(w5500_cris_enter, w5500_cris_exit);

	// Let W5500 finish power-up before first access
	vTaskDelay(pdMS_TO_TICKS(150));

	uint8_t txsize[8] = {2, 2, 2, 2, 2, 2, 2, 2};
	uint8_t rxsize[8] = {2, 2, 2, 2, 2, 2, 2, 2};
	wizchip_init(txsize, rxsize);
}
