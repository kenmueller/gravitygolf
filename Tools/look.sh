#!/bin/sh
# Opens a level, starts play and saves pictures of it:  Tools/look.sh <Level> <out prefix> ["x,y,z,pitch,yaw" ...]
# The first picture (<prefix>-tee.png) is from the tee; one more per view given (<prefix>-1.png, -2.png...).
# Play is left running, so a console command or another Tools/shot.sh can follow. Stop it with Tools/play.sh stop.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LEVEL="$1"
PREFIX="$2"
shift 2
"$ROOT/Tools/play.sh" stop
python3 "$ROOT/Tools/mcp.py" call call_tool "{\"toolset_name\":\"EditorToolset.LevelToolset\",\"tool_name\":\"LoadLevel\",\"arguments\":{\"levelPath\":\"/Game/Levels/$LEVEL\",\"unsavedChangesPolicy\":\"Discard\"}}" >/dev/null 2>&1
"$ROOT/Tools/play.sh" 10 >/dev/null
"$ROOT/Tools/shot.sh" "$LEVEL" "$PREFIX-tee.png" | tail -1
N=1
for VIEW in "$@"; do
	"$ROOT/Tools/shot.sh" "$LEVEL" "$PREFIX-$N.png" "$VIEW" | tail -1
	N=$((N + 1))
done
