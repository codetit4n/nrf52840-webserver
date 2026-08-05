#include "FreeRTOS.h"
#include "board.h"
#include "drivers/sd.h"
#include "drivers/spi.h"
#include "modules/logger.h"
#include "task.h"

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
static uint8_t sd_initialized = 0;

uint8_t sd_is_ready(void) {
	return sd_initialized;
}
uint64_t sd_get_block_count(void) {
	return block_count;
}

static sd_status_t sd_begin_cmd(const uint8_t cmd[6], uint8_t* r1) {
	if (cmd == NULL || r1 == NULL) {
		return SD_ERR_INVALID_ARG;
	}

	if (spi_begin(&sd_dev) != 0) {
		return SD_ERR_SPI;
	}

	if (spi_tx(cmd, 6) != 0) {
		spi_end();
		return SD_ERR_SPI;
	}

	*r1 = 0xFF;

	for (int tries = 0; tries < SD_R1_POLL_TRIES; tries++) {
		if (spi_rx(r1, 1) != 0) {
			spi_end();
			return SD_ERR_SPI;
		}

		if (*r1 != 0xFF) {
			break;
		}
	}

	if (*r1 == 0xFF) {
		spi_end();
		return SD_ERR_TIMEOUT;
	}

	return SD_OK;
}

static sd_status_t sd_wait_token(const uint8_t expected, uint32_t timeout_ms) {
	TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

	if (timeout_ticks == 0) {
		timeout_ticks = 1;
	}

	uint8_t token = 0xFF;
	TickType_t start_tick = xTaskGetTickCount();

	while ((xTaskGetTickCount() - start_tick) < timeout_ticks) {
		if (spi_rx(&token, 1) != 0) {
			return SD_ERR_SPI;
		}

		if (token == expected) {
			return SD_OK;
		}

		if (token != 0xFF) {
			// Unexpected token or SD data-error token.

			logger_log_uint_len("SD WAIT TOKEN:UNEXPECTED TOKEN",
				sizeof("SD WAIT TOKEN:UNEXPECTED TOKEN") - 1,
				&token,
				sizeof(token));

			return SD_ERR_DATA_TOKEN;
		}
	}

	return SD_ERR_DATA_TOKEN_TIMEOUT;
}

static sd_status_t
sd_exec_cmd(const uint8_t cmd[6], uint8_t* r1, uint8_t* extra, size_t extra_len) {
	if (cmd == NULL || r1 == NULL) {
		return SD_ERR_INVALID_ARG;
	}

	if (extra_len > 0 && extra == NULL) {
		return SD_ERR_INVALID_ARG;
	}

	if (spi_begin(&sd_dev) != 0) {
		return SD_ERR_SPI;
	}

	if (spi_tx(cmd, 6) != 0) {
		spi_end();
		return SD_ERR_SPI;
	}

	*r1 = 0xFF;

	for (int tries = 0; tries < SD_R1_POLL_TRIES; tries++) {
		if (spi_rx(r1, 1) != 0) {
			spi_end();
			return SD_ERR_SPI;
		}

		if (*r1 != 0xFF) {
			break;
		}
	}

	if (*r1 == 0xFF) {
		spi_end();
		return SD_ERR_TIMEOUT;
	}

	if (extra_len > 0) {
		if (spi_rx(extra, extra_len) != 0) {
			spi_end();
			return SD_ERR_SPI;
		}
	}

	spi_end();

	if (spi_clock_idle(&sd_dev, 1) != 0) {
		return SD_ERR_SPI;
	}

	return SD_OK;
}

static void sd_end_cmd(void) {
	spi_end();
	spi_clock_idle(&sd_dev, 1);
}

