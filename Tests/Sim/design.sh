#!/bin/sh
# Runs the standalone tools on a Workshop design:  Tests/Sim/design.sh <design.json> solve|search|probe [arguments]
#   solve                              the puzzle-law proof
#   search [leg] [millions]            random search for machine spots and a swing
#   probe <leg> <yaw> <power> [x y]... trace one swing
# The design comes from the game: in the Workshop, the console command `GGWorkshop json <file>` writes it.
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/Saved/SimTest"
mkdir -p "$OUT"
python3 "$ROOT/Tools/courses/from_json.py" "$1" design >/dev/null
TOOL="$2"
shift 2
case "$TOOL" in
solve) SRC=Solve.cpp ;;
search) SRC=Search.cpp ;;
probe) SRC=Probe.cpp ;;
*) echo "solve, search or probe" >&2; exit 2 ;;
esac
clang++ -std=c++20 -O3 -I "$ROOT/Source/GravityGolf/Public" -I "$ROOT/Tests/Sim" \
	"$ROOT/Source/GravityGolf/Private/Sim/GGSim.cpp" "$ROOT/Tests/Sim/$SRC" -o "$OUT/design_$TOOL"
"$OUT/design_$TOOL" "$OUT/design.course" "$@"
