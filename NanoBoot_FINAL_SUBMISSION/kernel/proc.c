#include "common.h"
#include "param.h"
#include "riscv.h"
#include "string.h"
#include "uart.h"
#include "proc.h"
#include "vm.h"
#include "timer.h"

proc_t proc_table[NPROC];
proc_t *current_proc;
unsigned char user_mem_base[NPROC][USER_TOTAL_PAGES][PGSIZE] __attribute__((aligned(PGSIZE)));
static __attribute__((aligned(PGSIZE))) trapframe_t trapframes[NPROC];
static int next_pid = PID_FIRST;
static int last_slot = -1;
extern unsigned char _binary_user_sh_bin_start[];
extern unsigned char _binary_user_sh_bin_end[];
extern void idle_loop(void);

static trapframe_t idle_tf __attribute__((aligned(16)));
static __attribute__((aligned(PGSIZE))) uint8_t kernel_stacks[NPROC][PGSIZE];
static __attribute__((aligned(PGSIZE))) uint8_t idle_stack[PGSIZE];

static void tf_clear(trapframe_t *tf)
{
    memset(tf, 0, sizeof(*tf));
}

static void prepare_user_tf(trapframe_t *tf, int slot)
{
    tf_clear(tf);
    tf->sepc = USER_BASE;
    tf->sstatus = SSTATUS_SPIE; /* sret enters U-mode and re-enables S interrupts */
    tf->regs[2] = USER_STACK_TOP - 16;
    /* user_trap_entry needs a valid kernel stack before entering C. */
    tf->kernel_sp = (uint64_t)&kernel_stacks[slot][PGSIZE];
}

static void prepare_idle_tf(void)
{
    tf_clear(&idle_tf);
    idle_tf.sepc = (uint64_t)idle_loop;
    idle_tf.sstatus = SSTATUS_SPIE | SSTATUS_SPP;
    idle_tf.regs[2] = (uint64_t)&idle_stack[PGSIZE];
    idle_tf.kernel_sp = (uint64_t)&idle_stack[PGSIZE];
}

void proc_init(void)
{
    memset(proc_table, 0, sizeof(proc_table));
    current_proc = NULL;
    next_pid = PID_FIRST;
    last_slot = -1;
    for (int i = 0; i < NPROC; ++i) {
        proc_table[i].pid = 0;
        proc_table[i].state = P_UNUSED;
        proc_table[i].tf = &trapframes[i];
        tf_clear(&trapframes[i]);
        trapframes[i].kernel_sp = (uint64_t)&kernel_stacks[i][PGSIZE];
    }
    prepare_idle_tf();
}

proc_t *proc_create_init(const unsigned char *image, size_t image_size)
{
    for (int i = 0; i < NPROC; ++i) {
        if (proc_table[i].state != P_UNUSED) continue;
        proc_t *p = &proc_table[i];
        p->pid = next_pid++;
        p->state = P_RUNNABLE;
        p->parent = NULL;
        p->exit_status = 0;
        p->wake_tick = 0;
        p->pagetable = vm_create_user(image, image_size, i);
        if (!p->pagetable) {
            p->state = P_UNUSED;
            return NULL;
        }
        p->tf = &trapframes[i];
        prepare_user_tf(p->tf, i);
        return p;
    }
    return NULL;
}

proc_t *proc_by_pid(int pid)
{
    for (int i = 0; i < NPROC; ++i)
        if (proc_table[i].state != P_UNUSED && proc_table[i].pid == pid)
            return &proc_table[i];
    return NULL;
}

static int slot_of(proc_t *p)
{
    return (int)(p - proc_table);
}

int proc_fork(trapframe_t *parent_tf)
{
    proc_t *parent = current_proc;
    if (!parent) return -1;

    for (int i = 0; i < NPROC; ++i) {
        if (proc_table[i].state != P_UNUSED) continue;
        proc_t *child = &proc_table[i];
        int pslot = slot_of(parent);
        child->pid = next_pid++;
        child->state = P_RUNNABLE;
        child->parent = parent;
        child->exit_status = 0;
        child->wake_tick = 0;
        /* Clone the parent address space into the child slot. */
        child->pagetable = vm_clone_user(i, pslot);
        if (!child->pagetable) {
            child->state = P_UNUSED;
            return -1;
        }
        child->tf = &trapframes[i];
        memcpy(child->tf, parent_tf, sizeof(*parent_tf));
        child->tf->kernel_sp = (uint64_t)&kernel_stacks[i][PGSIZE];
        child->tf->regs[10] = 0;
        return child->pid;
    }
    return -1;
}