sd_status_t sd_init(void) {
	sd_initialized = 0;
	block_count = 0;

	spi_device_init(&sd_dev);

	if (spi_clock_idle(&sd_dev, SD_INIT_CLOCK_BYTES) != 0) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"IDLE CLOCK FAILED",
			(uint8_t)(sizeof("IDLE CLOCK FAILED") - 1));

		return SD_ERR_SPI;
	}

	// CMD0

	uint8_t r1 = 0xFF;

	sd_status_t status = sd_exec_cmd(SD_CMD0, &r1, NULL, 0);

	if (status != SD_OK) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD0 FAILED",
			(uint8_t)(sizeof("CMD0 FAILED") - 1));

		return status;
	}

	// CMD8

	r1 = 0xFF;
	uint8_t r7[4] = {0};

	status = sd_exec_cmd(SD_CMD8, &r1, r7, sizeof(r7));

	if (status != SD_OK) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD8 FAILED",
			(uint8_t)(sizeof("CMD8 FAILED") - 1));

		return status;
	}

	if (r1 == 0x05) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"LEGACY CARD REJECTED",
			(uint8_t)(sizeof("LEGACY CARD REJECTED") - 1));

		return SD_ERR_UNSUPPORTED_CARD;
	}

	if (r1 != 0x01 || r7[0] != 0x00 || r7[1] != 0x00 || r7[2] != 0x01 || r7[3] != 0xAA) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"UNKNOWN CMD8 RESPONSE",
			(uint8_t)(sizeof("UNKNOWN CMD8 RESPONSE") - 1));

		return SD_ERR_CMD_RESPONSE;
	}

	// CMD55 + ACMD41 initialization loop

	r1 = 0xFF;
	uint8_t initialized = 0;

	for (uint32_t i = 0; i < 100; i++) {
		status = sd_exec_cmd(SD_CMD55, &r1, NULL, 0);

		if (status != SD_OK) {
			logger_log_literal_len("SD INIT:",
				(uint8_t)(sizeof("SD INIT:") - 1),
				"CMD55 FAILED",
				(uint8_t)(sizeof("CMD55 FAILED") - 1));

			return status;
		}

		status = sd_exec_cmd(SD_ACMD41, &r1, NULL, 0);

		if (status != SD_OK) {
			logger_log_literal_len("SD INIT:",
				(uint8_t)(sizeof("SD INIT:") - 1),
				"ACMD41 FAILED",
				(uint8_t)(sizeof("ACMD41 FAILED") - 1));

			return status;
		}

		if (r1 == 0x00) {
			initialized = 1;
			break;
		}

		if (r1 != 0x01) {
			logger_log_literal_len("SD INIT:",
				(uint8_t)(sizeof("SD INIT:") - 1),
				"ACMD41 ERROR",
				(uint8_t)(sizeof("ACMD41 ERROR") - 1));

			return SD_ERR_CMD_RESPONSE;
		}

		vTaskDelay(pdMS_TO_TICKS(10));
	}

	if (!initialized) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"ACMD41 TIMEOUT",
			(uint8_t)(sizeof("ACMD41 TIMEOUT") - 1));

		return SD_ERR_TIMEOUT;
	}

	// CMD58

	r1 = 0xFF;
	uint8_t ocr[4] = {0};

	status = sd_exec_cmd(SD_CMD58, &r1, ocr, sizeof(ocr));

	if (status != SD_OK) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD58 FAILED",
			(uint8_t)(sizeof("CMD58 FAILED") - 1));

		return status;
	}

	if (r1 != 0x00) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD58 ERROR",
			(uint8_t)(sizeof("CMD58 ERROR") - 1));

		return SD_ERR_CMD_RESPONSE;
	}

	uint8_t ccs = (ocr[0] & 0x40) ? 1 : 0;

	if (!ccs) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"SDSC CARD - NOT SUPPORTED",
			(uint8_t)(sizeof("SDSC CARD - NOT SUPPORTED") - 1));

		return SD_ERR_UNSUPPORTED_CARD;
	}

	// CMD9 - get details about card capacity etc

	vTaskDelay(pdMS_TO_TICKS(10));

	r1 = 0xFF;

	status = sd_begin_cmd(SD_CMD9, &r1);

	if (status != SD_OK) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD9 BEGIN FAILED",
			(uint8_t)(sizeof("CMD9 BEGIN FAILED") - 1));

		return status;
	}

	if (r1 != 0x00) {
		sd_end_cmd();

		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD9 ERROR",
			(uint8_t)(sizeof("CMD9 ERROR") - 1));

		return SD_ERR_CMD_RESPONSE;
	}

	status = sd_wait_token(0xFE, 100);

	if (status != SD_OK) {
		sd_end_cmd();

		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD9 DATA TOKEN FAILED",
			(uint8_t)(sizeof("CMD9 DATA TOKEN FAILED") - 1));

		return status;
	}

	// CSD bytes

	uint8_t csd[16];

	if (spi_rx(csd, sizeof(csd)) != 0) {
		sd_end_cmd();

		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD9 CSD READ FAILED",
			(uint8_t)(sizeof("CMD9 CSD READ FAILED") - 1));

		return SD_ERR_SPI;
	}

	// CRC bytes

	uint8_t crc[2];

	if (spi_rx(crc, sizeof(crc)) != 0) {
		sd_end_cmd();

		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"CMD9 CRC READ FAILED",
			(uint8_t)(sizeof("CMD9 CRC READ FAILED") - 1));

		return SD_ERR_SPI;
	}

	sd_end_cmd();

	// decode CSD data to get capacity and block count
	// get C_SIZE

	uint32_t c_size =
		((uint32_t)(csd[7] & 0x3F) << 16) | ((uint32_t)csd[8] << 8) | (uint32_t)csd[9];

	block_count = ((uint64_t)c_size + 1ULL) * 1024ULL;

	sd_initialized = 1;

	return SD_OK;
}

