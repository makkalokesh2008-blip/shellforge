# Requirements-to-Evidence Matrix

| OS concept / requirement | Implementation files | Live command / evidence |
|---|---|---|
| RISC-V boot and UART console | `kernel/boot.S`, `kernel/main.c`, `kernel/uart.c` | QEMU boot banner |
| Supervisor-mode trap vector | `kernel/entry.S`, `kernel/trap.c` | `ecall` shell commands and trap code |
| System-call dispatch (`ecall`) | `user/syscall.S`, `kernel/syscall.c` | `getpid`, `ps`, `ls`, `cat`, `sleep`, `forktest` |
| `read` / `write` | `kernel/syscall.c`, `kernel/uart.c` | typing commands and seeing output |
| Per-process Sv39 page tables | `kernel/vm.c`, `kernel/proc.c` | static verification plus user shell execution |
| User-memory validation | `kernel/vm.c`, `kernel/syscall.c` | `isotest` expects `isolation PASS: kernel address rejected` |
| Round-robin process scheduling | `kernel/proc.c`, `kernel/trap.c` | `ps`, `forktest`, timer-driven scheduling |
| `fork`, `exit`, `wait` | `kernel/proc.c`, `kernel/syscall.c` | `forktest` expects child and wait markers |
| Sleep / wakeup | `kernel/proc.c`, `kernel/timer.c` | `sleep 3` prints sleeping / awake |
| RAMFS | `kernel/ramfs.c`, `kernel/syscall.c` | `ls`, `cat README.txt`, `cat hello.txt` |
| User shell | `user/sh.c`, `user/user.ld` | `help` at `nb$` |
| Automated runtime test | `tests/test.sh`, `tests/demo_commands.txt` | `make qemu-test`; output goes to `tests/qemu-output.txt` |

## Verification status

The release packaging build and static checks passed with a RISC-V-targeting Clang/LLD toolchain. Live QEMU results are deliberately not marked passed in this package because QEMU was unavailable in the packaging environment. The target Ubuntu machine must run `bash START_NANOBOOT.sh` to produce the runtime transcript.
