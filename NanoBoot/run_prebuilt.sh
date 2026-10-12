#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
if ! command -v qemu-system-riscv64 >/dev/null 2>&1; then
  echo "qemu-system-riscv64 is not installed. Run ./setup_ubuntu.sh first." >&2
  exit 1
fi
qemu-system-riscv64 -machine virt -nographic -bios default -kernel kernel.elf -m 128M
