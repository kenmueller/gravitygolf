#!/bin/sh
# Design aid for a hole with one machine and two places it could stand: where to put the cup so that only one of
# them is the answer.  Tests/Sim/cupscan.sh <course> <min x> <ax> <ay> <bx> <by>
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/Saved/SimTest"
COURSE="$1"
shift
python3 "$ROOT/Tools/courses/$COURSE.py" >/dev/null
clang++ -std=c++20 -O3 -I "$ROOT/Source/GravityGolf/Public" -I "$ROOT/Tests/Sim" \
	"$ROOT/Source/GravityGolf/Private/Sim/GGSim.cpp" "$ROOT/Tests/Sim/CupScan.cpp" -o "$OUT/cupscan"
"$OUT/cupscan" "$OUT/$COURSE.course" "$@"
