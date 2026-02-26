#include "FreeRTOS.h"
#include "task.h"
#include "flight_types.h"
#include "watchdog.h"
#include "uart_driver.h"

extern volatile uint32_t g_adcs_heartbeat;
extern TaskHandle_t      g_adcs_task_handle;

volatile uint8_t g_fault_flags = FAULT_NONE;

void health_monitor_task(void *p) {
    (void)p;
    TickType_t xLastWake = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(100); /* 10 Hz */
    uint32_t last_hb = 0;
 
    for (;;) {
        vTaskDelayUntil(&xLastWake, xPeriod);
        
        /* --- Check ADCS heartbeat --- */
        uint32_t current_hb = g_adcs_heartbeat;
        if (current_hb == last_hb) {
            g_fault_flags |= FAULT_ADCS_TIMEOUT;
            uart_puts("[HM] FAULT: ADCS heartbeat missed!\r\n");
        } else {
            g_fault_flags &= ~FAULT_ADCS_TIMEOUT;
        }
        last_hb = current_hb;
    
        /* --- Check ADCS stack high-water mark --- */
        if (g_adcs_task_handle != NULL) {
            UBaseType_t watermark = uxTaskGetStackHighWaterMark(g_adcs_task_handle);
            if (watermark < 32) { /* < 128 bytes remaining */
                g_fault_flags |= FAULT_STACK_LOW;
                uart_printf("[HM] WARNING: ADCS stack low: %u words left\r\n",
                            (unsigned)watermark);
            } else {
                g_fault_flags &= ~FAULT_STACK_LOW;
            }
        }
    
        /* --- Kick watchdog (confirms health monitor is alive) --- */
        watchdog_kick();
    
        /* --- Periodic status log (every 10 cycles = 1 second) --- */
        static uint8_t log_divider = 0;
        if (++log_divider >= 10) {
            log_divider = 0;
            uart_printf("[HM] tick=%u faults=0x%02X adcs_hb=%u\r\n",
                        (unsigned)xTaskGetTickCount(),
                        g_fault_flags,
                        (unsigned)current_hb);
        }
    }
}