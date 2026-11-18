#!/usr/bin/env bash
# Release check for the developer console: the binary must contain no command name.
# Usage: tools/check_release_console.sh [path/to/executable]    (default: build/release/my_app)
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
binary="${1:-$root/build/release/my_app}"

if [ ! -f "$binary" ]; then
	echo "no executable at $binary (build it first: cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release && cmake --build build/release)" >&2
	exit 2
fi

# One name per command group: engine, editor, game, variables
names="comp_add|con_pause|spawn_here|phys_debug|level_load|tp_spawn|gun_reach|timescale|Unknown command"
found="$(strings "$binary" | grep -E "$names" || true)"

if [ -n "$found" ]; then
	echo "FAIL: console strings found in $binary:" >&2
	echo "$found" >&2
	exit 1
fi
echo "OK: no console strings in $binary"
