#include "FreeRTOS.h" // IWYU pragma: keep
#include "board.h"
#include "drivers/sd.h"
#include "drivers/spi.h"
#include "ff.h"
#include "modules/logger.h"
#include "modules/net.h"
#include "task.h"

static FATFS fs;

static int sd_recover(void) {
	sd_set_available(0);

	// Forget old FatFs mount state.
	f_mount(NULL, "0:", 0);

	logger_log_literal_len("SD RECOVER:",
		sizeof("SD RECOVER:") - 1,
		"STARTED",
		sizeof("STARTED") - 1);

	sd_status_t status = sd_init();

	if (status != SD_OK) {
		logger_log_literal_len("SD RECOVER:",
			sizeof("SD RECOVER:") - 1,
			"INIT FAILED",
			sizeof("INIT FAILED") - 1);

		return -1;
	}

	FRESULT fr = f_mount(&fs, "0:", 1);

	if (fr != FR_OK) {
		logger_log_literal_len("SD RECOVER:",
			sizeof("SD RECOVER:") - 1,
			"MOUNT FAILED",
			sizeof("MOUNT FAILED") - 1);

		return -1;
	}

	sd_set_available(1);

	logger_log_literal_len("SD RECOVER:",
		sizeof("SD RECOVER:") - 1,
		"DONE",
		sizeof("DONE") - 1);

	return 0;
}

static void startup_task(void* arg) {
	(void)arg;

	// Give SD card/module power some time to settle.
	vTaskDelay(pdMS_TO_TICKS(200));

	logger_log_literal_len("SD INIT:",
		(uint8_t)(sizeof("SD INIT:") - 1),
		"STARTED",
		(uint8_t)(sizeof("STARTED") - 1));

	int status = -1;

	for (uint8_t attempt = 0; attempt < SD_INIT_RECOVERY_MAX_ATTEMPTS; attempt++) {

		status = sd_init();

		if (status == SD_OK) {
			logger_log_literal_len("SD INIT:",
				(uint8_t)(sizeof("SD INIT:") - 1),
				"DONE",
				(uint8_t)(sizeof("DONE") - 1));

			break;
		}

		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"FAILED, RETRYING",
			(uint8_t)(sizeof("FAILED, RETRYING") - 1));

		vTaskDelay(pdMS_TO_TICKS(1000));
	}

	if (status != SD_OK) {

		sd_set_available(0);

		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"FAILED",
			(uint8_t)(sizeof("FAILED") - 1));

	} else {

		FRESULT fr = f_mount(&fs, "0:", 1);

		if (fr != FR_OK) {

			sd_set_available(0);

			logger_log_literal_len("SD INIT:",
				(uint8_t)(sizeof("SD INIT:") - 1),
				"MOUNT FAILED",
				(uint8_t)(sizeof("MOUNT FAILED") - 1));

		} else {

			sd_set_available(1);

			logger_log_literal_len("SD INIT:",
				(uint8_t)(sizeof("SD INIT:") - 1),
				"MOUNT DONE",
				(uint8_t)(sizeof("MOUNT DONE") - 1));
		}
	}

	logger_flush();

	logger_log_literal_len("NET INIT:",
		(uint8_t)(sizeof("NET INIT:") - 1),
		"STARTED",
		(uint8_t)(sizeof("STARTED") - 1));

	int st = net_init();

	if (st != 0) {

		logger_log_literal_len("NET INIT:",
			(uint8_t)(sizeof("NET INIT:") - 1),
			"FAILED",
			(uint8_t)(sizeof("FAILED") - 1));

	} else {

		logger_log_literal_len("NET INIT:",
			(uint8_t)(sizeof("NET INIT:") - 1),
			"DONE",
			(uint8_t)(sizeof("DONE") - 1));
	}

	for (;;) {

		if (sd_is_recovery_requested()) {

			sd_set_recovery_requested(0);

			uint8_t recovered = 0;

			for (uint8_t attempt = 0; attempt < SD_INIT_RECOVERY_MAX_ATTEMPTS;
				attempt++) {

				if (sd_recover() == 0) {
					recovered = 1;
					break;
				}

				vTaskDelay(pdMS_TO_TICKS(1000));
			}

			if (!recovered) {

				logger_log_literal_len("SD RECOVER:",
					sizeof("SD RECOVER:") - 1,
					"FAILED, STOPPING",
					sizeof("FAILED, STOPPING") - 1);
			}
		}

		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

int main(void) {
	logger_init(); // need logger before anything else
	spim_init();   // SPI used by both peripherals

	pin_high(SD_CSN_PIN);
	pin_high(W5500_CSN_PIN);

	BaseType_t ok = xTaskCreate(startup_task, "startup", 1024, NULL, 2, NULL);

	if (ok != pdPASS) {
		taskDISABLE_INTERRUPTS();

		for (;;)
			;
	}

	vTaskStartScheduler();

	/* Should never reach here unless scheduler failed to start. */
	taskDISABLE_INTERRUPTS();

	for (;;)
		;

	return 0;
}
