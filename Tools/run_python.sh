#!/bin/sh
# Runs an editor Python script headless against this project, e.g. Tools/run_python.sh Tools/build_materials.py
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.9}"
SCRIPT="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
"$UE/Engine/Binaries/Mac/UnrealEditor-Cmd" "$ROOT/GravityGolf.uproject" \
	-run=pythonscript -script="$SCRIPT" -AllowCommandletRendering -unattended -nosplash -stdout -FullStdOutLogOutput 2>&1 |
	grep -E "GG:|Error|error:|LogPython|Traceback|  File " || true
