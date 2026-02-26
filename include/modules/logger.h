#pragma once

#include <stddef.h>
#include <stdint.h>

#define LOGGER_MAX_LOG_PAYLOAD 64
#define LOGGER_QUEUE_CAP 64
#define LOGGER_MAX_LOG_LABEL 16

typedef enum {
	LOG_HEX,
	LOG_UINT,
	LOG_STRING,
} payload_t;

typedef struct {
	payload_t type;
	uint8_t label[LOGGER_MAX_LOG_LABEL];
	uint8_t payload[LOGGER_MAX_LOG_PAYLOAD];
	uint8_t len;
} log_t;

void logger_init(void);
void logger_log(log_t log);
void logger_task(void* arg); // freertos

void logger_log_literal_len(const char* label,
	uint8_t label_len,
	const char* text,
	uint8_t text_len);
void logger_log_uint_len(const char* label,
	uint8_t label_len,
	const void* value,
	uint8_t value_len);
void logger_log_hex_len(const char* label,
	uint8_t label_len,
	const uint8_t* data,
	uint8_t data_len);
