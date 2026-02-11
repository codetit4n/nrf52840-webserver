#include "FreeRTOS.h"
#include "task.h"

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
	(void)xTask;
	(void)pcTaskName;
	taskDISABLE_INTERRUPTS();
	for (;;)
		;
}

void vApplicationMallocFailedHook(void) {
	taskDISABLE_INTERRUPTS();
	for (;;)
		;
}

void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer,
	StackType_t** ppxIdleTaskStackBuffer,
	uint32_t* pulIdleTaskStackSize) {
	static StaticTask_t xIdleTaskTCB;
	static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
