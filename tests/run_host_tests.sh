#!/bin/sh
# Runs the logic that needs no hardware with the computer's own C compiler.
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
cc -std=c99 -Wall -Wextra -Werror -fsanitize=address,undefined -g \
    -I"$root/app" -I"$root/tests" \
    "$root/app/easy_chord.c" "$root/tests/easy_chord_test.c" -o "$out/easy_chord_test"
"$out/easy_chord_test"