void proc_exit(int status)
{
    proc_t *p = current_proc;
    if (!p) return;
    p->exit_status = status;

    /* Complete a blocked parent wait immediately. */
    if (p->parent && p->parent->state == P_SLEEPING && p->parent->wake_tick == 0) {
        proc_t *parent = p->parent;
        parent->tf->regs[10] = (uint64_t)p->pid;
        parent->state = P_RUNNABLE;
        vm_switch(kernel_pagetable());
        vm_destroy(p->pagetable);
        p->pagetable = NULL;
        p->state = P_UNUSED;
        p->pid = 0;
        p->parent = NULL;
    } else if (p->parent == NULL) {
        vm_switch(kernel_pagetable());
        vm_destroy(p->pagetable);
        p->pagetable = NULL;
        p->state = P_UNUSED;
        p->pid = 0;
    } else {
        p->state = P_ZOMBIE;
    }

    current_proc = proc_schedule();
}

int proc_wait(int *status)
{
    proc_t *p = current_proc;
    if (!p) return -1;

    for (int i = 0; i < NPROC; ++i) {
        proc_t *c = &proc_table[i];
        if (c->state == P_UNUSED || c->parent != p) continue;
        if (c->state == P_ZOMBIE) {
            int pid = c->pid;
            if (status) *status = c->exit_status;
            vm_destroy(c->pagetable);
            c->pagetable = NULL;
            c->state = P_UNUSED;
            c->pid = 0;
            c->parent = NULL;
            return pid;
        }
    }

    int have_child = 0;
    for (int i = 0; i < NPROC; ++i)
        if (proc_table[i].state != P_UNUSED && proc_table[i].parent == p)
            have_child = 1;
    if (!have_child) return -1;

    /* Sleep until proc_exit() records a child pid in our trapframe. */
    p->state = P_SLEEPING;
    current_proc = proc_schedule();
    return 0;
}

void proc_sleep(uint64_t ticks)
{
    if (!current_proc) return;
    if (ticks == 0) return;
    current_proc->wake_tick = timer_now() + ticks;
    current_proc->state = P_SLEEPING;
    current_proc = proc_schedule();
}

void proc_wakeup(uint64_t now)
{
    for (int i = 0; i < NPROC; ++i) {
        proc_t *p = &proc_table[i];
        if (p->state == P_SLEEPING && p->wake_tick != 0 && p->wake_tick <= now) {
            p->wake_tick = 0;
            p->state = P_RUNNABLE;
        }
    }
}

void proc_tick(void)
{
    uint64_t now = timer_now();
    proc_wakeup(now);
}

proc_t *proc_schedule(void)
{
    int start = last_slot;
    if (current_proc) start = slot_of(current_proc);

    for (int n = 1; n <= NPROC; ++n) {
        int i = (start + n) % NPROC;
        if (proc_table[i].state == P_RUNNABLE) {
            proc_t *next = &proc_table[i];
            last_slot = i;
            next->state = P_RUNNING;
            return next;
        }
    }
    return NULL;
}

const char *proc_state_name(procstate_t s)
{
    switch (s) {
    case P_UNUSED: return "UNUSED";
    case P_RUNNABLE: return "RUNNABLE";
    case P_RUNNING: return "RUNNING";
    case P_SLEEPING: return "SLEEPING";
    case P_ZOMBIE: return "ZOMBIE";
    default: return "?";
    }
}

void proc_ps(void)
{
    uart_puts("PID\tSTATE\n");
    for (int i = 0; i < NPROC; ++i) {
        if (proc_table[i].state == P_UNUSED) continue;
        uart_print_int(proc_table[i].pid);
        uart_puts("\t");
        uart_puts(proc_state_name(proc_table[i].state));
        uart_putc('\n');
    }
}

trapframe_t *idle_trapframe_ptr(void)
{
    return &idle_tf;
}
