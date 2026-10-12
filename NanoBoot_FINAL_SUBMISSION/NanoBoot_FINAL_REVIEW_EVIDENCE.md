# NanoBoot — Final Review Evidence & Evaluation Report
## Academic Year 2026-27 | Operating Systems Final Submission
### Students: P. Sharan Teja (2500032333) · M. Krishnama Nayudu (2500031918) · M. Lokesh (2500030720)
**Primary Project:** NanoBoot (RISC-V 64-bit Operating System & User Shell)  
**Submission Repository:** `https://github.com/makkalokesh2008-blip/shellforge.git`  
**Location:** `NanoBoot_FINAL_SUBMISSION/`

---

> ### Rubric Mapping & SLM Status Note
> The repository documents do not contain the raw SLM PDF wording. The eight points below represent a complete, rigorous **Four Deliverable Points + Four Evaluation Points** mapping derived from the NanoBoot operating system architecture, feature matrix, and internal review corrections. Each point is backed by genuine source code, exact reproduction commands, and deterministic verification results.

---

## Executive Summary of Verification Results

| Component | Target / Environment | Build Status | Automated Test Result | Verification Status |
|---|---|---|---|---|
| **Kernel & Boot** | RISC-V 64 (`virt`, QEMU) | PASS (`-Wall -Wextra -Werror`) | 12/12 Smoke Checks PASS | Verified Live |
| **Shell Lexer/Parser** | NanoBoot User Shell (`0x40000000`) | PASS (Binary: 6372 B / 16 KB limit) | 19/19 Regression Suite PASS | Verified Live |
| **Process & VM** | Sv39 Page Tables & Round-Robin | PASS | Memory Isolation PASS | Verified Live |

---

# SECTION 1: FOUR DELIVERABLE POINTS

---

### Deliverable Point 1: RISC-V Boot Process, UART Initialization, and Kernel Setup
1. **Rubric / Review Point:**
   Implementation of RISC-V 64 machine-to-supervisor handoff, OpenSBI integration, UART 16550 serial console driver, and kernel memory/bss zeroing.
2. **Implementation & Source Files:**
   - `kernel/boot.S`: Boot entry point `_start`, clearing `.bss` and `.sbss`, configuring initial stack `boot_stack_top`.
   - `kernel/uart.c` & `kernel/uart.h`: UART 16550 console driver supporting 8N1 transmission and polled receive.
   - `kernel/main.c`: Kernel bootstrap sequence (`uart_init()`, `vm_init()`, `trap_init()`, `proc_init()`, `ramfs_init()`).
3. **Faculty Explanation:**
   When QEMU powers on, OpenSBI initializes M-mode and jumps to the kernel's entry point in `kernel/boot.S` in S-mode at address `0x80200000`. The boot assembly zeros uninitialized kernel data, allocates a kernel stack, and transfers execution to C in `main()`. The kernel initializes the serial UART, memory paging, trap vectors, process table, and spawns Process 1 (`sh.elf`) before entering the round-robin scheduler.
4. **Exact Reproduction Command:**
   ```bash
   qemu-system-riscv64 -machine virt -nographic -bios default -kernel kernel.elf -m 128M
   ```
5. **Actual Verification Result:**
   ```text
   OpenSBI v1.3 ...
   ========================================
    NanoBoot - RISC-V Boot-to-Shell Kernel
   ========================================
   boot: supervisor mode / QEMU virt
   vm  : Sv39 per-process page tables
   trap: stvec + ecall + timer interrupt
   sched: fixed table / round-robin
   init: pid=1 ready
   Welcome to NanoBoot user shell.
   Type 'help' for commands.
   nb$ 
   ```
6. **Screenshot Reference:**
   - Filename: `screenshots/01_boot_and_kernel_init.png`
   - Visible content: OpenSBI banner, NanoBoot boot banner, `init: pid=1 ready`, and the interactive `nb$` prompt.

---

### Deliverable Point 2: User Shell Lexer and Tokenizer
1. **Rubric / Review Point:**
   Implementation of a discrete, standalone tokenizer/lexer in NanoBoot's user shell handling words, operators, whitespace, single/double quotes, and malformed quote errors.
