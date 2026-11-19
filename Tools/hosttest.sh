#!/bin/sh
# Hosting and joining for real: two standalone copies of the game on this machine. One hosts a hole, the other starts
# on a different hole and joins by address. Passes when the host accepts the join and the joiner loads the host's hole.
#   Tools/hosttest.sh [Level]        default: Canyon
# (Tools/nettest.sh checks that two players' copies of a hole stay in step; this checks the connection itself.)
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.9}/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
LEVEL="${1:-Canyon}"
OUT="$ROOT/Saved/HostTest"
mkdir -p "$OUT"
rm -f "$OUT/host.log" "$OUT/client.log"
wait_for() { # file, text, seconds
	i=0
	while [ $i -lt "$3" ]; do
		grep -q "$2" "$1" 2>/dev/null && return 0
		sleep 1
		i=$((i + 1))
	done
	return 1
}
"$UE" "$ROOT/GravityGolf.uproject" "/Game/Levels/$LEVEL?listen" -game -windowed -ResX=640 -ResY=360 -GGNoTitle -GGTestSave -nosplash -unattended \
	-abslog="$OUT/host.log" >/dev/null 2>&1 &
HOST=$!
STATUS=0
if wait_for "$OUT/host.log" "Loaded course " 120; then
	"$UE" "$ROOT/GravityGolf.uproject" 127.0.0.1 -game -windowed -ResX=640 -ResY=360 -GGNoTitle -GGTestSave -nosplash -unattended \
		-abslog="$OUT/client.log" >/dev/null 2>&1 &
	CLIENT=$!
	wait_for "$OUT/host.log" "Join succeeded" 120 && echo "ok    the host accepted the join" || { echo "FAIL  the host never accepted a join"; STATUS=1; }
	wait_for "$OUT/client.log" "Loaded course " 30 && echo "ok    the joiner loaded $(grep 'Loaded course ' "$OUT/client.log" | tail -1 | sed 's/.*Loaded course //')" \
		|| { echo "FAIL  the joiner never loaded the hole"; STATUS=1; }
	kill "$CLIENT" 2>/dev/null
else
	echo "FAIL  the host never loaded $LEVEL"
	STATUS=1
fi
kill "$HOST" 2>/dev/null
[ $STATUS -eq 0 ] && echo "PASS" || echo "FAIL"
exit $STATUS
