#!/bin/sh
# Play the last web build on this Mac, in the browser you name:  Tools/webplay.sh [Safari|"Google Chrome"|Firefox]
# Serves it at http://localhost:8967 (a page served from this computer itself needs no certificate) and opens it.
# The game's own log lines are printed here while it plays. The server stops a minute after the game's tab is
# closed, or with Ctrl-C. Build first with Tools/web.sh Development.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WEB="$ROOT/../gravitygolf-web/Saved/Web"
BROWSER="${1:-Safari}"
PORT=8967
[ -f "$WEB/Wasm/index.html" ] || { echo "No web build at $WEB/Wasm. Run Tools/web.sh Development first."; exit 1; }
if lsof -nP -iTCP:$PORT -sTCP:LISTEN >/dev/null 2>&1; then
	echo "Something is already serving on port $PORT; opening the page there."
	open -a "$BROWSER" "http://localhost:$PORT/index.html?ue-session=1"
	exit 0
fi
(sleep 2; open -a "$BROWSER" "http://localhost:$PORT/index.html?ue-session=1") &
echo "Opening http://localhost:$PORT/ in $BROWSER"
exec python3 -I -B "$WEB/serve.py" --root "$WEB/Wasm" --port "$PORT" --game-run --exit-after-session 60
