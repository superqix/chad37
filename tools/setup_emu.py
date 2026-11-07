#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 Michael Wilson
# Stages tools/fm1-emu/web/dist/ with the built Chordpad37 .fwsc packages as the default SMK-37 firmware,
# and optionally starts a local HTTP server at http://localhost:8642/smk37/.
import argparse
import functools
import http.server
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EMU_WEB = ROOT / "tools" / "fm1-emu" / "web"
DIST = EMU_WEB / "dist"
DIST_SMK = DIST / "smk37"


def stage_dist() -> Path:
    wasm = EMU_WEB / "fm1_web.wasm"
    if not wasm.exists():
        raise SystemExit(f"setup_emu: missing {wasm}")

    DIST.mkdir(parents=True, exist_ok=True)
    DIST_SMK.mkdir(parents=True, exist_ok=True)

    root_files = [
        "index.html", "style.css", "main.js", "persistence.js", "tryout.js",
        "worklet.js", "jit.js", "fwsc.js", "firmwares.json", "fm1-emu-report.pdf", "og.png",
    ]
    for name in root_files:
        src = EMU_WEB / name
        if src.exists():
            shutil.copy2(src, DIST / name)
    shutil.copy2(wasm, DIST / "fm1_web.wasm")

    smk_shared = [
        "style.css", "main.js", "persistence.js", "tryout.js",
        "worklet.js", "jit.js", "fwsc.js",
    ]
    for name in smk_shared:
        shutil.copy2(EMU_WEB / name, DIST_SMK / name)
    for name in ("index.html", "og.png"):
        src = EMU_WEB / "smk37" / name
        if src.exists():
            shutil.copy2(src, DIST_SMK / name)
    shutil.copy2(wasm, DIST_SMK / "fm1_web.wasm")

    # Copy built Chordpad37 .fwsc packages into dist/smk37/
    packages = [
        ("chordpad", "Chordpad37 (SMK-37 Pro)", ROOT / "app/build/smk37/chordpad-smk37.fwsc"),
        ("chordpad-elite", "Chordpad37 (SMK-37 Elite)", ROOT / "app/build/smk37-elite/chordpad-smk37-elite.fwsc"),
        ("chordpad-mke", "Chordpad37 (MKE-P37)", ROOT / "app/build/smk37-mke/chordpad-smk37-mke.fwsc"),
    ]
    entries = []
    for fw_id, title, pkg_path in packages:
        if pkg_path.exists():
            shutil.copy2(pkg_path, DIST_SMK / pkg_path.name)
            entries.append({
                "id": fw_id,
                "name": title,
                "version": "dev",
                "author": "Michael Wilson",
                "license": "MPL-2.0",
                "about": (
                    "Chordpad37 custom controller firmware for the M-VAVE SMK-37: "
                    "4-state CHORD x SCALE mode matrix, Easy Chord + manual chord detection, "
                    "SEQ REC pad assignment, diatonic scale pads, auto-strum, arpeggiator, and 8 presets."
                ),
                "repo": "https://github.com/no2games/chordpad37",
                "package": f"./{pkg_path.name}",
            })

    cat_src = json.loads((EMU_WEB / "smk37" / "firmwares.json").read_text(encoding="utf-8"))
    existing = [f for f in cat_src.get("firmwares", []) if not f.get("id", "").startswith("chordpad")]
    cat_out = {
        "note": cat_src.get("note", ""),
        "default": entries[0]["id"] if entries else cat_src.get("default", "snoop37"),
        "firmwares": entries + existing,
    }
    (DIST_SMK / "firmwares.json").write_text(json.dumps(cat_out, indent=1) + "\n", encoding="utf-8")
    return DIST


def main() -> None:
    ap = argparse.ArgumentParser(description="Stage and optionally serve the fm1-emu SMK-37 web emulator")
    ap.add_argument("--serve", action="store_true", help="start HTTP server after staging")
    ap.add_argument("--port", type=int, default=8642, help="HTTP port (default: 8642)")
    args = ap.parse_args()

    dist = stage_dist()
    print(f"staged   {dist / 'smk37'} (default firmware: chordpad-smk37.fwsc)")
    if args.serve:
        handler = functools.partial(http.server.SimpleHTTPRequestHandler, directory=str(dist))
        with http.server.ThreadingHTTPServer(("127.0.0.1", args.port), handler) as httpd:
            print(f"serving  http://localhost:{args.port}/smk37/?fw=chordpad")
            httpd.serve_forever()


if __name__ == "__main__":
    main()
