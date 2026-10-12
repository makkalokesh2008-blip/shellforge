# NanoBoot Viva Questions and Answers

## Why QEMU?
QEMU provides a reproducible RISC-V `virt` machine without requiring physical RISC-V hardware.

## Why OpenSBI?
The kernel runs in S-mode and uses the standard firmware interface for the machine-mode timer service.

## What is `stvec`?
It contains the S-mode trap-vector address used when the processor takes a supervisor trap.

## What is `scause`?
It identifies the reason for the current trap, such as user `ecall` or a supervisor timer interrupt.

## What is `sepc`?
It records the program counter at the point of the trap. For a user `ecall`, the kernel advances it by 4 bytes before returning.

## Why does `fork` need separate page tables?
A child must have an isolated user address space. This implementation copies the parent's user pages into another process slot.

## What is Sv39?
Sv39 is RISC-V's 3-level 39-bit virtual-address translation scheme. NanoBoot builds the three-level page-table hierarchy explicitly.

## How does the shell become user mode?
The kernel maps the embedded shell image, loads its entry address into `sepc`, clears `SPP`, sets `sscratch` to the process trapframe, and executes `sret`.

## How does scheduling work?
The fixed process table is scanned round-robin. Timer interrupts mark the current runnable process and choose another runnable slot.

## Why is there an idle loop?
When no process is runnable, the kernel switches to its kernel page table and waits for a timer interrupt. A later wakeup makes a process runnable again.

## What is RAMFS?
It is a tiny in-memory filesystem represented by compile-time strings. It demonstrates filesystem-style operations without implementing a disk driver.

## Why is C used with assembly?
C handles high-level kernel logic, while assembly is needed for the boot entry, trap register save/restore and context-switch primitive.
