#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

OUT=tests/qemu-output.txt
if ! command -v qemu-system-riscv64 >/dev/null 2>&1; then
  echo "ERROR: qemu-system-riscv64 not found. Run bash setup_ubuntu.sh first." >&2
  exit 1
fi
if ! command -v python3 >/dev/null 2>&1; then
  echo "ERROR: python3 not found. Run bash setup_ubuntu.sh first." >&2
  exit 1
fi

python3 tests/qemu_smoke.py
cat "$OUT"

assert_contains() {
  local marker="$1"
  local description="$2"
  if grep -Fq "$marker" "$OUT"; then
    echo "PASS: $description"
  else
    echo "FAIL: $description (missing marker: $marker)" >&2
    echo "Full transcript: $OUT" >&2
    exit 1
  fi
}

assert_sequence() {
  local description="$1"
  local needle="$2"
  if python3 - "$OUT" "$needle" <<'PYSEQ'
import sys
from pathlib import Path
path = Path(sys.argv[1])
data = path.read_bytes()
needle = sys.argv[2].encode()
if needle not in data:
    sys.exit(1)
PYSEQ
  then
    echo "PASS: $description"
  else
    echo "FAIL: $description (expected output sequence not found)" >&2
    echo "Full transcript: $OUT" >&2
    exit 1
  fi
}

assert_contains "NanoBoot - RISC-V Boot-to-Shell Kernel" "kernel boot banner"
assert_contains "Welcome to NanoBoot user shell" "user-mode shell reached"
assert_contains "Commands:" "help/system-call path"
assert_contains "PID" "process-table ps system call"
assert_sequence "write system call through echo command" $'echo hello from NanoBoot\r\nhello from NanoBoot'
assert_sequence "getpid system call from the user shell" $'getpid\r\npid=1'
assert_contains "Files:" "RAMFS ls system call"
assert_contains "Hello from the NanoBoot in-memory filesystem!" "RAMFS cat system call"
assert_contains "isolation PASS: kernel address rejected" "user/kernel address validation"
assert_contains "awake" "sleep/wakeup path"
assert_contains "forktest: child running" "fork child ran"
assert_contains "forktest: wait returned pid=" "wait returned child pid"

echo
echo "Smoke test PASS. Transcript saved to $OUT"
