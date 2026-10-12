#ifndef NB_TRAP_H
#define NB_TRAP_H

#include "trapframe.h"

trapframe_t *trap_handler(trapframe_t *tf);
trapframe_t *kernel_trap_handler(uint64_t scause);

#endif
