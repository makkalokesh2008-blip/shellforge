.section .text.boot
.globl _start
_start:
    la sp, boot_stack_top

    # Start in the firmware-provided physical address space.
    csrw satp, zero
    sfence.vma zero, zero

    # Clear the kernel BSS before entering C.
    la t0, __bss_start
    la t1, __bss_end
1:
    bgeu t0, t1, 2f
    sd zero, 0(t0)
    addi t0, t0, 8
    j 1b
2:
    call main
1:
    wfi
    j 1b

.section .bss
.align 12
boot_stack:
    .skip 16384
boot_stack_top:
