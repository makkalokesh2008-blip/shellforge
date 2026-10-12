# NanoBoot Submission Checklist

- [x] Makefile
- [x] Linker script
- [x] RISC-V boot assembly
- [x] UART driver
- [x] Trap entry/return assembly
- [x] Supervisor trap handler
- [x] SBI timer support
- [x] Fixed process table
- [x] Round-robin scheduler
- [x] Sleep/wakeup
- [x] `fork` / `exit` / `wait`
- [x] User-mode syscall wrappers (static/build checked)
- [x] Sv39 page tables
- [x] User memory copy validation
- [x] Tiny RAMFS
- [x] User shell
- [x] QEMU run target
- [x] QEMU smoke-test harness (runtime run is pending on target Ubuntu)
- [x] Prebuilt `kernel.elf` and `user/sh.elf` included
- [x] Ubuntu setup script
- [x] Demo script
- [x] Project report
- [x] Weekly build log
- [x] Viva Q&A

## Verification note

Static checks completed during packaging:
- RISC-V 64 ELF build/link passed with a RISC-V-capable Clang/LLD toolchain.
- User shell binary size checked against the 4-page image limit.
- Kernel boot/trap/image symbols checked.
- User shell `_start` verified at `0x40000000`.
- Source review found and fixed the lost per-process kernel stack pointer and the user entry-point ordering.

The live QEMU boot/syscall smoke test must run on the target Ubuntu laptop because this packaging environment does not contain QEMU. Use `bash START_NANOBOOT.sh`; it runs the live checks and saves `tests/qemu-output.txt` before opening the interactive demo.
