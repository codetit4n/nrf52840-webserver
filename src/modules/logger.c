#include "modules/logger.h"
#include "FreeRTOS.h" // IWYU pragma: keep
#include "drivers/uarte.h"
#include "memutils.h"
#include "semphr.h"
#include "task.h"
#include <stdint.h>

static log_t log_q[LOGGER_QUEUE_CAP];
static volatile uint8_t front;	 // read idx
static volatile uint8_t rear;	 // write idx
static volatile uint8_t ctr;	 // number of valid entries (0..CAP)
static volatile uint8_t dropped; // will use later

static TaskHandle_t logger_task_handle = NULL;
static SemaphoreHandle_t logger_flush_done = NULL;
static volatile uint8_t logger_flush_requested = 0;

static inline uint8_t idx_next(uint8_t i) {
	++i;
	return (i >= LOGGER_QUEUE_CAP) ? 0 : i; // wrap around
}

void logger_init(void) {
	uarte_init();
	front = rear = ctr = dropped = 0;
	logger_flush_requested = 0;

	logger_flush_done = xSemaphoreCreateBinary();

	if (logger_flush_done == NULL) {
		taskDISABLE_INTERRUPTS();
		for (;;)
			;
	}

	BaseType_t ok = xTaskCreate(logger_task, /* Task function */
		"logger_task",			 /* Name (for debug) */
		256,				 /* Stack size (words, not bytes) */
		NULL,				 /* Parameters */
		1, /* Priority  NOTE: Increase this if you want to see some important logs which are
		      not logging*/
		&logger_task_handle /* Task handle */
	);

	if (ok != pdPASS) {
		taskDISABLE_INTERRUPTS();
		for (;;)
			;
	}
}
void logger_flush(void) {
	if (logger_task_handle == NULL || logger_flush_done == NULL) {
		return;
	}

	taskENTER_CRITICAL();
	logger_flush_requested = 1;
	taskEXIT_CRITICAL();

	/* Ensure the logger task wakes even if it was waiting. */
	xTaskNotifyGive(logger_task_handle);

	/* Block until logger_task confirms the queue was drained. */
	xSemaphoreTake(logger_flush_done, portMAX_DELAY);
}

// Enqueue log entry: overwrite oldest when full
void logger_log(log_t log) {
	if (log.len > LOGGER_MAX_LOG_PAYLOAD) {
		log.len = LOGGER_MAX_LOG_PAYLOAD;
	}
	uint8_t was_empty = 0;

	taskENTER_CRITICAL();
	was_empty = (ctr == 0);

	if (ctr == LOGGER_QUEUE_CAP) {	 // full queue
		front = idx_next(front); // drop oldest, ctr stays at CAP
		++dropped;
	} else {
		++ctr;
	}

	log_q[rear] = log; // struct copy
	rear = idx_next(rear);

	taskEXIT_CRITICAL();

	if (was_empty && logger_task_handle != NULL) {
		xTaskNotifyGive(logger_task_handle); // wake logger task if it was waiting for logs
	}
}

uint8_t logger_try_pop(log_t* out) {
	if (out == NULL) {
		return 0;
	}

	uint8_t ok = 0;

	taskENTER_CRITICAL();

	if (ctr > 0) {
		*out = log_q[front];
		front = idx_next(front);
		--ctr;
		ok = 1;
	}

	taskEXIT_CRITICAL();

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

	log_t log = {0};

	uint8_t line[LOGGER_MAX_LOG_LABEL + (2 * LOGGER_MAX_LOG_PAYLOAD) + 2];

	ulTaskNotifyTake(pdTRUE, 0); // clear pending notification

	for (;;) {
		// Drain the queue.
		while (logger_try_pop(&log)) {
			size_t line_len = 0;

			fill_label(log.label, &log);

			mem_cpy(&line[line_len], log.label, LOGGER_MAX_LOG_LABEL);
			line_len += LOGGER_MAX_LOG_LABEL;

			switch (log.type) {
			case LOG_UINT: {
				uint32_t value = 0;
				mem_cpy(&value, log.payload, sizeof(value));

				line_len += format_u32(value, &line[line_len]);
				break;
			}

			case LOG_HEX:
				line_len += format_hex_bytes(log.payload, log.len, &line[line_len]);
				break;

			case LOG_STRING:
			default:
				mem_cpy(&line[line_len], log.payload, log.len);
				line_len += log.len;
				break;
			}

			line[line_len++] = '\r';
			line[line_len++] = '\n';

			// Send the complete log line using one UARTE transfer.
			uarte_write(line, line_len);
		}

		uint8_t signal_flush = 0;

		taskENTER_CRITICAL();

		if (logger_flush_requested && ctr == 0) {
			logger_flush_requested = 0;
			signal_flush = 1;
		}

		taskEXIT_CRITICAL();

		if (signal_flush) {
			xSemaphoreGive(logger_flush_done);
		}

		// Block until a producer adds another log entry.
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	}
}

void logger_log_literal_len(const char* label,
	uint8_t label_len,
	const char* text,
	uint8_t text_len) {
	log_t l = {0};
	l.type = LOG_STRING;

	if (label_len > LOGGER_MAX_LOG_LABEL)
		label_len = LOGGER_MAX_LOG_LABEL;

	if (text_len > LOGGER_MAX_LOG_PAYLOAD)
		text_len = LOGGER_MAX_LOG_PAYLOAD;

	if (label && label_len)
		mem_cpy(l.label, label, label_len);

	if (text && text_len) {
		mem_cpy(l.payload, text, text_len);
		l.len = text_len;
	} else {
		l.len = 0;
	}

	logger_log(l);
}

void logger_log_uint_len(const char* label,
	uint8_t label_len,
	const void* value,
	uint8_t value_len) {
	log_t l = {0};
	l.type = LOG_UINT;

	if (label_len > LOGGER_MAX_LOG_LABEL)
		label_len = LOGGER_MAX_LOG_LABEL;

	if (value_len > LOGGER_MAX_LOG_PAYLOAD)
		value_len = LOGGER_MAX_LOG_PAYLOAD;

	if (label && label_len)
		mem_cpy(l.label, label, label_len);

	if (value && value_len) {
		mem_cpy(l.payload, value, value_len);
		l.len = value_len;
	} else {
		l.len = 0;
	}

	logger_log(l);
}

void logger_log_hex_len(const char* label,
	uint8_t label_len,
	const uint8_t* data,
	uint8_t data_len) {
	log_t l = {0};
	l.type = LOG_HEX;

	if (label_len > LOGGER_MAX_LOG_LABEL)
		label_len = LOGGER_MAX_LOG_LABEL;

	if (data_len > LOGGER_MAX_LOG_PAYLOAD)
		data_len = LOGGER_MAX_LOG_PAYLOAD;

	if (label && label_len)
		mem_cpy(l.label, label, label_len);

	if (data && data_len) {
		mem_cpy(l.payload, data, data_len);
		l.len = data_len;
	} else {
		l.len = 0;
	}

	logger_log(l);
}

void logger_log_nl(void) {
	logger_log_literal_len("", 0, "\r\n", 2);
}
