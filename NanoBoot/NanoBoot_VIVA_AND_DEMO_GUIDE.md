# NanoBoot — Comprehensive Viva & Faculty Demonstration Guide
## Academic Year 2026-27 | Course: 25CS2104E (Operating Systems Capstone)
### Students: P. Sharan Teja (2500032333) · M. Krishnama Nayudu (2500031918) · M. Lokesh (2500030720)
**Project Title:** NanoBoot — From-Scratch RISC-V 64-bit Operating System & User-Mode Shell  
**Location:** `NanoBoot_FINAL_SUBMISSION/`

---

## 1. 30–60 Second Verbal Elevator Pitch (Script for Faculty Presentation)

> *"Good morning, Sir / Ma'am. For our operating systems capstone, our team built **NanoBoot**, a monolithic 64-bit operating system running on bare-metal RISC-V QEMU `virt` hardware.*  
> *When the machine powers on, OpenSBI initializes Machine mode and hands control to our kernel entry point in Supervisor mode. NanoBoot configures the 16550 UART serial driver, constructs three-level Sv39 virtual memory page tables, establishes hardware trap handling, and initializes a fixed process table.*  
> *It then spawns Process 1—an isolated User-mode shell linked at virtual address `0x40000000`. The shell features a custom finite-state tokenizer and recursive parser supporting quoted arguments and error diagnostics. User programs transition across the privilege boundary through RISC-V `ecall` system calls to perform I/O, process management via `fork`, `wait`, and `exit`, sleep coordination, memory isolation testing, and reading from an in-memory RAMFS.*  
> *Our entire implementation compiles cleanly with `-Wall -Wextra -Werror`, passes our 19-test automated regression suite, and is verified live in QEMU across eight rigorous rubric points."*

---

## 2. System Architecture & Memory Map

```
+--------------------------------------------------------------------------+
|                            PHYSICAL MEMORY                               |
+-------------------+------------------------------------------------------+
| Address Range     | Description                                          |
+-------------------+------------------------------------------------------+
| 0x10000000        | UART 16550 MMIO Registers (polled RX/TX)             |
| 0x80000000        | OpenSBI Firmware Base (Machine Mode)                 |
| 0x80200000        | NanoBoot Kernel Entry (`_start` in `kernel/boot.S`)  |
| 0x80200000-End    | Kernel .text, .rodata, .data, .bss, Page Table Pool  |
| 0x87000000+       | User Process Memory Banks (`user_mem_base[NPROC]`)   |
+-------------------+------------------------------------------------------+

+--------------------------------------------------------------------------+
|                      USER VIRTUAL ADDRESS SPACE (Sv39)                   |
+-------------------+------------------------------------------------------+
| Address Range     | Mapping & Permissions                                |
+-------------------+------------------------------------------------------+
| 0x40000000        | USER_BASE: User Shell Binary (.text/.data) [U+R+W+X] |
| 0x7FFFF000        | USER_STACK: 4 KB User Stack (grows down) [U+R+W]     |
| 0x80000000-End    | Supervisor Kernel Identity Mapping [R+W+X] (No PTE_U)|
+-------------------+------------------------------------------------------+
```

---

## 3. Core Subsystem Walkthroughs

### A. Boot Sequence
1. **Power-On & Firmware:** QEMU loads OpenSBI into memory at `0x80000000`. OpenSBI executes in M-mode, discovers platform hardware, configures PMP registers, and delegates traps.
2. **Supervisor Entry:** OpenSBI jumps to `0x80200000` (`kernel/boot.S`) in S-mode.
3. **Runtime Zeroing & Stack:** `boot.S` clears `.bss` and `.sbss` using 64-bit store instructions (`sd zero, (t0)`), initializes `sp` to `boot_stack_top` with 16-byte alignment, and calls C `main()`.
4. **Subsystem Bootstrap:** `main()` initializes UART (`0x10000000`), prints the boot banner, initializes Sv39 page table structures (`vm_init()`), trap vectors (`w_stvec(kernel_trap_entry)`), and process tables (`proc_init()`).
5. **Init Process Creation:** The embedded user shell ELF (`user/sh.bin`) is copied to process slot 0 (`proc_create_init()`), page tables are allocated, and `enter_user()` switches `satp` to the user page table and executes `sret` into U-mode at `0x40000000`.

