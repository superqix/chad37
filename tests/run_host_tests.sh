#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 Michael Wilson
# Runs the logic that needs no hardware with the computer's own C compiler.
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
cc -std=c99 -Wall -Wextra -Werror -fsanitize=address,undefined -g \
    -I"$root/app" -I"$root/tests" \
    "$root/app/easy_chord.c" "$root/tests/easy_chord_test.c" -o "$out/easy_chord_test"
"$out/easy_chord_test"

cc -std=c99 -Wall -Wextra -Werror -fsanitize=address,undefined -g \
    -I"$root/app" -I"$root/tests" \
    "$root/app/note_refs.c" "$root/tests/note_refs_test.c" -o "$out/note_refs_test"
"$out/note_refs_test"

cc -std=c99 -Wall -Wextra -Werror -fsanitize=address,undefined -g \
    -I"$root/app" -I"$root/tests" \
    "$root/app/easy_chord.c" "$root/app/note_refs.c" "$root/app/chord_scale.c" "$root/tests/chord_scale_test.c" \
    -o "$out/chord_scale_test"
"$out/chord_scale_test"

cc -std=c99 -Wall -Wextra -Werror -fsanitize=address,undefined -g \
    -DBOARD_SMK37=1 -I"$root/base/bsp/chip" -I"$root/base/bsp/boards/smk37" -I"$root/base/include" \
    "$root/base/tests/smk_trs_tx_test.c" -o "$out/smk_trs_tx_test"
"$out/smk_trs_tx_test"
