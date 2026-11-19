#!/bin/sh
# Play a hole together with a friend who is in a browser:  Tools/together.sh [Level]      (default: Terraces)
#        Tools/together.sh check     everything but the public links and the game window
#        GG_LOCAL=1 Tools/together.sh [Level]     the same with no public links: the "friend" is a browser on this Mac
#
# You play in a window on this Mac, which hosts the hole. Your friend opens the link this prints and lands in the
# same hole, in their browser, on a computer. A browser cannot send the game's packets itself, so a small relay
# (Epic's ws_relay.py) runs here and carries them; the link tells the page where it is.
#
# Two temporary public addresses are opened (Cloudflare quick tunnels, no account): one for the page, one for the
# relay. Both close when you press Ctrl-C here, which also closes the game window. While they are open, anyone who
# has the link can join your hole. The relay passes packets only to this Mac, and only from that page.
# Proved on this Mac without the public part (Tools/webjointest.sh). Across the internet it is untried.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="${UE_ROOT:-$HOME/UE59}"
UE="/Users/Shared/Epic Games/UE_5.9/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
WEB="${GG_WEB_COPY:-$ROOT/../gravitygolf-web}"
RELAY="$SRC/Engine/Platforms/Wasm/Build/Tools/web/ws_relay.py"
OUT="$ROOT/Saved/Together"
PAGEPORT=8971
RELAYPORT=8889
LEVEL="${1:-Terraces}"
mkdir -p "$OUT"

# The newest Development web build (a Shipping page ignores what a link asks of it, so it cannot be sent to a host).
STAGE="${GG_WEB_OUT:+$GG_WEB_OUT/Wasm}"
if [ -z "$STAGE" ]; then
	STAGE="$(ls -t "$WEB"/Saved/Web/Wasm/index.html "$WEB"/Saved/WebJoinBuild/Wasm/index.html 2>/dev/null | head -1 | xargs dirname 2>/dev/null)"
fi
[ -n "$STAGE" ] && [ -f "$STAGE/index.html" ] || { echo "No web build found. Run Tools/web.sh Development first."; exit 1; }
[ -f "$RELAY" ] || { echo "Epic's relay is not at $RELAY (it comes with the engine source)."; exit 1; }
[ -x "$UE" ] || { echo "The engine is not at $UE."; exit 1; }
command -v cloudflared >/dev/null 2>&1 || { echo "cloudflared is not installed: brew install cloudflared"; exit 1; }
if [ "$LEVEL" = "check" ]; then
	echo "ok: a web build ($STAGE), the relay, the engine and cloudflared are all here. The links and the game are not started."
	exit 0
fi
[ -f "$ROOT/Content/Levels/$LEVEL.umap" ] || { echo "No hole called $LEVEL. Try one of: $(ls "$ROOT/Content/Levels" | sed 's/\.umap//' | tr '\n' ' ')"; exit 1; }

PIDS=""
cleanup() { for P in $PIDS; do kill "$P" 2>/dev/null; done; echo; echo "Closed: the links, the relay and the game."; exit 0; }
trap cleanup INT TERM

# 1. The page, served from this Mac only.
(cd "$STAGE/.." && exec python3 -I -B "$WEB/Saved/Web/serve.py" --root "$STAGE" --port "$PAGEPORT") >"$OUT/page.log" 2>&1 &
PIDS="$PIDS $!"
# 2. The two public addresses. (Not with GG_LOCAL: then the page and the relay are reached on this Mac itself.)
: >"$OUT/tunnel-page.log"; : >"$OUT/tunnel-relay.log"
[ -n "$GG_LOCAL" ] || cloudflared tunnel --no-autoupdate --url "http://localhost:$PAGEPORT" --http-host-header localhost >"$OUT/tunnel-page.log" 2>&1 &
PIDS="$PIDS $!"
[ -n "$GG_LOCAL" ] || cloudflared tunnel --no-autoupdate --url "http://localhost:$RELAYPORT" >"$OUT/tunnel-relay.log" 2>&1 &
PIDS="$PIDS $!"
PAGE=""; RELAYURL=""; i=0
[ -n "$GG_LOCAL" ] && { PAGE="http://localhost:$PAGEPORT"; RELAYURL="http://127.0.0.1"; }
while { [ -z "$PAGE" ] || [ -z "$RELAYURL" ]; } && [ $i -lt 45 ]; do
	sleep 1
	PAGE="$(grep -o 'https://[a-z0-9-]*\.trycloudflare\.com' "$OUT/tunnel-page.log" | head -1)"
	RELAYURL="$(grep -o 'https://[a-z0-9-]*\.trycloudflare\.com' "$OUT/tunnel-relay.log" | head -1)"
	i=$((i + 1))
done
if [ -z "$PAGE" ] || [ -z "$RELAYURL" ]; then
	echo "Cloudflare did not give both addresses. See $OUT/tunnel-page.log and tunnel-relay.log."
	for P in $PIDS; do kill "$P" 2>/dev/null; done
	exit 1
fi
# 3. The relay, which will carry packets only for that page and only to this Mac.
"$SRC/Engine/Binaries/ThirdParty/Python3/Mac/bin/python3" -I "$RELAY" --port "$RELAYPORT" --origin "$PAGE" --allow 127.0.0.0/8 >"$OUT/relay.log" 2>&1 &
PIDS="$PIDS $!"
# 4. The game, hosting the hole, in a window. Your own save, your own progress.
"$UE" "$ROOT/GravityGolf.uproject" "/Game/Levels/$LEVEL?listen" -game -windowed -ResX=1600 -ResY=900 -GGNoTitle -nosplash \
	-abslog="$OUT/host.log" >/dev/null 2>&1 &
PIDS="$PIDS $!"

LINK="$PAGE/?args=127.0.0.1:7777%20-WebRelay=${RELAYURL#https://}:443"
[ -n "$GG_LOCAL" ] && LINK="$PAGE/?args=127.0.0.1:7777%20-WebRelay=127.0.0.1:$RELAYPORT"
echo
echo "Send your friend this link (a computer with Chrome, Edge or Safari; it joins your hole when the page has loaded):"
echo
echo "    $LINK"
echo
echo "The game window is opening on this Mac: that is your seat. Ctrl-C here closes everything."
echo "(When your friend arrives, this window says so.)"
SEEN=0
while :; do
	sleep 3
	NOW="$(grep -c "Join succeeded" "$OUT/host.log" 2>/dev/null)"
	if [ "${NOW:-0}" -gt "$SEEN" ]; then
		SEEN="$NOW"
		echo "A friend has joined. ($SEEN so far)"
	fi
done
