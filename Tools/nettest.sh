#!/bin/sh
# Plays hole 3 with two players in the editor (a listen server and one client) and checks that both machines' copies
# of the hole agree after every step. The client carries and places both machines; the server swings.
#   Tools/nettest.sh
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LOG="$HOME/Library/Logs/Unreal Engine/GravityGolfEditor/GravityGolf.log"
players() { sed "s/^PLAYERS = .*/PLAYERS = $1/" "$ROOT/Tools/net_players.py" > /tmp/gg_players.py; "$ROOT/Tools/py.sh" /tmp/gg_players.py >/dev/null; }
call() { python3 "$ROOT/Tools/mcp.py" call call_tool "$1" >/dev/null 2>&1; }
"$ROOT/Tools/play.sh" stop
players 2
call '{"toolset_name":"EditorToolset.LevelToolset","tool_name":"LoadLevel","arguments":{"levelPath":"/Game/Levels/Canyon","unsavedChangesPolicy":"Discard"}}'
GG_PLAYERS=2 "$ROOT/Tools/play.sh" 16 >/dev/null
STATUS=0
# step "<label>" <player> "<command>" <seconds to wait>: run it, then compare what each machine reports
step() {
	MARK=$(($(wc -l <"$LOG") + 1))
	"$ROOT/Tools/net.sh" "$2" "$3"
	sleep "$4"
	MARK2=$(($(wc -l <"$LOG") + 1))
	"$ROOT/Tools/net.sh" all "GGNet where 0"
	sleep 1
	SERVER=$(tail -n +"$MARK2" "$LOG" | grep "Net\[server\] where" | tail -1 | sed 's/.*| //')
	CLIENT=$(tail -n +"$MARK2" "$LOG" | grep "Net\[client\] where" | tail -1 | sed 's/.*| //')
	if [ -n "$SERVER" ] && [ "$SERVER" = "$CLIENT" ]; then
		printf 'ok    %-34s %s\n' "$1" "$SERVER"
	else
		printf 'DIFF  %-34s\n      server: %s\n      client: %s\n' "$1" "$SERVER" "$CLIENT"
		STATUS=1
	fi
}
step "start" all "GGNet where 0" 1
step "client picks up the attractor" 1 "GGNet carry 0" 1
step "client walks to the doorway" 1 "GGNet tp 2800,600,95" 1
step "client sets it down" 1 "GGNet place 2900,700,0" 1
step "client goes back for the repulsor" 1 "GGNet tp 1000,-100,95" 1
step "client picks up the repulsor" 1 "GGNet carry 1" 1
step "client walks to the far wall" 1 "GGNet tp 2800,1000,95" 1
step "client sets it down" 1 "GGNet place 2850,1200,0" 1
step "server swings the answer" 0 "GGNet swing -20,0.575" 16
step "server resets the hole" 0 "GGNet reset 0" 2
step "client swings, no machines" 1 "GGNet goto 0" 1
step "client swings, no machines" 1 "GGNet swing 0,0.3" 9
step "client calls the ball back" 1 "GGNet return 0" 1
"$ROOT/Tools/play.sh" stop
players 1
exit $STATUS
