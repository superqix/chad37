#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 Michael Wilson
# Chad37's tests with no hardware: the chord logic, then the app in the host simulator.
#   app/tests/run_tests.sh
set -eu
cd "$(dirname "$0")/../.."
PY="${PYTHON:-python3}"
sh tests/run_host_tests.sh
BOARD=smk37 "$PY" base/tools/build.py app --host
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
for script in app/tests/*.txt; do
    app/build/smk37/host/chad37-smk37_host "$script" "$out"
done
echo "app tests: ok"
