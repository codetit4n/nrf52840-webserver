#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Basic scheduler configuration
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 0

/*-----------------------------------------------------------
 * Clock and tick configuration
 *----------------------------------------------------------*/

/* nRF52840 runs at 64 MHz after SystemInit() */
#define configCPU_CLOCK_HZ (64000000UL)

/* 1 ms tick */
#define configTICK_RATE_HZ (1000)

/*-----------------------------------------------------------
 * Task and memory configuration
 *----------------------------------------------------------*/

/* Max number of priorities (0 .. 2) */
#define configMAX_PRIORITIES (3)

/* Stack sizes are in WORDS (not bytes) */
#define configMINIMAL_STACK_SIZE (128)

/* REQUIRED on Cortex-M4F */
#define configBYTE_ALIGNMENT 8

/* Heap used by heap_4.c */
#define configTOTAL_HEAP_SIZE (32 * 1024)

/* Task name length including '\0' */
#define configMAX_TASK_NAME_LEN (8)

#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1

/*-----------------------------------------------------------
 * Synchronisation primitives
 *----------------------------------------------------------*/

#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 1
#define configUSE_COUNTING_SEMAPHORES 1

/*-----------------------------------------------------------
 * Software timers (DISABLED for now)
 *----------------------------------------------------------*/

#define configUSE_TIMERS 0

/*-----------------------------------------------------------
 * Hook functions (ENABLE for safety)
 *----------------------------------------------------------*/

#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0

#define configCHECK_FOR_STACK_OVERFLOW 2
#define configUSE_MALLOC_FAILED_HOOK 1

/*-----------------------------------------------------------
 * Diagnostics and stats (DISABLED for now)
 *----------------------------------------------------------*/

#define configGENERATE_RUN_TIME_STATS 0
#define configUSE_TRACE_FACILITY 0
#define configUSE_STATS_FORMATTING_FUNCTIONS 0

/*-----------------------------------------------------------
 * Co-routines (DISABLED)
 *----------------------------------------------------------*/

#define configUSE_CO_ROUTINES 0

/*-----------------------------------------------------------
 * Assertions
 *----------------------------------------------------------*/

#define configASSERT(x)                                                                            \
	if ((x) == 0) {                                                                            \
		taskDISABLE_INTERRUPTS();                                                          \
		for (;;)                                                                           \
			;                                                                          \
	}

/*-----------------------------------------------------------
 * Cortex-M interrupt priority configuration
 *----------------------------------------------------------*/

/* nRF52840 implements 3 priority bits */
#define configPRIO_BITS 3

/* Lowest priority (numerically highest) */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 7

/* Highest priority allowed to call FreeRTOS API */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configKERNEL_INTERRUPT_PRIORITY                                                            \
	(configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#define configMAX_SYSCALL_INTERRUPT_PRIORITY                                                       \
	(configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/*-----------------------------------------------------------
 * Interrupt handler mapping
 *----------------------------------------------------------*/

#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

/*-----------------------------------------------------------
 * API inclusion (safe defaults)
 *----------------------------------------------------------*/

#define INCLUDE_vTaskDelay 1
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_vTaskDelete 1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1

/*-----------------------------------------------------------
 * Memory Allocation
 *----------------------------------------------------------*/
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configSUPPORT_STATIC_ALLOCATION 1

#endif /* FREERTOS_CONFIG_H */
