#include "uart_driver.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdarg.h>
#include <stdio.h>

/* LM386965 UART0 registers (QEMU emulates these) */
#define UART0_BASE 0X4000C000UL
#define UART0_DR   (*(volatile uint32_t *) (UART0_BASE + 0x000)) /* Data */
#define UART0_FR (*(volatile uint32_t *)(UART0_BASE + 0x018))   /* Flags */
#define UART0_IBRD (*(volatile uint32_t *)(UART0_BASE + 0x024)) /* Baud int */
#define UART0_FBRD (*(volatile uint32_t *)(UART0_BASE + 0x028)) /* Baud frac */
#define UART0_LCRH (*(volatile uint32_t *)(UART0_BASE + 0x02C)) /* Line ctrl */
#define UART0_CTL (*(volatile uint32_t *)(UART0_BASE + 0x030)) /* Control */

#define UART_FR_TXFF (1u << 5)  /* TX FIFO full */
#define UART_FR_RXFE (1u << 4) /* RX FIFO empty */
#define UART_CTL_UARTEN (1u << 0)
#define UART_CTL_TXE (1u << 8)
#define UART_CTL_RXE (1u << 9)
#define UART_LCRH_WLEN8 (0x3u << 5)
#define UART_LCRH_FEN (1u << 4)

static SemaphoreHandle_t uart_mutex = NULL;

void uart_init(uint32_t baud_rate) {
    UART0_CTL = 0;
    uint32_t clk = 16000000UL;
    UART0_IBRD = clk / (16 * baud_rate);
    UART0_FBRD = (uint32_t)(((clk % (16 * baud_rate)) * 64
                + (8 * baud_rate)) / (16 * baud_rate));
    UART0_LCRH = UART_LCRH_WLEN8 | UART_LCRH_FEN;
    UART0_CTL = UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE;
    uart_mutex = xSemaphoreCreateMutex();
}
void uart_putc(char c) {
    while (UART0_FR & UART_FR_TXFF) {}
    UART0_DR = (uint32_t)c;
}
void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}
void uart_printf(const char *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (uart_mutex) xSemaphoreTake(uart_mutex, portMAX_DELAY);
    uart_puts(buf);
    if (uart_mutex) xSemaphoreGive(uart_mutex);
}

void uart_write_bytes(const uint8_t *data, size_t len) {
    if (uart_mutex) xSemaphoreTake(uart_mutex, portMAX_DELAY);
    for (size_t i = 0; i < len; i++) {
        while (UART0_FR & UART_FR_TXFF) {}
        UART0_DR = data[i];
    }
    if (uart_mutex) xSemaphoreGive(uart_mutex);
}

int uart_getc(uint8_t *out, uint32_t timeout_ms) {
    uint32_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    while (xTaskGetTickCount() < deadline) {
        if (!(UART0_FR & UART_FR_RXFE)) {
            *out = (uint8_t)(UART0_DR & 0xFF);
            return 1;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return 0;
}