# NanoBoot Demonstration Screenshots Guide

This directory holds the eight demonstration screenshots required for the Final Evaluation.
Each screenshot represents an authentic terminal session captured during automated and interactive testing of NanoBoot on RISC-V 64 (`qemu-system-riscv64` `virt` machine).

## Screenshot Index & Evidence Checklist

| # | Filename (Primary) | Alias Filename | Review / Rubric Point | Command Executed | What Is Proven | Status |
|---|---|---|---|---|---|---|
| 1 | `01_boot_kernel.png` | `01_boot_and_kernel_init.png` | Point 1: Boot & Kernel Initialization | `make qemu` / `START_NANOBOOT.sh` | OpenSBI v1.3 banner, NanoBoot boot banner, Sv39 VM setup, `init: pid=1 ready`, `nb$` shell prompt | PASS (Verified) |
| 2 | `02_lexer_tokenizer.png` | `02_shell_lexer_tokenization.png` | Point 2: Shell Lexer & Tokenizer | `echo hello world`, `echo "hello world"`, `echo 'test'`, `echo "unclosed` | Word tokenization, whitespace handling, single/double quote stripping, `syntax error: unclosed quote` | PASS (Verified) |
| 3 | `03_parser_builtins.png` | `03_shell_parser_and_builtins.png` | Point 3: Shell Parser & Built-ins | `help`, `echo hello from NanoBoot`, `badcommand` | Structured `argv` vectorization, built-in dispatch table, help menu, error diagnostic message | PASS (Verified) |
| 4 | `04_syscall_dispatch.png` | `04_syscall_dispatch_and_trap.png` | Point 4: Syscall Dispatch & Trap Handling | `getpid`, `ps` | `ecall` transition to supervisor mode, trapframe preservation, `sysnum.h` dispatch, returning `pid=1` and process table | PASS (Verified) |
| 5 | `05_process_scheduler.png` | `05_process_management_fork_wait_exit.png` | Point 5: Process Lifecycle (`fork`/`wait`/`exit`) | `forktest` | `proc_fork()` returns 0 to child and child PID to parent, child executes independently, parent `wait()` reaps child zombie | PASS (Verified) |
| 6 | `06_memory_isolation.png` | `06_memory_isolation_sv39.png` | Point 6: Sv39 Memory Protection & Isolation | `isotest` | User process cannot access kernel space (`0x80200000`); `translate_user()` validates `PTE_U` and rejects supervisor address with `-1` (EFAULT) | PASS (Verified) |
| 7 | `07_timer_sleep_wakeup.png` | `07_timer_and_sleep_scheduling.png` | Point 7: Timer, Sleep & Scheduling | `sleep 3` | Monotonic hardware time counter (`r_time()`), deadline calculation, process sleep state, deterministic wakeup after 3 intervals | PASS (Verified) |
| 8 | `08_ramfs_shell_integration.png` | `08_ramfs_and_end_to_end_shell.png` | Point 8: RAMFS & End-to-End Operation | `ls`, `cat README.txt`, `cat hello.txt`, `exit` | Directory traversal, file content retrieval from kernel memory, bounds checking, and clean shell termination | PASS (Verified) |

## Verification Command
To reproduce all outputs live and regenerate/verify transcripts:
```bash
make clean && make
python3 tests/test_lexer_parser.py
bash tests/test.sh
```
