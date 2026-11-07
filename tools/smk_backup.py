#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 Michael Wilson
"""A full flash backup of an SMK-37 in the chip's mask-ROM bootloader (the USB disk "WL82 UBOOT"), read twice
with jl-uboot-tool (https://github.com/kagaimiq/jl-uboot-tool) and checked. Only reads: nothing is written.
After the method in fm1-to-smk37's docs/02-update-and-recovery.md, section 3 (Method A).

  smk_backup.py [--device DEV] [--tool JLUBOOTTOOL.py] [--out DIR]
  smk_backup.py --verify DUMP1.bin DUMP2.bin

Get there first: base/tools/fm1_install.py PACKAGE.fwsc --check-only --uboot. Run this with the Python that has
jl-uboot-tool's requirements, as administrator on Windows (raw SCSI on \\\\.\\PhysicalDriveN) or root on Linux
(/dev/sgN). Without --device, Windows finds the disk by its name; Linux needs --device.

Checks: both reads 1 MiB, the same SHA-256, "trimdat" (the calibration) at 0xC1224, and a flash id the board
accepts (0xEB6014 or the FM-1's Puya 0x856014: FL_JEDEC_OK in base/bsp/boards/smk37/fm1_flash_map.h; another id is
reported, not refused).
The dump holds this unit's own calibration, Bluetooth pairing and key data: keep it out of the repository, in two
places. Exit codes: 0 good, 1 a check failed, 2 bad arguments, 3 no UBOOT disk, 4 jl-uboot-tool failed.
"""
import argparse
import ctypes
import datetime
import hashlib
import json
import re
import subprocess
import sys
import threading
from pathlib import Path

SIZE = 0x100000
TRIM_AT, TRIM = 0xC1224, b"trimdat"
FLASH_IDS = (0xEB6014, 0x856014)
TOOL = Path.home() / "tools" / "jl-uboot-tool" / "jluboottool.py"


def check(a, b):
    """-> (problems, sha256 of a); the same checks as the port kit's step 6"""
    da, db = Path(a).read_bytes(), Path(b).read_bytes()
    bad = [f"{p.name}: {len(d)} bytes, not {SIZE}" for p, d in ((Path(a), da), (Path(b), db)) if len(d) != SIZE]
    ha, hb = hashlib.sha256(da).hexdigest(), hashlib.sha256(db).hexdigest()
    if ha != hb:
        bad.append(f"the two reads differ (SHA-256 {ha[:16]}... and {hb[:16]}...)")
    if da[TRIM_AT:TRIM_AT + len(TRIM)] != TRIM:
        bad.append(f"no {TRIM.decode()!r} at {TRIM_AT:#x} (found {da[TRIM_AT:TRIM_AT + len(TRIM)]!r})")
    return bad, ha


def find_disk():
    """Windows: the one disk whose name has UBOOT in it -> \\\\.\\PhysicalDriveN"""
    ps = ("Get-Disk | Where-Object { $_.FriendlyName -match 'UBOOT' } | "
          "Select-Object Number, FriendlyName | ConvertTo-Json -Compress")
    r = subprocess.run(["powershell", "-NoProfile", "-Command", ps], capture_output=True, text=True)
    if r.returncode:
        fail(3, f"Get-Disk failed (exit {r.returncode}): {r.stderr.strip() or 'give --device'}")
    try:
        found = json.loads(r.stdout) if r.stdout.strip() else []
    except json.JSONDecodeError as e:
        fail(3, f"could not parse Get-Disk output ({e}); give --device")
    found = [found] if isinstance(found, dict) else found
    if not found:
        fail(3, "no disk named UBOOT: is the keyboard in the mask-ROM bootloader (fm1_install.py ... --check-only --uboot)?")
    if len(found) > 1:
        fail(2, f"more than one UBOOT disk: {found}; give --device")
    print(f"disk: {found[0]['FriendlyName']} (disk {found[0]['Number']})")
    return f"\\\\.\\PhysicalDrive{found[0]['Number']}"


def is_admin():
    try:
        return bool(ctypes.windll.shell32.IsUserAnAdmin())
    except (AttributeError, OSError):
        return False


