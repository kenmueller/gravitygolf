#!/bin/sh
# Packages the desktop game for this Mac as an app that runs without the editor:  Tools/package.sh [Development|Shipping]
# The result is Saved/Package/Mac/GravityGolf.app (or GravityGolf-Mac-Shipping.app). The editor must be closed: the
# cook loads every level with its full scans and lighting, and two copies of the project fight over the same caches.
# It takes a long time the first time (every level's shaders and Nanite data for the game's own target).
#
# The app is signed for this Mac only. Another Mac refuses to open it until it is signed with an Apple Developer ID
# and notarized, which needs Ken's Apple account: nothing here does that.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.9}"
CONFIG="${1:-Development}"
OUT="$ROOT/Saved/Package"
if pgrep -f "UnrealEditor.app/Contents/MacOS/UnrealEditor" >/dev/null; then
	echo "The editor is open. Close it first (Tools/play.sh stop, then quit), or the cook will fight it for the caches."
	exit 1
fi
# The same list of levels as the web build (Tools/web.sh), kept there.
MAPS="$(sed -n 's/^MAPS="\(.*\)"$/\1/p' "$ROOT/Tools/web.sh")"
[ -n "$MAPS" ] || { echo "Could not read the list of levels from Tools/web.sh"; exit 1; }
"$UE/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun -project="$ROOT/GravityGolf.uproject" -platform=Mac \
	-clientconfig="$CONFIG" -build -cook -map="$MAPS" -stage -pak -iostore -package -archive -archivedirectory="$OUT" \
	-unattended -utf8output -nop4 -AdditionalCookerOptions="-unattended -nopause -nosplash"
echo "Packaged: $(ls -d "$OUT"/Mac/*.app 2>/dev/null | head -1)"
du -sh "$OUT/Mac" 2>/dev/null | cut -f1
