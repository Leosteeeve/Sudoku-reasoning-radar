#!/usr/bin/env python3
"""Collect MSYS2 UCRT64 DLL dependencies for a Windows release folder."""

from __future__ import annotations

import argparse
import datetime as _dt
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


SYSTEM_DLL_NAMES = {
    "advapi32.dll",
    "bcrypt.dll",
    "bcryptprimitives.dll",
    "cfgmgr32.dll",
    "combase.dll",
    "comctl32.dll",
    "comdlg32.dll",
    "crypt32.dll",
    "cryptbase.dll",
    "dwmapi.dll",
    "dwrite.dll",
    "dxcore.dll",
    "gdi32.dll",
    "gdi32full.dll",
    "glu32.dll",
    "imm32.dll",
    "iphlpapi.dll",
    "kernel32.dll",
    "kernelbase.dll",
    "msvcp_win.dll",
    "msvcrt.dll",
    "ntdll.dll",
    "ole32.dll",
    "oleaut32.dll",
    "opengl32.dll",
    "rpcrt4.dll",
    "sechost.dll",
    "secur32.dll",
    "setupapi.dll",
    "shell32.dll",
    "shlwapi.dll",
    "sspicli.dll",
    "ucrtbase.dll",
    "user32.dll",
    "usp10.dll",
    "version.dll",
    "windows.storage.dll",
    "win32u.dll",
    "winmm.dll",
    "wldap32.dll",
    "ws2_32.dll",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Recursively copy MSYS2 DLL dependencies.")
    parser.add_argument("--target", required=True, help="Release executable to scan.")
    parser.add_argument("--output", required=True, help="Release folder that receives DLLs.")
    parser.add_argument("--msys-bin", required=True, help="MSYS2 UCRT64 bin directory.")
    parser.add_argument("--ntldd", required=True, help="Path to ntldd.exe.")
    return parser.parse_args()


def normalized(path: Path) -> str:
    return str(path.resolve(strict=False)).replace("/", "\\").lower()


def dll_key(name: str) -> str:
    return Path(name.strip()).name.lower()


def clean_ntldd_path(raw: str) -> str:
    value = raw.strip().strip('"')
    value = re.sub(r"\s+\(0x[0-9a-fA-F]+\)\s*$", "", value).strip()
    return value


def msys_style_to_windows(raw: str, msys_bin: Path) -> Path | None:
    value = raw.replace("\\", "/")
    lower = value.lower()
    if lower.startswith("/ucrt64/bin/"):
        return msys_bin / value.split("/")[-1]
    if re.match(r"^/[a-z]/", value, flags=re.IGNORECASE):
        drive = value[1].upper()
        rest = value[2:].replace("/", "\\")
        return Path(f"{drive}:{rest}")
    return None


def parse_ntldd_line(line: str) -> tuple[str, str | None, bool] | None:
    text = line.strip()
    if not text or "=>" not in text:
        return None
    left, right = text.split("=>", 1)
    name = dll_key(left)
    target = clean_ntldd_path(right)
    if not name.endswith(".dll"):
        return None
    missing = target.lower() == "not found" or "not found" in target.lower()
    return name, None if missing else target, missing


def is_system_dll(name: str, target: Path | None) -> bool:
    lower_name = name.lower()
    if lower_name.startswith("api-ms-win-") or lower_name.startswith("ext-ms-"):
        return True
    if lower_name in SYSTEM_DLL_NAMES:
        return True
    if target is not None:
        text = normalized(target)
        if "\\windows\\system32\\" in text or "\\windows\\syswow64\\" in text or "\\windows\\winsxs\\" in text:
            return True
    return False


def build_msys_index(msys_bin: Path) -> dict[str, Path]:
    index: dict[str, Path] = {}
    for dll in msys_bin.glob("*.dll"):
        index[dll.name.lower()] = dll
    return index


def run_ntldd(ntldd: Path, binary: Path, env: dict[str, str]) -> list[str]:
    result = subprocess.run(
        [str(ntldd), "--recursive", "--search-dir", env["MSYS2_DLL_SEARCH_DIR"], str(binary)],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        env=env,
        check=False,
    )
    if result.returncode != 0:
        print(f"WARNING: ntldd returned {result.returncode} while scanning {binary.name}", file=sys.stderr)
    return result.stdout.splitlines()


def copy_msys_dll(src: Path, output: Path, copied: dict[str, Path]) -> Path:
    dst = output / src.name
    if src.name.lower() not in copied or not dst.exists():
        shutil.copy2(src, dst)
        copied[src.name.lower()] = src
    return dst


def should_report_missing(name: str, msys_index: dict[str, Path]) -> bool:
    lower = name.lower()
    if is_system_dll(lower, None):
        return False
    if lower in msys_index:
        return True
    third_party_prefixes = (
        "lib",
        "sdl",
        "opencv",
        "tesseract",
        "leptonica",
        "zlib",
    )
    return lower.startswith(third_party_prefixes)


def main() -> int:
    args = parse_args()
    target = Path(args.target).resolve(strict=False)
    output = Path(args.output).resolve(strict=False)
    msys_bin = Path(args.msys_bin).resolve(strict=False)
    ntldd = Path(args.ntldd).resolve(strict=False)

    if not target.exists():
        print(f"ERROR: target does not exist: {target}")
        return 2
    if not msys_bin.exists():
        print(f"ERROR: MSYS2 bin folder does not exist: {msys_bin}")
        return 2
    if not ntldd.exists():
        print("Missing ntldd.")
        print("Install it in MSYS2 UCRT64:")
        print("pacman -S --needed mingw-w64-ucrt-x86_64-ntldd")
        return 2

    output.mkdir(parents=True, exist_ok=True)
    msys_index = build_msys_index(msys_bin)
    msys_prefix = normalized(msys_bin) + "\\"

    env = os.environ.copy()
    system_path = os.environ.get("SystemRoot", r"C:\Windows")
    env["MSYS2_DLL_SEARCH_DIR"] = str(msys_bin)
    env["PATH"] = os.pathsep.join(
        [
            str(output),
            str(msys_bin),
            str(Path(system_path) / "System32"),
            system_path,
            str(Path(system_path) / "System32" / "Wbem"),
        ]
    )

    copied: dict[str, Path] = {}
    skipped: dict[str, str] = {}
    missing: dict[str, str] = {}

    for line in run_ntldd(ntldd, target, env):
        parsed = parse_ntldd_line(line)
        if parsed is None:
            continue
        name, raw_target, not_found = parsed

        target_path: Path | None = None
        if raw_target:
            target_path = msys_style_to_windows(raw_target, msys_bin) or Path(raw_target)

        if is_system_dll(name, target_path):
            skipped.setdefault(name, raw_target or "system")
            continue

        src: Path | None = None
        if target_path is not None:
            target_norm = normalized(target_path)
            if target_norm.startswith(msys_prefix) and target_path.exists():
                src = target_path

        if src is None and name in msys_index:
            src = msys_index[name]

        if src is not None:
            copy_msys_dll(src, output, copied)
            continue

        if not_found and should_report_missing(name, msys_index):
            missing.setdefault(name, f"required by {target.name} (ntldd: not found)")

    report_path = output / "dependency_report.txt"
    missing_path = output / "dependency_missing.txt"
    timestamp = _dt.datetime.now().isoformat(timespec="seconds")

    with report_path.open("w", encoding="utf-8", newline="\n") as report:
        report.write("Sudoku Reasoning Radar dependency report\n")
        report.write(f"timestamp: {timestamp}\n")
        report.write(f"scan root executable: {target}\n")
        report.write(f"output folder: {output}\n")
        report.write(f"MSYS2 bin path: {msys_bin}\n")
        report.write(f"ntldd path: {ntldd}\n")
        report.write(f"copied DLL count: {len(copied)}\n")
        report.write(f"skipped system DLL count: {len(skipped)}\n")
        report.write(f"missing DLL count: {len(missing)}\n\n")

        report.write("Copied DLLs:\n")
        for name in sorted(copied):
            report.write(f"  {name} <- {copied[name]}\n")

        report.write("\nSkipped system DLLs:\n")
        for name in sorted(skipped):
            report.write(f"  {name} ({skipped[name]})\n")

        report.write("\nMissing DLLs:\n")
        if missing:
            for name in sorted(missing):
                report.write(f"  {name}: {missing[name]}\n")
        else:
            report.write("  none\n")

        report.write("\nScan mode:\n")
        report.write("  ntldd --recursive\n")

    with missing_path.open("w", encoding="utf-8", newline="\n") as miss:
        for name in sorted(missing):
            miss.write(f"{name}: {missing[name]}\n")

    print(f"Copied DLL count: {len(copied)}")
    print(f"Missing DLL count: {len(missing)}")
    print(f"Report: {report_path}")
    if missing:
        print(f"WARNING: Missing DLLs were written to {missing_path}")
        return 1
    print("No missing non-system DLLs.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
