# ShellForge Repository

This repository contains two projects:

| Project | Directory | Description |
|---------|-----------|-------------|
| **NanoBoot** | NanoBoot/ | Primary OS project: educational RISC-V 64-bit OS for QEMU |
| ShellForge Shell | src/ | Linux user-space shell implementation |

---

# NanoBoot - RISC-V Educational Operating System

An educational RISC-V 64-bit operating system for QEMU irt, demonstrating kernel boot, traps, system calls, process management, virtual memory, timer handling, an in-memory filesystem, and a user-mode command shell.

## Verified Features

| Module | Result |
|--------|--------|
| Boot and Initialization | PASS |
| UART Console | PASS |
| Trap and Interrupt Handling | PASS |
| System Calls (read/write/fork/exit/wait/getpid/sleep/ps/ls/cat) | PASS |
| Process Management (fork/exit/wait/zombie reaping) | PASS |
| Round-Robin Scheduler | PASS |
| Sv39 Virtual Memory (per-process page tables) | PASS |
| Memory Isolation (cross-process + kernel address rejection) | PASS |
| Timer Interrupts (SBI TIME extension) | PASS |
| Blocking Sleep/Wakeup | PASS |
| RAMFS (ls and cat) | PASS |
| User-Mode Shell (lexer/parser/argv/built-ins) | PASS |
| Pipes and Redirection | PARTIAL (parsed; not executed) |
| Background Execution | PARTIAL (parsed; not executed) |
| Multi-Hart | NOT IMPLEMENTED (single-hart design) |
| Copy-on-Write | NOT IMPLEMENTED (full physical copy on fork) |

## Quick Start



## Tests



## Repository Structure



## Known Limitations

- Pipes (|), redirection (>), and background (&) are parsed but not executed
- Single-hart design only (no multi-CPU support)
- ork uses full physical copy (no copy-on-write)

## Course Context

Developed for **25CS2104E** Operating Systems course at KLEF.
Evidence, viva guide, and weekly build log are in NanoBoot/.
