#include "common.h"
#include "param.h"
#include "riscv.h"
#include "string.h"
#include "vm.h"

extern char __kernel_end[];

static __attribute__((aligned(PGSIZE))) uint8_t pt_pool[PT_POOL_PAGES][PGSIZE];
static uint8_t pt_used[PT_POOL_PAGES];
static pagetable_t kernel_pt;

static inline uint64_t vpn(uint64_t va, int level)
{
    return (va >> (12 + 9 * level)) & 0x1FFULL;
}

static inline uint64_t pa2pte(uint64_t pa) { return ((pa >> 12) << 10); }
static inline uint64_t pte2pa(pte_t pte) { return (pte >> 10) << 12; }

static pte_t *pt_alloc(void)
{
    for (size_t i = 0; i < PT_POOL_PAGES; ++i) {
        if (pt_used[i]) continue;
        pt_used[i] = 1;
        pte_t *p = (pte_t *)pt_pool[i];
        memset(p, 0, PGSIZE);
        return p;
    }
    return NULL;
}

static void pt_release(pagetable_t p)
{
    if (!p) return;
    uintptr_t base = (uintptr_t)&pt_pool[0][0];
    uintptr_t addr = (uintptr_t)p;
    if (addr < base || addr >= base + (uintptr_t)PT_POOL_PAGES * PGSIZE) return;
    size_t idx = (addr - base) / PGSIZE;
    if (idx < PT_POOL_PAGES) pt_used[idx] = 0;
}

static pagetable_t pt_from_pte(pte_t pte) { return (pagetable_t)pte2pa(pte); }

static void pt_destroy_level(pagetable_t pt, int level)
{
    if (!pt) return;
    if (level > 0) {
        for (size_t i = 0; i < 512; ++i) {
            pte_t pte = pt[i];
            if (!(pte & PTE_V)) continue;
            if (pte & (PTE_R | PTE_W | PTE_X)) continue;
            pt_destroy_level(pt_from_pte(pte), level - 1);
        }
    }
    pt_release(pt);
}

static int mappages(pagetable_t pt, uint64_t va, uint64_t size, uint64_t pa, uint64_t perm)
{
    uint64_t end = va + size;
    for (uint64_t a = va; a < end; a += PGSIZE, pa += PGSIZE) {
        pte_t *p = pt;
        for (int level = 2; level > 0; --level) {
            uint64_t idx = vpn(a, level);
            pte_t *pte = &p[idx];
            if (!(*pte & PTE_V)) {
                pte_t *child = pt_alloc();
                if (!child) return -1;
                *pte = pa2pte((uint64_t)child) | PTE_V;
            }
            p = (pte_t *)pte2pa(*pte);
        }
        pte_t *leaf = &p[vpn(a, 0)];
        if (*leaf & PTE_V) return -1;
        *leaf = pa2pte(pa) | perm | PTE_V | PTE_A;
        if (perm & PTE_W) *leaf |= PTE_D;
    }
    return 0;
}

static int walk_user_pte(pagetable_t pt, uint64_t va, pte_t **out)
{
    if (va >= (1ULL << 39)) return -1;
    pte_t *p = pt;
    for (int level = 2; level > 0; --level) {
        pte_t *e = &p[vpn(va, level)];
        if (!(*e & PTE_V)) return -1;
        p = pt_from_pte(*e);
    }
    *out = &p[vpn(va, 0)];
    return (*out && (**out & PTE_V)) ? 0 : -1;
}

static int kernel_map(pagetable_t pt)
{
    uint64_t kernel_start = 0x80000000ULL;
    uint64_t kernel_end = ((uint64_t)(__kernel_end) + PGSIZE - 1) & ~(PGSIZE - 1);
    if (mappages(pt, kernel_start, kernel_end - kernel_start, kernel_start,
                 PTE_R | PTE_W | PTE_X) < 0) return -1;
    if (mappages(pt, UART0, PGSIZE, UART0, PTE_R | PTE_W) < 0) return -1;
    return 0;
}

void vm_init(void)
{
    memset(pt_used, 0, sizeof(pt_used));
    kernel_pt = NULL;
}

