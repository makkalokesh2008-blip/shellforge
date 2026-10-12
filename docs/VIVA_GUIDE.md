# ShellForge + NanoBoot — Viva / Demo Guide
## Faculty: Dr. T. SelvamuthuKumar | Academic Year 2026-27

---

## PART A — ShellForge Viva (Unix Shell in C)

### Q: What is ShellForge?
ShellForge is a Unix-style interactive shell written in C. It implements its own lexer, parser, built-in command dispatcher, external command executor (using fork/exec), I/O redirection, multi-stage pipelines, variable expansion, and background job control.

### Q: How does the shell work end-to-end?
1. `readline()` reads one line from the user (with history support).
2. The **Lexer** (`src/lexer.c`) breaks the line into tokens: WORD, PIPE, REDIRECT_IN, REDIRECT_OUT, REDIRECT_APPEND, AMPERSAND, END.
3. The **Parser** (`src/parser.c`) groups tokens into a `Pipeline` of `Command` structs, filling `argv[]`, `input`, `output`, `append`, and `background`.
4. **Variable expansion** (`src/expand.c`) replaces `$VAR` / `${VAR}` with `getenv()` values.
5. The **dispatcher** in `src/main.c` checks if the single command is a built-in; if so, runs it in-process. Otherwise calls `execute_pipeline()`.
6. The **Executor** (`src/executor.c`) forks child processes, sets up pipes with `pipe(2)` + `dup2(2)`, opens redirection files with `open(2)`, and calls `execvp(2)` in each child. The parent calls `waitpid(2)` for foreground, or adds to the job table for background.

### Q: What system calls does ShellForge use?
| System call | Where used |
|---|---|
| `fork(2)` | Create child for every external command |
| `execvp(2)` | Replace child with the external program |
| `waitpid(2)` | Parent waits for foreground child |
| `pipe(2)` | Create read/write end for pipeline |
| `dup2(2)` | Wire pipe ends and files onto stdin/stdout |
| `open(2)` | Open input/output redirection files |
| `chdir(2)` | Implement `cd` built-in |
| `getcwd(3)` | Implement `pwd` built-in |
| `getenv(3)` | Variable expansion |
| `kill(2)` | Send SIGCONT for `fg`/`bg` |
| `setpgid(2)` | Place each pipeline in its own process group |
| `signal(2)` | SIGCHLD handler to reap background jobs |

### Q: Why does `cd` have to be a built-in?
`cd` changes the working directory of the **current** process. If `cd` were forked as a child, `chdir()` would change the child's directory and the parent (shell) would remain unchanged. Only by running in the shell process does `chdir()` affect subsequent commands.

### Q: Why did `help` previously fail with "Permission denied"?
The original code's built-in dispatcher did not handle `help`. When `help` was not recognised, it fell through to `execute_pipeline()`, which called `execvp("help", ...)`. `help` is a shell keyword — there is no executable called `help` in `$PATH`, so `execvp` fails with EACCES (Permission denied). The fix: add `help` to `builtin_execute()` before the executor is called.

### Q: How do pipelines work?
For `cmd1 | cmd2 | cmd3`:
1. For each adjacent pair, `pipe(2)` creates a (read_fd, write_fd) pair.
2. Each child `fork()`s, then uses `dup2()` to wire:
   - The previous pipe's read_fd → stdin
   - The current pipe's write_fd → stdout
3. All children `execvp()` simultaneously; output flows left to right through memory buffers managed by the kernel.
4. Parent calls `waitpid()` for each child in order.

### Q: What is variable expansion?
`src/expand.c` scans each token for `$VAR` or `${VAR}` and calls `getenv()`. Unrecognised `$` is preserved literally. Expansion runs after parsing but before execution.

### Q: How is quoting handled?
The lexer (`src/lexer.c`) detects a leading `'` or `"`, advances past the opening quote, collects characters until the matching closing quote, and returns them as a single `TOKEN_WORD` — without the quote characters. This means `echo 'hello world'` produces a single argv element `hello world`.

---

## PART B — NanoBoot Viva (RISC-V OS)

### Q: What is NanoBoot?
NanoBoot is a minimal RISC-V supervisor-mode OS kernel that boots on QEMU's `virt` machine (via OpenSBI firmware), handles traps and timer interrupts, exposes 10 system calls, schedules multiple processes in round-robin, enforces user/kernel isolation with Sv39 page tables, provides an in-memory filesystem (RAMFS), and runs a user-mode shell.