2. **Implementation & Source Files:**
   - `user/sh.c`: Token enumeration `token_type_t`, `token_t` structure, and `tokenize()` function.
3. **Faculty Explanation:**
   Previously, the shell relied on primitive string prefix matching. The new lexer parses the input stream character-by-character into discrete tokens (`TOK_WORD`, `TOK_PIPE`, `TOK_REDIR_IN`, `TOK_REDIR_OUT`, `TOK_REDIR_APPEND`, `TOK_AMPERSAND`). It strips surrounding quotes from single (`'...'`) and double (`"..."`) quoted strings, treats whitespace outside quotes as delimiters, and detects unclosed quotes, returning clean syntax errors without crashing. Memory allocation is completely static with fixed buffers (zero heap dependency).
4. **Exact Reproduction Command:**
   At the `nb$` prompt:
   ```text
   echo "hello world"
   echo 'single quote test'
   echo "unclosed quote
   ```
5. **Actual Verification Result:**
   ```text
   nb$ echo "hello world"
   hello world
   nb$ echo 'single quote test'
   single quote test
   nb$ echo "unclosed quote
   syntax error: unclosed quote
   nb$ 
   ```
6. **Screenshot Reference:**
   - Filename: `screenshots/02_shell_lexer_tokenization.png`
   - Visible content: Stripped quotes for double and single quotes, followed by the syntax error recovery line for unclosed quotes.

---

### Deliverable Point 3: User Shell Parser and Built-in Execution Engine
1. **Rubric / Review Point:**
   Conversion of tokens into a structured command (`command_t` / `argv[]`), syntax validation, error recovery, and robust dispatch of built-in commands.
2. **Implementation & Source Files:**
   - `user/sh.c`: `command_t` structure, `parse_command()`, `execute_command()`, and built-in routines (`help`, `echo`, `sleep`, `exit`).
3. **Faculty Explanation:**
   The parser transforms the token array into a structured command object containing `argc`, `argv[]`, redirection targets (`redir_in`, `redir_out`, `append`), pipeline flags, and background flags. It performs syntactic validation (e.g., verifying that redirection operators have file targets and that pipes are followed by commands). Built-in execution inspects `argv[0]` rather than whole-line strings, allowing proper argument passing. If unsupported features (like kernel pipes) are typed, the shell cleanly reports the architectural limitation rather than crashing.
4. **Exact Reproduction Command:**
   At the `nb$` prompt:
   ```text
   help
   echo hello from NanoBoot
   echo test >
   badcommand
   ```
5. **Actual Verification Result:**
   ```text
   nb$ help
   Commands:
     help                 show this help
     getpid               show current process id
     ps                   show process table
     echo TEXT            print TEXT
     sleep N              sleep for N timer ticks
     forktest             demonstrate fork/wait/exit
     ls                   list RAMFS files
     cat FILE             print RAMFS file
     isotest              verify kernel/user memory isolation
     exit                 terminate this shell
   nb$ echo hello from NanoBoot
   hello from NanoBoot
   nb$ echo test >
   syntax error: missing file for redirection
   nb$ badcommand
   unknown command; type 'help'
   nb$ 
   ```
6. **Screenshot Reference:**
   - Filename: `screenshots/03_shell_parser_and_builtins.png`
   - Visible content: `help` command menu, `echo` argument output, syntax error message on missing redirection target, and unknown command handler.

---

### Deliverable Point 4: System Call Interface and Privilege Transition
1. **Rubric / Review Point:**
   User-to-kernel privilege transition via RISC-V `ecall`, register state preservation via `trapframe`, syscall dispatch table, and user memory pointer validation (`copyin`/`copyout`).
2. **Implementation & Source Files:**
   - `user/syscall.S`: Macro-generated RISC-V assembly wrappers placing syscall ID in `a7` and issuing `ecall`.
   - `kernel/entry.S`: `user_trap_entry` saving all 32 general-purpose registers to per-process `trapframe_t`.
   - `kernel/trap.c`: `trap_handler()` reading `scause`, handling `CAUSE_USER_ECALL` (cause 8), advancing `sepc += 4`.
   - `kernel/syscall.c`: `syscall_dispatch()` validating inputs and routing to `sys_read`, `sys_write`, `sys_fork`, `sys_getpid`, `sys_ps`, etc.
   - `kernel/vm.c`: `copyin()`, `copyout()`, `copyinstr()` validating that user virtual addresses reside in valid `PTE_U` pages.
