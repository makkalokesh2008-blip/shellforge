#ifndef NB_SYSCALL_H
#define NB_SYSCALL_H

#include "trapframe.h"

void syscall_dispatch(trapframe_t *tf);

#endif
