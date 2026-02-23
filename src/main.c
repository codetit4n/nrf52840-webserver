#include "FreeRTOS.h" // IWYU pragma: keep
#include "drivers/spi.h"
#include "logger.h"
#include "net.h"
#include "task.h"
#include <stdint.h>

int main(void) {
	spim_init();
	logger_init();
	net_init();

	vTaskStartScheduler();

	taskDISABLE_INTERRUPTS();
	for (;;)
		;

	return 0;
}
