# NanoBoot — RISC-V Boot-to-Shell Kernel

A compact educational operating-system project targeting the QEMU `virt` machine on RISC-V 64. The project follows an xv6-style structure while keeping the code intentionally small enough for an undergraduate OS demonstration.

## Implemented project scope

- QEMU `virt` boot with OpenSBI firmware into RISC-V supervisor mode.
- UART console output and keyboard input over the QEMU virtual 16550 UART.
- RISC-V supervisor trap handling using `stvec`, `sstatus`, `sepc`, and `scause`.
- User-mode `ecall` system-call path.
- System calls: `read`, `write`, `fork`, `exit`, `wait`, `getpid`, `sleep`, `ps`, `ls`, `cat`.
- Fixed-size process table and round-robin scheduling.
- Timer interrupts through the SBI TIME extension.
- Sleep/wakeup coordination.
- Per-process Sv39 page tables with user/kernel isolation.
- User-mode shell with commands: `help`, `getpid`, `ps`, `echo`, `sleep`, `forktest`, `ls`, `cat`, `exit`.
- Tiny in-memory RAMFS containing demo files.
- Kernel spinlock primitive for the single-hart coordination layer.
- Context-switch assembly primitive (`swtch.S`) included as the kernel context-switch building block.
- Automated QEMU demo/test script.

## Directory layout

```text
NanoBoot/
├── Makefile
├── README.md
├── setup_ubuntu.sh
├── run_demo.sh
├── linker.ld
├── include/
│   ├── common.h
│   ├── param.h
│   ├── riscv.h
│   ├── spinlock.h
│   ├── trapframe.h
│   └── sysnum.h
├── kernel/
│   ├── boot.S
│   ├── entry.S
│   ├── main.c
│   ├── uart.c/.h
│   ├── string.c/.h
│   ├── spinlock.c
│   ├── vm.c/.h
│   ├── proc.c/.h
│   ├── syscall.c/.h
│   ├── trap.c/.h
│   ├── timer.c/.h
│   ├── ramfs.c/.h
│   └── swtch.S
├── user/
│   ├── user.ld
│   ├── syscall.S
│   └── sh.c
├── tests/
│   ├── demo_commands.txt
│   ├── qemu_smoke.py
│   └── test.sh
└── docs/
    ├── PROJECT_REPORT.md
    ├── DEMO_SCRIPT.md
    ├── FEATURE_MATRIX.md
    ├── EVIDENCE_GUIDE.md
    ├── VERIFICATION_REPORT.md
    ├── WEEKLY_BUILD_LOG.md
    ├── VIVA_QA.md
    └── SUBMISSION_CHECKLIST.md
```

## One-command Ubuntu start

After extracting the project in Ubuntu/WSL, change into the extracted folder and run:

```bash
bash START_NANOBOOT.sh
```

This checks for the required toolchain, installs the Ubuntu packages when needed, performs a clean build, runs static verification, executes the scripted QEMU smoke test, saves its transcript to `tests/qemu-output.txt`, and then opens the interactive QEMU serial console. The first setup needs working package-repository access and may ask for your Ubuntu password for `sudo`.

## Ubuntu setup

From an Ubuntu terminal:

```bash
cd NanoBoot
chmod +x setup_ubuntu.sh run_demo.sh tests/test.sh
bash setup_ubuntu.sh
```

The setup script installs the RISC-V bare-metal cross compiler, binutils and QEMU package used by this project.

## Build

```bash
make clean
make
```

## Run

The package includes rebuilt RISC-V artifacts as a convenience. After QEMU is installed, run:

```bash
bash run_prebuilt.sh
```

Or rebuild everything and run:

```bash
make clean
make
make qemu
```

The QEMU serial console is displayed directly in the Ubuntu terminal.

## Automated smoke demo

```bash
make qemu-test
```

This feeds a fixed set of shell commands to QEMU and saves the console transcript to `tests/qemu-output.txt`.

## Expected shell commands

```text
help
getpid
ps
echo hello from NanoBoot
isotest
ls
cat README.txt
cat hello.txt
sleep 3
forktest
schedtest
exit
```


## Automated Regression Test Suite

Run the comprehensive 20-test shell and syscall regression suite:

```bash
python3 tests/test_lexer_parser.py
```

This verifies boot, command dispatch, quote handling, whitespace, argument parsing, syntax error handling, isolation, scheduling, and graceful handling of unsupported operators.

## Architectural Notes & Limitations

- **RISC-V Boot, UART, Traps, Syscalls**: PASS (verified on QEMU virt)
- **Process Lifecycle (fork, exit, wait, ps)**: PASS (verified)
- **Sv39 Virtual Memory & Isolation**: PASS (verified via isotest)
- **Timer, Preemption, Sleep/Wakeup**: PASS (verified via sleep 3 and schedtest)
- **RAMFS File Operations (ls, cat)**: PASS (verified)
- **Shell Lexer & Parser**: PASS (20/20 regression tests PASS)
- **Shell Pipes**: PARTIAL (parsed with diagnostic; no fd table / pipe syscall in kernel)
- **I/O Redirection**: PARTIAL (parsed and validated; RAMFS is read-only)
- **Background Execution**: PARTIAL (parsed with syntax validation; single console UART)
- **Copy-on-Write (COW) Fork**: NOT IMPLEMENTED (direct physical page copy via vm_clone_user)
- **Multi-Hart (SMP)**: NOT IMPLEMENTED (Hart 0 single-core target)

## Verification

Run `make verify` after installing the toolchain. It checks the RISC-V ELF, embedded user image, size limits, boot/trap/BSS symbols, and the user entry address. `make disasm` and `make user-disasm` produce kernel and user-shell disassemblies. Run `make qemu-test` for the live emulator smoke test.

## Notes

This project intentionally uses OpenSBI as the standard RISC-V machine-mode firmware supplied by QEMU. NanoBoot itself runs in supervisor mode and implements the OS functionality described in the project specification. The timer path uses the SBI TIME extension, which is the standard way for an S-mode educational kernel to program the QEMU timer without implementing a second machine-mode firmware layer.
