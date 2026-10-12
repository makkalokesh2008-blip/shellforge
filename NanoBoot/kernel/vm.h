#ifndef NB_VM_H
#define NB_VM_H

#include "common.h"
#include "param.h"
#include "trapframe.h"

typedef uint64_t pte_t;
typedef pte_t *pagetable_t;

#define PTE_V (1ULL << 0)
#define PTE_R (1ULL << 1)
#define PTE_W (1ULL << 2)
#define PTE_X (1ULL << 3)
#define PTE_U (1ULL << 4)
#define PTE_A (1ULL << 6)
#define PTE_D (1ULL << 7)

void vm_init(void);
pagetable_t vm_create_user(const unsigned char *image, size_t image_size, int slot);
pagetable_t vm_clone_user(int dst_slot, int src_slot);
void vm_switch(pagetable_t pt);
pagetable_t kernel_pagetable(void);
void vm_destroy(pagetable_t pt);
uint64_t vm_satp(pagetable_t pt);
int copyin(void *dst, pagetable_t pt, uint64_t srcva, size_t len);
int copyout(pagetable_t pt, uint64_t dstva, const void *src, size_t len);
int copyinstr(char *dst, pagetable_t pt, uint64_t srcva, size_t max);

#endif
