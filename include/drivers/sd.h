#pragma once

#define SD_CSN_PIN 4
#define SD_BLOCK_LEN 512       // fixed for SDHC and SDXC
#define SD_INIT_CLOCK_BYTES 10 // 10 bytes = 80 clock cycles

void sd_init(void);
void sd_send_cmd(void);
void sd_read_block(void);
