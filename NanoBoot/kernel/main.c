#include "common.h"
#include "riscv.h"
#include "uart.h"
#include "vm.h"
#include "proc.h"
#include "timer.h"
#include "trap.h"

extern void user_trap_entry(void);
extern void kernel_trap_entry(void);
extern void enter_user(trapframe_t *tf, uint64_t satp);
extern void panic(const char *msg);
extern unsigned char _binary_user_sh_bin_start[];
extern unsigned char _binary_user_sh_bin_end[];

void panic(const char *msg)
{
    uart_puts("\nPANIC: ");
    uart_puts(msg);
    uart_puts("\n");
    for (;;) asm volatile("wfi");
}

static void print_banner(void)
{
    uart_puts("\n========================================\n");
    uart_puts(" NanoBoot - RISC-V Boot-to-Shell Kernel\n");
    uart_puts("========================================\n");
    uart_puts("boot: supervisor mode / QEMU virt\n");
    uart_puts("vm  : Sv39 per-process page tables\n");
    uart_puts("trap: stvec + ecall + timer interrupt\n");
    uart_puts("sched: fixed table / round-robin\n\n");
}

void main(void)
{
    uart_init();
    print_banner();

    vm_init();
    proc_init();

    w_stvec((uint64_t)kernel_trap_entry);
    asm volatile("csrs sie, %0" :: "r"(SIE_STIE));
    timer_init();

    size_t image_size = (size_t)(_binary_user_sh_bin_end - _binary_user_sh_bin_start);
    proc_t *init = proc_create_init(_binary_user_sh_bin_start, image_size);
    if (!init) panic("cannot create init process");

    current_proc = proc_schedule();
    if (!current_proc) panic("scheduler found no initial process");

    uart_puts("init: pid=");
    uart_print_int(current_proc->pid);
    uart_puts(" ready\n");

    /* Enter user mode. The remaining execution is driven by traps/scheduler. */
    enter_user(current_proc->tf, vm_satp(current_proc->pagetable));
    panic("returned from enter_user");
}
