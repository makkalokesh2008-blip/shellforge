#ifndef NB_SPINLOCK_H
#define NB_SPINLOCK_H

#include "common.h"

typedef struct spinlock {
    volatile uint32_t locked;
    const char *name;
} spinlock_t;

void spin_init(spinlock_t *lk, const char *name);
void spin_lock(spinlock_t *lk);
void spin_unlock(spinlock_t *lk);

#endif
