#!/bin/sh
# Lets the rival bot play a hole at each skill, with time sped up, and reports how long it took and how many swings.
#   Tools/rivaltest.sh [Level ...]      default: Canyon Returning
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LOG="$HOME/Library/Logs/Unreal Engine/GravityGolfEditor/GravityGolf.log"
STATUS=0
for LEVEL in ${*:-Canyon Returning}; do
	for SKILL in 3 2 1; do
		"$ROOT/Tools/play.sh" stop
		python3 "$ROOT/Tools/mcp.py" call call_tool "{\"toolset_name\":\"EditorToolset.LevelToolset\",\"tool_name\":\"LoadLevel\",\"arguments\":{\"levelPath\":\"/Game/Levels/$LEVEL\",\"unsavedChangesPolicy\":\"Discard\"}}" >/dev/null 2>&1
		"$ROOT/Tools/play.sh" 8 >/dev/null
		MARK=$(($(wc -l <"$LOG") + 1))
		"$ROOT/Tools/console.sh" "slomo 8"
		"$ROOT/Tools/console.sh" "GGRival $SKILL"
		RESULT=""
		i=0
		while [ $i -lt 90 ] && [ -z "$RESULT" ]; do
			sleep 2
			RESULT=$(tail -n +"$MARK" "$LOG" | grep "Rival: skill" | tail -1 | sed 's/.*Rival: //')
			i=$((i + 1))
		done
		[ -n "$RESULT" ] || { RESULT="did not finish in 24 game minutes ($(tail -n +"$MARK" "$LOG" | grep -c "Rival: leg") swings)"; STATUS=1; }
		printf '%-10s %s\n' "$LEVEL" "$RESULT"
	done
done
"$ROOT/Tools/play.sh" stop
exit $STATUS
