#include "common.h"
#include "riscv.h"
#include "uart.h"
#include "proc.h"
#include "vm.h"
#include "syscall.h"
#include "timer.h"
#include "trap.h"

extern void user_trap_entry(void);
extern void kernel_trap_entry(void);
extern pagetable_t kernel_pagetable(void);
extern void panic(const char *msg);
extern trapframe_t *idle_trapframe_ptr(void);

static void switch_to(proc_t *p)
{
    if (!p) {
        vm_switch(kernel_pagetable());
        w_stvec((uint64_t)kernel_trap_entry);
        return;
    }
    p->state = P_RUNNING;
    vm_switch(p->pagetable);
    w_stvec((uint64_t)user_trap_entry);
}

static trapframe_t *idle_target(void)
{
    switch_to(NULL);
    return idle_trapframe_ptr();
}

trapframe_t *trap_handler(trapframe_t *tf)
{
    uint64_t scause = tf->scause;
    if (!current_proc) return idle_target();

    current_proc->tf = tf;

    if (scause == SCAUSE_ECALL_U) {
        tf->sepc += 4;
        syscall_dispatch(tf);
    } else if (scause == SCAUSE_STIMER) {
        timer_tick();
        if (current_proc && current_proc->state == P_RUNNING) {
            current_proc->state = P_RUNNABLE;
            proc_t *next = proc_schedule();
            if (next && next != current_proc) {
                current_proc = next;
            } else if (!next) {
                current_proc = NULL;
            }
        }
    } else {
        uart_puts("\nNanoBoot: unexpected user trap scause=");
        uart_print_u64(scause);
        uart_puts(" sepc=");
        uart_print_u64(tf->sepc);
        uart_puts("\n");
        proc_exit(-1);
    }

    while (!current_proc) {
        uint64_t deadline = r_time() + TIMER_TICKS;
        while (r_time() < deadline) {
            asm volatile("nop");
        }
        timer_tick();
        current_proc = proc_schedule();
    }

    if (current_proc->state != P_RUNNING) {
        proc_t *next = proc_schedule();
        if (next) current_proc = next;
    }

    switch_to(current_proc);
    return current_proc->tf;
}

trapframe_t *kernel_trap_handler(uint64_t scause)
{
    if (scause != SCAUSE_STIMER) {
        uart_puts("\nNanoBoot: kernel trap scause=");
        uart_print_u64(scause);
        uart_puts("\n");
        for (;;) asm volatile("wfi");
    }

    timer_tick();
    if (!current_proc || current_proc->state != P_RUNNING) {
        proc_t *next = proc_schedule();
        current_proc = next;
    }

    if (!current_proc) {
        switch_to(NULL);
        return idle_trapframe_ptr();
    }

    switch_to(current_proc);
    return current_proc->tf;
}