sd_status_t sd_read_block(uint32_t block, uint8_t* buffer) {
	if (buffer == NULL) {
		return SD_ERR_INVALID_ARG;
	}

	if (!sd_initialized) {
		return SD_ERR_NOT_INITIALIZED;
	}

	if ((uint64_t)block >= block_count) {
		logger_log_literal_len("SD READ:",
			(uint8_t)(sizeof("SD READ:") - 1),
			"BLOCK NUMBER OUT OF RANGE",
			(uint8_t)(sizeof("BLOCK NUMBER OUT OF RANGE") - 1));

		return SD_ERR_INVALID_ARG;
	}

	// construct CMD17 command with the block number (big-endian)
	uint8_t cmd17[6] = {SD_CMD17[0],
		(uint8_t)(block >> 24),
		(uint8_t)(block >> 16),
		(uint8_t)(block >> 8),
		(uint8_t)block,
		SD_CMD17[5]};

	uint8_t r1 = 0xFF;

	sd_status_t status = sd_begin_cmd(cmd17, &r1);

	if (status != SD_OK) {
		logger_log_literal_len("SD READ BLOCK:",
			(uint8_t)(sizeof("SD READ BLOCK:") - 1),
			"CMD17 FAILED",
			(uint8_t)(sizeof("CMD17 FAILED") - 1));

		return status;
	}

	if (r1 != 0x00) {
		sd_end_cmd();

		logger_log_literal_len("SD READ BLOCK:",
			(uint8_t)(sizeof("SD READ BLOCK:") - 1),
			"CMD17 ERROR",
			(uint8_t)(sizeof("CMD17 ERROR") - 1));

		return SD_ERR_CMD_RESPONSE;
	}

	status = sd_wait_token(0xFE, 100);

	if (status != SD_OK) {
		sd_end_cmd();

		logger_log_literal_len("SD READ BLOCK:",
			(uint8_t)(sizeof("SD READ BLOCK:") - 1),
			"CMD17 DATA TOKEN FAILED",
			(uint8_t)(sizeof("CMD17 DATA TOKEN FAILED") - 1));

		return status;
	}

	// read the 512 bytes of data

	if (spi_rx(buffer, SD_BLOCK_LEN) != 0) {
		sd_end_cmd();

		logger_log_literal_len("SD READ BLOCK:",
			(uint8_t)(sizeof("SD READ BLOCK:") - 1),
			"CMD17 DATA READ FAILED",
			(uint8_t)(sizeof("CMD17 DATA READ FAILED") - 1));

		return SD_ERR_SPI;
	}

	// read the 2 CRC bytes (ignored for now)

	uint8_t crc[2];

	if (spi_rx(crc, sizeof(crc)) != 0) {
		sd_end_cmd();

		logger_log_literal_len("SD READ BLOCK:",
			(uint8_t)(sizeof("SD READ BLOCK:") - 1),
			"CMD17 CRC READ FAILED",
			(uint8_t)(sizeof("CMD17 CRC READ FAILED") - 1));

		return SD_ERR_SPI;
	}

	sd_end_cmd();

	return SD_OK;
}
