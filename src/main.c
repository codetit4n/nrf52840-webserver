#include "FreeRTOS.h" // IWYU pragma: keep
#include "drivers/spi.h"
#include "modules/logger.h"
#include "modules/net.h"
#include "task.h"

static void startup_task(void* arg) {
	(void)arg;

	// Scheduler dependent initialization
	logger_init();
	net_init();

	vTaskDelete(NULL);
}

int main(void) {
	// Baremetal initialization
	spim_init();

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
