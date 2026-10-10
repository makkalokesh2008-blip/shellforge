.section .text
.align 2

.globl user_trap_entry
user_trap_entry:
    # sscratch contains the current process trapframe pointer.
    csrrw a0, sscratch, a0
    sd zero, 0(a0)
    sd ra, 8(a0)
    sd sp, 16(a0)
    sd gp, 24(a0)
    sd tp, 32(a0)
    sd t0, 40(a0)
    sd t1, 48(a0)
    sd t2, 56(a0)
    sd s0, 64(a0)
    sd s1, 72(a0)
    sd a1, 88(a0)
    sd a2, 96(a0)
    sd a3, 104(a0)
    sd a4, 112(a0)
    sd a5, 120(a0)
    sd a6, 128(a0)
    sd a7, 136(a0)
    sd s2, 144(a0)
    sd s3, 152(a0)
    sd s4, 160(a0)
    sd s5, 168(a0)
    sd s6, 176(a0)
    sd s7, 184(a0)
    sd s8, 192(a0)
    sd s9, 200(a0)
    sd s10, 208(a0)
    sd s11, 216(a0)
    sd t3, 224(a0)
    sd t4, 232(a0)
    sd t5, 240(a0)
    sd t6, 248(a0)

    ld a1, 280(a0)
    mv sp, a1
    csrr t0, sscratch
    sd t0, 80(a0)
    csrr t0, sepc
    sd t0, 256(a0)
    csrr t0, sstatus
    sd t0, 264(a0)
    csrr t0, scause
    sd t0, 272(a0)

    call trap_handler
    j restore_user_or_idle

.globl kernel_trap_entry
kernel_trap_entry:
    addi sp, sp, -64
    sd ra, 0(sp)
    sd t0, 8(sp)
    sd t1, 16(sp)
    sd a0, 24(sp)
    sd a1, 32(sp)
    sd a2, 40(sp)
    sd a3, 48(sp)
    sd a4, 56(sp)

    csrr a0, scause
    call kernel_trap_handler

    ld ra, 0(sp)
    ld t0, 8(sp)
    ld t1, 16(sp)
    ld a1, 32(sp)
    ld a2, 40(sp)
    ld a3, 48(sp)
    ld a4, 56(sp)
    addi sp, sp, 64

    beqz a0, 1f
    j restore_user_or_idle
1:
    # If the C trap handler has no selected process, resume the idle context.
    call idle_trapframe_ptr
    j restore_user_or_idle

# a0 = trapframe pointer.  C has already selected the target process.
restore_user_or_idle:
    mv t6, a0

    # C sets stvec to user_trap_entry before a user return.
    # satp has already been switched if needed.
    ld t0, 256(t6)
    csrw sepc, t0
    ld t0, 264(t6)
    csrw sstatus, t0
    csrw sscratch, t6

    ld ra, 8(t6)
    ld sp, 16(t6)
    ld gp, 24(t6)
    ld tp, 32(t6)
    ld t0, 40(t6)
    ld t1, 48(t6)
    ld t2, 56(t6)
    ld s0, 64(t6)
    ld s1, 72(t6)
    ld a0, 80(t6)
    ld a1, 88(t6)
    ld a2, 96(t6)
    ld a3, 104(t6)
    ld a4, 112(t6)
    ld a5, 120(t6)
    ld a6, 128(t6)
    ld a7, 136(t6)
    ld s2, 144(t6)
    ld s3, 152(t6)
    ld s4, 160(t6)
    ld s5, 168(t6)
    ld s6, 176(t6)
    ld s7, 184(t6)
    ld s8, 192(t6)
    ld s9, 200(t6)
    ld s10, 208(t6)
    ld s11, 216(t6)
    ld t3, 224(t6)
    ld t4, 232(t6)
    ld t5, 240(t6)
    ld t6, 248(t6)
    sret

.globl enter_user
enter_user:
    # a0 = trapframe pointer, a1 = satp
    mv t6, a0
    csrw satp, a1
    sfence.vma zero, zero
    la t0, user_trap_entry
    csrw stvec, t0
    csrw sscratch, t6
    ld t0, 256(t6)
    csrw sepc, t0
    ld t0, 264(t6)
    csrw sstatus, t0
    j restore_user_or_idle

.globl idle_loop
idle_loop:
    la t0, kernel_trap_entry
    csrw stvec, t0
    csrw sscratch, zero
    li t0, 0x20
    csrs sie, t0
1:
    wfi
    j 1b
