#!/bin/sh
# Runs the packaged web build in Chrome with the engine's harness and saves screenshots:
#   Tools/webtest.sh [boot|play|phone|workshop|workshop-phone|tour|hole-<Level> ...]     default: play phone; tour runs every hole
# Needs a packaged build (Tools/web.sh) and Google Chrome. Results land in ../gravitygolf-web/Saved/WebTest/<name>.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="${UE_ROOT:-$HOME/UE59}"
WEB="${GG_WEB_COPY:-$ROOT/../gravitygolf-web}"
PY="$UE/Engine/Binaries/ThirdParty/Python3/Mac/bin/python3"
STATUS=0
# "tour" stands for every hole: open it, picture from the tee, play its solution. A web smoke test.
TOUR="hole-Terraces hole-SaltMirror hole-Canyon hole-Lantern hole-Returning hole-Brief hole-Colonnade hole-Stones hole-Glass hole-Wires"
NAMES=""
for NAME in ${*:-play phone}; do
	[ "$NAME" = "tour" ] && NAMES="$NAMES $TOUR" || NAMES="$NAMES $NAME"
done
for NAME in $NAMES; do
	OUT="$WEB/Saved/WebTest/$NAME"
	rm -rf "$OUT"
	if [ "$NAME" = "boot" ]; then
		"$PY" "$UE/Engine/Platforms/Wasm/Build/Tools/web/webrun.py" run --stage "$WEB/Saved/Web/Wasm" --fixture boot --out "$OUT" 2>&1 | tail -1
	else
		"$PY" "$UE/Engine/Platforms/Wasm/Build/Tools/web/webrun.py" run --stage "$WEB/Saved/Web/Wasm" \
			--fixtures "$ROOT/Build/Wasm/fixtures.json" --fixture "$NAME" --out "$OUT" 2>&1 | tail -1
	fi
	[ $? -eq 0 ] || STATUS=1
	grep -h -E "Demo: |Stroke [0-9]+ finished|UE_RUN ERROR|LogGGMode: Display: Workshop: " "$OUT/console.log" 2>/dev/null | sed 's/.*Log[A-Za-z]*: //' | cut -c1-190
	ls "$OUT"/*.png 2>/dev/null | sed "s|$OUT/||" | tr '\n' ' '
	echo
done
exit $STATUS
