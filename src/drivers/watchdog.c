#include "watchdog.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "uart_driver.h"

#define WDT_TIMEOUT_MS 2000

static TimerHandle_t wdt_timer = NULL;

static void wdt_expire_cb(TimerHandle_t xTimer) {
    (void)xTimer;
    /* In production this triggers NVIC_SystemReset() */
    /* In simulation: log the fault and halt */
    uart_puts("\r\n[WDT] WATCHDOG EXPIRED - system reset!\r\n");
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

void watchdog_init(void) {
    wdt_timer = xTimerCreate (
        "WDT",
        pdMS_TO_TICKS (WDT_TIMEOUT_MS),
        pdFALSE,        /* One-shot - must be reset each time */
        NULL,
        wdt_expire_cb
    );
    configASSERT(wdt_timer != NULL);
    xTimerStart(wdt_timer, 0);
}

void watchdog_kick(void) {
    xTimerReset (wdt_timer, 0);     /* Restart the 2-second countdown */
}
