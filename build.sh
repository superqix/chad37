#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# Build Chad37 for the M-Vave SMK-37 (Pro, Elite, MKE-P37) with fm1-base (base/), and the packages into
# build/chad37/ with their SHA256SUMS. Linux or WSL.
#   ./build.sh                    development builds: chad37-smk37{,-elite,-mke}.fwsc
#   ./build.sh --release 0.1      a release: chad37-smk37{,-elite,-mke}-0.1.fwsc, plus the installer, for a GitHub
#                                 release's downloads
#   JIELI_TOOLCHAIN  JieLi Linux toolchain (default: ~/.jieli/toolchain)
#   AC79_SDK         JieLi AC79 SDK checkout (default: ~/fw-AC79_AIoT_SDK)
#   PYTHON           a Python 3 with Pillow (and libraqm)
set -e
cd "$(dirname "$0")"
export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}"
export AC79_SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
PY="${PYTHON:-python3}"
V=""
prev=""
for a in "$@"; do
    [ "$prev" = "--release" ] && V="-$a"
    prev="$a"
done
for m in pro elite mke; do
    BOARD=smk37 MODEL=$m "$PY" base/tools/build.py app "$@"
done
BOARD=smk37 "$PY" base/tools/build.py app --host
mkdir -p build/chad37
rm -f build/chad37/*.fwsc build/chad37/fm1_install.py build/chad37/SHA256SUMS
# only this build's packages (the build directories also keep older names and versions)
cp "app/build/smk37/chad37-smk37$V.fwsc" "app/build/smk37-elite/chad37-smk37-elite$V.fwsc" \
   "app/build/smk37-mke/chad37-smk37-mke$V.fwsc" build/chad37/
[ -n "$V" ] && cp base/tools/fm1_install.py build/chad37/
(cd build/chad37 && sha256sum * > SHA256SUMS && cat SHA256SUMS)
