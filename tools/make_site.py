#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 Michael Wilson
"""Make Chad37's web installer (the GitHub Pages site) from a release's packages:

  index.html     the installer (site/index.html, with fm1-base's web/fm1pkg.js and web/fm1ota.js and the packages'
                 metadata inlined; Chrome or Edge, Web MIDI)
  logo.png       docs/chad37-logo.png
  firmware/      the release's packages, which the installer downloads: it picks the one for the keyboard it finds

  tools/make_site.py PKG_DIR VERSION OUT_DIR

PKG_DIR holds the release's chad37-smk37{,-elite,-mke}-VERSION.fwsc (./build.sh --release VERSION: build/chad37/, or a
GitHub release's downloads). Each must be a release build with the update loader in it; its identity (SMK-37 Elite_5XXYYZZ)
is what the installer checks the download against, and what the keyboard reports after the install. Standard library
only, so the Pages workflow (.github/workflows/pages.yml) runs it as is."""
import hashlib
import json
import re
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPO = "https://github.com/superqix/chad37"
MODELS = {"smk37": "SMK-37 Pro", "smk37-elite": "SMK-37 Elite", "smk37-mke": "MKE-P37"}   # package name -> model
BLOCKS, BLK, KEEP = 20, 0x30, 0x2F


def strip_module(src):
    """an ES module's text as plain script: no export, no import (to inline it in the page)"""
    src = re.sub(r"^export\s+", "", src, flags=re.M)
    return re.sub(r"^import .*?;\n", "", src, flags=re.M)


def product_of(raw):
    """the package identity: one marker byte after each of the first 20 blocks (fm1pkg.js productOf)"""
    return "".join(chr((m - i - 1) & 0xFF) for i in range(BLOCKS) if (m := raw[i * BLK + KEEP]) != 0x7D)


def release_id(version):
    """0.1 -> 000100, 1.2.3 -> 010203: the six digits after a release's 5 (fm1.toml smk_id_prefix)"""
    parts = version.split(".")
    if not 1 <= len(parts) <= 3 or not all(p.isdigit() and int(p) < 100 for p in parts):
        raise SystemExit(f"version {version!r}: X.Y or X.Y.Z, each below 100")
    return "".join(f"{int(p):02d}" for p in (parts + ["0", "0"])[:3])


def main(pkg_dir, version, out):
    pkg_dir, out = Path(pkg_dir), Path(out)
    want = "5" + release_id(version)
    models = []
    if out.exists():
        shutil.rmtree(out)
    (out / "firmware").mkdir(parents=True)
    for stem, model in MODELS.items():
        name = f"chad37-{stem}-{version}.fwsc"
        pkg = pkg_dir / name
        if not pkg.exists():
            raise SystemExit(f"no {pkg}")
        raw = pkg.read_bytes()
        product = product_of(raw)
        if product != f"{model}_{want}":
            raise SystemExit(f"{pkg}: identity {product!r}, expected {model}_{want} (a release build of {version}?)")
        if b"FELUCCA-LOADER-1" not in raw:
            raise SystemExit(f"{pkg}: no update loader in it")
        shutil.copy(pkg, out / "firmware" / name)
        models.append({"model": model, "product": product, "pkg": "firmware/" + name,
                       "sha256": hashlib.sha256(raw).hexdigest()})
    html = (ROOT / "site" / "index.html").read_text(encoding="utf-8")
    lib = "\n".join(strip_module((ROOT / "base" / "web" / f).read_text(encoding="utf-8"))
                    for f in ("fm1pkg.js", "fm1ota.js"))
    meta = json.dumps({"version": version, "models": models}, indent=1)
    for mark in ("/*LIB*/", "/*META*/"):
        if html.count(mark) != 1:
            raise SystemExit(f"site/index.html must contain {mark} once")
    for key, val in (("VERSION", version), ("REPO", REPO), ("REPO_SHORT", re.sub(r"^https?://", "", REPO))):
        html = html.replace("{{" + key + "}}", val)
    if "{{" in html:
        raise SystemExit("site/index.html: a {{...}} left unfilled")
    (out / "index.html").write_text(html.replace("/*LIB*/", lib).replace("/*META*/", meta), encoding="utf-8")
    shutil.copy(ROOT / "docs" / "chad37-logo.png", out / "logo.png")
    (out / ".nojekyll").write_text("")
    print(f"site: {out}: index.html, logo.png, firmware/ ({', '.join(m['product'] for m in models)})")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    main(*sys.argv[1:4])
