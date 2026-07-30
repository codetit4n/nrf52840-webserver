#pragma once

#include <stddef.h>
#include <stdint.h>

#define SD_BLOCK_LEN 512       // fixed for SDHC and SDXC
#define SD_INIT_CLOCK_BYTES 10 // 10 bytes = 80 clock cycles
#define SD_R1_POLL_TRIES 10

typedef enum {
	SD_OK = 0,
	SD_ERR_INVALID_ARG,
	SD_ERR_NOT_INITIALIZED,
	SD_ERR_SPI,
	SD_ERR_TIMEOUT,
	SD_ERR_CMD_RESPONSE,
	SD_ERR_DATA_TOKEN_TIMEOUT,
	SD_ERR_DATA_TOKEN,
	SD_ERR_UNSUPPORTED_CARD
} sd_status_t;

sd_status_t sd_init(void);
sd_status_t sd_read_block(uint32_t block, uint8_t* buffer);
