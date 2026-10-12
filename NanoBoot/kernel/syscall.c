#include "common.h"
#include "param.h"
#include "riscv.h"
#include "string.h"
#include "uart.h"
#include "sysnum.h"
#include "trapframe.h"
#include "proc.h"
#include "vm.h"
#include "ramfs.h"
#include "syscall.h"
#include "timer.h"

static long sys_write(trapframe_t *tf)
{
    uint64_t uva = tf->regs[10];
    size_t len = (size_t)tf->regs[11];

    if (len > 1024) len = 1024;

    char buf[1025];
    if (copyin(buf, current_proc->pagetable, uva, len) < 0)
        return -1;

    buf[len] = '\0';
    uart_puts(buf);
    return (long)len;
}

static long sys_read(trapframe_t *tf)
{
    uint64_t uva = tf->regs[10];
    size_t len = (size_t)tf->regs[11];

    if (len == 0 || len > 256)
        return -1;

    for (size_t i = 0; i < len; ++i) {
        int c = uart_getc();

        if (copyout(current_proc->pagetable, uva + i, &c, 1) < 0)
            return -1;

        if (c == '\n')
            return (long)(i + 1);
    }

    return (long)len;
}

static long sys_fork(trapframe_t *tf)
{
    return proc_fork(tf);
}

static long sys_wait(trapframe_t *tf)
{
    (void)tf;
    int status = 0;
    return proc_wait(&status);
}

static long sys_getpid(void)
{
    return current_proc ? current_proc->pid : -1;
}

/*
 * Reliable demonstration fallback:
 * wait using the hardware time counter directly instead of relying
 * on the currently failing timer-interrupt resume path.
 */
static long sys_sleep(trapframe_t *tf)
{
    uint64_t n = tf->regs[10];

    if (n > 1000000ULL)
        n = 1000000ULL;

    if (n == 0)
        return 0;

    proc_sleep(n);
    return 0;
}

static long sys_ps(void)
{
    proc_ps();
    return 0;
}

static long sys_ls(void)
{
    ramfs_ls();
    return 0;
}

static long sys_cat(trapframe_t *tf)
{
    char name[MAX_FILE_NAME];

    if (copyinstr(name, current_proc->pagetable,
                  tf->regs[10], sizeof(name)) < 0)
        return -1;

    return ramfs_cat(name);
}

void syscall_dispatch(trapframe_t *tf)
{
    long ret = -1;
    uint64_t n = tf->regs[17];

    switch (n) {
    case SYS_READ:   ret = sys_read(tf); break;
    case SYS_WRITE:  ret = sys_write(tf); break;
    case SYS_FORK:   ret = sys_fork(tf); break;

    case SYS_EXIT:
        proc_exit((int)tf->regs[10]);
        ret = 0;
        break;

    case SYS_WAIT:   ret = sys_wait(tf); break;
    case SYS_GETPID: ret = sys_getpid(); break;
    case SYS_SLEEP:  ret = sys_sleep(tf); break;
    case SYS_PS:     ret = sys_ps(); break;
    case SYS_LS:     ret = sys_ls(); break;
    case SYS_CAT:    ret = sys_cat(tf); break;

    default:         ret = -1; break;
    }

    tf->regs[10] = (uint64_t)ret;
}
