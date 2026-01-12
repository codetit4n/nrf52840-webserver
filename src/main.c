#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board.h"

static void led_task(void* arg) {
	(void)arg;

	for (;;) {
		board_led1_toggle();
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

int main(void) {
	/* Basic board init (GPIO only for now) */
	board_led1_init();

	/* Create LED task */
	BaseType_t ok = xTaskCreate(led_task, /* Task function */
				    "LED",    /* Name (for debug) */
				    128,      /* Stack size (words, not bytes) */
				    NULL,     /* Parameters */
				    1,	      /* Priority */
				    NULL      /* Task handle */
	);

	/* If task creation failed, halt */
	if (ok != pdPASS) {
		taskDISABLE_INTERRUPTS();
		for (;;)
			;
	}

	/* Start scheduler (never returns on success) */
	vTaskStartScheduler();

	/* Should never reach here */
	taskDISABLE_INTERRUPTS();
	for (;;)
		;
}
