#include "FreeRTOS.h" // IWYU pragma: keep
#include "board.h"
#include "drivers/sd.h"
#include "drivers/spi.h"
#include "modules/logger.h"
#include "modules/net.h"
#include "task.h"
#define SD_INIT_MAX_ATTEMPTS 3

#include "ff.h"

static FATFS fs;
static FIL file;

// Scheduler dependent initialization
static void startup_task(void* arg) {
	(void)arg;

	/*   SD CARD INIT */
	// Give SD card/module power some time to settle.
	vTaskDelay(pdMS_TO_TICKS(200));

	logger_log_literal_len("SD INIT:",
		(uint8_t)(sizeof("SD INIT:") - 1),
		"STARTED",
		(uint8_t)(sizeof("STARTED") - 1));

	int status = -1;

	for (uint8_t attempt = 0; attempt < SD_INIT_MAX_ATTEMPTS; attempt++) {
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

		vTaskDelay(pdMS_TO_TICKS(200));
	}

	if (status != SD_OK) {
		logger_log_literal_len("SD INIT:",
			(uint8_t)(sizeof("SD INIT:") - 1),
			"UNAVAILABLE",
			(uint8_t)(sizeof("UNAVAILABLE") - 1));
	} else {
		FRESULT fr = f_mount(&fs, "0:", 1);
		if (fr != FR_OK) {
			logger_log_literal_len("SD MOUNT:",
				(uint8_t)(sizeof("SD MOUNT:") - 1),
				"FAILED",
				(uint8_t)(sizeof("FAILED") - 1));
		} else {
			logger_log_literal_len("SD MOUNT:",
				(uint8_t)(sizeof("SD MOUNT:") - 1),
				"DONE",
				(uint8_t)(sizeof("DONE") - 1));

			char buffer[64];
			UINT bytes_read;

			FRESULT fr = f_open(&file, "0:/TEST.TXT", FA_READ);
			if (fr != FR_OK) {
				logger_log_literal_len("SD FILE OPEN:",
					(uint8_t)(sizeof("SD FILE OPEN:") - 1),
					"FAILED",
					(uint8_t)(sizeof("FAILED") - 1));
			}

			fr = f_read(&file, buffer, sizeof(buffer) - 1, &bytes_read);
			if (fr != FR_OK) {
				logger_log_literal_len("SD FILE READ:",
					(uint8_t)(sizeof("SD FILE READ:") - 1),
					"FAILED",
					(uint8_t)(sizeof("FAILED") - 1));

				f_close(&file);
			}

			buffer[bytes_read] = '\0';
			f_close(&file);

			logger_log_literal_len("SD FILE CONTENTS:",
				(uint8_t)(sizeof("SD FILE CONTENTS:") - 1),
				buffer,
				(uint8_t)bytes_read);
		}
	}

	logger_flush();

	/* Networking module init */
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

	vTaskDelete(NULL);
}

int main(void) {
	logger_init(); // need logger before anything else
	spim_init();   // spi used by 2 peripherals so get the driver ready
	pin_high(SD_CSN_PIN);
	pin_high(W5500_CSN_PIN);

	BaseType_t ok = xTaskCreate(startup_task, /* Task function */
		"startup",			  /* Name (for debug) */
		1024,				  /* Stack size (words, not bytes) */
		NULL,				  /* Parameters */
		2,				  /* Priority */
		NULL				  /* Task handle */
	);

	if (ok != pdPASS) {
		taskDISABLE_INTERRUPTS();
		for (;;)
			;
	}

	vTaskStartScheduler();

	// Should never reach here unless scheduler failed to start.
	taskDISABLE_INTERRUPTS();
	for (;;)
		;

	return 0;
}
