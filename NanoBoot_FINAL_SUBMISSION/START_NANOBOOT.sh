#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

need_setup=0
command -v qemu-system-riscv64 >/dev/null 2>&1 || need_setup=1
command -v riscv64-unknown-elf-gcc >/dev/null 2>&1 || need_setup=1
command -v riscv64-unknown-elf-ld >/dev/null 2>&1 || need_setup=1
command -v riscv64-unknown-elf-objcopy >/dev/null 2>&1 || need_setup=1
command -v riscv64-unknown-elf-objdump >/dev/null 2>&1 || need_setup=1
command -v file >/dev/null 2>&1 || need_setup=1
command -v python3 >/dev/null 2>&1 || need_setup=1

if [[ "$need_setup" -eq 1 ]]; then
    echo "NanoBoot: required Ubuntu tools are missing; installing them now."
    bash ./setup_ubuntu.sh
fi

mkdir -p tests
LOG=tests/verification-results.txt
: > "$LOG"
{
    make clean
    make
    make verify
    make disasm
    make user-disasm
    make qemu-test
} 2>&1 | tee -a "$LOG"

echo
echo "========================================"
echo " NanoBoot build + smoke checks passed."
echo " Serial transcript: tests/qemu-output.txt"
echo " Build/test log: tests/verification-results.txt"
echo " Starting the interactive QEMU shell."
echo "========================================"
echo
exec qemu-system-riscv64 -machine virt -nographic -bios default -kernel kernel.elf -m 128M