3. **Faculty Explanation:**
   When user code invokes `sys_getpid()`, register `a7` is set to 5 and `ecall` executes. The hardware elevates CPU privilege to Supervisor mode and jumps to `stvec` (`user_trap_entry`), which preserves user registers on the process kernel stack. The kernel dispatches syscall 5, writes the result to `tf->regs[10]` (register `a0`), advances `sepc` past `ecall`, and executes `sret` to resume user mode.
4. **Exact Reproduction Command:**
   At the `nb$` prompt:
   ```text
   getpid
   ps
   ```
5. **Actual Verification Result:**
   ```text
   nb$ getpid
   pid=1
   nb$ ps
   PID	STATE
   1	RUNNING
   nb$ 
   ```
6. **Screenshot Reference:**
   - Filename: `screenshots/04_syscall_dispatch_and_trap.png`
   - Visible content: `pid=1` output from `sys_getpid` and the process table header and running state from `sys_ps`.

---

# SECTION 2: FOUR EVALUATION POINTS

---

### Evaluation Point 5: Process Lifecycle Management (`fork`, `exit`, `wait`, `ps`)
1. **Rubric / Review Point:**
   Implementation and demonstration of process duplication (`fork`), termination (`exit`), parent reaping (`wait`), and state inspection (`ps`).
2. **Implementation & Source Files:**
   - `kernel/proc.c`: `proc_fork()` (copies user address space and trapframe), `proc_exit()` (marks process `PROC_ZOMBIE`), `proc_wait()` (blocks parent until child state change), and `proc_ps()`.
   - `kernel/swtch.S`: Low-level context switch `swtch()`.
   - `user/sh.c`: `forktest()` routine.
3. **Faculty Explanation:**
   `proc_fork()` allocates a new process control block, copies the parent's user memory pages via `vm_clone_user()`, copies the trapframe with child return value `a0 = 0`, and marks the child `PROC_RUNNABLE`. The child executes, prints its PID, sleeps, and terminates via `sys_exit(42)`. The parent calls `sys_wait()`, transitioning to `PROC_SLEEPING` until the child exits. The parent reaps the zombie child and prints the reaped PID.
4. **Exact Reproduction Command:**
   At the `nb$` prompt:
   ```text
   forktest
   ```
5. **Actual Verification Result:**
   ```text
   nb$ forktest
   forktest: parent created child pid=2, waiting...
   forktest: child running, pid=2
   forktest: child exiting
   forktest: wait returned pid=2
   nb$ 
   ```
6. **Screenshot Reference:**
   - Filename: `screenshots/05_process_management_fork_wait_exit.png`
   - Visible content: Parent creating child pid=2, child executing, child exiting, and parent wait returning child pid=2.

---

### Evaluation Point 6: Memory Protection and Sv39 Virtual Memory Isolation
1. **Rubric / Review Point:**
   Three-level Sv39 page table implementation, distinct kernel and user address mappings, and rejection of supervisor-only memory access attempts.
2. **Implementation & Source Files:**
   - `kernel/vm.c`: `vm_create_user()`, `mappages()`, `walk_user_pte()`, and `translate_user()`.
   - `user/sh.c`: `isotest` command.
3. **Faculty Explanation:**
   NanoBoot enforces strict hardware and software memory isolation using RISC-V Sv39 virtual memory. Kernel memory (`0x80000000+`) is mapped with supervisor-only permissions (lacking the `PTE_U` user bit). User code and data are mapped at `0x40000000` with `PTE_U`. In `kernel/vm.c`, `translate_user()` validates all syscall pointers: if a user process attempts to read or write a supervisor address, `PTE_U` is missing, causing `copyin`/`copyout` to fail and return `-1`.
4. **Exact Reproduction Command:**
   At the `nb$` prompt:
   ```text
   isotest
   ```
