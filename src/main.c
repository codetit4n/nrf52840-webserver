#include "FreeRTOS.h"
#include "drivers/spi.h"
#include "logger.h"
#include "task.h"
#include "w5500.h"

void w5500_init(void);

// just testing w5500 port
static void net_task(void* arg) {
	(void)arg;

	logger_log_literal_len("NET:",
		(uint8_t)(sizeof("NET:") - 1),
		"INIT",
		(uint8_t)(sizeof("INIT") - 1));

	w5500_init();

	for (;;) {
		uint8_t ver = getVERSIONR();

		logger_log_hex_len("W5500 VERSIONR:",
			(uint8_t)(sizeof("W5500 VERSIONR:") - 1),
			&ver,
			1);

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

int main(void) {
	spim_init();
	logger_init();

	BaseType_t ok = xTaskCreate(net_task, "net_task", 512, NULL, 2, NULL);
	if (ok != pdPASS) {
		taskDISABLE_INTERRUPTS();
		for (;;)
			;
	}

	vTaskStartScheduler();

	taskDISABLE_INTERRUPTS();
	for (;;)
		;

	return 0;
}
