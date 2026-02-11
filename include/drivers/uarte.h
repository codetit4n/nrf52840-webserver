#pragma once

#include <stddef.h>
#include <stdint.h>

#define UART_TX_BUF_SIZE 256

uint8_t uarte_write(const uint8_t* data, size_t len);
void uarte_init(void);
void uarte_recover(void);
