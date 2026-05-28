#include "FreeRTOS.h" // IWYU pragma: keep
#include "task.h"

#include "drivers/sd.h"
#include "modules/logger.h"

void sd_task(void* arg) {
	(void)arg;

	// Give SD card/module power some time to settle.
	vTaskDelay(pdMS_TO_TICKS(200));

	logger_log_literal_len("SD TASK:",
		(uint8_t)(sizeof("SD TASK:") - 1),
		"STARTED",
		(uint8_t)(sizeof("STARTED") - 1));

	if (sd_init() != 0) {
		logger_log_literal_len("SD TASK:",
			(uint8_t)(sizeof("SD TASK:") - 1),
			"INIT FAILED",
			(uint8_t)(sizeof("INIT FAILED") - 1));
	} else {
		logger_log_literal_len("SD TASK:",
			(uint8_t)(sizeof("SD TASK:") - 1),
			"INIT DONE",
			(uint8_t)(sizeof("INIT DONE") - 1));
	}

	for (;;) {
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void sd_module_init(void) {

	BaseType_t ok = xTaskCreate(sd_task, /* Task function */
		"sd_task",		     /* Name (for debug) */
		1024,			     /* Stack size (words, not bytes) */
		NULL,			     /* Parameters */
		2,			     /* Priority */
		NULL			     /* Task handle */
	);

	if (ok != pdPASS) {
		taskDISABLE_INTERRUPTS();
		for (;;)
			;
	}
}
