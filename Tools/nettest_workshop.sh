#!/bin/sh
# A Workshop hole played together: two players in the editor (the host and one who joined). The host starts a new
# hole, changes it, tests it and solves it; after every step both machines' copies of the hole must agree.
#   Tools/nettest_workshop.sh
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LOG="$HOME/Library/Logs/Unreal Engine/GravityGolfEditor/GravityGolf.log"
players() { sed "s/^PLAYERS = .*/PLAYERS = $1/" "$ROOT/Tools/net_players.py" > /tmp/gg_players.py; "$ROOT/Tools/py.sh" /tmp/gg_players.py >/dev/null; }
call() { python3 "$ROOT/Tools/mcp.py" call call_tool "$1" >/dev/null 2>&1; }
"$ROOT/Tools/play.sh" stop
players 2
call '{"toolset_name":"EditorToolset.LevelToolset","tool_name":"LoadLevel","arguments":{"levelPath":"/Game/Levels/Workshop","unsavedChangesPolicy":"Discard"}}'
"$ROOT/Tools/play.sh" 16 >/dev/null
"$ROOT/Tools/net.sh" all "GGTestSave" >/dev/null
STATUS=0
step() { # label, player, command, seconds
	"$ROOT/Tools/net.sh" "$2" "$3" >/dev/null
	sleep "$4"
	MARK=$(($(wc -l <"$LOG") + 1))
	"$ROOT/Tools/net.sh" all "GGNet where 0" >/dev/null
	sleep 1
	SERVER=$(tail -n +"$MARK" "$LOG" | grep "Net\[server\] where" | tail -1 | sed 's/.*| //')
	CLIENT=$(tail -n +"$MARK" "$LOG" | grep "Net\[client\] where" | tail -1 | sed 's/.*| //')
	if [ -n "$SERVER" ] && [ "$SERVER" = "$CLIENT" ]; then
		printf 'ok    %-34s %s\n' "$1" "$SERVER"
	else
		printf 'DIFF  %-34s\n      server: %s\n      client: %s\n' "$1" "$SERVER" "$CLIENT"
		STATUS=1
	fi
}
step "the host starts a new hole" 0 "GGWorkshop new" 2
step "the host adds a repulsor" 0 "GGWorkshop tool repulsor" 0
step "the host adds a repulsor" 0 "GGWorkshop stroke 525,-275" 2
step "the host takes it back" 0 "GGWorkshop undo" 2
step "the host starts the test" 0 "GGWorkshop test" 2
step "the host places the machine" 0 "GGMachine 1650,-50,0" 1
step "the host swings the answer" 0 "GGShot 44,0.15" 10
step "the host goes back to building" 0 "GGWorkshop build" 2
"$ROOT/Tools/play.sh" stop
players 1
[ $STATUS -eq 0 ] && echo "all passed" || echo "FAILED"
exit $STATUS
