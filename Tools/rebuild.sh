#!/bin/sh
# Closes the editor, compiles the C++, and brings one editor back with MCP ready.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.9}"
python3 "$ROOT/Tools/mcp.py" call call_tool '{"toolset_name":"EditorToolset.EditorAppToolset","tool_name":"ExitEditor","arguments":{"bDiscardUnsavedChanges":true}}' >/dev/null 2>&1
i=0
while ps -axo comm | grep -q "MacOS/UnrealEditor$" && [ $i -lt 60 ]; do sleep 2; i=$((i + 1)); done
pkill -9 -f "UnrealEditor.app/Contents/MacOS/UnrealEditor" 2>/dev/null
"$UE/Engine/Build/BatchFiles/Mac/Build.sh" GravityGolfEditor Mac Development -project="$ROOT/GravityGolf.uproject" -waitmutex 2>&1 | grep -E "error|Result"
"$ROOT/Tools/editor.sh" | tail -1
