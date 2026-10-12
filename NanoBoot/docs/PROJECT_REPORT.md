# NanoBoot Project Report

## Project title
**NanoBoot: RISC-V Boot-to-Shell Kernel**

## Team
- P. Sharan Teja — 2500032333
- M. Krishnama Nayudu — 2500031918
- M. Lokesh — 2500030720
- Section: 2
- Faculty: Dr.T.SelvamuthuKumar
- Academic year: 2026–27

## Problem statement
Build a small operating-system kernel for the RISC-V `virt` machine in QEMU that boots into supervisor mode, writes to a UART console, handles traps and timer interrupts, exposes basic system calls, schedules multiple processes, provides user/kernel isolation with Sv39, includes a tiny RAM filesystem, and presents a command shell.

## Selected approach
An xv6-style, single-hart NanoBoot kernel. QEMU's standard OpenSBI firmware supplies the machine-mode firmware layer; NanoBoot itself is an S-mode kernel.

## Architecture
```text
QEMU virt
   │
   ├── OpenSBI firmware
   │        │
   │        └── NanoBoot S-mode kernel
   │               ├── UART
   │               ├── trap + syscall layer
   │               ├── timer / SBI TIME
   │               ├── fixed process table
   │               ├── round-robin scheduler
   │               ├── Sv39 page tables
   │               ├── RAMFS
   │               └── user shell
   │
   └── serial console → Ubuntu terminal
```

## Core execution flow
1. OpenSBI starts the kernel in S-mode.
2. `boot.S` creates the kernel stack and enters `main()`.
3. `main()` initializes UART, VM, processes, traps and the timer.
4. A user shell image is embedded into the kernel and loaded into a per-process Sv39 address space.
5. `enter_user()` switches to the process page table and executes `sret` into user mode.
6. User applications invoke `ecall` wrappers.
7. `user_trap_entry` saves the full register state into a trapframe.
8. `trap_handler` dispatches the system call or timer interrupt.
9. The scheduler may keep the current process, switch to another runnable process, or enter the kernel idle loop.
10. `sret` returns to the chosen user process.

## System calls
| Number | Call | Purpose |
|---:|---|---|
| 0 | read | Read bytes from UART into user memory |
| 1 | write | Copy bytes from user memory to UART |
| 2 | fork | Duplicate a process address space |
| 3 | exit | Terminate the current process |
| 4 | wait | Wait for a child to exit |
| 5 | getpid | Return the current process id |
| 6 | sleep | Block until timer ticks advance |
| 7 | ps | Print the process table |
| 8 | ls | List RAMFS files |
| 9 | cat | Display an in-memory file |

## Memory isolation
Each process gets its own Sv39 root page table. Kernel memory is mapped supervisor-only. The user shell image and stack are mapped with the user permission bit. Kernel `copyin`, `copyout`, and `copyinstr` validate user mappings before data transfer.

## Scheduler
The process table is fixed-size (`NPROC = 8`). Runnable processes are selected in round-robin order. Timer interrupts provide preemption. A blocked `sleep` process becomes runnable when its timer deadline passes; a process blocked in `wait` remains blocked until a child-exit event wakes it.

## RAMFS
The RAMFS stores three small files in kernel memory:
- `README.txt`
- `hello.txt`
- `design.txt`

## Testing
`tests/test.sh` launches QEMU in `-nographic` mode, feeds shell commands, captures the serial console, and checks for boot, the user shell, `getpid`, RAMFS access, isolation, sleep/wakeup, fork and wait. The package-time static build passed; the live QEMU smoke test still must be run on the target Ubuntu machine because QEMU was not available in the packaging environment. See `VERIFICATION_REPORT.md` for exact status.

## Limitations / stretch work
- Single hart only in this submission; the spinlock primitive is present for later SMP work.
- Fork uses a full memory copy rather than copy-on-write.
- RAMFS is read-only and deliberately tiny.
- The kernel uses polling UART input for simplicity.
