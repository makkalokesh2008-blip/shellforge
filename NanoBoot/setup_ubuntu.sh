#!/usr/bin/env bash
set -euo pipefail

if [[ "$(id -u)" -eq 0 ]]; then
  APT=""
else
  APT="sudo"
fi

${APT:+$APT }apt-get update
${APT:+$APT }apt-get install -y \
  build-essential \
  make \
  gcc-riscv64-unknown-elf \
  binutils-riscv64-unknown-elf \
  qemu-system-misc \
  file \
  python3

echo
echo "NanoBoot toolchain setup complete."
echo "Next: make clean && make && make qemu"
