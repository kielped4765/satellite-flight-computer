#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>
#include <stddef.h>

void uart_init(uint32_t baud_rate);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_printf(const char *fmt, ...); /* Thread-safe formatted print */
void uart_write_bytes(const uint8_t *data, size_t len); /* Binary TX */
int uart_getc(uint8_t *out, uint32_t timeout_ms); /* Returns 1 if byte received */

#endif