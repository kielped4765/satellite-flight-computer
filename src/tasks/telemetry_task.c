#include "FreeRTOS.h"
#include "task.h"
#include "flight_types.h"
#include "uart_driver.h"

extern ADCSState_t          g_adcs_state;
extern SemaphoreHandle_t    g_adcs_mutex;

static uint16_t xor_checksum(const uint8_t *data, size_t len) {
    uint16_t c = 0;
    for (size_t i = 0; i < len; i++) c ^= data[i];
    return c;
}

void telemetry_task(void *p)  {
    (void)p;
    static uint16_t seq = 0;
    TelemetryPacket_t pkt;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000)); /* 1 Hz downlink */

        if (xSemaphoreTake(g_adcs_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            pkt.magic = TLM_MAGIC;
            pkt.sequence = seq++;
            pkt.timestamp_ms = g_adcs_state.timestamp_ms;
            pkt.roll_deg = g_adcs_state.euler[0];
            pkt.pitch_deg = g_adcs_state.euler[1];
            pkt.yaw_deg = g_adcs_state.euler[2];
            pkt.omega_x = g_adcs_state.omega[0];
            pkt.omega_y = g_adcs_state.omega[1];
            pkt.omega_z = g_adcs_state.omega[2];
            pkt.mode = g_adcs_state.mode;
            pkt.fault_flags = 0;
            pkt.temperature = 2350; /* 23.5 C */
            pkt.checksum = 0;
            pkt.checksum = xor_checksum((uint8_t *)&pkt, sizeof(pkt) - 2);
            xSemaphoreGive(g_adcs_mutex);
        }
 
        uart_write_bytes((uint8_t *)&pkt, sizeof(pkt));
    }
    
}