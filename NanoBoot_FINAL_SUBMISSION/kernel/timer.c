#include "common.h"
#include "param.h"
#include "riscv.h"
#include "uart.h"
#include "timer.h"
#include "proc.h"

static uint64_t ticks;

void timer_init(void)
{
    ticks = 0;
    sbi_time_set(r_time() + TIMER_TICKS);
    asm volatile("csrs sie, %0" :: "r"(SIE_STIE));
}

void timer_tick(void)
{
    ++ticks;
    proc_tick();
    sbi_time_set(r_time() + TIMER_TICKS);
}

unsigned long long timer_now(void)
{
    return ticks;
}
