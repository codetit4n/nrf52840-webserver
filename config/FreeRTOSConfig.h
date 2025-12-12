/*
 * FreeRTOS Kernel V10.0.0
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software. If you wish to use our Amazon
 * FreeRTOS name, please do so in a fair use way that does not cause confusion.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Possible configurations for system timer
 *----------------------------------------------------------*/
#define FREERTOS_USE_RTC      0  /* Use real time clock for the system */
#define FREERTOS_USE_SYSTICK  1  /* Use SysTick timer for system       */

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

/* Use SysTick as the RTOS tick source (simpler for bare-metal bringup). */
#define configTICK_SOURCE                         FREERTOS_USE_SYSTICK

#define configUSE_PREEMPTION                      1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION   1

/* Start without tickless idle to keep behaviour simple and predictable. */
#define configUSE_TICKLESS_IDLE                   0
#define configUSE_TICKLESS_IDLE_SIMPLE_DEBUG      0

/* Assume 64 MHz core clock for nRF52840 (adjust if you change clocks). */
#define configCPU_CLOCK_HZ                        ( 64000000UL )

/* 1024 Hz tick = ~0.976 ms per tick. You can make this 1000 if you prefer. */
#define configTICK_RATE_HZ                        ( 1024 )

#define configMAX_PRIORITIES                      ( 3 )

/* Stack sizes are in words (not bytes) on most Cortex-M ports. */
#define configMINIMAL_STACK_SIZE                  ( 60 )

/* Total heap for heap_4.c (tune as needed). */
#define configTOTAL_HEAP_SIZE                     ( 4096 )

#define configMAX_TASK_NAME_LEN                   ( 4 )
#define configUSE_16_BIT_TICKS                    0
#define configIDLE_SHOULD_YIELD                   1
#define configUSE_MUTEXES                         1
#define configUSE_RECURSIVE_MUTEXES               1
#define configUSE_COUNTING_SEMAPHORES             1
#define configUSE_ALTERNATIVE_API                 0    /* Deprecated. */
#define configQUEUE_REGISTRY_SIZE                 2
#define configUSE_QUEUE_SETS                      0
#define configUSE_TIME_SLICING                    0
#define configUSE_NEWLIB_REENTRANT                0
#define configENABLE_BACKWARD_COMPATIBILITY       1

/* Hook function related definitions. */
#define configUSE_IDLE_HOOK                       0
#define configUSE_TICK_HOOK                       0
#define configCHECK_FOR_STACK_OVERFLOW            0
#define configUSE_MALLOC_FAILED_HOOK              0

/* Run time and task stats gathering related definitions. */
#define configGENERATE_RUN_TIME_STATS             0
#define configUSE_TRACE_FACILITY                  0
#define configUSE_STATS_FORMATTING_FUNCTIONS      0

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES                     0
#define configMAX_CO_ROUTINE_PRIORITIES           ( 2 )

/* Software timer definitions. */
#define configUSE_TIMERS                          1
#define configTIMER_TASK_PRIORITY                 ( 2 )
#define configTIMER_QUEUE_LENGTH                  32
#define configTIMER_TASK_STACK_DEPTH              ( 80 )

/* Tickless Idle configuration. */
#define configEXPECTED_IDLE_TIME_BEFORE_SLEEP     2

/*-----------------------------------------------------------
 * Assert / debug
 *----------------------------------------------------------*/

/* Simple assert: halt with interrupts disabled if condition fails. */
#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/* FreeRTOS MPU specific definitions. */
#define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS    1

/*-----------------------------------------------------------
 * API inclusion options. Most linkers will strip unused ones anyway.
 *----------------------------------------------------------*/
#define INCLUDE_vTaskPrioritySet                  1
#define INCLUDE_uxTaskPriorityGet                 1
#define INCLUDE_vTaskDelete                       1
#define INCLUDE_vTaskSuspend                      1
#define INCLUDE_xResumeFromISR                    1
#define INCLUDE_vTaskDelayUntil                   1
#define INCLUDE_vTaskDelay                        1
#define INCLUDE_xTaskGetSchedulerState            1
#define INCLUDE_xTaskGetCurrentTaskHandle         1
#define INCLUDE_uxTaskGetStackHighWaterMark       1
#define INCLUDE_xTaskGetIdleTaskHandle            1
#define INCLUDE_xTimerGetTimerDaemonTaskHandle    1
#define INCLUDE_pcTaskGetTaskName                 1
#define INCLUDE_eTaskGetState                     1
#define INCLUDE_xEventGroupSetBitFromISR          1
#define INCLUDE_xTimerPendFunctionCall            1

/*-----------------------------------------------------------
 * Cortex-M interrupt priority configuration
 *
 * NOTE:
 *  - On Cortex-M, numerically lower = higher logical priority.
 *  - Only the top configPRIO_BITS bits are implemented in NVIC.
 *----------------------------------------------------------*/

/* nRF52840 implements 3 priority bits (0..7). */
#define configPRIO_BITS                           3

/* Library priority numbers (before shifting). */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   7
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

/* Actual NVIC priority values (shifted into top bits). */
#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
   See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

/*-----------------------------------------------------------
 * Map FreeRTOS port interrupt handlers to vector table names.
 *----------------------------------------------------------*/

#define vPortSVCHandler           SVC_Handler
#define xPortPendSVHandler        PendSV_Handler

/* Tick handler mapping depends on the tick source. */
#if ( configTICK_SOURCE == FREERTOS_USE_SYSTICK )

    /* SysTick will be configured automatically to CPU clock source. */
    #define xPortSysTickHandler   SysTick_Handler

#elif ( configTICK_SOURCE == FREERTOS_USE_RTC )

    #define configSYSTICK_CLOCK_HZ  ( 32768UL )
    #define xPortSysTickHandler     RTC1_IRQHandler

#else
    #error  Unsupported configTICK_SOURCE value
#endif

/*-----------------------------------------------------------
 * Settings that are generated automatically based on the above.
 *----------------------------------------------------------*/

/* Code below should be only used by the compiler, and not the assembler. */
#if !( defined( __ASSEMBLY__ ) || defined( __ASSEMBLER__ ) )
    /* Nothing Nordic-specific here anymore. You can add CMSIS includes later
       if you bring in nrf.h / SystemCoreClock, etc. */
#endif /* !assembler */

/* Tick auto-correction debug flag (keep 0 for now). */
#define configUSE_DISABLE_TICK_AUTO_CORRECTION_DEBUG  0

#endif /* FREERTOS_CONFIG_H */
