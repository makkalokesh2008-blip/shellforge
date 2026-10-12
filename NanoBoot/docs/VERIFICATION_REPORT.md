# NanoBoot Verification Report

Release review: 2026-10-10

## Checks completed while packaging

- The complete kernel and user shell compile and link successfully with a RISC-V-targeting Clang/LLD toolchain (`rv64imac_zicsr_zifencei`, LP64 ABI, freestanding, warnings-as-errors).
- The Makefile static verification target passes.
- The linked kernel is RV64 RISC-V and includes the boot entry, user trap entry, kernel trap entry, and embedded user image symbols.
- The user shell `_start` symbol is linked at `0x40000000`, the address where the kernel begins user execution.
- The process initialization code preserves a valid per-process kernel stack pointer in the trapframe after clearing the other registers.
- The user shell binary is checked against the 4-page image limit.
- `.sdata` / `.sbss` are included in the kernel's explicitly cleared data ranges.
- The no-runnable-process timer path returns an idle trapframe rather than trying to `ret` through an interrupted `ra` value.
- Timer wakeups only release timed sleepers; a process blocked in `wait` remains asleep until a child-exit event.

## Important runtime status

A live QEMU boot/smoke test was **not executable in the packaging environment** because `qemu-system-riscv64` is not installed here, and this environment could not reach its configured Debian package repositories. Therefore this report does not claim that the live syscalls or fork/wait test have passed at runtime yet.

On the Ubuntu/WSL laptop, `bash START_NANOBOOT.sh` installs missing packages (requires working apt access), rebuilds the source with the installed RISC-V GCC toolchain, performs static verification, runs `make qemu-test`, saves the console output to `tests/qemu-output.txt`, and opens an interactive QEMU serial console if the smoke tests pass.

The smoke test checks boot, user-mode shell entry, `getpid`, RAMFS `ls`/`cat`, user/kernel address validation, sleep/wakeup, fork and wait. If any expected marker is missing, it exits with a failure and keeps the transcript for debugging.

## Known scope limitations

- Single-hart QEMU `virt` target; the spinlock/context-switch building blocks are included, but multi-hart SMP is not claimed as demonstrated.
- The RAM filesystem is a tiny in-memory read-only demo, not a disk filesystem.
- `wait` currently returns the child PID; a full POSIX wait-status interface is outside this miniature kernel's interface.
- Timer-based sleeps are in logical timer ticks, not wall-clock seconds.