def read(tool, device, dest, timeout):
    """one full read, its output shown as it comes (tqdm's progress is on stderr); -> the flash id the tool
    printed (or None)"""
    dest = Path(dest)
    stage = Path(tool).parent / f".{dest.name}.part" if any(c.isspace() for c in str(dest)) else None
    arg_dest = stage.name if stage else str(dest)
    cmd = [sys.executable, "-u", str(tool), "--device", device, "--chip", "wl82", f"read 0 {SIZE:#x} {arg_dest}"]
    print("$ " + " ".join(cmd), flush=True)
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, cwd=Path(tool).parent)
    out = []

    def tee():
        for chunk in iter(lambda: p.stdout.read1(256), b""):
            out.append(chunk)
            sys.stdout.buffer.write(chunk)
            sys.stdout.flush()

    t = threading.Thread(target=tee, daemon=True)
    t.start()
    try:
        p.wait(timeout)
    except subprocess.TimeoutExpired:
        p.kill()
        if stage and stage.exists():
            stage.unlink()
        fail(4, f"jl-uboot-tool gave nothing back in {timeout} s: power-cycle the keyboard, "
                "send the key again (fm1_install.py --uboot) and run this again")
    t.join(2)
    if stage and stage.exists():
        stage.replace(dest)
    if p.returncode or not dest.exists():
        fail(4, f"jl-uboot-tool failed (exit {p.returncode})")
    m = re.search(r"ID:\s*0x([0-9a-fA-F]+)", b"".join(out).decode(errors="replace"))
    return int(m[1], 16) if m else None


def fail(code, msg):
    print(f"error: {msg}", file=sys.stderr)
    sys.exit(code)


def report(a, b):
    bad, sha = check(a, b)
    for p in bad:
        print(f"FAIL {p}")
    if not bad:
        print(f"ok   both reads {SIZE} bytes, SHA-256 {sha}, {TRIM.decode()!r} at {TRIM_AT:#x}")
    return bad, sha


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--device", help=r"the UBOOT disk: \\.\PhysicalDriveN (Windows) or /dev/sgN (Linux)")
    ap.add_argument("--tool", type=Path, default=TOOL, help=f"jluboottool.py (default {TOOL})")
    ap.add_argument("--out", type=Path, help="folder for the dumps (default ~/smk37-backups/DATE-TIME)")
    ap.add_argument("--verify", nargs=2, metavar="DUMP", help="only check two existing dumps")
    ap.add_argument("--timeout", type=int, default=300, help="seconds allowed for one read (default %(default)s)")
    a = ap.parse_args(argv)
    if a.verify:
        return 1 if report(*a.verify)[0] else 0
    if not a.tool.exists():
        fail(2, f"no jl-uboot-tool at {a.tool} (--tool)")
    if sys.platform == "win32" and not is_admin():
        fail(2, "not an administrator shell: the raw disk reads hang without it. "
                "Open PowerShell with 'Run as administrator' (its title bar says Administrator)")
    device = a.device or (find_disk() if sys.platform == "win32" else fail(2, "give --device /dev/sgN"))
    out = a.out or Path.home() / "smk37-backups" / datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    out.mkdir(parents=True, exist_ok=True)
    dumps = [out / "smk37-dump-1.bin", out / "smk37-dump-2.bin"]
    if any(d.exists() for d in dumps):
        fail(2, f"{out} already holds a dump: give another --out")
    ids = [read(a.tool, device, d, a.timeout) for d in dumps]
    bad, sha = report(*dumps)
    if ids[0] != ids[1]:
        msg = f"the two reads reported different flash ids ({ids[0]!r} vs {ids[1]!r})"
        print(f"FAIL {msg}")
        bad.append(msg)
    fid = ids[0] if ids[0] is not None else ids[1]
    if fid is None:
        print("note: the flash id was not in jl-uboot-tool's output")
    elif fid not in FLASH_IDS:
        print(f"note: flash id {fid:#08x}, not one the board accepts ({', '.join(f'{i:#08x}' for i in FLASH_IDS)}): "
              "tell the maintainer before installing")
    else:
        print(f"ok   flash id {fid:#08x}")
    (out / "SHA256SUMS").write_text("".join(f"{hashlib.sha256(d.read_bytes()).hexdigest()}  {d.name}\n" for d in dumps))
    (out / "README.txt").write_text(
        f"SMK-37 full flash backup, {datetime.datetime.now():%Y-%m-%d %H:%M}, read twice with jl-uboot-tool.\n"
        f"Flash id: {f'{fid:#08x}' if fid is not None else 'not printed'}. Checks: {'all passed' if not bad else '; '.join(bad)}.\n"
        "Unit-unique: 0xC0000-0xC1FFF (Bluetooth pairing, calibration) and 0xFF000-0xFFFFF (key_mac).\n"
        "Keep two copies, out of any public repository.\n")
    print(f"dumps in {out}")
    if bad:
        print("do not trust these dumps: read again")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
