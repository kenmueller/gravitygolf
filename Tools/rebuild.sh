#!/bin/sh
# Closes the editor, compiles the C++, and brings one editor back with MCP ready.
#   Tools/rebuild.sh only     compiles and leaves the editor closed (for a change that only the web build will test,
#                             or when the machine should stay quiet afterwards)
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.9}"
python3 "$ROOT/Tools/mcp.py" call call_tool '{"toolset_name":"EditorToolset.EditorAppToolset","tool_name":"ExitEditor","arguments":{"bDiscardUnsavedChanges":true}}' >/dev/null 2>&1
i=0
while ps -axo comm | grep -q "MacOS/UnrealEditor$" && [ $i -lt 60 ]; do sleep 2; i=$((i + 1)); done
pkill -9 -f "UnrealEditor.app/Contents/MacOS/UnrealEditor" 2>/dev/null
OUT="$("$UE/Engine/Build/BatchFiles/Mac/Build.sh" GravityGolfEditor Mac Development -project="$ROOT/GravityGolf.uproject" -waitmutex 2>&1)"
echo "$OUT" | grep -E "error|Result"
# A failed build leaves no module to load; the editor would open on a "rebuild missing modules?" dialog that blocks
# everything. Stop here instead and leave the editor closed until the code compiles.
if ! echo "$OUT" | grep -q "Result: Succeeded"; then
	echo "build failed: the editor was not restarted"
	exit 1
fi
[ "$1" = "only" ] && exit 0
"$ROOT/Tools/editor.sh" | tail -1
