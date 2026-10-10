.section .text

.macro SYSCALL name, num
.globl \name
\name:
    li a7, \num
    ecall
    ret
.endm

SYSCALL sys_read,   0
SYSCALL sys_write,  1
SYSCALL sys_fork,   2
SYSCALL sys_exit,   3
SYSCALL sys_wait,   4
SYSCALL sys_getpid, 5
SYSCALL sys_sleep,  6
SYSCALL sys_ps,     7
SYSCALL sys_ls,     8
SYSCALL sys_cat,    9
