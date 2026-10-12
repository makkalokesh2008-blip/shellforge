#ifndef NB_RISCV_H
#define NB_RISCV_H

#include "common.h"

#define SSTATUS_SIE   (1ULL << 1)
#define SSTATUS_SPIE  (1ULL << 5)
#define SSTATUS_SPP   (1ULL << 8)

#define SIE_STIE      (1ULL << 5)

#define SCAUSE_INTERRUPT (1ULL << 63)
#define SCAUSE_STIMER    (SCAUSE_INTERRUPT | 5ULL)
#define SCAUSE_ECALL_U   8ULL

#define SATP_MODE_SV39  (8ULL << 60)

static inline void w_stvec(uint64_t x) { asm volatile("csrw stvec, %0" :: "r"(x)); }
static inline void w_satp(uint64_t x)  { asm volatile("csrw satp, %0" :: "r"(x)); }
static inline uint64_t r_satp(void)    { uint64_t x; asm volatile("csrr %0, satp" : "=r"(x)); return x; }
static inline void sfence_vma(void)    { asm volatile("sfence.vma zero, zero" ::: "memory"); }
static inline void enable_sie(void)    { asm volatile("csrs sstatus, %0" :: "r"(SSTATUS_SIE)); }
static inline void disable_sie(void)   { asm volatile("csrc sstatus, %0" :: "r"(SSTATUS_SIE)); }
static inline uint64_t r_time(void)    { uint64_t x; asm volatile("rdtime %0" : "=r"(x)); return x; }
static inline uint64_t r_sstatus(void) { uint64_t x; asm volatile("csrr %0, sstatus" : "=r"(x)); return x; }

static inline uint64_t sbi_time_set(uint64_t when)
{
    register uint64_t a0 asm("a0") = when;
    register uint64_t a6 asm("a6") = 0;
    register uint64_t a7 asm("a7") = 0x54494D45ULL; /* SBI_EXT_TIME */
    asm volatile("ecall" : "+r"(a0) : "r"(a6), "r"(a7) : "memory");
    return a0;
}

#endif
