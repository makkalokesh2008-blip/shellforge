#include "common.h"
#include "param.h"
#include "uart.h"

static volatile uint8_t *uart = (volatile uint8_t *)UART0;

static inline void uart_write_reg(int reg, uint8_t value) { uart[reg] = value; }
static inline uint8_t uart_read_reg(int reg) { return uart[reg]; }

void uart_init(void)
{
    uart_write_reg(UART0_IER, 0x00);
    uart_write_reg(UART0_LCR, 0x03); /* 8N1 */
    uart_write_reg(UART0_FCR, 0x01);
    uart_write_reg(UART0_MCR, 0x03);
}

void uart_putc(int c)
{
    if (c == '\n') uart_putc('\r');
    while ((uart_read_reg(UART0_LSR) & UART_LSR_TX_IDLE) == 0) { }
    uart_write_reg(UART0_THR, (uint8_t)c);
}

void uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
}

int uart_getc(void)
{
    for (;;) {
        if (uart_read_reg(UART0_LSR) & UART_LSR_RX_READY)
            return uart_read_reg(UART0_RHR);
    }
}

void uart_print_u64(unsigned long long x)
{
    char buf[32];
    int i = 0;
    if (x == 0) { uart_putc('0'); return; }
    while (x && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (x % 10));
        x /= 10;
    }
    while (i--) uart_putc(buf[i]);
}

void uart_print_int(long long x)
{
    if (x < 0) {
        uart_putc('-');
        x = -x;
    }
    uart_print_u64((unsigned long long)x);
}
