#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "uart_driver.h"
#include "flight_types.h"
#include "watchdog.h"

#define PRIORITY_TELEMETRY 2
#define PRIORITY_COMMAND   3
#define PRIORITY_ADCS      4
#define PRIORITY_HEALTH    5

#define STACK_ADCS       512
#define STACK_TELEMETRY  256
#define STACK_HEALTH     256
#define STACK_COMMAND    256

QueueHandle_t g_telemetry_queue  = NULL;
QueueHandle_t g_command_queue    = NULL;
TaskHandle_t  g_adcs_task_handle = NULL;

void adcs_task(void *p);
void telemetry_task(void *p);
void health_monitor_task(void *p);
void command_handler_task(void *p);

int main(void) {
    uart_init(115200);
    uart_puts("\r\n=== Satellite Flight Computer Booting ===\r\n");

    g_telemetry_queue = xQueueCreate(16, sizeof(TelemetryPacket_t));
    g_command_queue   = xQueueCreate(8,  sizeof(uint8_t));
    configASSERT(g_telemetry_queue != NULL);
    configASSERT(g_command_queue   != NULL);

    configASSERT(xTaskCreate(adcs_task,           "ADCS", STACK_ADCS,      NULL, PRIORITY_ADCS,      &g_adcs_task_handle) == pdPASS);
    configASSERT(xTaskCreate(telemetry_task,       "TLM",  STACK_TELEMETRY, NULL, PRIORITY_TELEMETRY, NULL) == pdPASS);
    configASSERT(xTaskCreate(health_monitor_task,  "HLT",  STACK_HEALTH,    NULL, PRIORITY_HEALTH,    NULL) == pdPASS);
    configASSERT(xTaskCreate(command_handler_task, "CMD",  STACK_COMMAND,   NULL, PRIORITY_COMMAND,   NULL) == pdPASS);

    watchdog_init();

    uart_puts("Tasks created. Starting scheduler...\r\n");
    vTaskStartScheduler();

    uart_puts("FATAL: Scheduler failed!\r\n");
    for (;;) {}
}

void vApplicationIdleHook(void) {}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    uart_printf("FATAL: Stack overflow in task [%s]\r\n", pcTaskName);
    for (;;) {}
}

void vApplicationMallocFailedHook(void) {
    uart_puts("FATAL: Heap allocation failed. Increase configTOTAL_HEAP_SIZE.\r\n");
    for (;;) {}
}
