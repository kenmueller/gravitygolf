#!/bin/sh
# Design aid: random search for machine placements that solve a leg.  Tests/Sim/search.sh <course> [leg] [millions]
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/Saved/SimTest"
python3 "$ROOT/Tools/courses/$1.py" >/dev/null
clang++ -std=c++20 -O3 -Wall -Wextra -I "$ROOT/Source/GravityGolf/Public" \
	"$ROOT/Source/GravityGolf/Private/Sim/GGSim.cpp" "$ROOT/Tests/Sim/Search.cpp" -o "$OUT/search"
"$OUT/search" "$OUT/$1.course" "${2:-0}" "${3:-3}"
