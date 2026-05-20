#include "drivers/sd.h"
#include "drivers/spi.h"
#include "modules/logger.h"

static const uint8_t SD_CMD0[] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95}; // CMD0 with CRC

static const spi_device_t sd_dev = {.cs_pin = SD_CSN_PIN,
	.mode = SPI_MODE_0,
	.frequency = SPI_FREQ_250K,
	.order = SPI_MSB_FIRST,
	.dummy_byte = 0xFF};

void sd_init(void) {
	spi_device_init(&sd_dev);

	if (spi_clock_idle(&sd_dev, SD_INIT_CLOCK_BYTES) != 0) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"IDLE CLOCK FAILED",
			(uint8_t)(sizeof("IDLE CLOCK FAILED") - 1));

		return;
	}

	spi_begin(&sd_dev); // cs select

	// send CMD0
	if (spi_tx(SD_CMD0, sizeof(SD_CMD0)) != 0) {

		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD0 TX FAILED",
			(uint8_t)(sizeof("CMD0 TX FAILED") - 1));

		return;
	}

	int tries = 10;
	uint8_t r1 = 0xFF;

	while (tries-- > 0) {
		if (spi_rx(&r1, 1) != 0) {
			break;
		}

		logger_log_hex_len("CMD0 POLL:", sizeof("CMD0 POLL:") - 1, &r1, 1);

		if (r1 != 0xFF) {
			break;
		}
	}
	logger_log_hex_len("CMD0 F:", sizeof("CMD0 F:") - 1, &r1, 1);

	spi_end(); // cs deselect

	if (spi_clock_idle(&sd_dev, 1) != 0) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"IDLE CLOCK FAILED",
			(uint8_t)(sizeof("IDLE CLOCK FAILED") - 1));

		return;
	}

	logger_log_hex_len("SD INIT: R1", sizeof("SD INIT: R1") - 1, &r1, 1);
}
