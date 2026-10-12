#ifndef NB_UART_H
#define NB_UART_H

void uart_init(void);
void uart_putc(int c);
void uart_puts(const char *s);
int uart_getc(void);
void uart_print_u64(unsigned long long x);
void uart_print_int(long long x);

#endif
