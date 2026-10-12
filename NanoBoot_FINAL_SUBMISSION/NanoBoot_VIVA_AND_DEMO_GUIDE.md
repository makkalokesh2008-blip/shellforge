# NanoBoot — Viva & Faculty Demo Guide
## Rapid Q&A and Live Demonstration Cheatsheet

### 1. Project Overview & Architecture
- **Q: What is NanoBoot?**
  **A:** NanoBoot is an educational monolithic 64-bit operating system kernel written in C and RISC-V assembly. It boots from bare metal on QEMU `virt` via OpenSBI, establishes Sv39 virtual memory, configures supervisor trap handling, implements process scheduling with `fork`/`wait`/`exit`, and hosts an interactive user-mode shell with a custom lexer and parser.
- **Q: What privilege modes are used?**
  **A:** OpenSBI initializes M-mode (Machine mode) and hands over to NanoBoot in S-mode (Supervisor mode). The user shell runs in U-mode (User mode) at virtual address `0x40000000`.

### 2. Lexer, Parser & Shell Architecture
- **Q: Where is the shell implemented?**
  **A:** The shell is implemented in `user/sh.c`, running entirely in user mode (U-mode) as an isolated ELF binary (`user/sh.elf`) linked with `user/user.ld`.
- **Q: How does the shell handle quotes and whitespace?**
  **A:** `tokenize()` scans the input line character-by-character. Delimiters outside quotes terminate tokens. Within single (`'`) or double (`"`) quotes, whitespace and operator characters are preserved, and quotes are cleanly stripped into `token_t`. If an unclosed quote is encountered, the lexer returns a syntax error without executing.
- **Q: How are operators handled?**
  **A:** The lexer detects `|`, `<`, `>`, `>>`, and `&`. The parser validates syntax (e.g. ensuring a file follows `>`). If executed, the shell honestly informs the user that NanoBoot's minimal kernel does not implement pipes or writable filesystems, rather than crashing or faking functionality.

### 3. Virtual Memory & Isolation
- **Q: How is memory isolation enforced?**
  **A:** NanoBoot uses RISC-V Sv39 three-level page tables. Kernel mappings do not have the `PTE_U` (User) bit set. In `kernel/vm.c`, `translate_user()` validates all user pointers passed to syscalls; if a user passes a kernel address (e.g. `0x80200000`), the hardware and kernel copy routines reject the address. This is proven live by `isotest`.

### 4. Process Management
- **Q: How does `fork()` work in NanoBoot?**
  **A:** `proc_fork()` finds a free process slot in `proc_table`, copies the parent's user memory pages using `vm_clone_user()`, clones the trapframe, sets the child's return register `a0 = 0`, and returns the child's PID to the parent.
- **Q: How does `wait()` work?**
  **A:** If a child exists, `proc_wait()` sets the parent's state to `PROC_SLEEPING` and triggers `schedule()`. When the child calls `proc_exit()`, it transitions to `PROC_ZOMBIE`, which awakens the parent to reclaim the slot.

### 5. Live Demonstration Sequence for Faculty
Run: `bash START_NANOBOOT.sh` or `cd NanoBoot_FINAL_SUBMISSION && qemu-system-riscv64 -machine virt -nographic -bios default -kernel kernel.elf -m 128M`
Execute the following commands in order:
1. `help` (Shows available commands)
2. `getpid` (Demonstrates `ecall` syscall returning PID=1)
3. `ps` (Demonstrates kernel process table inspection)
4. `echo "hello world"` (Demonstrates quote-aware lexer & `argv` printing)
5. `isotest` (Demonstrates Sv39 virtual memory boundary protection)
6. `ls` (Demonstrates RAMFS directory listing)
7. `cat README.txt` (Demonstrates RAMFS file reading)
8. `sleep 3` (Demonstrates timer tick synchronization)
9. `forktest` (Demonstrates concurrent child process creation, execution, and wait reaping)
10. `exit` (Demonstrates clean shell termination)
