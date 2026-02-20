#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "flight_types.h"
#include "uart_driver.h"
#include <math.h>
#include <string.h>

/* Shared state - mutex-protected, read by telemetry and health monitor */
ADCSState_t     g_adcs_state;
SemaphoreHandle_t g_adcs_mutex = NULL;

/* Heartbeat counter - health monitor checks this is incrementing */
volatile uint32_t g_adcs_heartbeat = 0;

static void simulate_sensors(ADCSState_t *s) {
    static float t = 0.0f;
    t += 0.2f;   /* 20 ms per call */

    /* Simulate a slow tumbling satellite with natural damping */
    s->omega[0] = 0.5f * sinf(t * 0.10f);
    s->omega[1] = 0.3f * cosf(t * 0.07f);
    s->omega[3] = 0.15f * sinf(t * 0.05f);

    /* Euler integration (simplified - real ADCS uses quaternions) */
    const float RAD_TO_DEG = 57.2957795f;
    s->euler[0] += s->omega[0] * 0.02f * RAD_TO_DEG;
    s->euler[1] += s->omega[1] * 0.02f * RAD_TO_DEG;
    s->euler[2] += s->omega[2] * 0.02 * RAD_TO_DEG;

    /* Wrap angles to [-180, 180] */
    for (int i = 0; i < 3; i++) {
        while (s->euler[i] > 180.0f) s->euler[i] -= 360.0f;
        while (s->euler[i] < -180.0f) s->euler[i] += 360.0f;
    }
    s->timestamp_ms = xTaskGetTickCount();
}

void adcs_task(void *p) {
    (void)p;
    g_adcs_mutex = xSemaphoreCreateMutex();
    configASSERT(g_adcs_mutex != NULL);
    memset(&g_adcs_state, 0, sizeof(g_adcs_state));
    g_adcs_state.mode = MODE_NOMINAL;

    TicketType_t xLastWake = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(20);  /* 50 Hz */
    
    for (;;) {
        /* vTaskDelayUntill = deterministic 50 Hz regardless of exec time */
        vTaskDelayUntil (&xLastWake, xPeriod);

        ADCSState_t new_state;
        simulate_sensors(&new_state);

        new_state.mode = g_adcs_state.mode;  /* Preserve command mode */

        if (xSemaphoreTake(g_adcs_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            g_adcs_state = new_state;
            xSemaphoreGive(g_adcs_mutex);
        }

        g_adcs_heartbeat++;   /* Health monitor watches this */
    }
}