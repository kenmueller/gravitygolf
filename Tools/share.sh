#!/bin/sh
# Give someone a link to play the last web build:  Tools/share.sh        (Tools/share.sh check: everything but the link)
#
# Opens a temporary public address that leads to the game served from this Mac (a Cloudflare quick tunnel: no
# account, a random name that changes every time). Anyone who has the link can play in their browser, on a computer
# or a phone, until you press Ctrl-C here. This Mac has to stay awake and online, and the first load sends about
# 70 MB up this connection, so on a slow network the friend waits a few minutes.
#
# This is the one tool here that puts the game on the internet, and only while it runs. Nothing is uploaded or kept
# anywhere: the link reaches only the game's own files in the web build's folder. Using a quick tunnel means accepting
# Cloudflare's terms for it, which it prints when it starts. Playing together is not in the web version: a friend
# gets the whole game to themselves.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WEB="$ROOT/../gravitygolf-web/Saved/Web"
PORT=8967
LOG="$ROOT/Saved/share.log"
[ -f "$WEB/Wasm/index.html" ] || { echo "No web build at $WEB/Wasm. Run Tools/web.sh Development first."; exit 1; }
command -v cloudflared >/dev/null 2>&1 || { echo "cloudflared is not installed: brew install cloudflared"; exit 1; }
mkdir -p "$ROOT/Saved"

# The game's own server, on this Mac only. Started here if it is not already up, and left running.
if ! curl -s -o /dev/null "http://localhost:$PORT/index.html"; then
	(cd "$WEB" && nohup python3 -I -B serve.py --root Wasm --port "$PORT" >"$ROOT/Saved/webplay-server.log" 2>&1 &)
	sleep 2
fi
CODE="$(curl -s -o /dev/null -w '%{http_code}' "http://localhost:$PORT/index.html")"
[ "$CODE" = "200" ] || { echo "The game's own server is not answering on port $PORT (got $CODE)."; exit 1; }
if [ "$1" = "check" ]; then
	echo "ok: the web build is there, its server answers on this Mac, and cloudflared is installed. The link itself is not tried."
	exit 0
fi

# The tunnel. The game's server refuses requests addressed to names it does not know (a guard against other sites
# reaching it), so the tunnel hands them on addressed to localhost.
: >"$LOG"
cloudflared tunnel --no-autoupdate --url "http://localhost:$PORT" --http-host-header localhost >"$LOG" 2>&1 &
TUNNEL=$!
trap 'kill $TUNNEL 2>/dev/null; echo; echo "The link is closed."; exit 0' INT TERM
URL=""
i=0
while [ -z "$URL" ] && [ $i -lt 40 ]; do
	sleep 1
	URL="$(grep -o 'https://[a-z0-9-]*\.trycloudflare\.com' "$LOG" | head -1)"
	kill -0 $TUNNEL 2>/dev/null || break
	i=$((i + 1))
done
if [ -z "$URL" ]; then
	echo "No link came back from Cloudflare. What it said:"
	tail -8 "$LOG"
	kill $TUNNEL 2>/dev/null
	exit 1
fi
echo
echo "    $URL"
echo
command -v qrencode >/dev/null 2>&1 && qrencode -t ANSIUTF8 "$URL"
echo "Send that link. It works until you press Ctrl-C here. (Cloudflare's terms for quick tunnels: see $LOG)"

# Try it from outside, as the friend's browser will: the page, and the two headers the game cannot start without.
# A name this new is unknown to the network's own name server for a minute or two, and an early "no such name" gets
# remembered: the first version of this check gave up and warned about a link that worked. So the address is asked
# of a public name server and handed to curl directly.
HOST="${URL#https://}"
OK=""
i=0
while [ $i -lt 30 ]; do
	IP="$(dig +short @1.1.1.1 "$HOST" 2>/dev/null | grep -E '^[0-9.]+$' | head -1)"
	[ -n "$IP" ] || { sleep 3; i=$((i + 1)); continue; }
	HEAD="$(curl -s -m 10 --resolve "$HOST:443:$IP" -D - -o /dev/null "$URL/index.html")"
	if echo "$HEAD" | grep -qi "^HTTP/[0-9.]* 200"; then
		OK=1
		break
	fi
	sleep 3
	i=$((i + 1))
done
if [ -z "$OK" ]; then
	echo "WARNING: the link did not answer when tried from here in a minute and a half. Try it in a browser before sending it."
elif echo "$HEAD" | grep -qi "cross-origin-opener-policy: same-origin" && echo "$HEAD" | grep -qi "cross-origin-embedder-policy"; then
	echo "Checked: the link answers and carries the headers the game needs."
else
	echo "WARNING: the link answers but without the headers the game needs; it will not start in a browser."
fi
wait $TUNNEL
