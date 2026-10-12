#!/usr/bin/env python3
"""Automated verification test for NanoBoot Lexer, Parser, Built-ins, and Kernel Syscalls."""
from __future__ import annotations

import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "tests" / "lexer_parser_transcript.txt"
QEMU = shutil.which("qemu-system-riscv64")

TEST_COMMANDS = [
    ("help", ["Commands:", "help", "getpid", "ps", "echo", "sleep"]),
    ("getpid", ["pid=1"]),
    ("ps", ["PID", "1", "RUNNING"]),
    ("echo hello world", ["hello world"]),
    ("echo \"hello world\"", ["hello world"]),
    ("echo 'hello world'", ["hello world"]),
    ("ls", ["Files:", "README.txt", "hello.txt", "design.txt"]),
    ("cat README.txt", ["NanoBoot RAMFS", "This file is stored in kernel memory."]),
    ("sleep 3", ["sleeping...", "awake"]),
    ("forktest", ["forktest: child running", "forktest: wait returned pid="]),
    ("schedtest", ["schedtest: round-robin interleaving PASS"]),
    ("echo \"unclosed quote", ["syntax error: unclosed quote"]),
    ("badcommand", ["unknown command; type 'help'"]),
    ("echo hello | cat", ["pipe: not supported by NanoBoot architecture (no pipe syscall)"]),
    ("echo hello > out.txt", ["redirection: not supported by NanoBoot architecture (read-only RAMFS)"]),
    ("echo test >", ["syntax error: missing file for redirection"]),
    ("echo test |", ["syntax error: missing command after '|'"]),
    ("sleep 1 &", ["background: not supported by NanoBoot architecture"]),
    ("isotest", ["isolation PASS: per-process Sv39 isolation verified"]),
    ("exit", ["shell exiting"]),
]

def wait_for(proc: subprocess.Popen[bytes], marker: bytes, timeout: float) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            return marker in OUT.read_bytes()
        try:
            if marker in OUT.read_bytes():
                return True
        except OSError:
            pass
        time.sleep(0.05)
    return False

def wait_for_prompt_count(proc: subprocess.Popen[bytes], old_count: int, timeout: float) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            return False
        try:
            data = OUT.read_bytes()
        except OSError:
            data = b""
        if data.count(b"nb$ ") > old_count:
            return True
        time.sleep(0.05)
    return False

def type_command(proc: subprocess.Popen[bytes], command: str) -> None:
    if proc.stdin is None:
        raise RuntimeError("QEMU stdin pipe is unavailable")
    for byte in (command + "\n").encode("ascii"):
        proc.stdin.write(bytes((byte,)))
        proc.stdin.flush()
        time.sleep(0.008)

def main() -> int:
    if QEMU is None:
        print("qemu-system-riscv64 not found.", file=sys.stderr)
        return 2

    OUT.parent.mkdir(parents=True, exist_ok=True)
    try:
        OUT.unlink()
    except FileNotFoundError:
        pass

    qemu_args = [
        QEMU,
        "-machine", "virt",
        "-nographic",
        "-bios", "default",
        "-kernel", "kernel.elf",
        "-m", "128M",
    ]

    print("=========================================================")
    print(" Running NanoBoot Comprehensive Shell & Syscall Test Suite ")
    print("=========================================================")

    passed = 0
    total = len(TEST_COMMANDS)

    with OUT.open("wb") as log:
        proc = subprocess.Popen(
            qemu_args,
            cwd=ROOT,
            stdin=subprocess.PIPE,
            stdout=log,
            stderr=subprocess.STDOUT,
            start_new_session=True,
        )
        try:
            if not wait_for(proc, b"nb$ ", 8.0):
                print("FAIL: kernel boot did not reach nb$ shell prompt", file=sys.stderr)
                return 1
            print("PASS: Kernel boot & reached user-mode shell (nb$)")

            for cmd, expected_markers in TEST_COMMANDS:
                current_prompts = OUT.read_bytes().count(b"nb$ ")
                before_len = OUT.stat().st_size
                type_command(proc, cmd)

                if cmd == "exit":
                    if not wait_for(proc, b"shell exiting", 5.0):
                        print(f"FAIL: exit command failed")
                        return 1
                    passed += 1
                    print(f"PASS [{passed}/{total}]: exit command")
                    break

                timeout = 8.0 if cmd in ("forktest", "schedtest", "sleep 3") else 5.0
                if not wait_for_prompt_count(proc, current_prompts, timeout):
                    print(f"FAIL: timeout waiting for prompt after '{cmd}'", file=sys.stderr)
                    return 1

                # Read output generated for this command
                data = OUT.read_bytes()[before_len:].decode("utf-8", errors="replace")
                missing = [m for m in expected_markers if m not in data]
                if missing:
                    print(f"FAIL: command '{cmd}' missing expected output: {missing}", file=sys.stderr)
                    print(f"Received snippet:\n{data}\n", file=sys.stderr)
                    return 1

                passed += 1
                print(f"PASS [{passed}/{total}]: '{cmd}' -> verified {len(expected_markers)} marker(s)")

            time.sleep(0.25)
            if proc.stdin is not None:
                proc.stdin.close()
            if proc.poll() is None:
                proc.terminate()
            try:
                proc.wait(timeout=2.0)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait(timeout=2.0)
        finally:
            if proc.poll() is None:
                proc.kill()
                proc.wait(timeout=2.0)

    print("\n---------------------------------------------------------")
    print(f"All {passed}/{total} NanoBoot Shell & Kernel Tests PASSED!")
    print(f"Transcript saved to {OUT.relative_to(ROOT)}")
    print("---------------------------------------------------------")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
