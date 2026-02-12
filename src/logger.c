#include "logger.h"
#include "FreeRTOS.h"
#include "board.h"
#include "drivers/uarte.h"
#include "portmacro.h"
#include "semphr.h"
#include <stddef.h>
#include <stdint.h>

static log_t log_q[LOGGER_QUEUE_CAP];
static uint8_t front; // read idx
static uint8_t rear;  // write idx
static uint8_t ctr;   // number of valid entries (0..CAP)
static uint8_t dropped;

static SemaphoreHandle_t log_mutex = NULL;
static StaticSemaphore_t log_mutex_buf;

static inline uint8_t idx_next(uint8_t i) {
	++i;
	return (i >= LOGGER_QUEUE_CAP) ? 0 : i; // wrap around
}

void logger_init(void) {
	uarte_init();
	front = rear = ctr = dropped = 0;
	log_mutex = xSemaphoreCreateMutexStatic(&log_mutex_buf);
}

// Enqueue log entry: overwrite oldest when full
void logger_log(log_t log) {

	if (log.len > LOGGER_MAX_LOG_PAYLOAD) {
		log.len = LOGGER_MAX_LOG_PAYLOAD;
	}

	xSemaphoreTake(log_mutex, portMAX_DELAY);

	if (ctr == LOGGER_QUEUE_CAP) {	 // full queue
		front = idx_next(front); // drop oldest, ctr stays at CAP
		++dropped;
	} else {
		++ctr;
	}

	log_q[rear] = log; // struct copy
	rear = idx_next(rear);

	xSemaphoreGive(log_mutex);
}

uint8_t logger_try_pop(log_t* out) {
	if (out == NULL) {
		return 0;
	}

	uint8_t ok = 0;

	xSemaphoreTake(log_mutex, portMAX_DELAY);

	if (ctr > 0) {
		*out = log_q[front];
		front = idx_next(front);
		--ctr;
		ok = 1;
	}

	xSemaphoreGive(log_mutex);
	return ok;
}

static uint8_t* parse_hex(uint8_t* payload, uint8_t len) {
	// convert raw data to 0xXX hex string to be sent over UART no \0
	static uint8_t hex_buf[LOGGER_MAX_LOG_PAYLOAD * 2];
	for (uint8_t i = 0; i < len; ++i) {
		uint8_t byte = payload[i];
		hex_buf[2 * i] = "0123456789ABCDEF"[byte >> 4];
		hex_buf[2 * i + 1] = "0123456789ABCDEF"[byte & 0x0F];
	}
	return hex_buf;
}

void logger_task(void* arg) {
	(void)arg;

	for (;;) {
		log_t log = {0};
		uint8_t success = logger_try_pop(&log);
		if (success) {
			switch (log.type) {
			case LOG_HEX:
				uarte_write(parse_hex(log.payload, log.len), log.len);
				uarte_write((uint8_t*)"\r\n", 2);
				break;
			case LOG_STRING:
			default:
				uarte_write(log.payload, log.len);
			}
		} else {
			vTaskDelay(1);
		}
	}
}
