#pragma once

#include <stddef.h>
#include <stdint.h>

#define SD_CSN_PIN 4
#define SD_BLOCK_LEN 512       // fixed for SDHC and SDXC
#define SD_INIT_CLOCK_BYTES 10 // 10 bytes = 80 clock cycles
#define SD_R1_POLL_TRIES 10

int sd_init(void);
int sd_send_cmd(const uint8_t cmd[8], uint8_t* r1, uint8_t* extra, size_t extra_len);
void sd_read_block(void);
