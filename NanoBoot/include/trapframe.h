#ifndef NB_TRAPFRAME_H
#define NB_TRAPFRAME_H

#include "common.h"

typedef struct trapframe {
    uint64_t regs[32];
    uint64_t sepc;
    uint64_t sstatus;
    uint64_t scause;
    uint64_t kernel_sp;
} trapframe_t;

#define TF_RA       (8ULL * 1)
#define TF_SP       (8ULL * 2)
#define TF_A0       (8ULL * 10)
#define TF_A1       (8ULL * 11)
#define TF_A2       (8ULL * 12)
#define TF_A3       (8ULL * 13)
#define TF_A4       (8ULL * 14)
#define TF_A5       (8ULL * 15)
#define TF_A7       (8ULL * 17)
#define TF_SEPC     (8ULL * 32)
#define TF_SSTATUS  (8ULL * 33)
#define TF_SCAUSE   (8ULL * 34)
#define TF_KERNEL_SP (8ULL * 35)

#endif
