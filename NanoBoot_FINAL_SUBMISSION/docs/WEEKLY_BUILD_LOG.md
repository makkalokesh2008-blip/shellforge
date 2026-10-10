# NanoBoot Weekly Build Log

| Week | Milestone | Repository evidence |
|---:|---|---|
| 1 | Boot + UART | `boot.S`, `uart.c`, `make qemu` |
| 2 | Linker + stack + BSS | `linker.ld`, boot stack, C runtime BSS clear path |
| 3 | Process table | `proc.c`, fixed `NPROC` table |
| 4 | Scheduler + context primitive | `proc_schedule()`, `swtch.S` |
| 5 | Syscalls | `entry.S`, `syscall.c`, user syscall stubs |
| 6 | Trap + timer | `trap.c`, `timer.c`, SBI TIME |
| 7 | Sleep/wakeup | `proc_sleep()`, `proc_wakeup()`, `wait` blocking |
| 8 | Sv39 | `vm.c`, per-process root page tables |
| 9 | RAMFS | `ramfs.c` and `ls/cat` syscalls |
| 10 | Fork/exit/wait | `proc_fork()`, `proc_exit()`, `proc_wait()` |
| 11 | Coordination | `spinlock.c`; single-hart target kept intentionally simple |
| 12 | Shell + integration | `user/sh.c`, QEMU smoke test |
