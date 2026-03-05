#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ---- Core scheduler ---- */
#define configUSE_PREEMPTION            1
#define configUSE_TIME_SLICING          1
#define configUSE_IDLE_HOOK             1
#define configUSE_TICK_HOOK             0
#define configCPU_CLOCK_HZ              ((unsigned long) 12000000)
#define configTICK_RATE_HZ              ((TickType_t) 1000)
#define configMAX_PRIORITIES            8
#define configMINIMAL_STACK_SIZE        ((uint16_t) 128)
#define configTOTAL_HEAP_SIZE           ((size_t)(40 * 1024))
#define configMAX_TASK_NAME_LEN         16
#define configUSE_16_BIT_TICKS          0
#define configIDLE_SHOULD_YIELD         1

/* ---- Synchronisation primitives ---- */
#define configUSE_MUTEXES               1
#define configUSE_RECURSIVE_MUTEXES     1
#define configUSE_COUNTING_SEMAPHORES   1
#define configUSE_QUEUE_SETS            0

/* ---- Software timers ---- */
#define configUSE_TIMERS                1
#define configTIMER_TASK_PRIORITY       (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH        10
#define configTIMER_TASK_STACK_DEPTH    256

/* ---- Debug and safety ---- */
#define configUSE_TRACE_FACILITY        1
#define configUSE_STATS_FORMATTING_FUNCTIONS 1
#define configCHECK_FOR_STACK_OVERFLOW  2
#define configUSE_MALLOC_FAILED_HOOK    1
#define configASSERT(x) if((x) == 0) { taskDISABLE_INTERRUPTS(); for(;;); }

/* ---- API includes ---- */
#define INCLUDE_vTaskDelay              1
#define INCLUDE_vTaskDelayUntil         1
#define INCLUDE_vTaskDelete             1
#define INCLUDE_vTaskSuspend            1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_eTaskGetState           1

/* ---- Cortex-M3 interrupt priorities ---- */
#define configKERNEL_INTERRUPT_PRIORITY         ( 7 << 5 )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( 5 << 5 )

/* ---- FreeRTOS ARM_CM3 handler name mapping ---- */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */