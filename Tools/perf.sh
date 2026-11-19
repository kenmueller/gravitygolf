#!/bin/sh
# Measures frame rate in each hole from the tee, inside the running editor (Play-In-Editor).
#   Tools/perf.sh [Level ...]      default: every hole
# Editor numbers are a floor, not the shipping figure: the editor adds overhead and renders into a panel.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LOG="$HOME/Library/Logs/Unreal Engine/GravityGolfEditor/GravityGolf.log"
LEVELS="${*:-Terraces SaltMirror Canyon Lantern Returning Brief Colonnade}"
for LEVEL in $LEVELS; do
	"$ROOT/Tools/play.sh" stop
	python3 "$ROOT/Tools/mcp.py" call call_tool "{\"toolset_name\":\"EditorToolset.LevelToolset\",\"tool_name\":\"LoadLevel\",\"arguments\":{\"levelPath\":\"/Game/Levels/$LEVEL\",\"unsavedChangesPolicy\":\"Discard\"}}" >/dev/null 2>&1
	MARK=$(($(wc -l <"$LOG") + 1))
	"$ROOT/Tools/play.sh" 12 >/dev/null
	"$ROOT/Tools/console.sh" "GGPerf 6"
	RESULT=""
	i=0
	while [ $i -lt 30 ] && [ -z "$RESULT" ]; do
		sleep 2
		RESULT=$(tail -n +"$MARK" "$LOG" | grep "Perf: " | tail -1 | sed 's/.*Perf: //')
		i=$((i + 1))
	done
	echo "${RESULT:-$LEVEL: no result}"
done
"$ROOT/Tools/play.sh" stop
