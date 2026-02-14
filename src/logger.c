#include "logger.h"
#include "FreeRTOS.h"
#include "board.h"
#include "drivers/uarte.h"
#include "memutils.h"
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

// Converts uint32_t to ascii decimals, returns length.
static uint8_t format_u32(uint32_t value, uint8_t* out) {

	uint8_t tmp[10];
	uint8_t n = 0;

	if (out == NULL)
		return 0;

	// special case
	if (value == 0) {
		out[0] = (uint8_t)'0';
		return 1;
	}

	// Extract digits in reverse order
	while (value != 0 && n < sizeof(tmp)) {
		uint32_t digit = value % 10u;
		tmp[n++] = (uint8_t)('0' + digit);
		value /= 10u;
	}

	// Reverse to get correct order
	for (uint8_t i = 0; i < n; i++) {
		out[i] = tmp[n - 1 - i];
	}

	return n; // actual length
}

// Converts byte array to ascii hex, returns length.
static uint8_t format_hex_bytes(const uint8_t* in, size_t in_len, uint8_t* out) {
	if (in == NULL || out == NULL) {
		return 0;
	}

	for (size_t i = 0; i < in_len; i++) {
		uint8_t b = in[i];

		uint8_t high = b >> 4;
		uint8_t low = b & 0x0F;

		out[2 * i] = (high < 10) ? (uint8_t)('0' + high) : (uint8_t)('A' + (high - 10));
		out[2 * i + 1] = (low < 10) ? (uint8_t)('0' + low) : (uint8_t)('A' + (low - 10));
	}

	return (uint8_t)(2u * in_len);
}

static void fill_label(const uint8_t* label, log_t* log) {
	for (uint8_t i = 0; i < LOGGER_MAX_LOG_LABEL; i++) {
		if (label[i] != '\0') {
			log->label[i] = (uint8_t)label[i];
		} else {
			// pad remaining with spaces
			for (; i < LOGGER_MAX_LOG_LABEL; i++) {
				log->label[i] = ' ';
			}
			break;
		}
	}
}

void logger_task(void* arg) {
	(void)arg;

	for (;;) {
		log_t log = {0};
		uint8_t success = logger_try_pop(&log);
		if (success) {

			fill_label(log.label, &log);
			uarte_write(log.label, LOGGER_MAX_LOG_LABEL);

			switch (log.type) {
			case LOG_UINT: {
				uint32_t u32 = 0;
				memcpy_u8((uint8_t*)&u32, (const uint8_t*)log.payload, sizeof(u32));

				uint8_t out[10]; // max uint32_t is 10 digits
				uint8_t out_len = format_u32(u32, out);

				uarte_write(out, out_len);
				break;
			}
			case LOG_HEX: {
				uint8_t out[2 * LOGGER_MAX_LOG_PAYLOAD]; // each byte -> 2 hex chars
				uint8_t out_len =
					format_hex_bytes((const uint8_t*)log.payload, log.len, out);

				uarte_write(out, out_len);
				break;
			}
			case LOG_STRING:
			default:
				uarte_write(log.payload, log.len);
			}
			uarte_write((uint8_t*)"\r\n", 2);
		} else {
			vTaskDelay(1);
		}
	}
}