### B. Trap & Syscall Execution Flow
When user space executes an `ecall`:
1. **Hardware Transition:** CPU hardware sets `scause = 8` (`SCAUSE_ECALL_U`), saves user PC into `sepc`, records privilege state in `sstatus.SPP` (set to 0 for U-mode), disables interrupts (`sstatus.SIE = 0`), and jumps to `stvec` (`user_trap_entry`).
2. **State Preservation:** `user_trap_entry` swaps `a0` with `sscratch` to obtain a pointer to the process's `trapframe_t`. It saves all 31 general registers (`ra`, `sp`, `gp`, `tp`, `t0`-`t6`, `s0`-`s11`, `a0`-`a7`), switches to the per-process `kernel_sp`, and calls C `trap_handler()`.
3. **Dispatch:** `trap_handler()` advances `tf->sepc += 4` (so execution resumes after the `ecall`), inspects `tf->regs[17]` (`a7`, syscall number), and invokes `syscall_dispatch()`.
4. **Argument Validation:** All pointers passed from user space are validated via `copyin()`/`copyout()`. If a pointer lacks `PTE_U` or exceeds bounds, `-1` (EFAULT) is returned.
5. **Return to User Mode:** The return code is written into `tf->regs[10]` (`a0`). `restore_user_or_idle` restores user registers from the trapframe, sets `sscratch`, restores `sepc` and `sstatus`, and executes `sret`, switching back to U-mode.

### C. Scheduler & Process Lifecycle
- **Process Table:** Fixed table of 8 processes (`NPROC = 8`). Each process has an explicit state: `P_UNUSED`, `P_RUNNABLE`, `P_RUNNING`, `P_SLEEPING`, or `P_ZOMBIE`.
- **`fork()`:** `proc_fork()` clones the parent's address space into a free slot using `vm_clone_user()`, copies register state, sets child return register `a0 = 0`, and returns child PID to parent.
- **`exit()`:** `proc_exit()` transitions process to `P_ZOMBIE`, records exit status, wakes any waiting parent, and triggers the scheduler.
- **`wait()`:** `proc_wait()` searches for child processes. If a child is a zombie, it frees the child page table, reclaims the process slot and PID, and returns the child PID. If children exist but are still running, the parent transitions to `P_SLEEPING`.
- **Scheduler:** Round-robin selection across `proc_table`. If no process is runnable, the kernel idles via `wfi` in `idle_loop`.

### D. Sv39 Virtual Memory & Memory Isolation
- **Sv39 Mechanism:** 39-bit virtual address divided into three 9-bit page table indexes (`VPN[2]`, `VPN[1]`, `VPN[0]`) and a 12-bit offset.
- **Root Page Table:** Each process possesses its own distinct L2 root page table pointed to by `satp` (mode 8 = Sv39).
- **Isolation Enforcement:**
  - Kernel memory mappings (`0x80200000+`) have permissions `PTE_R | PTE_W | PTE_X`, but omit `PTE_U`.
  - When user code in U-mode attempts to access kernel addresses directly, the MMU hardware triggers a page fault.
  - When user code passes kernel pointers to syscalls (`write`, `read`, `cat`), `translate_user()` verifies that the target PTE has `PTE_U` set. If `PTE_U` is missing, `translate_user()` rejects the operation and returns `-1`. This is verified live by `isotest`.

### E. User Shell, Lexer & Built-ins
- **Lexer (`tokenize()`):** Character-by-character finite-state machine in `user/sh.c`. Groups non-whitespace characters into `TOK_WORD`. Preserves whitespace and operator characters enclosed within `'...'` or `"..."`. Detects unclosed quotes and returns `syntax error: unclosed quote`.
- **Parser (`parse_command()`):** Assembles token vectors into structured `command_t` containing `argv` arrays, redirection targets, and background flags. Validates argument limits (`MAX_ARGS = 16`, `MAX_TOKEN_LEN = 32`).
- **Built-in Execution:** Dispatches commands through structured `argv`:
  - `help`: Displays available commands.
  - `getpid`: Invokes `sys_getpid()` syscall.
  - `ps`: Invokes `sys_ps()` syscall.
  - `echo TEXT`: Prints arguments separated by spaces.
  - `sleep N`: Invokes `sys_sleep(N)` syscall.
  - `forktest`: Exercises `fork()`, child execution, `exit()`, and parent `wait()`.
  - `ls`: Invokes `sys_ls()` syscall on RAMFS.
  - `cat FILE`: Invokes `sys_cat()` syscall on RAMFS.
  - `isotest`: Validates memory protection boundary.
  - `exit`: Invokes `sys_exit(0)` syscall.

