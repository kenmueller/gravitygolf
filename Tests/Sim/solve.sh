#!/bin/sh
# Proves a course obeys the puzzle law: impossible without the machine, solvable with it.
#   Tests/Sim/solve.sh terraces        (add "all" to list every machine spot that works, when designing)
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/Saved/SimTest"
mkdir -p "$OUT"
python3 "$ROOT/Tools/courses/$1.py" >/dev/null
clang++ -std=c++20 -O3 -Wall -Wextra -Werror -I "$ROOT/Source/GravityGolf/Public" -I "$ROOT/Tests/Sim" \
	"$ROOT/Source/GravityGolf/Private/Sim/GGSim.cpp" "$ROOT/Tests/Sim/Solve.cpp" -o "$OUT/solve"
"$OUT/solve" "$OUT/$1.course" $2
