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
static const uint8_t SD_CMD9[] = {0x49, 0x00, 0x00, 0x00, 0x00, 0xFF};	// CMD9 with dummy CRC
static const uint8_t SD_CMD17[] = {0x51, 0x00, 0x00, 0x00, 0x00, 0xFF}; // CMD17 with dummy CRC

static const spi_device_t sd_dev = {.cs_pin = SD_CSN_PIN,
	.mode = SPI_MODE_0,
	.frequency = SPI_FREQ_250K,
	.order = SPI_MSB_FIRST,
	.dummy_byte = 0xFF};

static uint64_t block_count = 0;

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

	if (sd_exec_cmd(SD_CMD0, &r1, NULL, 0)) {
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

	if (sd_exec_cmd(SD_CMD8, &r1, r7, sizeof(r7)) != 0) {

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

		if (sd_exec_cmd(SD_CMD55, &r1, NULL, 0) != 0) {

			logger_log_literal_len("SD INIT:",
				(uint8_t)(sizeof("SD INIT:") - 1),
				"CMD55 FAILED",
				(uint8_t)(sizeof("CMD55 FAILED") - 1));
			return -1;
		}

		if (sd_exec_cmd(SD_ACMD41, &r1, NULL, 0) != 0) {

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

	if (sd_exec_cmd(SD_CMD58, &r1, ocr, 4) != 0) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD58 FAILED",
			(uint8_t)(sizeof("CMD58 FAILED") - 1));
		return -1;
	}

	if (r1 != 0x00) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD58 ERROR",
			(uint8_t)(sizeof("CMD58 ERROR") - 1));
		return -1;
	}

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

	// CMD9 - get details about card capacity etc

	vTaskDelay(pdMS_TO_TICKS(10));

	r1 = 0xFF;

	if (sd_begin_cmd(SD_CMD9, &r1) != 0) {
		spi_end();
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD9 FAILED",
			(uint8_t)(sizeof("CMD9 FAILED") - 1));
		return -1;
	}

	if (r1 != 0x00) {
		spi_end();
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD9 ERROR",
			(uint8_t)(sizeof("CMD9 ERROR") - 1));
		return -1;
	}

	if (sd_wait_token(0xFE, 100) != 0) {
		spi_end();
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD9 DATA TOKEN TIMEOUT",
			(uint8_t)(sizeof("CMD9 DATA TOKEN TIMEOUT") - 1));
		return -1;
	}

	// CSD bytes
	uint8_t csd[16];

	if (spi_rx(csd, sizeof(csd)) != 0) {
		spi_end();
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD9 CSD READ FAILED",
			(uint8_t)(sizeof("CMD9 CSD READ FAILED") - 1));
		return -1;
	}

	logger_log_hex_len("SD INIT:CMD9/CSD", sizeof("SD INIT:CMD9/CSD") - 1, csd, sizeof(csd));

	// CRC bytes
	uint8_t crc[2];
	if (spi_rx(crc, sizeof(crc)) != 0) {
		spi_end();
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD9 CRC READ FAILED",
			(uint8_t)(sizeof("CMD9 CRC READ FAILED") - 1));
		return -1;
	}

	logger_log_hex_len("SD INIT:CMD9/CRC", sizeof("SD INIT:CMD9/CRC") - 1, crc, sizeof(crc));

	if (sd_end_cmd() != 0) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD9 END FAILED",
			(uint8_t)(sizeof("CMD9 END FAILED") - 1));
		return -1;
	}

	// decode CSD data to get capacity and block count
	// get C_SIZE
	uint32_t c_size =
		((uint32_t)(csd[7] & 0x3F) << 16) | ((uint32_t)csd[8] << 8) | (uint32_t)csd[9];

	block_count = (c_size + 1U) * 1024U;
	logger_log_uint_len("SD INIT:BLOCK COUNT",
		sizeof("SD INIT:BLOCK COUNT") - 1,
		&block_count,
		sizeof(block_count));

	// temporary - testing block read
	vTaskDelay(pdMS_TO_TICKS(100));

	static uint8_t test_block[SD_BLOCK_LEN] = {0};

	if (sd_read_block(0, test_block) != 0) {
		logger_log_literal_len("SD INIT:TEST BLOCK 0",
			(uint8_t)(sizeof("SD INIT:TEST BLOCK 0") - 1),
			"READ FAILED",
			(uint8_t)(sizeof("READ FAILED") - 1));
		return -1;
	}

	logger_log_hex_len("SD READ:BLOCK 0",
		(uint8_t)(sizeof("SD READ:BLOCK 0") - 1),
		test_block,
		64);

	logger_log_hex_len("SD READ:BLOCK 0 END",
		(uint8_t)(sizeof("SD READ:BLOCK 0 END") - 1),
		&test_block[SD_BLOCK_LEN - 32],
		32);

	return 0;
}

