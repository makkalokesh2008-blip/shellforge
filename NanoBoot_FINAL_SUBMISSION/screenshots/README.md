# NanoBoot Demonstration Screenshots Guide

This directory holds the eight demonstration screenshots required for the Final Evaluation.
Because NanoBoot runs in a headless terminal (QEMU virt serial console), each screenshot must be captured from a running terminal session showing the exact command and kernel response.

## Screenshot Index & Checklist

| # | Filename | Review / Rubric Point | Command to Execute | What Must Be Visible | Status |
|---|---|---|---|---|---|
| 1 | `01_boot_and_kernel_init.png` | Point 1: Boot & Kernel Initialization | `bash START_NANOBOOT.sh` | OpenSBI banner, NanoBoot boot banner, `init: pid=1 ready`, `nb$` prompt | Ready to capture |
| 2 | `02_shell_lexer_tokenization.png` | Point 2: User Shell Lexer / Tokenizer | `echo "hello world"`, `echo 'test'`, `echo "unclosed` | Word tokens, stripped quotes, and `syntax error: unclosed quote` | Ready to capture |
| 3 | `03_shell_parser_and_builtins.png` | Point 3: Shell Parser & Built-ins | `help`, `echo hello from NanoBoot`, `badcmd` | Help menu, echo argv output, unknown command message | Ready to capture |
| 4 | `04_syscall_dispatch_and_trap.png` | Point 4: Syscall Dispatch & Trap Handling | `getpid`, `ps` | `pid=1` and process table with PID and STATE | Ready to capture |
| 5 | `05_process_management_fork_wait_exit.png` | Point 5: Process Lifecycle (`fork`/`wait`/`exit`) | `forktest` | Parent created child pid=2, child running, child exiting, wait returned pid=2 | Ready to capture |
| 6 | `06_memory_isolation_sv39.png` | Point 6: Sv39 Memory Protection & Isolation | `isotest` | `isolation PASS: kernel address rejected` | Ready to capture |
| 7 | `07_timer_and_sleep_scheduling.png` | Point 7: Timer, Sleep & Scheduling | `sleep 3` | `sleeping...` then `awake` after 3 timer ticks | Ready to capture |
| 8 | `08_ramfs_and_end_to_end_shell.png` | Point 8: RAMFS & End-to-End Operation | `ls`, `cat README.txt`, `exit` | Directory listing, file content, and `shell exiting` | Ready to capture |

## Capture Instructions
1. Run `./START_NANOBOOT.sh` or `qemu-system-riscv64 -machine virt -nographic -bios default -kernel kernel.elf -m 128M`.
2. Type each command listed in the table above.
3. Use your system screenshot utility (e.g. `Win + Shift + S` on Windows, or `gnome-screenshot` on Linux) to capture the terminal window.
4. Save the screenshot with the corresponding filename in this `screenshots/` directory.