---

## 4. Live Demonstration Step-by-Step Script

To run the live demonstration for faculty:

```bash
cd NanoBoot_FINAL_SUBMISSION
bash START_NANOBOOT.sh
```

Execute these commands in order:

| Step | Command | Faculty Explanation | Expected Observable Output |
|:---:|---|---|---|
| **1** | *(boot)* | "Here is OpenSBI booting in M-mode, transferring control to NanoBoot at 0x80200000 in S-mode, initializing Sv39 paging, and launching Process 1 in U-mode." | OpenSBI banner, NanoBoot banner, `init: pid=1 ready`, and `nb$` shell prompt. |
| **2** | `help` | "Demonstrates shell parsing and built-in help dispatch." | Menu listing all 10 supported commands. |
| **3** | `getpid` | "Issues an `ecall` to `sys_getpid()`. Demonstrates user-to-kernel trap dispatch." | `pid=1` |
| **4** | `ps` | "Syscall inspecting kernel process table directly from user space." | Table showing `PID 1` in `RUNNING` state. |
| **5** | `echo hello from NanoBoot` | "Demonstrates tokenization, argument splitting, and `sys_write`." | `hello from NanoBoot` |
| **6** | `echo "quoted argument"` | "Demonstrates quote-aware lexer stripping quotes and preserving strings." | `quoted argument` |
| **7** | `echo "unclosed quote` | "Demonstrates robust error handling for malformed input without kernel panic." | `syntax error: unclosed quote` |
| **8** | `isotest` | "Demonstrates Sv39 virtual memory boundary protection. Syscall tries to access kernel memory `0x80200000`; kernel rejects it safely." | `isolation PASS: kernel address rejected` |
| **9** | `ls` | "Queries the in-memory RAMFS via `sys_ls()`." | Lists `README.txt`, `hello.txt`, `design.txt`. |
| **10** | `cat hello.txt` | "Fetches file data across user/kernel boundary via `sys_cat()`." | `Hello from the NanoBoot in-memory filesystem!` |
| **11** | `sleep 3` | "Blocks execution for 3 timer intervals using monotonic hardware counter." | `sleeping...` followed by `awake`. |
| **12** | `forktest` | "Demonstrates complete process lifecycle: `fork()` clones address space, child runs with PID 2, exits with 42, parent `wait()` reaps zombie." | Parent creates child 2, child runs, child exits, `wait returned pid=2`. |
| **13** | `exit` | "Issues `sys_exit(0)` syscall and cleanly terminates shell." | `shell exiting` |

---

## 5. Comprehensive Viva Questions & Expert Answers

### Q1: What happens at the hardware level during an `ecall` from U-mode?
**Answer:** The RISC-V hardware sets `scause = 8` (`SCAUSE_ECALL_U`), copies the user program counter into `sepc`, sets `sstatus.SPP = 0` (indicating trap originated in U-mode), copies `sstatus.SIE` to `sstatus.SPIE`, sets `sstatus.SIE = 0` (disabling interrupts), and jumps to the address in `stvec` (`user_trap_entry`).

### Q2: Why is the `sscratch` register essential in `user_trap_entry`?
**Answer:** When entering supervisor mode from user mode, the kernel cannot trust user registers or the user stack pointer (`sp`). RISC-V provides `sscratch` as a dedicated CSR holding the kernel's per-process `trapframe_t` pointer. Using `csrrw a0, sscratch, a0`, the kernel atomically swaps user register `a0` with the trapframe pointer without corrupting any register.

### Q3: Why does `trap_handler()` add 4 to `sepc`?
**Answer:** On RISC-V, `ecall` is a 4-byte instruction. Unlike asynchronous interrupts where `sepc` points to the instruction to re-execute, an `ecall` trap records the address of the `ecall` itself. To prevent an infinite syscall trap loop, the kernel must advance `sepc += 4` so `sret` resumes execution at the instruction immediately following the `ecall`.

