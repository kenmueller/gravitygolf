#!/bin/sh
# Builds and runs the standalone simulation tests. Pass --sweep to brute-force the bake-off hole.
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/Saved/SimTest"
mkdir -p "$OUT"
clang++ -std=c++20 -O2 -Wall -Wextra -Werror -include string_view \
	-I "$ROOT/Source/GravityGolf/Public" \
	"$ROOT/Source/GravityGolf/Private/Sim/GGSim.cpp" "$ROOT/Tests/Sim/SimTest.cpp" -o "$OUT/simtest"
"$OUT/simtest" "$@"