### Q: Explain the boot sequence.
1. QEMU starts → OpenSBI (machine-mode firmware) initialises hardware.
2. OpenSBI hands control to `kernel.elf` at 0x80200000 in S-mode.
3. `kernel/boot.S` sets up the kernel stack and jumps to `main()` in C.
4. `main()` initialises UART, Sv39 VM, process table, trap vector (`stvec`), and the SBI timer.
5. The user shell image (embedded as `user/sh_blob.o`) is loaded into process 1's Sv39 address space.
6. `enter_user()` sets `sepc` = shell entry, clears `SPP` (so `sret` goes to user mode), and executes `sret` — the CPU now runs the shell in user mode.

### Q: What is an `ecall` and how is it handled?
`ecall` is the RISC-V user-to-supervisor trap instruction. When user code executes `ecall`:
1. CPU saves `sepc` = next instruction, sets `scause` = 8 (user ecall).
2. CPU jumps to `stvec` → `user_trap_entry` in `kernel/entry.S`.
3. `entry.S` saves all 32 registers into the process's `trapframe`.
4. `trap_handler()` in `kernel/trap.c` reads `scause`, sees it is a user ecall.
5. `sys_call()` in `kernel/syscall.c` reads `a7` (syscall number) and dispatches.
6. Return value is written to `a0` in the trapframe.
7. `sret` restores registers from trapframe and resumes user code.

### Q: What is Sv39 and why is it needed?
Sv39 is RISC-V's 39-bit virtual address translation scheme with three levels of page tables. Each user process gets its own root page table. Kernel pages are mapped supervisor-only (U-bit = 0); user pages are mapped with U-bit = 1. The hardware's MMU enforces that user code cannot access supervisor-only pages. This is demonstrated by `isotest`: the shell tries to read a kernel address; the kernel's `copyin()` validation rejects it before the hardware even needs to trigger a fault.

### Q: How does `fork` work in NanoBoot?
`do_fork()` in `kernel/proc.c`:
1. Finds a free process slot.
2. Copies the parent's Sv39 user pages into the new process slot's memory.
3. Copies the parent's trapframe (so the child sees the same register state as the parent after the ecall).
4. Sets the child's `a0` (return value of fork) to 0; sets the parent's return to the child's PID.
5. Marks the child RUNNABLE. The scheduler will pick it up on the next timer interrupt.

### Q: Difference between a real kernel system call and a shell built-in?
- **Shell built-in** (e.g., ShellForge's `cd`, `help`): code runs inside the shell process itself; no new process is created.
- **Kernel system call** (e.g., NanoBoot's `getpid`, `fork`): user code executes `ecall`; the CPU traps to the kernel; the kernel runs the handler in supervisor mode and returns via `sret`. This crosses the user/kernel privilege boundary.

---

## Live Demo Sequence

### ShellForge Demo (at WSL terminal)
```
cd /home/lokesh/shellforge-push
./shellforge

help                          # shows built-in list
echo hello world              # built-in echo
pwd                           # built-in pwd
cd /tmp && pwd                # cd changes dir; pwd confirms
echo hello world | wc -w      # pipeline: 2
echo line1 > /tmp/demo.txt    # output redirect
echo line2 >> /tmp/demo.txt   # append redirect
cat < /tmp/demo.txt           # input redirect: shows line1 line2
echo $HOME                    # variable expansion
uname -a                      # external command
exit
```

### NanoBoot Demo (at WSL terminal)
```
cd /home/lokesh/shellforge-push/NanoBoot_FINAL_SUBMISSION
bash run_prebuilt.sh

help                          # shell built-in, shows commands
getpid                        # ecall syscall 5 -> pid=1
ps                            # ecall syscall 7 -> process table
echo hello from NanoBoot      # ecall syscall 1 (write)
isotest                       # copyin validation -> isolation PASS
ls                            # ecall syscall 8 -> RAMFS file list
cat README.txt                # ecall syscall 9 -> file contents
sleep 3                       # ecall syscall 6 -> sleeping.../awake
forktest                      # ecall syscall 2 (fork) + 4 (wait)
exit                          # ecall syscall 3
# Press Ctrl+A then X to close QEMU
```
