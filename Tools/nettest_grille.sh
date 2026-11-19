#!/bin/sh
# Two players on hole 11 (Leapfrog): the joined player makes the carries, grilles and all, and the host swings.
# Both machines' copies of the hole are compared after every step, as in Tools/nettest.sh.   Tools/nettest_grille.sh
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LOG="$HOME/Library/Logs/Unreal Engine/GravityGolfEditor/GravityGolf.log"
players() { sed "s/^PLAYERS = .*/PLAYERS = $1/" "$ROOT/Tools/net_players.py" > /tmp/gg_players.py; "$ROOT/Tools/py.sh" /tmp/gg_players.py >/dev/null; }
call() { python3 "$ROOT/Tools/mcp.py" call call_tool "$1" >/dev/null 2>&1; }
"$ROOT/Tools/play.sh" stop
players 2
call '{"toolset_name":"EditorToolset.LevelToolset","tool_name":"LoadLevel","arguments":{"levelPath":"/Game/Levels/Leapfrog","unsavedChangesPolicy":"Discard"}}'
GG_PLAYERS=2 "$ROOT/Tools/play.sh" 16 >/dev/null
STATUS=0
LAST=""
step() {
	"$ROOT/Tools/net.sh" "$2" "$3"
	sleep "$4"
	MARK2=$(($(wc -l <"$LOG") + 1))
	"$ROOT/Tools/net.sh" all "GGNet where 0"
	sleep 1
	SERVER=$(tail -n +"$MARK2" "$LOG" | grep "Net\[server\] where" | tail -1 | sed 's/.*| //')
	CLIENT=$(tail -n +"$MARK2" "$LOG" | grep "Net\[client\] where" | tail -1 | sed 's/.*| //')
	LAST="$SERVER"
	if [ -n "$SERVER" ] && [ "$SERVER" = "$CLIENT" ]; then
		printf 'ok    %-52s %s\n' "$1" "$(echo "$SERVER" | cut -c1-90)"
	else
		printf 'DIFF  %-52s\n      server: %s\n      client: %s\n' "$1" "$SERVER" "$CLIENT"
		STATUS=1
	fi
}
step "start" all "GGNet where 0" 1
step "client picks up the second machine" 1 "GGNet carry 1" 1
step "client walks to the shut side grille" 1 "GGNet tp 1300,450,95" 1
step "client tries to step through it" 1 "GGNet tp 1700,450,95" 2
step "client tries to set it down beyond the wall" 1 "GGNet place 2000,0,0" 1
step "client sets it on the near plate instead" 1 "GGNet tp 850,-350,95" 1
step "client sets it on the near plate instead" 1 "GGNet place 900,-400,0" 1
step "client picks up the first machine" 1 "GGNet carry 0" 1
step "client carries it through the open side grille" 1 "GGNet tp 1300,450,95" 1
step "client carries it through the open side grille" 1 "GGNet tp 1700,450,95" 1
step "client sets it on the first plate beyond" 1 "GGNet tp 1950,60,95" 1
step "client sets it on the first plate beyond" 1 "GGNet place 2000,0,0" 1
step "client goes back and lifts the other" 1 "GGNet tp 850,-350,95" 1
step "client goes back and lifts the other" 1 "GGNet carry 1" 1
step "client carries it through the ball's grille" 1 "GGNet tp 1300,0,95" 1
step "client carries it through the ball's grille" 1 "GGNet tp 1700,0,95" 1
step "client sets it on the second plate" 1 "GGNet tp 2500,60,95" 1
step "client sets it on the second plate" 1 "GGNet place 2550,0,0" 1
step "server swings the answer" 0 "GGNet swing 0,0.2" 14
case "$LAST" in *sunk*|*Sunk*|*solved*|*phase\ 3*) echo "ok    the ball is in the cup on both machines" ;; *) echo "note  last state: $LAST" ;; esac
"$ROOT/Tools/play.sh" stop
players 1
[ $STATUS -eq 0 ] && echo "IN STEP" || echo "OUT OF STEP"
exit $STATUS
