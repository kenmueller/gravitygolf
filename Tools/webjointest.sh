#!/bin/sh
# A browser joins a desktop host: the web build in Chrome, a standalone copy of the game hosting hole 3, and Epic's
# relay between them (a browser has no UDP; the relay carries its packets as WebSocket messages). All on this Mac.
# The browser player carries both machines to the hole's answer and swings; the host's log and the browser's must
# report the same swing, to the centimetre: the two are different builds for different processors, and the game
# keeps a hole in step by trusting that both compute the same thing.
#   Tools/webjointest.sh        (needs a Development web build: Tools/web.sh Development; the editor may be closed)
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="${UE_ROOT:-$HOME/UE59}"
UE="/Users/Shared/Epic Games/UE_5.9/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
WEB="${GG_WEB_COPY:-$ROOT/../gravitygolf-web}"
STAGE="${GG_WEB_OUT:-$WEB/Saved/Web}/Wasm"
PY="$SRC/Engine/Binaries/ThirdParty/Python3/Mac/bin/python3"
OUT="$ROOT/Saved/WebJoin"
mkdir -p "$OUT"
rm -f "$OUT/host.log" "$OUT/relay.log"
"$UE" "$ROOT/GravityGolf.uproject" "/Game/Levels/Canyon?listen" -game -windowed -ResX=640 -ResY=360 -GGNoTitle -GGTestSave -nosplash \
	-unattended -abslog="$OUT/host.log" >/dev/null 2>&1 &
HOST=$!
"$PY" -I "$SRC/Engine/Platforms/Wasm/Build/Tools/web/ws_relay.py" >"$OUT/relay.log" 2>&1 &
RELAY=$!
trap 'kill $HOST $RELAY 2>/dev/null' EXIT
i=0
until grep -q "Loaded course " "$OUT/host.log" 2>/dev/null || [ $i -ge 120 ]; do sleep 1; i=$((i + 1)); done
grep -q "Loaded course " "$OUT/host.log" || { echo "FAIL  the host never loaded the hole"; exit 1; }
rm -rf "$WEB/Saved/WebTest/web-join"
"$PY" "$SRC/Engine/Platforms/Wasm/Build/Tools/web/webrun.py" run --stage "$STAGE" --fixtures "$ROOT/Build/Wasm/fixtures.json" \
	--fixture web-join --out "$WEB/Saved/WebTest/web-join" 2>&1 | tail -1 | cut -c1-110
LOG="$WEB/Saved/WebTest/web-join/console.log"
STATUS=0
grep -q "Join succeeded" "$OUT/host.log" && echo "ok    the host accepted the browser's join" || { echo "FAIL  the host never accepted a join"; STATUS=1; }
grep -q "Net\[client\] where" "$LOG" 2>/dev/null && echo "ok    the browser is a client in the host's hole" || { echo "FAIL  the browser never became a client"; STATUS=1; }
# Every swing is logged by both machines as "Stroke N finished: <state> at <where>", to a hundredth of a centimetre.
# The browser player plays two: the hole's answer (it sinks), then a plain swing that only comes to rest. Each
# machine's lines must be the same, in the same order.
swings() { grep "Stroke [0-9]* finished" "$1" 2>/dev/null | sed 's/.*Stroke/Stroke/' | tr -d '\r'; }
HOSTSWINGS="$(swings "$OUT/host.log")"
WEBSWINGS="$(swings "$LOG")"
COUNT="$(echo "$HOSTSWINGS" | grep -c "Stroke")"
if [ "$COUNT" -ge 2 ] && [ "$HOSTSWINGS" = "$WEBSWINGS" ]; then
	echo "ok    host and browser report the same $COUNT swings:"
	echo "$HOSTSWINGS" | sed 's/^/        /'
else
	echo "FAIL  the swings differ or are missing"
	echo "      host:"; echo "${HOSTSWINGS:-nothing}" | sed 's/^/        /'
	echo "      browser:"; echo "${WEBSWINGS:-nothing}" | sed 's/^/        /'
	STATUS=1
fi
echo "$HOSTSWINGS" | head -1 | grep -q "state 2" && echo "ok    the answer, carried and swung from the browser, sank the ball" || { echo "FAIL  the answer did not sink"; STATUS=1; }
[ $STATUS -eq 0 ] && echo "PASS" || echo "FAIL"
exit $STATUS
