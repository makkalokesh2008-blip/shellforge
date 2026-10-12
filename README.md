# ShellForge Repository

This repository contains two projects:

| Project | Directory | Description |
|---|---|---|
| **NanoBoot** | `NanoBoot/` | Primary OS project: educational RISC-V 64-bit OS for QEMU |
| **ShellForge Shell** | `src/` | Linux user-space shell implementation in C |

---

# NanoBoot — RISC-V Educational Operating System

An educational RISC-V 64-bit operating system for QEMU `virt`, demonstrating kernel boot, supervisor traps, hardware system calls, process management, Sv39 virtual memory, timer handling, an in-memory filesystem, and a user-mode command shell.

## Verified Features

| Module | Result | Verification Method |
|---|---|---|
| Boot & Initialization | **PASS** | OpenSBI handoff to supervisor mode at `0x80200000` |
| UART Console | **PASS** | 16550 serial driver with polled I/O |
| Trap & Interrupt Handling | **PASS** | `stvec` vector, `entry.S` context save/restore |
| System Calls | **PASS** | 10 syscalls (`read`, `write`, `fork`, `exit`, `wait`, `getpid`, `sleep`, `ps`, `ls`, `cat`) |
| Process Management | **PASS** | `proc_table[8]`, fork/exit/wait, zombie reclamation |
| Round-Robin Scheduler | **PASS** | Preemptive multitasking (`schedtest` interleave PASS) |
| Sv39 Virtual Memory | **PASS** | 3-level page tables, per-process address spaces |
| Memory Isolation | **PASS** | Kernel address rejected (`isotest` PASS) |
| Timer Interrupts | **PASS** | SBI TIME extension timer ticks |
| Blocking Sleep / Wakeup | **PASS** | `proc_sleep()` / `proc_wakeup()` (`sleep 3` PASS) |
| RAMFS | **PASS** | Embedded in-memory filesystem (`ls`, `cat` PASS) |
| User-Mode Shell | **PASS** | Tokenizer, parser, argv, built-in execution (20/20 PASS) |
| Shell Pipes (`|`) | **PARTIAL** | Parsed with clean diagnostic; execution blocked (no fd table) |
| I/O Redirection (`<`, `>`, `>>`) | **PARTIAL** | Parsed and validated; execution blocked (read-only RAMFS) |
| Background Execution (`&`) | **PARTIAL** | Parsed with syntax validation; execution blocked (single UART) |
| Multi-Hart Support | **NOT IMPLEMENTED** | Single-hart target (Hart 0); multi-hart halted by design |
| Copy-on-Write (COW) Fork | **NOT IMPLEMENTED** | Fixed 16 KB user images copied directly via `vm_clone_user()` |

## Quick Start (NanoBoot)

```bash
cd NanoBoot
make clean
make
make qemu
```

To run the prebuilt kernel without recompilation:
```bash
cd NanoBoot
bash run_prebuilt.sh
```

## Running Tests

From `NanoBoot/`:
```bash
# 1. Static ELF and size verification:
make verify

# 2. Automated 14-point QEMU smoke test:
bash tests/test.sh

# 3. Comprehensive 20-point shell and syscall regression suite:
python3 tests/test_lexer_parser.py
```

From repository root (ShellForge Linux shell):
```bash
make
./tests/test_regression.sh
```

## Repository Structure

```text
shellforge-push/
├── Makefile                        # ShellForge root Makefile
├── README.md                       # This file
├── shellforge                      # ShellForge compiled Linux shell binary
├── src/                            # ShellForge Linux shell source files
├── include/                        # ShellForge headers
├── tests/                          # ShellForge test suite
├── docs/                           # Combined project documentation
└── NanoBoot/                       # PRIMARY OS PROJECT (RISC-V 64 Kernel)
    ├── Makefile                    # Bare-metal RISC-V Makefile
    ├── README.md                   # NanoBoot complete documentation
    ├── START_NANOBOOT.sh           # One-click Ubuntu build, test, and run script
    ├── run_prebuilt.sh             # Launch prebuilt kernel in QEMU
    ├── linker.ld                   # Kernel linker script (0x80200000)
    ├── include/                    # Kernel and user common headers
    ├── kernel/                     # Kernel C and ASM source files
    ├── user/                       # User-space shell and syscall stubs
    ├── tests/                      # Automated smoke & regression test suites
    ├── screenshots/                # 8 verified execution screenshots
    └── docs/                       # Architecture, viva, and review evidence
```

## Course Context

Developed for **25CS2104E** (Operating Systems Capstone) course at KLEF.
Evidence, viva guide, and weekly build log are in `NanoBoot/`.