int sd_end_cmd(void) {
	spi_end();
	return spi_clock_idle(&sd_dev, 1);
}

int sd_wait_token(const uint8_t expected, uint32_t timeout_ms) {
	TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

	if (timeout_ticks == 0) {
		timeout_ticks = 1;
	}

	uint8_t token = 0xFF;
	TickType_t start_tick = xTaskGetTickCount();

	while ((xTaskGetTickCount() - start_tick) < timeout_ticks) {
		if (spi_rx(&token, 1) != 0) {
			return -1;
		}

		if (token == expected) {
			return 0;
		}

		if (token != 0xFF) {
			// Unexpected token or SD data-error token.
			return -1;
		}
	}

	return -1;
}

int sd_exec_cmd(const uint8_t cmd[8], uint8_t* r1, uint8_t* extra, size_t extra_len) {

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

int sd_begin_cmd(const uint8_t cmd[8], uint8_t* r1) {
	if (cmd == NULL || r1 == NULL)
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

	return 0;
}

int sd_read_block(uint32_t block_number, uint8_t* buffer) {

	if (buffer == NULL) {
		return -1;
	}

	if (block_number >= block_count) {
		logger_log_literal_len("SD READ:",
			(uint8_t)(sizeof("SD READ:") - 1),
			"BLOCK NUMBER OUT OF RANGE",
			(uint8_t)(sizeof("BLOCK NUMBER OUT OF RANGE") - 1));
		return -1;
	}

	uint8_t r1 = 0xFF;

	if (sd_begin_cmd(SD_CMD17, &r1) != 0) {
		spi_end();
		logger_log_literal_len("SD READ BLOCK:",
			(uint8_t)(sizeof("SD READ BLOCK:") - 1),
			"CMD17 FAILED",
			(uint8_t)(sizeof("CMD17 FAILED") - 1));
		return -1;
	}

	logger_log_uint_len("SD READ BLOCK:R1",
		(uint8_t)(sizeof("SD READ BLOCK:R1") - 1),
		&r1,
		sizeof(r1));

	if (r1 != 0x00) {
		spi_end();
		logger_log_literal_len("SD READ BLOCK:",
			(uint8_t)(sizeof("SD READ BLOCK:") - 1),
			"CMD17 ERROR",
			(uint8_t)(sizeof("CMD17 ERROR") - 1));
		return -1;
	}

	if (sd_wait_token(0xFE, 100) != 0) {
		spi_end();
		logger_log_literal_len("SD READ:BLOCK:",
			(uint8_t)(sizeof("SD READ BLOCK:") - 1),
			"CMD17 DATA TOKEN TIMEOUT",
			(uint8_t)(sizeof("CMD17 DATA TOKEN TIMEOUT") - 1));
		return -1;
	}

	// read the 512 bytes of data

	if (spi_rx(buffer, SD_BLOCK_LEN) != 0) {
		spi_end();
		logger_log_literal_len("SD READ:BLOCK:",
			(uint8_t)(sizeof("SD READ BLOCK:") - 1),
			"CMD17 DATA READ FAILED",
			(uint8_t)(sizeof("CMD17 DATA READ FAILED") - 1));
		return -1;
	}

	// read the 2 CRC bytes (ignored for now)
	uint8_t crc[2];

	if (spi_rx(crc, sizeof(crc)) != 0) {
		spi_end();
		logger_log_literal_len("SD READ:BLOCK:",
			(uint8_t)(sizeof("SD READ BLOCK:") - 1),
			"CMD17 CRC READ FAILED",
			(uint8_t)(sizeof("CMD17 CRC READ FAILED") - 1));
		return -1;
	}

	if (sd_end_cmd() != 0) {
		logger_log_literal_len("SD READ:BLOCK:",
			(uint8_t)(sizeof("SD READ BLOCK:") - 1),
			"CMD17 END FAILED",
			(uint8_t)(sizeof("CMD17 END FAILED") - 1));
		return -1;
	}

	return 0;
}