### Q4: How does Sv39 virtual memory address translation work?
**Answer:** Sv39 uses a 39-bit virtual address split into:
- `VPN[2]` (bits 38:30) — Level 2 page table index (root)
- `VPN[1]` (bits 29:21) — Level 1 page table index
- `VPN[0]` (bits 20:12) — Level 0 page table index (leaf)
- `Offset` (bits 11:0) — Byte offset in the 4 KB page.
The MMU walks the page tables using physical page numbers (PPN) stored in Page Table Entries (PTEs).

### Q5: How does NanoBoot prevent a malicious user program from reading kernel memory?
**Answer:** In the Sv39 page tables, kernel mappings do not have the `PTE_U` (User) flag set. If user code executes a load/store to a supervisor address, the hardware MMU generates a Load/Store Page Fault. Furthermore, all user pointer arguments to system calls are validated via `copyin()`/`copyout()` and `translate_user()`, which explicitly check for `PTE_U` and reject any kernel address before reading or writing.

### Q6: How does `fork()` clone a process without corrupting the parent's memory?
**Answer:** `proc_fork()` allocates a fresh process slot in `proc_table`. It calls `vm_clone_user()` which allocates a new Sv39 page directory and physical pages for the child, then copies the parent's memory contents page-by-page. The child's trapframe is duplicated, but its return register `a0` (`tf->regs[10]`) is explicitly set to `0`. The parent receives the child's PID.

### Q7: What is a zombie process and how does NanoBoot prevent zombie leaks?
**Answer:** A zombie is a process that has terminated via `exit()`, but its parent has not yet called `wait()` to read its exit status. NanoBoot marks the terminated child as `P_ZOMBIE`. When the parent invokes `proc_wait()`, the kernel retrieves the exit code, destroys the child's page tables via `vm_destroy()`, resets the child slot to `P_UNUSED`, and frees the PID for reuse.

### Q8: How does the shell lexer handle nested quotes or escaped characters?
**Answer:** The lexer uses an `in_quote` state variable tracking whether the current scan is inside single or double quotes. Characters inside quotes are appended verbatim to the token value, while outer quote characters are stripped. Unclosed quotes return `-1` with `syntax error: unclosed quote` before reaching the execution engine.

### Q9: Why is UART MMIO mapped into the user page table?
**Answer:** In NanoBoot, kernel code and UART0 (`0x10000000`) are mapped in the upper part of the page table without `PTE_U`. This allows supervisor trap handlers to continue accessing the serial port and kernel text without performing an expensive `satp` switch on every single trap.

### Q10: How is timer preemption handled?
**Answer:** The kernel sets timer deadlines via SBI `sbi_time_set()`. When the hardware timer fires, `scause = 0x8000000000000005` (`SCAUSE_STIMER`). The kernel increments tick counters, checks sleeping process deadlines, sets the running process to `P_RUNNABLE`, and context-switches to the next runnable process via `proc_schedule()`.

---

## 6. Honest Technical Limitations & Design Trade-Offs

| Subsystem | Implemented Behavior | Technical Limitation | Architectural Reason |
|---|---|---|---|
| **RAMFS** | In-memory read-only filesystem | No file modification or dynamic creation | Designed for embedded educational kernel without block device driver. |
| **Pipes (`\|`)** | Lexer/parser detects pipes; prints architectural message | No dynamic IPC pipe syscalls | Pipe buffers require kernel dynamic memory heaps and descriptor tables not in scope. |
| **Redirection (`>`)** | Parsed and validated syntax | No file writing redirection | Backed by read-only RAMFS; dynamic file creation not supported. |
| **Concurrency** | Single-hart round-robin scheduler | Multi-hart spinlock boot not enabled by default | Kept single-hart to ensure 100% deterministic, race-free viva demonstrations. |
| **Process Limit** | Fixed 8 slots (`NPROC = 8`) | Cannot exceed 8 concurrent processes | Static allocation guarantees zero runtime memory exhaustion or dynamic heap fragmentation. |

---
**Guide Certified by:** NanoBoot Systems Engineering Team (P. Sharan Teja, M. Krishnama Nayudu, M. Lokesh)  
**Date:** Academic Year 2026-27 | Evaluation Ready
