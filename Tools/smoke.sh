#!/bin/sh
# End-to-end check inside the running editor: opens each hole, presses Play, and has the game play its own
# recorded solution through the real player code. Prints PASS or FAIL per hole.
#   Tools/smoke.sh [Level ...]      default: every hole
# The whole script is one function, called on its last line: the shell reads all of it before running any of it, so
# editing this file while it runs cannot change or break the run in progress.
main() {
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LOG="$HOME/Library/Logs/Unreal Engine/GravityGolfEditor/GravityGolf.log"
LEVELS="${*:-Terraces SaltMirror Canyon Lantern Returning Brief Colonnade Stones Glass Periscope Leapfrog AfterYou WaitHere HandOver Wires}"
call() {
	python3 "$ROOT/Tools/mcp.py" call call_tool "$1" 2>&1
}
STATUS=0
for LEVEL in $LEVELS; do
	"$ROOT/Tools/play.sh" stop
	call "{\"toolset_name\":\"EditorToolset.LevelToolset\",\"tool_name\":\"LoadLevel\",\"arguments\":{\"levelPath\":\"/Game/Levels/$LEVEL\",\"unsavedChangesPolicy\":\"Discard\"}}" >/dev/null
	MARK=$(($(wc -l <"$LOG") + 1))
	"$ROOT/Tools/play.sh" 6 >/dev/null
	"$ROOT/Tools/console.sh" "GGDemo"
	RESULT=""
	i=0
	while [ $i -lt 60 ] && [ -z "$RESULT" ]; do
		sleep 2
		RESULT=$(tail -n +"$MARK" "$LOG" | grep -E "Demo: (PASS|FAIL)" | tail -1 | sed 's/.*Demo: //')
		i=$((i + 1))
	done
	if [ -z "$RESULT" ]; then
		RESULT="FAIL (no result within two minutes)"
	fi
	case "$RESULT" in PASS*) ;; *) STATUS=1 ;; esac
	printf '%-11s %s\n' "$LEVEL" "$RESULT"
done
"$ROOT/Tools/play.sh" stop
exit $STATUS
}
main "$@"; exit
