#!/usr/bin/env python3
"""Drive NanoBoot through its serial prompt and save a deterministic transcript."""
from __future__ import annotations

import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "tests" / "qemu-output.txt"
COMMANDS = (ROOT / "tests" / "demo_commands.txt").read_text(encoding="utf-8").splitlines()
QEMU = shutil.which("qemu-system-riscv64")


def fail(message: str, proc: subprocess.Popen[bytes] | None = None) -> int:
    print(f"QEMU smoke test FAILED: {message}", file=sys.stderr)
    if OUT.exists():
        print("--- last serial output ---", file=sys.stderr)
        try:
            print(OUT.read_text(encoding="utf-8", errors="replace")[-6000:], file=sys.stderr)
        except OSError:
            pass
    if proc is not None and proc.poll() is None:
        try:
            proc.terminate()
            proc.wait(timeout=2)
        except (OSError, subprocess.TimeoutExpired):
            proc.kill()
    return 1


def wait_for(proc: subprocess.Popen[bytes], marker: bytes, timeout: float, description: str) -> bool:
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


def wait_for_prompt_count(proc: subprocess.Popen[bytes], old_count: int, timeout: float, command: str) -> bool:
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
    print(f"Timed out waiting for shell prompt after command: {command}", file=sys.stderr)
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
        print("qemu-system-riscv64 not found. Run bash setup_ubuntu.sh first.", file=sys.stderr)
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
            if not wait_for(proc, b"nb$ ", 8.0, "initial shell prompt"):
                return fail("kernel did not reach the user shell prompt", proc)

            for command in COMMANDS:
                current = OUT.read_bytes().count(b"nb$ ")
                type_command(proc, command)
                if command == "exit":
                    if not wait_for(proc, b"shell exiting", 5.0, "shell exit message"):
                        return fail("shell did not handle the exit command", proc)
                    break
                timeout = 8.0 if command == "forktest" else 5.0
                if not wait_for_prompt_count(proc, current, timeout, command):
                    return fail(f"no prompt after command {command!r}", proc)

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

    print(f"Serial transcript saved to {OUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
