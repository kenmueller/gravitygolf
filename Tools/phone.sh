#!/bin/sh
# Play the last web build on a real phone:  Tools/phone.sh [port]
#
# A phone only runs the game from a secure page, so this serves it over HTTPS on the local network, with a
# certificate made here for this Mac's name and addresses. Put the phone on the same Wi-Fi and open the address it
# prints (or point the camera at the square). The phone will say it does not know the certificate: that is
# expected for a certificate made at home; choose to visit the page anyway. Nothing leaves the local network.
#   - If macOS asks whether Python may accept incoming connections, allow it. If the phone connects and then waits
#     for ever, the Mac's firewall is refusing it: System Settings, Network, Firewall, Options, allow python3.
#   - Build first with Tools/web.sh Development. Ctrl-C stops the server.
# Tools/phone.sh check   starts it, fetches the page over HTTPS from this Mac's own network address, and stops.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WEB="$ROOT/../gravitygolf-web/Saved/Web"
PORT=8443
CHECK=""
case "$1" in check) CHECK=1 ;; "") ;; *) PORT="$1" ;; esac
[ -f "$WEB/Wasm/index.html" ] || { echo "No web build at $WEB/Wasm. Run Tools/web.sh Development first."; exit 1; }

NAME="$(scutil --get LocalHostName 2>/dev/null || hostname -s)"
ADDRESSES="$(ifconfig | awk '/inet / && $2 != "127.0.0.1" {print $2}')"
[ -n "$ADDRESSES" ] || { echo "This Mac has no network address: join the Wi-Fi the phone is on."; exit 1; }

# The certificate names this Mac and every address it has now. It is made again when those change.
CERT="$ROOT/Saved/PhoneCert"
mkdir -p "$CERT"
SAN="DNS:$NAME.local,DNS:localhost,IP:127.0.0.1"
for A in $ADDRESSES; do SAN="$SAN,IP:$A"; done
if [ "$(cat "$CERT/names" 2>/dev/null)" != "$SAN" ] || [ ! -f "$CERT/cert.pem" ]; then
	openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -nodes -days 90 \
		-keyout "$CERT/key.pem" -out "$CERT/cert.pem" -subj "/CN=$NAME.local" \
		-addext "subjectAltName=$SAN" -addext "extendedKeyUsage=serverAuth" >/dev/null 2>&1 ||
		{ echo "Could not make a certificate (openssl)."; exit 1; }
	echo "$SAN" > "$CERT/names"
fi

# The Mac's firewall decides per program who may be reached from the network. The system's own Python is normally
# allowed already; a newer one from Homebrew is not (connections open, then nothing answers), so use the system's.
PY=/usr/bin/python3
[ -x "$PY" ] || PY=python3
FIRST="$(echo "$ADDRESSES" | head -1)"
if [ -n "$CHECK" ]; then
	# Started directly (not through a function), so that $! is the server itself and stopping it really stops it.
	"$PY" -I -B "$WEB/serve.py" --root "$WEB/Wasm" --bind 0.0.0.0 --port "$PORT" \
		--tls-cert "$CERT/cert.pem" --tls-key "$CERT/key.pem" >"$CERT/server.log" 2>&1 &
	SERVER=$!
	sleep 2
	HEAD="$(curl -sk -m 8 -D - -o /dev/null "https://$FIRST:$PORT/index.html")"
	kill $SERVER 2>/dev/null
	wait $SERVER 2>/dev/null
	if ! echo "$HEAD" | grep -qi "^HTTP/[0-9.]* 200"; then
		echo "FAIL: no page at https://$FIRST:$PORT/"
		grep -q "Address already in use" "$CERT/server.log" && echo "Port $PORT is taken: another copy is running, or pass another port."
		echo "$HEAD" | head -3
		exit 1
	fi
	echo "$HEAD" | grep -qi "cross-origin-opener-policy: same-origin" || { echo "FAIL: the page is not isolated (no COOP header)"; exit 1; }
	echo "$HEAD" | grep -qi "cross-origin-embedder-policy" || { echo "FAIL: the page is not isolated (no COEP header)"; exit 1; }
	echo "ok: https://$FIRST:$PORT/ serves the game with the headers a phone needs"
	exit 0
fi

echo "On the phone (same Wi-Fi), open one of:"
echo "    https://$NAME.local:$PORT/"
for A in $ADDRESSES; do echo "    https://$A:$PORT/"; done
echo "It will warn about the certificate. Choose to visit anyway (Safari: Show Details, then visit this website)."
command -v qrencode >/dev/null 2>&1 && qrencode -t ANSIUTF8 "https://$FIRST:$PORT/"
exec "$PY" -I -B "$WEB/serve.py" --root "$WEB/Wasm" --bind 0.0.0.0 --port "$PORT" \
	--tls-cert "$CERT/cert.pem" --tls-key "$CERT/key.pem"
