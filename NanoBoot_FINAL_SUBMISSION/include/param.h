#ifndef NB_PARAM_H
#define NB_PARAM_H

#define NPROC              8
#define PID_FIRST          1
#define PGSIZE             4096ULL
#define PT_POOL_PAGES      1024
#define USER_IMAGE_PAGES   4
#define USER_TOTAL_PAGES   5
#define USER_BASE          0x0000000040000000ULL
#define USER_STACK_TOP     0x0000000080000000ULL
#define UART0              0x0000000010000000ULL
#define UART0_RHR          0x00
#define UART0_THR          0x00
#define UART0_IER          0x01
#define UART0_FCR          0x02
#define UART0_LCR          0x03
#define UART0_MCR          0x04
#define UART0_LSR          0x05
#define UART_LSR_RX_READY  0x01
#define UART_LSR_TX_IDLE   0x20
#define TIMER_TICKS        100000ULL
#define MAX_FILE_NAME      32
#define MAX_FILE_DATA      512

#endif