pagetable_t vm_create_user(const unsigned char *image, size_t image_size, int slot)
{
    if (slot < 0 || slot >= NPROC) return NULL;
    pagetable_t pt = pt_alloc();
    if (!pt) return NULL;
    if (kernel_map(pt) < 0) {
        pt_destroy_level(pt, 2);
        return NULL;
    }

    extern unsigned char user_mem_base[][USER_TOTAL_PAGES][PGSIZE];
    unsigned char (*mem)[PGSIZE] = user_mem_base[slot];
    memset(mem, 0, USER_TOTAL_PAGES * PGSIZE);
    if (image_size > USER_IMAGE_PAGES * PGSIZE) image_size = USER_IMAGE_PAGES * PGSIZE;
    memcpy(mem, image, image_size);

    if (mappages(pt, USER_BASE, USER_IMAGE_PAGES * PGSIZE, (uint64_t)&mem[0][0],
                 PTE_R | PTE_W | PTE_X | PTE_U) < 0) {
        vm_destroy(pt);
        return NULL;
    }

    uint64_t stack_pa = (uint64_t)&mem[USER_TOTAL_PAGES - 1][0];
    uint64_t stack_va = USER_STACK_TOP - PGSIZE;
    if (mappages(pt, stack_va, PGSIZE, stack_pa, PTE_R | PTE_W | PTE_U) < 0) {
        vm_destroy(pt);
        return NULL;
    }
    return pt;
}

pagetable_t vm_clone_user(int dst_slot, int src_slot)
{
    if (dst_slot < 0 || dst_slot >= NPROC || src_slot < 0 || src_slot >= NPROC) return NULL;
    pagetable_t pt = pt_alloc();
    if (!pt) return NULL;
    if (kernel_map(pt) < 0) {
        pt_destroy_level(pt, 2);
        return NULL;
    }

    extern unsigned char user_mem_base[][USER_TOTAL_PAGES][PGSIZE];
    unsigned char (*dst)[PGSIZE] = user_mem_base[dst_slot];
    unsigned char (*src)[PGSIZE] = user_mem_base[src_slot];
    memcpy(dst, src, USER_TOTAL_PAGES * PGSIZE);

    if (mappages(pt, USER_BASE, USER_IMAGE_PAGES * PGSIZE, (uint64_t)&dst[0][0],
                 PTE_R | PTE_W | PTE_X | PTE_U) < 0) {
        vm_destroy(pt);
        return NULL;
    }
    if (mappages(pt, USER_STACK_TOP - PGSIZE, PGSIZE, (uint64_t)&dst[USER_TOTAL_PAGES - 1][0],
                 PTE_R | PTE_W | PTE_U) < 0) {
        vm_destroy(pt);
        return NULL;
    }
    return pt;
}

void vm_switch(pagetable_t pt)
{
    if (!pt) return;
    w_satp(SATP_MODE_SV39 | (((uint64_t)pt) >> 12));
    sfence_vma();
}

pagetable_t kernel_pagetable(void)
{
    if (!kernel_pt) {
        kernel_pt = pt_alloc();
        if (!kernel_pt) return NULL;
        if (kernel_map(kernel_pt) < 0) {
            pt_release(kernel_pt);
            kernel_pt = NULL;
            return NULL;
        }
    }
    return kernel_pt;
}

void vm_destroy(pagetable_t pt)
{
    if (!pt || pt == kernel_pt) return;
    pt_destroy_level(pt, 2);
}

uint64_t vm_satp(pagetable_t pt)
{
    return SATP_MODE_SV39 | (((uint64_t)pt) >> 12);
}

static int translate_user(pagetable_t pt, uint64_t va, uint64_t *pa)
{
    pte_t *pte;
    if (walk_user_pte(pt, va, &pte) < 0) return -1;
    if (!(*pte & PTE_U)) return -1;
    *pa = pte2pa(*pte) + (va & (PGSIZE - 1));
    return 0;
}

int copyin(void *dst, pagetable_t pt, uint64_t srcva, size_t len)
{
    unsigned char *d = dst;
    while (len) {
        uint64_t pa;
        if (translate_user(pt, srcva, &pa) < 0) return -1;
        size_t n = MIN(len, PGSIZE - (srcva & (PGSIZE - 1)));
        memcpy(d, (void *)pa, n);
        d += n;
        srcva += n;
        len -= n;
    }
    return 0;
}

int copyout(pagetable_t pt, uint64_t dstva, const void *src, size_t len)
{
    const unsigned char *s = src;
    while (len) {
        uint64_t pa;
        if (translate_user(pt, dstva, &pa) < 0) return -1;
        size_t n = MIN(len, PGSIZE - (dstva & (PGSIZE - 1)));
        memcpy((void *)pa, s, n);
        s += n;
        dstva += n;
        len -= n;
    }
    return 0;
}

int copyinstr(char *dst, pagetable_t pt, uint64_t srcva, size_t max)
{
    for (size_t i = 0; i < max; ++i) {
        if (copyin(&dst[i], pt, srcva + i, 1) < 0) return -1;
        if (dst[i] == '\0') return 0;
    }
    dst[max - 1] = '\0';
    return 0;
}
