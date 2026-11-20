#!/bin/sh
# Design aid for holes whose puzzle is the order of carrying: tries every layout of rooms, walls, grilles and weight
# plates of a given size and prints the ones whose shortest answer takes the most carries (see Rooms.cpp).
#   Tests/Sim/rooms.sh <rooms> <plates> <heavy machines> <mirrors> [spare plinths a room] [side grilles a wall] [show]
#   Tests/Sim/rooms.sh 3 4 2 0      found hole 14 (Hand Over Hand): seven carries
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/Saved/SimTest"
mkdir -p "$OUT"
clang++ -std=c++20 -O3 "$ROOT/Tests/Sim/Rooms.cpp" -o "$OUT/rooms"
"$OUT/rooms" "$@"
