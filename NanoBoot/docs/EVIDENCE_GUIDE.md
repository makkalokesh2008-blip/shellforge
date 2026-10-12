# End-Product Evidence Guide

For a project review, present the actual build artifacts and live test output rather than screenshots of source code alone.

## Build artifacts

- `kernel.elf`: linked RISC-V ELF image loaded by QEMU.
- `kernel.bin`: flat binary converted from the kernel ELF.
- `user/sh.elf`: linked user shell, with `_start` verified at `0x40000000`.
- `user/sh.bin`: user image embedded into the kernel.
- `kernel.disasm`: generated kernel machine-code disassembly.
- `user/sh.disasm`: generated user-shell disassembly, including system-call `ecall` instructions.

## Runtime evidence

`bash START_NANOBOOT.sh` runs the scripted smoke test before opening the interactive console. The test must report `PASS` for the boot banner, user shell, `getpid`, RAMFS `ls`/`cat`, isolation, sleep/wakeup, fork, and wait. The serial output is saved as `tests/qemu-output.txt`.

If the script stops on a `FAIL`, do not show the report as completed runtime verification. Keep `tests/qemu-output.txt` and the terminal error so the failing path can be repaired.
