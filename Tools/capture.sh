#!/bin/sh
# Launches the game, waits, takes a screenshot and quits.
#   Tools/capture.sh <name> [extra game args, e.g. -GGView=x,y,z,pitch,yaw -GGShot=yaw,loft,power -GGCaptureDelay=8]
# Output: Saved/Captures/<name>.png
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.9}"
NAME="$1"
shift
mkdir -p "$ROOT/Saved/Captures"
rm -f "$ROOT/Saved/Captures/$NAME.png"
"$UE/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" "$ROOT/GravityGolf.uproject" -game -windowed \
	-ResX=1920 -ResY=1080 -nosplash -unattended -ModelContextProtocolPort=8011 -log="Capture_$NAME.log" \
	-GGCapture="$ROOT/Saved/Captures/$NAME.png" "$@" >/dev/null 2>&1
grep -E "LogGG|Fatal|Ensure|Ray tracing" "$HOME/Library/Logs/GravityGolf/Capture_$NAME.log" | tail -12
ls -la "$ROOT/Saved/Captures/$NAME.png"
