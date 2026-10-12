#include "spinlock.h"

void spin_init(spinlock_t *lk, const char *name)
{
    lk->locked = 0;
    lk->name = name;
}

void spin_lock(spinlock_t *lk)
{
    uint32_t expected;
    do {
        expected = 0;
        asm volatile("amoswap.w.aq %0, %2, %1"
                     : "=&r"(expected), "+A"(lk->locked)
                     : "r"(1)
                     : "memory");
    } while (expected != 0);
}

void spin_unlock(spinlock_t *lk)
{
    asm volatile("amoswap.w.rl zero, zero, %0" : "+A"(lk->locked) :: "memory");
}
