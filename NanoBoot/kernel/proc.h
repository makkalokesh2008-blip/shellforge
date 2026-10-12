#ifndef NB_PROC_H
#define NB_PROC_H

#include "common.h"
#include "param.h"
#include "spinlock.h"
#include "trapframe.h"
#include "vm.h"

typedef enum procstate {
    P_UNUSED = 0,
    P_RUNNABLE,
    P_RUNNING,
    P_SLEEPING,
    P_ZOMBIE
} procstate_t;

typedef struct context {
    uint64_t ra;
    uint64_t sp;
    uint64_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
} context_t;

typedef struct proc {
    int pid;
    procstate_t state;
    int exit_status;
    struct proc *parent;
    pagetable_t pagetable;
    trapframe_t *tf;
    context_t context;
    uint64_t wake_tick;
} proc_t;

extern proc_t proc_table[NPROC];
extern proc_t *current_proc;
extern unsigned char user_mem_base[NPROC][USER_TOTAL_PAGES][PGSIZE];

void proc_init(void);
proc_t *proc_create_init(const unsigned char *image, size_t image_size);
int proc_fork(trapframe_t *parent_tf);
void proc_exit(int status);
int proc_wait(int *status);
void proc_sleep(uint64_t ticks);
void proc_wakeup(uint64_t now);
void proc_tick(void);
proc_t *proc_schedule(void);
proc_t *proc_by_pid(int pid);
void proc_ps(void);

const char *proc_state_name(procstate_t s);

#endif
