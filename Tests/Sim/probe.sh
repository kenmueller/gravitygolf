#!/bin/sh
# Design aid: play one swing and print what happened.  Tests/Sim/probe.sh <course> <leg> <yaw> <power> [x y]...
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/Saved/SimTest"
COURSE="$1"
shift
python3 "$ROOT/Tools/courses/$COURSE.py" >/dev/null
clang++ -std=c++20 -O2 -I "$ROOT/Source/GravityGolf/Public" -I "$ROOT/Tests/Sim" \
	"$ROOT/Source/GravityGolf/Private/Sim/GGSim.cpp" "$ROOT/Tests/Sim/Probe.cpp" -o "$OUT/probe"
"$OUT/probe" "$OUT/$COURSE.course" "$@"
