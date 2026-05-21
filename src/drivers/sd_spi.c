#include "drivers/sd.h"
#include "drivers/spi.h"
#include "modules/logger.h"

static const uint8_t SD_CMD0[] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95}; // CMD0 with CRC
static const uint8_t SD_CMD8[] = {0x48, 0x00, 0x00, 0x01, 0xAA, 0x87}; // CMD8 with CRC

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

	// CMD0
	spi_begin(&sd_dev); // cs select

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

		if (r1 != 0xFF) {
			break;
		}
	}

	spi_end(); // cs deselect

	if (spi_clock_idle(&sd_dev, 1) != 0) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"IDLE CLOCK FAILED",
			(uint8_t)(sizeof("IDLE CLOCK FAILED") - 1));

		return;
	}

	logger_log_hex_len("SD INIT:CMD0/R1", sizeof("SD INIT:CMD0/R1") - 1, &r1, 1);

	// CMD8

	spi_begin(&sd_dev); // cs select

	if (spi_tx(SD_CMD8, sizeof(SD_CMD8)) != 0) {

		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD8 TX FAILED",
			(uint8_t)(sizeof("CMD8 TX FAILED") - 1));

		return;
	}

	tries = 10;
	r1 = 0xFF;
	uint8_t r7[4] = {0xFF, 0xFF, 0xFF, 0xFF};

	while (tries-- > 0) {
		if (spi_rx(&r1, 1) != 0) {
			break;
		}

		if (r1 != 0xFF) {

			// read 4 more bytes of CMD8 response (R7)
			if (spi_rx(r7, sizeof(r7)) != 0) {
				logger_log_literal_len("SD INIT:",
					(uint8_t)(sizeof("SD INIT:") - 1),
					"CMD8 R7 RX FAILED",
					(uint8_t)(sizeof("CMD8 R7 RX FAILED") - 1));
			}

			break;
		}
	}

	spi_end(); // cs deselect

	if (spi_clock_idle(&sd_dev, 1) != 0) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"IDLE CLOCK FAILED",
			(uint8_t)(sizeof("IDLE CLOCK FAILED") - 1));

		return;
	}

	logger_log_hex_len("SD INIT:CMD8/R1", sizeof("SD INIT:CMD8/R1") - 1, &r1, 1);
	logger_log_hex_len("SD INIT:CMD8/R7", sizeof("SD INIT:CMD8/R7") - 1, r7, sizeof(r7));
}
