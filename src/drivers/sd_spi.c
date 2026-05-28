#include "FreeRTOS.h"
#include "drivers/sd.h"
#include "drivers/spi.h"
#include "modules/logger.h"
#include "task.h"
#include <stdint.h>

static const uint8_t SD_CMD0[] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};	// CMD0 with CRC
static const uint8_t SD_CMD8[] = {0x48, 0x00, 0x00, 0x01, 0xAA, 0x87};	// CMD8 with CRC
static const uint8_t SD_CMD55[] = {0x77, 0x00, 0x00, 0x00, 0x00, 0xFF}; // CMD55 with dummy CRC
static const uint8_t SD_ACMD41[] =
	{0x69, 0x40, 0x00, 0x00, 0x00, 0xFF}; // ACMD41 with HCS=1 and dummy CRC
static const uint8_t SD_CMD58[] = {0x7A, 0x00, 0x00, 0x00, 0x00, 0xFF}; // CMD58 with dummy CRC

static const spi_device_t sd_dev = {.cs_pin = SD_CSN_PIN,
	.mode = SPI_MODE_0,
	.frequency = SPI_FREQ_250K,
	.order = SPI_MSB_FIRST,
	.dummy_byte = 0xFF};

int sd_init(void) {
	spi_device_init(&sd_dev);

	if (spi_clock_idle(&sd_dev, SD_INIT_CLOCK_BYTES) != 0) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"IDLE CLOCK FAILED",
			(uint8_t)(sizeof("IDLE CLOCK FAILED") - 1));

		return -1;
	}

	// CMD0

	uint8_t r1 = 0xFF;

	if (sd_send_cmd(SD_CMD0, &r1, NULL, 0)) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD0 FAILED",
			(uint8_t)(sizeof("CMD0 FAILED") - 1));
		return -1;
	}

	logger_log_hex_len("SD INIT:CMD0/R1", sizeof("SD INIT:CMD0/R1") - 1, &r1, 1);

	// CMD8

	r1 = 0xFF;
	uint8_t r7[4] = {0};

	if (sd_send_cmd(SD_CMD8, &r1, r7, sizeof(r7)) != 0) {

		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD8 FAILED",
			(uint8_t)(sizeof("CMD8 FAILED") - 1));

		return -1;
	}

	logger_log_hex_len("SD INIT:CMD8/R1", sizeof("SD INIT:CMD8/R1") - 1, &r1, 1);
	logger_log_hex_len("SD INIT:CMD8/R7", sizeof("SD INIT:CMD8/R7") - 1, r7, sizeof(r7));

	if (r1 == 0x01 && r7[0] == 0x00 && r7[1] == 0x00 && r7[2] == 0x01 && r7[3] == 0xAA) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"SD V2 CARD DETECTED",
			(uint8_t)(sizeof("SD V2 CARD DETECTED") - 1));
	} else if (r1 == 0x05) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"LEGACY CARD REJECTED",
			(uint8_t)(sizeof("LEGACY CARD REJECTED") - 1));

		return -1;
	} else {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"UNKNOWN CMD8 RESPONSE",
			(uint8_t)(sizeof("UNKNOWN CMD8 RESPONSE") - 1));

		return -1;
	}

	// CMD55 + ACMD41 initialization loop

	r1 = 0xFF;
	uint8_t initialized = 0;

	for (uint32_t i = 0; i < 100; i++) {

		if (sd_send_cmd(SD_CMD55, &r1, NULL, 0) != 0) {

			logger_log_literal_len("SD INIT:",
				(uint8_t)(sizeof("SD INIT:") - 1),
				"CMD55 FAILED",
				(uint8_t)(sizeof("CMD55 FAILED") - 1));
			return -1;
		}

		if (sd_send_cmd(SD_ACMD41, &r1, NULL, 0) != 0) {

			logger_log_literal_len("SD INIT:",
				(uint8_t)(sizeof("SD INIT:") - 1),
				"ACMD41 FAILED",
				(uint8_t)(sizeof("ACMD41 FAILED") - 1));
			return -1;
		}

		logger_log_hex_len("SD INIT:ACMD41/R1", sizeof("SD INIT:ACMD41/R1") - 1, &r1, 1);

		if (r1 == 0x00) {
			initialized = 1;
			break;
		}

		if (r1 != 0x01) {
			logger_log_literal_len("SD INIT:",
				(uint8_t)(sizeof("SD INIT:") - 1),
				"ACMD41 ERROR",
				(uint8_t)(sizeof("ACMD41 ERROR") - 1));
			return -1;
		}

		vTaskDelay(pdMS_TO_TICKS(10));
	}

	if (!initialized) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"ACMD41 TIMEOUT",
			(uint8_t)(sizeof("ACMD41 TIMEOUT") - 1));
		return -1;
	}

	// CMD58

	r1 = 0xFF;
	uint8_t ocr[4] = {0};

	if (sd_send_cmd(SD_CMD58, &r1, ocr, 4) != 0) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD58 FAILED",
			(uint8_t)(sizeof("CMD58 FAILED") - 1));
		return -1;
	}

	logger_log_hex_len("SD INIT:CMD58/R1", sizeof("SD INIT:CMD58/R1") - 1, &r1, 1);
	logger_log_hex_len("SD INIT:CMD58/OCR", sizeof("SD INIT:CMD58/OCR") - 1, ocr, sizeof(ocr));

	uint8_t ccs = (ocr[0] & 0x40) ? 1 : 0;

	if (ccs) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"SDHC/SDXC CARD DETECTED",
			(uint8_t)(sizeof("SDHC/SDXC CARD DETECTED") - 1));
	} else {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"SDSC CARD - NOT SUPPORTED",
			(uint8_t)(sizeof("SDSC CARD - NOT SUPPORTED") - 1));
		return -1;
	}

	return 0;
}

int sd_send_cmd(const uint8_t cmd[8], uint8_t* r1, uint8_t* extra, size_t extra_len) {

	if (cmd == NULL || r1 == NULL)
		return -1;

	if (extra_len > 0 && extra == NULL)
		return -1;

	if (spi_begin(&sd_dev))
		return -1;

	if (spi_tx(cmd, 6) != 0) {
		spi_end();
		return -1;
	}

	*r1 = 0xFF;

	for (int tries = 0; tries < SD_R1_POLL_TRIES; tries++) {
		if (spi_rx(r1, 1) != 0) {
			spi_end();
			return -1;
		}

		if (*r1 != 0xFF) {
			break;
		}
	}

	if (*r1 == 0xFF) {
		spi_end();
		return -1;
	}

	if (extra_len > 0) {
		if (spi_rx(extra, extra_len) != 0) {
			spi_end();
			return -1;
		}
	}

	spi_end();

	if (spi_clock_idle(&sd_dev, 1) != 0) {
		return -1;
	}

	return 0;
}
