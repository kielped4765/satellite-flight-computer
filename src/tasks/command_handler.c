#include "FreeRTOS.h"
#include "task.h"
#include "flight_types.h"
#include "uart_driver.h"
#include "semphr.h"

extern ADCSState_t          g_adcs_state;
extern SemaphoreHandle_t    g_adcs_mutex;

#define CMD_NOMINAL     'N'
#define CMD_SAFE        'S'
#define CMD_DETUMBLE    'D'
#define CMD_REBOOT      'R'
#define CMD_STATUS      'T'

void command_handler_task(void*p) {
    (void)p;
    uint8_t byte;
    uart_puts("[CMD] Ready. Listening for uplink commands...\r\n");

    for (;;) {
        if (uart_getc(&byte, 1000) == 0) continue; /* 1s timeout, retry */

        uart_printf("[CMD] Rx: 0x%02X ('%c')\r\n", byte, byte);

        switch ((char)byte) {
            case CMD_NOMINAL:
                if (xSemaphoreTake(g_adcs_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    g_adcs_state.mode = MODE_NOMINAL;
                    xSemaphoreGive(g_adcs_mutex);
                    uart_puts("[CMD] Mode -> SAFE\r\n");
                }
                break;
            case CMD_SAFE:
                if (xSemaphoreTake(g_adcs_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    g_adcs_state.mode = MODE_SAFE;
                    xSemaphoreGive(g_adcs_mutex);
                    uart_puts("[CMD] Mode -> SAFE\r\n");
                }
                break;
            case CMD_DETUMBLE:
                if (xSemaphoreTake(g_adcs_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    g_adcs_state.mode = MODE_DETUMBLE;
                    xSemaphoreGive(g_adcs_mutex);
                    uart_puts("[CMD] Mode -> DETUMBLE\r\n");
                }
                break;
            case CMD_STATUS:
                uart_printf("[CMD] mode =%u tick %u faults=0x%02X\r\n",
                            g_adcs_state.mode,
                        (unsigned)xTaskGetTickCount(),
                        0);
                break;
            case CMD_REBOOT:
                uart_puts("[CMD] Rebooting...\r\n");
                vTaskSuspendAll();
                for (;;) {}
            default:
                uart_printf("[CMD] Unknown: 0x%02X\r\n", byte);
        }
    }
}