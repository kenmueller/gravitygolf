#!/bin/sh
# Checks the grille rule in the running game, on hole 11 (Leapfrog): a shut grille stops a carried machine, an open
# one does not, and the carries that solve the hole go through.   Tools/grilletest.sh     (needs the editor open)
# The player is moved in straight steps (GGView); the game's own per-frame rule decides whether each step is allowed.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LOG="$HOME/Library/Logs/Unreal Engine/GravityGolfEditor/GravityGolf.log"
FAILED=0
"$ROOT/Tools/play.sh" stop
python3 "$ROOT/Tools/mcp.py" call call_tool '{"toolset_name":"EditorToolset.LevelToolset","tool_name":"LoadLevel","arguments":{"levelPath":"/Game/Levels/Leapfrog","unsavedChangesPolicy":"Discard"}}' >/dev/null 2>&1
"$ROOT/Tools/play.sh" 8 >/dev/null
run() { "$ROOT/Tools/console.sh" "$1"; sleep 1; }
where() { # prints the player's x
	MARK=$(($(wc -l <"$LOG") + 1))
	"$ROOT/Tools/console.sh" "GGWhere"
	sleep 1
	tail -n +"$MARK" "$LOG" | grep "Where: at" | tail -1 | sed 's/.*Where: at \([-0-9]*\),.*/\1/'
}
expect() { # name, wanted side of the wall ("near" or "far"), x
	SIDE=far
	[ "${3:-0}" -lt 1500 ] && SIDE=near
	if [ -n "$3" ] && [ "$SIDE" = "$2" ]; then
		echo "ok    $1"
	else
		echo "FAIL  $1 (the player is at x = ${3:-unknown})"
		FAILED=1
	fi
}
# 1. Nothing on a plate. Carry the second machine up to the side grille and try to step through.
run "GGNet carry 1"
run "GGView 1300,450,100,0,0"
run "GGView 1700,450,100,0,0"
expect "a shut grille stops a carried machine" near "$(where)"
# 2. Set it on the near plate instead: the side grille opens.
run "GGView 850,-350,100,0,0"
run "GGNet place 900,-400,0"
run "GGNet carry 0"
run "GGView 1300,450,100,0,0"
run "GGView 1700,450,100,0,0"
expect "the side grille opens while a machine stands on the near plate" far "$(where)"
# 3. With that machine set on the first plate beyond, the ball's grille opens, and the other can follow through it.
run "GGView 1950,60,100,0,0"
run "GGNet place 2000,0,0"
run "GGView 850,-350,100,0,0"
run "GGNet carry 1"
run "GGView 1300,450,100,0,0"
run "GGView 1700,450,100,0,0"
expect "lifting the machine off the near plate shuts the side grille again" near "$(where)"
run "GGView 1300,0,100,0,0"
run "GGView 1700,0,100,0,0"
expect "the ball's grille is open while a machine stands on the plate beyond it" far "$(where)"
# 3b. No reaching from inside the bars: set the machine down beyond, stand in the shut side grille and try to take
# it. (GGNet pick takes a machine at any distance, so this tests the rule, not the length of an arm.)
run "GGView 2500,60,100,0,0"
run "GGNet place 2550,0,0"
run "GGView 1500,450,100,0,0"
run "GGNet pick 1"
CARRYING="$(MARK=$(($(wc -l <"$LOG") + 1)); "$ROOT/Tools/console.sh" "GGWhere"; sleep 1; tail -n +"$MARK" "$LOG" | grep "Where: at" | tail -1 | sed 's/.*carrying \([01]\).*/\1/')"
if [ "$CARRYING" = "0" ]; then echo "ok    no reaching for a machine from inside a shut grille"; else echo "FAIL  a machine was picked up from inside a shut grille"; FAILED=1; fi
# The same pick-up from the machine's own side works (so the refusal above was the rule, not a command that does
# nothing). From the near side it would be refused too, and rightly: the reach would cross the shut grille.
run "GGView 2450,60,100,0,0"
run "GGNet pick 1"
CARRYING="$(MARK=$(($(wc -l <"$LOG") + 1)); "$ROOT/Tools/console.sh" "GGWhere"; sleep 1; tail -n +"$MARK" "$LOG" | grep "Where: at" | tail -1 | sed 's/.*carrying \([01]\).*/\1/')"
if [ "$CARRYING" = "1" ]; then echo "ok    the same machine is picked up from its own side of the wall"; else echo "FAIL  the machine could not be picked up from its own side either"; FAILED=1; fi
run "GGNet place 2550,0,0"
# 4. Empty-handed, the player walks through a shut grille. (Set the machine down first, then step back through.)
run "GGView 1700,450,100,0,0"
run "GGView 1300,450,100,0,0"
expect "a player carrying nothing walks through a shut grille" near "$(where)"
"$ROOT/Tools/play.sh" stop
[ $FAILED -eq 0 ] && echo "PASS" || echo "FAILED"
[ $FAILED -eq 0 ]