5. **Actual Verification Result:**
   ```text
   nb$ isotest
   isolation PASS: kernel address rejected
   nb$ 
   ```
6. **Screenshot Reference:**
   - Filename: `screenshots/06_memory_isolation_sv39.png`
   - Visible content: `isolation PASS: kernel address rejected` verifying hardware/software memory boundary protection.

---

### Evaluation Point 7: Timer Interrupts, Preemption, and Sleep/Wakeup Synchronization
1. **Rubric / Review Point:**
   Programming the RISC-V timer via SBI, supervisor timer interrupt dispatch, round-robin time slicing, and sleep/wakeup synchronization.
2. **Implementation & Source Files:**
   - `kernel/timer.c`: `timer_init()`, `sbi_set_timer()`, timer tick counter `r_time()`.
   - `kernel/trap.c`: `CAUSE_SUPERVISOR_TIMER` handling and preemption dispatch.
   - `kernel/syscall.c`: `sys_sleep()`.
   - `user/sh.c`: `sleep N` built-in command.
3. **Faculty Explanation:**
   The kernel configures the timer through SBI calls (`sbi_set_timer`), scheduling interrupts at fixed intervals (`TIMER_TICKS`). When an interrupt fires, `trap_handler()` updates the system tick count and re-arms the timer. The `sleep N` system call blocks execution for `N` timer ticks using the hardware time counter (`r_time()`), preserving process state and ensuring predictable, deterministic wakeups.
4. **Exact Reproduction Command:**
   At the `nb$` prompt:
   ```text
   sleep 3
   ```
5. **Actual Verification Result:**
   ```text
   nb$ sleep 3
   sleeping...
   awake
   nb$ 
   ```
6. **Screenshot Reference:**
   - Filename: `screenshots/07_timer_and_sleep_scheduling.png`
   - Visible content: `sleeping...` followed by `awake` after the timer tick interval expires.

---

### Evaluation Point 8: RAMFS In-Memory Filesystem and End-to-End User Experience
1. **Rubric / Review Point:**
   Implementation of an in-memory read-only filesystem (RAMFS), system calls `sys_ls` and `sys_cat`, and reliable end-to-end user shell interaction.
2. **Implementation & Source Files:**
   - `kernel/ramfs.c` & `kernel/ramfs.h`: In-memory file table (`README.txt`, `hello.txt`, `design.txt`), `ramfs_ls()`, `ramfs_cat()`.
   - `kernel/syscall.c`: `sys_ls()` (syscall 8), `sys_cat()` (syscall 9).
   - `user/sh.c`: `ls`, `cat FILE`, `exit` commands.
3. **Faculty Explanation:**
   NanoBoot embeds a lightweight in-memory filesystem in the kernel image. User commands invoke `sys_ls()` to iterate through directory entries and `sys_cat()` to copy and print file contents. Together with the interactive line editor, lexer, parser, process manager, and memory protection, this delivers a complete, cohesive operating system demonstration suitable for faculty review.
4. **Exact Reproduction Command:**
   At the `nb$` prompt:
   ```text
   ls
   cat README.txt
   cat hello.txt
   exit
   ```
5. **Actual Verification Result:**
   ```text
   nb$ ls
   Files:
     README.txt
     hello.txt
     design.txt
   nb$ cat README.txt
   NanoBoot RAMFS
   This file is stored in kernel memory.
   nb$ cat hello.txt
   Hello from the NanoBoot in-memory filesystem!
   nb$ exit
   shell exiting
   ```
6. **Screenshot Reference:**
   - Filename: `screenshots/08_ramfs_and_end_to_end_shell.png`
   - Visible content: `ls` directory listing, `cat README.txt` and `cat hello.txt` contents, and clean shell exit message.

---

# Verification Logs Summary

- Build Log: `tests/final_build.log` (0 errors, 0 warnings with `-Wall -Wextra -Werror`).
- Smoke Test Log: `tests/final_qemu_smoke.log` (12/12 PASS).
- Shell & Lexer Regression Log: `tests/lexer_parser_transcript.txt` (19/19 PASS).
